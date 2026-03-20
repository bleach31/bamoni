=============================================
セットアップガイド
=============================================

開発環境セットアップ
----------------------------------------------------------------

ファームウェアビルド環境
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

1. `PlatformIO <https://platformio.org/>`_ をインストールする（VS Code拡張推奨）。
2. VS Codeでプロジェクトフォルダ（ ``impl/firmware_sensor`` または ``impl/firmware_repeater`` ）を開く。
3. PlatformIOツールバーから対象の環境を選択しビルドする。

ドキュメントビルド環境
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

1. Python 3.10以上をインストールする。
2. 依存パッケージをインストールする:

   .. code-block:: bash

       pip install -r docs/requirements.txt

3. ドキュメントをビルドする:

   .. code-block:: bash

       sphinx-build -b html docs/source docs/build

センサーセットアップ
----------------------------------------------------------------

ハードウェア準備
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

1. Seeed Studio XIAO ESP32C3 を用意する。
2. NiMH電池×4本（直列）を電池ボックスに入れ、1N4007ダイオード経由で5Vピンに接続する（回路は :need:`DES_POWER` を参照）。
3. 分圧回路（R1=100kΩ, R2=22kΩ, ツェナー3.6V, コンデンサ）を組み、A0ピンに接続する（回路は :need:`DES_VOLT_SENSE` を参照）。
4. GNDを車両ボディアースに接続する（ :need:`DES_GND` ）。

ファームウェア書き込み
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

1. ``impl/firmware_sensor`` フォルダをPlatformIOで開く。
2. 環境 ``seeed_xiao_esp32c3_main`` を選択してビルド・書き込みする。

中継器セットアップ
----------------------------------------------------------------

ハードウェア準備
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

1. Seeed Studio XIAO ESP32C3 を用意する（センサーと同一MCU）。
2. エアコンの壁穴を通したUSBケーブルで給電し、窓の外側に設置する。

WiFi・GAS設定
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

1. ``impl/firmware_repeater/src/secrets.h.example`` をコピーして ``secrets.h`` を作成する:

   .. code-block:: bash

       cp impl/firmware_repeater/src/secrets.h.example impl/firmware_repeater/src/secrets.h

2. ``secrets.h`` にWiFi SSID・パスワードとGAS Webhook URLを記入する。

Google Apps Script (GAS) Webhook構築
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

1. `Google Spreadsheet <https://sheets.google.com/>`_ を新規作成し、A1に ``timestamp`` 、B1に ``voltage_mv`` と入力する。
2. メニュー「拡張機能」→「Apps Script」を開き、以下のスクリプトを貼り付ける:

   .. code-block:: javascript

       function doPost(e) {
         var sheet = SpreadsheetApp.getActiveSpreadsheet().getActiveSheet();
         var csv = e.postData.contents;
         var rows = csv.trim().split("\n");
         for (var i = 0; i < rows.length; i++) {
           var cols = rows[i].split(",");
           sheet.appendRow(cols);
         }
         return ContentService.createTextOutput("OK");
       }

3. 「デプロイ」→「新しいデプロイ」→ 種類「ウェブアプリ」を選択する。
   実行するユーザーは「自分」、アクセス権は「全員」に設定しデプロイする。
4. 表示されるURLを ``secrets.h`` の ``GAS_WEBHOOK_URL`` に設定する。

ファームウェア書き込み
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

1. ``impl/firmware_repeater`` フォルダをPlatformIOで開く。
2. 環境 ``seeed_xiao_esp32c3_main`` を選択してビルド・書き込みする。
