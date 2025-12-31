# Zephyr Firmware

XIAO nRF52840 向けのファームウェア（Zephyr RTOS使用）

## 概要
- BLE Peripheral として iOS アプリと通信
- PCA9685 経由でサーボ制御
- 安全停止（ARM/DISARM、デッドマン）機能
- 設定管理とテレメトリ送信

## 開発環境
- Python 3.x
- Zephyr SDK（推奨）
- west（Zephyrのメタツール）

## 状態
**✅ 最小アプリケーション構築済み**  
アプリケーションコードは `firmware/zephyr/app/legctrl_fw/` に配置されています。

## ビルド方法
詳細な手順は [`docs/build_flash.md`](../../docs/build_flash.md) を参照してください。

**クイックスタート**（Zephyr環境セットアップ済みの場合）:
```bash
cd firmware/zephyr/app/legctrl_fw
west build -b xiao_nrf52840 .
west flash --runner uf2
```
