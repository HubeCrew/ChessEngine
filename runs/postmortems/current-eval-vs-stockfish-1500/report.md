# Gauntlet Postmortem

## Summary

- `engine`: current-eval
- `games_seen`: 100
- `engine_moves_analyzed`: 4779
- `flagged_events`: 120
- `loss_games_seen`: 41
- `average_delta_cp`: 99.28
- `worst_delta_cp`: -303
- `captures`: 1138
- `equal_value_captures`: 406
- `queen_trades`: 40
- `eval_swings`: 32
- `engine_reference_scores`: 120
- `engine_prefers_reference`: 19
- `engine_prefers_played`: 36
- `engine_ties_reference`: 65
- `engine_played_negative_see`: 32
- `engine_reference_negative_see`: 18

## Highest-Severity Events

### 1. Game 58, ply 107: d5h5 (Rh5)

- Categories: `engine-loss, eval-swing, opponent-recapture, recapture-trade, bad-trade-sequence`
- Score: `-243 -> -401` for current-eval, delta `-158`
- Sequence after reply `g4h5`: `-243 -> -738`, delta `-495`
- Engine recaptures available: `none`
- Material: `R` captures `-` for `0` cp
- Component movement: pawn_structure -116, threats -21, piece_square -9, rook_files -7, center_control +1
- Result: `1-0` `white_win:checkmate`
- FEN before: `2R2N2/7P/p4k2/1ppr4/5PK1/6P1/8/8 b - - 0 56`

- Engine re-search: `d5h5` score `-1489`

- Engine played-move constrained score: `-1489`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-1489`, delta `+0`
- Engine reference-move SEE: `0`

- Reference bestmove: `d5h5` score `-1053`

### 2. Game 76, ply 141: f4f3 (f3)

- Categories: `engine-loss, eval-swing, opponent-recapture, recapture-trade, bad-trade-sequence`
- Score: `+174 -> -11` for current-eval, delta `-185`
- Sequence after reply `e1f3`: `+174 -> -256`, delta `-430`
- Engine recaptures available: `none`
- Material: `P` captures `-` for `0` cp
- Component movement: pawn_structure -174, piece_square -10, threats -2, pawn_dynamics -2, safe_mobility +1, space +2
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/pK6/1pP5/1P2k3/P4p2/6p1/8/4N3 b - - 1 71`

- Engine re-search: `e5e4` score `-772`

- Engine played-move constrained score: `-841`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-772`, delta `+69`
- Engine reference-move SEE: `0`

- Reference bestmove: `e5e4` score `-558`
- Reference played-move score: `-704`, delta `+146`

### 3. Game 75, ply 158: d1e2 (Ke2)

- Categories: `engine-loss, eval-swing`
- Score: `-469 -> -758` for current-eval, delta `-289`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -321, center_control +2, piece_square +30
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/8/8/8/8/1pk5/3p4/3K4 w - - 0 80`

- Engine re-search: `d1e2` score `-1929`

- Engine played-move constrained score: `-1929`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-1929`, delta `+0`
- Engine reference-move SEE: `0`

- Reference bestmove: `d1e2` score `-4287`

### 4. Game 18, ply 187: g4g3 (Kg3)

- Categories: `eval-swing`
- Score: `+1648 -> +1345` for current-eval, delta `-303`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -302, mobility -2, safe_mobility -1, space +2
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/8/p1p5/1p2P3/5nk1/3r4/8/6K1 b - - 5 96`

- Engine re-search: `g4g3` score `899997`

- Engine played-move constrained score: `899997`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `899997`, delta `+0`
- Engine reference-move SEE: `0`

- Reference bestmove: `g4g3` score `899998`

### 5. Game 20, ply 120: f7g8 (Kg8)

- Categories: `engine-loss, eval-swing`
- Score: `-10 -> -275` for current-eval, delta `-265`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -243, piece_square -27, king_safety +5
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/1p1KPkp1/1P5p/p4p1P/7P/B7/8/8 b - - 2 62`

- Engine re-search: `f7g8` score `-899992`

- Engine played-move constrained score: `-899992`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-899992`, delta `+0`
- Engine reference-move SEE: `0`

- Reference bestmove: `f7g8` score `-899996`

### 6. Game 75, ply 160: e2f2 (Kf2)

- Categories: `engine-loss, eval-swing`
- Score: `-836 -> -1083` for current-eval, delta `-247`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -246, center_control -1
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/8/8/8/8/2k5/1p1pK3/8 w - - 0 81`

- Engine re-search: `e2e3` score `-1935`

- Engine played-move constrained score: `-1947`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-1935`, delta `+12`
- Engine reference-move SEE: `0`

- Reference bestmove: `e2e3` score `-899994`
- Reference played-move score: `-899995`, delta `+1`

### 7. Game 5, ply 196: a2a3 (Ka3)

- Categories: `engine-loss, eval-swing`
- Score: `-675 -> -905` for current-eval, delta `-230`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -229, piece_square -1
- Result: `0-1` `black_win:checkmate`
- FEN before: `7b/8/8/8/8/8/Kpk5/8 w - - 46 100`

- Engine re-search: `a2a3` score `-820519`

- Engine played-move constrained score: `-820519`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-820519`, delta `+0`
- Engine reference-move SEE: `0`

- Reference bestmove: `a2a3` score `-899997`

### 8. Game 13, ply 148: e4d3 (Kd3)

- Categories: `engine-loss, eval-swing`
- Score: `-725 -> -946` for current-eval, delta `-221`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -205, piece_square -10, space -4, center_control -4, threats +2
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/8/8/8/4K3/6kp/4p3/8 w - - 2 78`

- Engine re-search: `e4d3` score `-1884`

- Engine played-move constrained score: `-1884`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-1904`, delta `-20`
- Engine reference-move SEE: `0`

- Reference bestmove: `e4e5` score `-4293`
- Reference played-move score: `-4304`, delta `+11`

### 9. Game 58, ply 109: f6f5 (Kf5)

- Categories: `engine-loss, eval-swing`
- Score: `-738 -> -947` for current-eval, delta `-209`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -220, king_safety -3, center_control +4, piece_square +8
- Result: `1-0` `white_win:checkmate`
- FEN before: `2R2N2/7P/p4k2/1pp4K/5P2/6P1/8/8 b - - 0 57`

- Engine re-search: `f6f5` score `-1788`

- Engine played-move constrained score: `-1788`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-1788`, delta `+0`
- Engine reference-move SEE: `0`

- Reference bestmove: `f6f5` score `-1078`

### 10. Game 31, ply 90: e3f4 (Kf4)

- Categories: `engine-loss, eval-swing`
- Score: `-603 -> -811` for current-eval, delta `-208`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -190, threats -17, center_control -3, piece_square -2, space +2, king_safety +5
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/k5P1/p7/4P3/3P4/1p1qK2P/4b3/8 w - - 1 47`

- Engine re-search: `e3f4` score `-1759`

- Engine played-move constrained score: `-1759`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-1759`, delta `+0`
- Engine reference-move SEE: `0`

- Reference bestmove: `e3f4` score `-1203`

### 11. Game 86, ply 167: e4e3 (Ke3)

- Categories: `engine-loss, eval-swing`
- Score: `-191 -> -389` for current-eval, delta `-198`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -188, piece_square -10, center_control -4, space +4
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/8/5K2/7P/4k3/8/8/8 b - - 10 85`

- Engine re-search: `e4d4` score `-936`

- Engine played-move constrained score: `-936`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-936`, delta `+0`
- Engine reference-move SEE: `0`

- Reference bestmove: `e4d3` score `-3487`
- Reference played-move score: `-3492`, delta `+5`

### 12. Game 20, ply 122: a5a4 (a4)

- Categories: `engine-loss, eval-swing`
- Score: `-9 -> -204` for current-eval, delta `-195`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `P` captures `-` for `0` cp
- Component movement: pawn_structure -180, piece_square -15
- Result: `1-0` `white_win:checkmate`
- FEN before: `6k1/1p1KP1p1/1P5p/p4p1P/7P/8/1B6/8 b - - 4 63`

- Engine re-search: `g8h8` score `-899994`

- Engine played-move constrained score: `-899994`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-899994`, delta `+0`
- Engine reference-move SEE: `0`

- Reference bestmove: `a5a4` score `-899997`

### 13. Game 13, ply 132: d3d4 (Kd4)

- Categories: `engine-loss, eval-swing`
- Score: `-160 -> -355` for current-eval, delta `-195`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -213, center_control +4, piece_square +10
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/8/8/8/7p/3Kp3/5k1P/8 w - - 2 70`

- Engine re-search: `d3d4` score `-890`

- Engine played-move constrained score: `-890`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-961`, delta `-71`
- Engine reference-move SEE: `0`

- Reference bestmove: `h2h3` score `-675`
- Reference played-move score: `-759`, delta `+84`

### 14. Game 61, ply 87: b4a6 (Na6)

- Categories: `eval-swing`
- Score: `+3140 -> +2928` for current-eval, delta `-212`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `N` captures `-` for `0` cp
- Component movement: pawn_structure -174, piece_square -27, mobility -5, bishop_quality -5, space +1, center_control +2
- Result: `1-0` `white_win:checkmate`
- FEN before: `3Q4/k7/8/1B6/1N3P2/2K5/P5PP/7R w - - 1 44`

- Engine re-search: `b4c6` score `899997`

- Engine played-move constrained score: `899997`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `899997`, delta `+0`
- Engine reference-move SEE: `0`

- Reference bestmove: `d8c7` score `899998`
- Reference played-move score: `899999`, delta `-1`

### 15. Game 20, ply 106: c8f8 (Rf8)

- Categories: `engine-loss, eval-swing`
- Score: `+329 -> +142` for current-eval, delta `-187`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `R` captures `-` for `0` cp
- Component movement: pawn_structure -178, rook_files -6, threats -5, king_safety +2
- Result: `1-0` `white_win:checkmate`
- FEN before: `2r3k1/1pP1K1p1/1P2Pp1p/p1B4P/8/7P/8/8 b - - 1 55`

- Engine re-search: `c8a8` score `17`

- Engine played-move constrained score: `0`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-220`, delta `-220`
- Engine reference-move SEE: `0`

- Reference bestmove: `g7g5` score `-697`
- Reference played-move score: `-674`, delta `-23`

### 16. Game 20, ply 110: c8f8 (Rf8)

- Categories: `engine-loss, eval-swing`
- Score: `+304 -> +117` for current-eval, delta `-187`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `R` captures `-` for `0` cp
- Component movement: pawn_structure -178, rook_files -6, threats -5, king_safety +2
- Result: `1-0` `white_win:checkmate`
- FEN before: `2r3k1/1pP1K1p1/1P2Pp1p/p6P/7P/B7/8/8 b - - 2 57`

- Engine re-search: `c8f8` score `37`

- Engine played-move constrained score: `37`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `20`, delta `-17`
- Engine reference-move SEE: `0`

- Reference bestmove: `c8a8` score `-611`
- Reference played-move score: `-684`, delta `+73`

### 17. Game 76, ply 143: e5e4 (Ke4)

- Categories: `engine-loss, eval-swing`
- Score: `-256 -> -440` for current-eval, delta `-184`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -197, piece_square -1, space +3, threats +9
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/pK6/1pP5/1P2k3/P7/5Np1/8/8 b - - 0 72`

- Engine re-search: `e5e4` score `-1012`

- Engine played-move constrained score: `-1012`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-1012`, delta `+0`
- Engine reference-move SEE: `0`

- Reference bestmove: `e5f4` score `-647`
- Reference played-move score: `-637`, delta `-10`

### 18. Game 86, ply 165: d4e4 (Ke4)

- Categories: `engine-loss, eval-swing`
- Score: `-166 -> -346` for current-eval, delta `-180`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -180
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/6K1/8/7P/3k4/8/8/8 b - - 8 84`

- Engine re-search: `d4d5` score `-902`

- Engine played-move constrained score: `-936`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-936`, delta `+0`
- Engine reference-move SEE: `0`

- Reference bestmove: `d4e4` score `-3497`

### 19. Game 86, ply 161: d6c5 (Kc5)

- Categories: `engine-loss, eval-swing`
- Score: `-186 -> -362` for current-eval, delta `-176`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -180, space +4
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/6K1/3k4/7P/8/8/8/8 b - - 4 82`

- Engine re-search: `d6e6` score `-902`

- Engine played-move constrained score: `-902`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-902`, delta `+0`
- Engine reference-move SEE: `0`

- Reference bestmove: `d6e6` score `-3483`
- Reference played-move score: `-3497`, delta `+14`

### 20. Game 6, ply 117: c7d6 (Kd6)

- Categories: `engine-loss, eval-swing`
- Score: `-278 -> -452` for current-eval, delta `-174`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -203, mobility -6, space -5, threats -4, king_safety +9, piece_square +26
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/1Rk5/P7/8/P3p3/2p1Kp2/8/8 b - - 1 60`

- Engine re-search: `c7c6` score `-505`

- Engine played-move constrained score: `-622`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-614`, delta `+8`
- Engine reference-move SEE: `0`

- Reference bestmove: `c7c8` score `-579`
- Reference played-move score: `-583`, delta `+4`

### 21. Game 86, ply 157: e6d5 (Kd5)

- Categories: `engine-loss, eval-swing`
- Score: `-178 -> -348` for current-eval, delta `-170`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -188, space +5, piece_square +10
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/6K1/4k3/7P/8/8/8/8 b - - 0 80`

- Engine re-search: `e6f5` score `-902`

- Engine played-move constrained score: `-902`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-902`, delta `+0`
- Engine reference-move SEE: `0`

- Reference bestmove: `e6e5` score `-3491`
- Reference played-move score: `-3491`, delta `+0`

### 22. Game 76, ply 189: d4d3 (Kd3)

- Categories: `engine-loss, eval-swing`
- Score: `-1127 -> -1289` for current-eval, delta `-162`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -156, piece_square -8, center_control -4, king_safety +1, space +5
- Result: `1-0` `white_win:checkmate`
- FEN before: `1Q6/8/2K5/P7/3k4/8/8/8 b - - 2 95`

- Engine re-search: `d4d3` score `-1900`

- Engine played-move constrained score: `-1900`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-1900`, delta `+0`
- Engine reference-move SEE: `0`

- Reference bestmove: `d4c3` score `-4252`
- Reference played-move score: `-4247`, delta `-5`

### 23. Game 20, ply 94: c8a8 (Ra8)

- Categories: `engine-loss, eval-swing`
- Score: `+159 -> +1` for current-eval, delta `-158`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `R` captures `-` for `0` cp
- Component movement: pawn_structure -147, rook_files -6, piece_square -5
- Result: `1-0` `white_win:checkmate`
- FEN before: `2r3k1/1pP3p1/pP1BPp1p/7P/3K4/8/7P/8 b - - 7 49`

- Engine re-search: `c8a8` score `102`

- Engine played-move constrained score: `102`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `117`, delta `+15`
- Engine reference-move SEE: `0`

- Reference bestmove: `g8h8` score `-512`
- Reference played-move score: `-543`, delta `+31`

### 24. Game 76, ply 187: e4d4 (Kd4)

- Categories: `engine-loss, eval-swing`
- Score: `-1104 -> -1254` for current-eval, delta `-150`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -150
- Result: `1-0` `white_win:checkmate`
- FEN before: `1Q6/8/1K6/P7/4k3/8/8/8 b - - 0 94`

- Engine re-search: `e4d5` score `-1903`

- Engine played-move constrained score: `-1903`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-1903`, delta `+0`
- Engine reference-move SEE: `0`

- Reference bestmove: `e4f5` score `-899989`
- Reference played-move score: `-899990`, delta `+1`

### 25. Game 31, ply 114: d6c7 (Kc7)

- Categories: `engine-loss, eval-swing`
- Score: `-1294 -> -1443` for current-eval, delta `-149`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -135, piece_square -21, center_control -7, space -3, king_safety +17
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/k7/3K4/p7/2b1q2P/8/8/8 w - - 1 59`

- Engine re-search: `d6c7` score `-899994`

- Engine played-move constrained score: `-899994`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-899994`, delta `+0`
- Engine reference-move SEE: `0`

- Reference bestmove: `h4h5` score `-4503`
- Reference played-move score: `-4511`, delta `+8`


## Trade And Simplification Watchlist

### 1. Game 58, ply 107: d5h5 (Rh5)

- Categories: `engine-loss, eval-swing, opponent-recapture, recapture-trade, bad-trade-sequence`
- Score: `-243 -> -401` for current-eval, delta `-158`
- Sequence after reply `g4h5`: `-243 -> -738`, delta `-495`
- Engine recaptures available: `none`
- Material: `R` captures `-` for `0` cp
- Component movement: pawn_structure -116, threats -21, piece_square -9, rook_files -7, center_control +1
- Result: `1-0` `white_win:checkmate`
- FEN before: `2R2N2/7P/p4k2/1ppr4/5PK1/6P1/8/8 b - - 0 56`

- Engine re-search: `d5h5` score `-1489`

- Engine played-move constrained score: `-1489`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-1489`, delta `+0`
- Engine reference-move SEE: `0`

- Reference bestmove: `d5h5` score `-1053`

### 2. Game 76, ply 141: f4f3 (f3)

- Categories: `engine-loss, eval-swing, opponent-recapture, recapture-trade, bad-trade-sequence`
- Score: `+174 -> -11` for current-eval, delta `-185`
- Sequence after reply `e1f3`: `+174 -> -256`, delta `-430`
- Engine recaptures available: `none`
- Material: `P` captures `-` for `0` cp
- Component movement: pawn_structure -174, piece_square -10, threats -2, pawn_dynamics -2, safe_mobility +1, space +2
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/pK6/1pP5/1P2k3/P4p2/6p1/8/4N3 b - - 1 71`

- Engine re-search: `e5e4` score `-772`

- Engine played-move constrained score: `-841`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-772`, delta `+69`
- Engine reference-move SEE: `0`

- Reference bestmove: `e5e4` score `-558`
- Reference played-move score: `-704`, delta `+146`

### 3. Game 63, ply 58: c4f7 (Qf7)

- Categories: `engine-loss, opponent-recapture, recapture-trade, queen-trade-sequence, bad-trade-sequence`
- Score: `+331 -> +336` for current-eval, delta `+5`
- Sequence after reply `f8f7`: `+331 -> -565`, delta `-896`
- Engine recaptures available: `none`
- Material: `Q` captures `-` for `0` cp
- Component movement: threats -69, piece_square -5, space +15, king_safety +37
- Result: `0-1` `black_win:checkmate`
- FEN before: `2k1rr2/1bp4p/1p6/8/1PQ5/3P3P/P1P3PR/5K2 w - - 1 32`

- Engine re-search: `c4f4` score `-899996`

- Engine played-move constrained score: `-899996`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-899996`, delta `+0`
- Engine reference-move SEE: `0`

- Reference bestmove: `c4f7` score `-899998`

### 4. Game 60, ply 38: d4d7 (Qxd7)

- Categories: `engine-loss, opponent-recapture, recapture-trade, queen-trade-sequence, bad-trade-sequence`
- Score: `+503 -> +928` for current-eval, delta `+425`
- Sequence after reply `c6d7`: `+503 -> -106`, delta `-609`
- Engine recaptures available: `none`
- Material: `Q` captures `N` for `320` cp
- Component movement: piece_square -6, space -6, center_control -1, development -1, threats +34, material +320
- Result: `1-0` `white_win:checkmate`
- FEN before: `rk3b1r/p2Np1pp/2Q5/8/2Nq3P/7R/P2nPP2/4K3 b - - 2 21`

- Engine re-search: `d4d7` score `-175`

- Engine played-move constrained score: `-175`
- Engine played-move SEE: `-580`
- Engine reference-move constrained score: `-175`, delta `+0`
- Engine reference-move SEE: `-580`

- Reference bestmove: `d4d7` score `-916`

### 5. Game 14, ply 51: e8e7 (Re7)

- Categories: `engine-loss, opponent-recapture, recapture-trade, queen-trade-sequence, bad-trade-sequence`
- Score: `-183 -> -20` for current-eval, delta `+163`
- Sequence after reply `d7e7`: `-183 -> -694`, delta `-511`
- Engine recaptures available: `none`
- Material: `R` captures `-` for `0` cp
- Component movement: piece_square -4, king_safety +46, threats +89
- Result: `1-0` `white_win:checkmate`
- FEN before: `r3r1k1/pp1Q3p/4pppB/2pp4/2q5/2P2B1P/5PP1/R4RK1 b - - 0 29`

- Engine re-search: `e8e7` score `-899994`

- Engine played-move constrained score: `-899994`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-899994`, delta `+0`
- Engine reference-move SEE: `-400`

- Reference bestmove: `c4f1` score `-899997`
- Reference played-move score: `-899997`, delta `+0`

### 6. Game 65, ply 20: a4b4 (Qxb4)

- Categories: `opponent-recapture, recapture-trade, queen-trade-sequence, bad-trade-sequence`
- Score: `+372 -> +803` for current-eval, delta `+431`
- Sequence after reply `a6b4`: `+372 -> -157`, delta `-529`
- Engine recaptures available: `none`
- Material: `Q` captures `B` for `330` cp
- Component movement: threats -146, rook_files -1, pawn_dynamics -1, king_safety +99, material +330
- Result: `1-0` `white_win:checkmate`
- FEN before: `r1bqk2r/p2n2pp/n3Np2/1N2p3/Qbp5/4B3/PP2PPPP/3RKB1R w Kkq - 1 12`

- Engine re-search: `b5c3` score `469`

- Engine played-move constrained score: `288`
- Engine played-move SEE: `-570`
- Engine reference-move constrained score: `437`, delta `+149`
- Engine reference-move SEE: `0`

- Reference bestmove: `e3d2` score `435`
- Reference played-move score: `327`, delta `+108`

### 7. Game 80, ply 100: e5e8 (Re8)

- Categories: `engine-loss, opponent-recapture, recapture-trade, bad-trade-sequence`
- Score: `+364 -> +395` for current-eval, delta `+31`
- Sequence after reply `c8e8`: `+364 -> -214`, delta `-578`
- Engine recaptures available: `none`
- Material: `R` captures `-` for `0` cp
- Component movement: center_control -2, mobility +6, threats +22
- Result: `1-0` `white_win:checkmate`
- FEN before: `2R4R/kp3pp1/p7/P2pr3/3p4/8/8/3K4 b - - 11 52`

- Engine re-search: `e5e8` score `-899996`

- Engine played-move constrained score: `-899996`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-899996`, delta `+0`
- Engine reference-move SEE: `0`

- Reference bestmove: `e5e1` score `-899998`
- Reference played-move score: `-899998`, delta `+0`

### 8. Game 83, ply 48: f6f8 (Rf8+)

- Categories: `engine-loss, opponent-recapture, recapture-trade, bad-trade-sequence`
- Score: `+649 -> +668` for current-eval, delta `+19`
- Sequence after reply `e8f8`: `+649 -> +72`, delta `-577`
- Engine recaptures available: `none`
- Material: `R` captures `-` for `0` cp
- Component movement: mobility -8, space -3, center_control -3, safe_mobility -2, trade_context +5, king_safety +30
- Result: `0-1` `black_win:checkmate`
- FEN before: `4k1r1/pb5p/5R2/4P3/3p4/3Pn2q/PPP1Q1PP/R5K1 w - - 7 27`

- Engine re-search: `f6e6` score `-338`

- Engine played-move constrained score: `-495`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-338`, delta `+157`
- Engine reference-move SEE: `0`

- Reference bestmove: `f6e6` score `-700`
- Reference played-move score: `-715`, delta `+15`

### 9. Game 4, ply 53: c1h1 (Rh1+)

- Categories: `engine-loss, opponent-recapture, recapture-trade, bad-trade-sequence`
- Score: `+603 -> +570` for current-eval, delta `-33`
- Sequence after reply `a1h1`: `+603 -> +73`, delta `-530`
- Engine recaptures available: `none`
- Material: `R` captures `-` for `0` cp
- Component movement: threats -30, pawn_structure -20, rook_files -19, mobility -8, trade_context +19, king_safety +28
- Result: `1-0` `white_win:checkmate`
- FEN before: `4rk2/3p1p1p/1p1P1p1N/4p3/P1qp4/6QP/5PPK/R1r5 b - - 4 29`

- Engine re-search: `c1h1` score `-899996`

- Engine played-move constrained score: `-899996`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-899996`, delta `+0`
- Engine reference-move SEE: `0`

- Reference bestmove: `c1h1` score `-899998`

### 10. Game 54, ply 63: a2h2 (Rh2+)

- Categories: `engine-loss, opponent-recapture, recapture-trade, bad-trade-sequence`
- Score: `+409 -> +373` for current-eval, delta `-36`
- Sequence after reply `h3h2`: `+409 -> -117`, delta `-526`
- Engine recaptures available: `none`
- Material: `R` captures `-` for `0` cp
- Component movement: threats -24, pawn_structure -14, mobility -4, safe_mobility -2, king_safety +9
- Result: `1-0` `white_win:checkmate`
- FEN before: `7k/p4R1p/1p1p2pB/5p2/7P/6PK/r7/8 b - - 5 35`

- Engine re-search: `a2h2` score `-899996`

- Engine played-move constrained score: `-899996`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-899996`, delta `+0`
- Engine reference-move SEE: `0`

- Reference bestmove: `a2h2` score `-899998`

### 11. Game 83, ply 52: f1f8 (Rf8+)

- Categories: `engine-loss, opponent-recapture, recapture-trade, bad-trade-sequence`
- Score: `+120 -> +179` for current-eval, delta `+59`
- Sequence after reply `e8f8`: `+120 -> -441`, delta `-561`
- Engine recaptures available: `none`
- Material: `R` captures `-` for `0` cp
- Component movement: mobility -4, safe_mobility -1, king_safety +26, threats +31
- Result: `0-1` `black_win:checkmate`
- FEN before: `4k1r1/pb5p/8/4P3/3p4/3Pn2q/PPP1Q1PP/5RK1 w - - 2 29`

- Engine re-search: `f1f2` score `-1071`

- Engine played-move constrained score: `-1533`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-1071`, delta `+462`
- Engine reference-move SEE: `0`

- Reference bestmove: `f1f2` score `-957`
- Reference played-move score: `-899994`, delta `+899037`

### 12. Game 93, ply 32: c5f8 (Qxf8+)

- Categories: `engine-loss, opponent-recapture, recapture-trade, queen-trade-sequence, bad-trade-sequence`
- Score: `+882 -> +1370` for current-eval, delta `+488`
- Sequence after reply `g7f8`: `+882 -> +420`, delta `-462`
- Engine recaptures available: `none`
- Material: `Q` captures `R` for `500` cp
- Component movement: threats -81, piece_square -9, center_control -8, king_safety +23, material +500
- Result: `0-1` `black_win:checkmate`
- FEN before: `5r2/2p2Pkp/2n3p1/2Q5/2P1P3/2P2bPq/P4P1P/RB3RK1 w - - 1 20`

- Engine re-search: `c5f8` score `-899996`

- Engine played-move constrained score: `-899996`
- Engine played-move SEE: `-400`
- Engine reference-move constrained score: `-899996`, delta `+0`
- Engine reference-move SEE: `-400`

- Reference bestmove: `c5f8` score `-899998`

### 13. Game 12, ply 107: h3f3 (Rf3)

- Categories: `engine-loss, opponent-recapture, recapture-trade, bad-trade-sequence`
- Score: `+35 -> +83` for current-eval, delta `+48`
- Sequence after reply `f1f3`: `+35 -> -497`, delta `-532`
- Engine recaptures available: `none`
- Material: `R` captures `-` for `0` cp
- Component movement: king_safety +9, threats +23
- Result: `1-0` `white_win:checkmate`
- FEN before: `6k1/p2N2p1/1p4B1/2p5/P2p2K1/7r/8/5R2 b - - 14 55`

- Engine re-search: `h3f3` score `-899996`

- Engine played-move constrained score: `-899996`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-899996`, delta `+0`
- Engine reference-move SEE: `0`

- Reference bestmove: `h3h4` score `-899998`
- Reference played-move score: `-899998`, delta `+0`

### 14. Game 41, ply 53: e6c7 (Nc7)

- Categories: `engine-loss, opponent-recapture, recapture-trade, queen-trade-sequence, bad-trade-sequence`
- Score: `+385 -> +365` for current-eval, delta `-20`
- Sequence after reply `c8c7`: `+385 -> -1`, delta `-386`
- Engine recaptures available: `none`
- Material: `N` captures `-` for `0` cp
- Component movement: piece_square -15, center_control -4, king_safety -2, threats -2, safe_mobility +1, mobility +4
- Result: `0-1` `black_win:checkmate`
- FEN before: `2q2bk1/3n1pp1/4N2p/8/1p1NP3/pP1P4/P4PPP/K4B1R w - - 1 27`

- Engine re-search: `e6c7` score `-409`

- Engine played-move constrained score: `-409`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-409`, delta `+0`
- Engine reference-move SEE: `0`

- Reference bestmove: `e6c7` score `-899996`

### 15. Game 41, ply 55: d4c6 (Nc6)

- Categories: `engine-loss, opponent-recapture, recapture-trade, queen-trade-sequence, bad-trade-sequence`
- Score: `-1 -> -2` for current-eval, delta `-1`
- Sequence after reply `c7c6`: `-1 -> -388`, delta `-387`
- Engine recaptures available: `none`
- Material: `N` captures `-` for `0` cp
- Component movement: threats -16, piece_square -10, center_control +6, space +9
- Result: `0-1` `black_win:checkmate`
- FEN before: `5bk1/2qn1pp1/7p/8/1p1NP3/pP1P4/P4PPP/K4B1R w - - 0 28`

- Engine re-search: `d4c6` score `-819618`

- Engine played-move constrained score: `-819730`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-819618`, delta `+112`
- Engine reference-move SEE: `0`

- Reference bestmove: `d4c6` score `-899997`

### 16. Game 15, ply 60: c1c2 (Rc2)

- Categories: `engine-loss, opponent-recapture, recapture-trade, bad-trade-sequence`
- Score: `-629 -> -608` for current-eval, delta `+21`
- Sequence after reply `b2c2`: `-629 -> -1107`, delta `-478`
- Engine recaptures available: `none`
- Material: `R` captures `-` for `0` cp
- Component movement: mobility +6, threats +6
- Result: `0-1` `black_win:checkmate`
- FEN before: `5k2/5p1p/3p1qp1/p1p5/2Pn4/3P1P2/1r3PPP/2RK1B1R w - - 1 31`

- Engine re-search: `c1c2` score `-1773`

- Engine played-move constrained score: `-1773`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-899996`, delta `-898223`
- Engine reference-move SEE: `0`

- Reference bestmove: `c1a1` score `-899995`
- Reference played-move score: `-899995`, delta `+0`

### 17. Game 14, ply 53: c4f1 (Qxf1+)

- Categories: `engine-loss, opponent-recapture, recapture-trade, queen-trade-sequence, bad-trade-sequence`
- Score: `-694 -> -229` for current-eval, delta `+465`
- Sequence after reply `a1f1`: `-694 -> -1081`, delta `-387`
- Engine recaptures available: `none`
- Material: `Q` captures `R` for `500` cp
- Component movement: threats -86, piece_square -7, center_control -6, pawn_dynamics -4, king_safety +32, material +500
- Result: `1-0` `white_win:checkmate`
- FEN before: `r5k1/pp2Q2p/4pppB/2pp4/2q5/2P2B1P/5PP1/R4RK1 b - - 0 30`

- Engine re-search: `c4f1` score `-899996`

- Engine played-move constrained score: `-899996`
- Engine played-move SEE: `-400`
- Engine reference-move constrained score: `-899996`, delta `+0`
- Engine reference-move SEE: `-400`

- Reference bestmove: `c4f1` score `-899998`

### 18. Game 33, ply 48: c6e8 (Qxe8+)

- Categories: `opponent-recapture, recapture-trade, queen-trade-sequence, bad-trade-sequence`
- Score: `+1247 -> +1748` for current-eval, delta `+501`
- Sequence after reply `f7e8`: `+1247 -> +862`, delta `-385`
- Engine recaptures available: `none`
- Material: `Q` captures `R` for `500` cp
- Component movement: threats -54, piece_square -3, pawn_dynamics -2, mobility +22, material +500
- Result: `1-0` `white_win:checkmate`
- FEN before: `4r3/5kPp/p1Qp4/2pP1B2/2Pp1b2/N5P1/PP5P/6K1 w - - 1 28`

- Engine re-search: `c6e8` score `2257`

- Engine played-move constrained score: `2257`
- Engine played-move SEE: `-400`
- Engine reference-move constrained score: `2257`, delta `+0`
- Engine reference-move SEE: `-400`

- Reference bestmove: `c6e8` score `899991`

### 19. Game 55, ply 36: a5a7 (Qxa7)

- Categories: `engine-loss, opponent-recapture, recapture-trade, queen-trade-sequence, bad-trade-sequence`
- Score: `-87 -> +480` for current-eval, delta `+567`
- Sequence after reply `b8a7`: `-87 -> -445`, delta `-358`
- Engine recaptures available: `none`
- Material: `Q` captures `R` for `500` cp
- Component movement: piece_square -8, king_safety -5, pawn_dynamics -2, pawn_structure -1, threats +38, material +500
- Result: `0-1` `black_win:checkmate`
- FEN before: `1q2k2r/r2bbpp1/4p2p/Q1pnp3/8/3P4/PP1BPPPP/1K1R1B1R w k - 0 19`

- Engine re-search: `a5a7` score `-442`

- Engine played-move constrained score: `-442`
- Engine played-move SEE: `-400`
- Engine reference-move constrained score: `-442`, delta `+0`
- Engine reference-move SEE: `-400`

- Reference bestmove: `a5a7` score `-638`

### 20. Game 14, ply 49: a4d7 (Bd7)

- Categories: `engine-loss, opponent-recapture, recapture-trade, queen-trade-sequence, bad-trade-sequence`
- Score: `+170 -> +211` for current-eval, delta `+41`
- Sequence after reply `c7d7`: `+170 -> -183`, delta `-353`
- Engine recaptures available: `none`
- Material: `B` captures `-` for `0` cp
- Component movement: threats -19, bishop_quality -14, mobility -7, safe_mobility -3, trade_context +24, king_safety +49
- Result: `1-0` `white_win:checkmate`
- FEN before: `r3r1k1/ppQ4p/4pppB/2pp4/b1q5/2P2B1P/5PP1/R4RK1 b - - 1 28`

- Engine re-search: `c4f1` score `-871033`

- Engine played-move constrained score: `-899992`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-899992`, delta `+0`
- Engine reference-move SEE: `0`

- Reference bestmove: `a4d7` score `-899996`

### 21. Game 31, ply 82: c1c2 (Rxc2)

- Categories: `engine-loss, opponent-recapture, recapture-trade, queen-trade-sequence, bad-trade-sequence`
- Score: `-428 -> -191` for current-eval, delta `+237`
- Sequence after reply `b2c2`: `-428 -> -779`, delta `-351`
- Engine recaptures available: `none`
- Material: `R` captures `P` for `100` cp
- Component movement: threats +72, material +100
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/k7/p7/1p2P1P1/3P4/7P/1qp1bK2/2R5 w - - 0 43`

- Engine re-search: `c1c2` score `-1022`

- Engine played-move constrained score: `-1022`
- Engine played-move SEE: `-400`
- Engine reference-move constrained score: `-1022`, delta `+0`
- Engine reference-move SEE: `-400`

- Reference bestmove: `c1c2` score `-617`

### 22. Game 35, ply 52: a1d1 (Rd1+)

- Categories: `opponent-recapture, recapture-trade, bad-trade-sequence`
- Score: `+715 -> +767` for current-eval, delta `+52`
- Sequence after reply `f3d1`: `+715 -> +252`, delta `-463`
- Engine recaptures available: `none`
- Material: `R` captures `-` for `0` cp
- Component movement: threats -51, rook_files +26, king_safety +38
- Result: `1-0` `white_win:checkmate`
- FEN before: `3k4/1pp2p2/8/4RP1p/p4B2/2P2b2/PP3PrP/R4K2 w - - 6 27`

- Engine re-search: `e5e3` score `919`

- Engine played-move constrained score: `797`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `797`, delta `+0`
- Engine reference-move SEE: `0`

- Reference bestmove: `a1d1` score `611`

### 23. Game 46, ply 29: g6d3 (Bd3+)

- Categories: `engine-loss, opponent-recapture, recapture-trade, bad-trade-sequence`
- Score: `+849 -> +883` for current-eval, delta `+34`
- Sequence after reply `b5d3`: `+849 -> +414`, delta `-435`
- Engine recaptures available: `none`
- Material: `B` captures `-` for `0` cp
- Component movement: bishop_quality +7, king_safety +15
- Result: `1-0` `white_win:checkmate`
- FEN before: `2kr1b1r/Qpp1ppp1/5nbp/1BN5/3P2n1/7N/PP2KP1P/R1B4q b - - 1 16`

- Engine re-search: `g6d3` score `56`

- Engine played-move constrained score: `56`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `56`, delta `+0`
- Engine reference-move SEE: `0`

- Reference bestmove: `g6d3` score `-644`

### 24. Game 29, ply 101: c3b2 (Bb2)

- Categories: `engine-loss, opponent-recapture, recapture-trade, bad-trade-sequence`
- Score: `-327 -> -350` for current-eval, delta `-23`
- Sequence after reply `b1b2`: `-327 -> -711`, delta `-384`
- Engine recaptures available: `none`
- Material: `B` captures `-` for `0` cp
- Component movement: pawn_structure -37, threats -10, piece_square -5, space -1, mobility +8, king_safety +19
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/3k4/8/2KP3P/q4P2/2B5/3P3P/1r6 w - - 1 53`

- Engine re-search: `c3b2` score `-899996`

- Engine played-move constrained score: `-899996`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-899996`, delta `+0`
- Engine reference-move SEE: `0`

- Reference bestmove: `c3b2` score `-899998`

### 25. Game 28, ply 100: f6e7 (Be7+)

- Categories: `engine-loss, opponent-recapture, recapture-trade, bad-trade-sequence`
- Score: `-659 -> -694` for current-eval, delta `-35`
- Sequence after reply `a7e7`: `-659 -> -1029`, delta `-370`
- Engine recaptures available: `none`
- Material: `B` captures `-` for `0` cp
- Component movement: pawn_structure -31, threats -17, piece_square -10, center_control -1, mobility +7, king_safety +11
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/R5p1/p2K1b2/2Q1p3/k5p1/8/P7/8 b - - 5 51`

- Engine re-search: `f6e7` score `-899996`

- Engine played-move constrained score: `-899996`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-899996`, delta `+0`
- Engine reference-move SEE: `0`

- Reference bestmove: `f6e7` score `-899998`


## Loss-Game Watchlist

### 1. Game 58, ply 107: d5h5 (Rh5)

- Categories: `engine-loss, eval-swing, opponent-recapture, recapture-trade, bad-trade-sequence`
- Score: `-243 -> -401` for current-eval, delta `-158`
- Sequence after reply `g4h5`: `-243 -> -738`, delta `-495`
- Engine recaptures available: `none`
- Material: `R` captures `-` for `0` cp
- Component movement: pawn_structure -116, threats -21, piece_square -9, rook_files -7, center_control +1
- Result: `1-0` `white_win:checkmate`
- FEN before: `2R2N2/7P/p4k2/1ppr4/5PK1/6P1/8/8 b - - 0 56`

- Engine re-search: `d5h5` score `-1489`

- Engine played-move constrained score: `-1489`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-1489`, delta `+0`
- Engine reference-move SEE: `0`

- Reference bestmove: `d5h5` score `-1053`

### 2. Game 76, ply 141: f4f3 (f3)

- Categories: `engine-loss, eval-swing, opponent-recapture, recapture-trade, bad-trade-sequence`
- Score: `+174 -> -11` for current-eval, delta `-185`
- Sequence after reply `e1f3`: `+174 -> -256`, delta `-430`
- Engine recaptures available: `none`
- Material: `P` captures `-` for `0` cp
- Component movement: pawn_structure -174, piece_square -10, threats -2, pawn_dynamics -2, safe_mobility +1, space +2
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/pK6/1pP5/1P2k3/P4p2/6p1/8/4N3 b - - 1 71`

- Engine re-search: `e5e4` score `-772`

- Engine played-move constrained score: `-841`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-772`, delta `+69`
- Engine reference-move SEE: `0`

- Reference bestmove: `e5e4` score `-558`
- Reference played-move score: `-704`, delta `+146`

### 3. Game 75, ply 158: d1e2 (Ke2)

- Categories: `engine-loss, eval-swing`
- Score: `-469 -> -758` for current-eval, delta `-289`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -321, center_control +2, piece_square +30
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/8/8/8/8/1pk5/3p4/3K4 w - - 0 80`

- Engine re-search: `d1e2` score `-1929`

- Engine played-move constrained score: `-1929`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-1929`, delta `+0`
- Engine reference-move SEE: `0`

- Reference bestmove: `d1e2` score `-4287`

### 4. Game 20, ply 120: f7g8 (Kg8)

- Categories: `engine-loss, eval-swing`
- Score: `-10 -> -275` for current-eval, delta `-265`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -243, piece_square -27, king_safety +5
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/1p1KPkp1/1P5p/p4p1P/7P/B7/8/8 b - - 2 62`

- Engine re-search: `f7g8` score `-899992`

- Engine played-move constrained score: `-899992`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-899992`, delta `+0`
- Engine reference-move SEE: `0`

- Reference bestmove: `f7g8` score `-899996`

### 5. Game 75, ply 160: e2f2 (Kf2)

- Categories: `engine-loss, eval-swing`
- Score: `-836 -> -1083` for current-eval, delta `-247`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -246, center_control -1
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/8/8/8/8/2k5/1p1pK3/8 w - - 0 81`

- Engine re-search: `e2e3` score `-1935`

- Engine played-move constrained score: `-1947`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-1935`, delta `+12`
- Engine reference-move SEE: `0`

- Reference bestmove: `e2e3` score `-899994`
- Reference played-move score: `-899995`, delta `+1`

### 6. Game 5, ply 196: a2a3 (Ka3)

- Categories: `engine-loss, eval-swing`
- Score: `-675 -> -905` for current-eval, delta `-230`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -229, piece_square -1
- Result: `0-1` `black_win:checkmate`
- FEN before: `7b/8/8/8/8/8/Kpk5/8 w - - 46 100`

- Engine re-search: `a2a3` score `-820519`

- Engine played-move constrained score: `-820519`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-820519`, delta `+0`
- Engine reference-move SEE: `0`

- Reference bestmove: `a2a3` score `-899997`

### 7. Game 13, ply 148: e4d3 (Kd3)

- Categories: `engine-loss, eval-swing`
- Score: `-725 -> -946` for current-eval, delta `-221`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -205, piece_square -10, space -4, center_control -4, threats +2
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/8/8/8/4K3/6kp/4p3/8 w - - 2 78`

- Engine re-search: `e4d3` score `-1884`

- Engine played-move constrained score: `-1884`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-1904`, delta `-20`
- Engine reference-move SEE: `0`

- Reference bestmove: `e4e5` score `-4293`
- Reference played-move score: `-4304`, delta `+11`

### 8. Game 58, ply 109: f6f5 (Kf5)

- Categories: `engine-loss, eval-swing`
- Score: `-738 -> -947` for current-eval, delta `-209`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -220, king_safety -3, center_control +4, piece_square +8
- Result: `1-0` `white_win:checkmate`
- FEN before: `2R2N2/7P/p4k2/1pp4K/5P2/6P1/8/8 b - - 0 57`

- Engine re-search: `f6f5` score `-1788`

- Engine played-move constrained score: `-1788`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-1788`, delta `+0`
- Engine reference-move SEE: `0`

- Reference bestmove: `f6f5` score `-1078`

### 9. Game 31, ply 90: e3f4 (Kf4)

- Categories: `engine-loss, eval-swing`
- Score: `-603 -> -811` for current-eval, delta `-208`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -190, threats -17, center_control -3, piece_square -2, space +2, king_safety +5
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/k5P1/p7/4P3/3P4/1p1qK2P/4b3/8 w - - 1 47`

- Engine re-search: `e3f4` score `-1759`

- Engine played-move constrained score: `-1759`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-1759`, delta `+0`
- Engine reference-move SEE: `0`

- Reference bestmove: `e3f4` score `-1203`

### 10. Game 86, ply 167: e4e3 (Ke3)

- Categories: `engine-loss, eval-swing`
- Score: `-191 -> -389` for current-eval, delta `-198`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -188, piece_square -10, center_control -4, space +4
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/8/5K2/7P/4k3/8/8/8 b - - 10 85`

- Engine re-search: `e4d4` score `-936`

- Engine played-move constrained score: `-936`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-936`, delta `+0`
- Engine reference-move SEE: `0`

- Reference bestmove: `e4d3` score `-3487`
- Reference played-move score: `-3492`, delta `+5`

### 11. Game 20, ply 122: a5a4 (a4)

- Categories: `engine-loss, eval-swing`
- Score: `-9 -> -204` for current-eval, delta `-195`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `P` captures `-` for `0` cp
- Component movement: pawn_structure -180, piece_square -15
- Result: `1-0` `white_win:checkmate`
- FEN before: `6k1/1p1KP1p1/1P5p/p4p1P/7P/8/1B6/8 b - - 4 63`

- Engine re-search: `g8h8` score `-899994`

- Engine played-move constrained score: `-899994`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-899994`, delta `+0`
- Engine reference-move SEE: `0`

- Reference bestmove: `a5a4` score `-899997`

### 12. Game 13, ply 132: d3d4 (Kd4)

- Categories: `engine-loss, eval-swing`
- Score: `-160 -> -355` for current-eval, delta `-195`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -213, center_control +4, piece_square +10
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/8/8/8/7p/3Kp3/5k1P/8 w - - 2 70`

- Engine re-search: `d3d4` score `-890`

- Engine played-move constrained score: `-890`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-961`, delta `-71`
- Engine reference-move SEE: `0`

- Reference bestmove: `h2h3` score `-675`
- Reference played-move score: `-759`, delta `+84`

### 13. Game 20, ply 106: c8f8 (Rf8)

- Categories: `engine-loss, eval-swing`
- Score: `+329 -> +142` for current-eval, delta `-187`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `R` captures `-` for `0` cp
- Component movement: pawn_structure -178, rook_files -6, threats -5, king_safety +2
- Result: `1-0` `white_win:checkmate`
- FEN before: `2r3k1/1pP1K1p1/1P2Pp1p/p1B4P/8/7P/8/8 b - - 1 55`

- Engine re-search: `c8a8` score `17`

- Engine played-move constrained score: `0`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-220`, delta `-220`
- Engine reference-move SEE: `0`

- Reference bestmove: `g7g5` score `-697`
- Reference played-move score: `-674`, delta `-23`

### 14. Game 20, ply 110: c8f8 (Rf8)

- Categories: `engine-loss, eval-swing`
- Score: `+304 -> +117` for current-eval, delta `-187`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `R` captures `-` for `0` cp
- Component movement: pawn_structure -178, rook_files -6, threats -5, king_safety +2
- Result: `1-0` `white_win:checkmate`
- FEN before: `2r3k1/1pP1K1p1/1P2Pp1p/p6P/7P/B7/8/8 b - - 2 57`

- Engine re-search: `c8f8` score `37`

- Engine played-move constrained score: `37`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `20`, delta `-17`
- Engine reference-move SEE: `0`

- Reference bestmove: `c8a8` score `-611`
- Reference played-move score: `-684`, delta `+73`

### 15. Game 76, ply 143: e5e4 (Ke4)

- Categories: `engine-loss, eval-swing`
- Score: `-256 -> -440` for current-eval, delta `-184`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -197, piece_square -1, space +3, threats +9
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/pK6/1pP5/1P2k3/P7/5Np1/8/8 b - - 0 72`

- Engine re-search: `e5e4` score `-1012`

- Engine played-move constrained score: `-1012`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-1012`, delta `+0`
- Engine reference-move SEE: `0`

- Reference bestmove: `e5f4` score `-647`
- Reference played-move score: `-637`, delta `-10`

### 16. Game 86, ply 165: d4e4 (Ke4)

- Categories: `engine-loss, eval-swing`
- Score: `-166 -> -346` for current-eval, delta `-180`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -180
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/6K1/8/7P/3k4/8/8/8 b - - 8 84`

- Engine re-search: `d4d5` score `-902`

- Engine played-move constrained score: `-936`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-936`, delta `+0`
- Engine reference-move SEE: `0`

- Reference bestmove: `d4e4` score `-3497`

### 17. Game 86, ply 161: d6c5 (Kc5)

- Categories: `engine-loss, eval-swing`
- Score: `-186 -> -362` for current-eval, delta `-176`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -180, space +4
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/6K1/3k4/7P/8/8/8/8 b - - 4 82`

- Engine re-search: `d6e6` score `-902`

- Engine played-move constrained score: `-902`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-902`, delta `+0`
- Engine reference-move SEE: `0`

- Reference bestmove: `d6e6` score `-3483`
- Reference played-move score: `-3497`, delta `+14`

### 18. Game 6, ply 117: c7d6 (Kd6)

- Categories: `engine-loss, eval-swing`
- Score: `-278 -> -452` for current-eval, delta `-174`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -203, mobility -6, space -5, threats -4, king_safety +9, piece_square +26
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/1Rk5/P7/8/P3p3/2p1Kp2/8/8 b - - 1 60`

- Engine re-search: `c7c6` score `-505`

- Engine played-move constrained score: `-622`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-614`, delta `+8`
- Engine reference-move SEE: `0`

- Reference bestmove: `c7c8` score `-579`
- Reference played-move score: `-583`, delta `+4`

### 19. Game 86, ply 157: e6d5 (Kd5)

- Categories: `engine-loss, eval-swing`
- Score: `-178 -> -348` for current-eval, delta `-170`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -188, space +5, piece_square +10
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/6K1/4k3/7P/8/8/8/8 b - - 0 80`

- Engine re-search: `e6f5` score `-902`

- Engine played-move constrained score: `-902`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-902`, delta `+0`
- Engine reference-move SEE: `0`

- Reference bestmove: `e6e5` score `-3491`
- Reference played-move score: `-3491`, delta `+0`

### 20. Game 76, ply 189: d4d3 (Kd3)

- Categories: `engine-loss, eval-swing`
- Score: `-1127 -> -1289` for current-eval, delta `-162`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -156, piece_square -8, center_control -4, king_safety +1, space +5
- Result: `1-0` `white_win:checkmate`
- FEN before: `1Q6/8/2K5/P7/3k4/8/8/8 b - - 2 95`

- Engine re-search: `d4d3` score `-1900`

- Engine played-move constrained score: `-1900`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-1900`, delta `+0`
- Engine reference-move SEE: `0`

- Reference bestmove: `d4c3` score `-4252`
- Reference played-move score: `-4247`, delta `-5`


## Largest Eval Swings

### 1. Game 58, ply 107: d5h5 (Rh5)

- Categories: `engine-loss, eval-swing, opponent-recapture, recapture-trade, bad-trade-sequence`
- Score: `-243 -> -401` for current-eval, delta `-158`
- Sequence after reply `g4h5`: `-243 -> -738`, delta `-495`
- Engine recaptures available: `none`
- Material: `R` captures `-` for `0` cp
- Component movement: pawn_structure -116, threats -21, piece_square -9, rook_files -7, center_control +1
- Result: `1-0` `white_win:checkmate`
- FEN before: `2R2N2/7P/p4k2/1ppr4/5PK1/6P1/8/8 b - - 0 56`

- Engine re-search: `d5h5` score `-1489`

- Engine played-move constrained score: `-1489`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-1489`, delta `+0`
- Engine reference-move SEE: `0`

- Reference bestmove: `d5h5` score `-1053`

### 2. Game 76, ply 141: f4f3 (f3)

- Categories: `engine-loss, eval-swing, opponent-recapture, recapture-trade, bad-trade-sequence`
- Score: `+174 -> -11` for current-eval, delta `-185`
- Sequence after reply `e1f3`: `+174 -> -256`, delta `-430`
- Engine recaptures available: `none`
- Material: `P` captures `-` for `0` cp
- Component movement: pawn_structure -174, piece_square -10, threats -2, pawn_dynamics -2, safe_mobility +1, space +2
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/pK6/1pP5/1P2k3/P4p2/6p1/8/4N3 b - - 1 71`

- Engine re-search: `e5e4` score `-772`

- Engine played-move constrained score: `-841`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-772`, delta `+69`
- Engine reference-move SEE: `0`

- Reference bestmove: `e5e4` score `-558`
- Reference played-move score: `-704`, delta `+146`

### 3. Game 75, ply 158: d1e2 (Ke2)

- Categories: `engine-loss, eval-swing`
- Score: `-469 -> -758` for current-eval, delta `-289`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -321, center_control +2, piece_square +30
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/8/8/8/8/1pk5/3p4/3K4 w - - 0 80`

- Engine re-search: `d1e2` score `-1929`

- Engine played-move constrained score: `-1929`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-1929`, delta `+0`
- Engine reference-move SEE: `0`

- Reference bestmove: `d1e2` score `-4287`

### 4. Game 18, ply 187: g4g3 (Kg3)

- Categories: `eval-swing`
- Score: `+1648 -> +1345` for current-eval, delta `-303`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -302, mobility -2, safe_mobility -1, space +2
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/8/p1p5/1p2P3/5nk1/3r4/8/6K1 b - - 5 96`

- Engine re-search: `g4g3` score `899997`

- Engine played-move constrained score: `899997`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `899997`, delta `+0`
- Engine reference-move SEE: `0`

- Reference bestmove: `g4g3` score `899998`

### 5. Game 20, ply 120: f7g8 (Kg8)

- Categories: `engine-loss, eval-swing`
- Score: `-10 -> -275` for current-eval, delta `-265`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -243, piece_square -27, king_safety +5
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/1p1KPkp1/1P5p/p4p1P/7P/B7/8/8 b - - 2 62`

- Engine re-search: `f7g8` score `-899992`

- Engine played-move constrained score: `-899992`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-899992`, delta `+0`
- Engine reference-move SEE: `0`

- Reference bestmove: `f7g8` score `-899996`

### 6. Game 75, ply 160: e2f2 (Kf2)

- Categories: `engine-loss, eval-swing`
- Score: `-836 -> -1083` for current-eval, delta `-247`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -246, center_control -1
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/8/8/8/8/2k5/1p1pK3/8 w - - 0 81`

- Engine re-search: `e2e3` score `-1935`

- Engine played-move constrained score: `-1947`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-1935`, delta `+12`
- Engine reference-move SEE: `0`

- Reference bestmove: `e2e3` score `-899994`
- Reference played-move score: `-899995`, delta `+1`

### 7. Game 5, ply 196: a2a3 (Ka3)

- Categories: `engine-loss, eval-swing`
- Score: `-675 -> -905` for current-eval, delta `-230`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -229, piece_square -1
- Result: `0-1` `black_win:checkmate`
- FEN before: `7b/8/8/8/8/8/Kpk5/8 w - - 46 100`

- Engine re-search: `a2a3` score `-820519`

- Engine played-move constrained score: `-820519`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-820519`, delta `+0`
- Engine reference-move SEE: `0`

- Reference bestmove: `a2a3` score `-899997`

### 8. Game 13, ply 148: e4d3 (Kd3)

- Categories: `engine-loss, eval-swing`
- Score: `-725 -> -946` for current-eval, delta `-221`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -205, piece_square -10, space -4, center_control -4, threats +2
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/8/8/8/4K3/6kp/4p3/8 w - - 2 78`

- Engine re-search: `e4d3` score `-1884`

- Engine played-move constrained score: `-1884`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-1904`, delta `-20`
- Engine reference-move SEE: `0`

- Reference bestmove: `e4e5` score `-4293`
- Reference played-move score: `-4304`, delta `+11`

### 9. Game 58, ply 109: f6f5 (Kf5)

- Categories: `engine-loss, eval-swing`
- Score: `-738 -> -947` for current-eval, delta `-209`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -220, king_safety -3, center_control +4, piece_square +8
- Result: `1-0` `white_win:checkmate`
- FEN before: `2R2N2/7P/p4k2/1pp4K/5P2/6P1/8/8 b - - 0 57`

- Engine re-search: `f6f5` score `-1788`

- Engine played-move constrained score: `-1788`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-1788`, delta `+0`
- Engine reference-move SEE: `0`

- Reference bestmove: `f6f5` score `-1078`

### 10. Game 31, ply 90: e3f4 (Kf4)

- Categories: `engine-loss, eval-swing`
- Score: `-603 -> -811` for current-eval, delta `-208`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -190, threats -17, center_control -3, piece_square -2, space +2, king_safety +5
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/k5P1/p7/4P3/3P4/1p1qK2P/4b3/8 w - - 1 47`

- Engine re-search: `e3f4` score `-1759`

- Engine played-move constrained score: `-1759`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-1759`, delta `+0`
- Engine reference-move SEE: `0`

- Reference bestmove: `e3f4` score `-1203`

### 11. Game 86, ply 167: e4e3 (Ke3)

- Categories: `engine-loss, eval-swing`
- Score: `-191 -> -389` for current-eval, delta `-198`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -188, piece_square -10, center_control -4, space +4
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/8/5K2/7P/4k3/8/8/8 b - - 10 85`

- Engine re-search: `e4d4` score `-936`

- Engine played-move constrained score: `-936`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-936`, delta `+0`
- Engine reference-move SEE: `0`

- Reference bestmove: `e4d3` score `-3487`
- Reference played-move score: `-3492`, delta `+5`

### 12. Game 20, ply 122: a5a4 (a4)

- Categories: `engine-loss, eval-swing`
- Score: `-9 -> -204` for current-eval, delta `-195`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `P` captures `-` for `0` cp
- Component movement: pawn_structure -180, piece_square -15
- Result: `1-0` `white_win:checkmate`
- FEN before: `6k1/1p1KP1p1/1P5p/p4p1P/7P/8/1B6/8 b - - 4 63`

- Engine re-search: `g8h8` score `-899994`

- Engine played-move constrained score: `-899994`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-899994`, delta `+0`
- Engine reference-move SEE: `0`

- Reference bestmove: `a5a4` score `-899997`

### 13. Game 13, ply 132: d3d4 (Kd4)

- Categories: `engine-loss, eval-swing`
- Score: `-160 -> -355` for current-eval, delta `-195`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -213, center_control +4, piece_square +10
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/8/8/8/7p/3Kp3/5k1P/8 w - - 2 70`

- Engine re-search: `d3d4` score `-890`

- Engine played-move constrained score: `-890`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-961`, delta `-71`
- Engine reference-move SEE: `0`

- Reference bestmove: `h2h3` score `-675`
- Reference played-move score: `-759`, delta `+84`

### 14. Game 61, ply 87: b4a6 (Na6)

- Categories: `eval-swing`
- Score: `+3140 -> +2928` for current-eval, delta `-212`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `N` captures `-` for `0` cp
- Component movement: pawn_structure -174, piece_square -27, mobility -5, bishop_quality -5, space +1, center_control +2
- Result: `1-0` `white_win:checkmate`
- FEN before: `3Q4/k7/8/1B6/1N3P2/2K5/P5PP/7R w - - 1 44`

- Engine re-search: `b4c6` score `899997`

- Engine played-move constrained score: `899997`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `899997`, delta `+0`
- Engine reference-move SEE: `0`

- Reference bestmove: `d8c7` score `899998`
- Reference played-move score: `899999`, delta `-1`

### 15. Game 20, ply 106: c8f8 (Rf8)

- Categories: `engine-loss, eval-swing`
- Score: `+329 -> +142` for current-eval, delta `-187`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `R` captures `-` for `0` cp
- Component movement: pawn_structure -178, rook_files -6, threats -5, king_safety +2
- Result: `1-0` `white_win:checkmate`
- FEN before: `2r3k1/1pP1K1p1/1P2Pp1p/p1B4P/8/7P/8/8 b - - 1 55`

- Engine re-search: `c8a8` score `17`

- Engine played-move constrained score: `0`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-220`, delta `-220`
- Engine reference-move SEE: `0`

- Reference bestmove: `g7g5` score `-697`
- Reference played-move score: `-674`, delta `-23`

### 16. Game 20, ply 110: c8f8 (Rf8)

- Categories: `engine-loss, eval-swing`
- Score: `+304 -> +117` for current-eval, delta `-187`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `R` captures `-` for `0` cp
- Component movement: pawn_structure -178, rook_files -6, threats -5, king_safety +2
- Result: `1-0` `white_win:checkmate`
- FEN before: `2r3k1/1pP1K1p1/1P2Pp1p/p6P/7P/B7/8/8 b - - 2 57`

- Engine re-search: `c8f8` score `37`

- Engine played-move constrained score: `37`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `20`, delta `-17`
- Engine reference-move SEE: `0`

- Reference bestmove: `c8a8` score `-611`
- Reference played-move score: `-684`, delta `+73`

### 17. Game 76, ply 143: e5e4 (Ke4)

- Categories: `engine-loss, eval-swing`
- Score: `-256 -> -440` for current-eval, delta `-184`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -197, piece_square -1, space +3, threats +9
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/pK6/1pP5/1P2k3/P7/5Np1/8/8 b - - 0 72`

- Engine re-search: `e5e4` score `-1012`

- Engine played-move constrained score: `-1012`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-1012`, delta `+0`
- Engine reference-move SEE: `0`

- Reference bestmove: `e5f4` score `-647`
- Reference played-move score: `-637`, delta `-10`

### 18. Game 86, ply 165: d4e4 (Ke4)

- Categories: `engine-loss, eval-swing`
- Score: `-166 -> -346` for current-eval, delta `-180`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -180
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/6K1/8/7P/3k4/8/8/8 b - - 8 84`

- Engine re-search: `d4d5` score `-902`

- Engine played-move constrained score: `-936`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-936`, delta `+0`
- Engine reference-move SEE: `0`

- Reference bestmove: `d4e4` score `-3497`

### 19. Game 86, ply 161: d6c5 (Kc5)

- Categories: `engine-loss, eval-swing`
- Score: `-186 -> -362` for current-eval, delta `-176`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -180, space +4
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/6K1/3k4/7P/8/8/8/8 b - - 4 82`

- Engine re-search: `d6e6` score `-902`

- Engine played-move constrained score: `-902`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-902`, delta `+0`
- Engine reference-move SEE: `0`

- Reference bestmove: `d6e6` score `-3483`
- Reference played-move score: `-3497`, delta `+14`

### 20. Game 6, ply 117: c7d6 (Kd6)

- Categories: `engine-loss, eval-swing`
- Score: `-278 -> -452` for current-eval, delta `-174`
- Sequence after reply: `n/a`
- Engine recaptures available: `none`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -203, mobility -6, space -5, threats -4, king_safety +9, piece_square +26
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/1Rk5/P7/8/P3p3/2p1Kp2/8/8 b - - 1 60`

- Engine re-search: `c7c6` score `-505`

- Engine played-move constrained score: `-622`
- Engine played-move SEE: `0`
- Engine reference-move constrained score: `-614`, delta `+8`
- Engine reference-move SEE: `0`

- Reference bestmove: `c7c8` score `-579`
- Reference played-move score: `-583`, delta `+4`
