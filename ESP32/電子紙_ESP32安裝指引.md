# 微雪 2.9 吋三色電子紙 ESP32 安裝指引

## 已確認的模組

- 品牌：Waveshare（微雪）
- 尺寸：2.9 吋
- 顯示：黑／白／紅三色
- 解析度：128 × 296（使用者描述為 296 × 128，程式座標通常以寬 128、高 296 表示）
- 版本：V4；使用者另提供 Rev2.1，兩者應以模組背面標示及 FPC 型號為最後判定依據。

## 已下載的程式庫與官方範例

- `libraries\GxEPD2`：ESP32 Arduino 電子紙函式庫。
- `Waveshare_e-Paper_official\Arduino\epd2in9b_V4`：微雪官方 2.9 吋三色 V4 範例與低階驅動原始碼。

GxEPD2 的初始型號候選為 `GxEPD2_290_Z13c`（GDEH029Z13、UC8151D、128 × 296）。第一次接線測試仍需依實際模組背面／FPC 標示確認；若畫面全白、顏色錯誤或完全不更新，不要反覆寫入，應先核對控制器型號。

## 建議接線

以目前專案既有 GPIO 配置為基準：

| 電子紙模組腳位 | ESP32 GPIO | 說明 |
|---|---:|---|
| VCC | 3V3 | 使用 3.3 V；不要接 5 V，除非該模組明確標示支援 |
| GND | GND | 共地 |
| DIN / MOSI | GPIO23 | SPI 資料輸入 |
| CLK / SCK | GPIO18 | SPI 時脈 |
| CS | GPIO27 | 片選 |
| DC | GPIO26 | 資料／命令選擇 |
| RST / RES | GPIO25 | 電子紙硬體重置 |
| BUSY | GPIO34 | 電子紙忙碌狀態輸入；GPIO34 為輸入專用腳位 |

電子紙沒有 TFT 背光腳位，因此不接 LED 背光。若手上的驅動板另外有 `PWR` 腳位，先接 3V3；不要直接套用官方 Arduino R4 範例中的 GPIO6，因為 ESP32-Wrover 的 GPIO6～8 與 Flash 介面相關。

## 重要衝突與注意事項

- GPIO23、18、27、26、25、34 由電子紙使用後，原 ILI9225 不應同時接在同一組 SPI 控制腳位。
- GPIO34 沒有內建上拉／下拉，BUSY 必須由電子紙模組輸出；不要在程式使用 `INPUT_PULLUP`。
- GPIO27、26、25 原本是 ILI9225 的控制腳位；改接電子紙後，原 ILI9225 應拔除或重新分配控制腳位。
- 原專案的 DHT11 GPIO14、光敏電阻 GPIO33、三顆 LED GPIO15／2／4、IO0 按鍵 GPIO0 可先保留。
- GPIO0 是 BOOT 腳位，燒錄時不要按住按鍵。
- 電子紙刷新需要等待 `BUSY`，不可像 OLED 一樣每秒頻繁更新；感測資料建議維持 10 秒以上更新一次。
- 三色電子紙主要採全畫面刷新；紅色畫面更新通常比黑白慢，且會有閃爍，這是正常現象。

## GxEPD2 初始宣告範例

```cpp
#include <GxEPD2_3C.h>
#include <GxEPD2_290_Z13c.h>

GxEPD2_3C<GxEPD2_290_Z13c, GxEPD2_290_Z13c::HEIGHT> display(
  /*CS=*/27, /*DC=*/26, /*RST=*/25, /*BUSY=*/34
);
```

SPI 資料腳位使用 ESP32 VSPI：SCK GPIO18、MOSI GPIO23；程式初始化可使用：

```cpp
SPI.begin(18, -1, 23, 27);
display.init(115200, true, 2, false);
```

第一次測試應先執行清屏，再顯示少量黑色與紅色文字，確認 BUSY、黑色層與紅色層都能正常更新後，才整合 MQTT 與感測器畫面。
