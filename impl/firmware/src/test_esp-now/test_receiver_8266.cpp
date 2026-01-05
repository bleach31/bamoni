#include <ESP8266WiFi.h>
#include <espnow.h>

// ESP8266の代表的な内蔵LEDは GPIO 2 (D4) です
// ボードによっては GPIO 16 (D0) の場合もあります
#define LED_PIN 2 

// データを受信したときのコールバック関数
// ※ESP32とは引数の型が少し違います
void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  // 受信したらLEDを点灯（LOWで光るボードが多い）
  digitalWrite(LED_PIN, HIGH);
  
  // 一瞬待つ（肉眼確認用）
  delay(100);
  
  // 消灯
  digitalWrite(LED_PIN, LOW);
}

void setup() {
  delay(1000);
  Serial.begin(115200);
  delay(5000);
  
  // LEDの設定
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW); // 最初は消しておく（HIGHでOFF）

  // 点灯テスト
  digitalWrite(LED_PIN, HIGH);
  delay(1000);
  digitalWrite(LED_PIN, LOW);
  delay(1000);
  digitalWrite(LED_PIN, HIGH);
  delay(1000);
  digitalWrite(LED_PIN, LOW);

  // Wi-FiをSTAモードに
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(); // 既存のWiFi接続を切る

  // ESP8266のみ、ESP-NOW初期化の書き方がシンプル
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // 役割設定（受信だけであってもCOMBOにしておくのが無難）
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);

  // コールバック登録
  esp_now_register_recv_cb(OnDataRecv);

  Serial.println("ESP8266 Receiver Ready!");
}

void loop() {
  // ループは何もしなくてOK
}