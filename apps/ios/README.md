# iOS App - LegCtrlApp

iPad向けの操縦アプリ（CoreBluetooth使用）

## 概要
- BLE Central として XIAO nRF52840 に接続
- 操縦UI、接続管理、周期送信、テレメトリ表示を実装

## 開発環境
- macOS + Xcode 15.0 以上
- 言語: Swift 5.0
- BLE: CoreBluetooth
- UI: SwiftUI
- 対象デバイス: iPad (iOS 15.0+)

## ビルド方法

### 1. Xcodeでプロジェクトを開く
```bash
cd apps/ios/LegCtrlApp
open LegCtrlApp.xcodeproj
```

### 2. 開発チーム設定
- Xcodeで `LegCtrlApp` ターゲットを選択
- `Signing & Capabilities` タブを開く
- `Team` を選択（Apple Developer アカウントが必要）

### 3. 実機またはシミュレータで実行
- ターゲットデバイスを選択（iPad推奨）
- ⌘R でビルド＆実行

**注意**: BLE機能は実機でのみ動作します。シミュレータでは接続できません。

## 機能

### 1. BLE接続
- **Scan**: LegCtrlサービス（UUID: 12345678-1234-1234-1234-123456789abc）を持つデバイスをスキャン
- **Connect**: 発見したデバイスをタップして接続
- **Disconnect**: 接続を切断

### 2. ARM/DISARM制御
- **ARM**: システムを動作可能状態に遷移（サーボ駆動可能）
- **DISARM**: システムを安全停止状態に遷移（サーボ停止）

### 3. サーボ制御（CH0）
- **スライダー**: 500～2500 μsの範囲でパルス幅を設定
- **Periodic (50Hz)**: トグルONで50Hz周期での自動送信（デッドマン対策）
- **Send Once**: 手動で1回だけコマンドを送信

### 4. PING送信
- キープアライブコマンド送信（デッドマンタイムアウト回避）

### 5. テレメトリ表示
- **State**: システム状態（DISARMED/ARMED/FAULT）
- **Error**: エラーコード（None/Deadman Timeout/等）
- **Last Cmd**: 最終コマンドからの経過時間（ms）
- **Battery**: バッテリー電圧（V）

### 6. ログ表示
- BLE操作、送受信コマンドをリアルタイム表示
- 最大100件を保持

## 使用方法

### 基本操作フロー
1. **接続**
   - "Scan" ボタンを押す
   - デバイスリストから対象デバイスをタップ
   - "Ready" 状態になるまで待つ

2. **ARM**
   - "ARM" ボタンを押す
   - テレメトリで State が "ARMED" になることを確認

3. **サーボ制御**
   - スライダーでパルス幅を調整（1500 μs = 中立位置）
   - "Periodic (50Hz)" をONにして連続送信
   - または "Send Once" で単発送信

4. **停止**
   - "DISARM" ボタンを押す
   - サーボが停止し、State が "DISARMED" に戻る

### デッドマン対策
- ARMED状態では200ms以内にコマンドを送信し続ける必要があります
- "Periodic (50Hz)" モードを使用することで自動的に対策されます
- 200ms以上コマンドが途切れると FAULT 状態に遷移します

### 切断時の安全動作
- BLE切断時、ファームウェア側でデッドマンタイムアウトが発動し、自動的に安全停止します
- アプリ側でも周期送信を停止します

## プロトコル仕様

詳細は以下を参照:
- `docs/ble_protocol.md` - GATT/UUID/手動テスト手順
- `shared/protocol/spec/legctrl_protocol.md` - メッセージフォーマット詳細

### Service/Characteristic UUID
- **Service**: `12345678-1234-1234-1234-123456789abc`
- **Command**: `12345678-1234-1234-1234-123456789abd` (Write Without Response)
- **Telemetry**: `12345678-1234-1234-1234-123456789abe` (Notify)

## トラブルシューティング

### デバイスが見つからない
- ファームウェアが起動しているか確認
- Bluetoothが有効か確認
- サービスUUIDが一致しているか確認

### 接続できない
- 他のデバイスと接続していないか確認
- ファームウェアを再起動してみる

### サーボが動かない
- ARM状態になっているか確認
- テレメトリで State が "ARMED" であることを確認
- ファームウェアのログを確認

### デッドマンタイムアウトが頻発する
- "Periodic (50Hz)" モードを使用
- BLE接続が安定しているか確認（距離/障害物）

## 今後の拡張予定
- 複数チャンネル対応（>= 16ch）
- キャリブレーション機能
- ログのエクスポート（CSV/JSON）
- 設定画面（パラメータ調整）

## ライセンス
Apache-2.0 (プロジェクトルートのLICENSEを参照)
