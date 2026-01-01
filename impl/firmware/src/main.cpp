#include <Arduino.h>
#include <NimBLEDevice.h>
#include <ESP32Time.h>

ESP32Time rtc;

// 送信するデータの構造体（C言語の構造体そのまま送ります）
// __attribute__((packed)) は「隙間なく詰める」という指示です
struct SensorData {
  uint16_t companyId;   // 企業ID (テスト用: 0xFFFF)
  uint32_t timestamp;   // 時刻 (起動からの秒数など)
  uint16_t voltage_mv;  // 電圧 (ミリボルト単位: 1250 = 12.50V)
  int16_t  current_ma;  // 電流 (ミリアンペア単位: -500 = -0.5A)
  uint8_t  battery_pct; // 内部電池残量 (%)
} __attribute__((packed));

void setup() {
  Serial.begin(115200);
  delay(2000); 
  Serial.println("Starting bamoni Parking Mode Beacon...");

  // 1. 初期化
  // 送信出力を最大に設定 (Long Range用にパワー最大)
  NimBLEDevice::init("bamoni-P"); // 名前は短く(パケット節約)
  NimBLEDevice::setPower(ESP_PWR_LVL_P21); 
  
  // Coded PHY設定
  NimBLEDevice::setDefaultPhy(BLE_GAP_LE_PHY_CODED_MASK, BLE_GAP_LE_PHY_CODED_MASK);

  // 2. アドバタイジング設定
  NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();

  // ★ここがポイント: データをパケットに埋め込む
  SensorData data;
  data.companyId = 0xFFFF; // テスト用ID
  data.timestamp = rtc.getEpoch(); // 現在時刻(UnixTime)
  
  // ダミーデータ (実際はセンサーから読む値をセット)
  data.voltage_mv = 1260; // 12.60V
  data.current_ma = -150; // -150mA (放電)
  data.battery_pct = 95;  // 95%

  // 構造体をバイト列に変換してセット
  std::string payload((char*)&data, sizeof(data));
  
  NimBLEAdvertisementData advData;
  advData.setName("bamoni-P");
  advData.setManufacturerData(payload); // これでデータが乗ります！
  
  // フラグ設定 (General Discovery)
  // NimBLEが自動設定するので削除でもOKですが、明示するならこう
  // advData.setFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE); 

  pAdvertising->setAdvertisementData(advData);
  
  // 3. スタート
  pAdvertising->start();
  Serial.println("Beacon started with Sensor Data!");
}

void loop() {
  // 実際の運用では Deep Sleep する場所ですが、テスト用にデータを更新し続ける

  NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->stop(); // データ更新のために一旦止める

  // 新しいデータをセット
  SensorData data;
  data.companyId = 0xFFFF;
  data.timestamp = rtc.getEpoch();
  
  // 変動させてみる(テスト用)
  data.voltage_mv = 1200 + (millis() / 1000 % 100); // 時間で変化
  data.current_ma = -100 - (millis() / 1000 % 50);
  data.battery_pct = 90;

  std::string payload((char*)&data, sizeof(data));
  
  NimBLEAdvertisementData advData;
  advData.setName("bamoni-P");
  advData.setManufacturerData(payload);
  
  pAdvertising->setAdvertisementData(advData);
  pAdvertising->start(); // 再開

  Serial.printf("Updated: %d mV\n", data.voltage_mv);

  delay(2000); // 2秒ごとに更新
}