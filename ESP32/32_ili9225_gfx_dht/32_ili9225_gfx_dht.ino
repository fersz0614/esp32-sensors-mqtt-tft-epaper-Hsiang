#include <SPI.h>
#include <Adafruit_GFX.h>
#include <SimpleDHT.h>

const int TFT_SCK=18, TFT_MOSI=23, TFT_CS=27, TFT_DC=26, TFT_RST=25;
const int DHT_PIN=14, LIGHT_PIN=33;
#define SPI_FREQUENCY 8000000UL
const unsigned long SENSOR_INTERVAL = 5000UL;

void xfer16(uint16_t v){ SPI.transfer(v>>8); SPI.transfer(v); }

class ILI9225_GFX : public Adafruit_GFX {
public:
  ILI9225_GFX():Adafruit_GFX(176,220){}
  void begin(){
    pinMode(TFT_CS,OUTPUT); pinMode(TFT_DC,OUTPUT); pinMode(TFT_RST,OUTPUT);
    digitalWrite(TFT_CS,HIGH); SPI.begin(TFT_SCK,-1,TFT_MOSI,TFT_CS);
    digitalWrite(TFT_RST,LOW); delay(50); digitalWrite(TFT_RST,HIGH); delay(100);
    cmd(0x01,0x011C); cmd(0x02,0x0100); cmd(0x03,0x1030); cmd(0x08,0x0808);
    cmd(0x0F,0x0801); cmd(0x20,0); cmd(0x21,0); cmd(0x10,0);
    cmd(0x11,0x1B41); cmd(0x12,0x200E); cmd(0x13,0x0D00); cmd(0x14,0x0020);
    delay(50); cmd(0x10,0x0F00); delay(50); cmd(0x11,0x1B41);
    cmd(0x12,0x200E); cmd(0x13,0x0D00); cmd(0x14,0x0020);
    cmd(0x30,0); cmd(0x31,0x00DB); cmd(0x32,0); cmd(0x33,0);
    cmd(0x34,0x00DB); cmd(0x35,0); cmd(0x36,0x00AF); cmd(0x37,0);
    cmd(0x38,0x00DB); cmd(0x39,0); cmd(0x07,0x1017); delay(50);
  }
  void fillFast(int x,int y,int w,int h,uint16_t c){
    if(x<0){w+=x;x=0;} if(y<0){h+=y;y=0;} if(x+w>176)w=176-x; if(y+h>220)h=220-y;
    if(w<=0||h<=0)return; window(x,y,x+w-1,y+h-1);
    for(long i=0;i<(long)w*h;i++)xfer16(c); digitalWrite(TFT_CS,HIGH);
  }
  void drawPixel(int16_t x,int16_t y,uint16_t c) override { if(x<0||y<0||x>=176||y>=220)return; fillFast(x,y,1,1,c); }
private:
  void cmd(uint8_t c,uint16_t d){ digitalWrite(TFT_CS,LOW); digitalWrite(TFT_DC,LOW); SPI.transfer(c); digitalWrite(TFT_DC,HIGH); xfer16(d); digitalWrite(TFT_CS,HIGH); }
  void window(int x0,int y0,int x1,int y1){ cmd(0x36,x0);cmd(0x37,x1);cmd(0x38,y0);cmd(0x39,y1);cmd(0x20,x0);cmd(0x21,y0);digitalWrite(TFT_CS,LOW);digitalWrite(TFT_DC,LOW);SPI.transfer(0x22);digitalWrite(TFT_DC,HIGH); }
};

ILI9225_GFX tft;
SimpleDHT11 dht11(DHT_PIN);
int tempC=0, hum=0, light=0; bool ok=false; unsigned long lastRead=0;
const uint16_t BG=0xFFFF, PANEL=0x2145, WHITE=0xFFFF, RED=0xF8A6, BLUE=0x45BF, CYAN=0x5DFF, ORANGE=0xFD20, GREEN=0x45A5, YELLOW=0xFFE0;

void drawThermometer(int x,int y){ tft.fillCircle(x,y+18,10,RED); tft.fillRect(x-4,y-15,8,34,RED); tft.fillRect(x-2,y-10,4,28,ORANGE); }
void drawDrop(int x,int y){ tft.fillTriangle(x,y-22,x-14,y+7,x+14,y+7,BLUE); tft.fillCircle(x,y+4,14,BLUE); tft.fillCircle(x-4,y-6,2,WHITE); }
void drawSun(int x,int y){
  tft.fillCircle(x,y,9,YELLOW); tft.drawCircle(x,y,12,ORANGE);
  for(int i=0;i<8;i++){ float a=i*0.785398f; tft.drawLine(x+(int)(15*cos(a)),y+(int)(15*sin(a)),x+(int)(20*cos(a)),y+(int)(20*sin(a)),ORANGE); }
}

void drawScreen(){
  tft.fillScreen(BG); tft.fillRect(0,0,176,7,CYAN);
  tft.setTextColor(0x10A2); tft.setTextSize(2); tft.setCursor(13,15); tft.print("ENV");
  tft.setTextSize(1); tft.setCursor(125,19); tft.print("DHT11");
  tft.fillRoundRect(9,34,158,55,6,PANEL); drawThermometer(31,58);
  tft.setTextColor(RED); tft.setTextSize(1); tft.setCursor(53,43); tft.print("TEMPERATURE");
  tft.setTextColor(WHITE); tft.setTextSize(3); tft.setCursor(55,56); if(ok)tft.print(tempC);else tft.print("--");
  tft.setTextSize(2); tft.setCursor(119,63); tft.setTextColor(ORANGE); tft.print("C");
  tft.fillRoundRect(9,94,158,55,6,PANEL); drawDrop(31,118);
  tft.setTextColor(CYAN); tft.setTextSize(1); tft.setCursor(53,103); tft.print("HUMIDITY");
  tft.setTextColor(WHITE); tft.setTextSize(3); tft.setCursor(55,116); if(ok)tft.print(hum);else tft.print("--");
  tft.setTextSize(2); tft.setCursor(119,123); tft.setTextColor(CYAN); tft.print("%");
  tft.fillRoundRect(9,154,158,55,6,PANEL); drawSun(31,179);
  tft.setTextColor(GREEN); tft.setTextSize(1); tft.setCursor(53,163); tft.print("LIGHT");
  tft.setTextColor(WHITE); tft.setTextSize(3); tft.setCursor(55,176); tft.print(light);
  tft.setTextSize(2); tft.setCursor(119,183); tft.setTextColor(GREEN); tft.print("%");
}

// 只更新數字區域，不重畫背景、卡片、標題與圖示
void updateValues(){
  // 先用 Adafruit GFX 清除完整的舊溫度文字，再寫入新數值
  tft.fillRect(50,51,80,38,PANEL);
  tft.setTextColor(WHITE); tft.setTextSize(3); tft.setCursor(55,56);
  if(ok) tft.print(tempC); else tft.print("--");
  tft.setTextSize(2); tft.setCursor(119,63); tft.setTextColor(ORANGE); tft.print("C");

  // 先用 Adafruit GFX 清除完整的舊濕度文字，再寫入新數值
  tft.fillRect(50,111,80,38,PANEL);
  tft.setTextColor(WHITE); tft.setTextSize(3); tft.setCursor(55,116);
  if(ok) tft.print(hum); else tft.print("--");
  tft.setTextSize(2); tft.setCursor(119,123); tft.setTextColor(CYAN); tft.print("%");

  tft.fillRect(50,171,80,38,PANEL);
  tft.setTextColor(WHITE); tft.setTextSize(3); tft.setCursor(55,176); tft.print(light);
  tft.setTextSize(2); tft.setCursor(119,183); tft.setTextColor(GREEN); tft.print("%");
}

void readDHT(){
  byte t=0,h=0; ok=(dht11.read(&t,&h,NULL)==SimpleDHTErrSuccess);
  if(ok){tempC=t;hum=h;}
  light=constrain(map(analogRead(LIGHT_PIN),0,4095,0,100),0,100);
}

void setup(){ Serial.begin(115200); SPI.beginTransaction(SPISettings(SPI_FREQUENCY,MSBFIRST,SPI_MODE0)); tft.begin(); readDHT(); drawScreen(); SPI.endTransaction(); }
void loop(){ if(millis()-lastRead>=SENSOR_INTERVAL){lastRead=millis(); readDHT(); SPI.beginTransaction(SPISettings(SPI_FREQUENCY,MSBFIRST,SPI_MODE0)); updateValues(); SPI.endTransaction();} delay(10); }
