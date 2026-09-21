# 🚀 ESP32 Sensors & Display Hub (MQTT + TFT + E-Paper)

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![GitHub Issues](https://img.shields.io/github/issues/fersz0614/esp32-sensors-mqtt-tft-epaper-Hsiang)](https://github.com/fersz0614/esp32-sensors-mqtt-tft-epaper-Hsiang/issues)
[![GitHub Stars](https://img.shields.io/github/stars/fersz0614/esp32-sensors-mqtt-tft-epaper-Hsiang)](https://github.com/fersz0614/esp32-sensors-mqtt-tft-epaper-Hsiang/stargazers)

本專案整合了 **ESP32 微控制器**、多種感測器數據採集、**MQTT 通訊協定**傳輸，以及 **TFT 螢幕 / E-Paper（電子紙）** 的即時數據顯示。

---

## 📺 專案展示 (Live Demo)

> 💡 **成果展示與靜態網站展示**

- **展示網頁/儀表板**：[點此造訪 Live Demo (GitHub Pages)](https://fersz0614.github.io/esp32-sensors-mqtt-tft-epaper-Hsiang/)
- **硬體運作實拍**：
  *(在此處插入硬體運作照片或 GIF 動圖)*

---

## 🌟 專案亮點與特色

1. **程式與成果分享**
   - 開放式的架構設計，便於社群學習與模組化移植。
   - 提供完整的腳腳定義（Pinout）與電路接線圖。
2. ** MQTT 協定與雙顯示整合**
   - 低功耗 E-Paper 與高刷新率 TFT LCD 雙屏並行控制。
   - 支援遙測數據發布與遠端指令接收。

---

## 🛠️ 系統架構與硬體需求

### 硬體清單 (Hardware)
- ESP32 開發板
- TFT LCD 顯示器 (例如：ST7789 / ILI9341)
- E-Paper 電子紙模組 (例如：GxEPD2 相容)
- 感測器 (溫濕度 DHT22 / BME280 等)

---

## 🚀 快速開始 (Quick Start)

### 1. 複製儲存庫
```bash
git clone https://github.com/fersz0614/esp32-sensors-mqtt-tft-epaper-Hsiang.git
cd esp32-sensors-mqtt-tft-epaper-Hsiang
