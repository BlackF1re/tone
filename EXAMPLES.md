# Руководство по примерам использования Tone v3.0

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
