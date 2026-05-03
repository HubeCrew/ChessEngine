# Gauntlet Postmortem

## Summary

- `engine`: current-eval
- `games_seen`: 100
- `engine_moves_analyzed`: 4779
- `flagged_events`: 80
- `loss_games_seen`: 41
- `average_delta_cp`: 96.99
- `worst_delta_cp`: -303
- `captures`: 1138
- `equal_value_captures`: 406
- `queen_trades`: 40
- `eval_swings`: 32

## Highest-Severity Events

### 1. Game 58, ply 107: d5h5 (Rh5)

- Categories: `engine-loss, eval-swing, opponent-recapture, recapture-trade, bad-trade-sequence`
- Score: `-243 -> -401` for current-eval, delta `-158`
- Sequence after reply `g4h5`: `-243 -> -738`, delta `-495`
- Material: `R` captures `-` for `0` cp
- Component movement: pawn_structure -116, threats -21, piece_square -9, rook_files -7, center_control +1
- Result: `1-0` `white_win:checkmate`
- FEN before: `2R2N2/7P/p4k2/1ppr4/5PK1/6P1/8/8 b - - 0 56`

- Engine re-search: `f6g7` score `-325`

### 2. Game 76, ply 141: f4f3 (f3)

- Categories: `engine-loss, eval-swing, opponent-recapture, recapture-trade, bad-trade-sequence`
- Score: `+174 -> -11` for current-eval, delta `-185`
- Sequence after reply `e1f3`: `+174 -> -256`, delta `-430`
- Material: `P` captures `-` for `0` cp
- Component movement: pawn_structure -174, piece_square -10, threats -2, pawn_dynamics -2, safe_mobility +1, space +2
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/pK6/1pP5/1P2k3/P4p2/6p1/8/4N3 b - - 1 71`

- Engine re-search: `e5d5` score `-21`

### 3. Game 75, ply 158: d1e2 (Ke2)

- Categories: `engine-loss, eval-swing`
- Score: `-469 -> -758` for current-eval, delta `-289`
- Sequence after reply: `n/a`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -321, center_control +2, piece_square +30
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/8/8/8/8/1pk5/3p4/3K4 w - - 0 80`

- Engine re-search: `d1e2` score `-758`

### 4. Game 18, ply 187: g4g3 (Kg3)

- Categories: `eval-swing`
- Score: `+1648 -> +1345` for current-eval, delta `-303`
- Sequence after reply: `n/a`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -302, mobility -2, safe_mobility -1, space +2
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/8/p1p5/1p2P3/5nk1/3r4/8/6K1 b - - 5 96`

- Engine re-search: `a6a5` score `1496`

### 5. Game 20, ply 120: f7g8 (Kg8)

- Categories: `engine-loss, eval-swing`
- Score: `-10 -> -275` for current-eval, delta `-265`
- Sequence after reply: `n/a`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -243, piece_square -27, king_safety +5
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/1p1KPkp1/1P5p/p4p1P/7P/B7/8/8 b - - 2 62`

- Engine re-search: `f5f4` score `-682`

### 6. Game 75, ply 160: e2f2 (Kf2)

- Categories: `engine-loss, eval-swing`
- Score: `-836 -> -1083` for current-eval, delta `-247`
- Sequence after reply: `n/a`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -246, center_control -1
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/8/8/8/8/2k5/1p1pK3/8 w - - 0 81`

- Engine re-search: `e2e3` score `-1429`

### 7. Game 5, ply 196: a2a3 (Ka3)

- Categories: `engine-loss, eval-swing`
- Score: `-675 -> -905` for current-eval, delta `-230`
- Sequence after reply: `n/a`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -229, piece_square -1
- Result: `0-1` `black_win:checkmate`
- FEN before: `7b/8/8/8/8/8/Kpk5/8 w - - 46 100`

- Engine re-search: `a2a3` score `-1323`

### 8. Game 13, ply 148: e4d3 (Kd3)

- Categories: `engine-loss, eval-swing`
- Score: `-725 -> -946` for current-eval, delta `-221`
- Sequence after reply: `n/a`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -205, piece_square -10, space -4, center_control -4, threats +2
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/8/8/8/4K3/6kp/4p3/8 w - - 2 78`

- Engine re-search: `e4d5` score `-1330`

### 9. Game 58, ply 109: f6f5 (Kf5)

- Categories: `engine-loss, eval-swing`
- Score: `-738 -> -947` for current-eval, delta `-209`
- Sequence after reply: `n/a`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -220, king_safety -3, center_control +4, piece_square +8
- Result: `1-0` `white_win:checkmate`
- FEN before: `2R2N2/7P/p4k2/1pp4K/5P2/6P1/8/8 b - - 0 57`

- Engine re-search: `f6g7` score `-988`

### 10. Game 31, ply 90: e3f4 (Kf4)

- Categories: `engine-loss, eval-swing`
- Score: `-603 -> -811` for current-eval, delta `-208`
- Sequence after reply: `n/a`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -190, threats -17, center_control -3, piece_square -2, space +2, king_safety +5
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/k5P1/p7/4P3/3P4/1p1qK2P/4b3/8 w - - 1 47`

- Engine re-search: `e3f4` score `-1089`

### 11. Game 86, ply 167: e4e3 (Ke3)

- Categories: `engine-loss, eval-swing`
- Score: `-191 -> -389` for current-eval, delta `-198`
- Sequence after reply: `n/a`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -188, piece_square -10, center_control -4, space +4
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/8/5K2/7P/4k3/8/8/8 b - - 10 85`

- Engine re-search: `e4d4` score `-371`

### 12. Game 20, ply 122: a5a4 (a4)

- Categories: `engine-loss, eval-swing`
- Score: `-9 -> -204` for current-eval, delta `-195`
- Sequence after reply: `n/a`
- Material: `P` captures `-` for `0` cp
- Component movement: pawn_structure -180, piece_square -15
- Result: `1-0` `white_win:checkmate`
- FEN before: `6k1/1p1KP1p1/1P5p/p4p1P/7P/8/1B6/8 b - - 4 63`

- Engine re-search: `a5a4` score `-675`

### 13. Game 13, ply 132: d3d4 (Kd4)

- Categories: `engine-loss, eval-swing`
- Score: `-160 -> -355` for current-eval, delta `-195`
- Sequence after reply: `n/a`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -213, center_control +4, piece_square +10
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/8/8/8/7p/3Kp3/5k1P/8 w - - 2 70`

- Engine re-search: `d3c3` score `-172`

### 14. Game 61, ply 87: b4a6 (Na6)

- Categories: `eval-swing`
- Score: `+3140 -> +2928` for current-eval, delta `-212`
- Sequence after reply: `n/a`
- Material: `N` captures `-` for `0` cp
- Component movement: pawn_structure -174, piece_square -27, mobility -5, bishop_quality -5, space +1, center_control +2
- Result: `1-0` `white_win:checkmate`
- FEN before: `3Q4/k7/8/1B6/1N3P2/2K5/P5PP/7R w - - 1 44`

- Engine re-search: `d8c7` score `3117`

### 15. Game 20, ply 106: c8f8 (Rf8)

- Categories: `engine-loss, eval-swing`
- Score: `+329 -> +142` for current-eval, delta `-187`
- Sequence after reply: `n/a`
- Material: `R` captures `-` for `0` cp
- Component movement: pawn_structure -178, rook_files -6, threats -5, king_safety +2
- Result: `1-0` `white_win:checkmate`
- FEN before: `2r3k1/1pP1K1p1/1P2Pp1p/p1B4P/8/7P/8/8 b - - 1 55`

- Engine re-search: `a5a4` score `320`

### 16. Game 20, ply 110: c8f8 (Rf8)

- Categories: `engine-loss, eval-swing`
- Score: `+304 -> +117` for current-eval, delta `-187`
- Sequence after reply: `n/a`
- Material: `R` captures `-` for `0` cp
- Component movement: pawn_structure -178, rook_files -6, threats -5, king_safety +2
- Result: `1-0` `white_win:checkmate`
- FEN before: `2r3k1/1pP1K1p1/1P2Pp1p/p6P/7P/B7/8/8 b - - 2 57`

- Engine re-search: `f6f5` score `286`

### 17. Game 76, ply 143: e5e4 (Ke4)

- Categories: `engine-loss, eval-swing`
- Score: `-256 -> -440` for current-eval, delta `-184`
- Sequence after reply: `n/a`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -197, piece_square -1, space +3, threats +9
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/pK6/1pP5/1P2k3/P7/5Np1/8/8 b - - 0 72`

- Engine re-search: `e5d5` score `-452`

### 18. Game 86, ply 165: d4e4 (Ke4)

- Categories: `engine-loss, eval-swing`
- Score: `-166 -> -346` for current-eval, delta `-180`
- Sequence after reply: `n/a`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -180
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/6K1/8/7P/3k4/8/8/8 b - - 8 84`

- Engine re-search: `d4e5` score `-160`

### 19. Game 86, ply 161: d6c5 (Kc5)

- Categories: `engine-loss, eval-swing`
- Score: `-186 -> -362` for current-eval, delta `-176`
- Sequence after reply: `n/a`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -180, space +4
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/6K1/3k4/7P/8/8/8/8 b - - 4 82`

- Engine re-search: `d6e5` score `-160`

### 20. Game 6, ply 117: c7d6 (Kd6)

- Categories: `engine-loss, eval-swing`
- Score: `-278 -> -452` for current-eval, delta `-174`
- Sequence after reply: `n/a`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -203, mobility -6, space -5, threats -4, king_safety +9, piece_square +26
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/1Rk5/P7/8/P3p3/2p1Kp2/8/8 b - - 1 60`

- Engine re-search: `c7c6` score `-405`

### 21. Game 86, ply 157: e6d5 (Kd5)

- Categories: `engine-loss, eval-swing`
- Score: `-178 -> -348` for current-eval, delta `-170`
- Sequence after reply: `n/a`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -188, space +5, piece_square +10
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/6K1/4k3/7P/8/8/8/8 b - - 0 80`

- Engine re-search: `e6e5` score `-160`

### 22. Game 76, ply 189: d4d3 (Kd3)

- Categories: `engine-loss, eval-swing`
- Score: `-1127 -> -1289` for current-eval, delta `-162`
- Sequence after reply: `n/a`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -156, piece_square -8, center_control -4, king_safety +1, space +5
- Result: `1-0` `white_win:checkmate`
- FEN before: `1Q6/8/2K5/P7/3k4/8/8/8 b - - 2 95`

- Engine re-search: `d4e4` score `-1292`

### 23. Game 20, ply 94: c8a8 (Ra8)

- Categories: `engine-loss, eval-swing`
- Score: `+159 -> +1` for current-eval, delta `-158`
- Sequence after reply: `n/a`
- Material: `R` captures `-` for `0` cp
- Component movement: pawn_structure -147, rook_files -6, piece_square -5
- Result: `1-0` `white_win:checkmate`
- FEN before: `2r3k1/1pP3p1/pP1BPp1p/7P/3K4/8/7P/8 b - - 7 49`

- Engine re-search: `f6f5` score `133`

### 24. Game 76, ply 187: e4d4 (Kd4)

- Categories: `engine-loss, eval-swing`
- Score: `-1104 -> -1254` for current-eval, delta `-150`
- Sequence after reply: `n/a`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -150
- Result: `1-0` `white_win:checkmate`
- FEN before: `1Q6/8/1K6/P7/4k3/8/8/8 b - - 0 94`

- Engine re-search: `e4d3` score `-1267`

### 25. Game 31, ply 114: d6c7 (Kc7)

- Categories: `engine-loss, eval-swing`
- Score: `-1294 -> -1443` for current-eval, delta `-149`
- Sequence after reply: `n/a`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -135, piece_square -21, center_control -7, space -3, king_safety +17
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/k7/3K4/p7/2b1q2P/8/8/8 w - - 1 59`

- Engine re-search: `h4h5` score `-1389`


## Trade And Simplification Watchlist

### 1. Game 58, ply 107: d5h5 (Rh5)

- Categories: `engine-loss, eval-swing, opponent-recapture, recapture-trade, bad-trade-sequence`
- Score: `-243 -> -401` for current-eval, delta `-158`
- Sequence after reply `g4h5`: `-243 -> -738`, delta `-495`
- Material: `R` captures `-` for `0` cp
- Component movement: pawn_structure -116, threats -21, piece_square -9, rook_files -7, center_control +1
- Result: `1-0` `white_win:checkmate`
- FEN before: `2R2N2/7P/p4k2/1ppr4/5PK1/6P1/8/8 b - - 0 56`

- Engine re-search: `f6g7` score `-325`

### 2. Game 76, ply 141: f4f3 (f3)

- Categories: `engine-loss, eval-swing, opponent-recapture, recapture-trade, bad-trade-sequence`
- Score: `+174 -> -11` for current-eval, delta `-185`
- Sequence after reply `e1f3`: `+174 -> -256`, delta `-430`
- Material: `P` captures `-` for `0` cp
- Component movement: pawn_structure -174, piece_square -10, threats -2, pawn_dynamics -2, safe_mobility +1, space +2
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/pK6/1pP5/1P2k3/P4p2/6p1/8/4N3 b - - 1 71`

- Engine re-search: `e5d5` score `-21`

### 3. Game 29, ply 47: c3d4 (Qd4+)

- Categories: `engine-loss, opponent-recapture, recapture-trade, queen-trade-sequence, bad-trade-sequence`
- Score: `+134 -> +230` for current-eval, delta `+96`
- Sequence after reply `b6d4`: `+134 -> -867`, delta `-1001`
- Material: `Q` captures `-` for `0` cp
- Component movement: trade_context +12, king_safety +56
- Result: `0-1` `black_win:checkmate`
- FEN before: `6r1/1ppkbp2/pq2b3/7r/7p/1PQPR1P1/PB1P1P1P/4R1K1 w - - 6 26`

- Engine re-search: `c3c2` score `41`

### 4. Game 86, ply 73: f2f6 (Qf6)

- Categories: `engine-loss, opponent-recapture, recapture-trade, queen-trade-sequence, bad-trade-sequence`
- Score: `+151 -> +145` for current-eval, delta `-6`
- Sequence after reply `d8f6`: `+151 -> -800`, delta `-951`
- Material: `Q` captures `-` for `0` cp
- Component movement: pawn_structure -51, king_safety -1, space -1, center_control +7, threats +33
- Result: `1-0` `white_win:checkmate`
- FEN before: `3Q4/pp3pkp/3P2p1/1B6/7P/4p1NK/5q2/8 b - - 0 38`

- Engine re-search: `a7a6` score `147`

### 5. Game 56, ply 67: f6f4 (Qf4)

- Categories: `opponent-recapture, recapture-trade, queen-trade-sequence, bad-trade-sequence`
- Score: `+830 -> +827` for current-eval, delta `-3`
- Sequence after reply `e3f4`: `+830 -> -144`, delta `-974`
- Material: `Q` captures `-` for `0` cp
- Component movement: king_safety -21, mobility -10, pawn_structure -10, trade_context -8, threats +62
- Result: `1/2-1/2` `draw:threefold_repetition`
- FEN before: `r5kr/p4pp1/2pb1q2/1p1p2P1/2p5/4QN2/2B1R1K1/5R2 b - - 0 34`

- Engine re-search: `f6f4` score `813`

### 6. Game 31, ply 102: g8g7 (Qg7+)

- Categories: `engine-loss, opponent-recapture, recapture-trade, queen-trade-sequence, bad-trade-sequence`
- Score: `-1452 -> -1335` for current-eval, delta `+117`
- Sequence after reply `e5g7`: `-1452 -> -2353`, delta `-901`
- Material: `Q` captures `-` for `0` cp
- Component movement: center_control -1, trade_context +32, threats +39
- Result: `0-1` `black_win:checkmate`
- FEN before: `5KQ1/k7/p7/4q3/8/3b3P/8/1q6 w - - 2 53`

- Engine re-search: `g8g7` score `-1383`

### 7. Game 63, ply 58: c4f7 (Qf7)

- Categories: `engine-loss, opponent-recapture, recapture-trade, queen-trade-sequence, bad-trade-sequence`
- Score: `+331 -> +378` for current-eval, delta `+47`
- Sequence after reply `f8f7`: `+331 -> -565`, delta `-896`
- Material: `Q` captures `-` for `0` cp
- Component movement: threats -27, piece_square -5, space +15, king_safety +37
- Result: `0-1` `black_win:checkmate`
- FEN before: `2k1rr2/1bp4p/1p6/8/1PQ5/3P3P/P1P3PR/5K2 w - - 1 32`

- Engine re-search: `f1g1` score `434`

### 8. Game 35, ply 38: d4c3 (Qc3)

- Categories: `opponent-recapture, recapture-trade, queen-trade-sequence, bad-trade-sequence`
- Score: `+466 -> +460` for current-eval, delta `-6`
- Sequence after reply `c6c3`: `+466 -> -442`, delta `-908`
- Material: `Q` captures `-` for `0` cp
- Component movement: king_safety -17, piece_square -1, threats +3, center_control +5
- Result: `1-0` `white_win:checkmate`
- FEN before: `2k1r3/1pp1bp2/2q1b3/5p1p/p2QP3/8/PP1P1PPP/R1B1R1K1 w - - 1 20`

- Engine re-search: `d2d3` score `438`

### 9. Game 2, ply 56: h4e1 (Qe1)

- Categories: `opponent-recapture, recapture-trade, queen-trade-sequence, bad-trade-sequence`
- Score: `+1391 -> +1409` for current-eval, delta `+18`
- Sequence after reply `e2e1`: `+1391 -> +536`, delta `-855`
- Material: `Q` captures `-` for `0` cp
- Component movement: bishop_quality -6, mobility -1, safe_mobility -1, space +8, threats +14
- Result: `0-1` `black_win:checkmate`
- FEN before: `r6r/pp2kp1p/2pp2p1/4p3/2P3Nq/5P1B/P2bQ3/6NK b - - 0 28`

- Engine re-search: `h4e1` score `1409`

### 10. Game 3, ply 82: e5f4 (Qxf4+)

- Categories: `opponent-recapture, recapture-trade, queen-trade-sequence, bad-trade-sequence`
- Score: `+753 -> +906` for current-eval, delta `+153`
- Sequence after reply `f7f4`: `+753 -> -84`, delta `-837`
- Material: `Q` captures `P` for `100` cp
- Component movement: pawn_dynamics -11, threats -3, mobility -1, king_safety +22, material +100
- Result: `1-0` `white_win:checkmate`
- FEN before: `2r5/R1p2q2/1p2b2k/4Q3/4Pp2/6Pp/PPP4P/3R2K1 w - - 0 44`

- Engine re-search: `e5f4` score `851`

### 11. Game 79, ply 27: a5e5 (Qe5)

- Categories: `opponent-recapture, recapture-trade, queen-trade-sequence, bad-trade-sequence`
- Score: `+294 -> +383` for current-eval, delta `+89`
- Sequence after reply `d6e5`: `+294 -> -526`, delta `-820`
- Material: `Q` captures `-` for `0` cp
- Component movement: mobility -3, safe_mobility -1, piece_square +10, threats +62
- Result: `1/2-1/2` `draw:threefold_repetition`
- FEN before: `r1b1k2r/1pn1n1p1/2pqp2p/Q4p2/3P4/3B1N2/PP1B1PPP/R4RK1 w kq - 1 16`

- Engine re-search: `a5e5` score `336`

### 12. Game 53, ply 20: d1d4 (Qd4)

- Categories: `opponent-recapture, recapture-trade, queen-trade-sequence, bad-trade-sequence`
- Score: `+294 -> +326` for current-eval, delta `+32`
- Sequence after reply `f6d4`: `+294 -> -512`, delta `-806`
- Material: `Q` captures `-` for `0` cp
- Component movement: mobility -1, safe_mobility -1, piece_square +11, center_control +13
- Result: `1-0` `white_win:checkmate`
- FEN before: `rn3r1k/ppP4p/5qp1/6p1/2P1P1b1/2P2N2/P4PP1/R2QKB1R w KQ - 1 14`

- Engine re-search: `e4e5` score `381`

### 13. Game 5, ply 24: c6c7 (Qxc7)

- Categories: `engine-loss, opponent-recapture, recapture-trade, queen-trade-sequence, bad-trade-sequence`
- Score: `+263 -> +539` for current-eval, delta `+276`
- Sequence after reply `d8c7`: `+263 -> -385`, delta `-648`
- Material: `Q` captures `P` for `100` cp
- Component movement: mobility -3, threats +97, material +100
- Result: `0-1` `black_win:checkmate`
- FEN before: `1r1qk1nr/2pbb3/p1Q1pp2/1p4pp/2BP4/2N1P1B1/PP3PPP/R3K2R w KQk - 4 14`

- Engine re-search: `c6c7` score `514`

### 14. Game 32, ply 25: e6e2 (Qxe2+)

- Categories: `opponent-recapture, recapture-trade, queen-trade-sequence, bad-trade-sequence`
- Score: `+342 -> +808` for current-eval, delta `+466`
- Sequence after reply `d1e2`: `+342 -> -316`, delta `-658`
- Material: `Q` captures `B` for `330` cp
- Component movement: piece_square -6, center_control -6, king_safety +37, material +330
- Result: `0-1` `black_win:checkmate`
- FEN before: `1r2kb1r/p3pppp/2n1q3/3p4/3P2b1/1PP5/P3BPPP/R1BQK2R b KQk - 0 14`

- Engine re-search: `e6e4` score `373`

### 15. Game 60, ply 38: d4d7 (Qxd7)

- Categories: `engine-loss, opponent-recapture, recapture-trade, queen-trade-sequence, bad-trade-sequence`
- Score: `+503 -> +928` for current-eval, delta `+425`
- Sequence after reply `c6d7`: `+503 -> -106`, delta `-609`
- Material: `Q` captures `N` for `320` cp
- Component movement: piece_square -6, space -6, center_control -1, development -1, threats +34, material +320
- Result: `1-0` `white_win:checkmate`
- FEN before: `rk3b1r/p2Np1pp/2Q5/8/2Nq3P/7R/P2nPP2/4K3 b - - 2 21`

- Engine re-search: `d4d7` score `308`

### 16. Game 12, ply 71: a8f8 (Rf8)

- Categories: `engine-loss, opponent-recapture, recapture-trade, queen-trade-sequence, bad-trade-sequence`
- Score: `+347 -> +446` for current-eval, delta `+99`
- Sequence after reply `f2f8`: `+347 -> -247`, delta `-594`
- Material: `R` captures `-` for `0` cp
- Component movement: threats -6, rook_files +20, king_safety +45
- Result: `1-0` `white_win:checkmate`
- FEN before: `r5k1/pp4p1/2p3Nr/3pN1p1/Pq6/4R1K1/2B2Q2/8 b - - 3 37`

- Engine re-search: `g8h7` score `366`

### 17. Game 41, ply 35: c2c8 (Qxc8)

- Categories: `engine-loss, opponent-recapture, recapture-trade, queen-trade-sequence, bad-trade-sequence`
- Score: `+172 -> +544` for current-eval, delta `+372`
- Sequence after reply `f8c8`: `+172 -> -420`, delta `-592`
- Material: `Q` captures `B` for `330` cp
- Component movement: piece_square -24, development -6, threats -5, rook_files -1, bishop_quality +31, material +330
- Result: `0-1` `black_win:checkmate`
- FEN before: `rnb2rk1/5pp1/1q2p2p/pp6/1b1pP3/3P1N2/PPQ1NPPP/1KR2B1R w - - 4 18`

- Engine re-search: `e2d4` score `334`

### 18. Game 24, ply 33: d4d2 (Qxd2)

- Categories: `engine-loss, opponent-recapture, recapture-trade, queen-trade-sequence, bad-trade-sequence`
- Score: `+225 -> +700` for current-eval, delta `+475`
- Sequence after reply `d1d2`: `+225 -> -340`, delta `-565`
- Material: `Q` captures `B` for `330` cp
- Component movement: king_safety -5, center_control -3, piece_square -2, threats +98, material +330
- Result: `1-0` `white_win:checkmate`
- FEN before: `r2r2k1/ppp2ppp/5n2/8/Pb1qp3/N7/2NBQPPP/3R1RK1 b - - 1 19`

- Engine re-search: `d4d2` score `269`

### 19. Game 96, ply 63: e2e3 (Re3)

- Categories: `opponent-recapture, recapture-trade, queen-trade-sequence, bad-trade-sequence`
- Score: `+1091 -> +1051` for current-eval, delta `-40`
- Sequence after reply `b3e3`: `+1091 -> +544`, delta `-547`
- Material: `R` captures `-` for `0` cp
- Component movement: rook_files -18, bishop_quality -16, piece_square -9, mobility -3, king_safety +3, threats +5
- Result: `0-1` `black_win:checkmate`
- FEN before: `1k6/2p3pp/1pq2p2/1N1r4/P6P/1Q6/KP1br3/3R4 b - - 0 32`

- Engine re-search: `c6c5` score `1104`

### 20. Game 19, ply 63: c8f8 (Qxf8+)

- Categories: `opponent-recapture, recapture-trade, queen-trade-sequence, bad-trade-sequence`
- Score: `+1233 -> +1646` for current-eval, delta `+413`
- Sequence after reply `b4f8`: `+1233 -> +669`, delta `-564`
- Material: `Q` captures `B` for `330` cp
- Component movement: piece_square -3, development -3, center_control -2, pawn_dynamics -1, threats +33, material +330
- Result: `1-0` `white_win:checkmate`
- FEN before: `2Q2bk1/3N1p2/6p1/7p/pq1P4/4P3/PP3PPP/2K4R w - - 2 34`

- Engine re-search: `d7e5` score `1283`

### 21. Game 14, ply 51: e8e7 (Re7)

- Categories: `engine-loss, opponent-recapture, recapture-trade, queen-trade-sequence, bad-trade-sequence`
- Score: `-183 -> -74` for current-eval, delta `+109`
- Sequence after reply `d7e7`: `-183 -> -694`, delta `-511`
- Material: `R` captures `-` for `0` cp
- Component movement: piece_square -4, threats +35, king_safety +46
- Result: `1-0` `white_win:checkmate`
- FEN before: `r3r1k1/pp1Q3p/4pppB/2pp4/2q5/2P2B1P/5PP1/R4RK1 b - - 0 29`

- Engine re-search: `c4c3` score `-237`

### 22. Game 29, ply 51: e3e5 (Re5)

- Categories: `engine-loss, opponent-recapture, recapture-trade, bad-trade-sequence`
- Score: `+73 -> +100` for current-eval, delta `+27`
- Sequence after reply `h5e5`: `+73 -> -520`, delta `-593`
- Material: `R` captures `-` for `0` cp
- Component movement: bishop_quality -6, space -2, mobility +10, threats +21
- Result: `0-1` `black_win:checkmate`
- FEN before: `6r1/1p1kbp2/p1p1b3/7r/3B3p/1P1PR1P1/P2P1P1P/4R1K1 w - - 0 28`

- Engine re-search: `e1c1` score `-31`

### 23. Game 65, ply 20: a4b4 (Qxb4)

- Categories: `opponent-recapture, recapture-trade, queen-trade-sequence, bad-trade-sequence`
- Score: `+340 -> +835` for current-eval, delta `+495`
- Sequence after reply `a6b4`: `+340 -> -184`, delta `-524`
- Material: `Q` captures `B` for `330` cp
- Component movement: threats -82, rook_files -1, pawn_dynamics -1, king_safety +99, material +330
- Result: `1-0` `white_win:checkmate`
- FEN before: `r1bqk2r/p2n2pp/n3Np2/1N2p3/Qbp5/4B3/PP2PPPP/3RKB1R w Kkq - 1 12`

- Engine re-search: `b5c3` score `473`

### 24. Game 89, ply 45: e1e7 (Re7+)

- Categories: `opponent-recapture, recapture-trade, queen-trade-sequence, bad-trade-sequence`
- Score: `+943 -> +1129` for current-eval, delta `+186`
- Sequence after reply `d7e7`: `+943 -> +423`, delta `-520`
- Material: `R` captures `-` for `0` cp
- Component movement: mobility -1, safe_mobility -1, space -1, threats +46, king_safety +81
- Result: `1-0` `white_win:checkmate`
- FEN before: `2r2r2/ppnq3k/2N4p/5pp1/1BQ5/3P4/PPP2PPP/R3R1K1 w - - 1 25`

- Engine re-search: `e1e7` score `1334`

### 25. Game 63, ply 54: f2f1 (Rf1)

- Categories: `engine-loss, opponent-recapture, recapture-trade, queen-trade-sequence, bad-trade-sequence`
- Score: `-42 -> +47` for current-eval, delta `+89`
- Sequence after reply `d1f1`: `-42 -> -535`, delta `-493`
- Material: `R` captures `-` for `0` cp
- Component movement: king_safety +32, threats +49
- Result: `0-1` `black_win:checkmate`
- FEN before: `2k1r1r1/1bp4p/1p6/8/1PQ5/3P3P/P1P2RPR/3q2K1 w - - 6 30`

- Engine re-search: `f2f1` score `47`


## Loss-Game Watchlist

### 1. Game 58, ply 107: d5h5 (Rh5)

- Categories: `engine-loss, eval-swing, opponent-recapture, recapture-trade, bad-trade-sequence`
- Score: `-243 -> -401` for current-eval, delta `-158`
- Sequence after reply `g4h5`: `-243 -> -738`, delta `-495`
- Material: `R` captures `-` for `0` cp
- Component movement: pawn_structure -116, threats -21, piece_square -9, rook_files -7, center_control +1
- Result: `1-0` `white_win:checkmate`
- FEN before: `2R2N2/7P/p4k2/1ppr4/5PK1/6P1/8/8 b - - 0 56`

- Engine re-search: `f6g7` score `-325`

### 2. Game 76, ply 141: f4f3 (f3)

- Categories: `engine-loss, eval-swing, opponent-recapture, recapture-trade, bad-trade-sequence`
- Score: `+174 -> -11` for current-eval, delta `-185`
- Sequence after reply `e1f3`: `+174 -> -256`, delta `-430`
- Material: `P` captures `-` for `0` cp
- Component movement: pawn_structure -174, piece_square -10, threats -2, pawn_dynamics -2, safe_mobility +1, space +2
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/pK6/1pP5/1P2k3/P4p2/6p1/8/4N3 b - - 1 71`

- Engine re-search: `e5d5` score `-21`

### 3. Game 75, ply 158: d1e2 (Ke2)

- Categories: `engine-loss, eval-swing`
- Score: `-469 -> -758` for current-eval, delta `-289`
- Sequence after reply: `n/a`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -321, center_control +2, piece_square +30
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/8/8/8/8/1pk5/3p4/3K4 w - - 0 80`

- Engine re-search: `d1e2` score `-758`

### 4. Game 20, ply 120: f7g8 (Kg8)

- Categories: `engine-loss, eval-swing`
- Score: `-10 -> -275` for current-eval, delta `-265`
- Sequence after reply: `n/a`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -243, piece_square -27, king_safety +5
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/1p1KPkp1/1P5p/p4p1P/7P/B7/8/8 b - - 2 62`

- Engine re-search: `f5f4` score `-682`

### 5. Game 75, ply 160: e2f2 (Kf2)

- Categories: `engine-loss, eval-swing`
- Score: `-836 -> -1083` for current-eval, delta `-247`
- Sequence after reply: `n/a`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -246, center_control -1
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/8/8/8/8/2k5/1p1pK3/8 w - - 0 81`

- Engine re-search: `e2e3` score `-1429`

### 6. Game 5, ply 196: a2a3 (Ka3)

- Categories: `engine-loss, eval-swing`
- Score: `-675 -> -905` for current-eval, delta `-230`
- Sequence after reply: `n/a`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -229, piece_square -1
- Result: `0-1` `black_win:checkmate`
- FEN before: `7b/8/8/8/8/8/Kpk5/8 w - - 46 100`

- Engine re-search: `a2a3` score `-1323`

### 7. Game 13, ply 148: e4d3 (Kd3)

- Categories: `engine-loss, eval-swing`
- Score: `-725 -> -946` for current-eval, delta `-221`
- Sequence after reply: `n/a`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -205, piece_square -10, space -4, center_control -4, threats +2
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/8/8/8/4K3/6kp/4p3/8 w - - 2 78`

- Engine re-search: `e4d5` score `-1330`

### 8. Game 58, ply 109: f6f5 (Kf5)

- Categories: `engine-loss, eval-swing`
- Score: `-738 -> -947` for current-eval, delta `-209`
- Sequence after reply: `n/a`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -220, king_safety -3, center_control +4, piece_square +8
- Result: `1-0` `white_win:checkmate`
- FEN before: `2R2N2/7P/p4k2/1pp4K/5P2/6P1/8/8 b - - 0 57`

- Engine re-search: `f6g7` score `-988`

### 9. Game 31, ply 90: e3f4 (Kf4)

- Categories: `engine-loss, eval-swing`
- Score: `-603 -> -811` for current-eval, delta `-208`
- Sequence after reply: `n/a`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -190, threats -17, center_control -3, piece_square -2, space +2, king_safety +5
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/k5P1/p7/4P3/3P4/1p1qK2P/4b3/8 w - - 1 47`

- Engine re-search: `e3f4` score `-1089`

### 10. Game 86, ply 167: e4e3 (Ke3)

- Categories: `engine-loss, eval-swing`
- Score: `-191 -> -389` for current-eval, delta `-198`
- Sequence after reply: `n/a`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -188, piece_square -10, center_control -4, space +4
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/8/5K2/7P/4k3/8/8/8 b - - 10 85`

- Engine re-search: `e4d4` score `-371`

### 11. Game 20, ply 122: a5a4 (a4)

- Categories: `engine-loss, eval-swing`
- Score: `-9 -> -204` for current-eval, delta `-195`
- Sequence after reply: `n/a`
- Material: `P` captures `-` for `0` cp
- Component movement: pawn_structure -180, piece_square -15
- Result: `1-0` `white_win:checkmate`
- FEN before: `6k1/1p1KP1p1/1P5p/p4p1P/7P/8/1B6/8 b - - 4 63`

- Engine re-search: `a5a4` score `-675`

### 12. Game 13, ply 132: d3d4 (Kd4)

- Categories: `engine-loss, eval-swing`
- Score: `-160 -> -355` for current-eval, delta `-195`
- Sequence after reply: `n/a`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -213, center_control +4, piece_square +10
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/8/8/8/7p/3Kp3/5k1P/8 w - - 2 70`

- Engine re-search: `d3c3` score `-172`

### 13. Game 20, ply 106: c8f8 (Rf8)

- Categories: `engine-loss, eval-swing`
- Score: `+329 -> +142` for current-eval, delta `-187`
- Sequence after reply: `n/a`
- Material: `R` captures `-` for `0` cp
- Component movement: pawn_structure -178, rook_files -6, threats -5, king_safety +2
- Result: `1-0` `white_win:checkmate`
- FEN before: `2r3k1/1pP1K1p1/1P2Pp1p/p1B4P/8/7P/8/8 b - - 1 55`

- Engine re-search: `a5a4` score `320`

### 14. Game 20, ply 110: c8f8 (Rf8)

- Categories: `engine-loss, eval-swing`
- Score: `+304 -> +117` for current-eval, delta `-187`
- Sequence after reply: `n/a`
- Material: `R` captures `-` for `0` cp
- Component movement: pawn_structure -178, rook_files -6, threats -5, king_safety +2
- Result: `1-0` `white_win:checkmate`
- FEN before: `2r3k1/1pP1K1p1/1P2Pp1p/p6P/7P/B7/8/8 b - - 2 57`

- Engine re-search: `f6f5` score `286`

### 15. Game 76, ply 143: e5e4 (Ke4)

- Categories: `engine-loss, eval-swing`
- Score: `-256 -> -440` for current-eval, delta `-184`
- Sequence after reply: `n/a`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -197, piece_square -1, space +3, threats +9
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/pK6/1pP5/1P2k3/P7/5Np1/8/8 b - - 0 72`

- Engine re-search: `e5d5` score `-452`

### 16. Game 86, ply 165: d4e4 (Ke4)

- Categories: `engine-loss, eval-swing`
- Score: `-166 -> -346` for current-eval, delta `-180`
- Sequence after reply: `n/a`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -180
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/6K1/8/7P/3k4/8/8/8 b - - 8 84`

- Engine re-search: `d4e5` score `-160`

### 17. Game 86, ply 161: d6c5 (Kc5)

- Categories: `engine-loss, eval-swing`
- Score: `-186 -> -362` for current-eval, delta `-176`
- Sequence after reply: `n/a`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -180, space +4
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/6K1/3k4/7P/8/8/8/8 b - - 4 82`

- Engine re-search: `d6e5` score `-160`

### 18. Game 6, ply 117: c7d6 (Kd6)

- Categories: `engine-loss, eval-swing`
- Score: `-278 -> -452` for current-eval, delta `-174`
- Sequence after reply: `n/a`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -203, mobility -6, space -5, threats -4, king_safety +9, piece_square +26
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/1Rk5/P7/8/P3p3/2p1Kp2/8/8 b - - 1 60`

- Engine re-search: `c7c6` score `-405`

### 19. Game 86, ply 157: e6d5 (Kd5)

- Categories: `engine-loss, eval-swing`
- Score: `-178 -> -348` for current-eval, delta `-170`
- Sequence after reply: `n/a`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -188, space +5, piece_square +10
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/6K1/4k3/7P/8/8/8/8 b - - 0 80`

- Engine re-search: `e6e5` score `-160`

### 20. Game 76, ply 189: d4d3 (Kd3)

- Categories: `engine-loss, eval-swing`
- Score: `-1127 -> -1289` for current-eval, delta `-162`
- Sequence after reply: `n/a`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -156, piece_square -8, center_control -4, king_safety +1, space +5
- Result: `1-0` `white_win:checkmate`
- FEN before: `1Q6/8/2K5/P7/3k4/8/8/8 b - - 2 95`

- Engine re-search: `d4e4` score `-1292`


## Largest Eval Swings

### 1. Game 58, ply 107: d5h5 (Rh5)

- Categories: `engine-loss, eval-swing, opponent-recapture, recapture-trade, bad-trade-sequence`
- Score: `-243 -> -401` for current-eval, delta `-158`
- Sequence after reply `g4h5`: `-243 -> -738`, delta `-495`
- Material: `R` captures `-` for `0` cp
- Component movement: pawn_structure -116, threats -21, piece_square -9, rook_files -7, center_control +1
- Result: `1-0` `white_win:checkmate`
- FEN before: `2R2N2/7P/p4k2/1ppr4/5PK1/6P1/8/8 b - - 0 56`

- Engine re-search: `f6g7` score `-325`

### 2. Game 76, ply 141: f4f3 (f3)

- Categories: `engine-loss, eval-swing, opponent-recapture, recapture-trade, bad-trade-sequence`
- Score: `+174 -> -11` for current-eval, delta `-185`
- Sequence after reply `e1f3`: `+174 -> -256`, delta `-430`
- Material: `P` captures `-` for `0` cp
- Component movement: pawn_structure -174, piece_square -10, threats -2, pawn_dynamics -2, safe_mobility +1, space +2
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/pK6/1pP5/1P2k3/P4p2/6p1/8/4N3 b - - 1 71`

- Engine re-search: `e5d5` score `-21`

### 3. Game 75, ply 158: d1e2 (Ke2)

- Categories: `engine-loss, eval-swing`
- Score: `-469 -> -758` for current-eval, delta `-289`
- Sequence after reply: `n/a`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -321, center_control +2, piece_square +30
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/8/8/8/8/1pk5/3p4/3K4 w - - 0 80`

- Engine re-search: `d1e2` score `-758`

### 4. Game 18, ply 187: g4g3 (Kg3)

- Categories: `eval-swing`
- Score: `+1648 -> +1345` for current-eval, delta `-303`
- Sequence after reply: `n/a`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -302, mobility -2, safe_mobility -1, space +2
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/8/p1p5/1p2P3/5nk1/3r4/8/6K1 b - - 5 96`

- Engine re-search: `a6a5` score `1496`

### 5. Game 20, ply 120: f7g8 (Kg8)

- Categories: `engine-loss, eval-swing`
- Score: `-10 -> -275` for current-eval, delta `-265`
- Sequence after reply: `n/a`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -243, piece_square -27, king_safety +5
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/1p1KPkp1/1P5p/p4p1P/7P/B7/8/8 b - - 2 62`

- Engine re-search: `f5f4` score `-682`

### 6. Game 75, ply 160: e2f2 (Kf2)

- Categories: `engine-loss, eval-swing`
- Score: `-836 -> -1083` for current-eval, delta `-247`
- Sequence after reply: `n/a`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -246, center_control -1
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/8/8/8/8/2k5/1p1pK3/8 w - - 0 81`

- Engine re-search: `e2e3` score `-1429`

### 7. Game 5, ply 196: a2a3 (Ka3)

- Categories: `engine-loss, eval-swing`
- Score: `-675 -> -905` for current-eval, delta `-230`
- Sequence after reply: `n/a`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -229, piece_square -1
- Result: `0-1` `black_win:checkmate`
- FEN before: `7b/8/8/8/8/8/Kpk5/8 w - - 46 100`

- Engine re-search: `a2a3` score `-1323`

### 8. Game 13, ply 148: e4d3 (Kd3)

- Categories: `engine-loss, eval-swing`
- Score: `-725 -> -946` for current-eval, delta `-221`
- Sequence after reply: `n/a`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -205, piece_square -10, space -4, center_control -4, threats +2
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/8/8/8/4K3/6kp/4p3/8 w - - 2 78`

- Engine re-search: `e4d5` score `-1330`

### 9. Game 58, ply 109: f6f5 (Kf5)

- Categories: `engine-loss, eval-swing`
- Score: `-738 -> -947` for current-eval, delta `-209`
- Sequence after reply: `n/a`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -220, king_safety -3, center_control +4, piece_square +8
- Result: `1-0` `white_win:checkmate`
- FEN before: `2R2N2/7P/p4k2/1pp4K/5P2/6P1/8/8 b - - 0 57`

- Engine re-search: `f6g7` score `-988`

### 10. Game 31, ply 90: e3f4 (Kf4)

- Categories: `engine-loss, eval-swing`
- Score: `-603 -> -811` for current-eval, delta `-208`
- Sequence after reply: `n/a`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -190, threats -17, center_control -3, piece_square -2, space +2, king_safety +5
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/k5P1/p7/4P3/3P4/1p1qK2P/4b3/8 w - - 1 47`

- Engine re-search: `e3f4` score `-1089`

### 11. Game 86, ply 167: e4e3 (Ke3)

- Categories: `engine-loss, eval-swing`
- Score: `-191 -> -389` for current-eval, delta `-198`
- Sequence after reply: `n/a`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -188, piece_square -10, center_control -4, space +4
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/8/5K2/7P/4k3/8/8/8 b - - 10 85`

- Engine re-search: `e4d4` score `-371`

### 12. Game 20, ply 122: a5a4 (a4)

- Categories: `engine-loss, eval-swing`
- Score: `-9 -> -204` for current-eval, delta `-195`
- Sequence after reply: `n/a`
- Material: `P` captures `-` for `0` cp
- Component movement: pawn_structure -180, piece_square -15
- Result: `1-0` `white_win:checkmate`
- FEN before: `6k1/1p1KP1p1/1P5p/p4p1P/7P/8/1B6/8 b - - 4 63`

- Engine re-search: `a5a4` score `-675`

### 13. Game 13, ply 132: d3d4 (Kd4)

- Categories: `engine-loss, eval-swing`
- Score: `-160 -> -355` for current-eval, delta `-195`
- Sequence after reply: `n/a`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -213, center_control +4, piece_square +10
- Result: `0-1` `black_win:checkmate`
- FEN before: `8/8/8/8/7p/3Kp3/5k1P/8 w - - 2 70`

- Engine re-search: `d3c3` score `-172`

### 14. Game 61, ply 87: b4a6 (Na6)

- Categories: `eval-swing`
- Score: `+3140 -> +2928` for current-eval, delta `-212`
- Sequence after reply: `n/a`
- Material: `N` captures `-` for `0` cp
- Component movement: pawn_structure -174, piece_square -27, mobility -5, bishop_quality -5, space +1, center_control +2
- Result: `1-0` `white_win:checkmate`
- FEN before: `3Q4/k7/8/1B6/1N3P2/2K5/P5PP/7R w - - 1 44`

- Engine re-search: `d8c7` score `3117`

### 15. Game 20, ply 106: c8f8 (Rf8)

- Categories: `engine-loss, eval-swing`
- Score: `+329 -> +142` for current-eval, delta `-187`
- Sequence after reply: `n/a`
- Material: `R` captures `-` for `0` cp
- Component movement: pawn_structure -178, rook_files -6, threats -5, king_safety +2
- Result: `1-0` `white_win:checkmate`
- FEN before: `2r3k1/1pP1K1p1/1P2Pp1p/p1B4P/8/7P/8/8 b - - 1 55`

- Engine re-search: `a5a4` score `320`

### 16. Game 20, ply 110: c8f8 (Rf8)

- Categories: `engine-loss, eval-swing`
- Score: `+304 -> +117` for current-eval, delta `-187`
- Sequence after reply: `n/a`
- Material: `R` captures `-` for `0` cp
- Component movement: pawn_structure -178, rook_files -6, threats -5, king_safety +2
- Result: `1-0` `white_win:checkmate`
- FEN before: `2r3k1/1pP1K1p1/1P2Pp1p/p6P/7P/B7/8/8 b - - 2 57`

- Engine re-search: `f6f5` score `286`

### 17. Game 76, ply 143: e5e4 (Ke4)

- Categories: `engine-loss, eval-swing`
- Score: `-256 -> -440` for current-eval, delta `-184`
- Sequence after reply: `n/a`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -197, piece_square -1, space +3, threats +9
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/pK6/1pP5/1P2k3/P7/5Np1/8/8 b - - 0 72`

- Engine re-search: `e5d5` score `-452`

### 18. Game 86, ply 165: d4e4 (Ke4)

- Categories: `engine-loss, eval-swing`
- Score: `-166 -> -346` for current-eval, delta `-180`
- Sequence after reply: `n/a`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -180
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/6K1/8/7P/3k4/8/8/8 b - - 8 84`

- Engine re-search: `d4e5` score `-160`

### 19. Game 86, ply 161: d6c5 (Kc5)

- Categories: `engine-loss, eval-swing`
- Score: `-186 -> -362` for current-eval, delta `-176`
- Sequence after reply: `n/a`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -180, space +4
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/6K1/3k4/7P/8/8/8/8 b - - 4 82`

- Engine re-search: `d6e5` score `-160`

### 20. Game 6, ply 117: c7d6 (Kd6)

- Categories: `engine-loss, eval-swing`
- Score: `-278 -> -452` for current-eval, delta `-174`
- Sequence after reply: `n/a`
- Material: `K` captures `-` for `0` cp
- Component movement: pawn_structure -203, mobility -6, space -5, threats -4, king_safety +9, piece_square +26
- Result: `1-0` `white_win:checkmate`
- FEN before: `8/1Rk5/P7/8/P3p3/2p1Kp2/8/8 b - - 1 60`

- Engine re-search: `c7c6` score `-405`
