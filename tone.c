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
#include <dirent.h>
#include <stdarg.h>

// Log levels для простого логирования без syslog.h
#define LOG_INFO "INFO"
#define LOG_WARN "WARN"
#define LOG_ERR "ERROR"

// Конфигурация для сборки - может быть переопределена при компиляции
#ifndef BUZZER_GPIO_PATH
#define BUZZER_GPIO_PATH ""  // Пусто = автоопределение
#endif

#define CONFIG_FILE "/etc/tone.conf"
#define LOG_TAG "tone"

#define SPIN_THRESHOLD_US 100       // Микросекунды перед переключением на спин-лок
#define FREQ_MIN 20                 // Минимальная частота (Гц)
#define FREQ_MAX 20000              // Максимальная частота (Гц)
#define FREQ_OPTIMAL_MIN 2000       // Оптимальный диапазон для пьезозвучателя
#define FREQ_OPTIMAL_MAX 5000
#define MAX_DURATION_MS 300000      // Максимум 5 минут
#define PWM_CARRIER_FREQ 10000      // Частота несущей для PWM (10 кГц)

// Структура конфигурации
typedef struct {
    char buzzer_path[256];
    int spin_threshold_us;
    int pwm_carrier_freq;
    int default_volume;
    int enabled;
} tone_config_t;

// Конфигурация по умолчанию
tone_config_t default_config = {
    .buzzer_path = "",
    .spin_threshold_us = 100,
    .pwm_carrier_freq = 10000,
    .default_volume = 100,
    .enabled = 1
};

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

// Логирование в syslog
void log_message(const char *level, const char *format, ...) {
    va_list args;
    va_start(args, format);
    
    // Выводим в stderr
    fprintf(stderr, "[%s] ", level);
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    
    // Логируем в syslog через logger
    char cmd[512];
    va_list args2;
    va_copy(args2, args);
    vsnprintf(cmd, sizeof(cmd), format, args2);
    va_end(args2);
    
    char logger_cmd[768];
    snprintf(logger_cmd, sizeof(logger_cmd), "logger -t %s -p daemon.%s \"%s\"", 
             LOG_TAG, strcmp(level, "ERROR") == 0 ? "err" : 
                      strcmp(level, "WARN") == 0 ? "warning" : "info", cmd);
    system(logger_cmd);
    
    va_end(args);
}

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

// Чтение конфигурации из файла
void load_config(tone_config_t *config) {
    *config = default_config;
    
    // Пытаемся читать из /etc/tone.conf, потом из ./tone.conf
    FILE *f = fopen(CONFIG_FILE, "r");
    if (!f) {
        f = fopen("./tone.conf", "r");
        if (!f) {
            return;  // Ни один конфиг не найден - используем значения по умолчанию
        }
    }
    
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        // Удаляем комментарии и пробелы
        char *comment = strchr(line, '#');
        if (comment) *comment = '\0';
        
        // Парсим переменную=значение
        char *eq = strchr(line, '=');
        if (!eq) continue;
        
        *eq = '\0';
        char *key = line;
        char *value = eq + 1;
        
        // Удаляем пробелы
        while (*key && *key == ' ') key++;
        while (*value && (*value == ' ' || *value == '\n')) value++;
        char *end = value + strlen(value) - 1;
        while (end >= value && (*end == ' ' || *end == '\n')) *end-- = '\0';
        
        // Парсим параметры
        if (strcmp(key, "buzzer_path") == 0) {
            strncpy(config->buzzer_path, value, sizeof(config->buzzer_path) - 1);
        } else if (strcmp(key, "spin_threshold_us") == 0) {
            config->spin_threshold_us = atoi(value);
        } else if (strcmp(key, "pwm_carrier_freq") == 0) {
            config->pwm_carrier_freq = atoi(value);
        } else if (strcmp(key, "default_volume") == 0) {
            config->default_volume = atoi(value);
        } else if (strcmp(key, "enabled") == 0) {
            config->enabled = atoi(value);
        }
    }
    
    fclose(f);
}

// Сохранение конфигурации в файл
void save_config(const tone_config_t *config) {
    const char *config_path = CONFIG_FILE;
    FILE *f = fopen(config_path, "w");
    
    // Если /etc/tone.conf не доступен, пытаемся создать в текущей директории
    if (!f) {
        config_path = "./tone.conf";
        f = fopen(config_path, "w");
        if (!f) {
            log_message("WARN", "Cannot write config to %s or ./tone.conf", CONFIG_FILE);
            return;
        }
        log_message("WARN", "/etc/ not accessible, using local config: %s", config_path);
    }
    
    fprintf(f, "# OpenWRT Tone Generator Configuration\n");
    fprintf(f, "# Auto-generated config file\n\n");
    fprintf(f, "buzzer_path=%s\n", config->buzzer_path);
    fprintf(f, "spin_threshold_us=%d\n", config->spin_threshold_us);
    fprintf(f, "pwm_carrier_freq=%d\n", config->pwm_carrier_freq);
    fprintf(f, "default_volume=%d\n", config->default_volume);
    fprintf(f, "enabled=%d\n", config->enabled);
    
    fclose(f);
    log_message("INFO", "Configuration saved to %s", config_path);
}

// Поиск баззера по GPIO labels
const char* find_buzzer_by_label() {
    static char found_path[256] = {0};
    DIR *gpio_dir;
    struct dirent *entry;
    
    gpio_dir = opendir("/sys/class/gpio");
    if (!gpio_dir) return NULL;
    
    while ((entry = readdir(gpio_dir)) != NULL) {
        if (strncmp(entry->d_name, "gpio", 4) != 0) continue;
        
        // Проверяем label файл
        char label_path[256];
        snprintf(label_path, sizeof(label_path), "/sys/class/gpio/%s/label", entry->d_name);
        
        FILE *f = fopen(label_path, "r");
        if (!f) continue;
        
        char label[64];
        if (fgets(label, sizeof(label), f)) {
            // Удаляем перевод строки
            label[strcspn(label, "\n")] = '\0';
            
            if (strstr(label, "buzzer") || strstr(label, "beep") || strstr(label, "speaker")) {
                // Нашли! Проверяем наличие value файла
                snprintf(found_path, sizeof(found_path), "/sys/class/gpio/%s/value", entry->d_name);
                if (access(found_path, W_OK) == 0) {
                    fclose(f);
                    closedir(gpio_dir);
                    return found_path;
                }
            }
        }
        fclose(f);
    }
    
    closedir(gpio_dir);
    return NULL;
}

// Динамический поиск баззера в /sys/class/gpio/
const char* find_buzzer_by_scan() {
    static char found_path[256] = {0};
    DIR *gpio_dir;
    struct dirent *entry;
    
    gpio_dir = opendir("/sys/class/gpio");
    if (!gpio_dir) return NULL;
    
    // Ищем файлы gpio*/value и пробуем первый найденный
    while ((entry = readdir(gpio_dir)) != NULL) {
        if (strncmp(entry->d_name, "gpio", 4) != 0) continue;
        
        snprintf(found_path, sizeof(found_path), "/sys/class/gpio/%s/value", entry->d_name);
        
        // Пробуем записать - если можно, то это потенциально баззер
        if (access(found_path, W_OK) == 0) {
            // Проверяем что это не "gpiochip"
            if (strchr(entry->d_name + 4, 'c') == NULL) {  // Исключаем gpiochipXX
                closedir(gpio_dir);
                return found_path;
            }
        }
    }
    
    closedir(gpio_dir);
    return NULL;
}

// Поиск в device-tree
const char* find_buzzer_in_device_tree() {
    DIR *dt_dir;
    struct dirent *entry;
    
    dt_dir = opendir("/proc/device-tree");
    if (!dt_dir) return NULL;
    
    // Ищем папки с "buzzer" или "beeper" в названии
    while ((entry = readdir(dt_dir)) != NULL) {
        if (strstr(entry->d_name, "buzzer") || strstr(entry->d_name, "beeper")) {
            // Пробуем найти соответствующий GPIO в /sys/class/gpio/
            // Это упрощённый поиск - на реальных системах нужнопарсить device tree binary
            closedir(dt_dir);
            
            // Возвращаемся к обычному поиску, но знаем что баззер есть
            return find_buzzer_by_scan();
        }
    }
    
    closedir(dt_dir);
    return NULL;
}

// Главная функция поиска баззера
const char* find_buzzer_device(const tone_config_t *config) {
    static char found_path[256] = {0};
    
    // 1. Сначала ищем в конфиге
    if (strlen(config->buzzer_path) > 0 && access(config->buzzer_path, W_OK) == 0) {
        return config->buzzer_path;
    }
    
    // 2. Hardcoded пути (быстро)
    const char *search_paths[] = {
        "/sys/devices/platform/1e000000.palmbus/1e000600.gpio/gpiochip0/gpio/buzzer/value",
        "/sys/devices/platform/10000000.palmbus/10000600.gpio/gpiochip0/gpio/buzzer/value",
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
    
    // 3. Поиск по labels
    const char *label_path = find_buzzer_by_label();
    if (label_path) return label_path;
    
    // 4. Динамический поиск в /sys/class/gpio/
    const char *scan_path = find_buzzer_by_scan();
    if (scan_path) return scan_path;
    
    // 5. Поиск в device-tree
    const char *dt_path = find_buzzer_in_device_tree();
    if (dt_path) return dt_path;
    
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
void wait_until(long long target_time, int spin_threshold_us) {
    long long sleep_threshold_ns = (long long)spin_threshold_us * 1000LL;
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
void generate_tone_pwm(int fd, int freq, long duration_ms, int volume, int spin_threshold_us) {
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
        wait_until(get_nsecs() + pulse_width_ns, spin_threshold_us);
        
        // Пауза (LOW)
        if (write(fd, "0", 1) < 0) {
            stats.errors++;
            break;
        }
        long long pause_ns = period_ns - pulse_width_ns;
        wait_until(get_nsecs() + pause_ns, spin_threshold_us);
        
        stats.toggles += 2;
    }
}

// Обычная генерация тона (100% мощность)
void generate_tone_normal(int fd, int freq, long duration_ms, int spin_threshold_us) {
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
        wait_until(next_switch, spin_threshold_us);
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
    printf("\nNote: Optimal frequency range for piezo buzzers is typically 2-5 kHz.\n");
    printf("Config file location: /etc/tone.conf\n");

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
void generate_tone_chirp(int fd, int freq1, int freq2, long duration_ms, int spin_threshold_us) {
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
        wait_until(next_switch, spin_threshold_us);
        prev_switch = next_switch;
    }
    
    stats.total_ns = get_nsecs() - start_time;
}

int main(int argc, char *argv[]) {
    int freq = 0;
    long duration_ms = 0;
    const char *device_path = NULL;
    int verbose = 0;
    int pwm_volume = -1;  // -1 = use config default
    int chirp_freq = 0;
    
    // Загружаем конфиг
    tone_config_t config;
    load_config(&config);
    
    if (config.enabled == 0) {
        log_message(LOG_INFO, "tone: disabled in config");
        return 0;
    }
    
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
    
    // Если PWM volume не указан в командной строке, используем значение из конфига
    if (pwm_volume < 0) {
        pwm_volume = config.default_volume;
    }
    
    // Определяем путь к баззеру
    if (!device_path) {
        // 1. Сначала проверяем конфиг файл
        if (strlen(config.buzzer_path) > 0) {
            device_path = config.buzzer_path;
            if (verbose) {
                fprintf(stderr, "Using buzzer path from config: %s\n", device_path);
            }
        } else {
            // 2. Если в конфиге нет пути, ищем автоматически
            device_path = find_buzzer_device(&config);
            if (!device_path) {
                log_message(LOG_ERR, "tone: buzzer device not found");
                fprintf(stderr, "Error: buzzer device not found\n");
                fprintf(stderr, "Please specify with -d option or BUZZER_GPIO environment variable\n");
                return 1;
            }
            if (verbose) {
                fprintf(stderr, "Auto-detected buzzer at: %s\n", device_path);
            }
            // Сохраняем найденный путь в конфиг для следующего запуска
            strncpy(config.buzzer_path, device_path, sizeof(config.buzzer_path) - 1);
            save_config(&config);
            if (verbose) {
                fprintf(stderr, "Saved buzzer path to config: %s\n", CONFIG_FILE);
            }
        }
    } else {
        // Если путь указан в командной строке, обновляем конфиг
        strncpy(config.buzzer_path, device_path, sizeof(config.buzzer_path) - 1);
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
        
        // Выбираем режим генерации, передаём spin_threshold из конфига
        if (chirp_freq > 0) {
            // Chirp режим
            generate_tone_chirp(fd, freq, chirp_freq, duration_ms, config.spin_threshold_us);
        } else if (pwm_volume < 100) {
            // PWM режим
            generate_tone_pwm(fd, freq, duration_ms, pwm_volume, config.spin_threshold_us);
        } else {
            // Нормальный режим
            generate_tone_normal(fd, freq, duration_ms, config.spin_threshold_us);
        }
    }
    
    // Гарантированно выключаем баззер
    if (write(fd, "0", 1) < 0) {
        log_message(LOG_ERR, "tone: failed to turn off buzzer");
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