// ==================================================
// DESKBUDDY  —  ESP32-S3 Zero + 128x64 SSD1306 OLED
// Animated face / clock / weather / forecast desk companion
// ==================================================
// watch complete making video from here, please subscribe and support us
// you can buy components and this kit from www.esclabs.in
//
// EDISON SCIENCE CORNER - ESCLABS
// ==================================================
//
// HARDWARE
//   Board   : ESP32-S3 Zero
//   Display : SSD1306 128x64 OLED (I2C)
//   Input   : Push button on GPIO1 (other leg to GND, INPUT_PULLUP)
//
// PLATFORMIO.INI (required for ESP32-S3 Zero, 4MB flash board)
//   [env:esp32-s3-zero]
//   platform = espressif32
//   board = esp32-s3-zero        ; custom board json, 4MB flash
//   framework = arduino
//   build_flags =
//       -DARDUINO_USB_MODE=1
//       -DARDUINO_USB_CDC_ON_BOOT=1
//   monitor_speed = 115200
//
// CONTROLS
//   Single tap        : next page (cycles 0 -> 1 -> 2 -> 0)
//   Double tap         : toggle display brightness
//   Long press (page0) : cycle face mood
//   Long press (page1) : jump to World Clock (page 3)
//   Long press (page2) : jump to Forecast (page 4)
//   Single tap on 3/4  : return to page 1 / page 2
//   Hold button 3s at boot : force WiFi config portal
//
// FIRST-TIME SETUP
//   Edit config.h (same folder) with your WiFi/OpenWeatherMap/timezone
//   details before flashing. Alternatively hold the button 3 seconds
//   at power-on to open the "DeskBuddy-Setup" WiFi portal at 192.168.4.1
//   and enter details there (overrides config.h defaults, saved to flash).
//
// SERIAL DEBUG
//   115200 baud. Prints loaded config, WiFi connect status, weather/
//   forecast fetch results, and every button event (tap/double/long).
// ==================================================

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "time.h"
#include <math.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include "config.h"

// ==================================================
// 1. HARDWARE CONFIG
// ==================================================
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define SDA_PIN       8   // ESP32-S3 Zero GPIO8 — verify against your board silkscreen
#define SCL_PIN       9   // ESP32-S3 Zero GPIO9 — verify against your board silkscreen
#define TOUCH_PIN     1   // ESP32-S3 Zero GPIO1 — push button, other leg to GND, INPUT_PULLUP

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ==================================================
// 2. ICON BITMAPS (PROGMEM)
// ==================================================

// --- Big weather icons (32x32) ---
const unsigned char bmp_clear[] PROGMEM = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0xc0, 0x80, 0x00, 0x0f, 0xf0, 0x00, 0x00, 0x3f, 0xfc, 0x00, 0x00, 0x7f, 0xfe, 0x00, 0x00, 0xff, 0xff, 0x00, 0x06, 0xff, 0xff, 0x60, 0x06, 0xff, 0xff, 0x60, 0x06, 0xff, 0xff, 0x60, 0x00, 0xff, 0xff, 0x00, 0x3e, 0xff, 0xff, 0x7c, 0x3e, 0xff, 0xff, 0x7c, 0x3e, 0xff, 0xff, 0x7c, 0x00, 0xff, 0xff, 0x00, 0x06, 0xff, 0xff, 0x60, 0x06, 0xff, 0xff, 0x60, 0x06, 0xff, 0xff, 0x60, 0x00, 0xff, 0xff, 0x00, 0x00, 0x7f, 0xfe, 0x00, 0x00, 0x3f, 0xfc, 0x00, 0x01, 0x0f, 0xf0, 0x80, 0x00, 0x03, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
const unsigned char bmp_clouds[] PROGMEM = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xe0, 0x00, 0x00, 0x0f, 0xf8, 0x00, 0x00, 0x1f, 0xfc, 0x00, 0x00, 0x3f, 0xfe, 0x00, 0x00, 0x3f, 0xff, 0x00, 0x00, 0x7f, 0xff, 0x80, 0x00, 0xff, 0xff, 0xc0, 0x00, 0xff, 0xff, 0xe0, 0x01, 0xff, 0xff, 0xf0, 0x03, 0xff, 0xff, 0xf8, 0x07, 0xff, 0xff, 0xfc, 0x07, 0xff, 0xff, 0xfc, 0x0f, 0xff, 0xff, 0xfe, 0x0f, 0xff, 0xff, 0xfe, 0x1f, 0xff, 0xff, 0xff, 0x1f, 0xff, 0xff, 0xff, 0x1f, 0xff, 0xff, 0xff, 0x1f, 0xff, 0xff, 0xff, 0x1f, 0xff, 0xff, 0xff, 0x1f, 0xff, 0xff, 0xff, 0x0f, 0xff, 0xff, 0xfe, 0x07, 0xff, 0xff, 0xfc, 0x03, 0xff, 0xff, 0xf8, 0x00, 0xff, 0xff, 0xe0, 0x00, 0x3f, 0xff, 0x80, 0x00, 0x0f, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
const unsigned char bmp_rain[] PROGMEM = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xe0, 0x00, 0x00, 0x0f, 0xf8, 0x00, 0x00, 0x1f, 0xfc, 0x00, 0x00, 0x3f, 0xfe, 0x00, 0x00, 0x7f, 0xff, 0x80, 0x00, 0xff, 0xff, 0xc0, 0x01, 0xff, 0xff, 0xf0, 0x03, 0xff, 0xff, 0xf8, 0x07, 0xff, 0xff, 0xfc, 0x0f, 0xff, 0xff, 0xfe, 0x1f, 0xff, 0xff, 0xff, 0x1f, 0xff, 0xff, 0xff, 0x1f, 0xff, 0xff, 0xff, 0x1f, 0xff, 0xff, 0xff, 0x0f, 0xff, 0xff, 0xfe, 0x07, 0xff, 0xff, 0xfc, 0x03, 0xff, 0xff, 0xf8, 0x00, 0xff, 0xff, 0xe0, 0x00, 0x3f, 0xff, 0x80, 0x00, 0x0f, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x0c, 0x00, 0x00, 0x60, 0x0c, 0x00, 0x00, 0xe0, 0x1c, 0x00, 0x00, 0xc0, 0x18, 0x00, 0x03, 0x80, 0x70, 0x00, 0x03, 0x80, 0x70, 0x00, 0x03, 0x00, 0x60, 0x00, 0x02, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

// --- Mini weather icons (16x16, used on forecast cards) ---
const unsigned char mini_sun[] PROGMEM = { 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x10, 0x08, 0x04, 0x20, 0x03, 0xc0, 0x27, 0xe4, 0x07, 0xe0, 0x07, 0xe0, 0x27, 0xe4, 0x03, 0xc0, 0x04, 0x20, 0x10, 0x08, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00 };
const unsigned char mini_cloud[] PROGMEM = { 0x00, 0x00, 0x00, 0x00, 0x01, 0xc0, 0x07, 0xe0, 0x0f, 0xf0, 0x1f, 0xf8, 0x1f, 0xf8, 0x3f, 0xfc, 0x3f, 0xfc, 0x7f, 0xfe, 0x3f, 0xfe, 0x1f, 0xfc, 0x0f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
const unsigned char mini_rain[] PROGMEM = { 0x00, 0x00, 0x00, 0x00, 0x01, 0xc0, 0x07, 0xe0, 0x0f, 0xf0, 0x1f, 0xf8, 0x1f, 0xf8, 0x3f, 0xfc, 0x3f, 0xfc, 0x7f, 0xfe, 0x3f, 0xfe, 0x1f, 0xfc, 0x00, 0x00, 0x44, 0x44, 0x22, 0x22, 0x11, 0x11 };
const unsigned char bmp_tiny_drop[] PROGMEM = { 0x10, 0x38, 0x7c, 0xfe, 0xfe, 0x7c, 0x38, 0x00 };

// --- Emotion particles (16x16) ---
const unsigned char bmp_heart[] PROGMEM = { 0x00, 0x00, 0x0c, 0x60, 0x1e, 0xf0, 0x3f, 0xf8, 0x7f, 0xfc, 0x7f, 0xfc, 0x7f, 0xfc, 0x3f, 0xf8, 0x1f, 0xf0, 0x0f, 0xe0, 0x07, 0xc0, 0x03, 0x80, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
const unsigned char bmp_zzz[] PROGMEM = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x00, 0x0c, 0x00, 0x18, 0x00, 0x30, 0x00, 0x7e, 0x00, 0x00, 0x3c, 0x00, 0x0c, 0x00, 0x18, 0x00, 0x30, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x00, 0x00 };
const unsigned char bmp_anger[] PROGMEM = { 0x00, 0x00, 0x11, 0x10, 0x2a, 0x90, 0x44, 0x40, 0x80, 0x20, 0x80, 0x20, 0x44, 0x40, 0x2a, 0x90, 0x11, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

// ==================================================
// 3. GLOBAL STATE
// ==================================================

// --- Page / UI state ---
int currentPage = 0;                 // 0=face 1=clock 2=weather 3=world clock 4=forecast
bool highBrightness = true;
unsigned long lastPageSwitch = 0;
const unsigned long PAGE_INTERVAL = 8000;   // auto-advance pages 0-2 every 8s

// --- Touch input state ---
int tapCounter = 0;
unsigned long lastTapTime = 0;
bool lastPinState = false;
unsigned long pressStartTime = 0;
bool isLongPressHandled = false;
const unsigned long LONG_PRESS_TIME = 800;
const unsigned long DOUBLE_TAP_DELAY = 300;

// --- Mood states for the face page ---
#define MOOD_NORMAL      0
#define MOOD_HAPPY       1
#define MOOD_SURPRISED   2
#define MOOD_SLEEPY      3
#define MOOD_ANGRY       4
#define MOOD_SAD         5
#define MOOD_EXCITED     6
#define MOOD_LOVE        7
#define MOOD_SUSPICIOUS  8
int currentMood = MOOD_NORMAL;

// --- Config (loaded from flash via Preferences, see loadConfig()) ---
String city;
String countryCode;
String apiKey;
String wifiSsid;
String wifiPass;
String tzString;

// --- Weather data ---
unsigned long lastWeatherUpdate = 0;
float temperature = 0.0;
float feelsLike = 0.0;
int humidity = 0;
String weatherMain = "Loading";
String weatherDesc = "Wait...";

struct ForecastDay {
  String dayName;
  int temp;
  String iconType;
};
ForecastDay fcast[3];

const char* ntpServer = "pool.ntp.org";

// ==================================================
// 4. EYE PHYSICS ENGINE
// ==================================================
// Each eye is a spring-damped rectangle (sclera) with a secondary
// spring-damped pupil offset inside it, giving a soft "alive" motion
// instead of snapping instantly to target positions/sizes.

struct Eye {
  float x, y;   // current top-left position
  float w, h;   // current size
  float targetX, targetY, targetW, targetH;

  // Pupil offset from eye center (secondary motion, lags behind eye)
  float pupilX, pupilY;
  float targetPupilX, targetPupilY;

  // Physics constants
  float velX, velY, velW, velH;
  float pVelX, pVelY;
  float k  = 0.12;  // eye spring constant
  float d  = 0.60;  // eye damping
  float pk = 0.08;  // pupil spring constant (softer/laggier)
  float pd = 0.50;  // pupil damping

  bool blinking;
  unsigned long lastBlink;
  unsigned long nextBlinkTime;

  void init(float _x, float _y, float _w, float _h) {
    x = targetX = _x;
    y = targetY = _y;
    w = targetW = _w;
    h = targetH = _h;
    pupilX = targetPupilX = 0;
    pupilY = targetPupilY = 0;
    nextBlinkTime = millis() + random(1000, 4000);
  }

  void update() {
    // Main eye spring physics
    float ax = (targetX - x) * k;
    float ay = (targetY - y) * k;
    float aw = (targetW - w) * k;
    float ah = (targetH - h) * k;

    velX = (velX + ax) * d;
    velY = (velY + ay) * d;
    velW = (velW + aw) * d;
    velH = (velH + ah) * d;

    x += velX;
    y += velY;
    w += velW;
    h += velH;

    // Pupil spring physics (drags behind eye movement)
    float pax = (targetPupilX - pupilX) * pk;
    float pay = (targetPupilY - pupilY) * pk;
    pVelX = (pVelX + pax) * pd;
    pVelY = (pVelY + pay) * pd;
    pupilX += pVelX;
    pupilY += pVelY;
  }
};

Eye leftEye, rightEye;
unsigned long lastSaccade = 0;
unsigned long saccadeInterval = 3000;
float breathVal = 0;

// ==================================================
// 5. CONFIG PORTAL (WiFi + API key via local web page)
// ==================================================
#define CONFIG_AP_SSID  "DeskBuddy-Setup"
#define CONFIG_AP_PASS  "12345678"
#define CONFIG_HOLD_MS  3000

Preferences prefs;
WebServer configServer(80);
bool inConfigMode = false;
bool oledOk = false;

// Loads saved WiFi/weather/timezone settings from flash.
// Falls back to ESCLabs demo defaults if nothing has been saved yet.
void loadConfig() {
  prefs.begin("deskbuddy", true);
  wifiSsid    = prefs.getString("ssid", "");
  wifiPass    = prefs.getString("pass", "");
  apiKey      = prefs.getString("apikey", "");
  city        = prefs.getString("city", "");
  countryCode = prefs.getString("country", "");
  tzString    = prefs.getString("tz", "");
  prefs.end();

  if (wifiSsid.isEmpty()) {
    wifiSsid    = WIFI_SSID;
    wifiPass    = WIFI_PASS;
    apiKey      = OWM_API_KEY;
    city        = OWM_CITY;
    countryCode = OWM_COUNTRY;
    tzString    = TZ_STRING;
  } else {
    if (apiKey.isEmpty())      apiKey = OWM_API_KEY;
    if (city.isEmpty())        city = OWM_CITY;
    if (countryCode.isEmpty()) countryCode = OWM_COUNTRY;
    if (tzString.isEmpty())    tzString = TZ_STRING;
  }

  Serial.println("========== CONFIG LOADED ==========");
  Serial.println("WiFi SSID   : " + wifiSsid);
  Serial.println("API Key     : " + String(apiKey.isEmpty() ? "(not set)" : "(set)"));
  Serial.println("City        : " + city);
  Serial.println("Country     : " + countryCode);
  Serial.println("Timezone    : " + tzString);
  Serial.println("====================================");
}

void saveConfig(const String& s, const String& p, const String& ak,
                const String& cty, const String& ctry, const String& tz) {
  prefs.begin("deskbuddy", false);
  prefs.putString("ssid", s);
  prefs.putString("pass", p);
  prefs.putString("apikey", ak);
  prefs.putString("city", cty);
  prefs.putString("country", ctry);
  prefs.putString("tz", tz);
  prefs.end();
}

void handleConfigRoot() {
  prefs.begin("deskbuddy", true);
  String sSsid = prefs.getString("ssid", "");
  String sApik = prefs.getString("apikey", "");
  String sCity = prefs.getString("city", "Idukki");
  String sCtry = prefs.getString("country", "IN");
  String sTz   = prefs.getString("tz", "IST-5:30");
  prefs.end();

  String html = R"rawliteral(
<!DOCTYPE html><html><head><meta name="viewport" content="width=device-width,initial-scale=1">
<title>DeskBuddy Config</title>
<style>
body{font-family:sans-serif;max-width:420px;margin:30px auto;padding:24px;background:#0c1929;color:#e8f4fc;}
h1{color:#5ba3f5;margin-bottom:8px;}
input{width:100%;padding:10px;margin:6px 0;border:1px solid #2d4a6f;border-radius:6px;box-sizing:border-box;background:#1a2d47;color:#e8f4fc;}
input:focus{outline:none;border-color:#5ba3f5;}
button{width:100%;padding:12px;background:#3498db;color:#fff;border:none;border-radius:6px;font-size:16px;cursor:pointer;margin-top:16px;}
button:hover{background:#2980b9;}
label{display:block;margin-top:14px;color:#8ab4e8;font-size:14px;}
.section{margin-top:20px;padding-top:16px;border-top:1px solid #1e3a5f;}
.section-title{color:#5ba3f5;font-size:13px;margin-bottom:8px;}
</style></head><body>
<h1>DeskBuddy Setup</h1>
<form action="/save" method="POST">
<label>WiFi SSID</label><input name="ssid" placeholder="Your WiFi name" value=")rawliteral";
  html += sSsid;
  html += R"rawliteral(">
<label>WiFi Password</label><input name="pass" type="password" placeholder="WiFi password">
<div class="section"><div class="section-title">Weather (OpenWeatherMap)</div>
<label>API Key</label><input name="apikey" placeholder="API key" value=")rawliteral";
  html += sApik;
  html += R"rawliteral(">
<label>City</label><input name="city" placeholder="e.g. London" value=")rawliteral";
  html += sCity;
  html += R"rawliteral(">
<label>Country Code</label><input name="country" placeholder="e.g. IN, US, GB" value=")rawliteral";
  html += sCtry;
  html += R"rawliteral(">
</div>
<div class="section"><div class="section-title">Time</div>
<label>Timezone</label><input name="tz" placeholder="e.g. IST-5:30, EST5EDT" value=")rawliteral";
  html += sTz;
  html += R"rawliteral(">
</div>
<button type="submit">Save &amp; Reboot</button>
</form></body></html>)rawliteral";
  configServer.send(200, "text/html", html);
}

void handleConfigSave() {
  if (!configServer.hasArg("ssid") || configServer.arg("ssid").length() == 0) {
    configServer.send(400, "text/plain", "SSID required");
    return;
  }
  String s   = configServer.arg("ssid");
  String p   = configServer.arg("pass");
  String ak  = configServer.arg("apikey");
  String cty = configServer.arg("city");
  String ctr = configServer.arg("country");
  String tz  = configServer.arg("tz");

  prefs.begin("deskbuddy", true);
  if (ak.isEmpty())  ak  = prefs.getString("apikey", "");
  if (cty.isEmpty()) cty = prefs.getString("city", "Idukki");
  if (ctr.isEmpty()) ctr = prefs.getString("country", "IN");
  if (tz.isEmpty())  tz  = prefs.getString("tz", "IST-5:30");
  prefs.end();

  saveConfig(s, p, ak, cty, ctr, tz);
  configServer.send(200, "text/html",
    "<html><body style='font-family:sans-serif;background:#0c1929;color:#e8f4fc;padding:40px;'>"
    "<h2 style='color:#5ba3f5'>Saved!</h2><p>Rebooting in 2 seconds...</p></body></html>");
  delay(2000);
  ESP.restart();
}

void startConfigPortal() {
  inConfigMode = true;
  WiFi.mode(WIFI_AP);
  WiFi.softAP(CONFIG_AP_SSID, CONFIG_AP_PASS);
  configServer.on("/", handleConfigRoot);
  configServer.on("/save", HTTP_POST, handleConfigSave);
  configServer.begin();

  display.clearDisplay();
  display.setFont(NULL);
  display.setCursor(0, 0);
  display.print("Config mode\n\nConnect to:\n");
  display.print(CONFIG_AP_SSID);
  display.print("\n\nThen open:\n192.168.4.1");
  display.display();
}

// ==================================================
// 6. WEATHER LOGIC & NETWORK
// ==================================================

const unsigned char* getBigIcon(String w) {
  if (w == "Clear") return bmp_clear;
  if (w == "Clouds") return bmp_clouds;
  if (w == "Rain" || w == "Drizzle") return bmp_rain;
  return bmp_clouds;
}

const unsigned char* getMiniIcon(String w) {
  if (w == "Clear") return mini_sun;
  if (w == "Rain" || w == "Drizzle" || w == "Thunderstorm") return mini_rain;
  return mini_cloud;
}

// Maps current weather conditions/temperature onto a face mood.
void updateMoodBasedOnWeather() {
  int m = MOOD_NORMAL;
  if (weatherMain == "Clear") m = MOOD_HAPPY;
  else if (weatherMain == "Rain" || weatherMain == "Drizzle") m = MOOD_SAD;
  else if (weatherMain == "Thunderstorm") m = MOOD_SURPRISED;
  else if (weatherMain == "Clouds") m = MOOD_NORMAL;
  else if (temperature > 25) m = MOOD_EXCITED;
  else if (temperature < 5) m = MOOD_SLEEPY;
  currentMood = m;
}

// Polls the touch pin and converts raw state into tap/double-tap/long-press
// events, dispatching page changes, mood changes, and brightness toggles.
void handleTouch() {
  bool currentPinState = (digitalRead(TOUCH_PIN) == LOW);  // active-low: pressed pulls to GND
  unsigned long now = millis();

  if (currentPinState && !lastPinState) {
    // Touch just started
    pressStartTime = now;
    isLongPressHandled = false;
  } else if (currentPinState && lastPinState) {
    // Touch is being held
    if ((now - pressStartTime > LONG_PRESS_TIME) && !isLongPressHandled) {
      lastPageSwitch = now;
      if (currentPage == 0) {
        currentMood++;
        if (currentMood > MOOD_SUSPICIOUS) currentMood = 0;
        lastSaccade = 0;  // force an immediate gaze update
      } else if (currentPage == 1) currentPage = 3;
      else if (currentPage == 2) currentPage = 4;
      isLongPressHandled = true;
      Serial.println("[BUTTON] long press -> page " + String(currentPage) + ", mood " + String(currentMood));
    }
  } else if (!currentPinState && lastPinState) {
    // Touch just released
    if ((now - pressStartTime < LONG_PRESS_TIME) && !isLongPressHandled) {
      tapCounter++;
      lastTapTime = now;
    }
  }
  lastPinState = currentPinState;

  // Resolve buffered taps once the double-tap window has elapsed
  if (tapCounter > 0 && (now - lastTapTime > DOUBLE_TAP_DELAY)) {
    lastPageSwitch = now;
    if (tapCounter == 2) {
      highBrightness = !highBrightness;
      display.dim(!highBrightness);
      display.display();
      Serial.println("[BUTTON] double tap -> brightness " + String(highBrightness ? "HIGH" : "LOW"));
    } else if (tapCounter == 1) {
      if (currentPage == 3) currentPage = 1;
      else if (currentPage == 4) currentPage = 2;
      else {
        currentPage++;
        if (currentPage > 2) currentPage = 0;
      }
      Serial.println("[BUTTON] tap -> page " + String(currentPage));
    }
    tapCounter = 0;
  }
}

// Fetches current weather + 3-day forecast from OpenWeatherMap.
void getWeatherAndForecast() {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;

  // --- Current conditions ---
  String url = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + countryCode + "&appid=" + apiKey + "&units=metric";
  http.begin(url);
  if (http.GET() == 200) {
    String payload = http.getString();
    JSONVar myObject = JSON.parse(payload);
    if (JSON.typeof(myObject) != "undefined") {
      temperature = double(myObject["main"]["temp"]);
      feelsLike = double(myObject["main"]["feels_like"]);
      humidity = int(myObject["main"]["humidity"]);
      weatherMain = (const char*)myObject["weather"][0]["main"];
      weatherDesc = (const char*)myObject["weather"][0]["description"];
      weatherDesc[0] = toupper(weatherDesc[0]);
      updateMoodBasedOnWeather();

      Serial.println("---- Weather updated ----");
      Serial.println("Temp      : " + String(temperature) + " C");
      Serial.println("Feels like: " + String(feelsLike) + " C");
      Serial.println("Humidity  : " + String(humidity) + " %");
      Serial.println("Condition : " + weatherMain + " (" + weatherDesc + ")");
    } else {
      Serial.println("Weather JSON parse failed");
    }
  } else {
    Serial.println("Weather HTTP GET failed");
  }
  http.end();

  // --- 3-day forecast (3-hour steps, picking ~1 sample/day) ---
  url = "http://api.openweathermap.org/data/2.5/forecast?q=" + city + "," + countryCode + "&appid=" + apiKey + "&units=metric";
  http.begin(url);
  if (http.GET() == 200) {
    String payload = http.getString();
    JSONVar fo = JSON.parse(payload);
    if (JSON.typeof(fo) != "undefined") {
      struct tm t;
      getLocalTime(&t);
      int today = t.tm_wday;
      const char* days[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
      int indices[3] = { 7, 15, 23 };  // ~24h, ~48h, ~72h ahead
      for (int i = 0; i < 3; i++) {
        int idx = indices[i];
        fcast[i].temp = (int)double(fo["list"][idx]["main"]["temp"]);
        fcast[i].iconType = (const char*)fo["list"][idx]["weather"][0]["main"];
        int nextDayIndex = (today + i + 1) % 7;
        fcast[i].dayName = days[nextDayIndex];
        Serial.println("Forecast " + fcast[i].dayName + ": " + String(fcast[i].temp) + "C " + fcast[i].iconType);
      }
    } else {
      Serial.println("Forecast JSON parse failed");
    }
  } else {
    Serial.println("Forecast HTTP GET failed");
  }
  http.end();
}

// ==================================================
// 7. DRAWING & ANIMATION
// ==================================================

// Draws an expression "mask" over an already-drawn eye to carve out
// lid shapes (angry slant, sad slant, happy cheek, sleepy droop, etc).
void drawEyelidMask(float x, float y, float w, float h, int mood, bool isLeft) {
  int ix = (int)x;
  int iy = (int)y;
  int iw = (int)w;
  int ih = (int)h;
  display.setTextColor(SSD1306_BLACK);

  if (mood == MOOD_ANGRY) {
    if (isLeft)
      for (int i = 0; i < 16; i++) display.drawLine(ix, iy + i, ix + iw, iy - 6 + i, SSD1306_BLACK);
    else
      for (int i = 0; i < 16; i++) display.drawLine(ix, iy - 6 + i, ix + iw, iy + i, SSD1306_BLACK);
  }
  else if (mood == MOOD_SAD) {
    if (isLeft)
      for (int i = 0; i < 16; i++) display.drawLine(ix, iy - 6 + i, ix + iw, iy + i, SSD1306_BLACK);
    else
      for (int i = 0; i < 16; i++) display.drawLine(ix, iy + i, ix + iw, iy - 6 + i, SSD1306_BLACK);
  }
  else if (mood == MOOD_HAPPY || mood == MOOD_LOVE || mood == MOOD_EXCITED) {
    display.fillRect(ix, iy + ih - 12, iw, 14, SSD1306_BLACK);
    display.fillCircle(ix + iw / 2, iy + ih + 6, iw / 1.3, SSD1306_BLACK);
  }
  else if (mood == MOOD_SLEEPY) {
    display.fillRect(ix, iy, iw, ih / 2 + 2, SSD1306_BLACK);
  }
  else if (mood == MOOD_SUSPICIOUS) {
    if (isLeft) display.fillRect(ix, iy, iw, ih / 2 - 2, SSD1306_BLACK);
    else display.fillRect(ix, iy + ih - 8, iw, 8, SSD1306_BLACK);
  }
}

// Draws one eye: sclera, pupil (with gaze offset + clamp), specular
// highlight, then the mood-specific eyelid mask on top.
void drawUltraProEye(Eye& e, bool isLeft) {
  int ix = (int)e.x;
  int iy = (int)e.y;
  int iw = (int)e.w;
  int ih = (int)e.h;

  int r = (iw < 20) ? 3 : 8;
  display.fillRoundRect(ix, iy, iw, ih, r, SSD1306_WHITE);

  int cx = ix + iw / 2;
  int cy = iy + ih / 2;
  int pw = iw / 2.2;
  int ph = ih / 2.2;

  int px = cx + (int)e.pupilX - (pw / 2);
  int py = cy + (int)e.pupilY - (ph / 2);

  // Keep pupil fully inside the sclera bounds
  if (px < ix) px = ix;
  if (px + pw > ix + iw) px = ix + iw - pw;
  if (py < iy) py = iy;
  if (py + ph > iy + ih) py = iy + ih - ph;

  display.fillRoundRect(px, py, pw, ph, r / 2, SSD1306_BLACK);

  if (iw > 15 && ih > 15) {
    display.fillCircle(px + pw - 4, py + 4, 2, SSD1306_WHITE);
  }

  drawEyelidMask(e.x, e.y, e.w, e.h, currentMood, isLeft);
}

// Advances blink timing, idle gaze (saccades), breathing, and
// mood-driven eye target sizes, then steps both eyes' spring physics.
void updatePhysicsAndMood() {
  unsigned long now = millis();
  breathVal = sin(now / 800.0) * 1.5;

  // --- Blink ---
  if (now > leftEye.nextBlinkTime) {
    leftEye.blinking = true;
    leftEye.lastBlink = now;
    rightEye.blinking = true;
    leftEye.nextBlinkTime = now + random(2000, 6000);
  }
  if (leftEye.blinking) {
    leftEye.targetH = 2;
    rightEye.targetH = 2;
    if (now - leftEye.lastBlink > 120) {
      leftEye.blinking = false;
      rightEye.blinking = false;
    }
  }

  // --- Idle gaze (saccades) ---
  if (!leftEye.blinking && now - lastSaccade > saccadeInterval) {
    lastSaccade = now;
    saccadeInterval = random(500, 3000);

    int dir = random(0, 10);
    float lx = 0, ly = 0;

    if (dir < 4)       { lx = 0;  ly = 0;  }  // center (most common)
    else if (dir == 4) { lx = -6; ly = -4; }  // top-left
    else if (dir == 5) { lx = 6;  ly = -4; }  // top-right
    else if (dir == 6) { lx = -6; ly = 4;  }  // bottom-left
    else if (dir == 7) { lx = 6;  ly = 4;  }  // bottom-right
    else if (dir == 8) { lx = 8;  ly = 0;  }  // right
    else if (dir == 9) { lx = -8; ly = 0;  }  // left

    leftEye.targetPupilX = lx;
    leftEye.targetPupilY = ly;
    rightEye.targetPupilX = lx;
    rightEye.targetPupilY = ly;

    // Subtle head-follow: eye containers drift slightly toward gaze direction
    leftEye.targetX = 18 + (lx * 0.3);
    leftEye.targetY = 14 + (ly * 0.3);
    rightEye.targetX = 74 + (lx * 0.3);
    rightEye.targetY = 14 + (ly * 0.3);
  }

  // --- Mood-driven eye shape (overrides size targets) ---
  if (!leftEye.blinking) {
    float baseW = 36;
    float baseH = 36 + breathVal;

    switch (currentMood) {
      case MOOD_NORMAL:
        leftEye.targetW = baseW;  leftEye.targetH = baseH;
        rightEye.targetW = baseW; rightEye.targetH = baseH;
        break;
      case MOOD_HAPPY:
      case MOOD_LOVE:
        leftEye.targetW = 40;  leftEye.targetH = 32;
        rightEye.targetW = 40; rightEye.targetH = 32;
        break;
      case MOOD_SURPRISED:
        leftEye.targetW = 30;  leftEye.targetH = 45;
        rightEye.targetW = 30; rightEye.targetH = 45;
        leftEye.targetPupilX += random(-1, 2);
        break;
      case MOOD_SLEEPY:
        leftEye.targetW = 38;  leftEye.targetH = 30;
        rightEye.targetW = 38; rightEye.targetH = 30;
        break;
      case MOOD_ANGRY:
        leftEye.targetW = 34;  leftEye.targetH = 32;
        rightEye.targetW = 34; rightEye.targetH = 32;
        break;
      case MOOD_SAD:
        leftEye.targetW = 34;  leftEye.targetH = 40;
        rightEye.targetW = 34; rightEye.targetH = 40;
        break;
      case MOOD_SUSPICIOUS:
        leftEye.targetW = 36;  leftEye.targetH = 20;  // squint
        rightEye.targetW = 36; rightEye.targetH = 42; // wide
        break;
    }
  }

  leftEye.update();
  rightEye.update();
}

// Page 0: animated face with mood-based particles (heart/zzz/anger).
void drawEmoPage() {
  updatePhysicsAndMood();

  if (currentMood == MOOD_LOVE) {
    display.drawBitmap(56, 0, bmp_heart, 16, 16, SSD1306_WHITE);
  } else if (currentMood == MOOD_SLEEPY) {
    display.drawBitmap(110, 0, bmp_zzz, 16, 16, SSD1306_WHITE);
  } else if (currentMood == MOOD_ANGRY) {
    display.drawBitmap(56, 0, bmp_anger, 16, 16, SSD1306_WHITE);
  }

  drawUltraProEye(leftEye, true);
  drawUltraProEye(rightEye, false);
}

// Page 4: 3-day forecast cards with mini icons and temperatures.
void drawForecastPage() {
  display.fillRect(0, 0, 128, 16, SSD1306_WHITE);
  display.setFont(NULL);
  display.setTextColor(SSD1306_BLACK);
  display.setCursor(20, 4);
  display.print("3-DAY FORECAST");
  display.setTextColor(SSD1306_WHITE);
  display.drawLine(42, 16, 42, 64, SSD1306_WHITE);
  display.drawLine(85, 16, 85, 64, SSD1306_WHITE);

  for (int i = 0; i < 3; i++) {
    int xStart = i * 43;
    int centerX = xStart + 21;

    display.setFont(NULL);
    String d = fcast[i].dayName;
    if (d == "") d = "Wait";
    display.setCursor(centerX - (d.length() * 3), 20);
    display.print(d);

    display.drawBitmap(centerX - 8, 28, getMiniIcon(fcast[i].iconType), 16, 16, SSD1306_WHITE);

    display.setFont(&FreeSansBold9pt7b);
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(String(fcast[i].temp).c_str(), 0, 0, &x1, &y1, &w, &h);
    display.setCursor(centerX - (w / 2) - 2, 60);
    display.print(fcast[i].temp);
    display.fillCircle(centerX + (w / 2) + 1, 52, 2, SSD1306_WHITE);  // degree symbol
  }
}

// Page 1: large digital clock with date.
void drawClock() {
  struct tm t;
  if (!getLocalTime(&t)) {
    display.setFont(NULL);
    display.setCursor(30, 30);
    display.print("Syncing...");
    return;
  }

  String ampm = (t.tm_hour >= 12) ? "PM" : "AM";
  int h12 = t.tm_hour % 12;
  if (h12 == 0) h12 = 12;

  display.setTextColor(SSD1306_WHITE);
  display.setFont(NULL);
  display.setTextSize(1);
  display.setCursor(114, 0);
  display.print(ampm);

  display.setFont(&FreeSansBold18pt7b);
  char timeStr[6];
  sprintf(timeStr, "%02d:%02d", h12, t.tm_min);
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(timeStr, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, 42);
  display.print(timeStr);

  display.setFont(&FreeSans9pt7b);
  char dateStr[20];
  strftime(dateStr, 20, "%a, %b %d", &t);
  display.getTextBounds(dateStr, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, 62);
  display.print(dateStr);
}

// Page 2: current weather summary card.
void drawWeatherCard() {
  if (WiFi.status() != WL_CONNECTED) {
    display.setFont(NULL);
    display.setCursor(0, 0);
    display.print("No WiFi");
    return;
  }

  display.drawBitmap(96, 0, getBigIcon(weatherMain), 32, 32, SSD1306_WHITE);

  display.setFont(&FreeSansBold9pt7b);
  String c = city;
  c.toUpperCase();
  if (c.length() > 9) c = c.substring(0, 8) + ".";
  display.setCursor(0, 14);
  display.print(c);

  display.setFont(&FreeSansBold18pt7b);
  int tempInt = (int)temperature;
  display.setCursor(0, 48);
  display.print(tempInt);

  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(String(tempInt).c_str(), 0, 48, &x1, &y1, &w, &h);
  display.fillCircle(x1 + w + 5, 26, 4, SSD1306_WHITE);  // degree symbol

  display.setFont(NULL);
  display.drawBitmap(88, 32, bmp_tiny_drop, 8, 8, SSD1306_WHITE);
  display.setCursor(100, 32);
  display.print(humidity);
  display.print("%");

  display.setCursor(88, 45);
  display.print("~");
  display.print((int)feelsLike);

  display.drawLine(0, 52, 128, 52, SSD1306_WHITE);
  display.setCursor(0, 55);
  display.print(weatherDesc);
}

// Page 3: secondary clock showing India + Sydney time (hidden page,
// reached via long-press from Clock or Weather pages).
void drawWorldClock() {
  time_t now;
  time(&now);
  time_t indiaEpoch = now + (5 * 3600) + (30 * 60);
  time_t sydneyEpoch = now + (11 * 3600);

  struct tm* indiatm = gmtime(&indiaEpoch);
  int i_h = indiatm->tm_hour;
  int i_m = indiatm->tm_min;

  struct tm* sydneytm = gmtime(&sydneyEpoch);
  int s_h = sydneytm->tm_hour;
  int s_m = sydneytm->tm_min;

  display.fillRect(0, 0, 128, 16, SSD1306_WHITE);
  display.setFont(NULL);
  display.setTextColor(SSD1306_BLACK);
  display.setCursor(32, 4);
  display.print("WORLD CLOCK");
  display.setTextColor(SSD1306_WHITE);
  display.drawLine(64, 18, 64, 54, SSD1306_WHITE);

  display.setFont(NULL);
  display.setCursor(16, 22);
  display.print("INDIA");
  display.setFont(&FreeSansBold9pt7b);
  char iStr[10];
  sprintf(iStr, "%02d:%02d", i_h, i_m);
  display.setCursor(5, 46);
  display.print(iStr);

  display.setFont(NULL);
  display.setCursor(78, 22);
  display.print("SYDNEY");
  display.setFont(&FreeSansBold9pt7b);
  char sStr[10];
  sprintf(sStr, "%02d:%02d", s_h, s_m);
  display.setCursor(69, 46);
  display.print(sStr);

  display.setFont(NULL);
  display.setCursor(35, 56);
  display.print("Tap to Exit");
}

// ==================================================
// 8. BOOT SEQUENCE
// ==================================================

// Circle wipe in, inverse wipe out, then centered logo text.
void playBootAnimation() {
  display.setTextColor(SSD1306_WHITE);
  int cx = 64;
  int cy = 32;

  for (int r = 0; r < 80; r += 4) {
    display.clearDisplay();
    display.fillCircle(cx, cy, r, SSD1306_WHITE);
    display.display();
    delay(10);
  }
  for (int r = 0; r < 80; r += 4) {
    display.clearDisplay();
    display.fillCircle(cx, cy, 80, SSD1306_WHITE);
    display.fillCircle(cx, cy, r, SSD1306_BLACK);
    display.display();
    delay(10);
  }

  display.setFont(&FreeSansBold9pt7b);
  String bootText = "ESCLabs";
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(bootText, 0, 0, &x1, &y1, &w, &h);

  display.clearDisplay();
  display.setCursor((SCREEN_WIDTH - w) / 2, 36);
  display.print(bootText);
  display.display();
  delay(2000);
}

// ==================================================
// 9. SETUP & MAIN LOOP
// ==================================================

void setup() {
  Serial.begin(115200);
  unsigned long serialWait = millis();
  while (!Serial && millis() - serialWait < 5000) { delay(10); }
  delay(300);
  Serial.println("\n\n===== DESKBUDDY BOOTING =====");

  Wire.begin(SDA_PIN, SCL_PIN);
  pinMode(TOUCH_PIN, INPUT_PULLUP);

  // --- I2C scan: retries for a few seconds so this shows up even if
  //     the serial monitor reattaches late after the S3's USB re-enumerates ---
  int found = 0;
  for (int attempt = 1; attempt <= 6 && found == 0; attempt++) {
    delay(500);
    Serial.println("--- I2C scan attempt " + String(attempt) + " ---");
    for (byte addr = 1; addr < 127; addr++) {
      Wire.beginTransmission(addr);
      if (Wire.endTransmission() == 0) {
        Serial.print("  Found device at 0x");
        Serial.println(addr, HEX);
        found++;
      }
    }
    if (found == 0) Serial.println("  nothing found this attempt");
  }
  if (found == 0) Serial.println("STILL NOTHING after 6 attempts — check wiring/power (SDA=8, SCL=9)");

  oledOk = display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  if (!oledOk) {
    Serial.println("display.begin(0x3C) FAILED — trying 0x3D...");
    oledOk = display.begin(SSD1306_SWITCHCAPVCC, 0x3D);
    Serial.println(oledOk ? "display.begin(0x3D) OK" : "display.begin(0x3D) FAILED TOO — OLED not responding");
  } else {
    Serial.println("display.begin(0x3C) OK");
  }
  Serial.println(oledOk ? ">>> OLED INIT: SUCCESS <<<" : ">>> OLED INIT: FAILURE <<<");
  display.setTextColor(SSD1306_WHITE);

  // Hold button for 3s at boot to force the WiFi config portal
  bool forceConfig = false;
  for (unsigned long t = millis(); millis() - t < CONFIG_HOLD_MS; ) {
    if (digitalRead(TOUCH_PIN) == LOW) { forceConfig = true; break; }
    delay(80);
  }
  Serial.println(forceConfig ? "Boot button held -> config portal" : "Normal boot");

  loadConfig();

  if (forceConfig) {
    startConfigPortal();
    return;
  }

  leftEye.init(18, 14, 36, 36);
  rightEye.init(74, 14, 36, 36);

  playBootAnimation();

  display.clearDisplay();
  display.setFont(NULL);
  display.setCursor(40, 30);
  display.print("connecting");
  display.display();

  Serial.print("Connecting to WiFi: ");
  Serial.println(wifiSsid);
  WiFi.begin(wifiSsid.c_str(), wifiPass.c_str());
  unsigned long wifiStart = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - wifiStart < 15000)) {
    delay(200);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi FAILED -> starting config portal");
    startConfigPortal();
    return;
  }
  Serial.println("WiFi connected, IP: " + WiFi.localIP().toString());

  configTime(0, 0, ntpServer);
  setenv("TZ", tzString.c_str(), 1);
  tzset();

  getWeatherAndForecast();
  lastWeatherUpdate = millis();
  lastPageSwitch = millis();
  Serial.println("===== SETUP COMPLETE =====\n");
}

void loop() {
  if (inConfigMode) {
    configServer.handleClient();
    return;
  }

  static unsigned long lastOledWarn = 0;
  if (!oledOk && millis() - lastOledWarn > 3000) {
    lastOledWarn = millis();
    Serial.println(">>> OLED still not initialized — nothing will show on screen <<<");
  }

  unsigned long now = millis();
  handleTouch();

  // Refresh weather every 10 minutes
  if (now - lastWeatherUpdate > 600000) {
    getWeatherAndForecast();
    lastWeatherUpdate = now;
  }

  // Auto-advance through the three main pages (face/clock/weather)
  if (currentPage < 3 && now - lastPageSwitch > PAGE_INTERVAL) {
    currentPage++;
    if (currentPage > 2) currentPage = 0;
    lastPageSwitch = now;
    lastSaccade = 0;
  }

  display.clearDisplay();
  switch (currentPage) {
    case 0: drawEmoPage(); break;
    case 1: drawClock(); break;
    case 2: drawWeatherCard(); break;
    case 3: drawWorldClock(); break;
    case 4: drawForecastPage(); break;
  }
  display.display();
}