#!/bin/sh

# Скрипт для проигрывания мелодии из Super Mario Bros на OpenWRT роутере
# Использует программу tone для генерации звуков

TONE_BIN="/root/tone/tone"

# Проверяем наличие tone
if [ ! -x "$TONE_BIN" ]; then
    echo "Error: tone binary not found at $TONE_BIN"
    exit 1
fi

# ============================================================================
# ОСНОВНАЯ ТЕМА МАРИО
# ============================================================================

# Частоты нот для основной темы
play_mario() {
    # E7 E7 0 E7 (четвертные, 1000/12 = 83 ms)
    $TONE_BIN 2637 83
    sleep 0.108
    $TONE_BIN 2637 83
    sleep 0.108
    sleep 0.108
    $TONE_BIN 2637 83
    sleep 0.108
    
    # 0 C7 E7 0
    sleep 0.108
    $TONE_BIN 2093 83
    sleep 0.108
    $TONE_BIN 2637 83
    sleep 0.108
    sleep 0.108
    
    # G7 0 0 0
    $TONE_BIN 3136 83
    sleep 0.108
    sleep 0.108
    sleep 0.108
    sleep 0.108
    
    # G6 0 0 0
    $TONE_BIN 1568 83
    sleep 0.108
    sleep 0.108
    sleep 0.108
    sleep 0.108
    
    # C7 0 0 G6
    $TONE_BIN 2093 83
    sleep 0.108
    sleep 0.108
    sleep 0.108
    $TONE_BIN 1568 83
    sleep 0.108
    
    # 0 0 E6 0
    sleep 0.108
    sleep 0.108
    $TONE_BIN 1319 83
    sleep 0.108
    sleep 0.108
    
    # 0 A6 0 B6
    $TONE_BIN 1760 83
    sleep 0.108
    sleep 0.108
    $TONE_BIN 1976 83
    sleep 0.108
    
    # 0 AS6 A6 0
    sleep 0.108
    $TONE_BIN 1865 83
    sleep 0.108
    $TONE_BIN 1760 83
    sleep 0.108
    sleep 0.108
    
    # G6 E7 G7 (точка - 1000/9 = 111 ms)
    $TONE_BIN 1568 111
    sleep 0.144
    $TONE_BIN 2637 111
    sleep 0.144
    $TONE_BIN 3136 111
    sleep 0.144
    
    # A7 0 F7 G7
    $TONE_BIN 3520 83
    sleep 0.108
    sleep 0.108
    $TONE_BIN 1397 83
    sleep 0.108
    $TONE_BIN 3136 83
    sleep 0.108
    
    # 0 E7 0 C7
    sleep 0.108
    $TONE_BIN 2637 83
    sleep 0.108
    sleep 0.108
    $TONE_BIN 2093 83
    sleep 0.108
    
    # D7 B6 0 0
    $TONE_BIN 2349 83
    sleep 0.108
    $TONE_BIN 1976 83
    sleep 0.108
    sleep 0.108
    sleep 0.108
    
    # Повтор (вторая половина мелодии)
    
    # C7 0 0 G6
    $TONE_BIN 2093 83
    sleep 0.108
    sleep 0.108
    sleep 0.108
    $TONE_BIN 1568 83
    sleep 0.108
    
    # 0 0 E6 0
    sleep 0.108
    sleep 0.108
    $TONE_BIN 1319 83
    sleep 0.108
    sleep 0.108
    
    # 0 A6 0 B6
    $TONE_BIN 1760 83
    sleep 0.108
    sleep 0.108
    $TONE_BIN 1976 83
    sleep 0.108
    
    # 0 AS6 A6 0
    sleep 0.108
    $TONE_BIN 1865 83
    sleep 0.108
    $TONE_BIN 1760 83
    sleep 0.108
    sleep 0.108
    
    # G6 E7 G7 (точка)
    $TONE_BIN 1568 111
    sleep 0.144
    $TONE_BIN 2637 111
    sleep 0.144
    $TONE_BIN 3136 111
    sleep 0.144
    
    # A7 0 F7 G7
    $TONE_BIN 3520 83
    sleep 0.108
    sleep 0.108
    $TONE_BIN 1397 83
    sleep 0.108
    $TONE_BIN 3136 83
    sleep 0.108
    
    # 0 E7 0 C7
    sleep 0.108
    $TONE_BIN 2637 83
    sleep 0.108
    sleep 0.108
    $TONE_BIN 2093 83
    sleep 0.108
    
    # D7 B6 0 0
    $TONE_BIN 2349 83
    sleep 0.108
    $TONE_BIN 1976 83
    sleep 0.108
    sleep 0.108
    sleep 0.108
}

# ============================================================================
# ПОДЗЕМЕЛЬЕ (UNDERWORLD)
# ============================================================================

play_underworld() {
    # C4 C5 A3 A4 (четвертные 83ms)
    $TONE_BIN 262 83
    sleep 0.108
    $TONE_BIN 523 83
    sleep 0.108
    $TONE_BIN 220 83
    sleep 0.108
    $TONE_BIN 440 83
    sleep 0.108
    
    # AS3 AS4 0 (восьмые 167ms)
    $TONE_BIN 233 167
    sleep 0.217
    $TONE_BIN 466 167
    sleep 0.217
    sleep 0.333
    
    # пауза (половинная 500ms)
    sleep 0.65
    
    # Повтор первого куска
    $TONE_BIN 262 83
    sleep 0.108
    $TONE_BIN 523 83
    sleep 0.108
    $TONE_BIN 220 83
    sleep 0.108
    $TONE_BIN 440 83
    sleep 0.108
    
    $TONE_BIN 233 167
    sleep 0.217
    $TONE_BIN 466 167
    sleep 0.217
    sleep 0.333
    
    sleep 0.65
    
    # F3 F4 D3 D4
    $TONE_BIN 175 83
    sleep 0.108
    $TONE_BIN 349 83
    sleep 0.108
    $TONE_BIN 147 83
    sleep 0.108
    $TONE_BIN 294 83
    sleep 0.108
    
    # DS3 DS4 0
    $TONE_BIN 156 167
    sleep 0.217
    $TONE_BIN 311 167
    sleep 0.217
    sleep 0.333
    
    sleep 0.65
    
    # F3 F4 D3 D4 (повтор)
    $TONE_BIN 175 83
    sleep 0.108
    $TONE_BIN 349 83
    sleep 0.108
    $TONE_BIN 147 83
    sleep 0.108
    $TONE_BIN 294 83
    sleep 0.108
    
    # DS3 DS4 0 DS4 CS4 D4
    $TONE_BIN 156 167
    sleep 0.217
    $TONE_BIN 311 167
    sleep 0.217
    sleep 0.333
    $TONE_BIN 311 333
    sleep 0.433
    $TONE_BIN 277 333
    sleep 0.433
    $TONE_BIN 294 333
    sleep 0.433
    
    # CS4 DS4
    $TONE_BIN 277 333
    sleep 0.433
    $TONE_BIN 311 333
    sleep 0.433
    
    # DS4 GS3
    $TONE_BIN 311 333
    sleep 0.433
    $TONE_BIN 208 333
    sleep 0.433
    
    # G3 CS4
    $TONE_BIN 196 333
    sleep 0.433
    $TONE_BIN 277 333
    sleep 0.433
    
    # C4 FS4 F4 E3 AS4 A4
    $TONE_BIN 262 167
    sleep 0.217
    $TONE_BIN 740 167
    sleep 0.217
    $TONE_BIN 349 167
    sleep 0.217
    $TONE_BIN 165 167
    sleep 0.217
    $TONE_BIN 932 167
    sleep 0.217
    $TONE_BIN 440 167
    sleep 0.217
    
    # GS4 DS4 B3 (250ms)
    $TONE_BIN 831 250
    sleep 0.325
    $TONE_BIN 311 250
    sleep 0.325
    $TONE_BIN 247 250
    sleep 0.325
    
    # AS3 A3 GS3
    $TONE_BIN 233 250
    sleep 0.325
    $TONE_BIN 220 250
    sleep 0.325
    $TONE_BIN 208 250
    sleep 0.325
    
    # паузы
    sleep 1.3
}

# ============================================================================
# ОСНОВНАЯ ПРОГРАММА
# ============================================================================

choice="${1:-mario}"

case "$choice" in
    mario)
        echo "Playing: Mario Main Theme"
        play_mario
        echo "Finished"
        ;;
    underworld)
        echo "Playing: Underworld Theme"
        play_underworld
        echo "Finished"
        ;;
    both)
        echo "Playing: Mario Main Theme"
        play_mario
        sleep 2
        echo "Playing: Underworld Theme"
        play_underworld
        ;;
    *)
        echo "Unknown melody: $choice"
        echo ""
        echo "Usage: $0 [melody]"
        echo "Melodies: mario, underworld, both"
        exit 1
        ;;
esac

exit 0
