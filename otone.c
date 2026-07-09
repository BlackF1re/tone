#define _POSIX_C_SOURCE 199309L
#define _DEFAULT_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <sched.h>
#ifndef BUZZER_GPIO_PATH
#define BUZZER_GPIO_PATH ""
#endif
#define APP_NAME "otone"
#define CONFIG_FILE "/etc/otone.conf"
#define LOCAL_CONFIG_FILE "./otone.conf"
#define FREQ_MIN 20
#define FREQ_MAX 20000
#define MAX_DURATION_MS 300000
static volatile sig_atomic_t stop_signal = 0;
struct config {
    char buzzer_path[256];
    int spin_threshold_us;
    int default_volume;
    int enabled;
};
static struct config default_config = {
    .buzzer_path = "",
    .spin_threshold_us = 100,
    .default_volume = 100,
    .enabled = 1,
};
static void on_signal(int sig) {
    (void)sig;
    stop_signal = 1;
}
static long long now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000000000LL + ts.tv_nsec;
}
static void sleep_until(long long target_ns, int spin_threshold_us) {
    long long threshold_ns = (long long)spin_threshold_us * 1000LL;
    long long n = now_ns();
    if (n >= target_ns) return;
    long long left = target_ns - n;
    if (left > threshold_ns) {
        long long sleep_ns = left - threshold_ns;
        struct timespec ts;
        ts.tv_sec = sleep_ns / 1000000000LL;
        ts.tv_nsec = sleep_ns % 1000000000LL;
        nanosleep(&ts, NULL);
    }
    while (now_ns() < target_ns) {
    }
}
static void log_msg(const char *level, const char *fmt, ...) {
    va_list args;
    fprintf(stderr, "[%s] ", level);
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
}
static void trim(char *s) {
    char *p = s;
    while (*p == ' ' || *p == '\t') p++;
    if (p != s) memmove(s, p, strlen(p) + 1);
    size_t n = strlen(s);
    while (n > 0 && (s[n - 1] == ' ' || s[n - 1] == '\t' || s[n - 1] == '\n' || s[n - 1] == '\r')) {
        s[--n] = '\0';
    }
}
static void load_config(struct config *cfg) {
    *cfg = default_config;
    FILE *f = fopen(CONFIG_FILE, "r");
    if (!f) f = fopen(LOCAL_CONFIG_FILE, "r");
    if (!f) return;
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        char *comment = strchr(line, '#');
        if (comment) *comment = '\0';
        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        char *key = line;
        char *value = eq + 1;
        trim(key);
        trim(value);
        if (strcmp(key, "buzzer_path") == 0) {
            strncpy(cfg->buzzer_path, value, sizeof(cfg->buzzer_path) - 1);
        } else if (strcmp(key, "spin_threshold_us") == 0) {
            cfg->spin_threshold_us = atoi(value);
        } else if (strcmp(key, "default_volume") == 0) {
            cfg->default_volume = atoi(value);
        } else if (strcmp(key, "enabled") == 0) {
            cfg->enabled = atoi(value);
        }
    }
    fclose(f);
}
static void save_config(const struct config *cfg) {
    const char *path = CONFIG_FILE;
    FILE *f = fopen(path, "w");
    if (!f) {
        path = LOCAL_CONFIG_FILE;
        f = fopen(path, "w");
    }
    if (!f) return;
    fprintf(f, "# otone configuration\n");
    fprintf(f, "buzzer_path=%s\n", cfg->buzzer_path);
    fprintf(f, "spin_threshold_us=%d\n", cfg->spin_threshold_us);
    fprintf(f, "default_volume=%d\n", cfg->default_volume);
    fprintf(f, "enabled=%d\n", cfg->enabled);
    fclose(f);
}
static const char *find_by_label(void) {
    static char path[256];
    DIR *dir = opendir("/sys/class/gpio");
    if (!dir) return NULL;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "gpio", 4) != 0) continue;
        if (strstr(entry->d_name, "gpiochip")) continue;
        char label_path[256];
        snprintf(label_path, sizeof(label_path), "/sys/class/gpio/%s/label", entry->d_name);
        FILE *f = fopen(label_path, "r");
        if (!f) continue;
        char label[128];
        int matched = 0;
        if (fgets(label, sizeof(label), f)) {
            matched = strstr(label, "buzzer") || strstr(label, "beep") || strstr(label, "speaker");
        }
        fclose(f);
        if (matched) {
            snprintf(path, sizeof(path), "/sys/class/gpio/%s/value", entry->d_name);
            if (access(path, W_OK) == 0) {
                closedir(dir);
                return path;
            }
        }
    }
    closedir(dir);
    return NULL;
}
static const char *find_by_scan(void) {
    static char path[256];
    DIR *dir = opendir("/sys/class/gpio");
    if (!dir) return NULL;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "gpio", 4) != 0) continue;
        if (strstr(entry->d_name, "gpiochip")) continue;
        snprintf(path, sizeof(path), "/sys/class/gpio/%s/value", entry->d_name);
        if (access(path, W_OK) == 0) {
            closedir(dir);
            return path;
        }
    }
    closedir(dir);
    return NULL;
}
static const char *find_buzzer(struct config *cfg) {
    static const char *known[] = {
        BUZZER_GPIO_PATH,
        "/sys/class/gpio/buzzer/value",
        "/sys/devices/platform/1e000000.palmbus/1e000600.gpio/gpiochip0/gpio/buzzer/value",
        "/sys/devices/platform/10000000.palmbus/10000600.gpio/gpiochip0/gpio/buzzer/value",
        NULL,
    };
    if (cfg->buzzer_path[0] && access(cfg->buzzer_path, W_OK) == 0) return cfg->buzzer_path;
    for (int i = 0; known[i]; i++) {
        if (known[i][0] && access(known[i], W_OK) == 0) return known[i];
    }
    const char *path = find_by_label();
    if (path) return path;
    return find_by_scan();
}
static void usage(const char *prog) {
    printf("Usage: %s <frequency_hz> <duration_ms> [options]\n\n", prog);
    printf("Generate sound on an OpenWrt router buzzer through GPIO sysfs.\n\n");
    printf("Arguments:\n");
    printf("  frequency_hz        20-20000 Hz, 0 means silence\n");
    printf("  duration_ms         Duration in milliseconds, max %d\n\n", MAX_DURATION_MS);
    printf("Options:\n");
    printf("  -d, --device PATH   GPIO value path\n");
    printf("  -p, --pwm VALUE     Volume 0-100, default from config\n");
    printf("  -c, --chirp HZ      Sweep from start frequency to target\n");
    printf("  -v, --verbose       Verbose output\n");
    printf("  -vv                 Verbose output with statistics\n");
    printf("  -h, --help          Show help\n\n");
    printf("Examples:\n");
    printf("  %s 2000 500\n", prog);
    printf("  %s 1000 700 -c 4000\n", prog);
    printf("  %s 2500 300 -p 50\n", prog);
}
static int parse_int(const char *s, int *out) {
    char *end = NULL;
    long v = strtol(s, &end, 10);
    if (!s[0] || *end != '\0') return 0;
    *out = (int)v;
    return 1;
}
static int parse_long(const char *s, long *out) {
    char *end = NULL;
    long v = strtol(s, &end, 10);
    if (!s[0] || *end != '\0') return 0;
    *out = v;
    return 1;
}
static void write_gpio(int fd, const char *value, int *errors) {
    if (write(fd, value, 1) < 0) (*errors)++;
}
static void play_fixed(int fd, int freq, long duration_ms, int volume, int spin_us, int *toggles, int *errors) {
    if (freq <= 0) {
        usleep((useconds_t)duration_ms * 1000U);
        return;
    }
    long long period_ns = 1000000000LL / freq;
    long long high_ns = period_ns / 2;
    long long low_ns = period_ns / 2;
    if (volume >= 0 && volume < 100) {
        high_ns = (period_ns * volume) / 100;
        low_ns = period_ns - high_ns;
    }
    if (high_ns < 1000) high_ns = 1000;
    if (low_ns < 1000) low_ns = 1000;
    long long end = now_ns() + (long long)duration_ms * 1000000LL;
    long long next = now_ns();
    while (!stop_signal && now_ns() < end) {
        write_gpio(fd, "1", errors);
        (*toggles)++;
        next += high_ns;
        sleep_until(next, spin_us);
        write_gpio(fd, "0", errors);
        (*toggles)++;
        next += low_ns;
        sleep_until(next, spin_us);
    }
}
static void play_chirp(int fd, int start_freq, int end_freq, long duration_ms, int spin_us, int *toggles, int *errors) {
    long long start = now_ns();
    long long end = start + (long long)duration_ms * 1000000LL;
    long long next = start;
    int state = 0;
    while (!stop_signal && now_ns() < end) {
        long long current = now_ns();
        double progress = (double)(current - start) / (double)(end - start);
        if (progress < 0) progress = 0;
        if (progress > 1) progress = 1;
        int freq = start_freq + (int)((end_freq - start_freq) * progress);
        if (freq < FREQ_MIN) freq = FREQ_MIN;
        if (freq > FREQ_MAX) freq = FREQ_MAX;
        long long half = 500000000LL / freq;
        state = !state;
        write_gpio(fd, state ? "1" : "0", errors);
        (*toggles)++;
        next += half;
        sleep_until(next, spin_us);
    }
}
int main(int argc, char **argv) {
    if (argc == 1 || (argc == 2 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")))) {
        usage(argv[0]);
        return 0;
    }
    if (argc < 3) {
        usage(argv[0]);
        return 1;
    }
    int freq = 0;
    long duration_ms = 0;
    if (!parse_int(argv[1], &freq) || !parse_long(argv[2], &duration_ms) || duration_ms < 0 || duration_ms > MAX_DURATION_MS) {
        fprintf(stderr, "Error: invalid frequency or duration\n");
        return 1;
    }
    if (freq > 0 && (freq < FREQ_MIN || freq > FREQ_MAX)) {
        fprintf(stderr, "Error: frequency must be 0 or %d-%d Hz\n", FREQ_MIN, FREQ_MAX);
        return 1;
    }
    struct config cfg;
    load_config(&cfg);
    if (!cfg.enabled) return 0;
    const char *device = NULL;
    int volume = cfg.default_volume;
    int chirp = 0;
    int verbose = 0;
    for (int i = 3; i < argc; i++) {
        if (!strcmp(argv[i], "-d") || !strcmp(argv[i], "--device")) {
            if (++i >= argc) { fprintf(stderr, "Error: missing device path\n"); return 1; }
            device = argv[i];
        } else if (!strcmp(argv[i], "-p") || !strcmp(argv[i], "--pwm")) {
            if (++i >= argc || !parse_int(argv[i], &volume) || volume < 0 || volume > 100) {
                fprintf(stderr, "Error: PWM volume must be 0-100\n");
                return 1;
            }
        } else if (!strcmp(argv[i], "-c") || !strcmp(argv[i], "--chirp")) {
            if (++i >= argc || !parse_int(argv[i], &chirp) || chirp < FREQ_MIN || chirp > FREQ_MAX) {
                fprintf(stderr, "Error: invalid chirp target\n");
                return 1;
            }
        } else if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--verbose")) {
            if (verbose < 1) verbose = 1;
        } else if (!strcmp(argv[i], "-vv")) {
            verbose = 2;
        } else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            usage(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "Error: unknown option: %s\n", argv[i]);
            return 1;
        }
    }
    if (!device) device = find_buzzer(&cfg);
    if (!device) {
        log_msg("ERROR", "%s: buzzer device not found", APP_NAME);
        fprintf(stderr, "Error: buzzer device not found. Use -d /sys/class/gpio/.../value\n");
        return 1;
    }
    if (access(device, W_OK) != 0) {
        perror("Error: device not writable");
        return 1;
    }
    strncpy(cfg.buzzer_path, device, sizeof(cfg.buzzer_path) - 1);
    save_config(&cfg);
    if (verbose) {
        fprintf(stderr, "Device: %s\n", device);
        fprintf(stderr, "Frequency: %d Hz\n", freq);
        fprintf(stderr, "Duration: %ld ms\n", duration_ms);
        fprintf(stderr, "Volume: %d%%\n", volume);
        if (chirp) fprintf(stderr, "Chirp target: %d Hz\n", chirp);
    }
    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);
    int fd = open(device, O_WRONLY);
    if (fd < 0) {
        perror("Error opening device");
        return 1;
    }
    struct sched_param param;
    param.sched_priority = 50;
    sched_setscheduler(0, SCHED_FIFO, &param);
    int toggles = 0;
    int errors = 0;
    long long start = now_ns();
    if (chirp > 0 && freq > 0) {
        play_chirp(fd, freq, chirp, duration_ms, cfg.spin_threshold_us, &toggles, &errors);
    } else {
        play_fixed(fd, freq, duration_ms, volume, cfg.spin_threshold_us, &toggles, &errors);
    }
    write(fd, "0", 1);
    close(fd);
    if (verbose >= 2) {
        long long elapsed = now_ns() - start;
        fprintf(stderr, "Toggles: %d\n", toggles);
        fprintf(stderr, "Elapsed: %.3f ms\n", elapsed / 1000000.0);
        fprintf(stderr, "Errors: %d\n", errors);
    }
    return errors ? 1 : 0;
}
