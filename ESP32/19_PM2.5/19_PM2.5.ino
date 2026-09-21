#include <WiFi.h>//WiFi
#include <HTTPClient.h>//瀏覽器
#include <ArduinoJson.h>//請先安裝ArduinoJson程式庫

char ssid[] = "H"; //請修改為您連線的網路名稱
char password[] = "YOUR_WIFI_PASSWORD"; //請修改為您連線的網路密碼
char url[] = "https://data.moenv.gov.tw/api/v2/aqx_p_02?api_key=YOUR_API_KEY&limit=100"; //讀取的網址及授權密碼

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.print("開始連線到無線網路SSID:");
  Serial.println(ssid);
  //1.設定WiFi模式
  WiFi.mode(WIFI_STA);
  //2.啟動WiFi連線
  WiFi.begin(ssid, password);
  //3.檢查連線狀態
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("連線完成");
}

void loop() {
  //4.啟動網頁連線
  Serial.println("啟動網頁連線");
  HTTPClient http;
  http.begin(url);
  int httpCode = http.GET();
  Serial.print("httpCode=");
  Serial.println(httpCode);
  //5.檢查網頁連線是否正常
  if (httpCode == HTTP_CODE_OK) {
    //6.取得網頁內容
    String payload = http.getString();
    //Serial.print("payload="); //可以不顯示
    //7.將資料顯示在螢幕上
    //Serial.println(payload);//可以不顯示
    //JSON格式解析
    DynamicJsonDocument AQIJson(payload.length() * 2); //宣告一個JSON文件，名稱為AQIJson
    deserializeJson(AQIJson, payload);//解析網頁內容payload為JSON格式，存放在AQIJson內
    for (int i = 0; i < AQIJson.size(); i++) {
	  // 瀏覽records內的所有紀錄，直到找到site=="橋頭"
      if (AQIJson[i]["site"] == "中壢") {        
        String AQI = AQIJson[i]["pm25"];
        Serial.println("中壢 PM2.5=" + AQI);
		    break;
      }
    }
  }
  http.end();
  delay(30000);
}