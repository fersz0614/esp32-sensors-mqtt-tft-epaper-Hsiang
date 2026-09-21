#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <TFT_22_ILI9225.h>

constexpr char WIFI_SSID[] = "H";
constexpr char WIFI_PASSWORD[] = "YOUR_WIFI_PASSWORD";
constexpr char WEATHER_URL[] =
  "https://api.open-meteo.com/v1/forecast?latitude=22.6273&longitude=120.3014"
  "&current=temperature_2m,relative_humidity_2m,precipitation,wind_speed_10m,weather_code"
  "&daily=weather_code,temperature_2m_max,temperature_2m_min,precipitation_probability_max,wind_speed_10m_max"
  "&timezone=Asia%2FTaipei&forecast_days=7";

constexpr int8_t TFT_RST = 26;
constexpr int8_t TFT_RS = 27;
constexpr int8_t TFT_CS = 5;
constexpr int8_t TFT_LED = 25;
constexpr uint8_t DAYS = 7;
constexpr uint32_t FETCH_INTERVAL = 60000UL;
constexpr uint32_t PAGE_INTERVAL = 7000UL;

constexpr uint16_t BG = COLOR_DARKBLUE;
constexpr uint16_t HEADER = 0x18E3;
constexpr uint16_t PANEL = 0x2124;
constexpr uint16_t LINE = 0x52AA;

SPIClass vspi(VSPI);
TFT_22_ILI9225 tft(TFT_RST, TFT_RS, TFT_CS, TFT_LED);

String dates[DAYS];
int codes[DAYS];
float maxTemps[DAYS];
float minTemps[DAYS];
float rainChances[DAYS];
float maxWinds[DAYS];
float currentTemp = NAN;
float currentHumidity = NAN;
float currentRain = NAN;
float currentWind = NAN;
int currentCode = -1;
bool dataValid = false;
String statusText = "STARTING";
uint32_t lastFetch = 0;
uint32_t lastPage = 0;
uint8_t page = 0;

void text(uint16_t x, uint16_t y, const String &value, uint16_t color) {
  tft.drawText(x, y, value, color);
}

int arrayStartAfter(const String &json, const char *section, const char *key) {
  int sectionPos = json.indexOf(String("\"") + section + "\"");
  if (sectionPos < 0) return -1;
  int keyPos = json.indexOf(String("\"") + key + "\"", sectionPos);
  if (keyPos < 0) return -1;
  int colon = json.indexOf(':', keyPos);
  return json.indexOf('[', colon);
}

bool numberAfter(const String &json, const char *section, const char *key, float &value) {
  int sectionPos = json.indexOf(String("\"") + section + "\"");
  if (sectionPos < 0) return false;
  int keyPos = json.indexOf(String("\"") + key + "\"", sectionPos);
  if (keyPos < 0) return false;
  int colon = json.indexOf(':', keyPos);
  int start = colon + 1;
  while (start < json.length() && (json[start] == ' ' || json[start] == '\n')) ++start;
  int end = start;
  while (end < json.length() && (isDigit(json[end]) || json[end] == '.' || json[end] == '-')) ++end;
  if (end == start) return false;
  value = json.substring(start, end).toFloat();
  return true;
}

bool numberArray(const String &json, const char *section, const char *key, float output[]) {
  int pos = arrayStartAfter(json, section, key);
  if (pos < 0) return false;
  for (uint8_t i = 0; i < DAYS; ++i) {
    while (pos < json.length() && (json[pos] == '[' || json[pos] == ',' || json[pos] == ' ' || json[pos] == '\n')) ++pos;
    int end = pos;
    while (end < json.length() && (isDigit(json[end]) || json[end] == '.' || json[end] == '-')) ++end;
    if (end == pos) return false;
    output[i] = json.substring(pos, end).toFloat();
    pos = end;
  }
  return true;
}

bool stringArray(const String &json, const char *section, const char *key, String output[]) {
  int pos = arrayStartAfter(json, section, key);
  if (pos < 0) return false;
  for (uint8_t i = 0; i < DAYS; ++i) {
    int q1 = json.indexOf('"', pos + 1);
    int q2 = q1 < 0 ? -1 : json.indexOf('"', q1 + 1);
    if (q2 < 0) return false;
    output[i] = json.substring(q1 + 1, q2);
    pos = q2 + 1;
  }
  return true;
}

String dateShort(const String &date) {
  if (date.length() < 10) return "--/--";
  String result = date.substring(5, 10);
  result.replace('-', '/');
  return result;
}

String weatherName(int code) {
  if (code == 0) return "CLEAR";
  if (code <= 3) return "CLOUDY";
  if (code <= 48) return "FOG";
  if (code <= 67) return "RAIN";
  if (code <= 82) return "SHOWER";
  if (code >= 95) return "STORM";
  return "WEATHER";
}

void header(const String &title, const String &status) {
  tft.clear();
  tft.setBackgroundColor(BG);
  tft.setFont(Terminal6x8, MONOSPACE);
  tft.fillRectangle(0, 0, 175, 219, BG);
  tft.fillRectangle(0, 0, 175, 28, HEADER);
  text(28, 7, title, COLOR_WHITE);
  text(10, 205, status, status == "DATA OK" ? COLOR_CYAN : COLOR_ORANGE);
}

void drawCurrentPage() {
  header("KAOHSIUNG  CURRENT", statusText);
  tft.fillRectangle(10, 40, 165, 101, PANEL);
  tft.drawRectangle(10, 40, 165, 101, LINE);
  text(20, 53, "TEMPERATURE", COLOR_ORANGE);
  text(20, 71, "HUMIDITY", COLOR_CYAN);
  text(20, 89, "PRECIPITATION", COLOR_YELLOW);
  text(20, 107, "WIND SPEED", COLOR_GREENYELLOW);
  if (!dataValid) {
    text(112, 80, "NO DATA", COLOR_RED);
    return;
  }
  text(115, 53, String(static_cast<int>(round(currentTemp))) + " C", COLOR_WHITE);
  text(115, 71, String(static_cast<int>(round(currentHumidity))) + " %", COLOR_WHITE);
  text(115, 89, String(currentRain, 1) + " mm", COLOR_WHITE);
  text(115, 107, String(static_cast<int>(round(currentWind))) + " km/h", COLOR_WHITE);
  text(52, 133, weatherName(currentCode), COLOR_YELLOW);
  tft.drawLine(15, 158, 160, 158, LINE);
  text(34, 172, "7-DAY DETAILS NEXT", COLOR_CYAN);
}

void drawDailyPage() {
  header("7-DAY FORECAST", statusText);
  // Compact fixed columns for the 176-pixel-wide portrait screen.
  text(8, 35, "DATE", COLOR_YELLOW);
  text(47, 35, "T/M", COLOR_YELLOW);
  text(86, 35, "RAIN", COLOR_YELLOW);
  text(118, 35, "WIND", COLOR_YELLOW);
  text(150, 35, "SKY", COLOR_YELLOW);
  tft.drawLine(8, 47, 167, 47, LINE);
  if (!dataValid) {
    text(38, 100, "NO WEATHER DATA", COLOR_RED);
    return;
  }
  for (uint8_t i = 0; i < DAYS; ++i) {
    uint16_t y = 54 + i * 21;
    if (i % 2 == 0) tft.fillRectangle(8, y - 2, 167, y + 15, PANEL);
    text(8, y, dateShort(dates[i]), COLOR_WHITE);
    text(47, y, String(static_cast<int>(round(maxTemps[i]))) + "/" + String(static_cast<int>(round(minTemps[i]))), COLOR_ORANGE);
    text(86, y, String(static_cast<int>(round(rainChances[i]))) + "%", rainChances[i] >= 60 ? COLOR_RED : COLOR_CYAN);
    text(118, y, String(static_cast<int>(round(maxWinds[i]))), COLOR_WHITE);
    text(150, y, weatherName(codes[i]).substring(0, 3), codes[i] >= 51 ? COLOR_ORANGE : COLOR_YELLOW);
  }
}

bool fetchWeather() {
  if (WiFi.status() != WL_CONNECTED) {
    statusText = "WIFI OFFLINE";
    return false;
  }
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setTimeout(15000);
  http.setReuse(false);
  http.useHTTP10(true);
  if (!http.begin(client, WEATHER_URL)) {
    statusText = "HTTP BEGIN ERR";
    return false;
  }
  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    statusText = String("HTTP ") + httpCode;
    http.end();
    return false;
  }
  String json = http.getString();
  http.end();
  float t, h, p, w, c;
  float maxT[DAYS], minT[DAYS], rain[DAYS], winds[DAYS], dailyCodes[DAYS];
  bool ok = numberAfter(json, "current", "temperature_2m", t) &&
            numberAfter(json, "current", "relative_humidity_2m", h) &&
            numberAfter(json, "current", "precipitation", p) &&
            numberAfter(json, "current", "wind_speed_10m", w) &&
            numberAfter(json, "current", "weather_code", c) &&
            stringArray(json, "daily", "time", dates) &&
            numberArray(json, "daily", "temperature_2m_max", maxT) &&
            numberArray(json, "daily", "temperature_2m_min", minT) &&
            numberArray(json, "daily", "precipitation_probability_max", rain) &&
            numberArray(json, "daily", "wind_speed_10m_max", winds) &&
            numberArray(json, "daily", "weather_code", dailyCodes);
  if (!ok) {
    statusText = "PARSE ERROR";
    dataValid = false;
    return false;
  }
  currentTemp = t; currentHumidity = h; currentRain = p; currentWind = w;
  currentCode = static_cast<int>(round(c));
  for (uint8_t i = 0; i < DAYS; ++i) {
    maxTemps[i] = maxT[i]; minTemps[i] = minT[i];
    rainChances[i] = rain[i]; maxWinds[i] = winds[i];
    codes[i] = static_cast<int>(round(dailyCodes[i]));
  }
  statusText = "DATA OK";
  dataValid = true;
  return true;
}

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  statusText = "WIFI CONNECTING";
  drawCurrentPage();
  uint8_t attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts++ < 30) delay(500);
  if (WiFi.status() != WL_CONNECTED) statusText = "WIFI FAILED";
}

void drawPage() { if (page == 0) drawCurrentPage(); else drawDailyPage(); }

void setup() {
  Serial.begin(115200);
  vspi.begin(18, -1, 23, TFT_CS);
  tft.begin(vspi);
  pinMode(TFT_LED, OUTPUT);
  digitalWrite(TFT_LED, HIGH);
  tft.setOrientation(0);
  drawCurrentPage();
  connectWiFi();
  dataValid = fetchWeather();
  drawPage();
  lastFetch = millis();
  lastPage = millis();
}

void loop() {
  uint32_t now = millis();
  if (now - lastFetch >= FETCH_INTERVAL) {
    lastFetch = now;
    if (WiFi.status() != WL_CONNECTED) connectWiFi();
    dataValid = fetchWeather();
    drawPage();
  }
  if (now - lastPage >= PAGE_INTERVAL) {
    lastPage = now;
    page = page == 0 ? 1 : 0;
    drawPage();
  }
  delay(100);
}
