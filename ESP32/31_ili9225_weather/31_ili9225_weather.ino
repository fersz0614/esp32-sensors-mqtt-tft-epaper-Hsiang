#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <TFT_22_ILI9225.h>

constexpr char WIFI_SSID[] = "H";
constexpr char WIFI_PASSWORD[] = "YOUR_WIFI_PASSWORD";

// Simple current-weather request for Kaohsiung; no API key is required.
constexpr char WEATHER_URL[] =
  "https://api.open-meteo.com/v1/forecast?latitude=22.6273&longitude=120.3014"
  "&current=temperature_2m,relative_humidity_2m,precipitation,wind_speed_10m,weather_code"
  "&timezone=Asia%2FTaipei";

constexpr int8_t TFT_RST = 26;
constexpr int8_t TFT_RS = 27;
constexpr int8_t TFT_CS = 5;
constexpr int8_t TFT_LED = 25;
constexpr uint32_t UPDATE_INTERVAL = 60000UL;

constexpr uint16_t BG = COLOR_DARKBLUE;
constexpr uint16_t HEADER = 0x18E3;
constexpr uint16_t PANEL = 0x2124;
constexpr uint16_t LINE = 0x52AA;

SPIClass vspi(VSPI);
TFT_22_ILI9225 tft(TFT_RST, TFT_RS, TFT_CS, TFT_LED);

float temperature = NAN;
float humidity = NAN;
float precipitation = NAN;
float windSpeed = NAN;
int weatherCode = -1;
String statusText = "STARTING";
uint32_t lastUpdate = 0;

bool getJsonNumber(const String &json, const char *key, float &value) {
  // Open-Meteo returns the units object before the current object.  Both can
  // contain the same field names, so only search after the "current" object.
  int currentPos = json.indexOf("\"current\"");
  if (currentPos < 0) return false;
  int keyPos = json.indexOf(String("\"") + key + "\"", currentPos);
  if (keyPos < 0) return false;
  int colon = json.indexOf(':', keyPos);
  if (colon < 0) return false;
  int start = colon + 1;
  while (start < json.length() && (json[start] == ' ' || json[start] == '\n')) ++start;
  int end = start;
  while (end < json.length() && (isDigit(json[end]) || json[end] == '.' || json[end] == '-')) ++end;
  if (end == start) return false;
  value = json.substring(start, end).toFloat();
  return true;
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

void text(uint16_t x, uint16_t y, const String &value, uint16_t color) {
  tft.drawText(x, y, value, color);
}

void drawScreen() {
  tft.clear();
  tft.setBackgroundColor(BG);
  tft.setFont(Terminal6x8, MONOSPACE);
  tft.fillRectangle(0, 0, 175, 219, BG);
  tft.fillRectangle(0, 0, 175, 28, HEADER);
  text(34, 7, "KAOHSIUNG WEATHER", COLOR_WHITE);
  text(52, 35, "CURRENT", COLOR_CYAN);

  tft.fillRectangle(8, 52, 167, 91, PANEL);
  tft.drawRectangle(8, 52, 167, 91, LINE);
  tft.fillRectangle(8, 153, 82, 199, PANEL);
  tft.fillRectangle(93, 153, 167, 199, PANEL);
  tft.drawRectangle(8, 153, 82, 199, LINE);
  tft.drawRectangle(93, 153, 167, 199, LINE);

  text(18, 62, "TEMPERATURE", COLOR_ORANGE);
  text(18, 80, "HUMIDITY", COLOR_CYAN);
  text(18, 98, "WEATHER", COLOR_YELLOW);
  text(18, 116, "RAIN / WIND", COLOR_GREENYELLOW);

  if (isnan(temperature)) {
    text(110, 80, "-- C", COLOR_RED);
    text(110, 98, "-- %", COLOR_RED);
    text(110, 116, "NO DATA", COLOR_RED);
  } else {
    text(110, 62, String(static_cast<int>(round(temperature))) + " C", COLOR_WHITE);
    text(110, 80, String(static_cast<int>(round(humidity))) + " %", COLOR_WHITE);
    text(110, 98, weatherName(weatherCode), COLOR_YELLOW);
    text(110, 116, String(static_cast<int>(round(precipitation))) + " mm", COLOR_CYAN);
  }

  text(20, 163, "RAIN", COLOR_CYAN);
  text(105, 163, "WIND", COLOR_GREENYELLOW);
  if (isnan(precipitation)) {
    text(20, 181, "-- mm", COLOR_RED);
    text(105, 181, "-- km/h", COLOR_RED);
  } else {
    text(20, 181, String(static_cast<int>(round(precipitation))) + " mm", COLOR_WHITE);
    text(105, 181, String(static_cast<int>(round(windSpeed))) + " km/h", COLOR_WHITE);
  }
  text(10, 207, statusText, statusText == "DATA OK" ? COLOR_CYAN : COLOR_ORANGE);
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
  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    statusText = String("HTTP ") + code;
    http.end();
    return false;
  }
  String json = http.getString();
  http.end();

  float t, h, p, w, c;
  bool ok = getJsonNumber(json, "temperature_2m", t) &&
            getJsonNumber(json, "relative_humidity_2m", h) &&
            getJsonNumber(json, "precipitation", p) &&
            getJsonNumber(json, "wind_speed_10m", w) &&
            getJsonNumber(json, "weather_code", c);
  if (!ok) {
    statusText = "PARSE ERROR";
    return false;
  }
  temperature = t;
  humidity = h;
  precipitation = p;
  windSpeed = w;
  weatherCode = static_cast<int>(round(c));
  statusText = "DATA OK";
  return true;
}

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  statusText = "WIFI CONNECTING";
  drawScreen();
  uint8_t attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts++ < 30) delay(500);
  if (WiFi.status() != WL_CONNECTED) statusText = "WIFI FAILED";
}

void setup() {
  Serial.begin(115200);
  vspi.begin(18, -1, 23, TFT_CS);
  tft.begin(vspi);
  pinMode(TFT_LED, OUTPUT);
  digitalWrite(TFT_LED, HIGH);
  tft.setOrientation(0);
  drawScreen();
  connectWiFi();
  fetchWeather();
  drawScreen();
  lastUpdate = millis();
}

void loop() {
  if (millis() - lastUpdate >= UPDATE_INTERVAL) {
    lastUpdate = millis();
    if (WiFi.status() != WL_CONNECTED) connectWiFi();
    fetchWeather();
    drawScreen();
  }
  delay(100);
}
