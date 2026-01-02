# LegCtrl Firmware

Zephyrアプリケーション（XIAO nRF52840向け）- サーボ制御対応

## 概要
このアプリケーションは、XIAO nRF52840上で動作するZephyr RTOSベースのファームウェアです。
PCA9685 PWMコントローラを介してサーボモーターを制御します。

## 機能
- I2C経由でPCA9685 PWMコントローラと通信
- 50Hz PWM信号でサーボモーター制御（チャンネル0）
- 安全な初期位置（1500μs中心位置）での起動
- シリアルログ出力とハートビート監視

## ハードウェア要件
- XIAO nRF52840ボード
- PCA9685 PWMコントローラ（I2Cアドレス: 0x40）
- I2C接続:
  - SDA: P0.04 (D4)
  - SCL: P0.05 (D5)
- サーボモーター（チャンネル0に接続）

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
System started successfully
===========================================
Heartbeat: Servo active at center position
```

## サーボ制御仕様
詳細は `docs/servo_control.md` を参照してください。

- PWM周波数: 50Hz (20ms周期)
- パルス幅範囲: 1000-2000μs
- 安全な初期値: 1500μs (中心位置)
