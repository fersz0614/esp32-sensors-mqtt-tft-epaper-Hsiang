# 🚀 ESP32 Sensors & Display Hub (MQTT + TFT + E-Paper)

本專案整合 **ESP32 微控制器**、多種感測器數據採集、**MQTT 通訊協定**傳輸，以及 **TFT 螢幕 / E-Paper（電子紙）**的即時數據顯示。

---

## 📺 專案展示 (Live Demo)

> 💡 **成果展示與靜態網站展示**

- **展示網頁 / 儀表板**：[點此造訪 Live Demo (GitHub Pages)](https://fersz0614.github.io/esp32-sensors-mqtt-tft-epaper-Hsiang/)
- **GitHub Repository**：[esp32-sensors-mqtt-tft-epaper-Hsiang](https://github.com/fersz0614/esp32-sensors-mqtt-tft-epaper-Hsiang)

### 硬體運作實拍

![電子紙 MQTT 展示](ESP32/成果照片/38_epaper_mqtt-2.png)
![DHT11 OLED ThingSpeak 展示](ESP32/成果照片/23_dht_light_oled_thingspeak.png)
![TFT MQTT 展示](ESP32/成果照片/33_ili_mqtt.jpeg)

---

## 🌟 專案亮點與特色

1. **程式與成果分享**
   - 開放式架構，便於學習、除錯與模組化移植。
   - 收錄從 GPIO、感測器、OLED 到 TFT、電子紙與 IoT 的循序範例。
   - 每個範例皆可獨立開啟、編譯與上傳。
2. **MQTT 協定與雙顯示整合**
   - 以 MQTT 傳送遙測資料並接收遠端控制指令。
   - 結合高刷新率 TFT 與低功耗 E-Paper 顯示即時狀態。
3. **完整的學習路徑**
   - 從 LED、DHT11、PIR、超音波開始，逐步延伸到 HTTP API、ThingSpeak、LINE 與 MQTT。
   - 提供成果照片、函式庫與每隻程式的功能索引。

---

## 🛠️ 系統架構與硬體需求

### 硬體清單 (Hardware)

- ESP32 開發板
- TFT LCD 顯示器（ILI9225 或其他相容模組）
- E-Paper 電子紙模組（GxEPD2 / Waveshare 相容）
- DHT11 / DHT22 溫濕度感測器
- 光敏電阻、PIR 人體感測器、超音波感測器
- RGB LED、蜂鳴器、繼電器或風扇等輸出元件
- USB 資料線與適合的外接電源

### 系統資料流

```text
感測器 → ESP32 → Wi‑Fi → MQTT / HTTP API / ThingSpeak / LINE
   ↓                         ↓
 OLED / TFT LCD ← 狀態與控制 → E-Paper
```

> 實際腳位與接線請以各範例 `.ino` 內的定義為準。不同模組的電壓、SPI/I²C 腳位與驅動程式可能不同。

---

## 🚀 快速開始 (Quick Start)

### 1. 複製儲存庫

```bash
git clone https://github.com/fersz0614/esp32-sensors-mqtt-tft-epaper-Hsiang.git
cd esp32-sensors-mqtt-tft-epaper-Hsiang
```

### 2. 準備 Arduino IDE

1. 安裝 Arduino IDE 與 ESP32 board package。
2. 將需要的函式庫安裝到 Arduino `libraries` 目錄；外部函式庫來源記錄在 [.gitmodules](.gitmodules)。
3. 開啟 `ESP32/` 下的任一範例資料夾中的 `.ino` 檔案。
4. 選擇對應 ESP32 開發板、序列埠後編譯與上傳。

### 3. 設定網路與服務

公開版本已將 Wi‑Fi 密碼、API key、ThingSpeak key 與 LINE token 改成 placeholder。燒錄前請在本機程式中填入：

```cpp
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
```

請勿將真實密碼、token 或 API key 提交到 GitHub。

---

## 📚 範例程式索引

| 範例 | 功能說明 |
|---|---|
| [01_hello](ESP32/01_hello) | ESP32 基本啟動與序列埠輸出。 |
| [02_RGLED](ESP32/02_RGLED) | 紅綠 LED 輸出控制。 |
| [03_night_LED](ESP32/03_night_LED) | 夜間 LED 自動控制。 |
| [04_PIR](ESP32/04_PIR) | PIR 人體紅外線感測。 |
| [05_night_3_LED](ESP32/05_night_3_LED) | 夜間三顆 LED 情境控制。 |
| [06_dht](ESP32/06_dht) | 讀取 DHT11 溫度與濕度。 |
| [07_i2c](ESP32/07_i2c) | I²C 匯流排與裝置掃描。 |
| [08_OLED](ESP32/08_OLED) | OLED 基本文字與圖形顯示。 |
| [09_DHT_OLED](ESP32/09_DHT_OLED) | 將 DHT11 資料顯示在 OLED。 |
| [10_DHT_OLED_GYRBLED](ESP32/10_DHT_OLED_GYRBLED) | 溫濕度顯示與多色 LED 狀態提示。 |
| [11_DHT_OLED_GYRBLED_ALERT](ESP32/11_DHT_OLED_GYRBLED_ALERT) | 溫濕度警報與 LED 提示。 |
| [12_analogWrite_GLED](ESP32/12_analogWrite_GLED) | PWM 調整綠色 LED 亮度。 |
| [13_RGBLED](ESP32/13_RGBLED) | RGB LED 顏色混合與控制。 |
| [14_DHT_OLED_RGBLED](ESP32/14_DHT_OLED_RGBLED) | 依溫濕度狀態控制 RGB LED。 |
| [15_buzzer](ESP32/15_buzzer) | 蜂鳴器聲音與提示音。 |
| [16_DHT_OLED_RGBLED_Buzzer](ESP32/16_DHT_OLED_RGBLED_Buzzer) | DHT11、OLED、RGB LED 與蜂鳴器整合。 |
| [17_sonic](ESP32/17_sonic) | 超音波距離量測。 |
| [18_bt_command](ESP32/18_bt_command) | 以 Bluetooth 指令控制 ESP32。 |
| [19_PM2.5](ESP32/19_PM2.5) | 透過網路取得 PM2.5 資料。 |
| [20_PM25_OLED](ESP32/20_PM25_OLED) | 將 PM2.5 資料顯示在 OLED。 |
| [21_Weather_OLED](ESP32/21_Weather_OLED) | 取得氣象資料並顯示在 OLED。 |
| [22_dht_light_oled](ESP32/22_dht_light_oled) | DHT11 與光線感測器的 OLED 儀表。 |
| [23_dht_light_oled_thingspeak](ESP32/23_dht_light_oled_thingspeak) | 上傳溫濕度與光線資料到 ThingSpeak。 |
| [24_google_dht_light_oled](ESP32/24_google_dht_light_oled) | 將感測資料傳送到 Google 服務。 |
| [25_dht_line](ESP32/25_dht_line) | 透過 LINE 傳送感測器通知。 |
| [26_mqtt](ESP32/26_mqtt) | MQTT 感測資料發布與訂閱。 |
| [27_mqtt_ctrl](ESP32/27_mqtt_ctrl) | 以 MQTT 遠端控制輸出元件。 |
| [27_mqtt_retained](ESP32/27_mqtt_retained) | 示範 MQTT retained message。 |
| [28_ili9225](ESP32/28_ili9225) | ILI9225 TFT 基本顯示。 |
| [29_ilicolor](ESP32/29_ilicolor) | ILI9225 彩色圖形與文字。 |
| [30_dht_ili9225](ESP32/30_dht_ili9225) | 在 ILI9225 顯示 DHT11 資料。 |
| [31_ili9225_weather](ESP32/31_ili9225_weather) | 氣象資料與 TFT 顯示整合。 |
| [32_ili9225_gfx_dht](ESP32/32_ili9225_gfx_dht) | 使用 GFX 與 ILI9225 顯示 DHT11。 |
| [32_ili9225_weather_detailed](ESP32/32_ili9225_weather_detailed) | 詳細氣象資訊 TFT 儀表。 |
| [33_ili_mqtt](ESP32/33_ili_mqtt) | TFT 顯示 MQTT 感測資料。 |
| [34_ili_mqtt_ctrl](ESP32/34_ili_mqtt_ctrl) | TFT 介面搭配 MQTT 遠端控制。 |
| [35_ili_mqtt_ctl_page](ESP32/35_ili_mqtt_ctl_page) | 以頁面式 TFT 介面控制 MQTT 裝置。 |
| [36_epaper](ESP32/36_epaper) | 電子紙基本初始化與顯示。 |
| [37_epaper_dht](ESP32/37_epaper_dht) | 電子紙顯示 DHT11 溫濕度。 |
| [38_epaper_mqtt](ESP32/38_epaper_mqtt) | 電子紙顯示 MQTT 感測資料與控制狀態。 |

---

## 📷 更多成果照片

![MQTT](ESP32/成果照片/26_mqtt.png)
![MQTT 控制](ESP32/成果照片/27_mqtt_ctrl.png)
![DHT11 OLED ThingSpeak](ESP32/成果照片/23_dht_light_oled_thingspeak-1.png)

---

## ⚠️ 注意事項

- 請依照各範例的硬體模組調整腳位與電源，避免 3.3V/5V 不相容。
- 網路服務範例需要自行建立帳號、頻道與 API 憑證。
- `.arduino-build*`、`build_upload` 與暫存檔已排除，不應提交編譯輸出。
- 使用第三方函式庫時，請遵守各函式庫的授權條款。
