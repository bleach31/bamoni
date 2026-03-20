#include <Arduino.h>
#include <NimBLEDevice.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>
#include <bamoni_protocol.h>
#include "secrets.h"

// BLEスキャン設定
#define SCAN_DURATION_SEC  0  // 0 = 永続スキャン

// NTP設定
#define NTP_SERVER  "ntp.nict.jp"
#define GMT_OFFSET  (9 * 3600)   // JST = UTC+9
#define DST_OFFSET  0

static NimBLEScan* pScan = nullptr;
static bool wifiConnected = false;
static bool timeSync = false;
static time_t lastUploadedTimestamp = 0;  // 重複排除用

// WiFi接続
void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    return;
  }
  Serial.printf("[WiFi] Connecting to %s...\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(500);
  }
  wifiConnected = (WiFi.status() == WL_CONNECTED);
  if (wifiConnected) {
    Serial.printf("[WiFi] Connected. IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("[WiFi] Connection failed.");
  }
}

// NTP時刻同期
void syncTime() {
  if (timeSync) return;
  configTime(GMT_OFFSET, DST_OFFSET, NTP_SERVER);
  struct tm t;
  if (getLocalTime(&t, 10000)) {
    timeSync = true;
    Serial.printf("[NTP] Time synced: %04d-%02d-%02dT%02d:%02d:%02d\n",
      t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
  } else {
    Serial.println("[NTP] Sync failed.");
  }
}

// ISO8601形式の文字列生成
void formatISO8601(time_t t, char* buf, size_t bufLen) {
  struct tm* tm = localtime(&t);
  snprintf(buf, bufLen, "%04d-%02d-%02dT%02d:%02d:%02d",
    tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
    tm->tm_hour, tm->tm_min, tm->tm_sec);
}

// GASにデータ送信
bool postToGAS(const char* csvPayload) {
  if (!wifiConnected) return false;

  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.begin(GAS_WEBHOOK_URL);
  http.addHeader("Content-Type", "text/csv");
  int code = http.POST((uint8_t*)csvPayload, strlen(csvPayload));
  http.end();

  if (code == 200) {
    Serial.println("[GAS] Upload OK");
    return true;
  } else {
    Serial.printf("[GAS] Upload failed: %d\n", code);
    return false;
  }
}

// パケットをパースしてGASへ送信
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

  size_t expectedLen = BAMONI_PACKET_HEADER_SIZE + count * sizeof(uint16_t);
  if (len < expectedLen) {
    Serial.printf("[WARN] Packet truncated: %d < %d\n", len, expectedLen);
    return;
  }

  if (!timeSync) {
    Serial.println("[WARN] Time not synced, skipping upload.");
    return;
  }

  time_t now;
  time(&now);

  Serial.println("========================================");
  char tsBuf[32];
  formatISO8601(now, tsBuf, sizeof(tsBuf));
  Serial.printf("[RX] RSSI: %d dBm | Time: %s\n", rssi, tsBuf);
  Serial.printf("  Current Voltage: %u mV\n", pkt->currentVoltage);
  Serial.printf("  History Count:   %u\n", count);

  // CSVペイロード構築（新しいデータのみ）
  // 最大行数: 1(現在値) + MAX_HISTORY_LEN(過去ログ)
  // 各行: "2026-03-20T15:30:00,12680\n" = 最大32文字程度
  String csv = "";
  int newEntries = 0;

  // 現在値
  if (now > lastUploadedTimestamp) {
    formatISO8601(now, tsBuf, sizeof(tsBuf));
    csv += String(tsBuf) + "," + String(pkt->currentVoltage) + "\n";
    newEntries++;
  }

  // 履歴データ（最新→過去順）
  for (int i = 0; i < count; i++) {
    time_t histTime = now - (time_t)(i + 1) * BAMONI_MEASURE_INTERVAL_SEC;
    if (histTime > lastUploadedTimestamp) {
      formatISO8601(histTime, tsBuf, sizeof(tsBuf));
      csv += String(tsBuf) + "," + String(pkt->history[i]) + "\n";
      newEntries++;
    }
  }

  Serial.printf("  New entries: %d\n", newEntries);
  Serial.println("========================================");

  if (newEntries > 0 && postToGAS(csv.c_str())) {
    lastUploadedTimestamp = now;
  }
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

  // WiFi接続 → NTP同期
  connectWiFi();
  if (wifiConnected) {
    syncTime();
  }

  // BLE初期化（Coded PHY受信）
  NimBLEDevice::init("bamoni-R");
  NimBLEDevice::setDefaultPhy(BLE_GAP_LE_PHY_CODED_MASK, BLE_GAP_LE_PHY_CODED_MASK);

  pScan = NimBLEDevice::getScan();
  pScan->setScanCallbacks(new ScanCallbacks(), true);
  pScan->setActiveScan(false);

  Serial.printf("Scanning for '%s' on Coded PHY...\n", BAMONI_DEVICE_NAME);
  pScan->start(SCAN_DURATION_SEC);
}

void loop() {
  // WiFi再接続
  if (WiFi.status() != WL_CONNECTED) {
    wifiConnected = false;
    connectWiFi();
    if (wifiConnected && !timeSync) {
      syncTime();
    }
  }

  // スキャン再開
  if (!pScan->isScanning()) {
    Serial.println("[INFO] Scan stopped, restarting...");
    pScan->start(SCAN_DURATION_SEC);
  }
  delay(1000);
}