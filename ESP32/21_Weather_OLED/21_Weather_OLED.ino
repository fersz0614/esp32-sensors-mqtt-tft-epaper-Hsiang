#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <U8g2lib.h>  

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

String weather = "";
String temp = "";
String humd = "";

// WiFi 設定
char ssid[] = "H"; 
char password[] = "YOUR_WIFI_PASSWORD"; 
char url[] = "https://opendata.cwa.gov.tw/api/v1/rest/datastore/O-A0001-001?Authorization=YOUR_CWA_API_KEY";

void setup() {
  Serial.begin(115200);
  u8g2.begin();
  u8g2.enableUTF8Print();
  u8g2.setFont(u8g2_font_unifont_t_chinese1);
  u8g2.setFontPosTop();
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
    DynamicJsonDocument doc(payload.length() * 2);
    deserializeJson(doc, payload);

    JsonArray stations = doc["records"]["Station"].as<JsonArray>();

    for (JsonObject station : stations) {
      if (station["StationName"] == "前鎮氣象站") {
        JsonObject elements = station["WeatherElement"];

        weather = elements["Weather"].as<String>();
        temp = elements["AirTemperature"].as<String>();
        float humdValue = elements["RelativeHumidity"].as<float>();
        humd = String(humdValue, 0); // 轉成整數百分比

        Serial.println("前鎮氣象站 天氣=" + weather);
        Serial.println("前鎮氣象站 溫度=" + temp + "°C");
        Serial.println("前鎮氣象站 濕度=" + humd + "%");

        // 顯示到 OLED
        u8g2.clearBuffer();
        u8g2.setCursor(0, 10);
        u8g2.print("前鎮氣象站");

        u8g2.setCursor(0, 25);
        u8g2.print("天氣=" + weather);

        u8g2.setCursor(0, 40);
        u8g2.print("溫度=" + temp + " °C");

        u8g2.setCursor(0, 55);
        u8g2.print("濕度=" + humd + " %");

        u8g2.sendBuffer();
        break;
      }
    }
  }
  http.end();
  delay(30000); // 每 30 秒更新一次
}
