# LegCtrl Firmware

最小Zephyrアプリケーション（XIAO nRF52840向け）

## 概要
このアプリケーションは、XIAO nRF52840上で動作するZephyr RTOSベースのファームウェアです。
起動時にログを出力し、5秒ごとにハートビートメッセージを表示します。

## ビルド方法
詳細は `docs/build_flash.md` を参照してください。

## 動作確認
シリアルコンソール（115200 bps）に以下のようなログが表示されます：

```
===========================================
LegCtrl Firmware - XIAO nRF52840
===========================================
Zephyr version: x.x.x
Board: xiao_nrf52840
System started successfully
===========================================
Heartbeat: System running
```
