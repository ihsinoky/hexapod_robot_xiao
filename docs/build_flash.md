# Zephyr ファームウェア ビルド & フラッシュ手順

## 概要
このドキュメントでは、XIAO nRF52840向けZephyrファームウェアのビルドとフラッシュ手順を説明します。

---

## 前提条件

### 必須ソフトウェア
- Python 3.8以上
- Git
- Zephyr SDK（推奨バージョン: 0.16.x以上）
- west（Zephyrメタツール）

### XIAO nRF52840について
- **Seeed Studio製 XIAO nRF52840**は、Nordic nRF52840チップを搭載した小型開発ボード
- Zephyrでは `xiao_nrf52840` または `seeed_xiao_nrf52840` ボード名でサポート
- USB経由でのフラッシュ（UF2ブートローダー）に対応

---

## 環境構築

### 1. Python環境の準備
```bash
# Python 3がインストールされているか確認
python3 --version

# 仮想環境を作成（推奨）
python3 -m venv ~/zephyrproject/.venv
source ~/zephyrproject/.venv/bin/activate  # Linux/macOS
# または
~/zephyrproject/.venv/Scripts/activate  # Windows
```

### 2. westのインストール
```bash
pip install west
```

### 3. Zephyrワークスペースの初期化
```bash
# 作業用ディレクトリを作成
mkdir ~/zephyrproject
cd ~/zephyrproject

# Zephyrリポジトリを取得（最新安定版）
west init -m https://github.com/zephyrproject-rtos/zephyr --mr v3.6-branch
west update

# Zephyr依存パッケージをインストール
pip install -r ~/zephyrproject/zephyr/scripts/requirements.txt
```

> **注意**: `v3.6-branch` は例です。最新の安定版ブランチまたはタグを使用してください。

### 4. Zephyr SDKのインストール
```bash
# Linux/macOSの場合（バージョン 0.16.5-1 の例）
cd ~
wget https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.16.5/zephyr-sdk-0.16.5_linux-x86_64.tar.xz
tar xvf zephyr-sdk-0.16.5_linux-x86_64.tar.xz
cd zephyr-sdk-0.16.5
./setup.sh
```

Windowsの場合はインストーラーをダウンロードして実行してください。  
詳細: https://docs.zephyrproject.org/latest/develop/toolchains/zephyr_sdk.html

### 5. 環境変数の設定
```bash
# Zephyr環境変数を設定（セッションごとに実行）
export ZEPHYR_BASE=~/zephyrproject/zephyr
source ~/zephyrproject/zephyr/zephyr-env.sh

# または、~/.bashrc や ~/.zshrc に追加して永続化
echo "export ZEPHYR_BASE=~/zephyrproject/zephyr" >> ~/.bashrc
echo "source ~/zephyrproject/zephyr/zephyr-env.sh" >> ~/.bashrc
```

---

## ビルド手順

### ボード名の確認
XIAO nRF52840は、Zephyrで以下のボード名でサポートされています：

- **`xiao_nrf52840`** （Zephyr 3.5以降）
- **`seeed_xiao_nrf52840`** （古いバージョン）

使用しているZephyrバージョンに応じて、適切なボード名を確認してください。

```bash
# ボードがサポートされているか確認
west boards | grep xiao
```

### ビルドコマンド
```bash
# Zephyr環境を読み込む
cd ~/zephyrproject
source zephyr/zephyr-env.sh

# ファームウェアディレクトリへ移動（このリポジトリのパス）
cd /path/to/hexapod_robot_xiao/firmware/zephyr/app/legctrl_fw

# ビルド実行
west build -b xiao_nrf52840 .
```

**ビルドオプション**:
- `-b <board_name>`: ターゲットボードを指定（必須）
- `-p auto`: ビルドディレクトリをクリーンアップ（推奨）
- `--pristine`: 完全クリーンビルド

**例**:
```bash
# クリーンビルド
west build -p auto -b xiao_nrf52840 .
```

### ビルド成果物
ビルドが成功すると、以下のファイルが生成されます：
```
build/zephyr/
├── zephyr.elf        # ELFバイナリ
├── zephyr.hex        # HEXファイル
├── zephyr.bin        # バイナリファイル
└── zephyr.uf2        # UF2ファイル（XIAO nRF52840用）
```

---

## フラッシュ手順

### 方法1: UF2ブートローダー経由（推奨）

XIAO nRF52840は、UF2ブートローダーを搭載しているため、ドラッグ&ドロップでフラッシュできます。

#### 手順:
1. **ブートローダーモードに入る**
   - XIAOボード上の**RSTボタンを2回素早くダブルクリック**
   - または、XIAOのRSTピンをGNDに2回素早く接続
   - **LEDが緑色に点灯**し、USBドライブとしてマウントされます（`XIAO-SENSE`または`XIAO-NRF52`という名前）

2. **UF2ファイルをコピー**
   ```bash
   # Linux/macOS
   cp build/zephyr/zephyr.uf2 /media/<username>/XIAO-SENSE/

   # macOS
   cp build/zephyr/zephyr.uf2 /Volumes/XIAO-SENSE/

   # Windows
   # エクスプローラーで build\zephyr\zephyr.uf2 をドライブにドラッグ&ドロップ
   ```

3. **自動的に再起動**
   - コピーが完了すると、XIAOが自動的に再起動し、ファームウェアが実行されます

#### westコマンドでのフラッシュ（UF2）
```bash
# westを使用してフラッシュ（ボードがブートローダーモードの場合）
west flash --runner uf2
```

> **注意**: `west flash` がUF2デバイスを検出できない場合は、手動でコピーしてください。

---

### 方法2: J-Link経由（オプション）

外部J-Linkデバッガーを使用する場合の手順です。

#### 必要なもの:
- J-Link EDU / J-Link BASE / Segger J-Link互換デバッガー
- SWD接続（SWDIO, SWCLK, GND, VTref）

#### フラッシュコマンド:
```bash
west flash --runner jlink
```

#### デバッグコマンド:
```bash
west debug --runner jlink
```

---

### 方法3: nrfjprog経由（Nordic公式ツール）

Nordic Semiconductor公式のnrfjprogツールを使用する場合。

#### 必要なもの:
- nRF Command Line Tools（nrfjprog含む）
- J-Link接続（方法2と同様）

#### インストール:
https://www.nordicsemi.com/Products/Development-tools/nrf-command-line-tools

#### フラッシュコマンド:
```bash
west flash --runner nrfjprog
```

---

## 動作確認

### 1. シリアルコンソールの接続
XIAOをUSBで接続すると、シリアルポートとして認識されます。

#### ポートの確認:
```bash
# Linux
ls /dev/ttyACM*  # 通常は /dev/ttyACM0

# macOS
ls /dev/tty.usb*  # /dev/tty.usbmodem* など

# Windows
# デバイスマネージャーでCOMポートを確認（COM3, COM4など）
```

#### シリアル接続（115200 bps）:
```bash
# Linux/macOS（screenを使用）
screen /dev/ttyACM0 115200

# Linux（minicomを使用）
minicom -D /dev/ttyACM0 -b 115200

# PuTTY（Windows/Linux/macOS）
# 設定: シリアル、115200 bps、8N1

# または、west側のモニター機能
west attach
```

### 2. ログ出力の確認
正常に起動すると、以下のようなログが表示されます：

```
*** Booting Zephyr OS build v3.6.0 ***
===========================================
LegCtrl Firmware - XIAO nRF52840
===========================================
Zephyr version: 3.6.0
Board: xiao_nrf52840
System started successfully
===========================================
Heartbeat: System running
Heartbeat: System running
...
```

**確認項目**:
- [ ] 起動メッセージが表示される
- [ ] Zephyrバージョンとボード名が正しい
- [ ] 5秒ごとにハートビートメッセージが表示される

---

## トラブルシューティング

### ビルドエラー: `Board 'xiao_nrf52840' not found`
**原因**: 使用しているZephyrバージョンでボード名が異なる  
**解決策**:
```bash
# サポートされているボード名を確認
west boards | grep xiao

# 見つかったボード名でビルド
west build -b seeed_xiao_nrf52840 .
```

### フラッシュエラー: `No UF2 device found`
**原因**: XIAOがブートローダーモードになっていない  
**解決策**:
1. RSTボタンを2回素早くダブルクリック
2. 緑色LEDが点灯することを確認
3. USBドライブがマウントされることを確認（`XIAO-SENSE`など）
4. 再度 `west flash --runner uf2` または手動コピー

### シリアルコンソールに何も表示されない
**原因1**: ボーレートが間違っている  
**解決策**: 115200 bpsに設定

**原因2**: UARTが有効化されていない  
**解決策**: `prj.conf` に以下が含まれているか確認
```
CONFIG_SERIAL=y
CONFIG_CONSOLE=y
CONFIG_UART_CONSOLE=y
```

**原因3**: 間違ったシリアルポートを開いている  
**解決策**: `ls /dev/ttyACM*` または `ls /dev/tty.usb*` でポートを再確認

### ビルド時に `ZEPHYR_BASE` エラー
**原因**: 環境変数が設定されていない  
**解決策**:
```bash
export ZEPHYR_BASE=~/zephyrproject/zephyr
source ~/zephyrproject/zephyr/zephyr-env.sh
```

---

## 補足: ワークスペース構成オプション

### オプションA: 別ワークスペース（推奨）
Zephyr本体を `~/zephyrproject/` に配置し、このリポジトリは別ディレクトリで管理。

**メリット**: リポジトリが肥大化しない、Zephyr更新が容易  
**デメリット**: ビルド時にアプリケーションパスを指定する必要がある

### オプションB: west.yml を使用（モジュール化）
このリポジトリ内に `west.yml` を配置し、westワークスペースとして管理。

**例**: `firmware/zephyr/west.yml`
```yaml
manifest:
  remotes:
    - name: zephyrproject-rtos
      url-base: https://github.com/zephyrproject-rtos
  projects:
    - name: zephyr
      remote: zephyrproject-rtos
      revision: v3.6-branch
      import: true
  self:
    path: firmware/zephyr
```

**メリット**: 依存関係が明確、再現性が高い  
**デメリット**: 初期設定がやや複雑

詳細は以下を参照:  
https://docs.zephyrproject.org/latest/develop/west/workspaces.html

---

## 参照
- Zephyr公式ドキュメント: https://docs.zephyrproject.org/
- XIAO nRF52840 Wiki: https://wiki.seeedstudio.com/XIAO_BLE/
- Zephyr Getting Started: https://docs.zephyrproject.org/latest/develop/getting_started/index.html

---

## 改訂履歴

| Version | Date       | Author   | Changes                        |
|---------|------------|----------|--------------------------------|
| v1.0    | 2025-12-31 | copilot  | 初版作成（ビルド・フラッシュ手順）|
