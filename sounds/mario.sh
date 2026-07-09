#!/bin/sh

# Super Mario Bros melody player for otone on OpenWrt routers.

OTONE_BIN="${OTONE_BIN:-/usr/bin/otone}"

if [ ! -x "$OTONE_BIN" ]; then
    echo "Error: otone binary not found at $OTONE_BIN"
    echo "Set OTONE_BIN=/path/to/otone or install otone to /usr/bin/otone"
    exit 1
fi

play_mario() {
    $OTONE_BIN 2637 83; sleep 0.108
    $OTONE_BIN 2637 83; sleep 0.216
    $OTONE_BIN 2637 83; sleep 0.216
    $OTONE_BIN 2093 83; sleep 0.108
    $OTONE_BIN 2637 83; sleep 0.216
    $OTONE_BIN 3136 83; sleep 0.432
    $OTONE_BIN 1568 83; sleep 0.432
}

play_underworld() {
    $OTONE_BIN 262 83; sleep 0.108
    $OTONE_BIN 523 83; sleep 0.108
    $OTONE_BIN 220 83; sleep 0.108
    $OTONE_BIN 440 83; sleep 0.108
    $OTONE_BIN 233 167; sleep 0.217
    $OTONE_BIN 466 167; sleep 0.217
    sleep 0.333
    $OTONE_BIN 175 83; sleep 0.108
    $OTONE_BIN 349 83; sleep 0.108
    $OTONE_BIN 147 83; sleep 0.108
    $OTONE_BIN 294 83; sleep 0.108
}

choice="${1:-mario}"
case "$choice" in
    mario)
        echo "Playing: Mario main theme"
        play_mario
        ;;
    underworld)
        echo "Playing: Underworld theme"
        play_underworld
        ;;
    both)
        echo "Playing: Mario main theme"
        play_mario
        sleep 2
        echo "Playing: Underworld theme"
        play_underworld
        ;;
    *)
        echo "Usage: $0 [mario|underworld|both]"
        exit 1
        ;;
esac
