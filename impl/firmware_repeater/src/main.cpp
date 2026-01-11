#include <Arduino.h>
#include <WiFi.h>

// WiFiの設定
const char* ssid = "XIAO-MaxPower-Test";
const char* password = "password123";

WiFiServer server(80);

void setup() {
  delay(2000);
  Serial.begin(115200);
  delay(2000);

  Serial.println("WiFi Max Power Test Start...");

  // WiFiモードの設定
  WiFi.mode(WIFI_AP);

  // アクセスポイントの開始
  WiFi.softAP(ssid, password);

  // ★WiFiの送信出力を最大(19.5dBm)に設定
  // これにより、電波をできるだけ遠くまで飛ばす設定になります
  WiFi.setTxPower(WIFI_POWER_19_5dBm);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);

  server.begin();
  Serial.println("Server is ready!");
}

void loop() {
  WiFiClient client = server.available();

  if (client) {
    String currentLine = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (c == '\n') {
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            // シンプルな英語メッセージ
            client.print("<h1>WiFi Max Power Test</h1>");
            client.print("<p>Status: OK!</p>");
            client.print("<p>The power is now MAX (19.5dBm).</p>");
            client.print("<p>Please check the distance.</p>");
            client.println();
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }
    client.stop();
  }
}