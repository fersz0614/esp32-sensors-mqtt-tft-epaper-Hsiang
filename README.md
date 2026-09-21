# ⚡ ESP32 Lab: Sensors, MQTT, TFT & E-Paper Integration Practice

[![ESP32](https://img.shields.io/badge/Hardware-ESP32-blue.svg)](https://www.espressif.com/)
[![Language](https://img.shields.io/badge/Language-C%2FC%2B%2B-00599C.svg)](https://isocpp.org/)
[![Protocol](https://img.shields.io/badge/Protocol-MQTT%20%7C%20HTTP%20%7C%20SPI%20%7C%20I2C-orange.svg)]()
[![PlatformIO](https://img.shields.io/badge/IDE-PlatformIO%20%7C%20Arduino-brightgreen.svg)](https://platformio.org/)
[![Live Demo](https://img.shields.io/badge/Live_Demo-GitHub_Pages-222222.svg?logo=github)](https://fersz0614.github.io/esp32-sensors-mqtt-tft-epaper-Hsiang/)

> **從底層硬體訊號擷取、多協定無線傳輸到多端顯示（TFT / 電子紙 / 雲端）的模組化嵌入式韌體實作與工程展示。**

---

## 📌 專案簡介 (Project Overview)

本專案為 **ESP32 微控制器** 的嵌入式韌體與物聯網 (IoT) 軟硬體整合實戰展示庫。專案採用**模組化架構**設計，完整涵蓋從底層感測器訊號處理 (ADC / 時序讀取)、周邊通訊匯流排控制 (I2C / SPI)，到無線通訊協定 (Wi-Fi / MQTT / HTTP API / Bluetooth) 的跨層級串接。

此外，專案深化了多顯示終端（OLED、ILI9225 TFT、低功耗 E-Paper）與雲端服務（ThingSpeak、LINE Notify）的雙向資料互動，展示出具備產品化思維與邊緣端整合的即戰力。

👉 **[點此查看完整展示網頁 (Live Demo Website)](https://fersz0614.github.io/esp32-sensors-mqtt-tft-epaper-Hsiang/)**

---

## 🏗️ 系統架構與資料流向 (System Architecture & Flow)

系統架構採用標準四層式邊緣物聯網架構，確保各模組間的解耦與高擴充性：

```text
+---------------------------------------------------------------------------------------+
|  01 SENSOR (邊緣感測層)                                                               |
|  DHT11 (溫濕度) | PIR (紅外線人體) | LDR (光敏電阻) | 超音波 (測距) | RGB LED               |
+---------------------------------------------------------------------------------------+
                                           │ (GPIO / ADC / Pulse In)
                                           ▼
+---------------------------------------------------------------------------------------+
|  02 ESP32 CORE (邊緣控制與邏輯處理層)                                                    |
|  - 訊號解碼與濾波 processing                                                         |
|  - 匯流排控制器 (I2C / SPI)                                                           |
|  - 無線通訊 stack (Wi-Fi / MQTT / Bluetooth)                                          |
+---------------------------------------------------------------------------------------+
                     │                                           │
       (I2C / SPI)   │                                           │ (Wi-Fi / IP Stack)
                     ▼                                           ▼
+-------------------------------------------+ +-----------------------------------------+
|  04 DISPLAY (本地顯示終端)                 | |  03 CONNECT & CLOUD (網路傳輸與雲端服務)  |
|  - 0.96" OLED (I2C, 即時數據展示)         | |  - MQTT Broker (雙向 Remote Control)  |
|  - ILI9225 TFT (SPI, 彩色圖形 UI)          | |  - ThingSpeak API (數據分析與圖表)     |
|  - E-Paper (低功耗狀態儀表板)              | |  - LINE Notify (即時告警事件推播)      |
+-------------------------------------------+ +-----------------------------------------+
