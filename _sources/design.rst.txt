=============================================
設計ドキュメント
=============================================

:Author: bamoni Project Team
:Status: Draft

MCU選定
----------------------------------------------------------------

.. spec:: MCU選定
   :id: DES_MCU
   :status: open
   :tags: design, hardware, mcu
   :links: SEN_MEASURE; SEN_BLE_PARKING

   **採用:** Seeed Studio XIAO ESP32C3

   **選定理由:**

   * BLE 5.0対応（Coded PHY / Long Range が使用可能）
   * 小型・安価
   * Deep Sleep時の消費電流が低い

   **検討した代替案:**

   * Seeed XIAO nRF52840 — Coded PHY対応だが、価格面で不採用。

   **備考:**

   * センサーおよび中継器で同一MCUを採用し、ハードウェア・開発環境を共通化する。

電源回路
----------------------------------------------------------------

.. spec:: 電源回路設計
   :id: DES_POWER
   :status: open
   :tags: design, hardware, power
   :links: SEN_POWER; NFR_SAFETY; NFR_ENV

   **方式:** NiMH電池×4本（直列）による電池交換式。車両からの充電は行わない。

   **設計方針:**

   * **安全性:** 車内高温環境下でのリチウムイオン電池（Li-Po）の使用を回避し、NiMHを採用する。
   * **充電ICの制約:** XIAO ESP32C3搭載の充電IC（ETA4054）はリチウムイオン専用（4.2V充電停止）であり、NiMHの充電には不適合であるため使用しない。
   * **保護:** USB接続時の電源逆流防止および、NiMH 4本直列時の過電圧（約5.8V）をXIAOの許容範囲（5Vピン）へ適合させるため、整流用ダイオードを使用する。

   **見送り事項:**

   * 車両オルタネーターからのスマート充電（SEN_POWER）は、充電回路の複雑さから初版では見送り。電池交換で対応する。
   * 電池寿命の見積もりは初版実装後に実測ベースで評価する。

   **回路図:**

   .. code-block:: text

       [ Battery Box ]             [ Protection ]           [ MCU ]

       NiMH x 4 (Series)
       (approx. 4.8V - 5.8V)
             (+) --------------------|>|--------------------( 5V Pin )
                                   Diode (1N4007)
                                   Vf = ~0.7V drop

             (-) -------------------------------------------( GND Pin )

   **部品選定:**

   * **Diode:** 1N4007 (整流用ダイオード)
       * 目的1: 電圧降下（約0.7V）により、満充電時の5.8VをXIAOの安全圏（約5.1V）へ下げる。
       * 目的2: 開発時にUSBを接続した際、電池側への電流逆流（爆発・液漏れ）を防止する。

電圧測定回路
----------------------------------------------------------------

.. spec:: 電圧測定回路設計
   :id: DES_VOLT_SENSE
   :status: open
   :tags: design, hardware, measurement
   :links: SEN_MEASURE; NFR_SAFETY; NFR_ADC_ACCURACY

   車両の鉛バッテリー（12V系）の電圧を測定するため、分圧回路を用いてESP32C3のADC入力範囲（0〜3.0V程度）に変換する。

   **設計方針:**

   * **暗電流対策:** 常時接続となるため、高抵抗（合計122kΩ）を使用して消費電流を約0.1mAに抑える。
   * **サージ対策:** オルタネーターノイズやロードダンプからGPIOを保護するため、ツェナーダイオードとコンデンサを配置する。

   **回路図:**

   .. code-block:: text

       (Car Battery +)
              |
             [R1]  100kΩ
              |
              +-----> Measure Point (To XIAO A0/D0 Pin)
              |
              +-----> [D1] Zener Diode (3.6V) -> GND (Overvoltage Protection)
              |
             [R2]  22kΩ
              |
              +-----> [C1] Capacitor (0.1uF - 1uF) -> GND (Noise Filter)
              |
       (Car Body GND) --- (XIAO GND)

   **部品選定:**

   * R1: 100kΩ（確定）
   * R2: 22kΩ（確定）
   * D1: ツェナーダイオード 3.6V（型番未定）
   * C1: コンデンサ 0.1〜1μF（型番未定）

   **計算式:**

   * 分圧比:

     .. math::

        V_{out} = V_{in} \times \frac{R2}{R1 + R2} = V_{in} \times \frac{22}{122} \approx \frac{V_{in}}{5.545}

   * 最大測定可能電圧（ADC最大入力 3.3V 時）:

     .. math::

        V_{max} = 3.3V \times \frac{122}{22} \approx 18.3V

     通常の車両電圧（12.0V〜14.4V）に対し十分なマージンを持つ。

.. spec:: GND共通化制約
   :id: DES_GND
   :status: open
   :tags: design, hardware, constraint
   :links: DES_VOLT_SENSE

   電圧測定のため、電池駆動であってもセンサーのGNDを車両のボディアース（GND）に接続しなければならない。
   これは分圧回路の基準電位を車両側と共有するために必須の制約である。

   **備考:** 抵抗器の誤差（通常5%）を補正するため、実測値を用いたソフトウェア補正係数の調整を推奨する。

電流測定回路
----------------------------------------------------------------

.. spec:: 電流測定回路設計
   :id: DES_CURRENT_SENSE
   :status: open
   :tags: design, hardware, measurement
   :links: SEN_MEASURE; NFR_CURRENT_CALIB

   **方式:** ホール素子方式電流センサー（HSTS06L）を使用する。

   **ステータス:** 部品は購入済み。初版では電流測定は実装せず、電圧測定のみとする。回路設計・ファームウェア対応は次版以降で行う。

動作モード切替
----------------------------------------------------------------

.. spec:: 動作モード切替設計
   :id: DES_MODE_SWITCH
   :status: open
   :tags: design, firmware, mode
   :links: SEN_BLE_ACTIVE; SEN_BLE_PARKING

   **初版方針:** Parking Mode のみで動作する。Active Mode への切替および走行中／駐車中の判定方式は未検討。次版以降で対応する。

   **検討候補（次版以降）:**

   * 車両バッテリー電圧の閾値による判定（例：13.5V以上でオルタネーター稼働と判断）
   * ACC電源との接続による判定
   * 加速度センサーの追加

中継器設計
----------------------------------------------------------------

.. spec:: 中継器設計
   :id: DES_REPEATER
   :status: open
   :tags: design, hardware, firmware, repeater
   :links: RPT_CLOUD_SYNC; NFR_BLE_RANGE; DES_MCU

   **役割:** センサーのBLE Coded PHYパケットを受信し、自宅WiFi経由でGoogle Driveへデータを保存するゲートウェイ。

   **MCU:** Seeed Studio XIAO ESP32C3（センサーと共通）

   **設置場所:** 自宅の窓の外側。エアコンの壁穴を通してUSB給電する。

   **処理フロー:**

   1. BLE Coded PHYでセンサーのアドバタイズパケットを常時スキャン
   2. パケット受信時、受信時刻をタイムスタンプとして付与
   3. パケット内の過去ログを展開し、測定間隔から各ログのタイムスタンプを推定
   4. 自宅WiFiに接続し、Google Drive上のCSVファイルへ追記

   **未決定事項:**

   * Google Drive APIへのアクセス方法（サービスアカウント？OAuth？Google Apps Script Webhook？）
   * WiFi接続が切れた場合のローカルバッファリング
   * CSVフォーマットの仕様

BLEパケット仕様
----------------------------------------------------------------

.. spec:: BLEパケット設計 (Parking Mode)
   :id: DES_BLE_PACKET
   :status: open
   :tags: design, firmware, ble, parking-mode
   :links: SEN_BLE_PARKING; SEN_STORAGE

   Parking Mode時のBLEアドバタイズパケット仕様。Extended Advertising (BLE 5.x) を使用する。

   **容量制約:**

   * Extended Advertising の1パケット上限は **254バイト** 。これを超えるとChaining（分割送信）が発生し、電池消費が増えるため、254バイト以内に収める。
   * Manufacturer Data のオーバーヘッド（Length 1B + Type 1B + Company ID 2B = 4B）を除くと、自由データ領域は **250バイト** 。

   **パケットフォーマット（初版）:**

   .. code-block:: text

       Manufacturer Data (Type: 0xFF)
       +-------------------+---------------------+----------------+--------------------+
       | Company ID (2B)   | Current Voltage (2B) | History Count  | History[0..N] (2B  |
       | 0xFFFF            | mV単位               | (1B)           |  x N)              |
       +-------------------+---------------------+----------------+--------------------+

   * Company ID: 0xFFFF（テスト用）
   * Current Voltage: 現在の車両バッテリー電圧（mV, uint16_t）
   * History Count: 履歴データ件数（uint8_t）
   * History[]: 過去の電圧データ（mV, uint16_t配列、最新から過去へ順に並ぶ）

   **容量計算:**

   * CompanyID(2) + CurVolt(2) + Count(1) + History(2 x 48) = 101バイト（254バイト以内、分割なし）
   * 12分間隔 × 48件 = 9.6時間分の過去データを1パケットに格納可能

   **アドバタイズ送信時間:**

   * 起床ごとに **10秒間** アドバタイズを送出する。
   * 中継器が常時スキャンしているため、1回の送出で十分受信可能。

センサー設置設計
----------------------------------------------------------------

.. spec:: センサー設置方針
   :id: DES_SENSOR_PLACEMENT
   :status: open
   :tags: design, hardware, placement
   :links: DES_VOLT_SENSE; DES_GND

   センサー本体（XIAO ESP32C3）はアンテナ性能を最大化するため、車両のルーフ付近など電波が飛びやすい位置に設置する。
   バッテリー付近にセンサーを置いてアンテナケーブルを延長する方式は、高周波ケーブルのロスとコストの面から不採用とする。

   **方式:** 電源線およびセンサーケーブル（電圧測定線）を延長し、XIAO本体ごと設置場所に配置する。

   * 電池ボックスと分圧回路は安全な場所に設置
   * VCC、GND、アナログ入力の3本を電線で延長（2m程度）
   * XIAOはタッパー等の小型ケースに入れて設置

   **利点:**

   * 同軸ケーブル不要（コスト: 数百円）
   * アンテナが直結のまま → 感度最大
   * 特殊工具不要（ハンダ付けのみ）

データ保存方式
----------------------------------------------------------------

.. spec:: RTC メモリによるデータ保持
   :id: DES_RTC_MEMORY
   :status: open
   :tags: design, firmware, storage
   :links: SEN_STORAGE

   計測データの履歴はFlashではなく **RTCメモリ** に保持する。

   **理由:**

   * Deep Sleep中もデータが消えない
   * 読み書きが高速で消費電力がほぼゼロ
   * Flashは頻繁な書き込み（12分毎）で寿命・消費電力の面で不利

   **制約:**

   * 電池交換時にデータは消失する（許容範囲：電池交換前に中継器が最新データを受信済みであればよい）

通信方式の選定経緯
----------------------------------------------------------------

.. spec:: 通信方式選定: ESP-NOW vs BLE Coded PHY
   :id: DES_COMM_SELECTION
   :status: open
   :tags: design, communication, decision
   :links: SEN_BLE_PARKING; DES_MCU

   **採用:** BLE Coded PHY (Bluetooth Long Range)

   **経緯:**

   初期段階では、手持ちの受信機が ESP8266（Bluetooth非搭載）であったため、
   ESP32-C3（送信側）と ESP8266（受信側）の共通通信手段として **ESP-NOW（WiFi系プロトコル）** を使用していた。

   しかし、実地テスト（駐車場→自宅間）にて ESP-NOW では壁の貫通力が不足し、通信距離が足りないことが判明。

   **ESP-NOW と Coded PHY の比較:**

   * **ESP-NOW (WiFi):** 通信速度は速いが、壁を貫通する力が弱い。ESP8266でも使用可能。
   * **Coded PHY (BLE Long Range):** 通信速度は遅い（125kbps）が、受信感度が高く（-100dBm以下）、壁や障害物を越える能力が圧倒的に優れる。

   **結論:**

   受信機を ESP8266 から ESP32-C3 に変更し、BLE Coded PHY を採用することで通信距離の問題を解決した。
   ESP-NOW は不採用。test_esp-now/ 以下のコードは実験用として残置。