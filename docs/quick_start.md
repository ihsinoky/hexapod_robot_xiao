# Zephyr Firmware Quick Start

このドキュメントは、すでにZephyr環境が構築されている開発者向けのクイックスタートガイドです。

## 前提条件
- Zephyr SDK インストール済み
- west インストール済み
- Zephyrワークスペース (`~/zephyrproject/`) セットアップ済み

初回セットアップの場合は [`build_flash.md`](./build_flash.md) を参照してください。

## ビルド

```bash
# Zephyr環境を読み込む
cd ~/zephyrproject
source zephyr/zephyr-env.sh

# ファームウェアディレクトリへ移動
cd /path/to/hexapod_robot_xiao/firmware/zephyr/app/legctrl_fw

# ビルド
west build -b xiao_nrf52840 .

# クリーンビルド
west build -p auto -b xiao_nrf52840 .
```

## フラッシュ（UF2ブートローダー）

### 方法1: westコマンド
```bash
# 1. XIAOをブートローダーモードにする（RSTボタン2回ダブルクリック）
# 2. フラッシュ
west flash --runner uf2
```

### 方法2: 手動コピー
```bash
# 1. XIAOをブートローダーモードにする（RSTボタン2回ダブルクリック）
# 2. UF2ファイルをコピー
cp build/zephyr/zephyr.uf2 /Volumes/XIAO-SENSE/  # macOS
# または
cp build/zephyr/zephyr.uf2 /media/<user>/XIAO-SENSE/  # Linux
```

## 動作確認

```bash
# シリアルコンソールに接続（115200 bps）
screen /dev/ttyACM0 115200  # Linux
# または
screen /dev/tty.usbmodem* 115200  # macOS
```

期待される出力：
```
*** Booting Zephyr OS build v3.6.0 ***
===========================================
LegCtrl Firmware - XIAO nRF52840
===========================================
Zephyr version: 3.6.0
Board: xiao_nrf52840
System started successfully
===========================================
Heartbeat: System running
```

## トラブルシューティング

### ボード名エラー
```bash
# サポートされているボード名を確認
west boards | grep xiao

# 見つかったボード名を使用
west build -b seeed_xiao_nrf52840 .  # 古いバージョンの場合
```

### UF2デバイスが見つからない
1. RSTボタンを2回素早くダブルクリック
2. 緑色LEDが点灯することを確認
3. `XIAO-SENSE` ドライブがマウントされることを確認

### シリアル出力なし
- ボーレート: 115200 bps
- ポート確認: `ls /dev/ttyACM*` または `ls /dev/tty.usb*`

## 次のステップ
- BLE機能の追加（Bluetooth LE Peripheral）
- PCA9685 I2C通信の実装
- サーボ制御ロジックの追加

詳細は [`build_flash.md`](./build_flash.md) を参照してください。
