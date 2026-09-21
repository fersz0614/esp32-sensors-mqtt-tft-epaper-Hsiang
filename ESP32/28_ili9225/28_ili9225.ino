#include <Arduino.h>
#include <SPI.h>

// ILI9225 176x220 SPI display wiring
constexpr uint8_t TFT_SCK  = 18;
constexpr uint8_t TFT_MOSI = 23;
constexpr uint8_t TFT_CS   = 5;
constexpr uint8_t TFT_RS   = 27;  // RS/DC: command or data
constexpr uint8_t TFT_RST  = 26;
constexpr uint8_t TFT_LED  = 25;

constexpr uint16_t TFT_WIDTH = 176;
constexpr uint16_t TFT_HEIGHT = 220;

SPIClass tftSpi(VSPI);

void tftCommand(uint8_t command) {
  digitalWrite(TFT_RS, LOW);
  digitalWrite(TFT_CS, LOW);
  tftSpi.transfer(command);
  digitalWrite(TFT_CS, HIGH);
}

void tftData(uint16_t data) {
  digitalWrite(TFT_RS, HIGH);
  digitalWrite(TFT_CS, LOW);
  tftSpi.transfer16(data);
  digitalWrite(TFT_CS, HIGH);
}

void tftRegister(uint8_t command, uint16_t data) {
  tftCommand(command);
  tftData(data);
}

void setAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
  // ILI9225 register order is end then start for each address window pair.
  tftRegister(0x36, x1);  // horizontal address end
  tftRegister(0x37, x0);  // horizontal address start
  tftRegister(0x38, y1);  // vertical address end
  tftRegister(0x39, y0);  // vertical address start
  tftRegister(0x20, x0);  // current horizontal address
  tftRegister(0x21, y0);  // current vertical address
  tftCommand(0x22);       // start GRAM write
}

void fillScreen(uint16_t color) {
  setAddressWindow(0, 0, TFT_WIDTH - 1, TFT_HEIGHT - 1);
  digitalWrite(TFT_RS, HIGH);
  digitalWrite(TFT_CS, LOW);
  for (uint32_t i = 0; i < static_cast<uint32_t>(TFT_WIDTH) * TFT_HEIGHT; ++i) {
    tftSpi.transfer16(color);
  }
  digitalWrite(TFT_CS, HIGH);
}

// Small 5x7 font for the HELLO test message.
const uint8_t helloFont[5][7] = {
  {0b10001, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001}, // H
  {0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b11111}, // E
  {0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b11111}, // L
  {0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b11111}, // L
  {0b01110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110}  // O
};

void drawHello(uint16_t x, uint16_t y, uint16_t scale, uint16_t color) {
  constexpr uint16_t glyphWidth = 5;
  constexpr uint16_t glyphHeight = 7;
  constexpr uint16_t gap = 1;
  setAddressWindow(x, y, x + 5 * (glyphWidth + gap) * scale - gap * scale - 1,
                   y + glyphHeight * scale - 1);
  digitalWrite(TFT_RS, HIGH);
  digitalWrite(TFT_CS, LOW);
  for (uint8_t row = 0; row < glyphHeight; ++row) {
    for (uint8_t sy = 0; sy < scale; ++sy) {
      for (uint8_t letter = 0; letter < 5; ++letter) {
        for (uint8_t col = 0; col < glyphWidth; ++col) {
          uint16_t pixel = (helloFont[letter][row] & (1 << (glyphWidth - 1 - col)))
                             ? color : 0x0000;
          for (uint8_t sx = 0; sx < scale; ++sx) tftSpi.transfer16(pixel);
        }
        for (uint8_t sx = 0; sx < scale && letter < 4; ++sx) tftSpi.transfer16(0x0000);
      }
    }
  }
  digitalWrite(TFT_CS, HIGH);
}

void initILI9225() {
  digitalWrite(TFT_RST, LOW);
  delay(20);
  digitalWrite(TFT_RST, HIGH);
  delay(50);

  tftRegister(0x01, 0x011C);
  tftRegister(0x02, 0x0100);
  tftRegister(0x03, 0x1030);
  tftRegister(0x08, 0x0808);
  tftRegister(0x0C, 0x0000);
  tftRegister(0x0F, 0x0E01);
  tftRegister(0x20, 0x0000);
  tftRegister(0x21, 0x0000);
  tftRegister(0x10, 0x0800);
  tftRegister(0x11, 0x1038);
  delay(50);
  tftRegister(0x12, 0x1121);
  tftRegister(0x13, 0x0066);
  tftRegister(0x14, 0x5F60);
  delay(50);
  tftRegister(0x10, 0x0E00);
  delay(50);
  tftRegister(0x11, 0x103B);
  delay(50);

  tftRegister(0x30, 0x0000);
  tftRegister(0x31, 0x00DB);
  tftRegister(0x32, 0x0000);
  tftRegister(0x33, 0x0000);
  tftRegister(0x34, 0x00DB);
  tftRegister(0x35, 0x0000);
  tftRegister(0x36, 0x0000);
  tftRegister(0x37, 0x00AF);
  tftRegister(0x38, 0x0000);
  tftRegister(0x39, 0x00DB);
  tftRegister(0x07, 0x1017);
  delay(50);
}

void setup() {
  pinMode(TFT_CS, OUTPUT);
  pinMode(TFT_RS, OUTPUT);
  pinMode(TFT_RST, OUTPUT);
  pinMode(TFT_LED, OUTPUT);
  digitalWrite(TFT_CS, HIGH);
  digitalWrite(TFT_RS, HIGH);
  digitalWrite(TFT_LED, HIGH);

  tftSpi.begin(TFT_SCK, -1, TFT_MOSI, TFT_CS);
  tftSpi.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
  initILI9225();
  fillScreen(0x001F);  // blue background for an obvious wiring/display test

  // Centered, large white HELLO on a blue background.
  drawHello(18, 98, 4, 0xFFFF);
  tftSpi.endTransaction();
}

void loop() {
  // Static display test: keep HELLO on screen.
  delay(1000);
}
