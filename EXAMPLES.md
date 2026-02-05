# Руководство по примерам использования Tone v3.0

## ⚡ Быстрые примеры (одна команда)

Самые интересные звуки, которые можно воспроизвести прямо в командной строке:

```bash
# 1️⃣ Классический звуковой сигнал (школьный звонок)
tone 1000 300

# 2️⃣ Поднимающийся звук (ракета или волшебство)
tone 1000 0 -c 4000

# 3️⃣ Опускающийся звук (грустный результат)
tone 4000 0 -c 1000

# 4️⃣ Двойной сигнал (две ноты)
tone 1500 150 && tone 2000 150

# 5️⃣ Низкий гудок (сигнал готовности)
tone 200 500

# 6️⃣ Высокий писк (восклицание)
tone 5000 100

# 7️⃣ Микс громкости (громко + тихо + тишина)
tone 2000 100 -p 100 && tone 2000 100 -p 30 && tone 0 100

# 8️⃣ Тревога полусекундной длительности
tone 3000 500 -p 80

# 9️⃣ Проверка звука (4 разных частоты)
for f in 1000 2000 3000 4000; do tone $f 100 -p 80; done

# 🔟 Весёлый "ping" (как в компьютерных играх)
tone 3000 50 -p 100 && sleep 0.1 && tone 3000 50 -p 100
```

---

## 📝 Примеры скриптов для роутера

### 1. Простой звуковой сигнал

```bash
#!/bin/sh
# alarm.sh - Простое будильник

echo "Alarm!"
tone 2000 100
tone 0 50
tone 2000 100
```

### 2. Полиция-сирена

```bash
#!/bin/sh
# police_siren.sh - Классическая сирена

for i in 1 2 3 4 5; do
    tone 2000 100 -p 100   # Высокий звук на полную мощность
    tone 0 30              # Короткая пауза
    tone 1000 100 -p 100   # Низкий звук
    tone 0 30              # Пауза
done
```

### 3. Звуковой сигнал с понижением (тревога)

```bash
#!/bin/sh
# alert.sh

tone 3000 200 -c 1000  # Chirp вниз от 3кГц до 1кГц
sleep 1
tone 2500 200 -c 1000  # Повтор
```

### 4. Громкость регулируется от переменной

```bash
#!/bin/sh
# volume_control.sh

VOLUME=${1:-100}  # Громкость из параметра, по умолчанию 100%

if [ "$VOLUME" -lt 0 ] || [ "$VOLUME" -gt 100 ]; then
    echo "Volume must be 0-100"
    exit 1
fi

echo "Playing at ${VOLUME}% volume..."
tone 2500 500 -p "$VOLUME"
```

Использование:
```bash
./volume_control.sh 25     # Очень тихо
./volume_control.sh 100    # На полную
```

### 5. Мелодия через последовательные частоты

```bash
#!/bin/sh
# melody.sh - Простая мелодия

# Ноты: C=262, D=294, E=330, F=349, G=392, A=440, B=494

play_note() {
    freq=$1
    duration=$2
    tone "$freq" "$duration"
}

# Мелодия "Twinkle Twinkle Little Star"
play_note 262 300  # C
play_note 262 300  # C
play_note 392 300  # G
play_note 392 300  # G
play_note 440 300  # A
play_note 440 300  # A
play_note 392 600  # G

tone 0 200        # Пауза
```

### 6. Диагностический набор сигналов

```bash
#!/bin/sh
# diagnostics.sh - Проверка баззера

echo "Testing buzzer..."

# Тест 1: Низкая частота
echo "Low frequency test (500 Hz)..."
tone 500 200 -v

# Тест 2: Оптимальная частота
echo "Optimal frequency test (3000 Hz)..."
tone 3000 200 -v

# Тест 3: Высокая частота
echo "High frequency test (8000 Hz)..."
tone 8000 200 -v

# Тест 4: PWM (громкость)
echo "Volume test..."
for vol in 100 75 50 25 10; do
    echo "Volume: ${vol}%"
    tone 2000 100 -p "$vol"
    tone 0 50
done

# Тест 5: Chirp
echo "Chirp test..."
tone 1000 500 -c 5000 -vv

echo "All tests completed!"
```

### 7. Интеграция с системой (уведомления)

```bash
#!/bin/sh
# notify.sh - Системные уведомления

case "$1" in
    success)
        echo "Success!"
        tone 2000 100
        tone 0 50
        tone 2500 100
        ;;
    error)
        echo "Error!"
        tone 1000 100 -c 500
        ;;
    warning)
        echo "Warning!"
        tone 2000 200 -p 50
        ;;
    *)
        echo "Usage: $0 {success|error|warning}"
        ;;
esac
```

Использование в других скриптах:
```bash
if [ $? -eq 0 ]; then
    ./notify.sh success
else
    ./notify.sh error
fi
```

### 8. Звук по расписанию (cron)

```bash
# /etc/crontabs/root

# Ежедневный будильник в 7:00
0 7 * * * /root/alarm.sh

# Каждый час
0 * * * * tone 1000 50

# Каждые 15 минут (работающий статус)
*/15 * * * * tone 3000 30 -p 30

# Уведомление за 5 минут до полуночи (для перезагрузки)
55 23 * * * tone 2000 100 -c 1000
```

---

## 🔧 Практические советы

### Проверка работы баззера

```bash
# Проверить, что баззер найден и работает
tone 2000 100 -v

# Если не работает, проверить вручную
cat /sys/devices/platform/1e000000.palmbus/1e000600.gpio/gpiochip0/gpio/buzzer/value
echo "1" > /sys/devices/platform/1e000000.palmbus/1e000600.gpio/gpiochip0/gpio/buzzer/value
echo "0" > /sys/devices/platform/1e000000.palmbus/1e000600.gpio/gpiochip0/gpio/buzzer/value
```

### Диагностика проблем со звуком

```bash
# Проверить стабильность с полной статистикой
tone 2000 1000 -vv

# Если много jitter, может быть проблема с системной нагрузкой
# Попробуйте снизить нагрузку:
killall uhttpd    # Остановить веб-интерфейс
tone 2000 1000 -vv
```

### Оптимизация для вашего баззера

```bash
# Найти оптимальную частоту (обычно 2-4 кГц)
for f in 1000 2000 2500 3000 4000 5000; do
    echo "Testing $f Hz..."
    tone "$f" 200
    sleep 1
done
```

### Создание иконов громкости

```bash
#!/bin/sh
# volume_icon.sh - Визуальный индикатор громкости

VOLUME=$1

case "$VOLUME" in
    0)
        echo "Muted"
        tone 1000 50
        ;;
    25)
        echo "1/4 volume"
        tone 2000 50 -p 25
        ;;
    50)
        echo "1/2 volume"
        tone 2000 50 -p 50
        ;;
    75)
        echo "3/4 volume"
        tone 2000 50 -p 75
        ;;
    *)
        echo "Full volume"
        tone 2000 50
        ;;
esac
```

---

## 🎯 Примеры для различных сценариев

### WAN подключение (уведомление)

```bash
# В скрипте подключения WAN
tone 2500 100
tone 0 50
tone 2500 100
```

### Перезагрузка роутера

```bash
# /etc/init.d/reboot (перед перезагрузкой)
tone 3000 100 -c 1000  # Chirp вниз перед перезагрузкой
```

### Низкая батарея (если батарея есть)

```bash
# /root/check_battery.sh
BATTERY=$(cat /sys/class/power_supply/battery/capacity)

if [ "$BATTERY" -lt 20 ]; then
    tone 1000 100
    tone 0 100
    tone 1000 100
fi
```

### Мониторинг активности

```bash
# /root/activity_monitor.sh
# Периодический "живой" звук для подтверждения работы

tone 3000 30 -p 25  # Тихий звук на четверть мощности
```

---

## 📊 Рекомендуемые частоты

| Частота | Применение |
|---------|-----------|
| 500-1000 Hz | Низкие, глубокие сигналы |
| 1000-2000 Hz | Стандартные сигналы |
| **2000-4000 Hz** | **Оптимально для пьезозвучателей** |
| 4000-8000 Hz | Высокие, пронзительные сигналы |
| > 8000 Hz | Предупредительные звуки |

---

## ⚠️ Важные замечания

1. **Баззеры разные** — одна программа может звучать по-разному на разных роутерах. Экспериментируйте!

2. **PWM требует поддержки** — на некоторых роутерах PWM может не работать или работать неправильно. Тестируйте с `-vv`!

3. **Real-time приоритет** — попытаемся установить, но это может не работать. Программа продолжит работу с обычным приоритетом.

4. **Питание** — мощные звуки (частые переключения) потребляют больше энергии. Используйте PWM для снижения нагрузки.

5. **Безопасность** — не используйте слишком часто и слишком громко, это может повредить оборудование!

---

## 🎵 Весёлые трюки

Создайте файл с расширением `.tone` и интегрируйте в свою систему:

```bash
#!/bin/sh
# /root/bin/tonefile.sh - Воспроизведение из файла

FILE=$1
while IFS=':' read freq duration; do
    tone "$freq" "$duration"
done < "$FILE"
```

Содержимое файла (например `siren.tone`):
```
2000:100
0:50
1000:100
0:50
3000:200
0:100
```

Использование:
```bash
/root/bin/tonefile.sh siren.tone
```

---

## 🎼 Коллекция мелодий и звуков

### Чебурашка (классический звук)
```bash
#!/bin/sh
# Из мультфильма "Чебурашка"

tone 2000 100   # До
sleep 0.1
tone 2000 100   # До
sleep 0.1
tone 2500 100   # Ми
sleep 0.1
tone 2500 100   # Ми
sleep 0.2
tone 3000 200   # Соль
```

### Звук раздачи WAN (положительный)
```bash
#!/bin/sh
# Плиу-плиу-плиу - подключение WAN

for i in 1 2 3; do
    tone 2500 50
    sleep 0.05
    tone 3000 50
    sleep 0.05
done
```

### Потеря WAN (отрицательный)
```bash
#!/bin/sh
# Грустный звук - потеря соединения

tone 2000 200 -c 1000   # Свип вниз
sleep 0.2
tone 1500 150 -c 800
```

### Загрузка (прогресс)
```bash
#!/bin/sh
# Звуки загрузки 0-100%

for pct in 20 40 60 80 100; do
    freq=$((2000 + pct * 10))
    tone "$freq" 50
    sleep 0.05
done
```

### Музыкальный звонок (бонг)
```bash
#!/bin/sh
# Глубокий звонок

tone 1000 200
sleep 0.1
tone 1200 100
sleep 0.1
tone 800 200
```

### Старый телефон (классический звонок)
```bash
#!/bin/sh
# Звонок старого телефона

for i in 1 2 3 4 5; do
    tone 800 100
    tone 0 50
    tone 1200 100
    tone 0 150
done
```

### Электронный будильник
```bash
#!/bin/sh
# Пищащий будильник

for i in 1 2 3 4 5; do
    tone 4000 100
    tone 0 100
done
```

### Звук Error (компьютерный)
```bash
#!/bin/sh
# Классический "ошибка" из Windows

tone 1000 50
tone 0 20
tone 1000 50
tone 0 20
tone 1000 100
```

### Звук Success (позитивный)
```bash
#!/bin/sh
# Ура, все хорошо!

tone 2000 100
tone 0 50
tone 2500 100
tone 0 50
tone 3000 150
```

### Марш (веселый)
```bash
#!/bin/sh
# Маршевый ритм

tone 2000 100
tone 2500 100
tone 2000 100
sleep 0.1
tone 3000 200
```

### Сирена машины скорой помощи
```bash
#!/bin/sh
# Циклическая сирена

for i in 1 2 3 4 5; do
    tone 1500 100 -c 3000
    tone 0 50
    tone 3000 100 -c 1500
    tone 0 50
done
```

### Пасс-код (звуки нажатия кнопок на телефоне)
```bash
#!/bin/sh
# Тоны DTMF (как на старом телефоне)

# Цифра 1
tone 700 50
tone 1200 50
tone 0 50

# Цифра 2
tone 700 50
tone 1300 50
tone 0 50

# Цифра 3
tone 700 50
tone 1500 50
tone 0 50
```

---

## 🔧 Примеры для разработчиков

### Функция-обёртка для удобства
```bash
#!/bin/sh
# /root/bin/sound.sh

sound() {
    case "$1" in
        ok|success|done)
            tone 2000 100; tone 0 50; tone 2500 100
            ;;
        error|fail|bad)
            tone 1000 100 -c 500
            ;;
        warning|alert)
            tone 2000 200 -p 50
            ;;
        beep)
            tone 3000 50
            ;;
        tick)
            tone 2000 30
            ;;
        *)
            echo "Unknown sound: $1"
            return 1
            ;;
    esac
}

# Использование в других скриптах
sound ok
sound error
sound beep
```

### Логирование с звуком
```bash
#!/bin/sh
# Логирование с аудиоуведомлением

log_with_sound() {
    level=$1
    message=$2
    
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] [$level] $message" >> /tmp/log.txt
    
    case "$level" in
        ERROR)
            tone 1000 100 -c 500
            ;;
        WARN)
            tone 2000 100
            ;;
        INFO)
            tone 3000 30
            ;;
    esac
}

# Использование
log_with_sound ERROR "Потеря интернета"
log_with_sound INFO "Система загружена"
```

### Обратный отсчёт (countdown)
```bash
#!/bin/sh
# Обратный отсчёт 5-4-3-2-1

countdown() {
    for i in 5 4 3 2 1; do
        freq=$((1000 + i * 200))
        tone "$freq" 100
        sleep 0.2
    done
    
    # Завершение
    tone 4000 200
}

countdown
```

### Проверка отклика (ping-pong)
```bash
#!/bin/sh
# Звук для проверки отклика сервера

check_server() {
    host=$1
    
    if ping -c 1 -W 1 "$host" > /dev/null 2>&1; then
        # Успех - двойной пищащий звук
        tone 2500 100
        tone 0 50
        tone 2500 100
        return 0
    else
        # Ошибка - один звук вниз
        tone 1000 100 -c 500
        return 1
    fi
}

check_server 8.8.8.8
```

### Мониторинг температуры
```bash
#!/bin/sh
# Звук в зависимости от температуры

temp_alert() {
    temp=$1
    
    if [ "$temp" -lt 40 ]; then
        # Холодно - низкий звук
        tone 1000 100
    elif [ "$temp" -lt 60 ]; then
        # Нормально - средний звук
        tone 2000 100
    elif [ "$temp" -lt 80 ]; then
        # Горячо - высокий звук
        tone 3000 100
    else
        # Очень горячо - сирена!
        tone 4000 100 -c 2000
    fi
}

temp_alert 75
```

---

## 🎮 Интерактивные примеры

### Тестовое меню
```bash
#!/bin/sh
# Интерактивное меню с звуками

menu() {
    echo "1. Тестовый звук"
    echo "2. PWM тест (50%)"
    echo "3. Chirp эффект"
    echo "4. Статистика"
    
    read -p "Выберите (1-4): " choice
    
    case "$choice" in
        1)
            tone 2000 500
            ;;
        2)
            tone 2000 500 -p 50
            ;;
        3)
            tone 1000 500 -c 3000
            ;;
        4)
            tone 2000 1000 -vv
            ;;
        *)
            echo "Неверный выбор"
            return 1
            ;;
    esac
}

menu
```

### Генератор случайных тонов
```bash
#!/bin/sh
# Случайные звуки для веселья

random_tone() {
    freq=$((RANDOM % 4000 + 500))  # От 500 до 4500 Hz
    duration=$((RANDOM % 300 + 100))  # От 100 до 400 мс
    
    tone "$freq" "$duration"
}

# Генерируем 5 случайных звуков
for i in 1 2 3 4 5; do
    random_tone
    sleep 0.2
done
```

### Калибровка громкости
```bash
#!/bin/sh
# Проверить все уровни громкости

calibrate_volume() {
    echo "Калибровка громкости от 0% до 100%"
    
    for vol in 0 10 20 30 40 50 60 70 80 90 100; do
        echo "Громкость: ${vol}%"
        tone 2000 200 -p "$vol"
        sleep 0.5
    done
}

calibrate_volume
```

### Тест всех функций
```bash
#!/bin/sh
# Полный тест программы

full_test() {
    echo "=== Тест Tone Generator v3.0 ==="
    
    echo "1. Базовый звук..."
    tone 2000 200
    sleep 0.5
    
    echo "2. PWM (25%, 50%, 100%)..."
    tone 2000 100 -p 25
    tone 2000 100 -p 50
    tone 2000 100 -p 100
    sleep 0.5
    
    echo "3. Chirp вверх..."
    tone 1000 300 -c 3000
    sleep 0.5
    
    echo "4. Chirp вниз..."
    tone 3000 300 -c 1000
    sleep 0.5
    
    echo "5. Пауза..."
    tone 0 500
    sleep 0.5
    
    echo "6. Статистика..."
    tone 2000 1000 -vv
    
    echo "=== Тест завершён ==="
}

full_test
```

---

## 📡 Примеры для системного администратора

### Скрипт для мониторинга диска
```bash
#!/bin/sh
# Предупреждение о месте на диске

check_disk() {
    usage=$(df / | tail -1 | awk '{print $5}' | sed 's/%//')
    
    if [ "$usage" -gt 90 ]; then
        echo "КРИТИЧНО: Диск почти полон ($usage%)"
        # Срочно звонит
        for i in 1 2 3; do
            tone 4000 100
            tone 0 100
        done
    elif [ "$usage" -gt 80 ]; then
        echo "ВНИМАНИЕ: Диск заполнен ($usage%)"
        # Звонит медленнее
        tone 3000 200
    else
        echo "OK: Диск в норме ($usage%)"
        # Всё хорошо
        tone 2000 100
    fi
}

check_disk
```

### Мониторинг памяти
```bash
#!/bin/sh
# Уведомление об использовании памяти

check_memory() {
    free_mem=$(free | grep Mem | awk '{print ($7 / $2) * 100}')
    
    if (( $(echo "$free_mem < 10" | bc -l) )); then
        echo "КРИТИЧНО: Мало свободной памяти"
        tone 1000 50 -c 4000  # Быстрый свип вверх
    elif (( $(echo "$free_mem < 20" | bc -l) )); then
        echo "ВНИМАНИЕ: Память заканчивается"
        tone 2000 100
    else
        echo "OK: Памяти достаточно"
        tone 3000 50
    fi
}

check_memory
```

### Уведомление о процессах
```bash
#!/bin/sh
# Звук при появлении определённого процесса

watch_process() {
    process=$1
    sound_file=/tmp/process_watch.lock
    
    if pgrep "$process" > /dev/null; then
        if [ ! -f "$sound_file" ]; then
            echo "Процесс $process запущен"
            tone 2500 200
            touch "$sound_file"
        fi
    else
        if [ -f "$sound_file" ]; then
            echo "Процесс $process остановлен"
            tone 1000 200 -c 500
            rm "$sound_file"
        fi
    fi
}

watch_process nginx
```

---

## 🚨 Примеры аварийных сигналов

### Критическая ошибка
```bash
#!/bin/sh
# Паническая сирена

panic() {
    echo "!!! КРИТИЧЕСКАЯ ОШИБКА !!!"
    
    # Полная паника
    for i in 1 2 3 4 5; do
        tone 4000 50
        tone 0 50
        tone 3000 50
        tone 0 50
    done
}

panic
```

### Перезагрузка системы
```bash
#!/bin/sh
# Предупреждение перед перезагрузкой

reboot_warning() {
    echo "Система перезагружается за 10 секунд"
    
    for i in 10 9 8 7 6 5 4 3 2 1; do
        freq=$((1000 + i * 100))
        tone "$freq" 100
        sleep 0.8
    done
    
    # Окончательный сигнал
    tone 4000 500
}

reboot_warning
```

### Обновление системы
```bash
#!/bin/sh
# Статус обновления

update_status() {
    echo "Начало обновления..."
    tone 2000 100 -c 3000  # Свип вверх = начало
    
    # Имитация процесса
    sleep 2
    
    echo "Обновление завершено"
    tone 3000 100 -c 1000  # Свип вниз = конец
}

update_status
```

---

## 🎛️ Примеры с разными параметрами

### Сравнение громкостей
```bash
#!/bin/sh
# Слышать разницу между громкостями

echo "Прослушивание 2500 Hz на разных уровнях:"
for vol in 10 30 50 70 90 100; do
    echo "Громкость: ${vol}%"
    tone 2500 300 -p "$vol"
    sleep 0.3
done
```

### Тестирование частот
```bash
#!/bin/sh
# Тест разных частот

echo "Тестирование частотного диапазона:"
for freq in 500 1000 1500 2000 2500 3000 4000 5000; do
    echo "Частота: ${freq} Hz"
    tone "$freq" 200
    sleep 0.3
done
```

### Длительность
```bash
#!/bin/sh
# Разные длительности

echo "Разные длительности при 2000 Hz:"
for dur in 100 200 300 500 1000; do
    echo "Длительность: ${dur} ms"
    tone 2000 "$dur"
    sleep 0.3
done
```

### Комбинирование параметров
```bash
#!/bin/sh
# Сложная последовательность

echo "Сложная комбинация параметров:"

# Громкий низкий звук
tone 1000 200 -p 100
sleep 0.2

# Тихий средний звук
tone 2500 200 -p 30
sleep 0.2

# Свип с половинной громкостью
tone 1500 300 -c 3500 -p 50
```

---

## 🧪 Примеры для отладки и диагностики

### Полная диагностика
```bash
#!/bin/sh
# Полный отчёт о системе с звуками

diagnostics() {
    echo "=== Диагностика Tone Generator ==="
    
    # 1. Базовый функционал
    echo "1. Проверка базовой функциональности..."
    tone 2000 100
    
    # 2. PWM
    echo "2. Проверка PWM..."
    tone 2000 200 -p 50
    
    # 3. Chirp
    echo "3. Проверка Chirp..."
    tone 1000 300 -c 3000
    
    # 4. Статистика
    echo "4. Сбор статистики..."
    tone 2000 500 -vv
    
    echo "=== Диагностика завершена ==="
}

diagnostics
```

### Проверка стабильности
```bash
#!/bin/sh
# Долгий тест стабильности

stability_test() {
    echo "Тест стабильности (10 секунд при 2000 Hz)..."
    tone 2000 10000 -vv
    
    echo "Если программа отработала нормально,"
    echo "значит стабильность хорошая"
}

stability_test
```

### Измерение overhead
```bash
#!/bin/sh
# Измерение накладных расходов

measure_overhead() {
    echo "Измерение overhead цикла..."
    tone 2000 100 -v
    echo "Смотрите в выводе 'Loop overhead'"
}

measure_overhead
```

---

## 📊 Примеры для сбора статистики

### Сравнение режимов
```bash
#!/bin/sh
# Сравнить обычный режим и PWM

compare_modes() {
    echo "=== Обычный режим (100%) ==="
    tone 2000 1000 -vv
    sleep 1
    
    echo ""
    echo "=== PWM режим (50%) ==="
    tone 2000 1000 -p 50 -vv
}

compare_modes
```

### Анализ jitter
```bash
#!/bin/sh
# Анализ стабильности с разными нагрузками

analyze_jitter() {
    echo "Тест без нагрузки..."
    tone 2000 1000 -vv
    
    echo ""
    echo "Тест с нагрузкой (фоновое копирование)..."
    cp -r /var/log /tmp/backup &
    tone 2000 1000 -vv
    wait
}

analyze_jitter
```
