# ChessEngine Engineering Approach

This project should optimize for durable playing strength, not isolated benchmark wins.

## Decision Rules

- Do not tune from a single failed position unless it represents a repeated pattern.
- Prefer diagnostics before changes when the failure mode is ambiguous.
- Keep tactical strength as a hard guardrail. Any change that improves postmortem avoid labels but regresses the 250-position tactical suite needs a stronger justification or should be reverted.
- Treat Stockfish labels as evidence, not truth. Use deeper confirmation for avoid labels and avoid exact imitation when the position is noisy.
- Distinguish failure classes before implementing:
  - If the engine prefers the reference move when constrained, investigate search depth, move ordering, pruning, or extensions.
  - If the engine prefers the played move when constrained, investigate evaluation or horizon effects.
  - If SEE says the played capture is negative and the engine still prefers it, investigate compensation evaluation before adding blunt SEE penalties.
  - If SEE is wrong, fix SEE or move generation first.
- Avoid broad penalties that punish real sacrifices. A root or eval penalty must survive tactical-suite comparison and confirmed postmortem avoid checks.
- Make new heuristics explainable through `eval`, `see`, benchmark output, or postmortem fields before using them for tuning.

## Validation Ladder

1. Unit and smoke tests.
2. Focused postmortem suite from confirmed avoid labels.
3. 250-position tactical suite with no solved-count regression and no unjustified best-move churn.
4. Full postmortem regeneration.
5. Gauntlet only after the change survives the cheaper gates.

## Current Diagnostic Focus

The current postmortem flow records:

- Engine eval trace before and after moves.
- Engine constrained score for the played move.
- Engine constrained score for the reference move.
- Engine SEE for both candidate moves.
- Summary buckets for engine/reference preference and negative-SEE captures.

The active high-level question is not "how do we stop all losing captures?" It is:

"When SEE already says a capture loses material, why does deeper search/evaluation still think the compensation is enough?"

Answer that question with broad evidence before adding a new engine heuristic.
