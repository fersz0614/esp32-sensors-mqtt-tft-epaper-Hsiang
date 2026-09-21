//DHT宣告
#include <SimpleDHT.h>
int pinDHT11 = 19;
SimpleDHT11 dht11(pinDHT11);

//LED宣告
int R=25;
int G=26;
int B=27;

//OLED宣告
#include "Wire.h"
#include "U8g2lib.h"  //OLED 螢幕解析度為128*64
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);
int R=25;
int G=26;
int B=27;
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  u8g2.begin();                                //初始化
  u8g2.enableUTF8Print();                      //啟用 UTF8字集
  u8g2.setFont(u8g2_font_unifont_t_chinese1);  //設定使用中文字形
  u8g2.setFontPosTop();//座標從上開始
  pinMode(R,OUTPUT);pinMode(G,OUTPUT);pinMode(B,OUTPUT);
}

void loop() {
  // start working...
  Serial.println("=================================");
  Serial.println("Sample DHT11...");
  
  // read without samples.byte =>0~255整數,int=>20億左右,long 天文
  byte temperature = 0;
  byte humidity = 0;
  //如果讀取錯誤,就印出錯誤訊息並返回return,返回開始地方
  int err = SimpleDHTErrSuccess;
  if ((err = dht11.read(&temperature, &humidity, NULL)) != SimpleDHTErrSuccess) {
    Serial.print("Read DHT11 failed, err="); Serial.print(SimpleDHTErrCode(err));
    Serial.print(","); Serial.println(SimpleDHTErrDuration(err)); delay(1000);
    return;
  }
  
 Serial.println("Sample OK: " + (String)temperature +" *C, "+ (String)humidity +" H");
   //Serial.print("Sample OK: ");
   //Serial.print((int)temperature); Serial.print(" *C, "); 
   //Serial.print((int)humidity); Serial.println(" H");
   // (int)強制轉型為整數,
   // DHT11 sampling rate is 1HZ.
   if(humidity <= 60){analogWrite(R,0);analogWrite(G,255);analogWrite(B,0);}
   if(humidity >= 61 and humidity <= 70){analogWrite(R,255);analogWrite(G,255);analogWrite(B,0);}
   if(humidity >= 71 and humidity <= 80){analogWrite(R,255);analogWrite(G,187);analogWrite(B,0);}
   if(humidity >= 81 and humidity <= 90){analogWrite(R,0);analogWrite(G,255);analogWrite(B,204);}
   if(humidity >= 91 and humidity <= 100){analogWrite(R,255);analogWrite(G,0);analogWrite(B,0);}

  u8g2.clearBuffer();                        //顯示前清除螢幕
  u8g2.setCursor(0, 5);                     //移動游標
  u8g2.print("高屏澎勞動部分署");           //寫入文字

  u8g2.setCursor(0, 25);                     //移動游標
  u8g2.print("溫度："+(String)temperature+" C");           //寫入文字

  u8g2.setCursor(0, 45);                    //移動游標
  u8g2.print("濕度：" + (String)humidity + " %");  //寫入文字

  //u8g2.drawLine(0, 11, 30, 11);  //劃線從0,11->30,11

  u8g2.sendBuffer();  //送到螢幕顯示
  delay(1000);

}
