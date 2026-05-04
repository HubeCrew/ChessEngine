# Gauntlet Postmortem

## Summary

- `engine`: current-eval
- `games_seen`: 100
- `engine_moves_analyzed`: 4779
- `flagged_events`: 120
- `loss_games_seen`: 41
- `average_delta_cp`: 98.69
- `worst_delta_cp`: -303
- `captures`: 1138
- `equal_value_captures`: 406
- `queen_trades`: 40
- `eval_swings`: 33
- `engine_reference_scores`: 120
- `engine_prefers_reference`: 23
- `engine_prefers_played`: 29
- `engine_ties_reference`: 68
- `engine_played_negative_see`: 32
- `engine_reference_negative_see`: 17
- `engine_prefers_negative_see`: 10
- `engine_can_avoid_negative_see`: 4

## Highest-Severity Events

### 1. Game 58, ply 107: d5h5 (Rh5)

- Categories: `engine-loss, eval-swing, opponent-recapture, recapture-trade, bad-trade-sequence`
- Score: `-253 -> -410` for current-eval, delta `-157`
- Sequence after reply `g4h5`: `-253 -> -751`, delta `-498`
- Engine recaptures available: `none`
- Material: `R` captures `-` for `0` cp
- Component movement: pawn_structure -116, threats -21, piece_square -8, rook_files -7, center_control +1
- Result: `1-0` `white_win:checkmate`
- FEN before: `2R2N2/7P/p4k2/1ppr4/5PK1/6P1/8/8 b - - 0 56`

- Engine re-search: `d5h5` score `-1496`
- Engine re-search PV: `d5h5 g4h5 c5c4 h7h8q f6e7`

- Engine played-move constrained score: `-1496`
- Engine played-move PV: `d5h5 g4h5 c5c4 h7h8q f6e7`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-1496`, delta `+0`
- Engine reference-move PV: `d5h5 g4h5 c5c4 h7h8q f6e7`
- Engine reference-move SEE: `0`

- Reference bestmove: `d5h5` score `-1053`
- Reference PV: `d5h5 g4h5 f6f5 c8c5 f5e4 f8e6 b5b4 f4f5 a6a5 e6g5 e4d4 c5a5`

### 2. Game 76, ply 141: f4f3 (f3)

- Categories: `engine-loss, eval-swing, opponent-recapture, recapture-trade, bad-trade-sequence`
- Score: `+148 -> -36` for current-eval, delta `-184`
- Sequence after reply `e1f3`: `+148 -> -274`, delta `-422`
- Engine recaptures available: `none`
- Material: `P` captures `-` for `0` cp
- Component movement: pawn_structure -174, piece_square -9, threats -2, pawn_dynamics -2, safe_mobility +1, space +2
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/pK6/1pP5/1P2k3/P4p2/6p1/8/4N3 b - - 1 71`

- Engine re-search: `e5d4` score `-773`
- Engine re-search PV: `e5d4 c6c7 d4e3 c7c8q e3f2`

- Engine played-move constrained score: `-778`
- Engine played-move PV: `f4f3 e1f3 e5e4 f3e1 g3g2`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-786`, delta `-8`
- Engine reference-move PV: `e5e4 c6c7 e4e3 c7c8q e3f2`
- Engine reference-move SEE: `0`

- Reference bestmove: `e5e4` score `-558`
- Reference PV: `e5e4 c6c7 f4f3 e1f3 e4f3 c7c8r g3g2 c8c1 f3e3 c1e1 e3d2 e1a1`
- Reference played-move score: `-704`, delta `+146`
- Reference played-move PV: `e1f3 e5f4 f3g1 g3g2 c6c7 f4g3 b7b8 g3h2`

### 3. Game 75, ply 158: d1e2 (Ke2)

- Categories: `engine-loss, eval-swing`
- Score: `-461 -> -755` for current-eval, delta `-294`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -321, center_control +2, piece_square +25
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/8/8/8/8/1pk5/3p4/3K4 w - - 0 80`

- Engine re-search: `d1e2` score `-1928`
- Engine re-search PV: `d1e2 b3b2 e2e3 d2d1q e3e4`

- Engine played-move constrained score: `-1928`
- Engine played-move PV: `d1e2 b3b2 e2e3 d2d1q e3e4`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-1928`, delta `+0`
- Engine reference-move PV: `d1e2 b3b2 e2e3 d2d1q e3e4`
- Engine reference-move SEE: `0`

- Reference bestmove: `d1e2` score `-4287`
- Reference PV: `d1e2 b3b2 e2f2 b2b1q f2e3 b1g1 e3f3 g1h1 f3f4 h1h4 f4f5 h4h5 f5e6`

### 4. Game 18, ply 187: g4g3 (Kg3)

- Categories: `eval-swing`
- Score: `+1627 -> +1324` for current-eval, delta `-303`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -302, mobility -2, safe_mobility -1, space +2
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/8/p1p5/1p2P3/5nk1/3r4/8/6K1 b - - 5 96`

- Engine re-search: `g4g3` score `899997`
- Engine re-search PV: `g4g3 g1f1 d3d1`

- Engine played-move constrained score: `899997`
- Engine played-move PV: `g4g3 g1f1 d3d1`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `899997`, delta `+0`
- Engine reference-move PV: `g4g3 g1f1 d3d1`
- Engine reference-move SEE: `0`

- Reference bestmove: `g4g3` score `899998`
- Reference PV: `g4g3 g1f1 d3d1`

### 5. Game 20, ply 120: f7g8 (Kg8)

- Categories: `engine-loss, eval-swing`
- Score: `-45 -> -306` for current-eval, delta `-261`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -243, piece_square -23, king_safety +5
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/1p1KPkp1/1P5p/p4p1P/7P/B7/8/8 b - - 2 62`

- Engine re-search: `f7g8` score `-899992`
- Engine re-search PV: `f7g8 e7e8q g8h7 a3b2 a5a4`

- Engine played-move constrained score: `-899992`
- Engine played-move PV: `f7g8 e7e8q g8h7 a3b2 a5a4`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-899992`, delta `+0`
- Engine reference-move PV: `f7g8 e7e8q g8h7 a3b2 a5a4`
- Engine reference-move SEE: `0`

- Reference bestmove: `f7g8` score `-899996`
- Reference PV: `f7g8 a3b2 g8h7 e7e8q f5f4 e8g6 h7h8 g6g7`

### 6. Game 75, ply 160: e2f2 (Kf2)

- Categories: `engine-loss, eval-swing`
- Score: `-833 -> -1080` for current-eval, delta `-247`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -246, center_control -1
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/8/8/8/8/2k5/1p1pK3/8 w - - 0 81`

- Engine re-search: `e2e3` score `-1934`
- Engine re-search PV: `e2e3 d2d1q e3e4 d1d6 e4f3`

- Engine played-move constrained score: `-1946`
- Engine played-move PV: `e2f2 b2b1q f2e3 b1f5 e3e2`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-1934`, delta `+12`
- Engine reference-move PV: `e2e3 d2d1q e3e4 d1d6 e4f3`
- Engine reference-move SEE: `0`

- Reference bestmove: `e2e3` score `-899994`
- Reference PV: `e2e3 b2b1q e3f4 d2d1q f4e5 b1b6 e5f4 b6d4 f4g5 d1g4 g5h6 d4h8`
- Reference played-move score: `-899995`, delta `+1`
- Reference played-move PV: `b2b1q f2f3 d2d1q f3f4 d1d4 f4g5 b1g1 g5h6 d4h4`

### 7. Game 5, ply 196: a2a3 (Ka3)

- Categories: `engine-loss, eval-swing`
- Score: `-671 -> -901` for current-eval, delta `-230`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -229, piece_square -1
- Result: `0-1` `black_win:checkmate`
- FEN before: `7b/8/8/8/8/8/Kpk5/8 w - - 46 100`

- Engine re-search: `a2a3` score `-820514`
- Engine re-search PV: `a2a3 b2b1r a3a4 h8c3 a4a3`

- Engine played-move constrained score: `-820514`
- Engine played-move PV: `a2a3 b2b1r a3a4 h8c3 a4a3`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-820514`, delta `+0`
- Engine reference-move PV: `a2a3 b2b1r a3a4 h8c3 a4a3`
- Engine reference-move SEE: `0`

- Reference bestmove: `a2a3` score `-899997`
- Reference PV: `a2a3 b2b1q a3a4 b1b6 a4a3 b6a5`

### 8. Game 13, ply 148: e4d3 (Kd3)

- Categories: `engine-loss, eval-swing`
- Score: `-731 -> -951` for current-eval, delta `-220`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -205, piece_square -9, space -4, center_control -4, threats +2
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/8/8/8/4K3/6kp/4p3/8 w - - 2 78`

- Engine re-search: `e4d3` score `-1891`
- Engine re-search PV: `e4d3 e2e1q d3d4 h3h2 d4d3`

- Engine played-move constrained score: `-1891`
- Engine played-move PV: `e4d3 e2e1q d3d4 h3h2 d4d3`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-1907`, delta `-16`
- Engine reference-move PV: `e4e5 e2e1q e5d4 h3h2 d4d5`
- Engine reference-move SEE: `0`

- Reference bestmove: `e4e5` score `-4293`
- Reference PV: `e4e5 e2e1q e5f6 h3h2 f6f7 h2h1q f7g7 e1e6`
- Reference played-move score: `-4304`, delta `+11`
- Reference played-move PV: `e2e1q d3c4 h3h2 c4b5 e1e2 b5c5 e2e7 c5d5 e7d7 d5c5 d7c7 c5d5 h2h1q d5d4`

### 9. Game 58, ply 109: f6f5 (Kf5)

- Categories: `engine-loss, eval-swing`
- Score: `-751 -> -961` for current-eval, delta `-210`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -220, king_safety -3, center_control +4, piece_square +7
- Result: `1-0` `white_win:checkmate`
- FEN before: `2R2N2/7P/p4k2/1pp4K/5P2/6P1/8/8 b - - 0 57`

- Engine re-search: `f6f5` score `-1793`
- Engine re-search PV: `f6f5 c8c5 f5e4 f8e6 e4e3`

- Engine played-move constrained score: `-1793`
- Engine played-move PV: `f6f5 c8c5 f5e4 f8e6 e4e3`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-1793`, delta `+0`
- Engine reference-move PV: `f6f5 c8c5 f5e4 f8e6 e4e3`
- Engine reference-move SEE: `0`

- Reference bestmove: `f6f5` score `-1078`
- Reference PV: `f6f5 c8c5 f5e4 f4f5 b5b4 f5f6 b4b3 f6f7 b3b2 f8e6 b2b1b f7f8r`

### 10. Game 31, ply 90: e3f4 (Kf4)

- Categories: `engine-loss, eval-swing`
- Score: `-616 -> -824` for current-eval, delta `-208`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -190, threats -17, center_control -3, piece_square -2, space +2, king_safety +5
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/k5P1/p7/4P3/3P4/1p1qK2P/4b3/8 w - - 1 47`

- Engine re-search: `e3f4` score `-1772`
- Engine re-search PV: `e3f4 d3f3 f4g5 b3b2 g7g8q`

- Engine played-move constrained score: `-1772`
- Engine played-move PV: `e3f4 d3f3 f4g5 b3b2 g7g8q`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-1772`, delta `+0`
- Engine reference-move PV: `e3f4 b3b2 g7g8q d3f3 f4g5`
- Engine reference-move SEE: `0`

- Reference bestmove: `e3f4` score `-1203`
- Reference PV: `e3f4 b3b2 g7g8q d3f3 f4g5 f3g3 g5h6 g3g8 d4d5 b2b1q`

### 11. Game 13, ply 132: d3d4 (Kd4)

- Categories: `engine-loss, eval-swing`
- Score: `-172 -> -369` for current-eval, delta `-197`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -213, center_control +4, piece_square +8
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/8/8/8/7p/3Kp3/5k1P/8 w - - 2 70`

- Engine re-search: `d3d4` score `-901`
- Engine re-search PV: `d3d4 e3e2 d4c5 e2e1q c5d6`

- Engine played-move constrained score: `-901`
- Engine played-move PV: `d3d4 e3e2 d4c5 e2e1q c5d6`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-901`, delta `+0`
- Engine reference-move PV: `d3d4 e3e2 d4c5 e2e1q c5d6`
- Engine reference-move SEE: `0`

- Reference bestmove: `d3d4` score `-3530`
- Reference PV: `d3d4 e3e2 d4d5 h4h3 d5e4 f2g2 e4d4 g2h2`

### 12. Game 86, ply 167: e4e3 (Ke3)

- Categories: `engine-loss, eval-swing`
- Score: `-192 -> -388` for current-eval, delta `-196`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -188, piece_square -8, center_control -4, space +4
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/8/5K2/7P/4k3/8/8/8 b - - 10 85`

- Engine re-search: `e4d4` score `-939`
- Engine re-search PV: `e4d4 h5h6 d4e4 h6h7 e4d4`

- Engine played-move constrained score: `-939`
- Engine played-move PV: `e4e3 h5h6 e3f3 h6h7 f3e4`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-939`, delta `+0`
- Engine reference-move PV: `e4f3 h5h6 f3e4 h6h7 e4d4`
- Engine reference-move SEE: `0`

- Reference bestmove: `e4f3` score `-3487`
- Reference PV: `e4f3 h5h6 f3g4 h6h7 g4g3 h7h8q g3g2 h8f8 g2g3 f8a8 g3h2`
- Reference played-move score: `-3494`, delta `+7`
- Reference played-move PV: `h5h6 e3e2 h6h7 e2f1 h7h8q f1g2 f6f7 g2g3 f7g6 g3f3`

### 13. Game 20, ply 122: a5a4 (a4)

- Categories: `engine-loss, eval-swing`
- Score: `-38 -> -231` for current-eval, delta `-193`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `P` captures `-` for `0` cp
- Component movement: pawn_structure -180, piece_square -13
- Result: `1-0` `white_win:checkmate`
- FEN before: `6k1/1p1KP1p1/1P5p/p4p1P/7P/8/1B6/8 b - - 4 63`

- Engine re-search: `g8h8` score `-899994`
- Engine re-search PV: `g8h8 e7e8q h8h7 e8g6 h7g8`

- Engine played-move constrained score: `-899994`
- Engine played-move PV: `a5a4 e7e8q g8h7 e8g6 h7g8`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-899994`, delta `+0`
- Engine reference-move PV: `a5a4 e7e8q g8h7 e8g6 h7g8`
- Engine reference-move SEE: `0`

- Reference bestmove: `a5a4` score `-899997`
- Reference PV: `a5a4 e7e8q g8h7 e8g6 h7h8 g6g7`

### 14. Game 20, ply 106: c8f8 (Rf8)

- Categories: `engine-loss, eval-swing`
- Score: `+305 -> +118` for current-eval, delta `-187`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `R` captures `-` for `0` cp
- Component movement: pawn_structure -178, rook_files -6, threats -5, king_safety +2
- Result: `1-0` `white_win:checkmate`
- FEN before: `2r3k1/1pP1K1p1/1P2Pp1p/p1B4P/8/7P/8/8 b - - 1 55`

- Engine re-search: `c8a8` score `-8`
- Engine re-search PV: `c8a8 e7d7 f6f5 c5f2 a5a4`

- Engine played-move constrained score: `-29`
- Engine played-move PV: `c8f8 e7d7 f8a8 c5a3 f6f5`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-8`, delta `+21`
- Engine reference-move PV: `c8a8 e7d7 f6f5 c5f2 a5a4`
- Engine reference-move SEE: `0`

- Reference bestmove: `c8a8` score `-645`
- Reference PV: `c8a8 e7d7 f6f5 c7c8r a8c8 d7c8 a5a4 c8d7 f5f4 e6e7 f4f3 e7e8r g8h7`
- Reference played-move score: `-647`, delta `+2`
- Reference played-move PV: `e7d7 f8a8 c7c8q a8c8 d7c8 g8h7 c8d7 a5a4 e6e7`

### 15. Game 20, ply 110: c8f8 (Rf8)

- Categories: `engine-loss, eval-swing`
- Score: `+274 -> +87` for current-eval, delta `-187`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `R` captures `-` for `0` cp
- Component movement: pawn_structure -178, rook_files -6, threats -5, king_safety +2
- Result: `1-0` `white_win:checkmate`
- FEN before: `2r3k1/1pP1K1p1/1P2Pp1p/p6P/7P/B7/8/8 b - - 2 57`

- Engine re-search: `c8f8` score `3`
- Engine re-search PV: `c8f8 e7d7 f8a8 c7c8q a8c8`

- Engine played-move constrained score: `3`
- Engine played-move PV: `c8f8 e7d7 f8a8 c7c8q a8c8`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `3`, delta `+0`
- Engine reference-move PV: `c8f8 e7d7 f8a8 c7c8q a8c8`
- Engine reference-move SEE: `0`

- Reference bestmove: `c8f8` score `-707`
- Reference PV: `c8f8 e7d7 f8a8 c7c8r a8c8 d7c8 a5a4 e6e7 g8f7 c8d7 f7g8`

### 16. Game 76, ply 143: e5e4 (Ke4)

- Categories: `engine-loss, eval-swing`
- Score: `-274 -> -458` for current-eval, delta `-184`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -197, piece_square -1, space +3, threats +9
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/pK6/1pP5/1P2k3/P7/5Np1/8/8 b - - 0 72`

- Engine re-search: `e5e4` score `-1000`
- Engine re-search PV: `e5e4 f3g1 e4e3 b7a7 e3f2`

- Engine played-move constrained score: `-1000`
- Engine played-move PV: `e5e4 f3g1 e4e3 b7a7 e3f2`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-1000`, delta `+0`
- Engine reference-move PV: `e5f4 f3g1 f4e3 b7a7 e3f2`
- Engine reference-move SEE: `0`

- Reference bestmove: `e5f4` score `-685`
- Reference PV: `e5f4 f3g1 g3g2 c6c7 f4g3 c7c8r g3h2 c8c1 h2g3 c1b1`
- Reference played-move score: `-650`, delta `-35`
- Reference played-move PV: `f3g1 g3g2 c6c7 e4e3 c7c8r e3f4 c8f8 f4g4 f8f2`

### 17. Game 61, ply 87: b4a6 (Na6)

- Categories: `eval-swing`
- Score: `+3100 -> +2892` for current-eval, delta `-208`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `N` captures `-` for `0` cp
- Component movement: pawn_structure -174, piece_square -23, mobility -5, bishop_quality -5, space +1, center_control +2
- Result: `1-0` `white_win:checkmate`
- FEN before: `3Q4/k7/8/1B6/1N3P2/2K5/P5PP/7R w - - 1 44`

- Engine re-search: `b4c6` score `899997`
- Engine re-search PV: `b4c6 a7b7 d8b8`

- Engine played-move constrained score: `899997`
- Engine played-move PV: `b4a6 a7b7 d8b8`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `899997`, delta `+0`
- Engine reference-move PV: `b4c6 a7b7 d8b8`
- Engine reference-move SEE: `0`

- Reference bestmove: `b4c6` score `899998`
- Reference PV: `b4c6 a7b7 d8b8`
- Reference played-move score: `899999`, delta `-1`
- Reference played-move PV: `a7b7 d8b8`

### 18. Game 86, ply 165: d4e4 (Ke4)

- Categories: `engine-loss, eval-swing`
- Score: `-172 -> -352` for current-eval, delta `-180`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -180
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/6K1/8/7P/3k4/8/8/8 b - - 8 84`

- Engine re-search: `d4d5` score `-909`
- Engine re-search PV: `d4d5 h5h6 d5e6 h6h7 e6e5`

- Engine played-move constrained score: `-939`
- Engine played-move PV: `d4e4 g7f6 e4e3 h5h6 e3f3`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-939`, delta `+0`
- Engine reference-move PV: `d4e4 g7f6 e4f3 h5h6 f3e4`
- Engine reference-move SEE: `0`

- Reference bestmove: `d4e4` score `-3490`
- Reference PV: `d4e4 g7f6 e4f3 h5h6 f3g2 h6h7 g2f2 h7h8q f2g2 f6g7 g2f2 h8h2 f2f1`

### 19. Game 6, ply 117: c7d6 (Kd6)

- Categories: `engine-loss, eval-swing`
- Score: `-273 -> -451` for current-eval, delta `-178`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -203, mobility -6, space -5, threats -4, king_safety +9, piece_square +22
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/1Rk5/P7/8/P3p3/2p1Kp2/8/8 b - - 1 60`

- Engine re-search: `c7c6` score `-492`
- Engine re-search PV: `c7c6 b7b3 c3c2 b3c3 c6b6`

- Engine played-move constrained score: `-611`
- Engine played-move PV: `c7d6 b7b3 d6c7 b3c3 c7b6`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-492`, delta `+119`
- Engine reference-move PV: `c7c6 b7b3 c3c2 b3c3 c6b6`
- Engine reference-move SEE: `0`

- Reference bestmove: `c7c6` score `-562`
- Reference PV: `c7c6 b7b3 c3c2 b3c3 c6b6 c3c2 b6a6 c2b2`
- Reference played-move score: `-583`, delta `+21`
- Reference played-move PV: `b7b3 d6c7 b3c3 c7b6 c3c8 b6a6 c8c1 a6b7`

### 20. Game 86, ply 161: d6c5 (Kc5)

- Categories: `engine-loss, eval-swing`
- Score: `-190 -> -366` for current-eval, delta `-176`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -180, space +4
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/6K1/3k4/7P/8/8/8/8 b - - 4 82`

- Engine re-search: `d6e6` score `-909`
- Engine re-search PV: `d6e6 h5h6 e6e5 h6h7 e5f5`

- Engine played-move constrained score: `-939`
- Engine played-move PV: `d6c5 h5h6 c5b5 h6h7 b5c4`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-909`, delta `+30`
- Engine reference-move PV: `d6e6 h5h6 e6e5 h6h7 e5f5`
- Engine reference-move SEE: `0`

- Reference bestmove: `d6e6` score `-3487`
- Reference PV: `d6e6 h5h6 e6f5 h6h7 f5g4 h7h8q g4g3 g7f6 g3f3 f6f5 f3f2 h8f8 f2g3`
- Reference played-move score: `-3525`, delta `+38`
- Reference played-move PV: `h5h6 c5d4 h6h7 d4e3 h7h8q e3f2 g7f6 f2g2`

### 21. Game 86, ply 157: e6d5 (Kd5)

- Categories: `engine-loss, eval-swing`
- Score: `-182 -> -354` for current-eval, delta `-172`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -188, space +5, piece_square +8
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/6K1/4k3/7P/8/8/8/8 b - - 0 80`

- Engine re-search: `e6f5` score `-909`
- Engine re-search PV: `e6f5 h5h6 f5g5 h6h7 g5f5`

- Engine played-move constrained score: `-909`
- Engine played-move PV: `e6d5 h5h6 d5e6 h6h7 e6e5`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-909`, delta `+0`
- Engine reference-move PV: `e6e5 h5h6 e5d5 h6h7 d5e5`
- Engine reference-move SEE: `0`

- Reference bestmove: `e6e5` score `-3487`
- Reference PV: `e6e5 h5h6 e5f5 h6h7 f5g4 h7h8q g4g3 g7f6 g3g2 f6f5 g2f2 h8f8`
- Reference played-move score: `-3491`, delta `+4`
- Reference played-move PV: `h5h6 d5e4 h6h7 e4f3 h7h8q f3g3 h8h1 g3g4 h1h2 g4f3`

### 22. Game 76, ply 189: d4d3 (Kd3)

- Categories: `engine-loss, eval-swing`
- Score: `-1129 -> -1289` for current-eval, delta `-160`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -156, piece_square -6, center_control -4, king_safety +1, space +5
- Result: `1-0` `white_win:checkmate`
- FEN before: `1Q6/8/2K5/P7/3k4/8/8/8 b - - 2 95`

- Engine re-search: `d4d3` score `-1902`
- Engine re-search PV: `d4d3 a5a6 d3e3 a6a7 e3e4`

- Engine played-move constrained score: `-1902`
- Engine played-move PV: `d4d3 a5a6 d3e3 a6a7 e3e4`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-1902`, delta `+0`
- Engine reference-move PV: `d4e3 a5a6 e3d3 a6a7 d3e4`
- Engine reference-move SEE: `0`

- Reference bestmove: `d4e3` score `-899986`
- Reference PV: `d4e3 b8d6 e3f2 a5a6 f2f1 a6a7 f1g2 d6d5 g2g3 d5b3 g3f2 a7a8q f2g2 b3b2 g2h3`
- Reference played-move score: `-899989`, delta `+3`
- Reference played-move PV: `a5a6 d3e3 a6a7 e3e2 b8b2 e2f3 b2c3 f3g2 c3c2 g2f3 a7a8q f3e3 c2c3 e3e4 a8g8`

### 23. Game 20, ply 94: c8a8 (Ra8)

- Categories: `engine-loss, eval-swing`
- Score: `+142 -> -15` for current-eval, delta `-157`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `R` captures `-` for `0` cp
- Component movement: pawn_structure -147, rook_files -6, piece_square -4
- Result: `1-0` `white_win:checkmate`
- FEN before: `2r3k1/1pP3p1/pP1BPp1p/7P/3K4/8/7P/8 b - - 7 49`

- Engine re-search: `c8a8` score `87`
- Engine re-search PV: `c8a8 d6f4 a8c8 f4d6 c8a8`

- Engine played-move constrained score: `87`
- Engine played-move PV: `c8a8 d6f4 a8c8 f4d6 c8a8`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-56`, delta `-143`
- Engine reference-move PV: `f6f5 d4e5 f5f4 e5f4 a6a5`
- Engine reference-move SEE: `0`

- Reference bestmove: `f6f5` score `-402`
- Reference PV: `f6f5 d4d5 f5f4 d6f4 g8f8 f4d6 f8e8 d6b4`
- Reference played-move score: `-484`, delta `+82`
- Reference played-move PV: `d4d5 f6f5 d6a3 f5f4 d5d6 f4f3 a3c5`

### 24. Game 76, ply 187: e4d4 (Kd4)

- Categories: `engine-loss, eval-swing`
- Score: `-1108 -> -1258` for current-eval, delta `-150`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -150
- Result: `1-0` `white_win:checkmate`
- FEN before: `1Q6/8/1K6/P7/4k3/8/8/8 b - - 0 94`

- Engine re-search: `e4d5` score `-1907`
- Engine re-search PV: `e4d5 a5a6 d5e4 a6a7 e4d4`

- Engine played-move constrained score: `-1912`
- Engine played-move PV: `e4d4 b8f4 d4c3 a5a6 c3c2`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-1907`, delta `+5`
- Engine reference-move PV: `e4f5 a5a6 f5e4 a6a7 e4d4`
- Engine reference-move SEE: `0`

- Reference bestmove: `e4f5` score `-899986`
- Reference PV: `e4f5 a5a6 f5g6 b8g8 g6f5 a6a7 f5e4 a7a8q e4d3 g8b8 d3c2 b6c6 c2c3 b8d6 c3b3 d6d1 b3b4`
- Reference played-move score: `-899988`, delta `+2`
- Reference played-move PV: `b6c6 d4e3 a5a6 e3f3 a6a7 f3e2 b8b2 e2f3 b2c3 f3g2 c3c2 g2f3 a7a8q f3e3`

### 25. Game 31, ply 88: g6g7 (g7)

- Categories: `engine-loss, eval-swing`
- Score: `-679 -> -824` for current-eval, delta `-145`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `P` captures `-` for `0` cp
- Component movement: pawn_structure -145, piece_square -1, safe_mobility -1, threats +2
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/k7/p5P1/4P3/3P4/1p2K2P/2q1b3/8 w - - 0 46`

- Engine re-search: `g6g7` score `-1772`
- Engine re-search PV: `g6g7 b3b2 g7g8q c2d3 e3f4`

- Engine played-move constrained score: `-1772`
- Engine played-move PV: `g6g7 b3b2 g7g8q c2d3 e3f4`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-1772`, delta `+0`
- Engine reference-move PV: `g6g7 c2d3 e3f4 b3b2 g7g8q`
- Engine reference-move SEE: `0`

- Reference bestmove: `g6g7` score `-1233`
- Reference PV: `g6g7 c2d3 e3f4 b3b2 g7g8q d3f3 f4g5 f3g3 g5f6 g3g8 f6e7 b2b1q e7d7 b1b4`


## Trade And Simplification Watchlist

### 1. Game 58, ply 107: d5h5 (Rh5)

- Categories: `engine-loss, eval-swing, opponent-recapture, recapture-trade, bad-trade-sequence`
- Score: `-253 -> -410` for current-eval, delta `-157`
- Sequence after reply `g4h5`: `-253 -> -751`, delta `-498`
- Engine recaptures available: `none`
- Material: `R` captures `-` for `0` cp
- Component movement: pawn_structure -116, threats -21, piece_square -8, rook_files -7, center_control +1
- Result: `1-0` `white_win:checkmate`
- FEN before: `2R2N2/7P/p4k2/1ppr4/5PK1/6P1/8/8 b - - 0 56`

- Engine re-search: `d5h5` score `-1496`
- Engine re-search PV: `d5h5 g4h5 c5c4 h7h8q f6e7`

- Engine played-move constrained score: `-1496`
- Engine played-move PV: `d5h5 g4h5 c5c4 h7h8q f6e7`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-1496`, delta `+0`
- Engine reference-move PV: `d5h5 g4h5 c5c4 h7h8q f6e7`
- Engine reference-move SEE: `0`

- Reference bestmove: `d5h5` score `-1053`
- Reference PV: `d5h5 g4h5 f6f5 c8c5 f5e4 f8e6 b5b4 f4f5 a6a5 e6g5 e4d4 c5a5`

### 2. Game 76, ply 141: f4f3 (f3)

- Categories: `engine-loss, eval-swing, opponent-recapture, recapture-trade, bad-trade-sequence`
- Score: `+148 -> -36` for current-eval, delta `-184`
- Sequence after reply `e1f3`: `+148 -> -274`, delta `-422`
- Engine recaptures available: `none`
- Material: `P` captures `-` for `0` cp
- Component movement: pawn_structure -174, piece_square -9, threats -2, pawn_dynamics -2, safe_mobility +1, space +2
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/pK6/1pP5/1P2k3/P4p2/6p1/8/4N3 b - - 1 71`

- Engine re-search: `e5d4` score `-773`
- Engine re-search PV: `e5d4 c6c7 d4e3 c7c8q e3f2`

- Engine played-move constrained score: `-778`
- Engine played-move PV: `f4f3 e1f3 e5e4 f3e1 g3g2`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-786`, delta `-8`
- Engine reference-move PV: `e5e4 c6c7 e4e3 c7c8q e3f2`
- Engine reference-move SEE: `0`

- Reference bestmove: `e5e4` score `-558`
- Reference PV: `e5e4 c6c7 f4f3 e1f3 e4f3 c7c8r g3g2 c8c1 f3e3 c1e1 e3d2 e1a1`
- Reference played-move score: `-704`, delta `+146`
- Reference played-move PV: `e1f3 e5f4 f3g1 g3g2 c6c7 f4g3 b7b8 g3h2`

### 3. Game 63, ply 58: c4f7 (Qf7)

- Categories: `engine-loss, opponent-recapture, recapture-trade, queen-trade-sequence, bad-trade-sequence`
- Score: `+314 -> +319` for current-eval, delta `+5`
- Sequence after reply `f8f7`: `+314 -> -583`, delta `-897`
- Engine recaptures available: `none`
- Material: `Q` captures `-` for `0` cp
- Component movement: threats -69, piece_square -5, space +15, king_safety +37
- Result: `0-1` `black_win:checkmate`
- FEN before: `2k1rr2/1bp4p/1p6/8/1PQ5/3P3P/P1P3PR/5K2 w - - 1 32`

- Engine re-search: `c4f4` score `-899996`
- Engine re-search PV: `c4f4 f8f4 f1g1 e8e1`

- Engine played-move constrained score: `-899996`
- Engine played-move PV: `c4f7 f8f7 f1g1 e8e1`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-899996`, delta `+0`
- Engine reference-move PV: `c4f4 f8f4 f1g1 e8e1`
- Engine reference-move SEE: `0`

- Reference bestmove: `c4f4` score `-899998`
- Reference PV: `c4f4 f8f4 f1g1 e8e1`
- Reference played-move score: `-899998`, delta `+0`
- Reference played-move PV: `f8f7 f1g1 e8e1`

### 4. Game 60, ply 38: d4d7 (Qxd7)

- Categories: `engine-loss, opponent-recapture, recapture-trade, queen-trade-sequence, bad-trade-sequence, played-negative-see, reference-negative-see`
- Score: `+495 -> +921` for current-eval, delta `+426`
- Sequence after reply `c6d7`: `+495 -> -113`, delta `-608`
- Engine recaptures available: `none`
- Material: `Q` captures `N` for `320` cp
- Component movement: space -6, piece_square -5, center_control -1, development -1, threats +34, material +320
- Result: `1-0` `white_win:checkmate`
- FEN before: `rk3b1r/p2Np1pp/2Q5/8/2Nq3P/7R/P2nPP2/4K3 b - - 2 21`

- Engine re-search: `d4d7` score `-178`
- Engine re-search PV: `d4d7 c6d7 d2c4 h3c3 e7e5`

- Engine played-move constrained score: `-178`
- Engine played-move PV: `d4d7 c6d7 d2c4 h3c3 e7e5`
- Engine played-move SEE: `-580`
- Engine reference-move constrained score: `-178`, delta `+0`
- Engine reference-move PV: `d4d7 c6d7 d2c4 h3c3 e7e5`
- Engine reference-move SEE: `-580`

- Reference bestmove: `d4d7` score `-686`
- Reference PV: `d4d7 c6d7 e7e6 c4a5 d2f3 e1f1 f3d2 f1g1`

### 5. Game 14, ply 51: e8e7 (Re7)

- Categories: `engine-loss, opponent-recapture, recapture-trade, queen-trade-sequence, bad-trade-sequence, reference-negative-see`
- Score: `-205 -> -42` for current-eval, delta `+163`
- Sequence after reply `d7e7`: `-205 -> -716`, delta `-511`
- Engine recaptures available: `none`
- Material: `R` captures `-` for `0` cp
- Component movement: piece_square -4, king_safety +46, threats +89
- Result: `1-0` `white_win:checkmate`
- FEN before: `r3r1k1/pp1Q3p/4pppB/2pp4/2q5/2P2B1P/5PP1/R4RK1 b - - 0 29`

- Engine re-search: `e8e7` score `-899994`
- Engine re-search PV: `e8e7 d7e7 c4f1 g1f1 a7a6`

- Engine played-move constrained score: `-899994`
- Engine played-move PV: `e8e7 d7e7 c4f1 g1f1 a7a6`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-899994`, delta `+0`
- Engine reference-move PV: `c4f1 a1f1 e8e7 d7e7 a7a6`
- Engine reference-move SEE: `-400`

- Reference bestmove: `c4f1` score `-899997`
- Reference PV: `c4f1 a1f1 e8e7 d7e7 b7b6 e7g7`
- Reference played-move score: `-899997`, delta `+0`
- Reference played-move PV: `d7e7 c4f1 a1f1 b7b6 e7g7`

### 6. Game 65, ply 20: a4b4 (Qxb4)

- Categories: `opponent-recapture, recapture-trade, queen-trade-sequence, bad-trade-sequence, played-negative-see, engine-can-avoid-negative-see`
- Score: `+347 -> +777` for current-eval, delta `+430`
- Sequence after reply `a6b4`: `+347 -> -179`, delta `-526`
- Engine recaptures available: `none`
- Material: `Q` captures `B` for `330` cp
- Component movement: threats -146, rook_files -1, pawn_dynamics -1, king_safety +99, material +330
- Result: `1-0` `white_win:checkmate`
- FEN before: `r1bqk2r/p2n2pp/n3Np2/1N2p3/Qbp5/4B3/PP2PPPP/3RKB1R w Kkq - 1 12`

- Engine re-search: `b5c3` score `434`
- Engine re-search PV: `b5c3 d8a5 e6g7 e8f8 g7e6`

- Engine played-move constrained score: `277`
- Engine played-move PV: `a4b4 a6b4 b5c7 e8e7 e6d8`
- Engine played-move SEE: `-570`
- Engine reference-move constrained score: `410`, delta `+133`
- Engine reference-move PV: `e3d2 d8b6 e6g7 e8f8 d2b4`
- Engine reference-move SEE: `0`

- Reference bestmove: `e3d2` score `396`
- Reference PV: `e3d2 d8e7 e6c7 e8f8 d2b4 a6b4 a2a3 a8b8 a3b4`
- Reference played-move score: `325`, delta `+71`
- Reference played-move PV: `a6b4 b5c7 e8e7 e6d8 b4c2 e1d2 c2e3 d2e3 h8d8 c7a8 f6f5 f2f4 c8b7 a8c7 d7b6`

### 7. Game 83, ply 48: f6f8 (Rf8+)

- Categories: `engine-loss, opponent-recapture, recapture-trade, bad-trade-sequence`
- Score: `+615 -> +634` for current-eval, delta `+19`
- Sequence after reply `e8f8`: `+615 -> +38`, delta `-577`
- Engine recaptures available: `none`
- Material: `R` captures `-` for `0` cp
- Component movement: mobility -8, space -3, center_control -3, safe_mobility -2, trade_context +5, king_safety +30
- Result: `0-1` `black_win:checkmate`
- FEN before: `4k1r1/pb5p/5R2/4P3/3p4/3Pn2q/PPP1Q1PP/R5K1 w - - 7 27`

- Engine re-search: `f6e6` score `-354`
- Engine re-search PV: `f6e6 e8d7 e6d6 d7e7 g2g4`

- Engine played-move constrained score: `-528`
- Engine played-move PV: `f6f8 e8f8 a1f1 f8e7 f1f7`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-528`, delta `+0`
- Engine reference-move PV: `f6f8 e8f8 a1f1 f8e7 f1f7`
- Engine reference-move SEE: `0`

- Reference bestmove: `f6f8` score `-621`
- Reference PV: `f6f8 e8f8 e2f2 f8e8 g2g3 g8f8 f2e2 h3f5`

### 8. Game 80, ply 100: e5e8 (Re8)

- Categories: `engine-loss, opponent-recapture, recapture-trade, bad-trade-sequence`
- Score: `+316 -> +347` for current-eval, delta `+31`
- Sequence after reply `c8e8`: `+316 -> -261`, delta `-577`
- Engine recaptures available: `none`
- Material: `R` captures `-` for `0` cp
- Component movement: center_control -2, mobility +6, threats +22
- Result: `1-0` `white_win:checkmate`
- FEN before: `2R4R/kp3pp1/p7/P2pr3/3p4/8/8/3K4 b - - 11 52`

- Engine re-search: `e5e8` score `-899996`
- Engine re-search PV: `e5e8 h8e8 d4d3 c8a8`

- Engine played-move constrained score: `-899996`
- Engine played-move PV: `e5e8 h8e8 d4d3 c8a8`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-899996`, delta `+0`
- Engine reference-move PV: `e5e1 d1e1 d4d3 c8a8`
- Engine reference-move SEE: `0`

- Reference bestmove: `e5e1` score `-899998`
- Reference PV: `e5e1 d1e1 b7b6 c8c7`
- Reference played-move score: `-899998`, delta `+0`
- Reference played-move PV: `c8e8 d4d3 e8a8`

### 9. Game 4, ply 53: c1h1 (Rh1+)

- Categories: `engine-loss, opponent-recapture, recapture-trade, bad-trade-sequence`
- Score: `+579 -> +546` for current-eval, delta `-33`
- Sequence after reply `a1h1`: `+579 -> +49`, delta `-530`
- Engine recaptures available: `none`
- Material: `R` captures `-` for `0` cp
- Component movement: threats -30, pawn_structure -20, rook_files -19, mobility -8, trade_context +19, king_safety +28
- Result: `1-0` `white_win:checkmate`
- FEN before: `4rk2/3p1p1p/1p1P1p1N/4p3/P1qp4/6QP/5PPK/R1r5 b - - 4 29`

- Engine re-search: `c1h1` score `-899996`
- Engine re-search PV: `c1h1 a1h1 c4c1 g3g8`

- Engine played-move constrained score: `-899996`
- Engine played-move PV: `c1h1 a1h1 c4c1 g3g8`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-899996`, delta `+0`
- Engine reference-move PV: `c1h1 a1h1 c4d3 g3g8`
- Engine reference-move SEE: `0`

- Reference bestmove: `c1h1` score `-899998`
- Reference PV: `c1h1 a1h1 c4a4 g3g8`

### 10. Game 54, ply 63: a2h2 (Rh2+)

- Categories: `engine-loss, opponent-recapture, recapture-trade, bad-trade-sequence`
- Score: `+374 -> +338` for current-eval, delta `-36`
- Sequence after reply `h3h2`: `+374 -> -152`, delta `-526`
- Engine recaptures available: `none`
- Material: `R` captures `-` for `0` cp
- Component movement: threats -24, pawn_structure -14, mobility -4, safe_mobility -2, king_safety +9
- Result: `1-0` `white_win:checkmate`
- FEN before: `7k/p4R1p/1p1p2pB/5p2/7P/6PK/r7/8 b - - 5 35`

- Engine re-search: `a2h2` score `-899996`
- Engine re-search PV: `a2h2 h3h2 f5f4 f7f8`

- Engine played-move constrained score: `-899996`
- Engine played-move PV: `a2h2 h3h2 f5f4 f7f8`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-899996`, delta `+0`
- Engine reference-move PV: `a2h2 h3h2 f5f4 f7f8`
- Engine reference-move SEE: `0`

- Reference bestmove: `a2h2` score `-899998`
- Reference PV: `a2h2 h3h2 b6b5 f7f8`

### 11. Game 83, ply 52: f1f8 (Rf8+)

- Categories: `engine-loss, opponent-recapture, recapture-trade, bad-trade-sequence`
- Score: `+85 -> +144` for current-eval, delta `+59`
- Sequence after reply `e8f8`: `+85 -> -476`, delta `-561`
- Engine recaptures available: `none`
- Material: `R` captures `-` for `0` cp
- Component movement: mobility -4, safe_mobility -1, king_safety +26, threats +31
- Result: `0-1` `black_win:checkmate`
- FEN before: `4k1r1/pb5p/8/4P3/3p4/3Pn2q/PPP1Q1PP/5RK1 w - - 2 29`

- Engine re-search: `f1f2` score `-1094`
- Engine re-search PV: `f1f2 b7g2 f2f8 g8f8 c2c4`

- Engine played-move constrained score: `-1550`
- Engine played-move PV: `f1f8 e8f8 g1f2 g8g2 f2e1`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-1094`, delta `+456`
- Engine reference-move PV: `f1f2 b7g2 f2f8 g8f8 c2c4`
- Engine reference-move SEE: `0`

- Reference bestmove: `f1f2` score `-899990`
- Reference PV: `f1f2 e3g2 g1f1 g2f4 f1e1 f4e2 e1e2 h3e3 e2f1 e3c1 f1e2 c1c2 e2e1 g8g1 f2f1`
- Reference played-move score: `-899994`, delta `+4`
- Reference played-move PV: `e8f8 e2f2 f8e8 f2f7 e8f7 e5e6 f7e6 g1f2 g8g2 f2e1 h3h4`

### 12. Game 93, ply 32: c5f8 (Qxf8+)

- Categories: `engine-loss, opponent-recapture, recapture-trade, queen-trade-sequence, bad-trade-sequence, played-negative-see, reference-negative-see`
- Score: `+862 -> +1351` for current-eval, delta `+489`
- Sequence after reply `g7f8`: `+862 -> +396`, delta `-466`
- Engine recaptures available: `none`
- Material: `Q` captures `R` for `500` cp
- Component movement: threats -81, piece_square -8, center_control -8, king_safety +23, material +500
- Result: `0-1` `black_win:checkmate`
- FEN before: `5r2/2p2Pkp/2n3p1/2Q5/2P1P3/2P2bPq/P4P1P/RB3RK1 w - - 1 20`

- Engine re-search: `c5f8` score `-899996`
- Engine re-search PV: `c5f8 g7f8 f1d1 h3g2`

- Engine played-move constrained score: `-899996`
- Engine played-move PV: `c5f8 g7f8 f1d1 h3g2`
- Engine played-move SEE: `-400`
- Engine reference-move constrained score: `-899996`, delta `+0`
- Engine reference-move PV: `c5f8 g7f8 f1d1 h3g2`
- Engine reference-move SEE: `-400`

- Reference bestmove: `c5f8` score `-899998`
- Reference PV: `c5f8 g7f8 a2a3 h3g2`

### 13. Game 12, ply 107: h3f3 (Rf3)

- Categories: `engine-loss, opponent-recapture, recapture-trade, bad-trade-sequence`
- Score: `+6 -> +53` for current-eval, delta `+47`
- Sequence after reply `f1f3`: `+6 -> -526`, delta `-532`
- Engine recaptures available: `none`
- Material: `R` captures `-` for `0` cp
- Component movement: king_safety +9, threats +23
- Result: `1-0` `white_win:checkmate`
- FEN before: `6k1/p2N2p1/1p4B1/2p5/P2p2K1/7r/8/5R2 b - - 14 55`

- Engine re-search: `h3f3` score `-899996`
- Engine re-search PV: `h3f3 f1f3 c5c4 f3f8`

- Engine played-move constrained score: `-899996`
- Engine played-move PV: `h3f3 f1f3 c5c4 f3f8`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-899996`, delta `+0`
- Engine reference-move PV: `h3h4 g4h4 d4d3 f1f8`
- Engine reference-move SEE: `0`

- Reference bestmove: `h3h4` score `-899998`
- Reference PV: `h3h4 g4h4 c5c4 f1f8`
- Reference played-move score: `-899998`, delta `+0`
- Reference played-move PV: `f1f3 c5c4 f3f8`

### 14. Game 41, ply 53: e6c7 (Nc7)

- Categories: `engine-loss, opponent-recapture, recapture-trade, queen-trade-sequence, bad-trade-sequence`
- Score: `+352 -> +334` for current-eval, delta `-18`
- Sequence after reply `c8c7`: `+352 -> -30`, delta `-382`
- Engine recaptures available: `none`
- Material: `N` captures `-` for `0` cp
- Component movement: piece_square -13, center_control -4, king_safety -2, threats -2, safe_mobility +1, mobility +4
- Result: `0-1` `black_win:checkmate`
- FEN before: `2q2bk1/3n1pp1/4N2p/8/1p1NP3/pP1P4/P4PPP/K4B1R w - - 1 27`

- Engine re-search: `e6c7` score `-437`
- Engine re-search PV: `e6c7 c8c7 d4c6 c7c6 a1b1`

- Engine played-move constrained score: `-437`
- Engine played-move PV: `e6c7 c8c7 d4c6 c7c6 a1b1`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-437`, delta `+0`
- Engine reference-move PV: `e6c7 c8c7 d4c6 c7c6 a1b1`
- Engine reference-move SEE: `0`

- Reference bestmove: `e6c7` score `-899996`
- Reference PV: `e6c7 c8c7 d4c6 c7c6 a1b1 c6c3 f2f3 c3b2`

### 15. Game 14, ply 53: c4f1 (Qxf1+)

- Categories: `engine-loss, opponent-recapture, recapture-trade, queen-trade-sequence, bad-trade-sequence, played-negative-see, reference-negative-see`
- Score: `-716 -> -250` for current-eval, delta `+466`
- Sequence after reply `a1f1`: `-716 -> -1105`, delta `-389`
- Engine recaptures available: `none`
- Material: `Q` captures `R` for `500` cp
- Component movement: threats -86, piece_square -6, center_control -6, pawn_dynamics -4, king_safety +32, material +500
- Result: `1-0` `white_win:checkmate`
- FEN before: `r5k1/pp2Q2p/4pppB/2pp4/2q5/2P2B1P/5PP1/R4RK1 b - - 0 30`

- Engine re-search: `c4f1` score `-899996`
- Engine re-search PV: `c4f1 g1f1 a7a6 e7g7`

- Engine played-move constrained score: `-899996`
- Engine played-move PV: `c4f1 g1f1 a7a6 e7g7`
- Engine played-move SEE: `-400`
- Engine reference-move constrained score: `-899996`, delta `+0`
- Engine reference-move PV: `c4f1 a1f1 a7a6 e7g7`
- Engine reference-move SEE: `-400`

- Reference bestmove: `c4f1` score `-899998`
- Reference PV: `c4f1 a1f1 b7b6 e7g7`

### 16. Game 15, ply 60: c1c2 (Rc2)

- Categories: `engine-loss, opponent-recapture, recapture-trade, bad-trade-sequence`
- Score: `-630 -> -609` for current-eval, delta `+21`
- Sequence after reply `b2c2`: `-630 -> -1107`, delta `-477`
- Engine recaptures available: `none`
- Material: `R` captures `-` for `0` cp
- Component movement: mobility +6, threats +6
- Result: `0-1` `black_win:checkmate`
- FEN before: `5k2/5p1p/3p1qp1/p1p5/2Pn4/3P1P2/1r3PPP/2RK1B1R w - - 1 31`

- Engine re-search: `c1c2` score `-1765`
- Engine re-search PV: `c1c2 b2c2 g2g3 f6f3 d1e1`

- Engine played-move constrained score: `-1765`
- Engine played-move PV: `c1c2 b2c2 g2g3 f6f3 d1e1`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-1765`, delta `+0`
- Engine reference-move PV: `c1c2 b2c2 g2g3 f6f3 d1e1`
- Engine reference-move SEE: `0`

- Reference bestmove: `c1c2` score `-899996`
- Reference PV: `c1c2 b2c2 f1e2 f6f4 d1e1 c2e2 e1d1 f4d2`

### 17. Game 41, ply 55: d4c6 (Nc6)

- Categories: `engine-loss, opponent-recapture, recapture-trade, queen-trade-sequence, bad-trade-sequence`
- Score: `-30 -> -29` for current-eval, delta `+1`
- Sequence after reply `c7c6`: `-30 -> -414`, delta `-384`
- Engine recaptures available: `none`
- Material: `N` captures `-` for `0` cp
- Component movement: threats -16, piece_square -8, center_control +6, space +9
- Result: `0-1` `black_win:checkmate`
- FEN before: `5bk1/2qn1pp1/7p/8/1p1NP3/pP1P4/P4PPP/K4B1R w - - 0 28`

- Engine re-search: `d4c6` score `-819643`
- Engine re-search PV: `d4c6 c7c6 a1b1 c6c3 d3d4`

- Engine played-move constrained score: `-819751`
- Engine played-move PV: `d4c6 c7c6 a1b1 c6c3 d3d4`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-819643`, delta `+108`
- Engine reference-move PV: `d4c6 c7c6 a1b1 c6c3 f1e2`
- Engine reference-move SEE: `0`

- Reference bestmove: `d4c6` score `-899997`
- Reference PV: `d4c6 c7c6 a1b1 c6c3 f2f3 c3b2`

### 18. Game 33, ply 48: c6e8 (Qxe8+)

- Categories: `opponent-recapture, recapture-trade, queen-trade-sequence, bad-trade-sequence, played-negative-see, engine-prefers-negative-see`
- Score: `+1236 -> +1737` for current-eval, delta `+501`
- Sequence after reply `f7e8`: `+1236 -> +848`, delta `-388`
- Engine recaptures available: `none`
- Material: `Q` captures `R` for `500` cp
- Component movement: threats -54, piece_square -3, pawn_dynamics -2, mobility +22, material +500
- Result: `1-0` `white_win:checkmate`
- FEN before: `4r3/5kPp/p1Qp4/2pP1B2/2Pp1b2/N5P1/PP5P/6K1 w - - 1 28`

- Engine re-search: `c6e8` score `2235`
- Engine re-search PV: `c6e8 f7e8 g7g8q e8e7 g3f4`

- Engine played-move constrained score: `2235`
- Engine played-move PV: `c6e8 f7e8 g7g8q e8e7 g3f4`
- Engine played-move SEE: `-400`
- Engine reference-move constrained score: `1695`, delta `-540`
- Engine reference-move PV: `c6d7 f7f6 g3f4 e8g8 d7d6`
- Engine reference-move SEE: `0`

- Reference bestmove: `c6d7` score `1053`
- Reference PV: `c6d7 e8e7 g7g8q f7g8 d7e7 h7h5 e7e8 g8g7 e8h5 f4e3 g1f1`
- Reference played-move score: `899994`, delta `-898941`
- Reference played-move PV: `f7e8 g7g8q e8e7 g8g7 e7e8 f5e6 f4e3 g1g2 d4d3 g7f7 e8d8 f7d7`

### 19. Game 55, ply 36: a5a7 (Qxa7)

- Categories: `engine-loss, opponent-recapture, recapture-trade, queen-trade-sequence, bad-trade-sequence, played-negative-see, reference-negative-see`
- Score: `-113 -> +455` for current-eval, delta `+568`
- Sequence after reply `b8a7`: `-113 -> -471`, delta `-358`
- Engine recaptures available: `none`
- Material: `Q` captures `R` for `500` cp
- Component movement: piece_square -7, king_safety -5, pawn_dynamics -2, pawn_structure -1, threats +38, material +500
- Result: `0-1` `black_win:checkmate`
- FEN before: `1q2k2r/r2bbpp1/4p2p/Q1pnp3/8/3P4/PP1BPPPP/1K1R1B1R w k - 0 19`

- Engine re-search: `a5a7` score `-493`
- Engine re-search PV: `a5a7 b8a7 d1c1 e8g8 c1c4`

- Engine played-move constrained score: `-493`
- Engine played-move PV: `a5a7 b8a7 d1c1 e8g8 c1c4`
- Engine played-move SEE: `-400`
- Engine reference-move constrained score: `-493`, delta `+0`
- Engine reference-move PV: `a5a7 b8a7 d1c1 e8g8 c1c4`
- Engine reference-move SEE: `-400`

- Reference bestmove: `a5a7` score `-631`
- Reference PV: `a5a7 b8a7 e2e4 d5f6 d3d4 c5d4 d1e1`

### 20. Game 14, ply 49: a4d7 (Bd7)

- Categories: `engine-loss, opponent-recapture, recapture-trade, queen-trade-sequence, bad-trade-sequence`
- Score: `+150 -> +189` for current-eval, delta `+39`
- Sequence after reply `c7d7`: `+150 -> -205`, delta `-355`
- Engine recaptures available: `none`
- Material: `B` captures `-` for `0` cp
- Component movement: threats -19, bishop_quality -14, mobility -7, safe_mobility -3, trade_context +24, king_safety +49
- Result: `1-0` `white_win:checkmate`
- FEN before: `r3r1k1/ppQ4p/4pppB/2pp4/b1q5/2P2B1P/5PP1/R4RK1 b - - 1 28`

- Engine re-search: `c4f1` score `-871055`
- Engine re-search PV: `c4f1 a1f1 a4d7 c7c5 e8e7`

- Engine played-move constrained score: `-899992`
- Engine played-move PV: `a4d7 c7d7 e8e7 d7e7 c4f1`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-899992`, delta `+0`
- Engine reference-move PV: `a4d7 c7d7 c4f1 a1f1 e8e7`
- Engine reference-move SEE: `0`

- Reference bestmove: `a4d7` score `-899996`
- Reference PV: `a4d7 c7d7 c4f1 a1f1 e8e7 d7e7 b7b6 e7g7`

### 21. Game 35, ply 52: a1d1 (Rd1+)

- Categories: `opponent-recapture, recapture-trade, bad-trade-sequence`
- Score: `+701 -> +751` for current-eval, delta `+50`
- Sequence after reply `f3d1`: `+701 -> +234`, delta `-467`
- Engine recaptures available: `none`
- Material: `R` captures `-` for `0` cp
- Component movement: threats -51, rook_files +26, king_safety +38
- Result: `1-0` `white_win:checkmate`
- FEN before: `3k4/1pp2p2/8/4RP1p/p4B2/2P2b2/PP3PrP/R4K2 w - - 6 27`

- Engine re-search: `e5e3` score `893`
- Engine re-search PV: `e5e3 f3c6 a1d1 d8c8 e3e7`

- Engine played-move constrained score: `777`
- Engine played-move PV: `a1d1 f3d1 f1g2 d1c2 f5f6`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `893`, delta `+116`
- Engine reference-move PV: `e5e3 f3c6 a1d1 d8c8 e3e7`
- Engine reference-move SEE: `0`

- Reference bestmove: `e5e3` score `602`
- Reference PV: `e5e3 f3c6 f2f3 g2b2 c3c4 c6d7`
- Reference played-move score: `617`, delta `-15`
- Reference played-move PV: `f3d1 f1g2 a4a3 b2a3 d1c2 e5e1 c2f5`

### 22. Game 31, ply 82: c1c2 (Rxc2)

- Categories: `engine-loss, opponent-recapture, recapture-trade, queen-trade-sequence, bad-trade-sequence, played-negative-see, reference-negative-see`
- Score: `-437 -> -201` for current-eval, delta `+236`
- Sequence after reply `b2c2`: `-437 -> -788`, delta `-351`
- Engine recaptures available: `none`
- Material: `R` captures `P` for `100` cp
- Component movement: threats +72, material +100
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/k7/p7/1p2P1P1/3P4/7P/1qp1bK2/2R5 w - - 0 43`

- Engine re-search: `c1c2` score `-1019`
- Engine re-search PV: `c1c2 b2c2 h3h4 e2c4 f2f3`

- Engine played-move constrained score: `-1019`
- Engine played-move PV: `c1c2 b2c2 h3h4 e2c4 f2f3`
- Engine played-move SEE: `-400`
- Engine reference-move constrained score: `-1019`, delta `+0`
- Engine reference-move PV: `c1c2 b2c2 h3h4 e2c4 f2f3`
- Engine reference-move SEE: `-400`

- Reference bestmove: `c1c2` score `-592`
- Reference PV: `c1c2 b2d4 f2g2 d4d5 g2h2 d5e5 h2g2 e5e4 g2g1 e4c2`

### 23. Game 46, ply 29: g6d3 (Bd3+)

- Categories: `engine-loss, opponent-recapture, recapture-trade, bad-trade-sequence`
- Score: `+837 -> +871` for current-eval, delta `+34`
- Sequence after reply `b5d3`: `+837 -> +404`, delta `-433`
- Engine recaptures available: `none`
- Material: `B` captures `-` for `0` cp
- Component movement: bishop_quality +7, king_safety +15
- Result: `1-0` `white_win:checkmate`
- FEN before: `2kr1b1r/Qpp1ppp1/5nbp/1BN5/3P2n1/7N/PP2KP1P/R1B4q b - - 1 16`

- Engine re-search: `g6d3` score `48`
- Engine re-search PV: `g6d3 b5d3 b7b5 a7a6 c8b8`

- Engine played-move constrained score: `48`
- Engine played-move PV: `g6d3 b5d3 b7b5 a7a6 c8b8`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `48`, delta `+0`
- Engine reference-move PV: `g6d3 b5d3 b7b5 a7a6 c8b8`
- Engine reference-move SEE: `0`

- Reference bestmove: `g6d3` score `-566`
- Reference PV: `g6d3 b5d3 b7b5 d3e4 h1e4 c5e4 e7e6 a7a6 c8b8 a6b5 b8c8 b5a6 c8b8`

### 24. Game 29, ply 101: c3b2 (Bb2)

- Categories: `engine-loss, opponent-recapture, recapture-trade, bad-trade-sequence`
- Score: `-362 -> -384` for current-eval, delta `-22`
- Sequence after reply `b1b2`: `-362 -> -743`, delta `-381`
- Engine recaptures available: `none`
- Material: `B` captures `-` for `0` cp
- Component movement: pawn_structure -37, threats -10, piece_square -4, space -1, mobility +8, king_safety +19
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/3k4/8/2KP3P/q4P2/2B5/3P3P/1r6 w - - 1 53`

- Engine re-search: `c3b2` score `-899996`
- Engine re-search PV: `c3b2 b1b2 h2h3 b2b5`

- Engine played-move constrained score: `-899996`
- Engine played-move PV: `c3b2 b1b2 h2h3 b2b5`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-899996`, delta `+0`
- Engine reference-move PV: `c3b2 b1b2 d2d3 b2b5`
- Engine reference-move SEE: `0`

- Reference bestmove: `c3b2` score `-899998`
- Reference PV: `c3b2 b1b2 d5d6 b2b5`

### 25. Game 28, ply 100: f6e7 (Be7+)

- Categories: `engine-loss, opponent-recapture, recapture-trade, bad-trade-sequence`
- Score: `-667 -> -700` for current-eval, delta `-33`
- Sequence after reply `a7e7`: `-667 -> -1033`, delta `-366`
- Engine recaptures available: `none`
- Material: `B` captures `-` for `0` cp
- Component movement: pawn_structure -31, threats -17, piece_square -8, center_control -1, mobility +7, king_safety +11
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/R5p1/p2K1b2/2Q1p3/k5p1/8/P7/8 b - - 5 51`

- Engine re-search: `f6e7` score `-899996`
- Engine re-search PV: `f6e7 d6e7 a6a5 a7a5`

- Engine played-move constrained score: `-899996`
- Engine played-move PV: `f6e7 d6e7 a6a5 a7a5`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-899996`, delta `+0`
- Engine reference-move PV: `f6e7 d6e7 a6a5 a7a5`
- Engine reference-move SEE: `0`

- Reference bestmove: `f6e7` score `-899998`
- Reference PV: `f6e7 d6e7 g4g3 a7a6`


## Negative SEE Capture Audit

### 1. Game 60, ply 38: d4d7 (Qxd7)

- Categories: `engine-loss, opponent-recapture, recapture-trade, queen-trade-sequence, bad-trade-sequence, played-negative-see, reference-negative-see`
- Score: `+495 -> +921` for current-eval, delta `+426`
- Sequence after reply `c6d7`: `+495 -> -113`, delta `-608`
- Engine recaptures available: `none`
- Material: `Q` captures `N` for `320` cp
- Component movement: space -6, piece_square -5, center_control -1, development -1, threats +34, material +320
- Result: `1-0` `white_win:checkmate`
- FEN before: `rk3b1r/p2Np1pp/2Q5/8/2Nq3P/7R/P2nPP2/4K3 b - - 2 21`

- Engine re-search: `d4d7` score `-178`
- Engine re-search PV: `d4d7 c6d7 d2c4 h3c3 e7e5`

- Engine played-move constrained score: `-178`
- Engine played-move PV: `d4d7 c6d7 d2c4 h3c3 e7e5`
- Engine played-move SEE: `-580`
- Engine reference-move constrained score: `-178`, delta `+0`
- Engine reference-move PV: `d4d7 c6d7 d2c4 h3c3 e7e5`
- Engine reference-move SEE: `-580`

- Reference bestmove: `d4d7` score `-686`
- Reference PV: `d4d7 c6d7 e7e6 c4a5 d2f3 e1f1 f3d2 f1g1`

### 2. Game 65, ply 20: a4b4 (Qxb4)

- Categories: `opponent-recapture, recapture-trade, queen-trade-sequence, bad-trade-sequence, played-negative-see, engine-can-avoid-negative-see`
- Score: `+347 -> +777` for current-eval, delta `+430`
- Sequence after reply `a6b4`: `+347 -> -179`, delta `-526`
- Engine recaptures available: `none`
- Material: `Q` captures `B` for `330` cp
- Component movement: threats -146, rook_files -1, pawn_dynamics -1, king_safety +99, material +330
- Result: `1-0` `white_win:checkmate`
- FEN before: `r1bqk2r/p2n2pp/n3Np2/1N2p3/Qbp5/4B3/PP2PPPP/3RKB1R w Kkq - 1 12`

- Engine re-search: `b5c3` score `434`
- Engine re-search PV: `b5c3 d8a5 e6g7 e8f8 g7e6`

- Engine played-move constrained score: `277`
- Engine played-move PV: `a4b4 a6b4 b5c7 e8e7 e6d8`
- Engine played-move SEE: `-570`
- Engine reference-move constrained score: `410`, delta `+133`
- Engine reference-move PV: `e3d2 d8b6 e6g7 e8f8 d2b4`
- Engine reference-move SEE: `0`

- Reference bestmove: `e3d2` score `396`
- Reference PV: `e3d2 d8e7 e6c7 e8f8 d2b4 a6b4 a2a3 a8b8 a3b4`
- Reference played-move score: `325`, delta `+71`
- Reference played-move PV: `a6b4 b5c7 e8e7 e6d8 b4c2 e1d2 c2e3 d2e3 h8d8 c7a8 f6f5 f2f4 c8b7 a8c7 d7b6`

### 3. Game 93, ply 32: c5f8 (Qxf8+)

- Categories: `engine-loss, opponent-recapture, recapture-trade, queen-trade-sequence, bad-trade-sequence, played-negative-see, reference-negative-see`
- Score: `+862 -> +1351` for current-eval, delta `+489`
- Sequence after reply `g7f8`: `+862 -> +396`, delta `-466`
- Engine recaptures available: `none`
- Material: `Q` captures `R` for `500` cp
- Component movement: threats -81, piece_square -8, center_control -8, king_safety +23, material +500
- Result: `0-1` `black_win:checkmate`
- FEN before: `5r2/2p2Pkp/2n3p1/2Q5/2P1P3/2P2bPq/P4P1P/RB3RK1 w - - 1 20`

- Engine re-search: `c5f8` score `-899996`
- Engine re-search PV: `c5f8 g7f8 f1d1 h3g2`

- Engine played-move constrained score: `-899996`
- Engine played-move PV: `c5f8 g7f8 f1d1 h3g2`
- Engine played-move SEE: `-400`
- Engine reference-move constrained score: `-899996`, delta `+0`
- Engine reference-move PV: `c5f8 g7f8 f1d1 h3g2`
- Engine reference-move SEE: `-400`

- Reference bestmove: `c5f8` score `-899998`
- Reference PV: `c5f8 g7f8 a2a3 h3g2`

### 4. Game 14, ply 53: c4f1 (Qxf1+)

- Categories: `engine-loss, opponent-recapture, recapture-trade, queen-trade-sequence, bad-trade-sequence, played-negative-see, reference-negative-see`
- Score: `-716 -> -250` for current-eval, delta `+466`
- Sequence after reply `a1f1`: `-716 -> -1105`, delta `-389`
- Engine recaptures available: `none`
- Material: `Q` captures `R` for `500` cp
- Component movement: threats -86, piece_square -6, center_control -6, pawn_dynamics -4, king_safety +32, material +500
- Result: `1-0` `white_win:checkmate`
- FEN before: `r5k1/pp2Q2p/4pppB/2pp4/2q5/2P2B1P/5PP1/R4RK1 b - - 0 30`

- Engine re-search: `c4f1` score `-899996`
- Engine re-search PV: `c4f1 g1f1 a7a6 e7g7`

- Engine played-move constrained score: `-899996`
- Engine played-move PV: `c4f1 g1f1 a7a6 e7g7`
- Engine played-move SEE: `-400`
- Engine reference-move constrained score: `-899996`, delta `+0`
- Engine reference-move PV: `c4f1 a1f1 a7a6 e7g7`
- Engine reference-move SEE: `-400`

- Reference bestmove: `c4f1` score `-899998`
- Reference PV: `c4f1 a1f1 b7b6 e7g7`

### 5. Game 33, ply 48: c6e8 (Qxe8+)

- Categories: `opponent-recapture, recapture-trade, queen-trade-sequence, bad-trade-sequence, played-negative-see, engine-prefers-negative-see`
- Score: `+1236 -> +1737` for current-eval, delta `+501`
- Sequence after reply `f7e8`: `+1236 -> +848`, delta `-388`
- Engine recaptures available: `none`
- Material: `Q` captures `R` for `500` cp
- Component movement: threats -54, piece_square -3, pawn_dynamics -2, mobility +22, material +500
- Result: `1-0` `white_win:checkmate`
- FEN before: `4r3/5kPp/p1Qp4/2pP1B2/2Pp1b2/N5P1/PP5P/6K1 w - - 1 28`

- Engine re-search: `c6e8` score `2235`
- Engine re-search PV: `c6e8 f7e8 g7g8q e8e7 g3f4`

- Engine played-move constrained score: `2235`
- Engine played-move PV: `c6e8 f7e8 g7g8q e8e7 g3f4`
- Engine played-move SEE: `-400`
- Engine reference-move constrained score: `1695`, delta `-540`
- Engine reference-move PV: `c6d7 f7f6 g3f4 e8g8 d7d6`
- Engine reference-move SEE: `0`

- Reference bestmove: `c6d7` score `1053`
- Reference PV: `c6d7 e8e7 g7g8q f7g8 d7e7 h7h5 e7e8 g8g7 e8h5 f4e3 g1f1`
- Reference played-move score: `899994`, delta `-898941`
- Reference played-move PV: `f7e8 g7g8q e8e7 g8g7 e7e8 f5e6 f4e3 g1g2 d4d3 g7f7 e8d8 f7d7`

### 6. Game 55, ply 36: a5a7 (Qxa7)

- Categories: `engine-loss, opponent-recapture, recapture-trade, queen-trade-sequence, bad-trade-sequence, played-negative-see, reference-negative-see`
- Score: `-113 -> +455` for current-eval, delta `+568`
- Sequence after reply `b8a7`: `-113 -> -471`, delta `-358`
- Engine recaptures available: `none`
- Material: `Q` captures `R` for `500` cp
- Component movement: piece_square -7, king_safety -5, pawn_dynamics -2, pawn_structure -1, threats +38, material +500
- Result: `0-1` `black_win:checkmate`
- FEN before: `1q2k2r/r2bbpp1/4p2p/Q1pnp3/8/3P4/PP1BPPPP/1K1R1B1R w k - 0 19`

- Engine re-search: `a5a7` score `-493`
- Engine re-search PV: `a5a7 b8a7 d1c1 e8g8 c1c4`

- Engine played-move constrained score: `-493`
- Engine played-move PV: `a5a7 b8a7 d1c1 e8g8 c1c4`
- Engine played-move SEE: `-400`
- Engine reference-move constrained score: `-493`, delta `+0`
- Engine reference-move PV: `a5a7 b8a7 d1c1 e8g8 c1c4`
- Engine reference-move SEE: `-400`

- Reference bestmove: `a5a7` score `-631`
- Reference PV: `a5a7 b8a7 e2e4 d5f6 d3d4 c5d4 d1e1`

### 7. Game 31, ply 82: c1c2 (Rxc2)

- Categories: `engine-loss, opponent-recapture, recapture-trade, queen-trade-sequence, bad-trade-sequence, played-negative-see, reference-negative-see`
- Score: `-437 -> -201` for current-eval, delta `+236`
- Sequence after reply `b2c2`: `-437 -> -788`, delta `-351`
- Engine recaptures available: `none`
- Material: `R` captures `P` for `100` cp
- Component movement: threats +72, material +100
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/k7/p7/1p2P1P1/3P4/7P/1qp1bK2/2R5 w - - 0 43`

- Engine re-search: `c1c2` score `-1019`
- Engine re-search PV: `c1c2 b2c2 h3h4 e2c4 f2f3`

- Engine played-move constrained score: `-1019`
- Engine played-move PV: `c1c2 b2c2 h3h4 e2c4 f2f3`
- Engine played-move SEE: `-400`
- Engine reference-move constrained score: `-1019`, delta `+0`
- Engine reference-move PV: `c1c2 b2c2 h3h4 e2c4 f2f3`
- Engine reference-move SEE: `-400`

- Reference bestmove: `c1c2` score `-592`
- Reference PV: `c1c2 b2d4 f2g2 d4d5 g2h2 d5e5 h2g2 e5e4 g2g1 e4c2`

### 8. Game 73, ply 64: h1h7 (Rxh7+)

- Categories: `opponent-recapture, recapture-trade, bad-trade-sequence, played-negative-see, engine-prefers-negative-see`
- Score: `+1505 -> +1662` for current-eval, delta `+157`
- Sequence after reply `g7h7`: `+1505 -> +1110`, delta `-395`
- Engine recaptures available: `none`
- Material: `R` captures `P` for `100` cp
- Component movement: threats -28, pawn_structure -18, pawn_dynamics -10, mobility -4, piece_square +61, material +100
- Result: `1-0` `white_win:checkmate`
- FEN before: `1r6/1Q3Nkp/p7/4b3/2P5/8/PP3PP1/1K5R w - - 1 36`

- Engine re-search: `h1h7` score `1599`
- Engine re-search PV: `h1h7 g7h7 b7e4 h7g8 f7h6`

- Engine played-move constrained score: `1599`
- Engine played-move PV: `h1h7 g7h7 b7e4 h7g8 f7h6`
- Engine played-move SEE: `-400`
- Engine reference-move constrained score: `1471`, delta `-128`
- Engine reference-move PV: `b7d7 b8b2 b1c1 e5c3 f7d6`
- Engine reference-move SEE: `0`

- Reference bestmove: `b7d7` score `853`
- Reference PV: `b7d7 b8b2 b1c1 b2f2 f7g5 g7g6 g5e4 e5f4 c1b1 h7h6 d7e6 g6h7 e6f7 h7h8 f7f8 h8h7`
- Reference played-move score: `3535`, delta `-2682`
- Reference played-move PV: `g7f8 f7e5 b8b7 h7b7 f8g8 b7a7 g8h8 e5f7 h8g8 a7a6 g8f7`

### 9. Game 25, ply 16: b7b6 (Qxb6)

- Categories: `opponent-recapture, recapture-trade, queen-trade-sequence, bad-trade-sequence, played-negative-see, engine-prefers-negative-see`
- Score: `+546 -> +1033` for current-eval, delta `+487`
- Sequence after reply `c7b6`: `+546 -> +260`, delta `-286`
- Engine recaptures available: `none`
- Material: `Q` captures `R` for `500` cp
- Component movement: threats -37, space -6, king_safety -5, center_control -3, mobility +15, material +500
- Result: `1/2-1/2` `draw:threefold_repetition`
- FEN before: `1n1qk2r/1Qpb1ppp/1r3n2/p7/1B1P4/8/PP2PPPP/RN2KBNR w KQk - 1 10`

- Engine re-search: `b7b6` score `271`
- Engine re-search PV: `b7b6 c7b6 b4d6 d7f5 d6e5`

- Engine played-move constrained score: `271`
- Engine played-move PV: `b7b6 c7b6 b4d6 d7f5 d6e5`
- Engine played-move SEE: `-400`
- Engine reference-move constrained score: `240`, delta `-31`
- Engine reference-move PV: `b7f3 b6b4 b1d2 e8g8 d2b3`
- Engine reference-move SEE: `0`

- Reference bestmove: `b7f3` score `104`
- Reference PV: `b7f3 b6b4 f3c3 e8g8 a2a3 f6e4 c3e3 b4c4 g1f3 d7f5 b1c3 f8e8 c3e4 f5e4 e3b3`
- Reference played-move score: `-67`, delta `+171`
- Reference played-move PV: `c7b6 b4d2 d8e7 g1f3 b8c6 e2e3 g7g5 f1e2 g5g4 f3e5 c6e5`

### 10. Game 45, ply 38: c6c8 (Qxc8+)

- Categories: `engine-loss, recapture-trade, opponent-recapture, queen-trade-sequence, bad-trade-sequence, played-negative-see, reference-negative-see`
- Score: `+370 -> +1043` for current-eval, delta `+673`
- Sequence after reply `f5c8`: `+370 -> +110`, delta `-260`
- Engine recaptures available: `none`
- Material: `Q` captures `R` for `500` cp
- Component movement: piece_square -6, center_control -2, pawn_dynamics -2, threats +92, material +500
- Result: `0-1` `black_win:checkmate`
- FEN before: `2r3k1/p4ppp/2Q2n2/5q2/3p4/8/PP1NPPPP/4KB1R w K - 1 21`

- Engine re-search: `c6c8` score `28`
- Engine re-search PV: `c6c8 f5c8 e1d1 c8a6 a2a3`

- Engine played-move constrained score: `28`
- Engine played-move PV: `c6c8 f5c8 e1d1 c8a6 a2a3`
- Engine played-move SEE: `-400`
- Engine reference-move constrained score: `28`, delta `+0`
- Engine reference-move PV: `c6c8 f5c8 e1d1 c8a6 a2a3`
- Engine reference-move SEE: `-400`

- Reference bestmove: `c6c8` score `-470`
- Reference PV: `c6c8 f5c8 d2b3 c8c2 f2f3 c2b1 e1f2 b1b2`

### 11. Game 8, ply 52: c8d8 (Qxd8)

- Categories: `engine-loss, opponent-recapture, recapture-trade, queen-trade-sequence, bad-trade-sequence, played-negative-see, reference-negative-see`
- Score: `-165 -> +448` for current-eval, delta `+613`
- Sequence after reply `d6d8`: `-165 -> -423`, delta `-258`
- Engine recaptures available: `none`
- Material: `Q` captures `R` for `500` cp
- Component movement: trade_context -8, pawn_dynamics -1, threats +43, material +500
- Result: `1-0` `white_win:checkmate`
- FEN before: `rkqR4/p1r3pp/3Q4/3Bp3/P7/2P5/2P2P1P/5K2 b - - 2 27`

- Engine re-search: `c8d8` score `-997`
- Engine re-search PV: `c8d8 d6d8 c7c8 d8d6 c8c7`

- Engine played-move constrained score: `-997`
- Engine played-move PV: `c8d8 d6d8 c7c8 d8d6 c8c7`
- Engine played-move SEE: `-400`
- Engine reference-move constrained score: `-997`, delta `+0`
- Engine reference-move PV: `c8d8 d6d8 c7c8 d8d6 c8c7`
- Engine reference-move SEE: `-400`

- Reference bestmove: `c8d8` score `-899996`
- Reference PV: `c8d8 d6d8 c7c8 d8d7 c8c7 d7b5 b8c8 b5e8`

### 12. Game 7, ply 31: h7f6 (Nxf6+)

- Categories: `engine-loss, opponent-recapture, recapture-trade, bad-trade-sequence, played-negative-see`
- Score: `+429 -> +813` for current-eval, delta `+384`
- Sequence after reply `d8f6`: `+429 -> +161`, delta `-268`
- Engine recaptures available: `none`
- Material: `N` captures `P` for `100` cp
- Component movement: bishop_quality -11, center_control -2, king_safety +79, material +100
- Result: `0-1` `black_win:checkmate`
- FEN before: `r1bbq1kr/6pN/2pp1pn1/p6Q/2p1PN2/3P2B1/PPP3PP/R4RK1 w - - 4 17`

- Engine re-search: `h5g6` score `426`
- Engine re-search PV: `h5g6 e8g6 f4g6 h8h7 g3d6`

- Engine played-move constrained score: `409`
- Engine played-move PV: `h7f6 g7f6 h5g6 e8g6 f4g6`
- Engine played-move SEE: `-220`
- Engine reference-move constrained score: `426`, delta `+17`
- Engine reference-move PV: `h5g6 e8g6 f4g6 h8h7 g3d6`
- Engine reference-move SEE: `320`

- Reference bestmove: `h5g6` score `155`
- Reference PV: `h5g6 e8g6 f4g6 h8h7 d3c4 h7h6 g6f4 f6f5 e4f5 c8f5 f4d3`
- Reference played-move score: `130`, delta `+25`
- Reference played-move PV: `d8f6 h5g6 e8g6 f4g6 h8h6 e4e5 d6e5 g6e5 c4d3 c2d3 c8e6 a2a3 e6d5 a1d1`

### 13. Game 59, ply 35: f1c4 (Bxc4+)

- Categories: `recapture-trade, opponent-recapture, bad-trade-sequence, played-negative-see, reference-negative-see`
- Score: `+1870 -> +2105` for current-eval, delta `+235`
- Sequence after reply `b6c4`: `+1870 -> +1584`, delta `-286`
- Engine recaptures available: `none`
- Material: `B` captures `P` for `100` cp
- Component movement: threats -17, king_safety +52, material +100
- Result: `1-0` `white_win:checkmate`
- FEN before: `2N2b1r/1b3kpp/1n6/P7/2p1P3/1p2B3/1P3PPP/R2QKB1R w KQ - 0 20`

- Engine re-search: `f1c4` score `2350`
- Engine re-search PV: `f1c4 b6c4 d1d7 f8e7 d7e7`

- Engine played-move constrained score: `2350`
- Engine played-move PV: `f1c4 b6c4 d1d7 f8e7 d7e7`
- Engine played-move SEE: `-230`
- Engine reference-move constrained score: `2350`, delta `+0`
- Engine reference-move PV: `f1c4 b6c4 d1d7 f8e7 d7e7`
- Engine reference-move SEE: `-230`

- Reference bestmove: `f1c4` score `1326`
- Reference PV: `f1c4 b7d5 c4d5 b6d5 d1g4 d5e3 f2e3`

### 14. Game 50, ply 18: c6d4 (Nxd4)

- Categories: `engine-loss, opponent-recapture, recapture-trade, bad-trade-sequence, played-negative-see`
- Score: `+86 -> +373` for current-eval, delta `+287`
- Sequence after reply `f3d4`: `+86 -> -147`, delta `-233`
- Engine recaptures available: `none`
- Material: `N` captures `P` for `100` cp
- Component movement: bishop_quality -11, material +100, threats +133
- Result: `1-0` `white_win:checkmate`
- FEN before: `r1bq1k1r/ppp2ppp/2nbp3/8/2PP4/5N2/PP2QPPP/RNB2RK1 b - - 0 11`

- Engine re-search: `d8f6` score `82`
- Engine re-search PV: `d8f6 f1d1 f6g6 f3e5 g6f5`

- Engine played-move constrained score: `64`
- Engine played-move PV: `c6d4 f3d4 d6h2 g1h2 d8d6`
- Engine played-move SEE: `-220`
- Engine reference-move constrained score: `81`, delta `+17`
- Engine reference-move PV: `f8g8 b1c3 c6d4 f3d4 d6h2`
- Engine reference-move SEE: `0`

- Reference bestmove: `f8g8` score `-126`
- Reference PV: `f8g8 b1c3 b7b6 f1d1 h7h6 a2a3 a7a5 e2e4 c8b7 c4c5 d6f8 c1f4 b6c5 d4c5`
- Reference played-move score: `-425`, delta `+299`
- Reference played-move PV: `f3d4 b7b6 d4b5 d6h2 g1h2 c8b7 h2g1 a7a6 b5c3 b7g2`

### 15. Game 80, ply 74: e4f4 (Rxf4+)

- Categories: `engine-loss, opponent-recapture, recapture-trade, bad-trade-sequence, played-negative-see, engine-can-avoid-negative-see`
- Score: `+684 -> +1079` for current-eval, delta `+395`
- Sequence after reply `f3f4`: `+684 -> +471`, delta `-213`
- Engine recaptures available: `none`
- Material: `R` captures `B` for `330` cp
- Component movement: rook_files -16, pawn_dynamics -2, center_control -1, mobility +21, material +330
- Result: `1-0` `white_win:checkmate`
- FEN before: `1k6/pp3ppp/8/P2pr1PR/3prB2/5K2/R7/8 b - - 5 39`

- Engine re-search: `h7h6` score `393`
- Engine re-search PV: `h7h6 f4e5 e4e5 g5h6 e5h5`

- Engine played-move constrained score: `353`
- Engine played-move PV: `e4f4 f3f4 e5e4 f4f3 e4e3`
- Engine played-move SEE: `-170`
- Engine reference-move constrained score: `393`, delta `+40`
- Engine reference-move PV: `h7h6 f4e5 e4e5 g5h6 e5h5`
- Engine reference-move SEE: `0`

- Reference bestmove: `h7h6` score `-267`
- Reference PV: `h7h6 f4e5 e4e5 h5h4 h6g5 h4d4 b8c7 a5a6`
- Reference played-move score: `-316`, delta `+49`
- Reference played-move PV: `f3f4 e5e4 f4f3 e4e3 f3f2 e3e5 h5h7 e5g5 a2d2 b8c7 d2d4 a7a6`

### 16. Game 5, ply 28: c4e6 (Bxe6)

- Categories: `engine-loss, opponent-recapture, recapture-trade, bad-trade-sequence, played-negative-see, engine-prefers-negative-see`
- Score: `+381 -> +629` for current-eval, delta `+248`
- Sequence after reply `d7e6`: `+381 -> +172`, delta `-209`
- Engine recaptures available: `none`
- Material: `B` captures `P` for `100` cp
- Component movement: bishop_quality -3, pawn_dynamics -3, king_safety +39, material +100
- Result: `0-1` `black_win:checkmate`
- FEN before: `2r1k1nr/2Bbb3/p3pp2/1p4pp/2BP4/2N1P3/PP3PPP/R3K2R w KQk - 1 16`

- Engine re-search: `c4e6` score `146`
- Engine re-search PV: `c4e6 d7e6 d4d5 e7b4 d5d6`

- Engine played-move constrained score: `146`
- Engine played-move PV: `c4e6 d7e6 d4d5 e7b4 d5d6`
- Engine played-move SEE: `-230`
- Engine reference-move constrained score: `56`, delta `-90`
- Engine reference-move PV: `c4b3 c8c7 e1g1 e8f8 a1c1`
- Engine reference-move SEE: `0`

- Reference bestmove: `c4b3` score `-334`
- Reference PV: `c4b3 c8c7 d4d5 e6e5 a2a4 c7c8 h2h4`
- Reference played-move score: `-347`, delta `+13`
- Reference played-move PV: `d7e6 c7a5 f6f5 d4d5 e6d7 h2h4 g5g4 e3e4 f5e4 d5d6 e7d6`

### 17. Game 31, ply 30: d3c4 (Bxc4)

- Categories: `engine-loss, opponent-recapture, recapture-trade, bad-trade-sequence, played-negative-see, engine-can-avoid-negative-see`
- Score: `+9 -> +99` for current-eval, delta `+90`
- Sequence after reply `d5c4`: `+9 -> -199`, delta `-208`
- Engine recaptures available: `none`
- Material: `B` captures `P` for `100` cp
- Component movement: threats -30, space -4, bishop_quality -4, mobility -3, king_safety +15, material +100
- Result: `0-1` `black_win:checkmate`
- FEN before: `2kr3r/1p2npp1/pn2b3/q2pP2p/2pP4/2NB2Q1/P1P1N1PP/1R3RK1 w - - 4 17`

- Engine re-search: `g3g7` score `-38`
- Engine re-search PV: `g3g7 c4d3 c2d3 c8b8 g7f6`

- Engine played-move constrained score: `-130`
- Engine played-move PV: `d3c4 b6c4 g3g5 e7g6 g1h1`
- Engine played-move SEE: `-230`
- Engine reference-move constrained score: `-38`, delta `+92`
- Engine reference-move PV: `g3g7 c4d3 c2d3 c8b8 g7f6`
- Engine reference-move SEE: `100`

- Reference bestmove: `g3g7` score `-273`
- Reference PV: `g3g7 c4d3 c2d3 e7c6 b1b3 h8g8 g7h6 b6a4 c3a4 a5a4 h6c1 a4a5 b3a3 a5b6 a3b3 b6c7`
- Reference played-move score: `-468`, delta `+195`
- Reference played-move PV: `b6c4 e2f4 c4d2 g3g7 d2b1 f4e6 f7e6 f1b1 a5c3 g7e7 c3d4 g1h1 d8d7 e7e6 h8d8`

### 18. Game 55, ply 108: g3f4 (Bxf4)

- Categories: `engine-loss, opponent-recapture, recapture-trade, queen-trade-sequence, bad-trade-sequence, played-negative-see, reference-negative-see`
- Score: `-115 -> +180` for current-eval, delta `+295`
- Sequence after reply `d4f4`: `-115 -> -231`, delta `-116`
- Engine recaptures available: `none`
- Material: `B` captures `P` for `100` cp
- Component movement: pawn_dynamics -4, material +100, threats +166
- Result: `0-1` `black_win:checkmate`
- FEN before: `1r3bk1/6p1/4p3/2p5/3q1p2/P1RPRPBP/1P4P1/1K6 w - - 0 55`

- Engine re-search: `g3f4` score `-212`
- Engine re-search PV: `g3f4 d4f4 e3e6 f4d2 c3c2`

- Engine played-move constrained score: `-212`
- Engine played-move PV: `g3f4 d4f4 e3e6 f4d2 c3c2`
- Engine played-move SEE: `-230`
- Engine reference-move constrained score: `-212`, delta `+0`
- Engine reference-move PV: `g3f4 d4f4 e3e6 f4d2 c3c2`
- Engine reference-move SEE: `-230`

- Reference bestmove: `g3f4` score `-610`
- Reference PV: `g3f4 d4f4 d3d4 c5d4 c3c8 b8c8 e3e4 f4h2 b1a2 h2g2 e4d4`

### 19. Game 17, ply 16: e5c6 (Nxc6)

- Categories: `engine-loss, opponent-recapture, recapture-trade, bad-trade-sequence, played-negative-see, engine-prefers-negative-see`
- Score: `+274 -> +416` for current-eval, delta `+142`
- Sequence after reply `d7c6`: `+274 -> +74`, delta `-200`
- Engine recaptures available: `none`
- Material: `N` captures `P` for `100` cp
- Component movement: pawn_structure -12, threats +14, material +100
- Result: `0-1` `black_win:checkmate`
- FEN before: `r1b1k2r/2ppqpp1/2p2n2/p1b1N3/4PP1p/2N2Q2/PPPP2PP/R1B2R1K w kq - 0 11`

- Engine re-search: `e5d3` score `241`
- Engine re-search PV: `e5d3 h4h3 d3c5 e7c5 g2h3`

- Engine played-move constrained score: `214`
- Engine played-move PV: `e5c6 d7c6 e4e5 e8g8 f3c6`
- Engine played-move SEE: `-220`
- Engine reference-move constrained score: `162`, delta `-52`
- Engine reference-move PV: `h2h3 e8g8 d2d3 a8b8 e5c4`
- Engine reference-move SEE: `0`

- Reference bestmove: `h2h3` score `111`
- Reference PV: `h2h3 f6h5 h1h2 c5d4 d2d3 d4e5 f4e5 e7e5 h2g1 f7f6 c1d2 d7d6 a1e1 c8e6`
- Reference played-move score: `-228`, delta `+339`
- Reference played-move PV: `d7c6 e4e5 f6d7 h2h3 c8b7 d2d3 e8c8 c1d2 c5b6 a1e1`

### 20. Game 71, ply 36: b3c4 (Bxc4)

- Categories: `opponent-recapture, recapture-trade, bad-trade-sequence, played-negative-see`
- Score: `+390 -> +547` for current-eval, delta `+157`
- Sequence after reply `e5c4`: `+390 -> +166`, delta `-224`
- Engine recaptures available: `none`
- Material: `B` captures `P` for `100` cp
- Component movement: threats -18, space -2, center_control -1, king_safety +26, material +100
- Result: `1-0` `white_win:checkmate`
- FEN before: `1rb2b1r/p2k1Pp1/3p3p/1p2n3/2p2N2/1B2B3/PPP2PPP/R1K4R w - - 0 20`

- Engine re-search: `b3c4` score `201`
- Engine re-search PV: `b3c4 b5c4 h1d1 b8b7 d1d4`

- Engine played-move constrained score: `201`
- Engine played-move PV: `b3c4 b5c4 h1d1 b8b7 d1d4`
- Engine played-move SEE: `-230`
- Engine reference-move constrained score: `200`, delta `-1`
- Engine reference-move PV: `e3a7 b8b7 a7d4 c4b3 a2b3`
- Engine reference-move SEE: `100`

- Reference bestmove: `e3a7` score `-104`
- Reference PV: `e3a7 b8b7 a7d4 e5f7 f4d5 c4b3 a2b3 f8e7 a1a8 e7d8 h1d1`
- Reference played-move score: `-156`, delta `+52`
- Reference played-move PV: `b5c4 e3a7 b8a8 a7d4 e5f7 f4d5 a8a6 d5b6 d7c7 b6c4 f8e7 c4e3 c8b7`

### 21. Game 65, ply 46: d6d7 (Rxd7+)

- Categories: `opponent-recapture, recapture-trade, bad-trade-sequence, played-negative-see, reference-negative-see`
- Score: `+1164 -> +1503` for current-eval, delta `+339`
- Sequence after reply `e7d7`: `+1164 -> +974`, delta `-190`
- Engine recaptures available: `none`
- Material: `R` captures `N` for `320` cp
- Component movement: threats -53, king_safety +17, material +320
- Result: `1-0` `white_win:checkmate`
- FEN before: `1r6/3nk1p1/p2R1p1p/1b2p3/2N5/5P2/PP1KPBPP/5B1R w - - 2 25`

- Engine re-search: `d6d7` score `896`
- Engine re-search PV: `d6d7 e7d7 c4b6 d7c6 a2a4`

- Engine played-move constrained score: `896`
- Engine played-move PV: `d6d7 e7d7 c4b6 d7c6 a2a4`
- Engine played-move SEE: `-180`
- Engine reference-move constrained score: `896`, delta `+0`
- Engine reference-move PV: `d6d7 e7d7 c4b6 d7c6 a2a4`
- Engine reference-move SEE: `-180`

- Reference bestmove: `d6d7` score `658`
- Reference PV: `d6d7 e7d7 c4b6 d7d6 a2a4 b8b6 f2b6 b5a4 b6f2`

### 22. Game 63, ply 26: g5f6 (Bxf6)

- Categories: `engine-loss, opponent-recapture, recapture-trade, bad-trade-sequence, played-negative-see, engine-prefers-negative-see`
- Score: `+55 -> +258` for current-eval, delta `+203`
- Sequence after reply `g7f6`: `+55 -> -99`, delta `-154`
- Engine recaptures available: `none`
- Material: `B` captures `P` for `100` cp
- Component movement: threats -21, mobility -5, center_control -5, king_safety +58, material +100
- Result: `0-1` `black_win:checkmate`
- FEN before: `r3k2r/ppp1b1pp/5p2/6Bq/6b1/3P1N2/PPP3PP/R3QK1R w kq - 0 16`

- Engine re-search: `g5f6` score `-103`
- Engine re-search PV: `g5f6 g7f6 e1e4 g4f3 g2f3`

- Engine played-move constrained score: `-103`
- Engine played-move PV: `g5f6 g7f6 e1e4 g4f3 g2f3`
- Engine played-move SEE: `-230`
- Engine reference-move constrained score: `-152`, delta `-49`
- Engine reference-move PV: `g5d2 g4f3 g2f3 h5f3 f1g1`
- Engine reference-move SEE: `0`

- Reference bestmove: `g5d2` score `-337`
- Reference PV: `g5d2 g4f3 g2f3 h5f3 f1g1 e8f7 e1f2 f3d5 f2g2 a8d8 d2e3`
- Reference played-move score: `-516`, delta `+179`
- Reference played-move PV: `g7f6 f3h4 e8c8 e1e7 h5h4 h2h3 g4d7 e7c5 d8e8 c5f2 h4g5`

### 23. Game 56, ply 31: f4d5 (Nxd5)

- Categories: `recapture-trade, opponent-recapture, bad-trade-sequence, played-negative-see, reference-negative-see`
- Score: `+325 -> +523` for current-eval, delta `+198`
- Sequence after reply `c3d5`: `+325 -> +151`, delta `-174`
- Engine recaptures available: `none`
- Material: `N` captures `P` for `100` cp
- Component movement: mobility -11, space -5, safe_mobility -2, threats +43, material +100
- Result: `1/2-1/2` `draw:threefold_repetition`
- FEN before: `r2qkb1r/pppbppp1/8/2pP4/5nP1/2N1PN2/PPQ5/R3KBR1 b Qkq - 0 16`

- Engine re-search: `f4d5` score `307`
- Engine re-search PV: `f4d5 c3d5 d7c6 d5c7 d8c7`

- Engine played-move constrained score: `307`
- Engine played-move PV: `f4d5 c3d5 d7c6 d5c7 d8c7`
- Engine played-move SEE: `-220`
- Engine reference-move constrained score: `307`, delta `+0`
- Engine reference-move PV: `f4d5 c3d5 d7c6 d5c7 d8c7`
- Engine reference-move SEE: `-220`

- Reference bestmove: `f4d5` score `-140`
- Reference PV: `f4d5 c3d5 e7e6 d5c3 d7c6 f1b5 c6b5 c3b5 d8d7 b5c3 e8c8 e1e2 f7f5 g4f5 e6f5 a1d1`

### 24. Game 3, ply 74: f3g5 (Nxg5)

- Categories: `opponent-recapture, recapture-trade, bad-trade-sequence, played-negative-see, engine-can-avoid-negative-see`
- Score: `+792 -> +1025` for current-eval, delta `+233`
- Sequence after reply `f6g5`: `+792 -> +618`, delta `-174`
- Engine recaptures available: `none`
- Material: `N` captures `P` for `100` cp
- Component movement: mobility -7, king_safety +61, material +100
- Result: `1-0` `white_win:checkmate`
- FEN before: `2r5/R1p2qk1/1p1Qbp2/4p1p1/4P3/5NPp/PPP2P1P/3R3K w - - 1 40`

- Engine re-search: `d6c6` score `837`
- Engine re-search PV: `d6c6 g7g8 c6a4 f7h5 a4a3`

- Engine played-move constrained score: `754`
- Engine played-move PV: `f3g5 f6g5 d6e5 g7h7 d1d2`
- Engine played-move SEE: `-220`
- Engine reference-move constrained score: `831`, delta `+77`
- Engine reference-move PV: `d6d3 g5g4 f3h4 e6a2 d3d7`
- Engine reference-move SEE: `0`

- Reference bestmove: `d6d3` score `468`
- Reference PV: `d6d3 e6g4 d1f1 c8d8 d3d8 g4f3 h1g1 f3e4`
- Reference played-move score: `371`, delta `+97`
- Reference played-move PV: `f6g5 d6e5 g7h7 f2f4 e6g4 d1d2 g5f4 e5f4 f7f4 g3f4`

### 25. Game 37, ply 44: f7e7 (Rxe7)

- Categories: `recapture-trade, opponent-recapture, bad-trade-sequence, played-negative-see, engine-prefers-negative-see`
- Score: `+1825 -> +2097` for current-eval, delta `+272`
- Sequence after reply `c5e7`: `+1825 -> +1653`, delta `-172`
- Engine recaptures available: `none`
- Material: `R` captures `N` for `320` cp
- Component movement: threats -48, pawn_structure -37, rook_files -9, king_safety -4, mobility +17, material +320
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/4nR2/p5P1/1pb1p1pk/8/1B6/PPPP2PP/R1B4K w - - 1 25`

- Engine re-search: `f7e7` score `2177`
- Engine re-search PV: `f7e7 h5g6 e7e5 c5d4 e5e6`

- Engine played-move constrained score: `2177`
- Engine played-move PV: `f7e7 h5g6 e7e5 c5d4 e5e6`
- Engine played-move SEE: `-180`
- Engine reference-move constrained score: `2123`, delta `-54`
- Engine reference-move PV: `g6g7 e7g8 f7c7 c5b6 c7c6`
- Engine reference-move SEE: `0`

- Reference bestmove: `g6g7` score `1217`
- Reference PV: `g6g7 e5e4 f7e7 c5e7 g7g8r`
- Reference played-move score: `1251`, delta `-34`
- Reference played-move PV: `c5e7 g6g7 e5e4 g7g8r h5g4 d2d4 e4d3 c2d3 e7f6`


## Engine Prefers Negative SEE

### 1. Game 33, ply 48: c6e8 (Qxe8+)

- Categories: `opponent-recapture, recapture-trade, queen-trade-sequence, bad-trade-sequence, played-negative-see, engine-prefers-negative-see`
- Score: `+1236 -> +1737` for current-eval, delta `+501`
- Sequence after reply `f7e8`: `+1236 -> +848`, delta `-388`
- Engine recaptures available: `none`
- Material: `Q` captures `R` for `500` cp
- Component movement: threats -54, piece_square -3, pawn_dynamics -2, mobility +22, material +500
- Result: `1-0` `white_win:checkmate`
- FEN before: `4r3/5kPp/p1Qp4/2pP1B2/2Pp1b2/N5P1/PP5P/6K1 w - - 1 28`

- Engine re-search: `c6e8` score `2235`
- Engine re-search PV: `c6e8 f7e8 g7g8q e8e7 g3f4`

- Engine played-move constrained score: `2235`
- Engine played-move PV: `c6e8 f7e8 g7g8q e8e7 g3f4`
- Engine played-move SEE: `-400`
- Engine reference-move constrained score: `1695`, delta `-540`
- Engine reference-move PV: `c6d7 f7f6 g3f4 e8g8 d7d6`
- Engine reference-move SEE: `0`

- Reference bestmove: `c6d7` score `1053`
- Reference PV: `c6d7 e8e7 g7g8q f7g8 d7e7 h7h5 e7e8 g8g7 e8h5 f4e3 g1f1`
- Reference played-move score: `899994`, delta `-898941`
- Reference played-move PV: `f7e8 g7g8q e8e7 g8g7 e7e8 f5e6 f4e3 g1g2 d4d3 g7f7 e8d8 f7d7`

### 2. Game 73, ply 64: h1h7 (Rxh7+)

- Categories: `opponent-recapture, recapture-trade, bad-trade-sequence, played-negative-see, engine-prefers-negative-see`
- Score: `+1505 -> +1662` for current-eval, delta `+157`
- Sequence after reply `g7h7`: `+1505 -> +1110`, delta `-395`
- Engine recaptures available: `none`
- Material: `R` captures `P` for `100` cp
- Component movement: threats -28, pawn_structure -18, pawn_dynamics -10, mobility -4, piece_square +61, material +100
- Result: `1-0` `white_win:checkmate`
- FEN before: `1r6/1Q3Nkp/p7/4b3/2P5/8/PP3PP1/1K5R w - - 1 36`

- Engine re-search: `h1h7` score `1599`
- Engine re-search PV: `h1h7 g7h7 b7e4 h7g8 f7h6`

- Engine played-move constrained score: `1599`
- Engine played-move PV: `h1h7 g7h7 b7e4 h7g8 f7h6`
- Engine played-move SEE: `-400`
- Engine reference-move constrained score: `1471`, delta `-128`
- Engine reference-move PV: `b7d7 b8b2 b1c1 e5c3 f7d6`
- Engine reference-move SEE: `0`

- Reference bestmove: `b7d7` score `853`
- Reference PV: `b7d7 b8b2 b1c1 b2f2 f7g5 g7g6 g5e4 e5f4 c1b1 h7h6 d7e6 g6h7 e6f7 h7h8 f7f8 h8h7`
- Reference played-move score: `3535`, delta `-2682`
- Reference played-move PV: `g7f8 f7e5 b8b7 h7b7 f8g8 b7a7 g8h8 e5f7 h8g8 a7a6 g8f7`

### 3. Game 25, ply 16: b7b6 (Qxb6)

- Categories: `opponent-recapture, recapture-trade, queen-trade-sequence, bad-trade-sequence, played-negative-see, engine-prefers-negative-see`
- Score: `+546 -> +1033` for current-eval, delta `+487`
- Sequence after reply `c7b6`: `+546 -> +260`, delta `-286`
- Engine recaptures available: `none`
- Material: `Q` captures `R` for `500` cp
- Component movement: threats -37, space -6, king_safety -5, center_control -3, mobility +15, material +500
- Result: `1/2-1/2` `draw:threefold_repetition`
- FEN before: `1n1qk2r/1Qpb1ppp/1r3n2/p7/1B1P4/8/PP2PPPP/RN2KBNR w KQk - 1 10`

- Engine re-search: `b7b6` score `271`
- Engine re-search PV: `b7b6 c7b6 b4d6 d7f5 d6e5`

- Engine played-move constrained score: `271`
- Engine played-move PV: `b7b6 c7b6 b4d6 d7f5 d6e5`
- Engine played-move SEE: `-400`
- Engine reference-move constrained score: `240`, delta `-31`
- Engine reference-move PV: `b7f3 b6b4 b1d2 e8g8 d2b3`
- Engine reference-move SEE: `0`

- Reference bestmove: `b7f3` score `104`
- Reference PV: `b7f3 b6b4 f3c3 e8g8 a2a3 f6e4 c3e3 b4c4 g1f3 d7f5 b1c3 f8e8 c3e4 f5e4 e3b3`
- Reference played-move score: `-67`, delta `+171`
- Reference played-move PV: `c7b6 b4d2 d8e7 g1f3 b8c6 e2e3 g7g5 f1e2 g5g4 f3e5 c6e5`

### 4. Game 5, ply 28: c4e6 (Bxe6)

- Categories: `engine-loss, opponent-recapture, recapture-trade, bad-trade-sequence, played-negative-see, engine-prefers-negative-see`
- Score: `+381 -> +629` for current-eval, delta `+248`
- Sequence after reply `d7e6`: `+381 -> +172`, delta `-209`
- Engine recaptures available: `none`
- Material: `B` captures `P` for `100` cp
- Component movement: bishop_quality -3, pawn_dynamics -3, king_safety +39, material +100
- Result: `0-1` `black_win:checkmate`
- FEN before: `2r1k1nr/2Bbb3/p3pp2/1p4pp/2BP4/2N1P3/PP3PPP/R3K2R w KQk - 1 16`

- Engine re-search: `c4e6` score `146`
- Engine re-search PV: `c4e6 d7e6 d4d5 e7b4 d5d6`

- Engine played-move constrained score: `146`
- Engine played-move PV: `c4e6 d7e6 d4d5 e7b4 d5d6`
- Engine played-move SEE: `-230`
- Engine reference-move constrained score: `56`, delta `-90`
- Engine reference-move PV: `c4b3 c8c7 e1g1 e8f8 a1c1`
- Engine reference-move SEE: `0`

- Reference bestmove: `c4b3` score `-334`
- Reference PV: `c4b3 c8c7 d4d5 e6e5 a2a4 c7c8 h2h4`
- Reference played-move score: `-347`, delta `+13`
- Reference played-move PV: `d7e6 c7a5 f6f5 d4d5 e6d7 h2h4 g5g4 e3e4 f5e4 d5d6 e7d6`

### 5. Game 17, ply 16: e5c6 (Nxc6)

- Categories: `engine-loss, opponent-recapture, recapture-trade, bad-trade-sequence, played-negative-see, engine-prefers-negative-see`
- Score: `+274 -> +416` for current-eval, delta `+142`
- Sequence after reply `d7c6`: `+274 -> +74`, delta `-200`
- Engine recaptures available: `none`
- Material: `N` captures `P` for `100` cp
- Component movement: pawn_structure -12, threats +14, material +100
- Result: `0-1` `black_win:checkmate`
- FEN before: `r1b1k2r/2ppqpp1/2p2n2/p1b1N3/4PP1p/2N2Q2/PPPP2PP/R1B2R1K w kq - 0 11`

- Engine re-search: `e5d3` score `241`
- Engine re-search PV: `e5d3 h4h3 d3c5 e7c5 g2h3`

- Engine played-move constrained score: `214`
- Engine played-move PV: `e5c6 d7c6 e4e5 e8g8 f3c6`
- Engine played-move SEE: `-220`
- Engine reference-move constrained score: `162`, delta `-52`
- Engine reference-move PV: `h2h3 e8g8 d2d3 a8b8 e5c4`
- Engine reference-move SEE: `0`

- Reference bestmove: `h2h3` score `111`
- Reference PV: `h2h3 f6h5 h1h2 c5d4 d2d3 d4e5 f4e5 e7e5 h2g1 f7f6 c1d2 d7d6 a1e1 c8e6`
- Reference played-move score: `-228`, delta `+339`
- Reference played-move PV: `d7c6 e4e5 f6d7 h2h3 c8b7 d2d3 e8c8 c1d2 c5b6 a1e1`

### 6. Game 63, ply 26: g5f6 (Bxf6)

- Categories: `engine-loss, opponent-recapture, recapture-trade, bad-trade-sequence, played-negative-see, engine-prefers-negative-see`
- Score: `+55 -> +258` for current-eval, delta `+203`
- Sequence after reply `g7f6`: `+55 -> -99`, delta `-154`
- Engine recaptures available: `none`
- Material: `B` captures `P` for `100` cp
- Component movement: threats -21, mobility -5, center_control -5, king_safety +58, material +100
- Result: `0-1` `black_win:checkmate`
- FEN before: `r3k2r/ppp1b1pp/5p2/6Bq/6b1/3P1N2/PPP3PP/R3QK1R w kq - 0 16`

- Engine re-search: `g5f6` score `-103`
- Engine re-search PV: `g5f6 g7f6 e1e4 g4f3 g2f3`

- Engine played-move constrained score: `-103`
- Engine played-move PV: `g5f6 g7f6 e1e4 g4f3 g2f3`
- Engine played-move SEE: `-230`
- Engine reference-move constrained score: `-152`, delta `-49`
- Engine reference-move PV: `g5d2 g4f3 g2f3 h5f3 f1g1`
- Engine reference-move SEE: `0`

- Reference bestmove: `g5d2` score `-337`
- Reference PV: `g5d2 g4f3 g2f3 h5f3 f1g1 e8f7 e1f2 f3d5 f2g2 a8d8 d2e3`
- Reference played-move score: `-516`, delta `+179`
- Reference played-move PV: `g7f6 f3h4 e8c8 e1e7 h5h4 h2h3 g4d7 e7c5 d8e8 c5f2 h4g5`

### 7. Game 37, ply 44: f7e7 (Rxe7)

- Categories: `recapture-trade, opponent-recapture, bad-trade-sequence, played-negative-see, engine-prefers-negative-see`
- Score: `+1825 -> +2097` for current-eval, delta `+272`
- Sequence after reply `c5e7`: `+1825 -> +1653`, delta `-172`
- Engine recaptures available: `none`
- Material: `R` captures `N` for `320` cp
- Component movement: threats -48, pawn_structure -37, rook_files -9, king_safety -4, mobility +17, material +320
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/4nR2/p5P1/1pb1p1pk/8/1B6/PPPP2PP/R1B4K w - - 1 25`

- Engine re-search: `f7e7` score `2177`
- Engine re-search PV: `f7e7 h5g6 e7e5 c5d4 e5e6`

- Engine played-move constrained score: `2177`
- Engine played-move PV: `f7e7 h5g6 e7e5 c5d4 e5e6`
- Engine played-move SEE: `-180`
- Engine reference-move constrained score: `2123`, delta `-54`
- Engine reference-move PV: `g6g7 e7g8 f7c7 c5b6 c7c6`
- Engine reference-move SEE: `0`

- Reference bestmove: `g6g7` score `1217`
- Reference PV: `g6g7 e5e4 f7e7 c5e7 g7g8r`
- Reference played-move score: `1251`, delta `-34`
- Reference played-move PV: `c5e7 g6g7 e5e4 g7g8r h5g4 d2d4 e4d3 c2d3 e7f6`

### 8. Game 1, ply 27: e4b7 (Bxb7)

- Categories: `opponent-recapture, recapture-trade, bad-trade-sequence, played-negative-see, engine-prefers-negative-see`
- Score: `+485 -> +797` for current-eval, delta `+312`
- Sequence after reply `c8b7`: `+485 -> +326`, delta `-159`
- Engine recaptures available: `none`
- Material: `B` captures `P` for `100` cp
- Component movement: threats +92, material +100
- Result: `1-0` `white_win:checkmate`
- FEN before: `r1bq1k1r/1p4p1/p3p3/2p2p1p/4Bb2/2NP1N2/PPP2PPP/R2QR1K1 w - - 0 14`

- Engine re-search: `e4b7` score `522`
- Engine re-search PV: `e4b7 c8b7 e1e6 b7d5 c3d5`

- Engine played-move constrained score: `522`
- Engine played-move PV: `e4b7 c8b7 e1e6 b7d5 c3d5`
- Engine played-move SEE: `-230`
- Engine reference-move constrained score: `409`, delta `-113`
- Engine reference-move PV: `g2g3 f5e4 c3e4 f4d6 e1f1`
- Engine reference-move SEE: `0`

- Reference bestmove: `g2g3` score `391`
- Reference PV: `g2g3 f5e4 c3e4 f4c7 e4c5 h8h6 d3d4 h5h4 f3h4`
- Reference played-move score: `195`, delta `+196`
- Reference played-move PV: `c8b7 e1e6 f8f7 e6e1 d8c7 d3d4 c5d4 d1d4 b7f3 g2f3 f4h2 g1g2 a8d8 c3d5 c7d6`

### 9. Game 28, ply 66: c7c4 (Rxc4)

- Categories: `engine-loss, opponent-recapture, recapture-trade, bad-trade-sequence, played-negative-see, engine-prefers-negative-see`
- Score: `-187 -> +149` for current-eval, delta `+336`
- Sequence after reply `d3c4`: `-187 -> -317`, delta `-130`
- Engine recaptures available: `none`
- Material: `R` captures `B` for `330` cp
- Component movement: threats -69, center_control -1, bishop_quality +36, material +330
- Result: `1-0` `white_win:checkmate`
- FEN before: `k7/2r2ppp/p4n2/4p3/2BbP2P/3P2B1/P3KPP1/1R6 b - - 1 34`

- Engine re-search: `c7c4` score `-104`
- Engine re-search PV: `c7c4 d3c4 f6e4 b1e1 e4c5`

- Engine played-move constrained score: `-104`
- Engine played-move PV: `c7c4 d3c4 f6e4 b1e1 e4c5`
- Engine played-move SEE: `-170`
- Engine reference-move constrained score: `-174`, delta `-70`
- Engine reference-move PV: `f6h5 g3h2 a8a7 b1b4 a6a5`
- Engine reference-move SEE: `0`

- Reference bestmove: `f6h5` score `-225`
- Reference PV: `f6h5 g3h2 a8a7 g2g4 h5f6 f2f3 f6e8 f3f4 e8d6 c4b3 f7f6 e2f3 d6b5 g4g5`
- Reference played-move score: `-338`, delta `+113`
- Reference played-move PV: `d3c4 f6e4 b1b3 a6a5 b3a3 e4c3 e2d3 a5a4`

### 10. Game 7, ply 45: f1f6 (Rxf6)

- Categories: `engine-loss, opponent-recapture, recapture-trade, bad-trade-sequence, played-negative-see, engine-prefers-negative-see`
- Score: `+663 -> +1014` for current-eval, delta `+351`
- Sequence after reply `g7f6`: `+663 -> +538`, delta `-125`
- Engine recaptures available: `none`
- Material: `R` captures `B` for `330` cp
- Component movement: threats -83, king_safety -2, pawn_dynamics -2, rook_files -1, bishop_quality +35, material +330
- Result: `0-1` `black_win:checkmate`
- FEN before: `r7/6pk/b1Np1b2/p6r/2p1P3/3P2B1/PPP3PP/3R1R1K w - - 1 24`

- Engine re-search: `f1f6` score `656`
- Engine re-search PV: `f1f6 g7f6 g3d6 a6b5 c6d4`

- Engine played-move constrained score: `656`
- Engine played-move PV: `f1f6 g7f6 g3d6 a6b5 c6d4`
- Engine played-move SEE: `-170`
- Engine reference-move constrained score: `480`, delta `-176`
- Engine reference-move PV: `b2b4 c4b3 a2b3 a8c8 f1f6`
- Engine reference-move SEE: `0`

- Reference bestmove: `b2b4` score `214`
- Reference PV: `b2b4 c4b3 a2b3 a6b5 f1f6 g7f6 c6d4 a5a4 c2c4 b5d7 b3a4`
- Reference played-move score: `139`, delta `+75`
- Reference played-move PV: `g7f6 d3c4 h5c5 d1d6 c5c4 c6d4 c4a4 a2a3 a8e8 g3f2 a6c8 d6f6 e8e4`


## Loss-Game Watchlist

### 1. Game 58, ply 107: d5h5 (Rh5)

- Categories: `engine-loss, eval-swing, opponent-recapture, recapture-trade, bad-trade-sequence`
- Score: `-253 -> -410` for current-eval, delta `-157`
- Sequence after reply `g4h5`: `-253 -> -751`, delta `-498`
- Engine recaptures available: `none`
- Material: `R` captures `-` for `0` cp
- Component movement: pawn_structure -116, threats -21, piece_square -8, rook_files -7, center_control +1
- Result: `1-0` `white_win:checkmate`
- FEN before: `2R2N2/7P/p4k2/1ppr4/5PK1/6P1/8/8 b - - 0 56`

- Engine re-search: `d5h5` score `-1496`
- Engine re-search PV: `d5h5 g4h5 c5c4 h7h8q f6e7`

- Engine played-move constrained score: `-1496`
- Engine played-move PV: `d5h5 g4h5 c5c4 h7h8q f6e7`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-1496`, delta `+0`
- Engine reference-move PV: `d5h5 g4h5 c5c4 h7h8q f6e7`
- Engine reference-move SEE: `0`

- Reference bestmove: `d5h5` score `-1053`
- Reference PV: `d5h5 g4h5 f6f5 c8c5 f5e4 f8e6 b5b4 f4f5 a6a5 e6g5 e4d4 c5a5`

### 2. Game 76, ply 141: f4f3 (f3)

- Categories: `engine-loss, eval-swing, opponent-recapture, recapture-trade, bad-trade-sequence`
- Score: `+148 -> -36` for current-eval, delta `-184`
- Sequence after reply `e1f3`: `+148 -> -274`, delta `-422`
- Engine recaptures available: `none`
- Material: `P` captures `-` for `0` cp
- Component movement: pawn_structure -174, piece_square -9, threats -2, pawn_dynamics -2, safe_mobility +1, space +2
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/pK6/1pP5/1P2k3/P4p2/6p1/8/4N3 b - - 1 71`

- Engine re-search: `e5d4` score `-773`
- Engine re-search PV: `e5d4 c6c7 d4e3 c7c8q e3f2`

- Engine played-move constrained score: `-778`
- Engine played-move PV: `f4f3 e1f3 e5e4 f3e1 g3g2`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-786`, delta `-8`
- Engine reference-move PV: `e5e4 c6c7 e4e3 c7c8q e3f2`
- Engine reference-move SEE: `0`

- Reference bestmove: `e5e4` score `-558`
- Reference PV: `e5e4 c6c7 f4f3 e1f3 e4f3 c7c8r g3g2 c8c1 f3e3 c1e1 e3d2 e1a1`
- Reference played-move score: `-704`, delta `+146`
- Reference played-move PV: `e1f3 e5f4 f3g1 g3g2 c6c7 f4g3 b7b8 g3h2`

### 3. Game 75, ply 158: d1e2 (Ke2)

- Categories: `engine-loss, eval-swing`
- Score: `-461 -> -755` for current-eval, delta `-294`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -321, center_control +2, piece_square +25
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/8/8/8/8/1pk5/3p4/3K4 w - - 0 80`

- Engine re-search: `d1e2` score `-1928`
- Engine re-search PV: `d1e2 b3b2 e2e3 d2d1q e3e4`

- Engine played-move constrained score: `-1928`
- Engine played-move PV: `d1e2 b3b2 e2e3 d2d1q e3e4`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-1928`, delta `+0`
- Engine reference-move PV: `d1e2 b3b2 e2e3 d2d1q e3e4`
- Engine reference-move SEE: `0`

- Reference bestmove: `d1e2` score `-4287`
- Reference PV: `d1e2 b3b2 e2f2 b2b1q f2e3 b1g1 e3f3 g1h1 f3f4 h1h4 f4f5 h4h5 f5e6`

### 4. Game 20, ply 120: f7g8 (Kg8)

- Categories: `engine-loss, eval-swing`
- Score: `-45 -> -306` for current-eval, delta `-261`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -243, piece_square -23, king_safety +5
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/1p1KPkp1/1P5p/p4p1P/7P/B7/8/8 b - - 2 62`

- Engine re-search: `f7g8` score `-899992`
- Engine re-search PV: `f7g8 e7e8q g8h7 a3b2 a5a4`

- Engine played-move constrained score: `-899992`
- Engine played-move PV: `f7g8 e7e8q g8h7 a3b2 a5a4`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-899992`, delta `+0`
- Engine reference-move PV: `f7g8 e7e8q g8h7 a3b2 a5a4`
- Engine reference-move SEE: `0`

- Reference bestmove: `f7g8` score `-899996`
- Reference PV: `f7g8 a3b2 g8h7 e7e8q f5f4 e8g6 h7h8 g6g7`

### 5. Game 75, ply 160: e2f2 (Kf2)

- Categories: `engine-loss, eval-swing`
- Score: `-833 -> -1080` for current-eval, delta `-247`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -246, center_control -1
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/8/8/8/8/2k5/1p1pK3/8 w - - 0 81`

- Engine re-search: `e2e3` score `-1934`
- Engine re-search PV: `e2e3 d2d1q e3e4 d1d6 e4f3`

- Engine played-move constrained score: `-1946`
- Engine played-move PV: `e2f2 b2b1q f2e3 b1f5 e3e2`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-1934`, delta `+12`
- Engine reference-move PV: `e2e3 d2d1q e3e4 d1d6 e4f3`
- Engine reference-move SEE: `0`

- Reference bestmove: `e2e3` score `-899994`
- Reference PV: `e2e3 b2b1q e3f4 d2d1q f4e5 b1b6 e5f4 b6d4 f4g5 d1g4 g5h6 d4h8`
- Reference played-move score: `-899995`, delta `+1`
- Reference played-move PV: `b2b1q f2f3 d2d1q f3f4 d1d4 f4g5 b1g1 g5h6 d4h4`

### 6. Game 5, ply 196: a2a3 (Ka3)

- Categories: `engine-loss, eval-swing`
- Score: `-671 -> -901` for current-eval, delta `-230`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -229, piece_square -1
- Result: `0-1` `black_win:checkmate`
- FEN before: `7b/8/8/8/8/8/Kpk5/8 w - - 46 100`

- Engine re-search: `a2a3` score `-820514`
- Engine re-search PV: `a2a3 b2b1r a3a4 h8c3 a4a3`

- Engine played-move constrained score: `-820514`
- Engine played-move PV: `a2a3 b2b1r a3a4 h8c3 a4a3`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-820514`, delta `+0`
- Engine reference-move PV: `a2a3 b2b1r a3a4 h8c3 a4a3`
- Engine reference-move SEE: `0`

- Reference bestmove: `a2a3` score `-899997`
- Reference PV: `a2a3 b2b1q a3a4 b1b6 a4a3 b6a5`

### 7. Game 13, ply 148: e4d3 (Kd3)

- Categories: `engine-loss, eval-swing`
- Score: `-731 -> -951` for current-eval, delta `-220`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -205, piece_square -9, space -4, center_control -4, threats +2
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/8/8/8/4K3/6kp/4p3/8 w - - 2 78`

- Engine re-search: `e4d3` score `-1891`
- Engine re-search PV: `e4d3 e2e1q d3d4 h3h2 d4d3`

- Engine played-move constrained score: `-1891`
- Engine played-move PV: `e4d3 e2e1q d3d4 h3h2 d4d3`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-1907`, delta `-16`
- Engine reference-move PV: `e4e5 e2e1q e5d4 h3h2 d4d5`
- Engine reference-move SEE: `0`

- Reference bestmove: `e4e5` score `-4293`
- Reference PV: `e4e5 e2e1q e5f6 h3h2 f6f7 h2h1q f7g7 e1e6`
- Reference played-move score: `-4304`, delta `+11`
- Reference played-move PV: `e2e1q d3c4 h3h2 c4b5 e1e2 b5c5 e2e7 c5d5 e7d7 d5c5 d7c7 c5d5 h2h1q d5d4`

### 8. Game 58, ply 109: f6f5 (Kf5)

- Categories: `engine-loss, eval-swing`
- Score: `-751 -> -961` for current-eval, delta `-210`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -220, king_safety -3, center_control +4, piece_square +7
- Result: `1-0` `white_win:checkmate`
- FEN before: `2R2N2/7P/p4k2/1pp4K/5P2/6P1/8/8 b - - 0 57`

- Engine re-search: `f6f5` score `-1793`
- Engine re-search PV: `f6f5 c8c5 f5e4 f8e6 e4e3`

- Engine played-move constrained score: `-1793`
- Engine played-move PV: `f6f5 c8c5 f5e4 f8e6 e4e3`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-1793`, delta `+0`
- Engine reference-move PV: `f6f5 c8c5 f5e4 f8e6 e4e3`
- Engine reference-move SEE: `0`

- Reference bestmove: `f6f5` score `-1078`
- Reference PV: `f6f5 c8c5 f5e4 f4f5 b5b4 f5f6 b4b3 f6f7 b3b2 f8e6 b2b1b f7f8r`

### 9. Game 31, ply 90: e3f4 (Kf4)

- Categories: `engine-loss, eval-swing`
- Score: `-616 -> -824` for current-eval, delta `-208`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -190, threats -17, center_control -3, piece_square -2, space +2, king_safety +5
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/k5P1/p7/4P3/3P4/1p1qK2P/4b3/8 w - - 1 47`

- Engine re-search: `e3f4` score `-1772`
- Engine re-search PV: `e3f4 d3f3 f4g5 b3b2 g7g8q`

- Engine played-move constrained score: `-1772`
- Engine played-move PV: `e3f4 d3f3 f4g5 b3b2 g7g8q`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-1772`, delta `+0`
- Engine reference-move PV: `e3f4 b3b2 g7g8q d3f3 f4g5`
- Engine reference-move SEE: `0`

- Reference bestmove: `e3f4` score `-1203`
- Reference PV: `e3f4 b3b2 g7g8q d3f3 f4g5 f3g3 g5h6 g3g8 d4d5 b2b1q`

### 10. Game 13, ply 132: d3d4 (Kd4)

- Categories: `engine-loss, eval-swing`
- Score: `-172 -> -369` for current-eval, delta `-197`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -213, center_control +4, piece_square +8
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/8/8/8/7p/3Kp3/5k1P/8 w - - 2 70`

- Engine re-search: `d3d4` score `-901`
- Engine re-search PV: `d3d4 e3e2 d4c5 e2e1q c5d6`

- Engine played-move constrained score: `-901`
- Engine played-move PV: `d3d4 e3e2 d4c5 e2e1q c5d6`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-901`, delta `+0`
- Engine reference-move PV: `d3d4 e3e2 d4c5 e2e1q c5d6`
- Engine reference-move SEE: `0`

- Reference bestmove: `d3d4` score `-3530`
- Reference PV: `d3d4 e3e2 d4d5 h4h3 d5e4 f2g2 e4d4 g2h2`

### 11. Game 86, ply 167: e4e3 (Ke3)

- Categories: `engine-loss, eval-swing`
- Score: `-192 -> -388` for current-eval, delta `-196`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -188, piece_square -8, center_control -4, space +4
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/8/5K2/7P/4k3/8/8/8 b - - 10 85`

- Engine re-search: `e4d4` score `-939`
- Engine re-search PV: `e4d4 h5h6 d4e4 h6h7 e4d4`

- Engine played-move constrained score: `-939`
- Engine played-move PV: `e4e3 h5h6 e3f3 h6h7 f3e4`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-939`, delta `+0`
- Engine reference-move PV: `e4f3 h5h6 f3e4 h6h7 e4d4`
- Engine reference-move SEE: `0`

- Reference bestmove: `e4f3` score `-3487`
- Reference PV: `e4f3 h5h6 f3g4 h6h7 g4g3 h7h8q g3g2 h8f8 g2g3 f8a8 g3h2`
- Reference played-move score: `-3494`, delta `+7`
- Reference played-move PV: `h5h6 e3e2 h6h7 e2f1 h7h8q f1g2 f6f7 g2g3 f7g6 g3f3`

### 12. Game 20, ply 122: a5a4 (a4)

- Categories: `engine-loss, eval-swing`
- Score: `-38 -> -231` for current-eval, delta `-193`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `P` captures `-` for `0` cp
- Component movement: pawn_structure -180, piece_square -13
- Result: `1-0` `white_win:checkmate`
- FEN before: `6k1/1p1KP1p1/1P5p/p4p1P/7P/8/1B6/8 b - - 4 63`

- Engine re-search: `g8h8` score `-899994`
- Engine re-search PV: `g8h8 e7e8q h8h7 e8g6 h7g8`

- Engine played-move constrained score: `-899994`
- Engine played-move PV: `a5a4 e7e8q g8h7 e8g6 h7g8`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-899994`, delta `+0`
- Engine reference-move PV: `a5a4 e7e8q g8h7 e8g6 h7g8`
- Engine reference-move SEE: `0`

- Reference bestmove: `a5a4` score `-899997`
- Reference PV: `a5a4 e7e8q g8h7 e8g6 h7h8 g6g7`

### 13. Game 20, ply 106: c8f8 (Rf8)

- Categories: `engine-loss, eval-swing`
- Score: `+305 -> +118` for current-eval, delta `-187`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `R` captures `-` for `0` cp
- Component movement: pawn_structure -178, rook_files -6, threats -5, king_safety +2
- Result: `1-0` `white_win:checkmate`
- FEN before: `2r3k1/1pP1K1p1/1P2Pp1p/p1B4P/8/7P/8/8 b - - 1 55`

- Engine re-search: `c8a8` score `-8`
- Engine re-search PV: `c8a8 e7d7 f6f5 c5f2 a5a4`

- Engine played-move constrained score: `-29`
- Engine played-move PV: `c8f8 e7d7 f8a8 c5a3 f6f5`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-8`, delta `+21`
- Engine reference-move PV: `c8a8 e7d7 f6f5 c5f2 a5a4`
- Engine reference-move SEE: `0`

- Reference bestmove: `c8a8` score `-645`
- Reference PV: `c8a8 e7d7 f6f5 c7c8r a8c8 d7c8 a5a4 c8d7 f5f4 e6e7 f4f3 e7e8r g8h7`
- Reference played-move score: `-647`, delta `+2`
- Reference played-move PV: `e7d7 f8a8 c7c8q a8c8 d7c8 g8h7 c8d7 a5a4 e6e7`

### 14. Game 20, ply 110: c8f8 (Rf8)

- Categories: `engine-loss, eval-swing`
- Score: `+274 -> +87` for current-eval, delta `-187`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `R` captures `-` for `0` cp
- Component movement: pawn_structure -178, rook_files -6, threats -5, king_safety +2
- Result: `1-0` `white_win:checkmate`
- FEN before: `2r3k1/1pP1K1p1/1P2Pp1p/p6P/7P/B7/8/8 b - - 2 57`

- Engine re-search: `c8f8` score `3`
- Engine re-search PV: `c8f8 e7d7 f8a8 c7c8q a8c8`

- Engine played-move constrained score: `3`
- Engine played-move PV: `c8f8 e7d7 f8a8 c7c8q a8c8`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `3`, delta `+0`
- Engine reference-move PV: `c8f8 e7d7 f8a8 c7c8q a8c8`
- Engine reference-move SEE: `0`

- Reference bestmove: `c8f8` score `-707`
- Reference PV: `c8f8 e7d7 f8a8 c7c8r a8c8 d7c8 a5a4 e6e7 g8f7 c8d7 f7g8`

### 15. Game 76, ply 143: e5e4 (Ke4)

- Categories: `engine-loss, eval-swing`
- Score: `-274 -> -458` for current-eval, delta `-184`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -197, piece_square -1, space +3, threats +9
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/pK6/1pP5/1P2k3/P7/5Np1/8/8 b - - 0 72`

- Engine re-search: `e5e4` score `-1000`
- Engine re-search PV: `e5e4 f3g1 e4e3 b7a7 e3f2`

- Engine played-move constrained score: `-1000`
- Engine played-move PV: `e5e4 f3g1 e4e3 b7a7 e3f2`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-1000`, delta `+0`
- Engine reference-move PV: `e5f4 f3g1 f4e3 b7a7 e3f2`
- Engine reference-move SEE: `0`

- Reference bestmove: `e5f4` score `-685`
- Reference PV: `e5f4 f3g1 g3g2 c6c7 f4g3 c7c8r g3h2 c8c1 h2g3 c1b1`
- Reference played-move score: `-650`, delta `-35`
- Reference played-move PV: `f3g1 g3g2 c6c7 e4e3 c7c8r e3f4 c8f8 f4g4 f8f2`

### 16. Game 86, ply 165: d4e4 (Ke4)

- Categories: `engine-loss, eval-swing`
- Score: `-172 -> -352` for current-eval, delta `-180`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -180
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/6K1/8/7P/3k4/8/8/8 b - - 8 84`

- Engine re-search: `d4d5` score `-909`
- Engine re-search PV: `d4d5 h5h6 d5e6 h6h7 e6e5`

- Engine played-move constrained score: `-939`
- Engine played-move PV: `d4e4 g7f6 e4e3 h5h6 e3f3`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-939`, delta `+0`
- Engine reference-move PV: `d4e4 g7f6 e4f3 h5h6 f3e4`
- Engine reference-move SEE: `0`

- Reference bestmove: `d4e4` score `-3490`
- Reference PV: `d4e4 g7f6 e4f3 h5h6 f3g2 h6h7 g2f2 h7h8q f2g2 f6g7 g2f2 h8h2 f2f1`

### 17. Game 6, ply 117: c7d6 (Kd6)

- Categories: `engine-loss, eval-swing`
- Score: `-273 -> -451` for current-eval, delta `-178`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -203, mobility -6, space -5, threats -4, king_safety +9, piece_square +22
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/1Rk5/P7/8/P3p3/2p1Kp2/8/8 b - - 1 60`

- Engine re-search: `c7c6` score `-492`
- Engine re-search PV: `c7c6 b7b3 c3c2 b3c3 c6b6`

- Engine played-move constrained score: `-611`
- Engine played-move PV: `c7d6 b7b3 d6c7 b3c3 c7b6`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-492`, delta `+119`
- Engine reference-move PV: `c7c6 b7b3 c3c2 b3c3 c6b6`
- Engine reference-move SEE: `0`

- Reference bestmove: `c7c6` score `-562`
- Reference PV: `c7c6 b7b3 c3c2 b3c3 c6b6 c3c2 b6a6 c2b2`
- Reference played-move score: `-583`, delta `+21`
- Reference played-move PV: `b7b3 d6c7 b3c3 c7b6 c3c8 b6a6 c8c1 a6b7`

### 18. Game 86, ply 161: d6c5 (Kc5)

- Categories: `engine-loss, eval-swing`
- Score: `-190 -> -366` for current-eval, delta `-176`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -180, space +4
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/6K1/3k4/7P/8/8/8/8 b - - 4 82`

- Engine re-search: `d6e6` score `-909`
- Engine re-search PV: `d6e6 h5h6 e6e5 h6h7 e5f5`

- Engine played-move constrained score: `-939`
- Engine played-move PV: `d6c5 h5h6 c5b5 h6h7 b5c4`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-909`, delta `+30`
- Engine reference-move PV: `d6e6 h5h6 e6e5 h6h7 e5f5`
- Engine reference-move SEE: `0`

- Reference bestmove: `d6e6` score `-3487`
- Reference PV: `d6e6 h5h6 e6f5 h6h7 f5g4 h7h8q g4g3 g7f6 g3f3 f6f5 f3f2 h8f8 f2g3`
- Reference played-move score: `-3525`, delta `+38`
- Reference played-move PV: `h5h6 c5d4 h6h7 d4e3 h7h8q e3f2 g7f6 f2g2`

### 19. Game 86, ply 157: e6d5 (Kd5)

- Categories: `engine-loss, eval-swing`
- Score: `-182 -> -354` for current-eval, delta `-172`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -188, space +5, piece_square +8
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/6K1/4k3/7P/8/8/8/8 b - - 0 80`

- Engine re-search: `e6f5` score `-909`
- Engine re-search PV: `e6f5 h5h6 f5g5 h6h7 g5f5`

- Engine played-move constrained score: `-909`
- Engine played-move PV: `e6d5 h5h6 d5e6 h6h7 e6e5`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-909`, delta `+0`
- Engine reference-move PV: `e6e5 h5h6 e5d5 h6h7 d5e5`
- Engine reference-move SEE: `0`

- Reference bestmove: `e6e5` score `-3487`
- Reference PV: `e6e5 h5h6 e5f5 h6h7 f5g4 h7h8q g4g3 g7f6 g3g2 f6f5 g2f2 h8f8`
- Reference played-move score: `-3491`, delta `+4`
- Reference played-move PV: `h5h6 d5e4 h6h7 e4f3 h7h8q f3g3 h8h1 g3g4 h1h2 g4f3`

### 20. Game 76, ply 189: d4d3 (Kd3)

- Categories: `engine-loss, eval-swing`
- Score: `-1129 -> -1289` for current-eval, delta `-160`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -156, piece_square -6, center_control -4, king_safety +1, space +5
- Result: `1-0` `white_win:checkmate`
- FEN before: `1Q6/8/2K5/P7/3k4/8/8/8 b - - 2 95`

- Engine re-search: `d4d3` score `-1902`
- Engine re-search PV: `d4d3 a5a6 d3e3 a6a7 e3e4`

- Engine played-move constrained score: `-1902`
- Engine played-move PV: `d4d3 a5a6 d3e3 a6a7 e3e4`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-1902`, delta `+0`
- Engine reference-move PV: `d4e3 a5a6 e3d3 a6a7 d3e4`
- Engine reference-move SEE: `0`

- Reference bestmove: `d4e3` score `-899986`
- Reference PV: `d4e3 b8d6 e3f2 a5a6 f2f1 a6a7 f1g2 d6d5 g2g3 d5b3 g3f2 a7a8q f2g2 b3b2 g2h3`
- Reference played-move score: `-899989`, delta `+3`
- Reference played-move PV: `a5a6 d3e3 a6a7 e3e2 b8b2 e2f3 b2c3 f3g2 c3c2 g2f3 a7a8q f3e3 c2c3 e3e4 a8g8`


## Largest Eval Swings

### 1. Game 58, ply 107: d5h5 (Rh5)

- Categories: `engine-loss, eval-swing, opponent-recapture, recapture-trade, bad-trade-sequence`
- Score: `-253 -> -410` for current-eval, delta `-157`
- Sequence after reply `g4h5`: `-253 -> -751`, delta `-498`
- Engine recaptures available: `none`
- Material: `R` captures `-` for `0` cp
- Component movement: pawn_structure -116, threats -21, piece_square -8, rook_files -7, center_control +1
- Result: `1-0` `white_win:checkmate`
- FEN before: `2R2N2/7P/p4k2/1ppr4/5PK1/6P1/8/8 b - - 0 56`

- Engine re-search: `d5h5` score `-1496`
- Engine re-search PV: `d5h5 g4h5 c5c4 h7h8q f6e7`

- Engine played-move constrained score: `-1496`
- Engine played-move PV: `d5h5 g4h5 c5c4 h7h8q f6e7`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-1496`, delta `+0`
- Engine reference-move PV: `d5h5 g4h5 c5c4 h7h8q f6e7`
- Engine reference-move SEE: `0`

- Reference bestmove: `d5h5` score `-1053`
- Reference PV: `d5h5 g4h5 f6f5 c8c5 f5e4 f8e6 b5b4 f4f5 a6a5 e6g5 e4d4 c5a5`

### 2. Game 76, ply 141: f4f3 (f3)

- Categories: `engine-loss, eval-swing, opponent-recapture, recapture-trade, bad-trade-sequence`
- Score: `+148 -> -36` for current-eval, delta `-184`
- Sequence after reply `e1f3`: `+148 -> -274`, delta `-422`
- Engine recaptures available: `none`
- Material: `P` captures `-` for `0` cp
- Component movement: pawn_structure -174, piece_square -9, threats -2, pawn_dynamics -2, safe_mobility +1, space +2
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/pK6/1pP5/1P2k3/P4p2/6p1/8/4N3 b - - 1 71`

- Engine re-search: `e5d4` score `-773`
- Engine re-search PV: `e5d4 c6c7 d4e3 c7c8q e3f2`

- Engine played-move constrained score: `-778`
- Engine played-move PV: `f4f3 e1f3 e5e4 f3e1 g3g2`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-786`, delta `-8`
- Engine reference-move PV: `e5e4 c6c7 e4e3 c7c8q e3f2`
- Engine reference-move SEE: `0`

- Reference bestmove: `e5e4` score `-558`
- Reference PV: `e5e4 c6c7 f4f3 e1f3 e4f3 c7c8r g3g2 c8c1 f3e3 c1e1 e3d2 e1a1`
- Reference played-move score: `-704`, delta `+146`
- Reference played-move PV: `e1f3 e5f4 f3g1 g3g2 c6c7 f4g3 b7b8 g3h2`

### 3. Game 75, ply 158: d1e2 (Ke2)

- Categories: `engine-loss, eval-swing`
- Score: `-461 -> -755` for current-eval, delta `-294`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -321, center_control +2, piece_square +25
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/8/8/8/8/1pk5/3p4/3K4 w - - 0 80`

- Engine re-search: `d1e2` score `-1928`
- Engine re-search PV: `d1e2 b3b2 e2e3 d2d1q e3e4`

- Engine played-move constrained score: `-1928`
- Engine played-move PV: `d1e2 b3b2 e2e3 d2d1q e3e4`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-1928`, delta `+0`
- Engine reference-move PV: `d1e2 b3b2 e2e3 d2d1q e3e4`
- Engine reference-move SEE: `0`

- Reference bestmove: `d1e2` score `-4287`
- Reference PV: `d1e2 b3b2 e2f2 b2b1q f2e3 b1g1 e3f3 g1h1 f3f4 h1h4 f4f5 h4h5 f5e6`

### 4. Game 18, ply 187: g4g3 (Kg3)

- Categories: `eval-swing`
- Score: `+1627 -> +1324` for current-eval, delta `-303`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -302, mobility -2, safe_mobility -1, space +2
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/8/p1p5/1p2P3/5nk1/3r4/8/6K1 b - - 5 96`

- Engine re-search: `g4g3` score `899997`
- Engine re-search PV: `g4g3 g1f1 d3d1`

- Engine played-move constrained score: `899997`
- Engine played-move PV: `g4g3 g1f1 d3d1`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `899997`, delta `+0`
- Engine reference-move PV: `g4g3 g1f1 d3d1`
- Engine reference-move SEE: `0`

- Reference bestmove: `g4g3` score `899998`
- Reference PV: `g4g3 g1f1 d3d1`

### 5. Game 20, ply 120: f7g8 (Kg8)

- Categories: `engine-loss, eval-swing`
- Score: `-45 -> -306` for current-eval, delta `-261`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -243, piece_square -23, king_safety +5
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/1p1KPkp1/1P5p/p4p1P/7P/B7/8/8 b - - 2 62`

- Engine re-search: `f7g8` score `-899992`
- Engine re-search PV: `f7g8 e7e8q g8h7 a3b2 a5a4`

- Engine played-move constrained score: `-899992`
- Engine played-move PV: `f7g8 e7e8q g8h7 a3b2 a5a4`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-899992`, delta `+0`
- Engine reference-move PV: `f7g8 e7e8q g8h7 a3b2 a5a4`
- Engine reference-move SEE: `0`

- Reference bestmove: `f7g8` score `-899996`
- Reference PV: `f7g8 a3b2 g8h7 e7e8q f5f4 e8g6 h7h8 g6g7`

### 6. Game 75, ply 160: e2f2 (Kf2)

- Categories: `engine-loss, eval-swing`
- Score: `-833 -> -1080` for current-eval, delta `-247`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -246, center_control -1
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/8/8/8/8/2k5/1p1pK3/8 w - - 0 81`

- Engine re-search: `e2e3` score `-1934`
- Engine re-search PV: `e2e3 d2d1q e3e4 d1d6 e4f3`

- Engine played-move constrained score: `-1946`
- Engine played-move PV: `e2f2 b2b1q f2e3 b1f5 e3e2`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-1934`, delta `+12`
- Engine reference-move PV: `e2e3 d2d1q e3e4 d1d6 e4f3`
- Engine reference-move SEE: `0`

- Reference bestmove: `e2e3` score `-899994`
- Reference PV: `e2e3 b2b1q e3f4 d2d1q f4e5 b1b6 e5f4 b6d4 f4g5 d1g4 g5h6 d4h8`
- Reference played-move score: `-899995`, delta `+1`
- Reference played-move PV: `b2b1q f2f3 d2d1q f3f4 d1d4 f4g5 b1g1 g5h6 d4h4`

### 7. Game 5, ply 196: a2a3 (Ka3)

- Categories: `engine-loss, eval-swing`
- Score: `-671 -> -901` for current-eval, delta `-230`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -229, piece_square -1
- Result: `0-1` `black_win:checkmate`
- FEN before: `7b/8/8/8/8/8/Kpk5/8 w - - 46 100`

- Engine re-search: `a2a3` score `-820514`
- Engine re-search PV: `a2a3 b2b1r a3a4 h8c3 a4a3`

- Engine played-move constrained score: `-820514`
- Engine played-move PV: `a2a3 b2b1r a3a4 h8c3 a4a3`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-820514`, delta `+0`
- Engine reference-move PV: `a2a3 b2b1r a3a4 h8c3 a4a3`
- Engine reference-move SEE: `0`

- Reference bestmove: `a2a3` score `-899997`
- Reference PV: `a2a3 b2b1q a3a4 b1b6 a4a3 b6a5`

### 8. Game 13, ply 148: e4d3 (Kd3)

- Categories: `engine-loss, eval-swing`
- Score: `-731 -> -951` for current-eval, delta `-220`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -205, piece_square -9, space -4, center_control -4, threats +2
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/8/8/8/4K3/6kp/4p3/8 w - - 2 78`

- Engine re-search: `e4d3` score `-1891`
- Engine re-search PV: `e4d3 e2e1q d3d4 h3h2 d4d3`

- Engine played-move constrained score: `-1891`
- Engine played-move PV: `e4d3 e2e1q d3d4 h3h2 d4d3`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-1907`, delta `-16`
- Engine reference-move PV: `e4e5 e2e1q e5d4 h3h2 d4d5`
- Engine reference-move SEE: `0`

- Reference bestmove: `e4e5` score `-4293`
- Reference PV: `e4e5 e2e1q e5f6 h3h2 f6f7 h2h1q f7g7 e1e6`
- Reference played-move score: `-4304`, delta `+11`
- Reference played-move PV: `e2e1q d3c4 h3h2 c4b5 e1e2 b5c5 e2e7 c5d5 e7d7 d5c5 d7c7 c5d5 h2h1q d5d4`

### 9. Game 58, ply 109: f6f5 (Kf5)

- Categories: `engine-loss, eval-swing`
- Score: `-751 -> -961` for current-eval, delta `-210`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -220, king_safety -3, center_control +4, piece_square +7
- Result: `1-0` `white_win:checkmate`
- FEN before: `2R2N2/7P/p4k2/1pp4K/5P2/6P1/8/8 b - - 0 57`

- Engine re-search: `f6f5` score `-1793`
- Engine re-search PV: `f6f5 c8c5 f5e4 f8e6 e4e3`

- Engine played-move constrained score: `-1793`
- Engine played-move PV: `f6f5 c8c5 f5e4 f8e6 e4e3`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-1793`, delta `+0`
- Engine reference-move PV: `f6f5 c8c5 f5e4 f8e6 e4e3`
- Engine reference-move SEE: `0`

- Reference bestmove: `f6f5` score `-1078`
- Reference PV: `f6f5 c8c5 f5e4 f4f5 b5b4 f5f6 b4b3 f6f7 b3b2 f8e6 b2b1b f7f8r`

### 10. Game 31, ply 90: e3f4 (Kf4)

- Categories: `engine-loss, eval-swing`
- Score: `-616 -> -824` for current-eval, delta `-208`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -190, threats -17, center_control -3, piece_square -2, space +2, king_safety +5
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/k5P1/p7/4P3/3P4/1p1qK2P/4b3/8 w - - 1 47`

- Engine re-search: `e3f4` score `-1772`
- Engine re-search PV: `e3f4 d3f3 f4g5 b3b2 g7g8q`

- Engine played-move constrained score: `-1772`
- Engine played-move PV: `e3f4 d3f3 f4g5 b3b2 g7g8q`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-1772`, delta `+0`
- Engine reference-move PV: `e3f4 b3b2 g7g8q d3f3 f4g5`
- Engine reference-move SEE: `0`

- Reference bestmove: `e3f4` score `-1203`
- Reference PV: `e3f4 b3b2 g7g8q d3f3 f4g5 f3g3 g5h6 g3g8 d4d5 b2b1q`

### 11. Game 13, ply 132: d3d4 (Kd4)

- Categories: `engine-loss, eval-swing`
- Score: `-172 -> -369` for current-eval, delta `-197`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -213, center_control +4, piece_square +8
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/8/8/8/7p/3Kp3/5k1P/8 w - - 2 70`

- Engine re-search: `d3d4` score `-901`
- Engine re-search PV: `d3d4 e3e2 d4c5 e2e1q c5d6`

- Engine played-move constrained score: `-901`
- Engine played-move PV: `d3d4 e3e2 d4c5 e2e1q c5d6`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-901`, delta `+0`
- Engine reference-move PV: `d3d4 e3e2 d4c5 e2e1q c5d6`
- Engine reference-move SEE: `0`

- Reference bestmove: `d3d4` score `-3530`
- Reference PV: `d3d4 e3e2 d4d5 h4h3 d5e4 f2g2 e4d4 g2h2`

### 12. Game 86, ply 167: e4e3 (Ke3)

- Categories: `engine-loss, eval-swing`
- Score: `-192 -> -388` for current-eval, delta `-196`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -188, piece_square -8, center_control -4, space +4
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/8/5K2/7P/4k3/8/8/8 b - - 10 85`

- Engine re-search: `e4d4` score `-939`
- Engine re-search PV: `e4d4 h5h6 d4e4 h6h7 e4d4`

- Engine played-move constrained score: `-939`
- Engine played-move PV: `e4e3 h5h6 e3f3 h6h7 f3e4`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-939`, delta `+0`
- Engine reference-move PV: `e4f3 h5h6 f3e4 h6h7 e4d4`
- Engine reference-move SEE: `0`

- Reference bestmove: `e4f3` score `-3487`
- Reference PV: `e4f3 h5h6 f3g4 h6h7 g4g3 h7h8q g3g2 h8f8 g2g3 f8a8 g3h2`
- Reference played-move score: `-3494`, delta `+7`
- Reference played-move PV: `h5h6 e3e2 h6h7 e2f1 h7h8q f1g2 f6f7 g2g3 f7g6 g3f3`

### 13. Game 20, ply 122: a5a4 (a4)

- Categories: `engine-loss, eval-swing`
- Score: `-38 -> -231` for current-eval, delta `-193`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `P` captures `-` for `0` cp
- Component movement: pawn_structure -180, piece_square -13
- Result: `1-0` `white_win:checkmate`
- FEN before: `6k1/1p1KP1p1/1P5p/p4p1P/7P/8/1B6/8 b - - 4 63`

- Engine re-search: `g8h8` score `-899994`
- Engine re-search PV: `g8h8 e7e8q h8h7 e8g6 h7g8`

- Engine played-move constrained score: `-899994`
- Engine played-move PV: `a5a4 e7e8q g8h7 e8g6 h7g8`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-899994`, delta `+0`
- Engine reference-move PV: `a5a4 e7e8q g8h7 e8g6 h7g8`
- Engine reference-move SEE: `0`

- Reference bestmove: `a5a4` score `-899997`
- Reference PV: `a5a4 e7e8q g8h7 e8g6 h7h8 g6g7`

### 14. Game 20, ply 106: c8f8 (Rf8)

- Categories: `engine-loss, eval-swing`
- Score: `+305 -> +118` for current-eval, delta `-187`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `R` captures `-` for `0` cp
- Component movement: pawn_structure -178, rook_files -6, threats -5, king_safety +2
- Result: `1-0` `white_win:checkmate`
- FEN before: `2r3k1/1pP1K1p1/1P2Pp1p/p1B4P/8/7P/8/8 b - - 1 55`

- Engine re-search: `c8a8` score `-8`
- Engine re-search PV: `c8a8 e7d7 f6f5 c5f2 a5a4`

- Engine played-move constrained score: `-29`
- Engine played-move PV: `c8f8 e7d7 f8a8 c5a3 f6f5`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-8`, delta `+21`
- Engine reference-move PV: `c8a8 e7d7 f6f5 c5f2 a5a4`
- Engine reference-move SEE: `0`

- Reference bestmove: `c8a8` score `-645`
- Reference PV: `c8a8 e7d7 f6f5 c7c8r a8c8 d7c8 a5a4 c8d7 f5f4 e6e7 f4f3 e7e8r g8h7`
- Reference played-move score: `-647`, delta `+2`
- Reference played-move PV: `e7d7 f8a8 c7c8q a8c8 d7c8 g8h7 c8d7 a5a4 e6e7`

### 15. Game 20, ply 110: c8f8 (Rf8)

- Categories: `engine-loss, eval-swing`
- Score: `+274 -> +87` for current-eval, delta `-187`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `R` captures `-` for `0` cp
- Component movement: pawn_structure -178, rook_files -6, threats -5, king_safety +2
- Result: `1-0` `white_win:checkmate`
- FEN before: `2r3k1/1pP1K1p1/1P2Pp1p/p6P/7P/B7/8/8 b - - 2 57`

- Engine re-search: `c8f8` score `3`
- Engine re-search PV: `c8f8 e7d7 f8a8 c7c8q a8c8`

- Engine played-move constrained score: `3`
- Engine played-move PV: `c8f8 e7d7 f8a8 c7c8q a8c8`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `3`, delta `+0`
- Engine reference-move PV: `c8f8 e7d7 f8a8 c7c8q a8c8`
- Engine reference-move SEE: `0`

- Reference bestmove: `c8f8` score `-707`
- Reference PV: `c8f8 e7d7 f8a8 c7c8r a8c8 d7c8 a5a4 e6e7 g8f7 c8d7 f7g8`

### 16. Game 76, ply 143: e5e4 (Ke4)

- Categories: `engine-loss, eval-swing`
- Score: `-274 -> -458` for current-eval, delta `-184`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -197, piece_square -1, space +3, threats +9
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/pK6/1pP5/1P2k3/P7/5Np1/8/8 b - - 0 72`

- Engine re-search: `e5e4` score `-1000`
- Engine re-search PV: `e5e4 f3g1 e4e3 b7a7 e3f2`

- Engine played-move constrained score: `-1000`
- Engine played-move PV: `e5e4 f3g1 e4e3 b7a7 e3f2`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-1000`, delta `+0`
- Engine reference-move PV: `e5f4 f3g1 f4e3 b7a7 e3f2`
- Engine reference-move SEE: `0`

- Reference bestmove: `e5f4` score `-685`
- Reference PV: `e5f4 f3g1 g3g2 c6c7 f4g3 c7c8r g3h2 c8c1 h2g3 c1b1`
- Reference played-move score: `-650`, delta `-35`
- Reference played-move PV: `f3g1 g3g2 c6c7 e4e3 c7c8r e3f4 c8f8 f4g4 f8f2`

### 17. Game 61, ply 87: b4a6 (Na6)

- Categories: `eval-swing`
- Score: `+3100 -> +2892` for current-eval, delta `-208`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `N` captures `-` for `0` cp
- Component movement: pawn_structure -174, piece_square -23, mobility -5, bishop_quality -5, space +1, center_control +2
- Result: `1-0` `white_win:checkmate`
- FEN before: `3Q4/k7/8/1B6/1N3P2/2K5/P5PP/7R w - - 1 44`

- Engine re-search: `b4c6` score `899997`
- Engine re-search PV: `b4c6 a7b7 d8b8`

- Engine played-move constrained score: `899997`
- Engine played-move PV: `b4a6 a7b7 d8b8`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `899997`, delta `+0`
- Engine reference-move PV: `b4c6 a7b7 d8b8`
- Engine reference-move SEE: `0`

- Reference bestmove: `b4c6` score `899998`
- Reference PV: `b4c6 a7b7 d8b8`
- Reference played-move score: `899999`, delta `-1`
- Reference played-move PV: `a7b7 d8b8`

### 18. Game 86, ply 165: d4e4 (Ke4)

- Categories: `engine-loss, eval-swing`
- Score: `-172 -> -352` for current-eval, delta `-180`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -180
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/6K1/8/7P/3k4/8/8/8 b - - 8 84`

- Engine re-search: `d4d5` score `-909`
- Engine re-search PV: `d4d5 h5h6 d5e6 h6h7 e6e5`

- Engine played-move constrained score: `-939`
- Engine played-move PV: `d4e4 g7f6 e4e3 h5h6 e3f3`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-939`, delta `+0`
- Engine reference-move PV: `d4e4 g7f6 e4f3 h5h6 f3e4`
- Engine reference-move SEE: `0`

- Reference bestmove: `d4e4` score `-3490`
- Reference PV: `d4e4 g7f6 e4f3 h5h6 f3g2 h6h7 g2f2 h7h8q f2g2 f6g7 g2f2 h8h2 f2f1`

### 19. Game 6, ply 117: c7d6 (Kd6)

- Categories: `engine-loss, eval-swing`
- Score: `-273 -> -451` for current-eval, delta `-178`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -203, mobility -6, space -5, threats -4, king_safety +9, piece_square +22
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/1Rk5/P7/8/P3p3/2p1Kp2/8/8 b - - 1 60`

- Engine re-search: `c7c6` score `-492`
- Engine re-search PV: `c7c6 b7b3 c3c2 b3c3 c6b6`

- Engine played-move constrained score: `-611`
- Engine played-move PV: `c7d6 b7b3 d6c7 b3c3 c7b6`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-492`, delta `+119`
- Engine reference-move PV: `c7c6 b7b3 c3c2 b3c3 c6b6`
- Engine reference-move SEE: `0`

- Reference bestmove: `c7c6` score `-562`
- Reference PV: `c7c6 b7b3 c3c2 b3c3 c6b6 c3c2 b6a6 c2b2`
- Reference played-move score: `-583`, delta `+21`
- Reference played-move PV: `b7b3 d6c7 b3c3 c7b6 c3c8 b6a6 c8c1 a6b7`

### 20. Game 86, ply 161: d6c5 (Kc5)

- Categories: `engine-loss, eval-swing`
- Score: `-190 -> -366` for current-eval, delta `-176`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -180, space +4
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/6K1/3k4/7P/8/8/8/8 b - - 4 82`

- Engine re-search: `d6e6` score `-909`
- Engine re-search PV: `d6e6 h5h6 e6e5 h6h7 e5f5`

- Engine played-move constrained score: `-939`
- Engine played-move PV: `d6c5 h5h6 c5b5 h6h7 b5c4`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-909`, delta `+30`
- Engine reference-move PV: `d6e6 h5h6 e6e5 h6h7 e5f5`
- Engine reference-move SEE: `0`

- Reference bestmove: `d6e6` score `-3487`
- Reference PV: `d6e6 h5h6 e6f5 h6h7 f5g4 h7h8q g4g3 g7f6 g3f3 f6f5 f3f2 h8f8 f2g3`
- Reference played-move score: `-3525`, delta `+38`
- Reference played-move PV: `h5h6 c5d4 h6h7 d4e3 h7h8q e3f2 g7f6 f2g2`
