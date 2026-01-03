# LegCtrlApp - Quick Start Guide

## 概要
iPad用の最小限BLEコントロールアプリ。XIAO nRF52840ファームウェアと通信してサーボを制御します。

## アーキテクチャ

```
┌─────────────────────────────────────┐
│         ContentView (SwiftUI)       │
│  - 接続UI                           │
│  - ARM/DISARM ボタン                │
│  - サーボコントロール                │
│  - テレメトリ表示                    │
└─────────────┬───────────────────────┘
              │
              ▼
┌─────────────────────────────────────┐
│         BLEManager                  │
│  - CoreBluetooth管理                │
│  - スキャン/接続/切断               │
│  - コマンド送信                     │
│  - テレメトリ受信                   │
│  - 周期送信タイマー                 │
└─────────────┬───────────────────────┘
              │
              ▼
┌─────────────────────────────────────┐
│       ProtocolTypes                 │
│  - メッセージエンコード/デコード     │
│  - UUID定義                         │
│  - 状態/エラー定義                  │
└─────────────────────────────────────┘
              │
              ▼ BLE (CoreBluetooth)
┌─────────────────────────────────────┐
│    XIAO nRF52840 Firmware           │
│  - BLE GATT Server                  │
│  - コマンド処理                     │
│  - サーボ駆動                       │
│  - テレメトリ送信 (10Hz)            │
└─────────────────────────────────────┘
```

## ファイル構成

```
LegCtrlApp/
├── LegCtrlApp.xcodeproj/
│   └── project.pbxproj          # Xcodeプロジェクト設定
├── LegCtrlApp/
│   ├── LegCtrlAppApp.swift      # アプリエントリーポイント
│   ├── ContentView.swift        # メインUI (SwiftUI)
│   ├── BLEManager.swift         # BLE通信管理
│   ├── ProtocolTypes.swift      # プロトコル定義
│   ├── Info.plist               # アプリ設定
│   └── Assets.xcassets/         # アセット（アイコン等）
└── .gitignore                    # Git除外設定
```

## 主要クラス

### 1. ProtocolTypes.swift
プロトコルv0.1の定義:
- `ProtocolConstants`: UUID、バージョン定義
- `MessageType`: コマンド/テレメトリタイプ
- `SystemState`: DISARMED/ARMED/FAULT
- `ErrorCode`: エラーコード定義
- `CommandMessage`: コマンドメッセージビルダー
- `TelemetryData`: テレメトリパーサー

**重要**: Little-endianエンコーディング

### 2. BLEManager.swift
CoreBluetoothラッパー:
- `startScanning()`: BLEデバイススキャン
- `connect(to:)`: デバイスに接続
- `disconnect()`: 切断
- `arm()`, `disarm()`: システム制御
- `setServoCh0(pulseUs:)`: サーボ指令送信
- `ping()`: キープアライブ
- `startPeriodicSending(pulseUs:)`: 50Hz周期送信開始
- `stopPeriodicSending()`: 周期送信停止

**デッドマン対策**: 周期送信で200msタイムアウトを回避

### 3. ContentView.swift
SwiftUI UI:
- 接続セクション: スキャン/接続/切断
- コントロールセクション: ARM/DISARM/サーボ制御
- テレメトリセクション: 状態/エラー/電圧表示
- ログセクション: 操作履歴表示

## プロトコル実装詳細

### コマンドフォーマット

#### ARM (0x01)
```
[01 01 00 00]
 │  │  │  └─ reserved
 │  │  └──── payload_len (0)
 │  └─────── msg_type (0x01)
 └────────── version (0x01)
```

#### DISARM (0x02)
```
[01 02 00 00]
```

#### SET_SERVO_CH0 (0x03)
```
[01 03 02 00 DC 05]
 │  │  │  │  └───┴─ pulse_us (Little-endian, DC 05 = 1500)
 │  │  │  └──────── reserved
 │  │  └─────────── payload_len (2)
 │  └────────────── msg_type (0x03)
 └───────────────── version (0x01)
```

#### PING (0x04)
```
[01 04 00 00]
```

### テレメトリフォーマット (0x10)

```
[01 10 08 00 | 01 00 05 00 | E8 1C 00 00]
 └─ Header ──┘ └─ Payload ─────────────┘
 
Header (4 bytes):
  01: version
  10: msg_type (TELEMETRY)
  08: payload_len
  00: reserved

Payload (8 bytes):
  01: state (0x01 = ARMED)
  00: error_code (0x00 = None)
  05 00: last_cmd_age_ms (Little-endian, 5ms)
  E8 1C: battery_mv (Little-endian, 0x1CE8 = 7400mV = 7.4V)
  00 00: reserved
```

## 使用シナリオ

### シナリオ1: 基本操作
1. アプリ起動
2. "Scan" → デバイス選択 → 接続
3. "ARM" → State: ARMED確認
4. スライダー調整 + "Send Once"
5. サーボ動作確認
6. "DISARM" → 停止

### シナリオ2: 周期送信
1. 接続 → ARM
2. "Periodic (50Hz)" ON
3. スライダーでリアルタイム制御
4. デッドマン回避自動実行
5. "DISARM" または Periodic OFF で停止

### シナリオ3: デバッグ
1. 接続後、ログを確認
2. "Send PING" で通信確認
3. テレメトリで `last_cmd_age_ms` 監視
4. 200ms超えでFAULT遷移を確認

## トラブルシューティング

### ビルドエラー
```
"Signing for 'LegCtrlApp' requires a development team"
```
→ Xcode > Signing & Capabilities > Team を選択

### 実機接続エラー
```
"This app does not have permission to use Bluetooth"
```
→ Info.plist に `NSBluetoothAlwaysUsageDescription` が必要（既に設定済み）

### BLE接続できない
- ファームウェアが起動しているか確認
- サービスUUID一致確認
- 他デバイスとの競合確認

### アプリをバックグラウンドにすると停止する
- **仕様**: iOSはバックグラウンドアプリのタイマーを一時停止します
- **結果**: 周期送信が停止し、200ms後にデッドマンタイムアウトが発動
- **安全性**: ロボットは自動的にFAULT状態に遷移し停止します
- **対策**: 
  - 操作中はアプリを前面に保つ
  - iPadの自動ロック設定を無効化
  - Settings > Display & Brightness > Auto-Lock > Never

## 次のステップ

- [ ] 実機テストで動作確認
- [ ] 複数チャンネル対応（16ch以上）
- [ ] キャリブレーション機能
- [ ] ログエクスポート
- [ ] エラーハンドリング強化

## 参考資料

- `docs/ble_protocol.md` - GATT/UUID詳細
- `shared/protocol/spec/legctrl_protocol.md` - プロトコル仕様
- `firmware/zephyr/app/legctrl_fw/` - ファームウェア実装
