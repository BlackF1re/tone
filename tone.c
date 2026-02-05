#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sched.h>
#include <time.h>

// Функция для получения времени в наносекундах
long long get_nsecs() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

int main(int argc, char *argv[]) {
    if (argc < 3) return 1;

    int freq = atoi(argv[1]);
    long duration_ms = atol(argv[2]);

    // 1. Повышаем приоритет до Real-Time (FIFO)
    struct sched_param param;
    param.sched_priority = 99; // Максимальный
    sched_setscheduler(0, SCHED_FIFO, &param);

    int fd = open("/sys/devices/platform/1e000000.palmbus/1e000600.gpio/gpiochip0/gpio/buzzer/value", O_WRONLY);
    if (fd < 0) return 1;

    if (freq <= 0) {
        usleep(duration_ms * 1000);
    } else {
        long long period_ns = 1000000000LL / freq;
        long long half_period_ns = period_ns / 2;
        long long start_time = get_nsecs();
        long long end_time = start_time + (long long)duration_ms * 1000000LL;

        while (get_nsecs() < end_time) {
            // Устанавливаем 1
            write(fd, "1", 1);
            long long next_switch = get_nsecs() + half_period_ns;
            while (get_nsecs() < next_switch); // Активное ожидание (Busy wait)

            // Устанавливаем 0
            write(fd, "0", 1);
            next_switch = get_nsecs() + half_period_ns;
            while (get_nsecs() < next_switch); // Активное ожидание (Busy wait)
        }
    }

    close(fd);
    return 0;
}