#include <Arduino.h>
#include <NimBLEDevice.h>
#include <bamoni_protocol.h>

// テスト用ダミー電圧 (mV)
#define TEST_VOLTAGE  12500

static uint32_t txCount = 0;

void setup() {
  delay(2000);
  Serial.begin(115200);
  delay(1000);

  Serial.println("=== Coded PHY Range Test Sender ===");

  NimBLEDevice::init(BAMONI_DEVICE_NAME);
  NimBLEDevice::setPower(ESP_PWR_LVL_P21);

  // パケット組み立て: ヘッダー + ダミー履歴1件
  std::string payload;
  payload.resize(BAMONI_PACKET_HEADER_SIZE + sizeof(uint16_t));
  uint8_t* p = (uint8_t*)payload.data();
  *(uint16_t*)(p + 0) = BAMONI_COMPANY_ID;
  *(uint16_t*)(p + 2) = TEST_VOLTAGE;
  *(uint8_t*)(p + 4)  = 1;
  *(uint16_t*)(p + 5) = TEST_VOLTAGE;

  NimBLEExtAdvertising* pAdv = NimBLEDevice::getAdvertising();
  NimBLEExtAdvertisement extAdv(BLE_HCI_LE_PHY_CODED, BLE_HCI_LE_PHY_CODED);
  extAdv.setName(BAMONI_DEVICE_NAME);
  extAdv.setManufacturerData(payload);
  extAdv.setConnectable(false);
  extAdv.setScannable(false);

  pAdv->setInstanceData(0, extAdv);
  pAdv->start(0);

  Serial.println("Transmitting continuously on Coded PHY...");
  Serial.println("Walk away with nRF Connect and check RSSI.");
}

void loop() {
  txCount++;
  Serial.printf("[TX] count=%u (still advertising)\n", txCount);
  delay(5000);
}
