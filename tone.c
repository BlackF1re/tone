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

#define DEFAULT_BUZZER_PATH "/sys/devices/platform/1e000000.palmbus/1e000600.gpio/gpiochip0/gpio/buzzer/value"
#define SPIN_THRESHOLD_US 100    // Микросекунды перед переключением на спин-лок
#define FREQ_MIN 20              // Минимальная частота (Гц)
#define FREQ_MAX 20000           // Максимальная частота (Гц)
#define FREQ_OPTIMAL_MIN 2000    // Оптимальный диапазон для пьезозвучателя
#define FREQ_OPTIMAL_MAX 5000

volatile sig_atomic_t stop_signal = 0;

// Функция для получения времени в наносекундах
static inline long long get_nsecs() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

// Обработчик сигналов для graceful shutdown
void signal_handler(int sig) {
    (void)sig;  // Подавляем warning неиспользуемого параметра
    stop_signal = 1;
}

// Измерение overhead цикла для калибровки
long long measure_overhead() {
    long long start = get_nsecs();
    for (volatile int i = 0; i < 1000; i++);
    return (get_nsecs() - start) / 1000;
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

void print_usage(const char *prog) {
    printf("Usage: %s <frequency_hz> <duration_ms> [options]\n\n", prog);
    printf("Generate a tone on OpenWRT router buzzer.\n\n");
    printf("Arguments:\n");
    printf("  <frequency_hz>      Frequency in Hertz (1-20000, 0 = silence)\n");
    printf("  <duration_ms>       Duration in milliseconds\n\n");
    printf("Options:\n");
    printf("  -d, --device PATH   GPIO buzzer device path\n");
    printf("                      (default: %s)\n", DEFAULT_BUZZER_PATH);
    printf("  -v, --verbose       Enable verbose output\n");
    printf("  -h, --help          Show this help message\n\n");
    printf("Examples:\n");
    printf("  %s 1000 500                    # 1 kHz tone for 500 ms\n", prog);
    printf("  %s 440 1000                    # A4 note for 1 second\n", prog);
    printf("  %s 0 2000                      # Silence for 2 seconds\n", prog);
    printf("  %s 2000 300 -d /sys/class/gpio/gpio10/value\n", prog);
    printf("\nNote: Requires root privileges for real-time scheduling.\n");
}

int parse_args(int argc, char *argv[], int *freq, long *duration_ms, 
               const char **device_path, int *verbose) {
    if (argc < 3) {
        print_usage(argv[0]);
        return 0;
    }
    
    // Парсим frequency
    char *endptr;
    *freq = (int)strtol(argv[1], &endptr, 10);
    if (endptr == argv[1] || *endptr != '\0') {
        fprintf(stderr, "Error: invalid frequency '%s'\n", argv[1]);
        return -1;
    }
    
    // Парсим duration
    *duration_ms = strtol(argv[2], &endptr, 10);
    if (endptr == argv[2] || *endptr != '\0' || *duration_ms < 0) {
        fprintf(stderr, "Error: invalid duration '%s'\n", argv[2]);
        return -1;
    }
    
    // Парсим опции
    for (int i = 3; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            *verbose = 1;
        } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--device") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: -d requires an argument\n");
                return -1;
            }
            *device_path = argv[++i];
        } else {
            fprintf(stderr, "Error: unknown option '%s'\n", argv[i]);
            return -1;
        }
    }
    
    return 1;
}

int main(int argc, char *argv[]) {
    int freq = 0;
    long duration_ms = 0;
    const char *device_path = NULL;
    int verbose = 0;
    
    // Парсим аргументы
    int parse_result = parse_args(argc, argv, &freq, &duration_ms, &device_path, &verbose);
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
    
    // Используем переменную окружения, если не указан параметр
    if (!device_path) {
        device_path = getenv("BUZZER_GPIO");
        if (!device_path) {
            device_path = DEFAULT_BUZZER_PATH;
        }
    }
    
    if (verbose) {
        fprintf(stderr, "Device: %s\n", device_path);
        fprintf(stderr, "Frequency: %d Hz\n", freq);
        fprintf(stderr, "Duration: %ld ms\n", duration_ms);
    }
    
    // Регистрируем обработчики сигналов для graceful shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Открываем GPIO файл
    int fd = open(device_path, O_WRONLY);
    if (fd < 0) {
        perror("Error opening device");
        fprintf(stderr, "Failed to open: %s\n", device_path);
        return 1;
    }
    
    // Устанавливаем Real-Time приоритет
    struct sched_param param;
    param.sched_priority = 50; // Высокий приоритет, но не максимум
    if (sched_setscheduler(0, SCHED_FIFO, &param) < 0) {
        if (verbose) {
            perror("Warning: failed to set real-time scheduling");
            fprintf(stderr, "Continuing with normal scheduling (may affect audio quality)\n");
        }
    }
    
    int ret = 0;
    
    if (freq <= 0) {
        // Молчание (пауза)
        if (verbose) fprintf(stderr, "Silence for %ld ms\n", duration_ms);
        usleep(duration_ms * 1000);
    } else {
        // Генерируем звук
        long long period_ns = 1000000000LL / freq;
        long long half_period_ns = period_ns / 2;
        long long start_time = get_nsecs();
        long long end_time = start_time + (long long)duration_ms * 1000000LL;
        
        if (verbose) {
            long long overhead = measure_overhead();
            fprintf(stderr, "Loop overhead: ~%lld ns\n", overhead);
            fprintf(stderr, "Starting tone generation...\n");
        }
        
        int state = 0; // Текущее состояние GPIO (0 или 1)
        
        while (get_nsecs() < end_time && !stop_signal) {
            // Переключаем состояние
            state = !state;
            const char *value = state ? "1" : "0";
            
            ssize_t written = write(fd, value, 1);
            if (written < 0) {
                perror("Error writing to device");
                ret = 1;
                break;
            }
            
            // Гибридное ожидание до следующего переключения
            long long next_switch = get_nsecs() + half_period_ns;
            wait_until(next_switch);
        }
        
        // Гарантированно выключаем баззер
        if (write(fd, "0", 1) < 0) {
            perror("Warning: failed to turn off buzzer");
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