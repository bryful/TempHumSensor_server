# TempHumSensor_server

ESP32-S3 上で動作する温湿度センサー集約サーバーです。
ローカルの AHT20/BMP280 を読み取りつつ、登録した子機(最大6台)へ TCP で問い合わせを行い、ST7789 ディスプレイへ一覧表示します。

## 主な機能

- AHT20/BMP280 から自機の温度・湿度・気圧を取得
- 子機へ `TCP:12345` で `{"cmd":"get_data"}` を送信してデータ収集
- 240x320(ST7789) にヘッダ/子機6行/フッタを描画
- 一定時間操作なしでスクリーンセーバー(輝度0)
- Preferences に設定を保存
  - ボード名
  - 固定IP / ゲートウェイ
  - 子機IPリスト
  - スキャン周期
- シリアル(JSON)で設定の取得・更新

## 対応環境

- PlatformIO
- Framework: Arduino(ESP32)
- Board
  - `seeed_xiao_esp32s3`
  - `esp32-s3-dev`

## 依存ライブラリ

`platformio.ini` の `lib_deps`:

- `lovyan03/LovyanGFX@^1.2.7`
- `bblanchon/ArduinoJson@^7.4.1`
- `adafruit/Adafruit AHTX0@^2.0.5`
- `adafruit/Adafruit BMP280 Library@^2.6.8`

## ディレクトリ構成

- `src/main.cpp` : メイン処理(センサー取得、子機監視、画面表示、シリアルコマンド)
- `src/FsUtils.*` : Preferences / JSON / 補助ユーティリティ
- `src/Sensor.hpp` : 共通定数・`SensorData` 定義
- `src/LGFX_*.hpp` : ボード別 LovyanGFX 設定
- `src/lgfxJapanGothic_*` : 日本語フォントデータ

## ビルド・書き込み

プロジェクトルートで実行:

```powershell
# ビルド(デフォルト環境: esp32-s3-dev)
platformio run

# Seeed XIAO ESP32S3 へ書き込み
platformio run -e seeed_xiao_esp32s3 -t upload

# シリアルモニタ
platformio device monitor -b 115200
```

## 子機通信仕様

- ポート: `12345`
- 送信(JSON):

```json
{"cmd":"get_data"}
```

- 子機からの応答例(JSON):

```json
{
  "id": "sensor-01",
  "time": "2026/06/09 12:34:56",
  "temp": 24.8,
  "hum": 58.2,
  "pres": 1007.5
}
```

## シリアルコマンド

プレーンテキスト:

- `getid`
- `getstatus`

JSONコマンド例:

```json
{"getStatus":1}
{"getBoardName":1}
{"setBoardName":"server-01"}
{"getGateway":1}
{"setGateway":"192.168.1.1"}
{"getMyIP":1}
{"setMyIP":"192.168.1.50"}
{"getIplist":1}
{"setIplist":"192.168.1.101,192.168.1.102,192.168.1.103,192.168.1.104,192.168.1.105,192.168.1.106"}
{"getScanTime":1}
{"setScanTime":15000}
{"setBr":80}
{"getBr":1}
```

## 設定キー(Preferences)

- `boardname`
- `myip`
- `gatewayadr`
- `children`
- `scantiming`

## 備考

- `platformio.ini` の `default_envs` は `esp32-s3-dev` です。
- ボタン(`GPIO43`)でスクリーンセーバーのON/OFFを切り替えます。
- Wi-Fi未接続時は自動で再接続処理を行います。
