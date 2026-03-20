#include <Arduino.h>
#include <NimBLEDevice.h>
#include <bamoni_protocol.h>

// 電圧測定用
#define PIN_BAT_VOLT A0 // 電圧測定用のPIN A0(GPIO2)
#define R1 100000.0 // 電圧分割抵抗1 (100kΩ)
#define R2 22000.0  // 電圧分割抵抗2 (22kΩ)
#define VOLTAGE_RATIO ((R1 + R2) / R2)

// --- 変数 (Deep Sleepで消えない領域) ---
RTC_DATA_ATTR uint32_t bootCount = 0;
RTC_DATA_ATTR uint16_t historyBuffer[BAMONI_MAX_HISTORY_LEN]; // リングバッファ
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
  Serial.printf("%s Wakeup #%d\n", BAMONI_DEVICE_NAME, bootCount + 1);

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
  if (historyHead >= BAMONI_MAX_HISTORY_LEN) {
    historyHead = 0;
    isHistoryFilled = true;
  }
  Serial.println("History Updated!");

  // 3. BLE送信 (Extended Advertising / Coded PHY)
  NimBLEDevice::init(BAMONI_DEVICE_NAME);
  NimBLEDevice::setPower(ESP_PWR_LVL_P21);

  // パケット組み立て: CompanyID(2) + CurVolt(2) + Count(1) + History(2*N)
  std::string payload;
  payload.resize(BAMONI_PACKET_HEADER_SIZE + (BAMONI_MAX_HISTORY_LEN * sizeof(uint16_t)));
  
  uint8_t* pData = (uint8_t*)payload.data();
  int offset = 0;

  // Company ID
  *(uint16_t*)(pData + offset) = BAMONI_COMPANY_ID; offset += 2;
  // Current Voltage
  *(uint16_t*)(pData + offset) = (uint16_t)car_voltage_mv; offset += 2;
  // History Count
  uint8_t count = isHistoryFilled ? BAMONI_MAX_HISTORY_LEN : historyHead;
  *(uint8_t*)(pData + offset) = count; offset += 1;

  // History Data (最新から過去へ)
  int idx = historyHead - 1;
  for (int i = 0; i < count; i++) {
      if (idx < 0) idx = BAMONI_MAX_HISTORY_LEN - 1;
      *(uint16_t*)(pData + offset) = historyBuffer[idx]; 
      offset += 2;
      idx--;
  }
  payload.resize(offset);

  NimBLEExtAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
  NimBLEExtAdvertisement extAdv(BLE_HCI_LE_PHY_CODED, BLE_HCI_LE_PHY_CODED);
  extAdv.setName(BAMONI_DEVICE_NAME);
  extAdv.setManufacturerData(payload);
  extAdv.setConnectable(false);
  extAdv.setScannable(false);

  pAdvertising->setInstanceData(0, extAdv);
  pAdvertising->start(0);  // instance 0
  Serial.printf("Advertising for %d sec...\n", BAMONI_ADVERTISE_DUR_SEC);
  delay(BAMONI_ADVERTISE_DUR_SEC * 1000);
  pAdvertising->stop(0);
  Serial.println("Stop Advertising");

  // 4. Deep Sleep
  esp_sleep_enable_timer_wakeup(BAMONI_MEASURE_INTERVAL_SEC * 1000000ULL);
  Serial.println("Going to sleep...");
  Serial.flush();
  esp_deep_sleep_start();
}

void loop() {}