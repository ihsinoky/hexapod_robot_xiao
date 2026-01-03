# Copilot Review Instructions (Repo)

These instructions guide GitHub Copilot code review in this repository. Keep reviews actionable and sprint-focused.

## Language
- In code reviews, write all explanations in Japanese.
- Keep code identifiers (class/function/variable names) as-is; do not translate them.

## Always include severity
Prefix every finding with exactly one label:
- 【致命的】 must fix before merge (security, data loss, incorrect behavior likely)
- 【高】 should fix before merge
- 【中】 mergeable, but likely to cause near-term bugs or maintenance cost
- 【低】 optional improvement
- 【提案】 idea / question / follow-up ticket (especially if outside the Sprint Goal)

## Sprint Goal first
- Read the PR description and linked issue(s) to find the Sprint Goal.
- Start the review with: `スプリントゴール: ...` (1–2 sentences).
- If not stated, write: `スプリントゴール: （PR本文に明記なし。目的を推測: ...）`
- For every finding, explicitly state the impact on the Sprint Goal (blocks / risks / neutral / out of scope).

## Responsibilities (class / component boundaries)
- Check whether each class/component has a clear responsibility (explain in 1 sentence).
- Flag mixing of concerns (UI, state, I/O such as API/DB, business rules) in one place.
- If the fix is large, provide:
  1) the smallest safe change that fits this sprint, and
  2) a separate 【提案】 for a follow-up refactor ticket.

## Quick review checks (use as needed)
- Correctness & edge cases (null/empty, error handling, concurrency where relevant)
- Security (authn/authz, input validation, avoid logging secrets or personal data)
- Tests (add/update tests for new logic; failing tests are 【高】 or 【致命的】)

## Review format (mandatory)
Use this structure:

### サマリ
- スプリントゴール: ...
- 主要リスク: ...
- マージ判断: ...（例: 「【致命的】が解消されれば可」）

### 指摘（重要度順・最大5件）
- 【重要度】[種類] 対象: <file> / <symbol>
  - 事実: ...
  - 影響（ゴール）: ...
  - 提案（最小修正）: ...
  - （任意）次の改善案: ...（【提案】にする）

## Noise control
- Prefer high-signal issues over style nits.
- Do not request large rewrites unless necessary for correctness/security or the Sprint Goal.
- If uncertain, say what to verify instead of asserting.