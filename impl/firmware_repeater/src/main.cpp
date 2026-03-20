#include <Arduino.h>
#include <NimBLEDevice.h>
#include <bamoni_protocol.h>

// BLEスキャン設定
#define SCAN_DURATION_SEC  0  // 0 = 永続スキャン

static NimBLEScan* pScan = nullptr;

// パケットをパースしてシリアル出力
void handlePacket(const uint8_t* data, size_t len, int rssi) {
  if (len < BAMONI_PACKET_HEADER_SIZE) {
    Serial.printf("[WARN] Packet too short: %d bytes\n", len);
    return;
  }

  const BamoniPacket* pkt = reinterpret_cast<const BamoniPacket*>(data);

  if (pkt->companyId != BAMONI_COMPANY_ID) {
    return;
  }

  uint8_t count = pkt->historyCount;
  if (count > BAMONI_MAX_HISTORY_LEN) {
    count = BAMONI_MAX_HISTORY_LEN;
  }

  // 受信データ内に履歴が収まっているか確認
  size_t expectedLen = BAMONI_PACKET_HEADER_SIZE + count * sizeof(uint16_t);
  if (len < expectedLen) {
    Serial.printf("[WARN] Packet truncated: %d < %d\n", len, expectedLen);
    return;
  }

  // 現在時刻を取得（タイムスタンプ用）
  unsigned long now = millis() / 1000;

  Serial.println("========================================");
  Serial.printf("[RX] RSSI: %d dBm | Time: %lus\n", rssi, now);
  Serial.printf("  Current Voltage: %u mV\n", pkt->currentVoltage);
  Serial.printf("  History Count:   %u\n", count);

  for (int i = 0; i < count; i++) {
    // 履歴は最新→過去順。各エントリは BAMONI_MEASURE_INTERVAL_SEC 間隔
    int agoSec = (i + 1) * BAMONI_MEASURE_INTERVAL_SEC;
    if (agoSec >= 120) {
      Serial.printf("  [-%3d min] %u mV\n", agoSec / 60, pkt->history[i]);
    } else {
      Serial.printf("  [-%3d sec] %u mV\n", agoSec, pkt->history[i]);
    }
  Serial.println("========================================");
}

// スキャンコールバック
class ScanCallbacks : public NimBLEScanCallbacks {
  void onResult(const NimBLEAdvertisedDevice* device) override {
    // デバイス名でフィルタ
    if (!device->haveName() || device->getName() != BAMONI_DEVICE_NAME) {
      return;
    }

    // Manufacturer Dataを取得
    if (!device->haveManufacturerData()) {
      return;
    }

    std::string mfgData = device->getManufacturerData();
    handlePacket(
      reinterpret_cast<const uint8_t*>(mfgData.data()),
      mfgData.size(),
      device->getRSSI()
    );
  }
};

void setup() {
  delay(2000);
  Serial.begin(115200);
  delay(1000);

  Serial.println("=== Bamoni Repeater Starting ===");

  // BLE初期化（Coded PHY受信）
  NimBLEDevice::init("bamoni-R");
  NimBLEDevice::setDefaultPhy(BLE_GAP_LE_PHY_CODED_MASK, BLE_GAP_LE_PHY_CODED_MASK);

  pScan = NimBLEDevice::getScan();
  pScan->setScanCallbacks(new ScanCallbacks(), true);  // true = 重複パケットも通知
  pScan->setActiveScan(false);  // パッシブスキャン（アドバタイズのみ受信）

  Serial.printf("Scanning for '%s' on Coded PHY...\n", BAMONI_DEVICE_NAME);
  pScan->start(SCAN_DURATION_SEC);
}

void loop() {
  // スキャンが停止した場合は再開
  if (!pScan->isScanning()) {
    Serial.println("[INFO] Scan stopped, restarting...");
    pScan->start(SCAN_DURATION_SEC);
  }
  delay(1000);
}