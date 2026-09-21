#include <SPI.h>
#include "epd2in9b_V4.h"
#include "logo_three_color.h"

constexpr uint16_t IMAGE_BYTES = EPD_WIDTH * EPD_HEIGHT / 8;
constexpr uint16_t LOGO_Y = (EPD_HEIGHT - 128) / 2;

uint8_t blackImage[IMAGE_BYTES];
uint8_t redImage[IMAGE_BYTES];
Epd epd;

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("36_epaper: three-color logo test");

  if (epd.Init() != 0) {
    Serial.println("e-Paper init failed");
    return;
  }

  memset(blackImage, 0xFF, sizeof(blackImage));
  memset(redImage, 0xFF, sizeof(redImage));

  for (uint16_t row = 0; row < 128; ++row) {
    const uint16_t dst = (LOGO_Y + row) * (EPD_WIDTH / 8);
    const uint16_t src = row * (EPD_WIDTH / 8);
    memcpy(&blackImage[dst], &LOGO_BLACK[src], EPD_WIDTH / 8);
    memcpy(&redImage[dst], &LOGO_RED[src], EPD_WIDTH / 8);
  }

  epd.Clear();
  epd.Display(blackImage, redImage);
  epd.Sleep();
  Serial.println("36_epaper: logo display complete");
}

void loop() {}
