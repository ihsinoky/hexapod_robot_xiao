# AGENTS.md

## Goal
- Prefer small PRs (one issue = one PR).
- Follow existing patterns; do not invent new architecture without discussion.

## How to run
- Install: <fill in>
- Dev: <fill in>
- Test: <fill in>
- Lint/Format: <fill in>

## Definition of Done
- Tests pass.
- Error cases are handled (at least: auth, empty data).
- Update docs if behavior changes.

## Safety / non-goals
- Do not change authentication flow without explicit instruction.
- Avoid adding new dependencies unless necessary.

## Zephyr firmware (firmware/zephyr)
- 変更後は必ず以下でビルド検証する:
  west build -p always -b xiao_ble -s firmware/zephyr/app/legctrl_fw
- Devicetree のエラー/警告は必ず解消する（overlay名、bindings、#pwm-cells等）
- CIやワークフローの変更は要求がない限り行わない
- 修正は最小差分で行う

## PRレビュー / コードレビュー（Copilot coding agent 向け）

- コメントは日本語で記述すること

### 0) レビューの前提（一次ソース）
- スプリントの仕様・用語定義・決定事項は `docs/sprints/SPRINT_XX.md` を一次ソースとして参照すること。
  - Issue/PR本文と矛盾がある場合は、Docs を正として **Issue/PR本文（説明）を更新**し、実装・レビューも Docs 準拠で進めること。
- 「どのIssue/スプリントのゴールに紐づくPRか」を最初に特定してからレビューすること（スコープ逸脱を防ぐ）。

### 1) コンポーネントの責務を踏まえてレビューすること（必須）
- 変更対象が属するレイヤ/責務を明確にしてからレビューすること。
- レビュー観点（最低限）:
  - 変更が「そのコンポーネントであるべき理由」があるか（責務の逸脱がないか）
  - 境界を跨ぐ変更が必要以上に広がっていないか
  - 既存の依存方向を壊していないか
  - “ついで改修”で責務が混線していないか（必要ならバックログへ）

### 2) スプリントのゴールを踏まえてレビューすること（必須）
- そのスプリントのゴール/AC（受け入れ条件）に照らして、以下を必ずコメントすること:
  - 何が「ゴール達成」に寄与しているか
  - ゴール/AC に対して不足している点は何か
  - 逆に、スプリントスコープ外の変更が混入していないか（混入している場合は分離を提案）
- “良いが今やらない”改善案は、原則として **バックログ化**してスプリントのスコープを守る。

### 3) バックログを追加する場合は docs/BACKLOG.md に追記すること（必須）
- レビューの結果、新規の改善タスク/追加要件/技術負債返済が必要だと判断した場合:
  - **実装に混ぜない**（スプリントゴールを毀損しやすい）
  - `docs/BACKLOG.md` に追記すること（既存フォーマットがある場合はそれに従う）
- 追記内容の最低要件:
  - 目的（なぜ必要か）
  - 期待効果（ユーザー価値 or 開発効率/品質）
  - 完了条件（ざっくりで良い）
  - 参照（関連Issue/PR番号、該当コード領域）

### 4) レビューコメントの型（推奨テンプレ）
- Summary: 変更の要点（1〜3行）
- Sprint alignment: ゴール/ACとの整合（OK/NGと根拠）
- Responsibility check: コンポーネント責務の観点（OK/懸念点）
- Risks & tests: リスク、テスト/動作確認観点、再現手順
- Backlog: スコープ外だが価値がある提案（ある場合のみ。追記済みならその旨も記載）