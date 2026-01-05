#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

// ブロードキャスト用アドレス（全員宛て）
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

void setup() {
  Serial.begin(115200);
  
  // Wi-Fiを立ち上げる（重要：チャンネルを固定する）
  WiFi.mode(WIFI_STA);
  
  // 送信パワーを最大（19.5dBm）に設定
  WiFi.setTxPower(WIFI_POWER_19_5dBm);

  // ※チャンネルを1に固定しないと、受信側とすれ違うことがあります
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // 送信先（ブロードキャスト）を登録
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 1; // チャンネル1
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
}

void loop() {
  // 適当なデータを送る
  const char *message = "PING";
  esp_now_send(broadcastAddress, (uint8_t *)message, strlen(message));
  
  // 0.5秒間隔くらいで連呼
  delay(500);
}