# CI セットアップガイド

このドキュメントでは、GitHub Actions を使用した CI（継続的インテグレーション）の設定と運用方法について説明します。

## 概要

このリポジトリでは、以下の自動チェックを PR/Push 時に実行します：

1. **Markdown Lint**: すべての Markdown ファイルの品質チェック
2. **YAML Lint**: すべての YAML ファイルの品質チェック
3. **Zephyr Firmware Build**: `firmware/zephyr/app/legctrl_fw/` が存在する場合、xiao_ble ボード向けのビルド確認

## CI ワークフロー

### トリガー条件

CI は以下の場合に自動実行されます：

- `main` または `develop` ブランチへの Push
- `main` または `develop` ブランチへの Pull Request 作成/更新

### ジョブ構成

#### 1. Markdown Lint (`markdown-lint`)

- **ツール**: markdownlint-cli2
- **対象**: すべての `.md` ファイル
- **設定ファイル**: `.markdownlint-cli2.yaml`

#### 2. YAML Lint (`yaml-lint`)

- **ツール**: yamllint
- **対象**: すべての `.yml` および `.yaml` ファイル
- **設定ファイル**: `.yamllint.yaml`

#### 3. Zephyr Firmware Build (`zephyr-build`)

- **ボード**: xiao_ble (XIAO nRF52840)
- **対象**: `firmware/zephyr/app/legctrl_fw/`
- **ビルドコマンド**: `west build -b xiao_ble app/legctrl_fw`
- **条件**: ファームウェアディレクトリが存在する場合のみ実行

ビルド成果物（`zephyr.*` ファイル）は GitHub Actions のアーティファクトとして 7 日間保存されます。

## ローカルでの実行方法

CI で失敗する前に、ローカル環境で事前チェックを行うことを推奨します。

### Markdown Lint

```bash
# インストール（初回のみ）
npm install -g markdownlint-cli2

# 実行
markdownlint-cli2 "**/*.md"
```

### YAML Lint

```bash
# インストール（初回のみ）
pip install yamllint

# 実行
yamllint .
```

### Zephyr Firmware Build

ファームウェアのビルド手順については、[build_flash.md](./build_flash.md) を参照してください。

## Branch Protection の設定

CI を `main` ブランチへのマージ条件として強制するには、GitHub のリポジトリ設定で Branch Protection を有効化します。

### 設定手順

1. GitHub リポジトリページで **Settings** → **Branches** に移動
2. **Branch protection rules** セクションで **Add rule** をクリック
3. 以下の設定を行います：

   **Branch name pattern**: `main`

   **Protect matching branches** セクションで以下をチェック：
   - ✅ **Require a pull request before merging**
     - 推奨: "Require approvals" を 1 以上に設定
   - ✅ **Require status checks to pass before merging**
     - 検索ボックスで以下のステータスチェックを追加：
       - `Markdown Lint`
       - `YAML Lint`
       - `Zephyr Firmware Build` （ファームウェアが存在する場合）
   - ✅ **Require branches to be up to date before merging**
   - オプション: **Do not allow bypassing the above settings**

4. **Create** または **Save changes** をクリック

### 推奨設定

- **main**: 上記の Branch Protection を適用
- **develop**: 同様の設定を推奨（ただし承認なしで merge 可能にしても可）
- **feature ブランチ**: 制限なし

## Linter の設定カスタマイズ

### Markdown Lint

`.markdownlint-cli2.yaml` を編集して、ルールをカスタマイズできます。

主な設定項目：
- `MD013`: 行の長さ制限（デフォルト: 120 文字）
- `MD033`: インライン HTML の許可/禁止
- `MD041`: ファイル先頭の見出し要求

詳細: [markdownlint ルール一覧](https://github.com/DavidAnson/markdownlint/blob/main/doc/Rules.md)

### YAML Lint

`.yamllint.yaml` を編集して、ルールをカスタマイズできます。

主な設定項目：
- `line-length`: 行の長さ制限（デフォルト: 120 文字）
- `truthy`: 真偽値の形式（yes/no, true/false など）
- `indentation`: インデント幅（デフォルト: 2 スペース）

詳細: [yamllint ルール一覧](https://yamllint.readthedocs.io/en/stable/rules.html)

## トラブルシューティング

### CI が失敗する場合

1. **Markdown Lint エラー**
   - エラーメッセージを確認し、該当するルールを理解する
   - ローカルで `markdownlint-cli2 "**/*.md"` を実行して修正
   - ルールが厳しすぎる場合は `.markdownlint-cli2.yaml` でルールを調整

2. **YAML Lint エラー**
   - インデントやスペースの問題が多い
   - ローカルで `yamllint .` を実行して修正
   - 必要に応じて `.yamllint.yaml` でルールを調整

3. **Zephyr Build エラー**
   - ローカルでビルドを実行して問題を特定
   - 依存関係や設定ファイルの確認
   - [build_flash.md](./build_flash.md) の手順に従っているか確認

### CI の実行をスキップする

緊急の場合やドラフト PR の場合、コミットメッセージに `[skip ci]` を含めることで CI をスキップできます。

```bash
git commit -m "Draft: WIP changes [skip ci]"
```

**注意**: `main` ブランチへのマージ前には必ず CI を通すこと。

## 将来の拡張

このドキュメントは Sprint 1 時点の内容です。今後以下の追加を検討します：

- iOS アプリのビルド確認（Xcode Cloud または macOS runner）
- ユニットテストの追加
- コードカバレッジの測定
- 自動デプロイ（リリースビルド）

## 参照

- [GitHub Actions ドキュメント](https://docs.github.com/ja/actions)
- [markdownlint-cli2](https://github.com/DavidAnson/markdownlint-cli2)
- [yamllint](https://yamllint.readthedocs.io/)
- [Zephyr Getting Started](https://docs.zephyrproject.org/latest/getting_started/index.html)
