# otone

`otone` is a small OpenWrt/Linux command line tool for playing beeps, alerts, chirps, and simple melodies through a router buzzer connected to GPIO sysfs.

It is useful for OpenWrt router diagnostics, audible boot/status alerts, GPIO buzzer testing, embedded Linux experiments, and tiny shell-script notifications.

## Features

- Written in C with minimal dependencies
- Works through `/sys/class/gpio/.../value`
- Auto-detects common buzzer GPIO paths
- Saves the detected buzzer path to `/etc/otone.conf`
- Supports fixed frequency beeps, silence, PWM volume, and chirp frequency sweeps
- Optional verbose diagnostics and simple timing statistics
- Includes a Mario melody script example in `sounds/mario.sh`

## Install on OpenWrt by copy-paste

SSH into the router and run:

```sh
opkg update
opkg install git git-http gcc make libc-dev
cd /tmp
git clone https://github.com/BlackF1re/otone.git
cd otone
make
install -D -m 0755 ./otone /usr/bin/otone
otone 2000 300
```

If the repository is still available through the old GitHub redirect, this also works:

```sh
opkg update
opkg install git git-http gcc make libc-dev
cd /tmp
git clone https://github.com/BlackF1re/tone.git otone
cd otone
make
install -D -m 0755 ./otone /usr/bin/otone
otone 2000 300
```

## Install from a local Linux PC

```sh
git clone https://github.com/BlackF1re/otone.git
cd otone
make
scp ./otone root@192.168.1.1:/usr/bin/otone
ssh root@192.168.1.1 'chmod +x /usr/bin/otone && otone 2000 300'
```

## Manual build

```sh
gcc -O2 -Wall -Wextra -std=c99 -o otone otone.c -lm
```

Build with a fixed GPIO path to skip detection:

```sh
gcc -O2 -Wall -Wextra -std=c99 -DBUZZER_GPIO_PATH='"/sys/class/gpio/gpio10/value"' -o otone otone.c -lm
```

## Usage

```sh
otone <frequency_hz> <duration_ms> [options]
```

Examples:

```sh
otone 2000 300
otone 0 1000
otone 2500 300 -p 50
otone 1000 800 -c 4000
otone 2000 300 -d /sys/class/gpio/gpio10/value
otone 2000 300 -vv
```

Options:

```text
-d, --device PATH   GPIO value file for the buzzer
-p, --pwm VALUE     PWM volume from 0 to 100
-c, --chirp HZ      Sweep from the start frequency to HZ
-v, --verbose       Verbose output
-vv                 Verbose output with statistics
-h, --help          Show help
```

## Configuration

The detected path is saved to:

```text
/etc/otone.conf
```

Example config:

```text
buzzer_path=/sys/class/gpio/gpio10/value
spin_threshold_us=100
default_volume=100
enabled=1
```

## Melody example

```sh
chmod +x sounds/mario.sh
sounds/mario.sh mario
sounds/mario.sh underworld
sounds/mario.sh both
```

## Troubleshooting

No sound:

```sh
ls -la /sys/class/gpio
cat /etc/otone.conf 2>/dev/null || true
otone 2000 300 -v
otone 2000 300 -d /sys/class/gpio/gpio10/value
```

Permission error:

```sh
ssh root@192.168.1.1
otone 2000 300
```

Compiler missing:

```sh
opkg update
opkg install gcc make libc-dev
```

## License

BSD 3-Clause. See [LICENSE](LICENSE).
