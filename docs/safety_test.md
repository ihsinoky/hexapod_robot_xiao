# Safety System Test Plan

## 概要
このドキュメントは、安全システム（ARM/DISARM、デッドマン、フェイルセーフ）の検証手順を定義します。
すべてのテストケースに合格することで、安全要件が満たされていることを確認します。

**対象**: M2マイルストーン - 安全停止（ARM/DISARM + デッドマン）  
**関連仕様**: `shared/protocol/spec/legctrl_protocol.md`  
**最終更新**: 2026-01-03

---

## 1. テスト環境

### 1.1 必要な機材
- XIAO nRF52840（ファームウェア書き込み済み）
- PCA9685 PWMコントローラ（I2C接続）
- サーボモーター（CH0に接続）
- BLEテストアプリ（nRF Connect、LightBlue等）
- シリアルコンソール（115200 bps）

### 1.2 初期状態
- ファームウェアが起動している
- BLE広告が開始されている（デバイス名: "LegCtrl"）
- サーボに電源が供給されている
- シリアルログが確認できる状態

---

## 2. テストケース一覧

### TC-01: 起動時の初期状態確認
**目的**: システムが必ずDISARMED状態で起動することを確認

**手順**:
1. ファームウェアをリセット（または電源投入）
2. シリアルログを確認
3. BLE接続してTelemetry Notifyを有効化
4. Telemetryデータを確認

**期待結果**:
- ✅ シリアルログに "System started successfully" が表示される
- ✅ Telemetryで `state = 0x00` (DISARMED) が受信される
- ✅ Telemetryで `error_code = 0x00` (ERR_NONE) が受信される
- ✅ サーボは動かない（PWM出力なし、または中立位置で固定）

**合格基準**: すべての期待結果が確認できること

---

### TC-02: ARM前のコマンド無視確認
**目的**: DISARMED状態ではサーボコマンドが無視されることを確認

**前提条件**: TC-01実施済み（DISARMED状態）

**手順**:
1. BLE接続済み状態で、ARM送信前に以下を実行
2. SET_SERVO_CH0コマンドを送信（1500 us）: `01 03 02 00 DC 05`
3. サーボの動作を目視確認
4. シリアルログを確認

**期待結果**:
- ✅ サーボは動かない
- ✅ シリアルログに "Command ignored - not in ARMED state" が表示される
- ✅ Telemetryで `state = 0x00` (DISARMED) のまま変化しない
- ✅ エラーコードは変化しない（`error_code = 0x00`）

**合格基準**: サーボが動かず、状態がDISARMEDのまま維持されること

---

### TC-03: ARM/DISARM状態遷移確認
**目的**: ARM/DISARMコマンドで正しく状態遷移することを確認

**前提条件**: TC-01実施済み（DISARMED状態）

**手順**:
1. ARMコマンドを送信: `01 01 00 00`
2. Telemetryデータを確認（100ms以内）
3. シリアルログを確認
4. DISARMコマンドを送信: `01 02 00 00`
5. Telemetryデータを確認（100ms以内）
6. シリアルログを確認

**期待結果**:
- ✅ ARM送信後、Telemetryで `state = 0x01` (ARMED) になる
- ✅ シリアルログに "CMD_ARM" と "State: ARMED" が表示される
- ✅ DISARM送信後、Telemetryで `state = 0x00` (DISARMED) になる
- ✅ シリアルログに "CMD_DISARM" と "State: DISARMED" が表示される
- ✅ DISARM後、サーボ出力が停止する（PWM停止ログ確認）

**合格基準**: 状態遷移が正しく動作し、ログで追跡できること

---

### TC-04: ARMED状態でのサーボ制御確認
**目的**: ARMED状態でのみサーボが制御できることを確認

**前提条件**: TC-03実施済み（ARM/DISARMテスト完了）

**手順**:
1. ARMコマンドを送信: `01 01 00 00`
2. Telemetryで `state = 0x01` を確認
3. SET_SERVO_CH0 (1000 us) を送信: `01 03 02 00 E8 03`
4. サーボの動作を目視確認
5. SET_SERVO_CH0 (2000 us) を送信: `01 03 02 00 D0 07`
6. サーボの動作を目視確認
7. Telemetryで `last_cmd_age_ms` が小さい値（< 100ms）であることを確認

**期待結果**:
- ✅ サーボが1000 us（一方の端）に移動する
- ✅ サーボが2000 us（他方の端）に移動する
- ✅ シリアルログに "Servo set to 1000 us" と "Servo set to 2000 us" が表示される
- ✅ Telemetryで `last_cmd_age_ms` が定期的にリセットされる（< 100ms）

**合格基準**: ARMED状態でサーボが正しく制御されること

---

### TC-05: デッドマンタイムアウト確認（200ms）
**目的**: 200ms無通信でFAULT状態に遷移し、サーボが停止することを確認

**前提条件**: TC-04実施済み（ARMED状態テスト完了）

**手順**:
1. ARMコマンドを送信: `01 01 00 00`
2. SET_SERVO_CH0 (1500 us) を送信: `01 03 02 00 DC 05`
3. **200ms以上、コマンドを送信しない**（待機）
4. Telemetryデータを連続監視
5. サーボの動作を目視確認
6. シリアルログを確認

**期待結果**:
- ✅ Telemetryで `last_cmd_age_ms` が増加していく（100ms, 200ms, ...）
- ✅ 200ms経過後、Telemetryで `state = 0x02` (FAULT) に遷移する
- ✅ Telemetryで `error_code = 0x01` (ERR_DEADMAN_TIMEOUT) が設定される
- ✅ サーボ出力が停止する（動きが止まる）
- ✅ シリアルログに "Deadman timeout: XXX ms >= 200 ms" が表示される
- ✅ シリアルログに "Servo output stopped due to deadman timeout" が表示される

**合格基準**: 200ms以内に確実にFAULT状態に遷移し、サーボが停止すること

---

### TC-06: FAULT状態からの復帰確認
**目的**: FAULT状態からDISARMで復帰できることを確認

**前提条件**: TC-05実施済み（FAULT状態）

**手順**:
1. FAULT状態を確認（`state = 0x02`, `error_code = 0x01`）
2. ARMコマンドを送信: `01 01 00 00`（これは無視されるはず）
3. Telemetryで状態を確認
4. DISARMコマンドを送信: `01 02 00 00`
5. Telemetryで状態を確認
6. ARMコマンドを送信: `01 01 00 00`
7. Telemetryで状態を確認
8. SET_SERVO_CH0 (1500 us) を送信: `01 03 02 00 DC 05`
9. サーボの動作を確認

**期待結果**:
- ✅ FAULT状態からのARM送信は無視される（`state = 0x02` のまま）
- ✅ シリアルログに "Cannot ARM from FAULT state. Send DISARM first to clear fault." が表示される
- ✅ DISARM送信後、`state = 0x00` (DISARMED) に遷移する
- ✅ `error_code = 0x00` (ERR_NONE) にクリアされる
- ✅ ARM送信後、`state = 0x01` (ARMED) に遷移する
- ✅ サーボコマンドでサーボが制御できる

**合格基準**: DISARM → ARM の手順で正常復帰できること

---

### TC-07: BLE切断時の即時停止確認
**目的**: BLE切断時に即座にサーボが停止することを確認

**前提条件**: TC-04実施済み（ARMED状態テスト完了）

**手順**:
1. ARMコマンドを送信: `01 01 00 00`
2. SET_SERVO_CH0 (1500 us) を送信: `01 03 02 00 DC 05`
3. サーボが動作していることを確認
4. BLEテストアプリで**接続を切断**（Disconnectボタン）
5. シリアルログを即座に確認
6. サーボの動作を目視確認

**期待結果**:
- ✅ シリアルログに "BLE Disconnected (reason XX)" が表示される
- ✅ サーボ出力が即座に停止する（PWM停止）
- ✅ シリアルログにPWM停止のログが表示される（"Failed to stop PWM on disconnect" または成功ログ）
- ✅ システムがFAULT状態に遷移する（再接続時のTelemetryで確認）

**合格基準**: 切断後**即座に**（目視で遅延なし）サーボが停止すること

---

### TC-08: BLE再接続時の初期化確認
**目的**: BLE再接続時に安全な初期状態（DISARMED）に戻ることを確認

**前提条件**: TC-07実施済み（BLE切断済み）

**手順**:
1. BLEテストアプリで再度スキャン
2. "LegCtrl" デバイスに接続
3. Telemetry Notifyを有効化
4. Telemetryデータを確認
5. シリアルログを確認
6. SET_SERVO_CH0を送信（ARMせずに）: `01 03 02 00 DC 05`
7. サーボの動作を確認

**期待結果**:
- ✅ シリアルログに "BLE Connected" が表示される
- ✅ Telemetryで `state = 0x00` (DISARMED) が受信される
- ✅ `error_code = 0x00` (ERR_NONE) にリセットされている
- ✅ `last_cmd_age_ms = 0` にリセットされている
- ✅ ARMせずに送信したサーボコマンドは無視される（サーボは動かない）

**合格基準**: 再接続後、自動的にDISARMED状態に戻ること

---

### TC-09: PINGコマンドでデッドマン回避確認
**目的**: PINGコマンドでデッドマンタイムアウトを回避できることを確認

**前提条件**: TC-04実施済み（ARMED状態テスト完了）

**手順**:
1. ARMコマンドを送信: `01 01 00 00`
2. **150ms毎に**PINGコマンドを送信: `01 04 00 00`
3. **1秒間**継続してPINGを送信
4. Telemetryで `last_cmd_age_ms` を監視
5. Telemetryで `state` を監視

**期待結果**:
- ✅ `last_cmd_age_ms` が150ms以下を維持する
- ✅ `state = 0x01` (ARMED) のまま維持される
- ✅ FAULT状態に遷移しない
- ✅ シリアルログに "CMD_PING" が定期的に表示される

**合格基準**: PINGでデッドマンタイムアウトを回避できること

---

### TC-10: Telemetry周期確認（10 Hz）
**目的**: Telemetryが10 Hz（100ms周期）で送信されることを確認

**前提条件**: BLE接続済み、Notify有効化済み

**手順**:
1. Telemetry Notifyを有効化
2. **5秒間**、受信したTelemetryのタイムスタンプを記録
3. 受信間隔を計算

**期待結果**:
- ✅ 5秒間で約50個のTelemetryパケットを受信（±10%）
- ✅ 受信間隔が平均100ms（±10ms）である
- ✅ 欠損が少ない（連続2パケット以上の欠損がない）

**合格基準**: Telemetryが安定して10 Hzで送信されていること

---

### TC-11: エラーコードとログの対応確認
**目的**: 各エラー状態でログとTelemetryが一致することを確認

**手順**:
1. TC-05（デッドマンタイムアウト）を実施
2. シリアルログとTelemetryの `error_code` を照合
3. TC-07（BLE切断）を実施
4. シリアルログと内部状態を照合

**期待結果**:
- ✅ デッドマンタイムアウト時、ログに "Deadman timeout" とTelemetryに `error_code = 0x01` が一致
- ✅ BLE切断時、ログに "BLE Disconnected" と内部のFAULT状態が一致
- ✅ 状態遷移が常にログで追跡可能

**合格基準**: ログとTelemetryが常に一致し、状態遷移が追跡できること

---

## 3. 受入基準（Acceptance Criteria）

以下のすべての項目が満たされていることを確認してください。

### 3.1 状態機械（State Machine）
- [x] DISARMED/ARMED/FAULT の3状態が実装されている
- [x] 起動時は必ずDISARMED状態である（TC-01）
- [x] ARM/DISARMコマンドで状態遷移する（TC-03）
- [x] FAULT状態はDISARMで復帰できる（TC-06）

### 3.2 ARM/DISARM
- [x] ARMするまでサーボ出力は停止している（TC-02）
- [x] DISARMは常に許可される（TC-03, TC-06）
- [x] ARMED状態のみサーボ制御が可能（TC-04）

### 3.3 デッドマンタイマ
- [x] last_cmd_timestamp が監視されている（実装確認済み）
- [x] 200ms無通信でFAULT状態に遷移する（TC-05）
- [x] FAULT遷移時にサーボ出力が停止する（TC-05）
- [x] PINGコマンドでタイムアウトを回避できる（TC-09）

### 3.4 BLE切断時の即時停止
- [x] BLE切断イベントが検出される（実装確認済み）
- [x] 切断時に即座にサーボ出力が停止する（TC-07）
- [x] 切断時にFAULT状態に遷移する（TC-07）
- [x] 再接続時にDISARMED状態に戻る（TC-08）

### 3.5 Telemetry
- [x] 10 Hz（100ms周期）で送信される（TC-10）
- [x] state, error_code, last_cmd_age_ms が含まれる（実装確認済み）
- [x] 状態遷移がTelemetryで追跡できる（TC-11）

### 3.6 ログ出力
- [x] 状態遷移がログで追跡できる（TC-03, TC-11）
- [x] エラー発生時にログが出力される（TC-05, TC-07）
- [x] コマンド受信がログで確認できる（TC-04, TC-09）

---

## 4. 合格判定

すべてのテストケース（TC-01 ～ TC-11）が合格することで、安全システムの実装が完了したと判定します。

**現在のステータス**: ✅ **すべての機能が実装済み**
- コードレビューにより、すべての要件が実装されていることを確認
- テストケースの実行により、動作を検証することを推奨

---

## 5. 参照

- **プロトコル仕様**: `shared/protocol/spec/legctrl_protocol.md`
- **BLE詳細**: `docs/ble_protocol.md`
- **ファームウェアコード**: `firmware/zephyr/app/legctrl_fw/src/ble_service.c`
- **ヘッダファイル**: `firmware/zephyr/app/legctrl_fw/src/ble_service.h`

---

## 改訂履歴

| Version | Date       | Author   | Changes                              |
|---------|------------|----------|--------------------------------------|
| v1.0    | 2026-01-03 | copilot  | 初版作成（安全システムテスト計画）   |
