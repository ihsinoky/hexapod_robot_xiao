# BLE Protocol Details

## 概要
このドキュメントは、BLE（Bluetooth Low Energy）プロトコルの詳細（GATT、Characteristic、UUID、パケット構造）を記述します。
汎用BLEテストアプリ（nRF Connect、LightBlue等）を使用して手動検証できる状態を確保します。

**関連ドキュメント**:
- メッセージフォーマット・状態遷移の詳細: `shared/protocol/spec/legctrl_protocol.md`
- システム構成: `README.md`

---

## 1. GATT Service / Characteristic 構成

### 1.1 Service UUID
本システムでは、以下のカスタムService UUIDを使用します。

**Service UUID**: `12345678-1234-1234-1234-123456789abc`

> **注意**: これはプロトタイプ用のUUIDです。本番環境では適切なUUID（登録済みまたはランダム生成）を使用してください。

### 1.2 Characteristic 一覧

| Characteristic | UUID | Properties | Direction | Description |
|----------------|------|------------|-----------|-------------|
| Command | `12345678-1234-1234-1234-123456789abd` | Write, Write Without Response | iOS → FW | コマンド送信用（ARM/DISARM/SET_SERVO/PING） |
| Telemetry | `12345678-1234-1234-1234-123456789abe` | Notify | FW → iOS | テレメトリデータ通知（状態/電圧/エラー） |

### 1.3 Characteristic 詳細

#### Command Characteristic
- **UUID**: `12345678-1234-1234-1234-123456789abd`
- **Properties**: `Write`, `Write Without Response`
- **用途**: iOS → FW へのコマンド送信
- **推奨Write方式**: **Write Without Response**
  - 理由: 低遅延を優先。ACKを待たずに次のコマンドを送信可能
  - デッドマン方式により通信失敗は自動検出されるため、個別ACKは不要
  - 高頻度送信（50-60Hz）に適している

> **補足**: `Write` (Write with Response) も使用可能ですが、RTT（Round Trip Time）が増加し、遅延が発生する可能性があります。通常のオペレーションでは `Write Without Response` を推奨します。

#### Telemetry Characteristic
- **UUID**: `12345678-1234-1234-1234-123456789abe`
- **Properties**: `Notify`
- **用途**: FW → iOS へのテレメトリ通知
- **通知周期**: **10 Hz（100 ms周期）± 10%**
- **Payload**: 状態、エラーコード、最終コマンド経過時間、バッテリー電圧等

---

## 2. MTU / パケット長の考慮事項

### 2.1 MTU（Maximum Transmission Unit）
- **想定最小MTU**: **23 bytes**（BLE 4.2デフォルト）
- **実効ペイロード**: MTU - 3 bytes（ATTヘッダ） = **20 bytes**

### 2.2 パケット長設計
本プロトコルは、**MTU = 23 bytes（実効20 bytes）でも成立する**ように設計されています。

#### コマンドパケット（最大例）
```
[Header: 4 bytes] + [Payload: 最大2 bytes] = 6 bytes
```
- CMD_ARM: 4 bytes
- CMD_DISARM: 4 bytes
- CMD_SET_SERVO_CH0: 6 bytes (ヘッダ4 + pulse_us 2)
- CMD_PING: 4 bytes

#### テレメトリパケット
```
[Header: 4 bytes] + [Payload: 8 bytes] = 12 bytes
```

すべてのパケットは20 bytes未満に収まるため、**最小MTUでも分割なしに送信可能**です。

### 2.3 MTUネゴシエーション
- iOS側（Central）は接続時にMTU拡張を試みることが推奨されますが、必須ではありません
- ファームウェア側（Peripheral）は最小MTU（23 bytes）での動作を保証します
- より大きなMTU（例: 185 bytes以上）が利用可能な場合、将来的な拡張（複数チャンネル同時更新等）に活用できます

---

## 3. Notify周期とPayload概要

### 3.1 Telemetry Notify仕様
- **周期**: 10 Hz（100 ms間隔）± 10%
- **送信方式**: 定期的なNotify（イベント駆動ではなく周期駆動）
- **用途**:
  - iOS側でのUI更新（バッテリー電圧、状態表示）
  - デッドマン監視（`last_cmd_age_ms` の確認）
  - エラー検出とフォールト状態の通知

### 3.2 Telemetry Payload構造（概要）
```
Total: 12 bytes
├─ Header (4 bytes)
│  ├─ version: 0x01
│  ├─ msg_type: 0x10 (TELEMETRY)
│  ├─ payload_len: 0x08
│  └─ reserved: 0x00
└─ Payload (8 bytes)
   ├─ state: 1 byte (DISARMED/ARMED/FAULT)
   ├─ error_code: 1 byte (エラーコード)
   ├─ last_cmd_age_ms: 2 bytes (最終コマンドからの経過時間)
   ├─ battery_mv: 2 bytes (バッテリー電圧 mV)
   └─ reserved: 2 bytes (将来用)
```

詳細は `shared/protocol/spec/legctrl_protocol.md` を参照してください。

---

## 4. 手動テスト手順（nRF Connect / LightBlue）

汎用BLEテストアプリを使用して、「接続 → Write → Notify確認」を実施できます。

### 4.1 必要なツール
- **iOS**: nRF Connect for Mobile, LightBlue
- **Android**: nRF Connect for Mobile
- **macOS**: LightBlue

### 4.2 テスト手順

#### Step 1: デバイスをスキャンして接続
1. nRF ConnectまたはLightBlueを起動
2. スキャンを開始
3. デバイス名を確認（例: "LegCtrl" または XIAO nRF52840のデフォルト名）
4. デバイスをタップして接続

#### Step 2: Serviceを確認
1. 接続後、Service一覧を表示
2. Service UUID `12345678-1234-1234-1234-123456789abc` を探す
3. Serviceを展開してCharacteristicを表示

#### Step 3: Telemetry Notifyを有効化
1. Telemetry Characteristic (`...789abe`) を選択
2. **Notify を有効化**（通知アイコンをタップ）
3. 100ms周期でテレメトリデータが受信されることを確認
4. 初期状態では `state = 0x00` (DISARMED) が表示されるはず

**受信例（Hex）**:
```
01 10 08 00 00 00 00 00 E8 1C 00 00
```
- state = 0x00 (DISARMED)
- error_code = 0x00 (正常)
- last_cmd_age_ms = 0x0000 (0 ms)
- battery_mv = 0x1CE8 (7400 mV)

#### Step 4: ARM コマンドを送信
1. Command Characteristic (`...789abd`) を選択
2. **Write type**: "Write Without Response" を選択（推奨）
3. **値を入力**（Hex）: `01 01 00 00`
4. **Write** ボタンを押す

**期待される結果**:
- Telemetry Notifyで `state = 0x01` (ARMED) が受信される

**受信例（Hex）**:
```
01 10 08 00 01 00 05 00 E8 1C 00 00
```
- state = 0x01 (ARMED)

#### Step 5: サーボコマンドを送信（1500 us）
1. Command Characteristic (`...789abd`) を選択
2. **値を入力**（Hex）: `01 03 02 00 DC 05`
   - `01`: version
   - `03`: msg_type (CMD_SET_SERVO_CH0)
   - `02`: payload_len
   - `00`: reserved
   - `DC 05`: pulse_us = 1500 (0x05DC, Little Endian)
3. **Write** ボタンを押す

**期待される結果**:
- サーボ（CH0）が1500 us（中立位置）に移動
- Telemetry Notifyで `last_cmd_age_ms` が小さい値（< 100ms）で更新される

#### Step 6: サーボコマンドを送信（2000 us）
1. **値を入力**（Hex）: `01 03 02 00 D0 07`
   - pulse_us = 2000 (0x07D0, Little Endian)
2. **Write** ボタンを押す

**期待される結果**:
- サーボ（CH0）が2000 usに移動

#### Step 7: DISARM コマンドを送信
1. **値を入力**（Hex）: `01 02 00 00`
2. **Write** ボタンを押す

**期待される結果**:
- Telemetry Notifyで `state = 0x00` (DISARMED) が受信される
- サーボ出力が停止（PWM信号なし）

#### Step 8: デッドマンタイムアウトの確認
1. ARM コマンドを送信（`01 01 00 00`）
2. **200ms以上何もコマンドを送らない**
3. Telemetry Notifyを観察

**期待される結果**:
- `last_cmd_age_ms` が増加し続ける（100ms, 200ms, ...）
- 200ms到達後、`state = 0x02` (FAULT), `error_code = 0x01` (ERR_DEADMAN_TIMEOUT) に遷移

**受信例（Hex）**:
```
01 10 08 00 02 01 C8 00 E8 1C 00 00
```
- state = 0x02 (FAULT)
- error_code = 0x01 (ERR_DEADMAN_TIMEOUT)
- last_cmd_age_ms = 0x00C8 (200 ms)

---

## 5. コマンド送信例（Quick Reference）

### 5.1 コマンド一覧（Hex表記）

| コマンド | Hex値 | 説明 |
|---------|-------|------|
| ARM | `01 01 00 00` | システムをARMED状態に遷移 |
| DISARM | `01 02 00 00` | システムをDISARMED状態に遷移（安全停止） |
| SET_SERVO_CH0 (500 us) | `01 03 02 00 F4 01` | サーボCH0を500 usに設定 |
| SET_SERVO_CH0 (1500 us) | `01 03 02 00 DC 05` | サーボCH0を1500 us（中立）に設定 |
| SET_SERVO_CH0 (2000 us) | `01 03 02 00 D0 07` | サーボCH0を2000 usに設定 |
| SET_SERVO_CH0 (2500 us) | `01 03 02 00 C4 09` | サーボCH0を2500 usに設定 |
| PING | `01 04 00 00` | キープアライブ（デッドマン回避） |

### 5.2 パルス幅 ↔ Hex変換補助

Little Endianのため、バイト順が逆になります。

| パルス幅 (us) | Decimal | Hex (Big Endian) | Hex (Little Endian) |
|--------------|---------|------------------|---------------------|
| 500 | 500 | 0x01F4 | `F4 01` |
| 1000 | 1000 | 0x03E8 | `E8 03` |
| 1500 | 1500 | 0x05DC | `DC 05` |
| 2000 | 2000 | 0x07D0 | `D0 07` |
| 2500 | 2500 | 0x09C4 | `C4 09` |

**計算例**:
- 1750 us を送る場合
  - Decimal: 1750
  - Hex: 0x06D6
  - Little Endian: `D6 06`
  - コマンド全体: `01 03 02 00 D6 06`

---

## 6. トラブルシューティング

### 6.1 Writeが成功しない
- **Write type**: "Write Without Response" を使用しているか確認
- **Characteristic UUID**: 正しいCommand Characteristic (`...789abd`) に送信しているか確認
- **接続状態**: BLE接続が確立しているか確認

### 6.2 Notifyが受信できない
- **Notify有効化**: Telemetry Characteristic (`...789abe`) のNotifyを有効にしたか確認
- **CCCD設定**: 一部のアプリでは明示的にCCCD（Client Characteristic Configuration Descriptor）を設定する必要があります

### 6.3 サーボが動かない
- **状態確認**: Telemetry Notifyで `state = 0x01` (ARMED) になっているか確認
- **ARM送信**: DISARMEDの場合、先にARMコマンドを送信
- **パルス幅**: 500–2500 usの範囲内か確認
- **エンディアン**: Little Endianで送信しているか確認（例: 1500 = `DC 05`）

### 6.4 デッドマンタイムアウトが発生する
- **送信周期**: 200ms以内に有効なコマンドを送信しているか確認
- **ARMED状態**: ARMED状態での有効コマンドは `CMD_SET_SERVO_CH0`, `CMD_PING` のみ
- **対策**: 操作がない場合でも100ms周期でPINGを送信

---

## 7. 実装チェックリスト

### 7.1 ファームウェア実装者
- [ ] Service UUID `12345678-1234-1234-1234-123456789abc` を登録
- [ ] Command Characteristic (`...789abd`) を `Write`, `Write Without Response` プロパティで実装
- [ ] Telemetry Characteristic (`...789abe`) を `Notify` プロパティで実装
- [ ] Telemetry を 10 Hz（100 ms周期）で送信
- [ ] MTU 23 bytes（実効20 bytes）での動作を保証
- [ ] デッドマンタイムアウト（200ms）を実装

### 7.2 iOS実装者
- [ ] Service UUID `12345678-1234-1234-1234-123456789abc` でサービスを検索
- [ ] Command Characteristic へのWriteは "Write Without Response" を使用
- [ ] Telemetry Characteristic のNotifyを有効化（CCCD設定）
- [ ] 50–60 Hz周期でコマンドを送信（デッドマン余裕確保）
- [ ] Telemetry `last_cmd_age_ms` を監視し、150ms超過時にPINGを送信
- [ ] Little Endianでパケットを組み立て

---

## 8. 参照

- **メッセージフォーマット詳細**: `shared/protocol/spec/legctrl_protocol.md`
  - ヘッダ構造、コマンド定義、テレメトリ詳細、エラーコード、デッドマン仕様
- **システム構成**: `README.md`
- **マイルストーン**: `MILESTONE.md`

---

## 改訂履歴

| Version | Date       | Author   | Changes                        |
|---------|------------|----------|--------------------------------|
| v1.0    | 2025-12-31 | copilot  | GATT/UUID/手動テスト手順の追加  |
