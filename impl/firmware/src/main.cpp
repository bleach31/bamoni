#include <Arduino.h>
#include <NimBLEDevice.h>
#include <ESP32Time.h> 

ESP32Time rtc; // 時計オブジェクトの作成
NimBLECharacteristic *pTimeCharacteristic; // 時間を送るためのBLEキャラクタリスティック

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("Starting BLE work!");
  // 1. 時計の初期化 (例: 2025年12月30日 21時00分00秒 に設定)
  // setTime(秒, 分, 時, 日, 月, 年);
  rtc.setTime(0, 0, 21, 30, 12, 2025);

  // 2. Initialize BLE
  NimBLEDevice::init("ESP32C3-LongRange");
  NimBLEDevice::setDefaultPhy(BLE_GAP_LE_PHY_CODED_MASK, BLE_GAP_LE_PHY_CODED_MASK);

  // MACアドレスを表示
  String macAddr = NimBLEDevice::getAddress().toString().c_str();
  Serial.print("My MAC Address is: ");
  Serial.println(macAddr);

  // 3. サーバー作成
  NimBLEServer *pServer = NimBLEDevice::createServer();
  
  // 4. サービス作成
  NimBLEService *pService = pServer->createService("180D"); // 心拍数サービスのUUIDを借用

  // 5. キャラクタリスティック作成 (Notifyプロパティを追加)
  // Notify = 値が変わったらスマホに通知を送る機能
  pTimeCharacteristic = pService->createCharacteristic(
                                         "0617", 
                                         NIMBLE_PROPERTY::READ | 
                                         NIMBLE_PROPERTY::NOTIFY
                                       );

  pService->start();

  // 6. アドバタイジング開始
  NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->addServiceUUID("180D");
  
  // 名前を表示させる設定
  NimBLEAdvertisementData advData;
  advData.setName("ESP32C3-Bamoni");
  advData.setCompleteServices(NimBLEUUID("180D"));
  pAdvertising->setAdvertisementData(advData);
  
  pAdvertising->start();
  Serial.println("Waiting for connections...");
}

void loop() {
  // 接続されている場合のみ更新して送信
  if (NimBLEDevice::getServer()->getConnectedCount() > 0) {
    
    // 現在時刻を文字列で取得 (時:分:秒 フォーマット)
    String timeStr = rtc.getTime("%H:%M:%S");
    
    // 値をセット
    pTimeCharacteristic->setValue(timeStr);
    
    // スマホに通知(Notify)を送る
    pTimeCharacteristic->notify();
    
    Serial.print("Sent time: ");
    Serial.println(timeStr);
  }

  delay(1000); // 1秒待つ
}