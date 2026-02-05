#define _POSIX_C_SOURCE 199309L
#define _DEFAULT_SOURCE

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sched.h>
#include <time.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

// Конфигурация для сборки - может быть переопределена при компиляции
#ifndef BUZZER_GPIO_PATH
#define BUZZER_GPIO_PATH ""  // Пусто = автоопределение
#endif

#define SPIN_THRESHOLD_US 100       // Микросекунды перед переключением на спин-лок
#define FREQ_MIN 20                 // Минимальная частота (Гц)
#define FREQ_MAX 20000              // Максимальная частота (Гц)
#define FREQ_OPTIMAL_MIN 2000       // Оптимальный диапазон для пьезозвучателя
#define FREQ_OPTIMAL_MAX 5000
#define MAX_DURATION_MS 300000      // Максимум 5 минут
#define PWM_CARRIER_FREQ 10000      // Частота несущей для PWM (10 кГц)

volatile sig_atomic_t stop_signal = 0;

// Структура для статистики
typedef struct {
    int toggles;
    long long total_ns;
    long long min_period_ns;
    long long max_period_ns;
    int errors;
} stats_t;

stats_t stats = {0};

// Функция для получения времени в наносекундах
static inline long long get_nsecs() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

// Обработчик сигналов для graceful shutdown
void signal_handler(int sig) {
    (void)sig;
    stop_signal = 1;
}

// Измерение overhead цикла для калибровки
long long measure_overhead() {
    long long start = get_nsecs();
    for (volatile int i = 0; i < 1000; i++);
    return (get_nsecs() - start) / 1000;
}

// Поиск файла баззера в стандартных местах
const char* find_buzzer_device() {
    static char found_path[256] = {0};
    
    // Стандартные пути для OpenWRT баззеров
    const char *search_paths[] = {
        // Типовой путь для MediaTek/Ralink
        "/sys/devices/platform/1e000000.palmbus/1e000600.gpio/gpiochip0/gpio/buzzer/value",
        "/sys/devices/platform/10000000.palmbus/10000600.gpio/gpiochip0/gpio/buzzer/value",
        // Альтернативные пути
        "/sys/class/gpio/buzzer/value",
        "/sys/class/gpio/gpio0/value",
        "/sys/class/gpio/gpio1/value",
        "/sys/class/gpio/gpio2/value",
        "/sys/class/gpio/gpio3/value",
        "/sys/class/gpio/gpio4/value",
        NULL
    };
    
    for (int i = 0; search_paths[i]; i++) {
        if (access(search_paths[i], W_OK) == 0) {
            strncpy(found_path, search_paths[i], sizeof(found_path) - 1);
            return found_path;
        }
    }
    
    return NULL;
}

// Получить текущее состояние GPIO
char read_gpio_state(int fd) {
    char state = '0';
    lseek(fd, 0, SEEK_SET);
    if (read(fd, &state, 1) > 0) {
        return state;
    }
    return '0';
}

// Гибридное ожидание: sleep + spin-lock для точности
void wait_until(long long target_time) {
    long long sleep_threshold_ns = SPIN_THRESHOLD_US * 1000LL;
    long long now = get_nsecs();
    
    if (now >= target_time) return;
    
    long long remaining = target_time - now;
    
    // Если осталось много времени, спим
    if (remaining > sleep_threshold_ns) {
        long long sleep_ns = remaining - sleep_threshold_ns;
        struct timespec ts = {
            .tv_sec = sleep_ns / 1000000000LL,
            .tv_nsec = sleep_ns % 1000000000LL
        };
        nanosleep(&ts, NULL);
    }
    
    // Последние микросекунды - спин-лок для точности
    while (get_nsecs() < target_time);
}

// PWM генерация для контроля громкости (0-100%)
void generate_tone_pwm(int fd, int freq, long duration_ms, int volume) {
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;
    
    long long period_ns = 1000000000LL / freq;
    long long pulse_width_ns = (period_ns * volume) / 100;
    long long start_time = get_nsecs();
    long long end_time = start_time + (long long)duration_ms * 1000000LL;
    
    while (get_nsecs() < end_time && !stop_signal) {
        // Импульс (HIGH)
        if (write(fd, "1", 1) < 0) {
            stats.errors++;
            break;
        }
        wait_until(get_nsecs() + pulse_width_ns);
        
        // Пауза (LOW)
        if (write(fd, "0", 1) < 0) {
            stats.errors++;
            break;
        }
        long long pause_ns = period_ns - pulse_width_ns;
        wait_until(get_nsecs() + pause_ns);
        
        stats.toggles += 2;
    }
}

// Обычная генерация тона (100% мощность)
void generate_tone_normal(int fd, int freq, long duration_ms) {
    long long period_ns = 1000000000LL / freq;
    long long half_period_ns = period_ns / 2;
    long long start_time = get_nsecs();
    long long end_time = start_time + (long long)duration_ms * 1000000LL;
    long long prev_time = start_time;
    
    int state = 0;
    
    while (get_nsecs() < end_time && !stop_signal) {
        state = !state;
        const char *value = state ? "1" : "0";
        
        if (write(fd, value, 1) < 0) {
            stats.errors++;
            break;
        }
        
        long long now = get_nsecs();
        long long period = now - prev_time;
        
        if (stats.toggles > 0) {
            if (period < stats.min_period_ns) stats.min_period_ns = period;
            if (period > stats.max_period_ns) stats.max_period_ns = period;
        } else {
            stats.min_period_ns = period;
            stats.max_period_ns = period;
        }
        
        prev_time = now;
        stats.toggles++;
        
        long long next_switch = get_nsecs() + half_period_ns;
        wait_until(next_switch);
    }
    
    stats.total_ns = get_nsecs() - start_time;
}

void print_usage(const char *prog) {
    printf("Usage: %s <frequency_hz> <duration_ms> [options]\n\n", prog);
    printf("Generate a tone on OpenWRT router buzzer.\n\n");
    printf("Arguments:\n");
    printf("  <frequency_hz>      Frequency in Hertz (1-20000, 0 = silence)\n");
    printf("  <duration_ms>       Duration in milliseconds (max %d)\n", MAX_DURATION_MS);
    printf("\nOptions:\n");
    printf("  -d, --device PATH   GPIO buzzer device path (auto-detected if not set)\n");
    printf("  -v, --verbose       Enable verbose output\n");
    printf("  -vv                 Very verbose (include stats)\n");
    printf("  -p, --pwm VOLUME    PWM volume control (0-100%%, default 100)\n");
    printf("  -c, --chirp END_HZ  Frequency sweep from START to END (0-100%%)\n");
    printf("  -h, --help          Show this help message\n\n");
    printf("Examples:\n");
    printf("  %s 1000 500                    # 1 kHz tone for 500 ms\n", prog);
    printf("  %s 440 1000                    # A4 note for 1 second\n", prog);
    printf("  %s 0 2000                      # Silence for 2 seconds\n", prog);
    printf("  %s 2500 300 -p 50              # Half volume (50%% PWM)\n", prog);
    printf("  %s 1000 500 -c 2000            # Chirp from 1kHz to 2kHz\n", prog);
    printf("  %s 1000 500 -v                 # Verbose mode\n", prog);
    printf("\nNote: Requires root privileges for real-time scheduling.\n");
}

int parse_args(int argc, char *argv[], int *freq, long *duration_ms, 
               const char **device_path, int *verbose, int *pwm_volume, int *chirp_freq) {
    if (argc < 3) {
        print_usage(argv[0]);
        return 0;
    }
    
    char *endptr;
    *freq = (int)strtol(argv[1], &endptr, 10);
    if (endptr == argv[1] || *endptr != '\0') {
        fprintf(stderr, "Error: invalid frequency '%s'\n", argv[1]);
        return -1;
    }
    
    *duration_ms = strtol(argv[2], &endptr, 10);
    if (endptr == argv[2] || *endptr != '\0' || *duration_ms < 0) {
        fprintf(stderr, "Error: invalid duration '%s'\n", argv[2]);
        return -1;
    }
    
    if (*duration_ms > MAX_DURATION_MS) {
        fprintf(stderr, "Error: duration exceeds maximum (%d ms)\n", MAX_DURATION_MS);
        return -1;
    }
    
    // Парсим опции
    for (int i = 3; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-vv") == 0) {
            *verbose = 2;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            if (*verbose < 2) *verbose = 1;
        } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--device") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: -d requires an argument\n");
                return -1;
            }
            *device_path = argv[++i];
        } else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--pwm") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: -p requires an argument\n");
                return -1;
            }
            *pwm_volume = (int)strtol(argv[++i], &endptr, 10);
            if (*pwm_volume < 0 || *pwm_volume > 100) {
                fprintf(stderr, "Error: PWM volume must be 0-100\n");
                return -1;
            }
        } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--chirp") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: -c requires an argument\n");
                return -1;
            }
            *chirp_freq = (int)strtol(argv[++i], &endptr, 10);
            if (*chirp_freq < FREQ_MIN || *chirp_freq > FREQ_MAX) {
                fprintf(stderr, "Error: chirp frequency out of range\n");
                return -1;
            }
        } else {
            fprintf(stderr, "Error: unknown option '%s'\n", argv[i]);
            return -1;
        }
    }
    
    return 1;
}

// Chirp (частотная модуляция) - линейный свип от freq1 к freq2
void generate_tone_chirp(int fd, int freq1, int freq2, long duration_ms) {
    long long start_time = get_nsecs();
    long long end_time = start_time + (long long)duration_ms * 1000000LL;
    long long total_time = end_time - start_time;
    
    int state = 0;
    long long prev_switch = start_time;
    
    while (get_nsecs() < end_time && !stop_signal) {
        long long now = get_nsecs();
        long long elapsed = now - start_time;
        
        // Линейная интерполяция частоты
        double progress = (double)elapsed / total_time;
        int current_freq = freq1 + (int)((freq2 - freq1) * progress);
        
        if (current_freq < FREQ_MIN) current_freq = FREQ_MIN;
        if (current_freq > FREQ_MAX) current_freq = FREQ_MAX;
        
        long long period_ns = 1000000000LL / current_freq;
        long long half_period_ns = period_ns / 2;
        
        state = !state;
        const char *value = state ? "1" : "0";
        
        if (write(fd, value, 1) < 0) {
            stats.errors++;
            break;
        }
        
        stats.toggles++;
        
        long long next_switch = prev_switch + half_period_ns;
        wait_until(next_switch);
        prev_switch = next_switch;
    }
    
    stats.total_ns = get_nsecs() - start_time;
}

int main(int argc, char *argv[]) {
    int freq = 0;
    long duration_ms = 0;
    const char *device_path = NULL;
    int verbose = 0;
    int pwm_volume = 100;
    int chirp_freq = 0;
    
    // Парсим аргументы
    int parse_result = parse_args(argc, argv, &freq, &duration_ms, &device_path, &verbose, &pwm_volume, &chirp_freq);
    if (parse_result <= 0) {
        return parse_result == 0 ? 0 : 1;
    }
    
    // Проверяем частоту
    if (freq > 0 && (freq < FREQ_MIN || freq > FREQ_MAX)) {
        fprintf(stderr, "Error: frequency out of range [%d-%d Hz]\n", FREQ_MIN, FREQ_MAX);
        return 1;
    }
    
    if (freq > 0 && (freq < FREQ_OPTIMAL_MIN || freq > FREQ_OPTIMAL_MAX)) {
        if (verbose) {
            fprintf(stderr, "Warning: frequency %d Hz is outside optimal range [%d-%d Hz] for piezo buzzer\n",
                    freq, FREQ_OPTIMAL_MIN, FREQ_OPTIMAL_MAX);
        }
    }
    
    // Определяем путь к баззеру
    if (!device_path) {
        device_path = getenv("BUZZER_GPIO");
        if (!device_path || strlen(device_path) == 0) {
            // Если определён путь при компиляции, используем его
            if (strlen(BUZZER_GPIO_PATH) > 0) {
                device_path = BUZZER_GPIO_PATH;
            } else {
                // Иначе ищем автоматически
                device_path = find_buzzer_device();
                if (!device_path) {
                    fprintf(stderr, "Error: buzzer device not found\n");
                    fprintf(stderr, "Please specify with -d option or BUZZER_GPIO environment variable\n");
                    return 1;
                }
                if (verbose) {
                    fprintf(stderr, "Auto-detected buzzer at: %s\n", device_path);
                }
            }
        }
    }
    
    if (verbose) {
        fprintf(stderr, "Device: %s\n", device_path);
        fprintf(stderr, "Frequency: %d Hz\n", freq);
        fprintf(stderr, "Duration: %ld ms\n", duration_ms);
        if (pwm_volume < 100) {
            fprintf(stderr, "PWM Volume: %d%%\n", pwm_volume);
        }
        if (chirp_freq > 0) {
            fprintf(stderr, "Chirp target: %d Hz\n", chirp_freq);
        }
    }
    
    // Проверяем доступность файла перед использованием
    if (access(device_path, W_OK) != 0) {
        perror("Error: device not accessible");
        fprintf(stderr, "Cannot write to: %s\n", device_path);
        return 1;
    }
    
    // Регистрируем обработчики сигналов для graceful shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Открываем GPIO файл
    int fd = open(device_path, O_WRONLY);
    if (fd < 0) {
        perror("Error opening device");
        return 1;
    }
    
    // Сохраняем исходное состояние GPIO
    char original_state = read_gpio_state(fd);
    if (verbose) {
        fprintf(stderr, "Original GPIO state: %c\n", original_state);
    }
    
    // Устанавливаем Real-Time приоритет
    struct sched_param param;
    param.sched_priority = 50;
    if (sched_setscheduler(0, SCHED_FIFO, &param) < 0) {
        if (verbose) {
            perror("Warning: failed to set real-time scheduling");
        }
    }
    
    int ret = 0;
    
    if (freq <= 0) {
        // Молчание (пауза)
        if (verbose) fprintf(stderr, "Silence for %ld ms\n", duration_ms);
        usleep(duration_ms * 1000);
    } else {
        if (verbose) {
            long long overhead = measure_overhead();
            fprintf(stderr, "Loop overhead: ~%lld ns\n", overhead);
            fprintf(stderr, "Starting tone generation...\n");
        }
        
        // Выбираем режим генерации
        if (chirp_freq > 0) {
            // Chirp режим
            generate_tone_chirp(fd, freq, chirp_freq, duration_ms);
        } else if (pwm_volume < 100) {
            // PWM режим
            generate_tone_pwm(fd, freq, duration_ms, pwm_volume);
        } else {
            // Нормальный режим
            generate_tone_normal(fd, freq, duration_ms);
        }
    }
    
    // Гарантированно выключаем баззер
    if (write(fd, "0", 1) < 0) {
        perror("Warning: failed to turn off buzzer");
    }
    
    // Восстанавливаем исходное состояние GPIO
    if (original_state != '0') {
        if (write(fd, &original_state, 1) < 0 && verbose) {
            perror("Warning: failed to restore GPIO state");
        }
    }
    
    // Выводим статистику если включен verbose
    if (verbose >= 2 && freq > 0) {
        fprintf(stderr, "\n--- Statistics ---\n");
        fprintf(stderr, "Total toggles: %d\n", stats.toggles);
        fprintf(stderr, "Total time: %lld ns (%.3f ms)\n", stats.total_ns, stats.total_ns / 1e6);
        if (stats.toggles > 1) {
            fprintf(stderr, "Min period: %lld ns\n", stats.min_period_ns);
            fprintf(stderr, "Max period: %lld ns\n", stats.max_period_ns);
            fprintf(stderr, "Jitter: %lld ns\n", stats.max_period_ns - stats.min_period_ns);
        }
        if (stats.errors > 0) {
            fprintf(stderr, "Errors: %d\n", stats.errors);
        }
    }
    
    // Закрываем файл
    if (close(fd) < 0) {
        perror("Error closing device");
        ret = 1;
    }
    
    if (verbose && stop_signal) {
        fprintf(stderr, "Interrupted by signal\n");
    }
    
    return ret;
}