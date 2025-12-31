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
現在準備中。アプリケーションコードは `firmware/zephyr/app/legctrl_fw/` に配置予定。

## ビルド方法
詳細はルートREADMEの「ビルド & フラッシュ（ファームウェア）」セクションを参照してください。
