# 6脚ロボット（プロト）: iPad BLE 操作 + XIAO nRF52840 + PCA9685

このリポジトリは、以下を **同一リポジトリ（Monorepo）** で管理します。

- iPad（iOS）操作アプリ（自作, BLE Central）
- XIAO nRF52840（Zephyr）ファームウェア（BLE Peripheral）
- 共有プロトコル仕様（FW/iOSで共通化）
- 設計・配線・テストドキュメント

## 現行スコープ（カメラなし）
- 操作: iPad → BLE → XIAO nRF52840
- サーボ制御: XIAO → I2C → PCA9685 → サーボPWM出力
- 目的: “低遅延で確実に動く操縦系” と “安全停止（フェイルセーフ）” の確立

---

## システム概要

```
[iPad (iOS App)]
BLE Central
|
v
[XIAO nRF52840 (Zephyr FW)]
BLE受信/フェイルセーフ/制御ロジック
I2C
|
v
[PCA9685]
50Hz PWM
|
v
[Servos]
```

---

## リポジトリ構成

- `apps/ios/`  
  iPad向けアプリ（CoreBluetooth）。操縦UI、接続、周期送信、テレメトリ表示。

- `firmware/zephyr/`  
  XIAO nRF52840向けファーム（Zephyr）。BLE Peripheral、PCA9685制御、安全停止、設定管理。

- `shared/protocol/`  
  BLEプロトコルの “正本”。仕様（spec）と、必要なら生成コード（C/Swift）を配置。

- `docs/`  
  全体構成、BLE仕様、配線、テスト計画、トラブルシュート。

---

## まず最初にやること（MVPの定義）
MVP（カメラなし）は以下が成立することをもって達成とします。

1. iPadからBLE接続できる
2. ARM操作後に、操作コマンドを送るとサーボが動く（まずは1ch）
3. 通信が途切れたら一定時間内に安全停止する（デッドマン）
4. バッテリー電圧等の最小テレメトリがiPadへ通知される

詳細は `Milestone.md` を参照してください。

---

## 開発環境

### iOS（iPad）アプリ
- macOS + Xcode（最新安定版推奨）
- 言語: Swift
- BLE: CoreBluetooth
- 入口: `apps/ios/LegCtrlApp/`

### MCUファーム（Zephyr）
- Python 3.x
- Zephyr SDK（推奨）
- west（Zephyrのメタツール）

FWのソースは `firmware/zephyr/app/legctrl_fw/` にあります。

> 依存（Zephyr本体・モジュール）をリポジトリにコミットしない方針の場合、
> `firmware/zephyr/west.yml` を用いて取得します（westワークスペース上で展開）。
> 依存をどこに展開するかはチーム運用に合わせます。

---

## ビルド & フラッシュ（ファームウェア）
このREADMEでは “代表例” を示します。運用方針（in-repo / out-of-repo のworkspace）はチームで統一してください。

### 方式A（推奨）: Zephyr依存を別workspaceに展開（リポジトリは汚さない）
例:
1) 任意の作業用ディレクトリを作成して west workspace を初期化  
2) `firmware/zephyr/west.yml` を参照して依存を取得  
3) `firmware/zephyr/app/legctrl_fw` をビルド対象に指定

※ west運用を固定する場合は `firmware/zephyr/scripts/` に bootstrap スクリプトを追加します（BACKLOG参照）。

### 方式B: 依存をリポジトリ配下に展開（手軽だが .gitignore 必須）
- `firmware/zephyr/` 配下へ Zephyr本体等が展開されるため、必ず `.gitignore` で除外します。

---

## BLEプロトコル（入口）
- 人間が読む仕様: `shared/protocol/spec/legctrl_protocol.md`
- 詳細（GATT/Characteristic/パケット）: `docs/ble_protocol.md`

---

## テスト（初期）
- iOS専用Appが未完成でも、BLE疎通確認は汎用BLEテストアプリで代替可能。
- ただし操縦（周期送信・UI・安全操作）をMVPに含める場合は、最終的に専用Appが必要。

---

## 安全上の注意（重要）
- サーボ電源（V+）は別系統で供給し、XIAOから直接取らないこと
- ロジックGNDとサーボGNDは共通化するが、配線はノイズが戻らない形（スター配線気味）にする
- 必ずARM/DISARMとデッドマンを実装し、切断時に停止すること

**詳細は以下のドキュメントを参照**:
- `docs/wiring_pca9685.md` - PCA9685配線ガイド（電源分離・GND共通化・初回通電手順）
- `docs/safety.md` - 安全運用ガイドライン（ARM/DISARM・緊急停止・プリフライトチェック）

---

## 進め方
- 直近の作業: `BACKLOG.md` の P0 を上から潰す
- リリース計画: `Milestone.md` の順に達成する

