#include <Arduino.h>
#include <SPI.h>
#include <DHT.h>
#include <TFT_22_ILI9225.h>

// Existing wiring: ILI9225 SPI and DHT11
constexpr int8_t TFT_RST = 26;
constexpr int8_t TFT_RS  = 27;
constexpr int8_t TFT_CS  = 5;
constexpr int8_t TFT_MOSI = 23;
constexpr int8_t TFT_SCK  = 18;
constexpr int8_t TFT_LED  = 25;
constexpr uint8_t DHT_PIN = 14;

constexpr uint16_t BG = COLOR_DARKBLUE;
constexpr uint16_t PANEL = 0x2124;
constexpr uint16_t PANEL_EDGE = 0x52AA;
constexpr uint16_t TEXT = COLOR_WHITE;
constexpr uint16_t SOFT = COLOR_LIGHTBLUE;
constexpr uint16_t TEMP_COLOR = COLOR_ORANGE;
constexpr uint16_t HUMI_COLOR = COLOR_CYAN;

// Use the library's explicit software-SPI constructor so every signal maps
// exactly to the wiring, independent of the ESP32 board SPI defaults.
TFT_22_ILI9225 tft(TFT_RST, TFT_RS, TFT_CS, TFT_MOSI, TFT_SCK, TFT_LED);
DHT dht(DHT_PIN, DHT11);

float lastTemp = NAN;
float lastHumi = NAN;

void roundedPanel(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
  tft.fillRectangle(x + 3, y, x + w - 4, y + h - 1, PANEL);
  tft.fillRectangle(x, y + 3, x + w - 1, y + h - 4, PANEL);
  tft.fillCircle(x + 3, y + 3, 3, PANEL);
  tft.fillCircle(x + w - 4, y + 3, 3, PANEL);
  tft.fillCircle(x + 3, y + h - 4, 3, PANEL);
  tft.fillCircle(x + w - 4, y + h - 4, 3, PANEL);
  tft.drawRectangle(x + 3, y, x + w - 4, y + h - 1, PANEL_EDGE);
  tft.drawRectangle(x, y + 3, x + w - 1, y + h - 4, PANEL_EDGE);
}

// Custom colourful thermometer icon.
void drawThermometer(uint16_t cx, uint16_t cy) {
  tft.fillCircle(cx, cy + 18, 11, TEMP_COLOR);
  tft.fillRectangle(cx - 7, cy - 13, cx + 7, cy + 18, TEMP_COLOR);
  tft.fillCircle(cx, cy + 18, 5, COLOR_RED);
  tft.fillRectangle(cx - 3, cy - 8, cx + 3, cy + 18, COLOR_RED);
  tft.drawCircle(cx, cy + 18, 11, TEXT);
  tft.drawRectangle(cx - 7, cy - 13, cx + 7, cy + 18, TEXT);
  for (uint8_t i = 0; i < 3; ++i) {
    tft.drawLine(cx + 10, cy - 7 + i * 8, cx + 15, cy - 7 + i * 8, SOFT);
  }
}

// Custom colourful water-drop icon.
void drawDrop(uint16_t cx, uint16_t cy) {
  tft.fillTriangle(cx, cy - 20, cx - 16, cy + 8, cx + 16, cy + 8, HUMI_COLOR);
  tft.fillCircle(cx, cy + 7, 16, HUMI_COLOR);
  tft.fillTriangle(cx - 2, cy - 10, cx - 8, cy + 3, cx - 2, cy + 1, COLOR_WHITE);
  tft.drawTriangle(cx, cy - 20, cx - 16, cy + 8, cx + 16, cy + 8, TEXT);
}

void drawFixedLayout() {
  tft.clear();
  tft.setBackgroundColor(BG);
  tft.setFont(Terminal6x8, MONOSPACE);
  tft.drawText(52, 8, "ENVIRONMENT", TEXT);
  tft.drawText(55, 32, "DHT11  LIVE", SOFT);

  roundedPanel(8, 50, 76, 112);
  roundedPanel(92, 50, 76, 112);
  drawThermometer(25, 78);
  drawDrop(126, 78);

  tft.setFont(Terminal6x8, MONOSPACE);
  tft.drawText(16, 115, "TEMP", TEMP_COLOR);
  tft.drawText(98, 115, "HUMID", HUMI_COLOR);
  tft.drawLine(16, 174, 160, 174, PANEL_EDGE);
  tft.drawText(40, 190, "ESP32 / ILI9225", SOFT);
}

void clearValueArea(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
  tft.fillRectangle(x, y, x + w - 1, y + h - 1, PANEL);
}

void drawValues(float temp, float humi) {
  // Values are always drawn at fixed baselines and inside fixed-width areas.
  clearValueArea(13, 128, 66, 35);
  clearValueArea(97, 128, 66, 35);
  tft.setFont(Terminal12x16, MONOSPACE);
  char tempText[10];
  char humiText[10];
  if (isnan(temp) || isnan(humi)) {
    tft.drawText(43, 136, "--", COLOR_RED);
    tft.drawText(127, 136, "--", COLOR_RED);
    return;
  }
  snprintf(tempText, sizeof(tempText), "%2dC", static_cast<int>(round(temp)));
  snprintf(humiText, sizeof(humiText), "%2d%%", static_cast<int>(round(humi)));
  tft.drawText(17, 136, tempText, TEXT);
  tft.drawText(101, 136, humiText, TEXT);
}

void setup() {
  tft.begin();
  // TFT_22_ILI9225 does not drive the backlight pin on ESP32 internally.
  // The module's LED/BL pin is wired to GPIO25, so turn it on explicitly.
  pinMode(TFT_LED, OUTPUT);
  digitalWrite(TFT_LED, HIGH);
  tft.setOrientation(0);
  dht.begin();
  drawFixedLayout();
  drawValues(NAN, NAN);
}

void loop() {
  float temp = dht.readTemperature();
  float humi = dht.readHumidity();
  if (!isnan(temp) && !isnan(humi)) {
    lastTemp = temp;
    lastHumi = humi;
  }
  drawValues(lastTemp, lastHumi);
  delay(2000);
}
