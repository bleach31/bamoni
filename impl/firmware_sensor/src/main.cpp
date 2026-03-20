#include <Arduino.h>
#include <NimBLEDevice.h>

// --- 設定値 ---
#define WAKE_INTERVAL_SEC  720  // 12分ごとに起きる
#define ADVERTISE_DUR_SEC  10   // 10秒間送信する

// 電圧測定用
#define PIN_BAT_VOLT A0 // 電圧測定用のPIN A0(GPIO2)
#define R1 100000.0 // 電圧分割抵抗1 (100kΩ)
#define R2 22000.0  // 電圧分割抵抗2 (22kΩ)
#define VOLTAGE_RATIO ((R1 + R2) / R2) 

// --- データ構造 ---
#define MAX_HISTORY_LEN 48 // 12分x48 = 9.6時間分

// --- 変数 (Deep Sleepで消えない領域) ---
RTC_DATA_ATTR uint32_t bootCount = 0;
RTC_DATA_ATTR uint16_t historyBuffer[MAX_HISTORY_LEN]; // リングバッファ
RTC_DATA_ATTR int historyHead = 0; // 最新データの書き込み位置
RTC_DATA_ATTR bool isHistoryFilled = false; // バッファが一周したか

void setup() {
  // シリアル開始・電源安定待ち
  delay(3000); 
  Serial.begin(115200);
  unsigned long startWait = millis();
  while (!Serial && (millis() - startWait < 4000)) {
    delay(10);
  }
  delay(500);
  Serial.printf("Bamoni-P Wakeup #%d\n", bootCount + 1);

  // 1. 電圧計測
  pinMode(PIN_BAT_VOLT, ANALOG);
  uint32_t adc_mv = 0;
  for(int i = 0; i < 16; i++) {
    adc_mv += analogReadMilliVolts(PIN_BAT_VOLT);
  }
  float car_voltage_mv = VOLTAGE_RATIO * adc_mv / 16;
  
  if (bootCount == 0) {
    memset(historyBuffer, 0, sizeof(historyBuffer));
  }
  bootCount++;

  Serial.printf("Volt: %.0f mV\n", car_voltage_mv);

  // 2. 履歴保存 
  historyBuffer[historyHead] = (uint16_t)car_voltage_mv;
  historyHead++;
  if (historyHead >= MAX_HISTORY_LEN) {
    historyHead = 0;
    isHistoryFilled = true;
  }
  Serial.println("History Updated!");

  // 3. BLE送信
  NimBLEDevice::init("bamoni-P");
  NimBLEDevice::setPower(ESP_PWR_LVL_P21); 
  NimBLEDevice::setDefaultPhy(BLE_GAP_LE_PHY_CODED_MASK, BLE_GAP_LE_PHY_CODED_MASK);
  NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();

  // パケット: CompanyID(2) + CurVolt(2) + Count(1) + History(2*48=96) = 101バイト
  std::string payload;
  payload.resize(sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint8_t) + (MAX_HISTORY_LEN * sizeof(uint16_t)));
  
  uint8_t* pData = (uint8_t*)payload.data();
  int offset = 0;

  // Company ID
  *(uint16_t*)(pData + offset) = 0xFFFF; offset += 2;
  // Current Voltage
  *(uint16_t*)(pData + offset) = (uint16_t)car_voltage_mv; offset += 2;
  // History Count
  uint8_t count = isHistoryFilled ? MAX_HISTORY_LEN : historyHead;
  *(uint8_t*)(pData + offset) = count; offset += 1;

  // History Data (最新から過去へ)
  int idx = historyHead - 1;
  for (int i = 0; i < count; i++) {
      if (idx < 0) idx = MAX_HISTORY_LEN - 1;
      *(uint16_t*)(pData + offset) = historyBuffer[idx]; 
      offset += 2;
      idx--;
  }
  payload.resize(offset);

  NimBLEAdvertisementData advData;
  advData.setName("bamoni-P");
  advData.setManufacturerData(payload);
  
  pAdvertising->setAdvertisementData(advData);
  pAdvertising->start();
  Serial.printf("Advertising for %d sec...\n", ADVERTISE_DUR_SEC);
  delay(ADVERTISE_DUR_SEC * 1000);
  pAdvertising->stop();
  Serial.println("Stop Advertising");

  // 4. Deep Sleep
  esp_sleep_enable_timer_wakeup(WAKE_INTERVAL_SEC * 1000000ULL);
  Serial.println("Going to sleep...");
  Serial.flush();
  esp_deep_sleep_start();
}

void loop() {}