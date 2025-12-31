# BLE Protocol Specification (legctrl_protocol) v0.1

## 概要
このドキュメントは、iPad（iOS）アプリと XIAO nRF52840 ファームウェア間のBLE通信プロトコルの正式仕様書（v0.1）です。
FW/iOS 両方の実装は本仕様を「正本」として参照し、曖昧性を排除することでブレを防ぎます。

**バージョン**: v0.1 (Sprint 1 最小縦切り用)  
**対象マイルストーン**: M1, M2  
**最終更新**: 2025-12-31

---

## 1. メッセージ形式（Message Format）

### 1.1 フレーミング（Framing）
BLE GATT Characteristic の Write/Notify を使用します。1つの Write/Notify が 1つの完結したメッセージを表します。

- **MTU**: 最小 23 bytes を想定（BLE 4.2 デフォルト）
- **パケット構造**: `[Header] + [Payload]`
- **バイトオーダー**: **Little Endian** （すべての multi-byte フィールド）

### 1.2 共通ヘッダ（Common Header）
すべてのメッセージは以下のヘッダで始まります。

```
Offset | Size | Field          | Description
-------|------|----------------|----------------------------------
0      | 1    | version        | プロトコルバージョン (0x01 = v0.1)
1      | 1    | msg_type       | メッセージタイプ (下記参照)
2      | 1    | payload_len    | ペイロード長 (bytes)
3      | 1    | reserved       | 予約（将来用、常に 0x00）
4      | N    | payload        | メッセージ固有のペイロード
```

**合計ヘッダサイズ**: 4 bytes

### 1.3 メッセージタイプ（msg_type）

| Value | Name                | Direction      | Description                    |
|-------|---------------------|----------------|--------------------------------|
| 0x01  | CMD_ARM             | iOS → FW       | ARM 要求                       |
| 0x02  | CMD_DISARM          | iOS → FW       | DISARM 要求                    |
| 0x03  | CMD_SET_SERVO_CH0   | iOS → FW       | サーボ CH0 設定                |
| 0x04  | CMD_PING            | iOS → FW       | Ping/Keepalive                 |
| 0x10  | TELEMETRY           | FW → iOS       | テレメトリデータ               |

---

## 2. 状態（States）

ファームウェアは以下の3つの状態を持ちます。

### 2.1 状態定義

| State       | Value | Description                                                      |
|-------------|-------|------------------------------------------------------------------|
| DISARMED    | 0x00  | 安全状態。サーボ出力は停止（PWM信号なし）。コマンド受付しない。    |
| ARMED       | 0x01  | 動作可能状態。SET_SERVO コマンドを受け付け、サーボを駆動する。    |
| FAULT       | 0x02  | エラー状態。安全のため停止。復帰には DISARM → ARM が必要。       |

### 2.2 状態遷移図

```
起動時
  │
  v
[DISARMED] ──CMD_ARM──> [ARMED]
  ^                        │
  │                        │ CMD_DISARM
  │                        │ OR デッドマンタイムアウト
  │                        │ OR エラー検出
  │                        v
  └───CMD_DISARM────── [FAULT]
                          │
                          │ CMD_DISARM (クリア)
                          v
                      [DISARMED]
```

**遷移ルール**:
1. **起動時**: DISARMED 状態で開始
2. **DISARMED → ARMED**: CMD_ARM コマンドを受信
3. **ARMED → DISARMED**: CMD_DISARM コマンドを受信
4. **ARMED → FAULT**: デッドマンタイムアウト、または内部エラー検出時
5. **FAULT → DISARMED**: CMD_DISARM コマンドを受信（エラークリア）

---

## 3. コマンド定義（Commands）

### 3.1 CMD_ARM (0x01)
システムを ARMED 状態に遷移させます。

**Header**:
- `version`: 0x01
- `msg_type`: 0x01
- `payload_len`: 0 (ペイロードなし)
- `reserved`: 0x00

**Payload**: なし

**Example (hex)**:
```
01 01 00 00
```

**応答**: テレメトリで state = ARMED (0x01) を返す

---

### 3.2 CMD_DISARM (0x02)
システムを DISARMED 状態に遷移させます。FAULT 状態からの復帰にも使用します。

**Header**:
- `version`: 0x01
- `msg_type`: 0x02
- `payload_len`: 0
- `reserved`: 0x00

**Payload**: なし

**Example (hex)**:
```
01 02 00 00
```

**応答**: テレメトリで state = DISARMED (0x00) を返す

---

### 3.3 CMD_SET_SERVO_CH0 (0x03)
サーボチャンネル0のパルス幅を設定します（ARMED 状態のみ有効）。

**Header**:
- `version`: 0x01
- `msg_type`: 0x03
- `payload_len`: 2
- `reserved`: 0x00

**Payload**:
```
Offset | Size | Field          | Description
-------|------|----------------|----------------------------------
0      | 2    | pulse_us       | パルス幅（マイクロ秒）、範囲: 500–2500
```

**単位**: マイクロ秒 (us)  
**範囲**: 500 ~ 2500 us  
**推奨中立**: 1500 us  
**エンディアン**: Little Endian

**Example (hex)**: 1500 us (0x05DC) を設定
```
01 03 02 00 DC 05
```

**Example (hex)**: 2000 us (0x07D0) を設定
```
01 03 02 00 D0 07
```

**動作**:
- ARMED 状態の場合: pulse_us の値をサーボに出力
- DISARMED/FAULT 状態の場合: コマンドは無視される（テレメトリで状態確認可能）

---

### 3.4 CMD_PING (0x04)
デッドマンタイマーをリセットするためのキープアライブコマンド。

**Header**:
- `version`: 0x01
- `msg_type`: 0x04
- `payload_len`: 0
- `reserved`: 0x00

**Payload**: なし

**Example (hex)**:
```
01 04 00 00
```

**用途**: 
- サーボコマンドを送らない期間もデッドマンタイムアウトを防ぐ
- 接続維持確認

---

## 4. テレメトリ（Telemetry）

ファームウェアはテレメトリデータを **10 Hz（100 ms 周期）±10% の周期で必ず** Notify 送信しなければなりません（デッドマン監視用の `last_cmd_age_ms` はこの周期を前提とする）。

### 4.1 TELEMETRY (0x10)

**Header**:
- `version`: 0x01
- `msg_type`: 0x10
- `payload_len`: 8
- `reserved`: 0x00

**Payload**:
```
Offset | Size | Field            | Description
-------|------|------------------|----------------------------------
0      | 1    | state            | 現在の状態 (0x00=DISARMED, 0x01=ARMED, 0x02=FAULT)
1      | 1    | error_code       | エラーコード (0x00=正常, 下記参照)
2      | 2    | last_cmd_age_ms  | 最終有効コマンドからの経過時間 (ms)
4      | 2    | battery_mv       | バッテリー電圧 (mV) ※スタブ可
6      | 2    | reserved         | 予約（将来用、常に 0x0000）
```

**単位**:
- `state`: 列挙型（上記参照）
- `error_code`: 列挙型（下記参照）
- `last_cmd_age_ms`: ミリ秒 (ms)
- `battery_mv`: ミリボルト (mV)
- **エンディアン**: Little Endian

**Example (hex)**: state=ARMED, no error, 50ms since last cmd, 7400mV battery
```
01 10 08 00 01 00 32 00 E8 1C 00 00
```

解説:
- `01`: version
- `10`: msg_type (TELEMETRY)
- `08`: payload_len
- `00`: reserved
- `01`: state (ARMED)
- `00`: error_code (no error)
- `32 00`: last_cmd_age_ms = 50 (0x0032)
- `E8 1C`: battery_mv = 7400 (0x1CE8)
- `00 00`: reserved

---

### 4.2 エラーコード（error_code）

| Value | Name                  | Description                          |
|-------|-----------------------|--------------------------------------|
| 0x00  | ERR_NONE              | エラーなし                           |
| 0x01  | ERR_DEADMAN_TIMEOUT   | デッドマンタイムアウト               |
| 0x02  | ERR_LOW_BATTERY       | 低電圧検出                           |
| 0x03  | ERR_I2C_FAULT         | I2C 通信エラー                       |
| 0x04  | ERR_INVALID_CMD       | 無効なコマンド受信                   |
| 0xFF  | ERR_UNKNOWN           | 不明なエラー                         |

---

## 5. デッドマン仕様（Deadman Specification）

### 5.1 基本動作
「最終有効コマンド」からの経過時間が閾値を超えた場合、ファームウェアは自動的に FAULT 状態へ遷移し、サーボ出力を停止します。

**最終有効コマンド**: CMD_ARM, CMD_SET_SERVO_CH0, CMD_PING のいずれか

### 5.2 タイムアウト閾値
**デフォルト値**: **200 ms**

### 5.3 動作シーケンス
1. ARMED 状態で有効コマンドを受信 → タイマーリセット
2. 200 ms 間、有効コマンドが届かない → FAULT 状態へ遷移
3. サーボ出力停止（PWM出力を完全停止し、サーボは保持動作を行わない）
4. テレメトリで `state = FAULT`, `error_code = ERR_DEADMAN_TIMEOUT` を送信
5. 復帰: CMD_DISARM → CMD_ARM

### 5.4 BLE切断時の挙動
BLE接続が切断された場合:
1. 即座に FAULT 状態へ遷移
2. サーボ出力停止
3. 再接続時は DISARMED 状態で起動（ARMコマンドが必要）

---

## 6. 単位系（Units）

明確化のため、すべての物理量は以下の単位を使用します。

| Quantity              | Unit            | Field Name           | Range/Notes              |
|-----------------------|-----------------|----------------------|--------------------------|
| サーボパルス幅        | マイクロ秒 (us) | pulse_us             | 500 ~ 2500 us            |
| 時間（経過時間）      | ミリ秒 (ms)     | last_cmd_age_ms      | 0 ~ 65535                |
| 電圧                  | ミリボルト (mV) | battery_mv           | 0 ~ 65535 (例: 7400 mV)  |
| プロトコルバージョン  | -               | version              | 0x01 (v0.1)              |

---

## 7. 使用例（Usage Examples）

### 7.1 基本操作シーケンス（ARM → SET_SERVO → DISARM）

**前提**: ファームウェアは DISARMED 状態、BLE接続済み

#### Step 1: ARM
iOS → FW:
```
01 01 00 00
```

FW → iOS (Telemetry):
```
01 10 08 00 01 00 00 00 E8 1C 00 00
```
（state=ARMED, last_cmd_age_ms=0）

---

#### Step 2: SET_SERVO_CH0 (1500 us)
iOS → FW:
```
01 03 02 00 DC 05
```

FW → iOS (Telemetry, ~100ms後):
```
01 10 08 00 01 00 64 00 E8 1C 00 00
```
（state=ARMED, last_cmd_age_ms=100）

---

#### Step 3: SET_SERVO_CH0 (2000 us)
iOS → FW:
```
01 03 02 00 D0 07
```

FW → iOS (Telemetry):
```
01 10 08 00 01 00 05 00 E8 1C 00 00
```
（state=ARMED, last_cmd_age_ms=5, サーボ位置更新済み）

---

#### Step 4: DISARM
iOS → FW:
```
01 02 00 00
```

FW → iOS (Telemetry):
```
01 10 08 00 00 00 00 00 E8 1C 00 00
```
（state=DISARMED, サーボ出力停止）

---

### 7.2 デッドマンタイムアウトシナリオ

**前提**: ARMED 状態、最後のコマンドから 200ms 経過

FW → iOS (Telemetry):
```
01 10 08 00 02 01 C8 00 E8 1C 00 00
```
解説:
- `state = 0x02` (FAULT)
- `error_code = 0x01` (ERR_DEADMAN_TIMEOUT)
- `last_cmd_age_ms = 0x00C8` (200 ms)

**復帰手順**:
1. iOS → FW: CMD_DISARM (エラークリア)
2. iOS → FW: CMD_ARM (再起動)

---

### 7.3 汎用BLEアプリでの送信例

**nRF Connect / LightBlue 等を使用**:

1. **プロトコルで定義された Service UUID を選択する**（正式な Service/Characteristic UUID は本仕様および `docs/ble_protocol.md` を参照。以下はスキャナ画面上での表示例: `12345678-1234-1234-1234-123456789abc`）
2. **Command Characteristic (Write)** を開く
3. **ARM コマンド送信**:
   - 値: `01 01 00 00` (Hex)
4. **サーボ設定 (1500 us)**:
   - 値: `01 03 02 00 DC 05` (Hex)
5. **DISARM コマンド送信**:
   - 値: `01 02 00 00` (Hex)
6. **Telemetry Characteristic (Notify)** を Subscribe して受信確認

---

## 8. 実装ガイドライン

### 8.1 ファームウェア (FW) 実装者向け
- テレメトリは 10 Hz (100 ms 周期) で Notify 送信を推奨
- デッドマンタイマーは 1 ms 精度でカウント
- サーボパルス幅の範囲チェック (500–2500 us) を実施
- DISARMED/FAULT 状態では SET_SERVO コマンドを無視

### 8.2 iOS 実装者向け
- コマンド送信は 30–60 Hz を推奨（過剰送信を避ける）
- デッドマン回避のため、操作がない場合も PING を定期送信
- Telemetry の `last_cmd_age_ms` を監視し、200 ms 超過前に送信
- 再接続時は必ず DISARMED 状態を想定し、ARM から開始

### 8.3 共通注意事項
- **バイトオーダー**: すべて Little Endian
- **単位の厳守**: pulse_us (us), last_cmd_age_ms (ms), battery_mv (mV)
- **状態遷移の順守**: 不正な遷移は受け付けない

---

## 9. 将来の拡張予定（Out of Scope for v0.1）

以下は v0.2 以降で検討します:
- 複数サーボチャンネル対応（CMD_SET_SERVO_MULTI）
- キャリブレーション設定（Config Characteristic）
- ACK/NACK 応答（コマンド受理確認）
- エラーログ取得
- バージョンネゴシエーション

---

## 10. 参照

- **詳細な GATT 仕様**: `docs/ble_protocol.md`（UUID、Characteristic 詳細）
- **システム構成**: `README.md`
- **マイルストーン**: `MILESTONE.md`

---

## 改訂履歴

| Version | Date       | Author   | Changes                        |
|---------|------------|----------|--------------------------------|
| v0.1    | 2025-12-31 | copilot  | Initial minimal specification  |
