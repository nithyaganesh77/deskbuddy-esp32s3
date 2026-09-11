# 🤖 DeskBuddy — ESP32-S3 Zero + SSD1306

> ### 🌐 A tiny desk companion from **NG Labs**

An animated-face desk companion powered by an **ESP32-S3 Zero** and a **128×64 SSD1306 OLED**.

DeskBuddy combines an expressive animated face with useful everyday information — including a **clock, live weather, 5-day forecast, and world clock** — all controlled through a single push button.

🌦️ **The weather even changes DeskBuddy's mood.**

---

## ✨ Features

* 👀 Expressive animated eyes
* 😉 Natural blinking and eye movement
* 😴 Breathing / idle animation
* 😊 Weather-driven moods
* 🕐 NTP-synchronized clock
* 🌤️ Live weather information
* 📅 5-day weather forecast
* 🌎 Multi-timezone world clock
* 💡 Adjustable OLED brightness
* 📡 Built-in Wi-Fi configuration portal
* 💾 Persistent configuration using ESP32 NVS
* 🔐 Credentials kept outside the source code
* 🎛️ Single-button interface
* ⚡ Built specifically for ESP32-S3 Zero

---

## 🧠 How It Works

```text
                 ┌─────────────────────┐
                 │     ESP32-S3 Zero   │
                 │        🧠 MCU        │
                 └──────────┬──────────┘
                            │
              ┌─────────────┼─────────────┐
              │             │             │
              ▼             ▼             ▼
          📡 Wi-Fi       🌦️ Weather      🕐 NTP
              │             │             │
              └─────────────┼─────────────┘
                            ▼
                    🧠 DeskBuddy Logic
                            │
                 ┌──────────┴──────────┐
                 ▼                     ▼
             👀 OLED Face          📊 Info Pages
                 │                     │
                 └──────────┬──────────┘
                            ▼
                     🖥️ SSD1306 OLED

                       🔘
                  Single Push Button
```

---

## 🎨 Display Pages

### 👀 Face

The default screen features animated eyes with:

* 👁️ Blinking
* ↔️ Saccadic eye movement
* 🫁 Breathing / idle motion
* 😊 Multiple moods
* 🌦️ Weather-driven expressions

---

### 🕐 Clock

Displays the current local time using:

* 🌐 NTP synchronization
* 🗺️ Configurable timezone
* ⏱️ Automatic time updates

---

### 🌤️ Weather

Displays current weather information for the configured city.

The face reacts to the weather, giving DeskBuddy a little personality. 😄

---

### 📅 5-Day Forecast

View the upcoming weather outlook directly on the OLED.

---

### 🌎 World Clock

Check the time across multiple configured timezones.

Perfect for keeping track of teammates, friends, or projects around the world. 🌍

---

# 🛠️ Hardware

| Component       | Details               |
| --------------- | --------------------- |
| 🧠 MCU          | ESP32-S3 Zero         |
| 💾 Flash        | 4 MB                  |
| 🧠 PSRAM        | None                  |
| 🖥️ Display     | SSD1306 128×64 OLED   |
| 🔌 Interface    | I²C                   |
| 🔘 Input        | Momentary push button |
| 📡 Connectivity | Wi-Fi                 |

---

## 🔌 Wiring

| ESP32-S3 Zero | Connects To  |
| ------------- | ------------ |
| `3V3`         | OLED VCC     |
| `GND`         | OLED GND     |
| `GPIO8`       | OLED SDA     |
| `GPIO9`       | OLED SCL     |
| `GPIO1`       | Button leg A |
| `GND`         | Button leg B |

### 🔘 Button

The button uses:

```cpp
INPUT_PULLUP
```

No external resistor is required.

> ⚠️ **Note:** ESP32-S3 Zero boards and clones can have different pin layouts. Verify **GPIO8 / GPIO9** against your board's silkscreen before wiring.

---

# 🔧 NG Labs Changes

This version was adapted and configured specifically for the **ESP32-S3 Zero**.

### ⚡ ESP32-S3 Zero Port

* Custom ESP32-S3 Zero board definition
* 4 MB flash configuration
* USB-CDC-on-boot configuration
* GPIO remapping for the OLED
* GPIO1 configured for the push button

### 🔌 I²C Configuration

```text
SDA → GPIO8
SCL → GPIO9
```

### 🔐 Credential Separation

Credentials are kept outside the main firmware source.

```text
src/config.h
```

is ignored by Git, while:

```text
src/config.h.example
```

provides the configuration template.

---

# 📦 Installation

### 1️⃣ Clone the repository

```bash
git clone https://github.com/nithyaganesh77/deskbuddy-esp32s3.git
```

```bash
cd deskbuddy-esp32s3
```

### 2️⃣ Create your configuration

```bash
cp src/config.h.example src/config.h
```

Then edit:

```text
src/config.h
```

Add:

* 📡 Wi-Fi SSID
* 🔑 Wi-Fi password
* 🌦️ OpenWeatherMap API key
* 📍 City
* 🕐 POSIX timezone

> 🔐 `config.h` is intentionally gitignored so credentials aren't committed to the repository.

---

## 🌦️ OpenWeatherMap

Create an API key from:

**OpenWeatherMap**

Then add it to your local configuration.

> ⏳ Newly created API keys may take some time before becoming active.

---

# 🚀 Build & Upload

Using PlatformIO:

```bash
pio run --target upload
```

Then open the serial monitor:

```bash
pio device monitor
```

---

# 📡 Configure Without Reflashing

DeskBuddy includes a built-in Wi-Fi configuration portal.

### 🔘 Hold the button for 3 seconds during power-on.

DeskBuddy creates:

```text
DeskBuddy-Setup
```

Connect to the Wi-Fi network and open:

```text
http://192.168.4.1
```

You can then configure:

* 📡 Wi-Fi
* 🌦️ Weather API
* 📍 City
* 🕐 Timezone

Configuration is stored in **ESP32 NVS flash storage**, allowing the settings to survive reboots.

---

# 🎮 Controls

| Action                       | Result                          |
| ---------------------------- | ------------------------------- |
| 👆 Single tap                | Next page                       |
| 👆👆 Double tap              | Toggle brightness               |
| 👆 Long press on Face        | Change mood                     |
| 👆 Long press on Clock       | Open World Clock                |
| 👆 Long press on Weather     | Open Forecast                   |
| 👆 Single tap on World Clock | Return                          |
| 👆 Single tap on Forecast    | Return                          |
| 🔘 Hold 3 sec at boot        | Open Wi-Fi configuration portal |

---

# 🗂️ Project Structure

```text
DeskBuddy/
│
├── 📁 boards/
│   └── esp32-s3-zero.json
│
├── 📁 include/
│   └── README
│
├── 📁 lib/
│   └── README
│
├── 📁 src/
│   ├── main.cpp
│   └── config.h.example
│
├── 📁 test/
│   └── README
│
├── 📄 platformio.ini
├── 📄 README.md
├── 📄 LICENSE
└── 📄 .gitignore
```

---

# 🐛 Troubleshooting

### 🖥️ OLED is blank

Check:

* 🔌 Power connections
* 🔄 SDA/SCL wiring
* 📍 I²C pins
* 🔢 OLED I²C address

Most SSD1306 displays use:

```text
0x3C
```

Some use:

```text
0x3D
```

If necessary, run an I²C scanner to identify the correct address.

---

### 🌦️ Weather doesn't load

Check the serial monitor:

```bash
pio device monitor
```

Then verify:

* 🔑 API key
* 📍 City name
* 📡 Wi-Fi connection
* ⏳ API key activation

The city name must match the location recognized by OpenWeatherMap.

---

### 🕐 Clock is incorrect

Check your:

```text
TZ_STRING
```

POSIX timezone strings use an inverted sign convention.

For India:

```text
IST-5:30
```

not:

```text
IST+5:30
```

---

# 💡 Why DeskBuddy?

DeskBuddy started with a simple idea:

> **Why should a tiny OLED only display information when it can have a personality too?** 👀

Instead of building another static IoT dashboard, NG Labs turned the display into a small interactive companion that can **see the time, understand the weather, change its mood, and interact with its owner.**

Small hardware.
Simple interface.
A little personality. 🤖❤️

---

# 🧪 NG Labs

### 🔬 Built by **NG Labs**

Exploring the intersection of:

```text
🤖 Artificial Intelligence
        +
🔌 Embedded Systems
        +
🌐 Connected Devices
        +
⚙️ Automation
        +
🧠 Human Interaction
```

**Build → Experiment → Break → Learn → Improve → Ship 🚀**

---

# 📜 Attribution

The eye-animation approach is derived from:

**playfultechnology/esp32-eyes**

This project documents the **NG Labs build, ESP32-S3 Zero port, hardware configuration, and additional functionality**.

---

# 📄 License

See [`LICENSE`](LICENSE) for the applicable license terms.

---

<div align="center">

## 🤖 NG Labs

**Making hardware a little smarter — and a lot more fun. ⚡**

⭐ Star the repository if you like DeskBuddy!

</div>
