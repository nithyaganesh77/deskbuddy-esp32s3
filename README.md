# DeskBuddy — ESP32-S3 Zero + SSD1306

Animated-face desk companion on a 128x64 OLED. Shows an expressive blinking face, a clock, live weather, a 5-day forecast and a world clock, all driven from a single push button. Mood changes with the weather.

> **Attribution.** Base firmware is **DeskBuddy by Edison Science Corner (ESC Labs)** — [esclabs.in](https://www.esclabs.in). Eye-animation approach derives from [playfultechnology/esp32-eyes](https://github.com/playfultechnology/esp32-eyes). This repository documents my build and my port to the ESP32-S3 Zero. See [My changes](#my-changes).

<!-- Add a photo or GIF of the display here. -->

## My changes

<!-- Replace these with what you actually did. Delete any that don't apply. -->
- Ported from ESP32-C3 Super Mini to **ESP32-S3 Zero** — custom board definition (`boards/esp32-s3-zero.json`, 4 MB flash), remapped I²C to GPIO8/9 and the button to GPIO1, USB-CDC-on-boot build flags.
- Moved all credentials out of source into a gitignored `src/config.h`.

## Hardware

| Part | Detail |
|---|---|
| MCU | ESP32-S3 Zero (4 MB flash, no PSRAM) |
| Display | SSD1306 128x64 OLED, I²C |
| Input | Momentary push button |

### Wiring

| ESP32-S3 Zero | Connects to |
|---|---|
| 3V3 | OLED VCC |
| GND | OLED GND |
| GPIO8 | OLED SDA |
| GPIO9 | OLED SCL |
| GPIO1 | Button leg A |
| GND | Button leg B |

Button uses `INPUT_PULLUP` — no external resistor needed. Verify GPIO8/9 against your board's silkscreen; ESP32-S3 Zero clones vary.

## Setup

```
git clone https://github.com/nithyaganesh77/deskbuddy-esp32s3.git
cd deskbuddy-esp32s3
```

```
cp src/config.h.example src/config.h
```

Edit `src/config.h` with your Wi-Fi, your [OpenWeatherMap](https://home.openweathermap.org/api_keys) API key, city and POSIX timezone. This file is gitignored.

```
pio run --target upload
```

```
pio device monitor
```

### Configuring without reflashing

Hold the button for 3 seconds at power-on. The board opens a `DeskBuddy-Setup` Wi-Fi access point — connect and browse to `http://192.168.4.1` to enter Wi-Fi and API details. Settings save to flash (NVS) and override `config.h`.

## Controls

| Action | Result |
|---|---|
| Single tap | Next page (face → clock → weather) |
| Double tap | Toggle brightness |
| Long press on face | Cycle mood |
| Long press on clock | Jump to world clock |
| Long press on weather | Jump to 5-day forecast |
| Single tap on world clock / forecast | Back |
| Hold 3 s at boot | Open Wi-Fi config portal |

## Pages

- **Face** — animated eyes with blinking, saccades, breathing motion and weather-driven mood
- **Clock** — NTP-synced local time
- **Weather** — current conditions for the configured city
- **Forecast** — 5-day outlook
- **World clock** — multiple timezones

## Troubleshooting

**Blank display.** Wrong I²C address or pins. Most SSD1306 modules are `0x3C`, some are `0x3D`. Run an I²C scanner sketch to confirm both address and that SDA/SCL aren't swapped.

**Weather never loads.** Check the serial monitor. A new OpenWeatherMap key takes up to a couple of hours to activate, and the city name must match OWM's spelling exactly.

**Clock is wrong.** `TZ_STRING` uses POSIX sign convention, which is inverted from what you'd expect — India is `IST-5:30`, not `IST+5:30`.

## Project layout

```
src/main.cpp            firmware — face animation, pages, weather, config portal
src/config.h.example    credentials template (copy to src/config.h)
boards/esp32-s3-zero.json  custom board definition
platformio.ini          build config and library deps
```

## License

Base firmware © Edison Science Corner (ESC Labs). Check their terms before redistributing. My modifications are offered under the same terms as the original.
