=============================================
要件定義
=============================================

.. spec:: 車両用バッテリー監視システム要件定義
   :id: SYS_ROOT
   :status: open
   :tags: system, overview

   車両バッテリーの電圧・電流を監視・ロギングし、
   Google Driveを経由して複数ユーザーでデータを共有・管理するシステム。

   **データ収集の目的:**

   * **長期傾向データ:** バッテリー電圧・電流の長期的な変化を記録し、バッテリー劣化を検知する。
   * **短期高頻度データ:** 走行中の充放電パターンを高頻度で記録し、車両の充電制御戦略を分析する。

   **システム構成:**

   * **センサー:** 車両に設置し、バッテリーの電圧等を計測・BLE送信するデバイス
   * **中継器:** センサーのBLEパケットを受信し、自宅WiFi経由でGoogle Driveにデータを保存するゲートウェイデバイス
   * **アプリ:** スマートフォン上で動作し、BLEでセンサーのデータを受信・表示・クラウド連携するアプリケーション

   **動作モード:**

   * **Parking Mode (駐車中):** 車両が駐車中。センサーは低頻度・長距離通信（BLE Coded PHY）で送信し、中継器経由でGoogle Driveへ保存する。長期傾向データの収集に対応。
   * **Active Mode (走行中):** 車両が走行中。センサーは高頻度・短距離通信（BLE 1M/2M PHY）で送信し、アプリで受信・Google Driveへ保存する。短期高頻度データの収集に対応。

================================================================
センサー要件
================================================================

.. req:: 計測パラメータ
   :id: SEN_MEASURE
   :status: open
   :tags: sensor, measurement
   :priority: Must

   センサーは以下の項目を計測しなければならない。

   * 車両バッテリー電圧
   * 車両バッテリー電流（充放電方向の識別必須）
   * デバイス内蔵バッテリー電圧

   なお、センサーはRTCなどにより内部時刻を保持する必要はなく、受信側（中継器またはアプリ）で受信時刻をタイムスタンプとして使用する。

.. req:: 電源管理
   :id: SEN_POWER
   :status: open
   :tags: sensor, power
   :priority: Should

   * 電源は乾電池または充電池を設計に基づいて選択する。
   * 充電池を使用する場合は、車両が発電中（オルタネーター稼働中）の場合のみ充電を行う。
   * 車両停止時は暗電流を極小化する。

.. req:: BLE送信 (Active Mode)
   :id: SEN_BLE_ACTIVE
   :status: open
   :tags: sensor, communication, ble, active-mode
   :priority: Should

   Active Mode時、センサーはBLE 1M/2M PHYを使用し、リアルタイム計測値を高頻度で送信する。

.. req:: BLE送信 (Parking Mode)
   :id: SEN_BLE_PARKING
   :status: open
   :tags: sensor, communication, ble, parking-mode
   :priority: Must

   Parking Mode時、センサーはBLE Coded PHY (Long Range)を使用し、低頻度（例：12分毎）で送信する。
   送信パケットには「現在値」に加え「過去N回分のログ」を含め、
   通信パケット欠落時のデータ補完を可能にする。

.. req:: 内部データ保持
   :id: SEN_STORAGE
   :status: open
   :tags: sensor, storage
   :priority: Must

   計測データは内部メモリのリングバッファに保持し、未送信時のデータを保全する。

================================================================
中継器要件
================================================================

.. req:: BLE受信とクラウド保存 (Parking Mode)
   :id: RPT_CLOUD_SYNC
   :status: open
   :tags: repeater, cloud, parking-mode
   :priority: Must

   中継器はセンサーからのBLE Coded PHYパケットを常時受信し、自宅WiFi経由でGoogle Driveへデータを保存する。

   * 受信時刻をタイムスタンプとして付与する。過去ログのタイムスタンプは測定間隔から推定する。
   * 共通のGoogleアカウントまたは共有フォルダを使用する。
   * 受信した全データ（測定値）をDrive上のCSVファイルへ追記する。
   * 受信できなかったパケットはエラーとせず、次回受信時のパケット内過去ログで補完する。

================================================================
アプリ要件
================================================================

.. req:: データ受信とクラウド保存 (Active Mode)
   :id: APP_DATA_SYNC
   :status: open
   :tags: app, logic, active-mode
   :priority: Should

   Active Mode時、アプリはセンサーからのBLEパケットを受信し、Google Driveへデータを保存する。

   * 受信時刻をタイムスタンプとして付与する。
   * 中継器と同一のGoogle Drive・CSVフォーマットを使用し、データを統合可能にする。
   * 受信できなかったパケットはエラーとせず無視する。


.. req:: ユーザーインターフェース (Active Mode)
   :id: APP_UI
   :status: open
   :tags: app, ui
   :priority: Should

   Active Mode時には以下の画面・機能を提供する。

   * ダッシュボード（リアルタイム表示）
   * 履歴グラフ表示
   * デバイス設定（電流0点補正）
   * 通知（異常値検知時のみ）

================================================================
非機能要件
================================================================

.. req:: 車両安全性
   :id: NFR_SAFETY
   :status: open
   :tags: non-functional, safety
   :priority: Must

   計測回路の高インピーダンス化およびヒューズ設置により、車両電気系統への悪影響（短絡、暗電流増大）を防止すること。

.. req:: 耐環境性とバッテリー選定
   :id: NFR_ENV
   :status: open
   :tags: non-functional, hardware
   :priority: Should

   車載環境（温度変化）に耐えうる部品を選定する。特に充電池使用時はNiMH等の安全性の高いものを推奨する。

.. req:: 保守・アップデート
   :id: NFR_MAINTENANCE
   :status: open
   :tags: non-functional, maintenance
   :priority: Could

   ファームウェア更新は物理接続（SWD/USB等）にて行い、OTA機能は実装しない。

.. req:: ADC精度とデバイス電源電圧変動
   :id: NFR_ADC_ACCURACY
   :status: open
   :tags: non-functional, measurement, accuracy
   :priority: Could

   デバイスが電池駆動の場合、電池電圧の低下に伴いVDDが変動し、ADC計測値に影響を及ぼす可能性がある。必要に応じて以下の対策を検討すること。

   * デバイス内蔵バッテリー電圧を同時に計測し、送信データに含める（SEN_MEASUREの「デバイス内蔵バッテリー電圧」に該当）
   * 既知の電圧源を用いたキャリブレーション機能の実装

.. req:: Parking Mode到達距離
   :id: NFR_BLE_RANGE
   :status: open
   :tags: non-functional, communication
   :priority: Should

   Parking Mode時、駐車場から中継器設置場所（例：自宅窓の外）まで十分な通信距離を確保すること。

.. req:: 電流キャリブレーション
   :id: NFR_CURRENT_CALIB
   :status: open
   :tags: non-functional, measurement, calibration
   :priority: Could

   電流計測値の精度を確保するため、現在の電流値を「0A（ゼロ点）」として補正し、オフセット値を不揮発性メモリに保存・適用する機能を実装する。
   直接到達が困難な場合は、中継器等のアーキテクチャ上の対策を設計段階で検討する。