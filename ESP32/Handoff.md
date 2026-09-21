# ESP32 專案交接紀錄

更新日期：2026-08-31（Asia/Taipei）

## 工作區

- 根目錄：`D:\Hsiang\20260601 圖形介面程式設計 ESP32\ESP32`
- 主要硬體：ESP32 Dev Module、DHT11、光敏電阻、0.96 吋 SSD1306 OLED。
- 目前 USB 序列埠：COM6（USB 重新插拔後可能變更；先前曾使用 COM8、COM9）。

## 硬體接線

- DHT11：GPIO14
- 光敏電阻：GPIO33（ADC 0–4095 線性換算為 0–100%）
- OLED I²C：SDA GPIO21、SCL GPIO22
- OLED 控制器：目前程式使用 `U8G2_SSD1306_128X64_NONAME_F_HW_I2C`

## 已完成的主要專案

### `22_dht_light_oled`

- 每秒讀取溫濕度與亮度。
- OLED 以三欄式顯示溫度、濕度、亮度，含溫度計、水滴與太陽圖示。
- 已編譯並上傳。

### `23_dht_light_oled_thingspeak`

- Wi‑Fi：SSID `a`。
- 每 15 秒上傳 ThingSpeak。
- Field 1＝溫度、Field 2＝濕度、Field 3＝亮度。
- OLED 顯示 Wi‑Fi 與上傳狀態。
- 已成功編譯並上傳。

### `24_google_dht_light_oled`

- Wi‑Fi：SSID `a`。
- 每 10 秒將溫度、濕度、亮度寫入 Google Sheet 工作表 `data1`。
- 使用範例文件中的 Google Apps Script `/exec` 網址。
- Google Sheet ID：`1xPUW_jhTFVC8TJthiWnef2BqVDeWKtv-WPamYopdars`。
- 已成功編譯並上傳；當時使用 COM8。

### `25_dht_line`（目前最終告警版本）

- Wi‑Fi：SSID `a`、密碼已寫入程式。
- 異常條件：溫度 `> 28°C` 或濕度 `> 70%`。
- 異常時使用 LINE Messaging API Push 通知指定使用者。
- 異常持續期間每 30 秒最多通知一次。
- LINE 訊息已中文化：`警告！溫度：XX C，濕度：XX %`。
- OLED 保持顯示三欄感測值；傳送時顯示 `LINE sending`，完成後顯示 `LINE sent` 或 `LINE error`。
- 已成功編譯並燒錄至 ESP32（COM8）。
- 程式檔案：`25_dht_line\25_dht_line.ino`
- LINE User ID 與 Channel Access Token 已直接寫在程式中；交接文件不重複記錄敏感值。

### `27_mqtt_retained`（目前 MQTT 感測與 LED 控制版本）

- 程式檔案：`27_mqtt_retained\27_mqtt_retained.ino`。
- Wi‑Fi：SSID `H`；密碼已寫入程式。
- MQTT Broker：`mqttgo.io`，Port `1883`，不使用帳號密碼；Client ID 每次啟動產生亂數。
- 感測資料每 10 秒發布至 `Hsiang/class305/data`，格式為 `{"temp":25,"humi":65,"light":95}`。
- 三顆 LED 使用獨立控制與 retained Topic：
  - 綠燈 GPIO15：`Hsiang/class305/led/green`
  - 黃燈 GPIO2：`Hsiang/class305/led/yellow`
  - 紅燈 GPIO4：`Hsiang/class305/led/red`
- MQTT callback 依 Topic 控制對應 LED；控制 Payload 可使用 `{"gled":"on"}`、`{"yled":"off"}`、`{"rled":"on"}`。
- 控制後會將 `ON`／`OFF` 以 retained 方式發布回同一個 LED Topic；重新連線時可恢復各 LED 狀態，並已避免狀態回報造成無限循環。
- OLED 顯示 Wi‑Fi、溫度、濕度與亮度；三欄感測資料使用完整圓角外框，收到控制指令時顯示約 1 秒。
- 已成功編譯並上傳至 ESP32；最後確認使用 COM6。
- 測試確認三個獨立 LED Topic 均可正常控制。先前控制失敗原因是手機控制端仍使用錯誤／舊 Topic，修正為上述 Topic 後恢復正常。

## 報告成果

- Word 報告：`0810物聯網成果報告.docx`
- 報告包含：專案摘要、發展歷程、硬體配置、系統架構圖、程式流程圖、測試結果、改善方向與成果照片。
- 架構圖與流程圖後來改用影像生成工具製作，並已中文化為繁體中文版本。
- 成果照片位於：`成果照片\`
  - `25_dht_line.png`：LINE 中文警告通知畫面
  - `24_google_dht_light_oled.png`：Google Sheet 紀錄畫面
  - `23_dht_light_oled_thingspeak.png`：ThingSpeak 圖表畫面
- 生成圖素材位於：`報告素材\`
  - `架構圖_AI_中文.png`
  - `流程圖_AI_中文.png`

## 函式庫與工具

- 專案內建函式庫：`libraries\SimpleDHT`、`libraries\U8g2`、`libraries\ESP32Servo`、`libraries\PubSubClient`。
- Arduino CLI 位於 Arduino IDE 2 的資源目錄：
  `C:\Users\user\AppData\Local\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe`
- ESP32 核心版本：2.0.6。
- 編譯時若 Arduino 預設暫存目錄發生相依檔錯誤，可使用工作區專用建置目錄，例如：`.arduino-build-25b`，並以 `upload --input-dir` 上傳。

## 後續注意事項

1. LINE Channel Access Token 已放在韌體原始碼；若程式公開或 Token 外洩，應立即在 LINE Developers 重新簽發 Token。
2. 光敏電阻亮度百分比目前為線性換算；若實際電路方向相反，需將百分比反向或重新校正。
3. 若 OLED 完全無顯示，先確認模組是 SSD1306 128×64；若為 SH1106，需更換 U8g2 建構式。
4. USB 序列埠可能因重新插拔改變，燒錄前先執行 Arduino CLI 的 board list 確認目前 COM 埠。
5. Google Sheet、ThingSpeak 與 LINE 是前期／其他功能版本；MQTT 互動控制版本為 `27_mqtt_retained`。
6. 使用 `27_mqtt_retained` 時，手機控制端必須分別設定三個 LED Topic；不可只修改 ESP32 端而保留舊的控制 Topic。

---

# 2026-09-21 `38_epaper_mqtt` 電子紙 MQTT 更新

## 專案檔案

- 程式：`38_epaper_mqtt\38_epaper_mqtt.ino`
- 硬體：Waveshare 2.9 吋三色電子紙 V4，128×296。
- 電子紙資料層：黑色與紅色 bitmap 分開建立，白色像素會清除兩個資料層。
- 目前圖示改為自製點陣圖：溫度計、水滴、光照。

## 腳位

### 電子紙

- DIN / MOSI：GPIO23
- CLK / SCK：GPIO18
- CS：GPIO27
- DC：GPIO26
- RST：GPIO25
- BUSY：GPIO34

### 感測器與 LED

- DHT11：GPIO14
- 光敏電阻：GPIO33
- 綠色 LED：GPIO15
- 黃色 LED：GPIO2
- 紅色 LED：GPIO4

## 顯示與更新週期

- 電子紙畫面每 60 秒更新一次。
- 電子紙每次更新流程為：`Init()` → `Display()` → `Sleep()`。
- MQTT 感測資料每 10 秒發布一次。
- 無效的溫度／濕度會在電子紙顯示 `--`。
- 光敏電阻 ADC 讀值 0～4095 線性換算為 0～100%。
- 目前畫面標題為 `E-PAPER DATA`，包含 TEMP、HUMI、LIGHT 三張資訊卡。

## Wi‑Fi / MQTT 設定

- Wi‑Fi SSID：`H`
- Wi‑Fi 密碼：已寫入程式，交接文件不重複記錄。
- MQTT Broker：`mqttgo.io`
- MQTT Port：`1883`
- 感測資料 Topic：`Hsiang/class305/data`
- 感測資料格式：

```json
{"temp":25,"humi":60,"light":75}
```

### LED 控制 Topic

- 綠燈 GPIO15：`Hsiang/class305/led/green`
- 黃燈 GPIO2：`Hsiang/class305/led/yellow`
- 紅燈 GPIO4：`Hsiang/class305/led/red`

目前 LED callback 支援文字內容中包含 `ON`、`OFF`、`1`、`0` 的控制訊息，也可接受例如：

```json
{"gled":"ON"}
```

注意：目前 callback 使用字串包含判斷，控制訊息格式應保持單純，避免其他文字包含 `on` 或數字造成誤判。

## 目前程式與先前版本的差異

- 以 `char bufT / bufH / bufL` 組合顯示資料，降低畫面繪製時的暫時字串使用。
- 電子紙畫面與感測器讀取分離，MQTT 可每 10 秒傳送，電子紙不必每 10 秒刷新。
- 每次電子紙更新後呼叫 `epd.Sleep()`，降低耗電。
- 圖示由幾何圖形改為 bitmap，較適合固定解析度的三色電子紙。
- MQTT 連線失敗時會在主迴圈中定期重試 Wi‑Fi 與 MQTT。

## 最新確認狀態

- 使用者已自行更新 `38_epaper_mqtt.ino`。
- 最新程式碼已讀取檢查，設定與主要功能均已記錄於本節。
- 目前未因本次文件更新重新修改程式或重新上傳。


---

# 2026-09-14 ESP32 / ILI9225 專案交接更新

## 本次確認的開發環境

- Arduino IDE：**2.3.6**
- 開發板：**NMK99 ESP32 影像辨識組合包**
- Arduino Board 設定：本次決定改用 **ESP32 Wrover Module**
- ESP32 Arduino Core：**2.0.6**
- TFT：**ILI9225，3.3V**
- 5V：目前尚未接
- LED 供電：由 ESP32 提供
- 外接模組：後續若新增，由使用者另外告知；不要自行假設新增硬體。
- `33_ili_mqtt`、`34_ili_mqtt_ctrl`：使用者已確認腳位與主要功能正常。

## NMK99 腳位圖確認

使用者提供 NMK99 實際腳位圖，確認：

- GPIO4：一般 GPIO / ADC10 / TOUCH0
- GPIO0：**BOOT**
- GPIO2：一般 GPIO / ADC12 / TOUCH2
- GPIO15：HSPI SS / ADC13 / TOUCH3
- GPIO8：FLASH D1
- GPIO7：FLASH D0
- GPIO6：FLASH SCK

因此後續不可僅憑白燈外觀推論 GPIO4 就是板載 Flash LED。
使用者已明確表示：**不需要製作白燈確認測試程式**。

## ILI9225 目前固定腳位

依照已正常工作的 33 / 34 專案：

- TFT SCK → GPIO18
- TFT MOSI → GPIO23
- TFT RS → GPIO27
- TFT RST → GPIO26
- TFT CS → GPIO32
- TFT LED → GPIO25

三顆外接 LED：

- 綠燈 → GPIO15
- 黃燈 → GPIO2
- 紅燈 → GPIO4

換頁按鍵：

- IO0 / BOOT → GPIO0

## WiFi / MQTT 設定

目前使用：

- WiFi SSID：`H`
- WiFi 密碼：已寫入程式，交接紀錄不重複記錄敏感值
- MQTT Broker：`mqttgo.io`
- MQTT Port：1883

感測資料：

- Topic：`Hsiang/class305/data`
- JSON 格式：
  `{"temp":25,"humi":25,"light":95}`
- MQTT 更新間隔：10 秒

LED 控制 Topic：

- 綠燈：`Hsiang/class305/led/green`
- 黃燈：`Hsiang/class305/led/yellow`
- 紅燈：`Hsiang/class305/led/red`

MQTT callback 用於 LED 控制，JSON 使用 ArduinoJson。
既有控制功能已確認三個 LED Topic 可正常控制。

## `35_ili_mqtt_ctl_page` 需求與目前修改方向

目標檔名：

- `35_ili_mqtt_ctl_page`

功能需求：

### 第一頁

- 保留目前感測器畫面。
- 顯示：
  - TEMPERATURE
  - HUMIDITY
  - LIGHT
- 三個感測項目的色調希望與上方狀態列一致。
- WiFi / MQTT 狀態列保留。
- 畫面最下方增加台灣時間。
- 台灣時區：UTC+8。
- 時間格式：
  `09/14 Mon 13:20`
- 時間與畫面排版需要避免被 TFT 底部裁切。
- 感測 / MQTT 資料更新：10 秒。

### 第二頁

使用 GPIO0 / IO0 換頁。

- 第一頁：目前畫面。
- 第二頁分上下兩區。

上半部 Gauge：

- 範圍：10～40
- 10～20：綠色
- 20～30：黃色
- 30～40：紅色

下半部折線圖：

- Y 軸：10～40
- X 軸：最近 10 分鐘
- 每 10 秒更新一個資料點，因此約 61 個點可涵蓋最近 10 分鐘（包含目前點）。

### IO0 換頁

目前曾出現「按 IO0 沒反應」的問題。

後續版本應使用可靠的按鍵事件處理：

- GPIO0 設為 `INPUT_PULLUP`
- 按下偵測
- 防彈跳
- 一次按下只切換一次頁面
- 放開後才允許下一次切換
- Serial Monitor 可顯示：
  `IO0 BUTTON PRESSED -> PAGE 2`
  或
  `IO0 BUTTON PRESSED -> PAGE 1`

因為 GPIO0 是 BOOT，燒錄時不要按住 IO0。

## TFT 顯示問題

使用者實拍 TFT 後確認：

1. 顯示整體色調偏淡，需要提高對比。
2. 第一頁時間位置需要重新調整。
3. 時間不能貼近底部或被裁切。
4. TEMPERATURE / HUMIDITY / LIGHT 希望與狀態列採相同色調。
5. 不需要中文顯示，英文即可。

## 板載白燈

使用者曾指出 ESP32 板上有非常亮的白光。

目前決策：

- **不要控制 WS2812 RGB 可編程燈。**
- **不製作白燈確認測試程式。**
- 目前先不因白燈問題修改 GPIO4。
- 已取得 NMK99 腳位圖，確認 GPIO6/7/8 與 Flash 介面相關，因此不可直接推論 GPIO4 就是 Flash LED。

## 編譯錯誤修正

`35_ili_mqtt_ctl_page` 曾出現：

`'pageButtonISR' was not declared in this scope`

問題原因：

- `setup()` 中先使用 `pageButtonISR`
- 函式本體在後面
- 目前 ESP32 Arduino Core 2.0.6 環境下未自動產生可用宣告

修正方式：

在全域區加入明確函式宣告：

`void IRAM_ATTR pageButtonISR();`

保留原本函式本體，不做無關重構。

## 最新上傳狀態 / COM Port

最新一次編譯：

- 程式已**成功編譯**
- 編譯結果約：
  - Sketch 使用 787001 bytes（60%）
  - Global variables 使用 45052 bytes（13%）
- 失敗不是 Compiler Error，而是上傳階段：
  `Could not open COM4, the port doesn't exist`

因此下一步應先：

1. 拔除 ESP32 USB。
2. 再插回。
3. Arduino IDE → 工具 → 序列埠。
4. 選擇重新插入後實際存在的新 COM Port。
5. 不要繼續使用不存在的 COM4。
6. 若需要進入下載模式，使用 BOOT/IO0 搭配 RESET/EN，但燒錄時不要長按 IO0。

目前不要因 COM4 問題修改程式。

## 後續修改原則

使用者已明確要求：

1. 開始寫程式前先確認：
   - ESP32 型號
   - ESP-IDF / Arduino Core 版本
   - 開發板
   - GPIO 配置
   - 外接模組與感測器
   - 通訊協定
   - 電源電壓
   - 錯誤訊息
   - 預期功能
   - 已正常工作的部分
2. 資訊不足時，不自行假設關鍵硬體參數。
3. 貼程式碼時：
   - 先分析
   - 找 Bug
   - 說明原因
   - 給完整修改程式
   - 保留正常功能
   - 不任意重構無關部分
4. 貼 Compiler Error / Runtime Error / Serial Monitor 時：
   - 依錯誤訊息逐步定位
   - 不直接猜答案。

## 目前專案狀態摘要

- `33_ili_mqtt`：已確認正常。
- `34_ili_mqtt_ctrl`：已確認正常。
- `35_ili_mqtt_ctl_page`：正在調整中。
- Board：目前決定使用 **ESP32 Wrover Module**。
- TFT：ILI9225 / 3.3V。
- GPIO 配置以 33 / 34 已正常版本為基準。
- IO0 換頁仍需以實機確認新版程式是否正常。
- 最新一次程式已成功編譯，但因 COM4 不存在而未成功上傳。
