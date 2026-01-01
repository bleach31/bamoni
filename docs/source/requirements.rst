=============================================
要件定義
=============================================

.. spec:: 車両用バッテリー監視システム要件定義
   :id: SYS_ROOT
   :status: open
   :tags: system, overview

   車両バッテリーの電圧・電流を監視・ロギングするデバイスを作成し、スマートフォンアプリおよびGoogle Driveを経由して、複数ユーザーでデータを共有・管理するシステム。

----------------------------------------------------------------

.. req:: 計測パラメータ
   :id: DEV_MEASURE
   :status: open
   :tags: device, measurement

   デバイスは以下の項目を計測・保持しなければならない。
   
   * 車両バッテリー電圧
   * 車両バッテリー電流（充放電方向の識別必須）
   * デバイス内蔵バッテリー電圧
   * 現在時刻（RTC）

.. req:: 電流キャリブレーション
   :id: DEV_CALIB
   :status: open
   :tags: device, measurement, calibration

   アプリからの指示を受け、現在の電流計測値を「0A（ゼロ点）」として補正し、オフセット値を不揮発性メモリに保存・適用する機能を実装する。

.. req:: 電源管理とスマート充電
   :id: DEV_POWER
   :status: open
   :tags: device, power

   * 電源は乾電池または充電池（選択式）とする。
   * 充電池モード時は、車両が発電中（オルタネーター稼働中）の場合のみ充電を行う。
   * 車両停止時は暗電流を極小化する。

.. req:: BLE通信モード制御
   :id: DEV_BLE_MODE
   :status: open
   :tags: device, communication, ble

   車両の状態に応じて通信モードを切り替える。

   * **Active Mode (走行中):** BLE 1M/2M PHYを使用し、リアルタイム値を高頻度送信する。
   * **Parking Mode (駐車中):** BLE Coded PHY (Long Range)を使用し、低頻度（例：10分毎）で送信する。

.. req:: 冗長データ送信
   :id: DEV_BLE_REDUNDANCY
   :status: open
   :tags: device, communication

   Parking Mode時の送信パケットには、「現在値」に加え「過去N回分のログ」を含め、通信パケット欠落時のデータ補完を可能にする。

.. req:: セキュリティ（ペアリング）
   :id: DEV_SECURITY
   :status: open
   :tags: device, security

   BLE Bonding（ペアリング）機能を実装し、認証されたスマートフォン以外からの接続・操作を拒否する。

.. req:: 内部データ保持
   :id: DEV_STORAGE
   :status: open
   :tags: device, storage

   計測データは内部メモリのリングバッファに保持し、未接続時のデータを保全する。

----------------------------------------------------------------

.. req:: データ受信と結合処理
   :id: APP_DATA_SYNC
   :status: open
   :tags: app, logic

   * 受信パケットに含まれる過去ログ（冗長分）を活用し、欠落データを補完する。
   * 定期送信を受信できなかった場合はエラーとせず無視し、次回受信時の補完で対応する。

.. req:: 時刻同期
   :id: APP_TIME_SYNC
   :status: open
   :tags: app, logic

   BLE接続確立時にスマートフォンの時刻をデバイスへ送信し、RTCを補正する。

.. req:: Google Drive連携
   :id: APP_CLOUD_SYNC
   :status: open
   :tags: app, cloud

   * 共通のGoogleアカウントまたは共有フォルダを使用する。
   * アプリ内の未送信データをDrive上のCSVファイルへ追記（アップロード）する。
   * アプリ起動時にDrive上のCSVファイルを読み込み（ダウンロード）、グラフを表示する。
   * ログファイルは月次分割せず、単一またはシンプルな構成とする。

.. req:: ユーザーインターフェース
   :id: APP_UI
   :status: open
   :tags: app, ui

   以下の画面・機能を提供する。

   * ダッシュボード（リアルタイム表示）
   * 履歴グラフ表示
   * デバイス設定（ペアリング、電流0点補正）
   * 通知（異常値検知時のみ）

----------------------------------------------------------------

.. req:: 車両安全性
   :id: NFR_SAFETY
   :status: open
   :tags: non-functional, safety

   計測回路の高インピーダンス化およびヒューズ設置により、車両電気系統への悪影響（短絡、暗電流増大）を防止すること。

.. req:: 耐環境性とバッテリー選定
   :id: NFR_ENV
   :status: open
   :tags: non-functional, hardware

   車載環境（温度変化）に耐えうる部品を選定する。特に充電池使用時はNiMH等の安全性の高いものを推奨する。

.. req:: 保守・アップデート
   :id: NFR_MAINTENANCE
   :status: open
   :tags: non-functional, maintenance

   ファームウェア更新は物理接続（SWD/USB等）にて行い、OTA機能は実装しない。