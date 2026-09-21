#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <U8g2lib.h>  // OLED 螢幕解析度為128*64
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);

String AQI ="";
// WiFi 設定
char ssid[] = "H"; //請修改為您連線的網路名稱
char password[] = "YOUR_WIFI_PASSWORD"; //請修改為您連線的網路密碼
char url[] = "https://data.moenv.gov.tw/api/v2/aqx_p_02?api_key=YOUR_API_KEY&limit=100"; //讀取的網址及授權密碼

void setup() {
  Serial.begin(115200);
  u8g2.begin();                                // 初始化
  u8g2.enableUTF8Print();                      // 啟用 UTF8字集
  u8g2.setFont(u8g2_font_unifont_t_chinese1);  // 設定使用中文字形
  u8g2.setFontPosTop();                        // 座標從上開始
  delay(1000);

  Serial.print("開始連線到無線網路SSID:");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("連線完成");
}

void loop() {
  Serial.println("啟動網頁連線");
  HTTPClient http;
  http.begin(url);
  int httpCode = http.GET();
  Serial.print("httpCode=");
  Serial.println(httpCode);

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    DynamicJsonDocument AQIJson(payload.length() * 2);
    deserializeJson(AQIJson, payload);

    for (int i = 0; i < AQIJson.size(); i++) {
      if (AQIJson[i]["site"] == "中壢") {        
        AQI = AQIJson[i]["pm25"].as<String>();
        Serial.println("中壢 PM2.5=" + AQI);

        // 顯示到 OLED
        u8g2.clearBuffer();   

        u8g2.setCursor(0, 5);                     
        u8g2.print("高屏澎勞動部分署");    

        u8g2.setCursor(0, 25);                     
        u8g2.print("中壢區空氣品質");           

        u8g2.setCursor(0, 45);                    
        u8g2.print("PM2.5=" + AQI + " ug/m3");  

        u8g2.sendBuffer();  

        break;
      }
    }
  }
  http.end();
  delay(30000); // 每 30 秒更新一次
}
