#include <Arduino.h>
#include <NimBLEDevice.h>
#include <ESP32Time.h>

// --- 設定値 ---
#define WAKE_INTERVAL_SEC  180  // 3分ごとに起きる
#define ADVERTISE_DUR_SEC  30   // 10秒間送信する
#define HISTORY_INTERVAL_MIN 12 // 履歴に残す間隔 (分)

// 夜間モード設定 (時)
#define NIGHT_START_HOUR 24
#define NIGHT_END_HOUR   6

// 電圧測定用
#define PIN_BAT_VOLT A0      
#define R1 100000.0
#define R2 22000.0
#define VOLTAGE_RATIO ((R1 + R2) / R2) 

// --- データ構造 ---

// 1件の履歴データ (2バイトで済ませる工夫: 10Vからの差分などではなく、単純にmV/10で保存)
// 例: 1250 => 12.50V (最大25.5Vまで表現可能)
struct HistoryPoint {
  uint8_t voltage_encoded; // 電圧(V) * 10 - 100 (オフセット) 等の工夫もできるが
                           // 今回はシンプルに uint8_t で 0.1V刻み (10.0V ~ 15.5Vを表現)
                           // 値 = (電圧mV - 10000) / 50 とすれば 10V~22Vを高精度に記録可能
                           // テスト用に単純な uint16_t にします（容量余裕あるので）
  uint16_t voltage_mv;     
} __attribute__((packed));

// 送信パケット構造
#define MAX_HISTORY_LEN 48 // 12分x48 = 9.6時間分 (パケットサイズ調整)

struct SensorData {
  uint16_t companyId;
  uint32_t timestamp;
  uint16_t current_voltage_mv;
  uint8_t  history_count;      // 履歴の個数
  uint16_t history[MAX_HISTORY_LEN]; // 過去の電圧データ配列
} __attribute__((packed));

// --- 変数 (Deep Sleepで消えない領域) ---
RTC_DATA_ATTR uint32_t bootCount = 0;
RTC_DATA_ATTR uint32_t lastHistoryTime = 0; // 最後に履歴保存した時刻
RTC_DATA_ATTR uint16_t historyBuffer[MAX_HISTORY_LEN]; // リングバッファ
RTC_DATA_ATTR int historyHead = 0; // 最新データの書き込み位置
RTC_DATA_ATTR bool isHistoryFilled = false; // バッファが一周したか

ESP32Time rtc;

void setup() {
  delay(3000); // 電源安定待ち
  Serial.begin(115200);
  
  // シリアル接続が確立するまで最大4秒待つ
  // これにより、起動直後のログがPCで見えるようになります。
  // (バッテリー駆動でPCがない時は、4秒経ったら諦めて先に進むので止まりません)
  unsigned long startWait = millis();
  while (!Serial && (millis() - startWait < 4000)) {
    delay(10);
  }
  delay(500); // 念のため接続後も一瞬待つ
  Serial.printf("Bamoni-P Wakeup #%d\n", bootCount + 1);

  // 1. 電圧計測
  analogSetAttenuation(ADC_11db);
  uint32_t adc_mv = analogReadMilliVolts(PIN_BAT_VOLT);
  float car_voltage_mv = adc_mv * VOLTAGE_RATIO;
  
  // 初回起動時のみRTC初期化 (実際はスマホから合わせる機能が必要ですが、今はコンパイル時刻で)
  if (bootCount == 0) {
    rtc.setTime(0, 0, 17, 1, 1, 2026); // 仮
    // 履歴バッファクリア
    memset(historyBuffer, 0, sizeof(historyBuffer));
  }
  bootCount++;

  uint32_t currentEpoch = rtc.getEpoch();
  int currentHour = rtc.getHour(true);

  Serial.printf("Time: %02d:%02d, Volt: %.0f mV\n", 
                currentHour, rtc.getMinute(), car_voltage_mv);

  // 2. 履歴の更新判定 (12分経過したか？)
  // 初回または前回保存から設定時間以上経過していたら保存
  if (bootCount == 1 || (currentEpoch - lastHistoryTime) >= (HISTORY_INTERVAL_MIN * 60)) {
    historyBuffer[historyHead] = (uint16_t)car_voltage_mv;
    historyHead++;
    if (historyHead >= MAX_HISTORY_LEN) {
      historyHead = 0;
      isHistoryFilled = true;
    }
    lastHistoryTime = currentEpoch;
    Serial.println("History Updated!");
  }

  // 3. 夜間モード判定
  bool isNight = false;
  if (NIGHT_START_HOUR > NIGHT_END_HOUR) {
    // 例: 22時〜6時 (日付またぎ)
    if (currentHour >= NIGHT_START_HOUR || currentHour < NIGHT_END_HOUR) isNight = true;
  } else {
    // 例: 0時〜5時
    if (currentHour >= NIGHT_START_HOUR && currentHour < NIGHT_END_HOUR) isNight = true;
  }

  // 4. 送信処理 (夜間じゃなければ送信)
  if (!isNight) {
    NimBLEDevice::init("bamoni-P");
    NimBLEDevice::setPower(ESP_PWR_LVL_P21); 
    NimBLEDevice::setDefaultPhy(BLE_GAP_LE_PHY_CODED_MASK, BLE_GAP_LE_PHY_CODED_MASK);
    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();

    // データ作成
    // パケットサイズ計算に注意。250バイトを超えないように。
    // Header(4) + Time(4) + CurVolt(2) + Count(1) + History(2*48 = 96) = 107バイト (余裕あり)
    
    // バイト列の準備
    std::string payload;
    payload.resize(sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint8_t) + (MAX_HISTORY_LEN * sizeof(uint16_t)));
    
    // ポインタ操作でデータを詰め込む
    uint8_t* pData = (uint8_t*)payload.data();
    int offset = 0;

    // Company ID
    *(uint16_t*)(pData + offset) = 0xFFFF; offset += 2;
    // Timestamp
    *(uint32_t*)(pData + offset) = currentEpoch; offset += 4;
    // Current Voltage
    *(uint16_t*)(pData + offset) = (uint16_t)car_voltage_mv; offset += 2;
    // History Count (常にMAX送るか、溜まっている分だけ送るか。今回は常にMAX枠確保で埋める)
    *(uint8_t*)(pData + offset) = MAX_HISTORY_LEN; offset += 1;

    // History Data (リングバッファを時系列順に並べ替えて詰めるのが親切だが、
    // ここでは単純にバッファの中身をそのままコピーし、アプリ側で「最新がどこか」判断させる、
    // あるいは毎回並べ替えて詰める。今回は「最新から過去へ」並べて詰めます)
    
    int idx = historyHead - 1; // 最新データのインデックス
    for (int i = 0; i < MAX_HISTORY_LEN; i++) {
        if (idx < 0) idx = MAX_HISTORY_LEN - 1;
        uint16_t val = historyBuffer[idx];
        // データがない(0)場合は0を入れる
        *(uint16_t*)(pData + offset) = val; 
        offset += 2;
        idx--;
    }
    // ペイロードサイズを実際に使用したサイズに調整
    payload.resize(offset);

    NimBLEAdvertisementData advData;
    advData.setName("bamoni-P");
    advData.setManufacturerData(payload);
    pAdvertising->setAdvertisementData(advData);
    
    // 10秒間送信
    pAdvertising->start();
    Serial.println("Advertising for 10s...");
    delay(ADVERTISE_DUR_SEC * 1000);
    pAdvertising->stop(); // 明示的に停止
    
  } else {
    Serial.println("Night Mode: Skipping TX");
  }

  // 5. Deep Sleep
  // 次回の起床時間を計算
  // 処理時間を考慮して厳密に計算もできるが、簡易的に一定時間スリープ
  esp_sleep_enable_timer_wakeup(WAKE_INTERVAL_SEC * 1000000ULL);
  Serial.println("Going to sleep...");
  Serial.flush();
  esp_deep_sleep_start();
}

void loop() {}