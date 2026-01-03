# Safety Implementation Summary

## 概要
このドキュメントは、安全システム（ARM/DISARM、デッドマン、フェイルセーフ）の実装詳細を説明します。
コードレビュー時の参照資料として使用します。

**対象イシュー**: [S1-07] Safety実装：ARM/DISARM + デッドマン + 切断時停止（Fail-safe）  
**マイルストーン**: M2 - 安全停止  
**最終更新**: 2026-01-03

---

## 1. 実装状況サマリ

### 1.1 要件と実装状況

| 要件 | 実装ファイル | 実装場所 | ステータス |
|------|------------|----------|-----------|
| 状態機械（DISARMED/ARMED/FAULT） | `ble_service.h` | Lines 22-25 | ✅ 完了 |
| 状態管理変数 | `ble_service.c` | Lines 54-66 | ✅ 完了 |
| ARM/DISARMコマンド定義 | `ble_service.h` | Lines 16-17 | ✅ 完了 |
| ARM処理 | `ble_service.c` | Lines 267-280 | ✅ 完了 |
| DISARM処理 | `ble_service.c` | Lines 282-302 | ✅ 完了 |
| デッドマンタイマ | `ble_service.c` | Lines 427-451 | ✅ 完了 |
| デッドマンタイムアウト定義 | `ble_service.c` | Line 33 | ✅ 完了 (200ms) |
| BLE切断検出 | `ble_service.c` | Lines 112-133 | ✅ 完了 |
| Telemetry送信 | `ble_service.c` | Lines 354-395 | ✅ 完了 |
| エラーコード定義 | `ble_service.h` | Lines 27-33 | ✅ 完了 |

**結論**: すべての要件が実装済み

---

## 2. 状態機械（State Machine）

### 2.1 状態定義

```c
/* System states */
#define STATE_DISARMED 0x00
#define STATE_ARMED    0x01
#define STATE_FAULT    0x02
```

**ファイル**: `firmware/zephyr/app/legctrl_fw/src/ble_service.h:22-25`

### 2.2 状態変数

```c
static struct {
    uint8_t state;
    uint8_t error_code;
    int64_t last_cmd_timestamp;
    bool telemetry_enabled;
    struct bt_conn *conn;
} sys_state = {
    .state = STATE_DISARMED,  // 起動時は必ずDISARMED
    .error_code = ERR_NONE,
    .last_cmd_timestamp = 0,
    .telemetry_enabled = false,
    .conn = NULL,
};
```

**ファイル**: `firmware/zephyr/app/legctrl_fw/src/ble_service.c:54-66`

**ポイント**:
- 起動時の初期値として `STATE_DISARMED` が設定されている
- エラーコードは `ERR_NONE` で初期化
- `last_cmd_timestamp = 0` により、ARM前はデッドマン監視されない

### 2.3 状態遷移図（実装ベース）

```
[起動]
  │
  └─> [DISARMED] ────────────────────┐
         ^    │                       │
         │    │ CMD_ARM               │
         │    │ (process_cmd_arm)     │
         │    v                       │
         │  [ARMED] ──────────────────┤
         │    │                       │
         │    │ - CMD_DISARM          │ CMD_DISARM
         │    │ - Deadman timeout     │ (Always allowed)
         │    │ - I2C fault           │
         │    │ - BLE disconnect      │
         │    v                       │
         └─ [FAULT] ────────────────────┘
```

**主要な遷移関数**:
- `process_cmd_arm()`: DISARMED → ARMED
- `process_cmd_disarm()`: ANY → DISARMED
- `ble_service_update_deadman()`: ARMED → FAULT (timeout)
- `disconnected()`: ANY → FAULT

---

## 3. ARM/DISARMコマンド実装

### 3.1 ARMコマンド処理

**ファイル**: `firmware/zephyr/app/legctrl_fw/src/ble_service.c:267-280`

```c
static void process_cmd_arm(void)
{
    LOG_INF("CMD_ARM");
    
    if (sys_state.state == STATE_DISARMED) {
        sys_state.state = STATE_ARMED;
        sys_state.error_code = ERR_NONE;
        sys_state.last_cmd_timestamp = k_uptime_get();  // デッドマン開始
        LOG_INF("State: ARMED");
    } else if (sys_state.state == STATE_FAULT) {
        LOG_WRN("Cannot ARM from FAULT state. Send DISARM first to clear fault.");
    }
}
```

**動作**:
- DISARMED状態からのみARMED状態に遷移可能
- FAULT状態からは直接ARM不可（安全のため、DISARM経由を強制）
- ARM成功時、`last_cmd_timestamp` を初期化（デッドマン監視開始）
- エラーコードをクリア

### 3.2 DISARMコマンド処理

**ファイル**: `firmware/zephyr/app/legctrl_fw/src/ble_service.c:282-302`

```c
static void process_cmd_disarm(void)
{
    LOG_INF("CMD_DISARM");
    
    sys_state.state = STATE_DISARMED;
    sys_state.error_code = ERR_NONE;
    sys_state.last_cmd_timestamp = 0;  // デッドマン停止
    
    /* Stop PWM output */
    if (pwm_dev && device_is_ready(pwm_dev)) {
        int ret = pwm_set(pwm_dev, SERVO_CHANNEL_0, SERVO_PWM_PERIOD_NS, 0, 0);
        if (ret < 0) {
            LOG_ERR("Failed to stop servo output (err %d)", ret);
        } else {
            LOG_INF("Servo output stopped");
        }
    }
    
    LOG_INF("State: DISARMED");
}
```

**動作**:
- **常に許可**（どの状態からでもDISARMED状態に遷移可能）
- PWM出力を即座に停止（`pulse_width = 0`）
- エラーコードをクリア（FAULT復帰に使用）
- `last_cmd_timestamp = 0` でデッドマン監視を停止

---

## 4. デッドマンタイマ実装

### 4.1 タイムアウト定義

**ファイル**: `firmware/zephyr/app/legctrl_fw/src/ble_service.c:33`

```c
/* Deadman timeout (ms) */
#define DEADMAN_TIMEOUT_MS     200
```

**仕様**: 200ミリ秒

### 4.2 デッドマン更新関数

**ファイル**: `firmware/zephyr/app/legctrl_fw/src/ble_service.c:427-451`

```c
void ble_service_update_deadman(void)
{
    /* Only check deadman in ARMED state */
    if (sys_state.state != STATE_ARMED) {
        return;
    }
    
    uint16_t age_ms = ble_service_get_last_cmd_age_ms();
    
    if (age_ms >= DEADMAN_TIMEOUT_MS) {
        LOG_WRN("Deadman timeout: %u ms >= %u ms", age_ms, DEADMAN_TIMEOUT_MS);
        sys_state.state = STATE_FAULT;
        sys_state.error_code = ERR_DEADMAN_TIMEOUT;
        
        /* Stop PWM output */
        if (pwm_dev && device_is_ready(pwm_dev)) {
            int ret = pwm_set(pwm_dev, SERVO_CHANNEL_0, SERVO_PWM_PERIOD_NS, 0, 0);
            if (ret != 0) {
                LOG_ERR("Failed to stop servo output (err %d)", ret);
            } else {
                LOG_INF("Servo output stopped due to deadman timeout");
            }
        }
    }
}
```

**動作**:
- ARMED状態でのみ監視（DISARMED/FAULTでは無視）
- 200ms経過でFAULT状態に遷移
- PWM出力を即座に停止
- `error_code = ERR_DEADMAN_TIMEOUT` を設定

### 4.3 呼び出し元

**ファイル**: `firmware/zephyr/app/legctrl_fw/src/main.c:42-51`

```c
static void telemetry_timer_handler(struct k_timer *timer)
{
    ARG_UNUSED(timer);
    
    /* Update deadman timer */
    ble_service_update_deadman();
    
    /* Send telemetry */
    ble_service_send_telemetry();
}
```

**周期**: 100ミリ秒（10 Hz）

**ポイント**:
- Telemetryタイマーと同期してデッドマンチェック
- 100ms周期でチェックするため、最大遅延は100ms（200ms + 最大100ms = 最悪300ms）
- 実際には、200ms到達後の次回チェック（100msまたは200ms時点）で検出される

### 4.4 タイムスタンプ更新

**ファイル**: `firmware/zephyr/app/legctrl_fw/src/ble_service.c`

以下のコマンドで `last_cmd_timestamp` が更新されます：

1. **ARM** (Line 275):
   ```c
   sys_state.last_cmd_timestamp = k_uptime_get();
   ```

2. **SET_SERVO_CH0** (Line 316):
   ```c
   sys_state.last_cmd_timestamp = k_uptime_get();
   ```

3. **PING** (Line 350):
   ```c
   sys_state.last_cmd_timestamp = k_uptime_get();
   ```

**重要**: DISARMED/FAULT状態ではタイムスタンプは更新されない（ARMED状態のみ）

---

## 5. BLE切断時の即時停止

### 5.1 切断検出とハンドリング

**ファイル**: `firmware/zephyr/app/legctrl_fw/src/ble_service.c:112-133`

```c
static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    LOG_INF("BLE Disconnected (reason %u)", reason);
    
    if (sys_state.conn) {
        bt_conn_unref(sys_state.conn);
        sys_state.conn = NULL;
    }
    
    /* Go to FAULT state on disconnect and stop PWM output */
    sys_state.state = STATE_FAULT;
    sys_state.error_code = ERR_NONE;
    sys_state.telemetry_enabled = false;
    
    /* Stop PWM output to ensure safe shutdown on disconnect */
    if (pwm_dev && device_is_ready(pwm_dev)) {
        int ret = pwm_set(pwm_dev, SERVO_CHANNEL_0, SERVO_PWM_PERIOD_NS, 0, 0);
        if (ret != 0) {
            LOG_ERR("Failed to stop PWM on disconnect (err %d)", ret);
        }
    }
}
```

**動作**:
- BLE切断イベントが発生すると即座に呼び出される
- FAULT状態に遷移（現在の状態に関わらず）
- PWM出力を即座に停止（同期処理、遅延なし）
- `error_code = ERR_NONE`（切断自体はエラーではない）

**重要な安全機能**:
- デッドマンタイマーを待たず**即座に**停止
- BLEスタックのコールバックなので遅延が最小
- 切断理由（reason）はログに記録

### 5.2 再接続時の初期化

**ファイル**: `firmware/zephyr/app/legctrl_fw/src/ble_service.c:96-110`

```c
static void connected(struct bt_conn *conn, uint8_t err)
{
    if (err) {
        LOG_ERR("Connection failed (err %u)", err);
        return;
    }

    LOG_INF("BLE Connected");
    sys_state.conn = bt_conn_ref(conn);
    
    /* Reset to DISARMED on new connection */
    sys_state.state = STATE_DISARMED;
    sys_state.error_code = ERR_NONE;
    sys_state.last_cmd_timestamp = 0;
}
```

**動作**:
- 再接続時、自動的にDISARMED状態に戻る
- エラーコードをクリア
- デッドマンタイマーをリセット（`timestamp = 0`）

**安全性**:
- 再接続後、必ずARMコマンドが必要
- 前回の状態（ARMED/FAULT）を引き継がない

---

## 6. Telemetry実装

### 6.1 Telemetry送信

**ファイル**: `firmware/zephyr/app/legctrl_fw/src/ble_service.c:354-395`

```c
int ble_service_send_telemetry(void)
{
    if (!sys_state.telemetry_enabled || !sys_state.conn) {
        return -ENOTCONN;
    }
    
    struct telemetry_msg msg;
    
    /* Fill header */
    msg.hdr.version = 0x01;
    msg.hdr.msg_type = MSG_TYPE_TELEMETRY;
    msg.hdr.payload_len = sizeof(struct telemetry_payload);
    msg.hdr.reserved = 0x00;
    
    /* Fill payload with explicit little-endian encoding */
    msg.payload.state = sys_state.state;
    msg.payload.error_code = sys_state.error_code;
    
    uint16_t last_cmd_age = ble_service_get_last_cmd_age_ms();
    msg.payload.last_cmd_age_ms = last_cmd_age;
    
    /* TODO: Replace stub battery voltage with actual ADC reading */
    uint16_t battery = 7400;  /* Stub value: 7.4V */
    msg.payload.battery_mv = battery;
    
    msg.payload.reserved = 0;
    
    /* Send notification */
    int ret = bt_gatt_notify(sys_state.conn, &legctrl_svc.attrs[TELEMETRY_ATTR_IDX], 
                             &msg, sizeof(msg));
    
    // ... error handling ...
}
```

**送信周期**: 10 Hz（100ミリ秒）

**ファイル**: `firmware/zephyr/app/legctrl_fw/src/main.c:106-108`

```c
k_timer_start(&telemetry_timer, K_MSEC(TELEMETRY_PERIOD_MS), 
              K_MSEC(TELEMETRY_PERIOD_MS));
```

### 6.2 Telemetry構造

**ファイル**: `firmware/zephyr/app/legctrl_fw/src/ble_service.h:48-61`

```c
/* Telemetry payload */
struct telemetry_payload {
    uint8_t state;               // 状態（DISARMED/ARMED/FAULT）
    uint8_t error_code;          // エラーコード
    uint16_t last_cmd_age_ms;    // 最終コマンドからの経過時間
    uint16_t battery_mv;         // バッテリー電圧（mV）
    uint16_t reserved;           // 予約
} __packed;

/* Full telemetry message */
struct telemetry_msg {
    struct protocol_header hdr;
    struct telemetry_payload payload;
} __packed;
```

**合計サイズ**: 12 bytes（ヘッダ4 + ペイロード8）

---

## 7. エラーコード定義

**ファイル**: `firmware/zephyr/app/legctrl_fw/src/ble_service.h:27-33`

```c
/* Error codes */
#define ERR_NONE              0x00
#define ERR_DEADMAN_TIMEOUT   0x01
#define ERR_LOW_BATTERY       0x02
#define ERR_I2C_FAULT         0x03
#define ERR_INVALID_CMD       0x04
#define ERR_UNKNOWN           0xFF
```

**使用箇所**:
- `ERR_DEADMAN_TIMEOUT`: デッドマンタイムアウト時（Line 439）
- `ERR_I2C_FAULT`: PWM設定失敗時（Line 336）
- `ERR_INVALID_CMD`: 未知のコマンド受信時（Line 259）

---

## 8. 安全性のポイント

### 8.1 多層防御（Defense in Depth）

1. **起動時安全**: 必ずDISARMED状態で起動
2. **明示的ARM**: ユーザーが明示的にARMするまでサーボ不動作
3. **デッドマン**: 200ms無通信で自動停止
4. **切断即時停止**: BLE切断時に遅延なく停止
5. **DISARM常時可**: どの状態からでもDISARM可能

### 8.2 フェイルセーフ設計

- **デフォルト安全**: 初期状態、エラー時、切断時はすべてDISARMED/FAULT（安全側）
- **PWM即時停止**: DISARM/FAULT/切断時に即座にPWM出力を停止（`pulse_width = 0`）
- **状態遷移制約**: FAULT状態からはDISARM経由でのみ復帰可能

### 8.3 可観測性（Observability）

- **ログ**: すべての状態遷移、コマンド、エラーがシリアルログに記録
- **Telemetry**: 10 Hzで状態、エラー、経過時間を通知
- **デバッグ容易**: ログとTelemetryの組み合わせで問題再現可能

---

## 9. テスト検証

詳細なテスト手順は `docs/safety_test.md` を参照してください。

### 9.1 主要テストケース

- TC-01: 起動時DISARMED確認
- TC-02: ARM前コマンド無視確認
- TC-03: ARM/DISARM遷移確認
- TC-04: ARMED状態サーボ制御確認
- TC-05: デッドマンタイムアウト確認（200ms）
- TC-06: FAULT復帰確認
- TC-07: BLE切断即時停止確認
- TC-08: BLE再接続初期化確認
- TC-09: PINGでデッドマン回避確認
- TC-10: Telemetry周期確認（10 Hz）
- TC-11: エラーコードとログ対応確認

**すべてのテストケースが実装により合格できることを確認済み**

---

## 10. 依存関係

### 10.1 S1-06（受信経路）

このイシューは S1-06 に依存していますが、S1-06 は既に完了しています：
- BLE GATT Service/Characteristic実装済み
- Command受信経路（Write callback）実装済み
- Telemetry送信経路（Notify）実装済み

### 10.2 外部依存

- Zephyr RTOS（タイマー、BLEスタック）
- PCA9685 PWMドライバ（I2C経由）
- nRF52840 BLEスタック

---

## 11. 今後の拡張（Out of Scope）

以下は本イシューの範囲外です（将来の拡張候補）：

- 複数チャンネル対応時の安全機能
- バッテリー電圧監視（ERR_LOW_BATTERY実装）
- Watchdog timer統合
- ハードウェア非常停止スイッチ
- 設定可能なデッドマンタイムアウト値

---

## 12. 参照

- **プロトコル仕様**: `shared/protocol/spec/legctrl_protocol.md`
- **BLE詳細**: `docs/ble_protocol.md`
- **テスト計画**: `docs/safety_test.md`
- **ソースコード**: `firmware/zephyr/app/legctrl_fw/src/`

---

## 改訂履歴

| Version | Date       | Author   | Changes                              |
|---------|------------|----------|--------------------------------------|
| v1.0    | 2026-01-03 | copilot  | 初版作成（安全システム実装詳細）     |
