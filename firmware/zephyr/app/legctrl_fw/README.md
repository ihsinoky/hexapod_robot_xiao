# LegCtrl Firmware

Zephyrアプリケーション（XIAO nRF52840向け）- BLE + サーボ制御対応

## 概要
このアプリケーションは、XIAO nRF52840上で動作するZephyr RTOSベースのファームウェアです。
BLE（Bluetooth Low Energy）経由でコマンドを受信し、PCA9685 PWMコントローラを介してサーボモーターを制御します。

## 機能
- BLE Peripheral（デバイス名: "LegCtrl"）
- GATT Service + Command/Telemetry Characteristics
- I2C経由でPCA9685 PWMコントローラと通信
- 50Hz PWM信号でサーボモーター制御（チャンネル0）
- ARM/DISARM状態管理と安全停止
- デッドマンタイムアウト（200ms）
- 10Hz テレメトリ通知（状態/エラー/電圧）
- ワークキューによるコマンド処理（I2Cハング回避）
- シリアルログ出力とハートビート監視

## ハードウェア要件
- XIAO nRF52840ボード
- PCA9685 PWMコントローラ（I2Cアドレス: 0x40）
- I2C接続:
  - SDA: P0.04 (D4)
  - SCL: P0.05 (D5)
- サーボモーター（チャンネル0に接続）

## BLE仕様
- **Service UUID**: `12345678-1234-1234-1234-123456789abc`
- **Command Characteristic**: `12345678-1234-1234-1234-123456789abd` (Write/Write Without Response)
- **Telemetry Characteristic**: `12345678-1234-1234-1234-123456789abe` (Notify)

詳細は `docs/ble_protocol.md` および `shared/protocol/spec/legctrl_protocol.md` を参照してください。

## ビルド方法
詳細は `docs/build_flash.md` を参照してください。

基本コマンド:
```bash
cd firmware/zephyr/app/legctrl_fw
west build -b xiao_ble .
west flash --runner uf2
```

## 動作確認
シリアルコンソール（115200 bps）に以下のようなログが表示されます：

```
===========================================
LegCtrl Firmware - XIAO nRF52840
===========================================
Zephyr version: x.x.x
Board: xiao_ble
PCA9685 PWM device ready
Servo channel 0 initialized:
  - PWM frequency: 50Hz (period: 20ms)
  - Pulse width: 1500us (center position)
  - Safe range: 1000-2000us
PWM device ready
Bluetooth initialized
BLE advertising started
Device name: LegCtrl
BLE service initialized
Telemetry timer started (10 Hz)
System started successfully
===========================================
Heartbeat: State=0, Error=0, LastCmdAge=0 ms
```

## BLEテスト
nRF Connect、LightBlue等の汎用BLEアプリから接続できます。

基本操作:
1. スキャンして "LegCtrl" を探す
2. 接続後、Telemetry CharacteristicのNotifyを有効化
3. Command CharacteristicにARMコマンドを送信: `01 01 00 00`
4. サーボコマンドを送信（1500us）: `01 03 02 00 DC 05`
5. DISARMコマンドを送信: `01 02 00 00`

詳細な手順とコマンド一覧は `docs/ble_protocol.md` を参照してください。

## サーボ制御仕様
詳細は `docs/servo_control.md` を参照してください。

- PWM周波数: 50Hz (20ms周期)
- パルス幅範囲: 500-2500μs（自動クランプ）
- 安全な初期値: 1500μs (中心位置)
