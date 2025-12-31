# バックログ

## 運用ルール
- 優先度: P0（最優先）/ P1（次）/ P2（後）
- 完了定義（DoD）:
  - 実装 + 最小限の動作確認 + docs更新（必要な場合）
  - 仕様変更がある場合は `shared/protocol/spec` を先に更新し、FW/iOSを追従させる

---

## P0（MVPに必須）

### Repo / Docs
- [ ] P0-01: `docs/architecture.md` を作成（現行: iPad→BLE→XIAO→PCA9685）
- [ ] P0-02: `docs/wiring_pca9685.md` を作成（電源/GND/デカップリング/注意点）
- [ ] P0-03: `docs/test_plan.md` を作成（Bring-up手順、受入基準）

### Shared Protocol
- [ ] P0-10: `shared/protocol/spec/legctrl_protocol.md` を最小仕様で確定（M1用）
- [ ] P0-11: FW/iOSで共通に参照する “用語” と “単位系” を明記（us/deg/s 等）

### Firmware（Zephyr / XIAO nRF52840）
- [ ] P0-20: PCA9685のI2C疎通とPWM 50Hz固定出力（サーボ1ch）
- [ ] P0-21: BLE GATT（Write/Notify）の最小実装（手動Writeで動作）
- [ ] P0-22: Write callbackでI2Cを叩かず、キュー/ループで更新（ハング回避）
- [ ] P0-23: ARM/DISARMの状態機械（接続しただけで動かない）
- [ ] P0-24: デッドマン（例: 200ms無通信で停止）
- [ ] P0-25: 最小テレメトリ（電圧/状態/エラー）Notify

### iOS（iPad App）
- [ ] P0-30: スキャン/接続/切断/再接続の骨格（CoreBluetooth）
- [ ] P0-31: ARM/DISARM UI（誤操作しにくい導線）
- [ ] P0-32: 最小操作UI（ボタン or 簡易スティック）→ コマンド送信
- [ ] P0-33: 周期送信（30–60Hz）と “最新値のみ送る” 送信制御
- [ ] P0-34: テレメトリ表示（電圧/状態/エラー）

---

## P1（安定化・拡張）

### Firmware
- [ ] P1-20: 複数チャンネルまとめ更新（I2Cバッチ）で周期のジッタ低減
- [ ] P1-21: サーボ中心/リミット/反転のキャリブレーション仕組み（保持先の検討）
- [ ] P1-22: パラメータ設定Characteristic（Config）追加
- [ ] P1-23: エラーハンドリング設計（低電圧/過電流疑い/センサー異常等）

### iOS
- [ ] P1-30: 設定画面（中心/リミット/ゲイン等）と保存
- [ ] P1-31: ログ出力（CSV/JSON）と不具合再現性向上
- [ ] P1-32: UI操作のフェイルセーフ（送信停止・再接続時の安全遷移）

### Shared Protocol / Tooling
- [ ] P1-40: コマンド/テレメトリのバージョニング方針（ver/互換性）
- [ ] P1-41: テストベクタ（test-vectors）整備（FW/iOSで同一入力を検証）
- [ ] P1-42: 生成コード運用（gen_protocol.py）を導入するか判断（手書き維持でも可）

---

## P2（将来: Pi統合・高度化）

- [ ] P2-10: Pi統合アーキテクチャ検討（iPad↔Pi↔MCU の責務分担）
- [ ] P2-11: “転送路非依存” プロトコル設計（BLE/UART/TCPで同一payload）
- [ ] P2-12: WebRTC/低遅延映像の評価（Zero 2 W前提）
- [ ] P2-13: 走行/歩容生成（ゲイト/IK）をFWに載せるかPi側に寄せるかの判断

