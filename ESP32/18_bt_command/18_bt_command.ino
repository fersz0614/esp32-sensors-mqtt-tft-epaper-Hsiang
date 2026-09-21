#include <BluetoothSerial.h>
BluetoothSerial BT;

int R = 4;   // 紅燈 (GPIO4)
int Y = 2;   // 黃燈 (GPIO2)
int G = 15;  // 綠燈 (GPIO15)

void setup() {
  Serial.begin(115200);
  BT.begin("被12號訓練中的日誌"); // 藍牙名稱
  pinMode(R, OUTPUT);
  pinMode(Y, OUTPUT);
  pinMode(G, OUTPUT);
}

void loop() {
  // 檢查序列監控視窗是否有輸入資料
  String Sdata = "";
  String BTdata = "";
  while (Serial.available()) {
    char Schar = Serial.read();   // 一次讀一個字元
    Sdata = Sdata + Schar;        // 累加字串
  }

  if (Sdata != "") BT.println(Sdata);

  // 檢查藍牙內是否有資料
  while (BT.available()) {
    char BTchar = BT.read();
    BTdata = BTdata + BTchar;     // 累加字串
  }

  if (BTdata != "") {
    Serial.println(BTdata);

    // 藍牙控制紅燈
    if (BTdata == "0") digitalWrite(R, LOW);
    if (BTdata == "1") digitalWrite(R, HIGH);

    // 藍牙控制黃燈
    if (BTdata == "2") digitalWrite(Y, LOW);
    if (BTdata == "3") digitalWrite(Y, HIGH);

    // 藍牙控制綠燈
    if (BTdata == "4") digitalWrite(G, LOW);
    if (BTdata == "5") digitalWrite(G, HIGH);
  }

  delay(10);
}
