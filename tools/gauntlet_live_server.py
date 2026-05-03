#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
from http import HTTPStatus
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from typing import Any


INDEX_HTML = r"""<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Gauntlet Live</title>
  <style>
    :root {
      color-scheme: dark;
      --bg: #101113;
      --surface: #17191d;
      --surface-2: #1f2228;
      --line: #30343c;
      --text: #f2f3f5;
      --muted: #969ca8;
      --soft: #c9cdd4;
      --accent: #67b7a4;
      --danger: #e06f66;
      --warn: #d4a84f;
      --white-square: #d7d0c3;
      --black-square: #696f67;
      --last-from: #b7a95d;
      --last-to: #d1bd58;
      --piece-white: #f8f8f4;
      --piece-black: #1b1c1f;
      --shadow: 0 24px 70px rgba(0, 0, 0, 0.34);
      font-family: ui-sans-serif, "Geist", "Satoshi", "Segoe UI", system-ui, sans-serif;
      font-size: 16px;
    }

    * {
      box-sizing: border-box;
    }

    body {
      margin: 0;
      min-height: 100dvh;
      background:
        linear-gradient(135deg, rgba(103, 183, 164, 0.08), transparent 34rem),
        radial-gradient(circle at top right, rgba(255, 255, 255, 0.055), transparent 30rem),
        var(--bg);
      color: var(--text);
    }

    button {
      font: inherit;
    }

    .shell {
      width: min(1480px, calc(100vw - 32px));
      margin: 0 auto;
      padding: 24px 0 32px;
    }

    .topbar {
      display: grid;
      grid-template-columns: 1.35fr auto;
      gap: 20px;
      align-items: end;
      border-bottom: 1px solid var(--line);
      padding-bottom: 18px;
      margin-bottom: 22px;
    }

    .eyebrow {
      display: inline-flex;
      align-items: center;
      gap: 8px;
      color: var(--muted);
      font-size: 12px;
      letter-spacing: 0.08em;
      text-transform: uppercase;
      margin-bottom: 10px;
    }

    .status-dot {
      width: 8px;
      height: 8px;
      border-radius: 999px;
      background: var(--warn);
      box-shadow: 0 0 0 4px rgba(212, 168, 79, 0.13);
    }

    .status-dot.live {
      background: var(--accent);
      box-shadow: 0 0 0 4px rgba(103, 183, 164, 0.13);
      animation: pulse 1.8s ease-in-out infinite;
    }

    .status-dot.error {
      background: var(--danger);
      box-shadow: 0 0 0 4px rgba(224, 111, 102, 0.13);
    }

    @keyframes pulse {
      0%, 100% { transform: scale(1); opacity: 1; }
      50% { transform: scale(0.72); opacity: 0.64; }
    }

    h1 {
      margin: 0;
      font-size: clamp(28px, 4vw, 54px);
      line-height: 0.98;
      letter-spacing: 0;
      font-weight: 730;
    }

    .subtitle {
      margin: 12px 0 0;
      color: var(--muted);
      max-width: 72ch;
      line-height: 1.55;
    }

    .controls {
      display: flex;
      gap: 10px;
      flex-wrap: wrap;
      justify-content: flex-end;
    }

    .button {
      border: 1px solid var(--line);
      background: var(--surface);
      color: var(--text);
      min-height: 38px;
      border-radius: 7px;
      padding: 0 14px;
      cursor: pointer;
      transition: transform 180ms cubic-bezier(0.16, 1, 0.3, 1), border-color 180ms, background 180ms;
    }

    .button:hover {
      border-color: #48505c;
      background: var(--surface-2);
    }

    .button:active {
      transform: translateY(1px) scale(0.99);
    }

    .layout {
      display: grid;
      grid-template-columns: minmax(420px, 0.95fr) minmax(360px, 0.7fr) minmax(360px, 0.62fr);
      gap: 20px;
      align-items: start;
    }

    .panel {
      background: rgba(23, 25, 29, 0.92);
      border: 1px solid var(--line);
      border-radius: 8px;
      box-shadow: var(--shadow);
      overflow: hidden;
    }

    .panel-header {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 14px;
      padding: 16px 18px;
      border-bottom: 1px solid var(--line);
      background: rgba(255, 255, 255, 0.02);
    }

    .panel-title {
      font-size: 13px;
      color: var(--soft);
      text-transform: uppercase;
      letter-spacing: 0.08em;
    }

    .panel-note {
      color: var(--muted);
      font-size: 12px;
      text-align: right;
    }

    .board-wrap {
      padding: 18px;
    }

    .players {
      display: grid;
      gap: 10px;
      margin-bottom: 14px;
    }

    .player {
      display: grid;
      grid-template-columns: 1fr auto;
      gap: 12px;
      align-items: center;
      padding: 11px 12px;
      background: var(--surface-2);
      border: 1px solid var(--line);
      border-radius: 7px;
    }

    .player.active {
      border-color: rgba(103, 183, 164, 0.68);
    }

    .player-name {
      min-width: 0;
      font-weight: 670;
      white-space: nowrap;
      overflow: hidden;
      text-overflow: ellipsis;
    }

    .player-color {
      color: var(--muted);
      font-size: 12px;
      margin-top: 2px;
    }

    .clock {
      font-family: ui-monospace, "JetBrains Mono", "Cascadia Mono", monospace;
      font-size: 17px;
      color: var(--text);
    }

    .board {
      display: grid;
      grid-template-columns: repeat(8, 1fr);
      aspect-ratio: 1 / 1;
      border: 1px solid var(--line);
      border-radius: 7px;
      overflow: hidden;
      background: var(--line);
    }

    .square {
      position: relative;
      display: grid;
      place-items: center;
      min-width: 0;
      min-height: 0;
    }

    .square.light { background: var(--white-square); }
    .square.dark { background: var(--black-square); }
    .square.last-from { background: color-mix(in srgb, var(--last-from), var(--white-square) 42%); }
    .square.last-to { background: color-mix(in srgb, var(--last-to), var(--black-square) 28%); }

    .coord {
      position: absolute;
      font-family: ui-monospace, "JetBrains Mono", monospace;
      font-size: clamp(8px, 1.1vw, 11px);
      color: rgba(20, 22, 24, 0.55);
      pointer-events: none;
    }

    .coord.file { right: 5px; bottom: 3px; }
    .coord.rank { left: 5px; top: 3px; }

    .piece {
      width: 70%;
      height: 70%;
      border-radius: 999px;
      display: grid;
      place-items: center;
      font-family: ui-monospace, "JetBrains Mono", monospace;
      font-weight: 800;
      font-size: clamp(18px, 4.8vw, 42px);
      line-height: 1;
      letter-spacing: 0;
      box-shadow: inset 0 1px 0 rgba(255,255,255,0.5), 0 6px 14px rgba(0,0,0,0.22);
    }

    .piece.white {
      color: var(--piece-black);
      background: var(--piece-white);
      border: 1px solid rgba(30, 32, 35, 0.18);
    }

    .piece.black {
      color: var(--piece-white);
      background: var(--piece-black);
      border: 1px solid rgba(255, 255, 255, 0.12);
    }

    .game-meta {
      display: grid;
      grid-template-columns: repeat(3, 1fr);
      gap: 10px;
      margin-top: 14px;
    }

    .metric {
      border-top: 1px solid var(--line);
      padding-top: 10px;
    }

    .metric-label {
      color: var(--muted);
      font-size: 12px;
      margin-bottom: 3px;
    }

    .metric-value {
      font-family: ui-monospace, "JetBrains Mono", monospace;
      font-size: 18px;
      color: var(--text);
      overflow-wrap: anywhere;
    }

    .moves {
      max-height: min(72dvh, 760px);
      overflow: auto;
      padding: 8px 0;
    }

    .move-row {
      display: grid;
      grid-template-columns: 56px 1fr 1fr;
      gap: 10px;
      align-items: center;
      min-height: 38px;
      padding: 0 16px;
      border-bottom: 1px solid rgba(48, 52, 60, 0.58);
      font-family: ui-monospace, "JetBrains Mono", monospace;
      font-size: 14px;
    }

    .move-number {
      color: var(--muted);
    }

    .move {
      color: var(--text);
      padding: 6px 8px;
      border-radius: 6px;
      min-height: 28px;
    }

    .move.last {
      background: rgba(103, 183, 164, 0.14);
      color: #dff8f1;
    }

    .summary {
      padding: 18px;
      display: grid;
      gap: 16px;
    }

    .scoreline {
      display: grid;
      gap: 10px;
    }

    .engine-score {
      display: grid;
      grid-template-columns: 1fr auto;
      gap: 14px;
      align-items: center;
      border-bottom: 1px solid var(--line);
      padding-bottom: 12px;
    }

    .engine-name {
      color: var(--soft);
      font-weight: 650;
      overflow: hidden;
      white-space: nowrap;
      text-overflow: ellipsis;
    }

    .score {
      font-family: ui-monospace, "JetBrains Mono", monospace;
      font-size: 20px;
    }

    .bar {
      height: 9px;
      border-radius: 99px;
      background: #2a2e35;
      overflow: hidden;
      border: 1px solid var(--line);
    }

    .bar-fill {
      width: 0%;
      height: 100%;
      background: var(--accent);
      transition: width 450ms cubic-bezier(0.16, 1, 0.3, 1);
    }

    .results {
      display: grid;
      gap: 8px;
      max-height: 340px;
      overflow: auto;
      padding-right: 4px;
    }

    .result-row {
      display: grid;
      grid-template-columns: 42px 1fr auto;
      gap: 10px;
      align-items: center;
      padding: 10px 0;
      border-bottom: 1px solid rgba(48, 52, 60, 0.68);
      font-size: 13px;
    }

    .result-row .reason {
      color: var(--muted);
      margin-top: 2px;
      overflow-wrap: anywhere;
    }

    .pill {
      display: inline-flex;
      align-items: center;
      justify-content: center;
      min-height: 26px;
      padding: 0 9px;
      border: 1px solid var(--line);
      border-radius: 999px;
      color: var(--soft);
      background: rgba(255, 255, 255, 0.03);
      font-family: ui-monospace, "JetBrains Mono", monospace;
      font-size: 12px;
      white-space: nowrap;
    }

    .empty {
      color: var(--muted);
      line-height: 1.5;
      padding: 22px 18px;
    }

    .error-box {
      display: none;
      margin-top: 14px;
      color: #ffe0dc;
      background: rgba(224, 111, 102, 0.11);
      border: 1px solid rgba(224, 111, 102, 0.36);
      border-radius: 7px;
      padding: 12px 14px;
    }

    .error-box.visible {
      display: block;
    }

    @media (max-width: 1180px) {
      .layout {
        grid-template-columns: minmax(380px, 1fr) minmax(320px, 0.8fr);
      }
      .summary-panel {
        grid-column: 1 / -1;
      }
    }

    @media (max-width: 820px) {
      .shell {
        width: min(100vw - 20px, 720px);
        padding-top: 16px;
      }
      .topbar {
        grid-template-columns: 1fr;
        align-items: start;
      }
      .controls {
        justify-content: flex-start;
      }
      .layout {
        grid-template-columns: 1fr;
      }
      .game-meta {
        grid-template-columns: 1fr;
      }
      .moves {
        max-height: 440px;
      }
    }
  </style>
</head>
<body>
  <main class="shell">
    <section class="topbar">
      <div>
        <div class="eyebrow"><span id="statusDot" class="status-dot"></span><span id="connection">Waiting for live state</span></div>
        <h1>Gauntlet Live</h1>
        <p class="subtitle">Watch the current gauntlet game as it is played. The board, move list, clocks, and aggregate score update from the runner's live-state file.</p>
        <div id="errorBox" class="error-box"></div>
      </div>
      <div class="controls">
        <button class="button" id="flipButton" type="button">Flip board</button>
        <button class="button" id="refreshButton" type="button">Refresh now</button>
      </div>
    </section>

    <section class="layout">
      <article class="panel">
        <div class="panel-header">
          <div class="panel-title">Board</div>
          <div class="panel-note" id="gameLabel">No active game</div>
        </div>
        <div class="board-wrap">
          <div class="players">
            <div class="player" id="blackPlayer">
              <div>
                <div class="player-name" id="blackName">Black</div>
                <div class="player-color">Black</div>
              </div>
              <div class="clock" id="blackClock">--:--</div>
            </div>
            <div class="player" id="whitePlayer">
              <div>
                <div class="player-name" id="whiteName">White</div>
                <div class="player-color">White</div>
              </div>
              <div class="clock" id="whiteClock">--:--</div>
            </div>
          </div>
          <div id="board" class="board" aria-label="Current chess board"></div>
          <div class="game-meta">
            <div class="metric">
              <div class="metric-label">Opening</div>
              <div class="metric-value" id="opening">--</div>
            </div>
            <div class="metric">
              <div class="metric-label">Ply</div>
              <div class="metric-value" id="plies">0</div>
            </div>
            <div class="metric">
              <div class="metric-label">Result</div>
              <div class="metric-value" id="result">ongoing</div>
            </div>
          </div>
        </div>
      </article>

      <article class="panel">
        <div class="panel-header">
          <div class="panel-title">Move stream</div>
          <div class="panel-note" id="lastMove">Last move --</div>
        </div>
        <div id="moves" class="moves"><div class="empty">No moves yet.</div></div>
      </article>

      <article class="panel summary-panel">
        <div class="panel-header">
          <div class="panel-title">Match</div>
          <div class="panel-note" id="updatedAt">--</div>
        </div>
        <div class="summary">
          <div class="scoreline">
            <div class="engine-score">
              <div class="engine-name" id="engineA">Engine A</div>
              <div class="score" id="scoreA">0.0</div>
            </div>
            <div class="engine-score">
              <div class="engine-name" id="engineB">Engine B</div>
              <div class="score" id="scoreB">0.0</div>
            </div>
            <div class="bar"><div class="bar-fill" id="scoreBar"></div></div>
          </div>
          <div class="game-meta">
            <div class="metric">
              <div class="metric-label">Games</div>
              <div class="metric-value" id="games">0/0</div>
            </div>
            <div class="metric">
              <div class="metric-label">Clean</div>
              <div class="metric-value" id="cleanGames">0</div>
            </div>
            <div class="metric">
              <div class="metric-label">Elo diff</div>
              <div class="metric-value" id="eloDiff">--</div>
            </div>
          </div>
          <div>
            <div class="panel-title" style="margin-bottom: 8px;">Completed games</div>
            <div id="results" class="results"><div class="empty">Completed games will appear here.</div></div>
          </div>
        </div>
      </article>
    </section>
  </main>

  <script>
    const state = {
      flipped: localStorage.getItem("gauntlet-live-flipped") === "true",
      lastStateText: "",
      error: null
    };

    const files = ["a", "b", "c", "d", "e", "f", "g", "h"];
    const pieceLabels = { p: "P", n: "N", b: "B", r: "R", q: "Q", k: "K" };

    function squareIndex(square) {
      const file = square.charCodeAt(0) - 97;
      const rank = Number(square[1]) - 1;
      return { file, rank };
    }

    function pieceColor(piece) {
      return piece === piece.toUpperCase() ? "white" : "black";
    }

    function parseFen(fen) {
      const board = Array.from({ length: 8 }, () => Array(8).fill(null));
      const [placement] = fen.split(/\s+/);
      const ranks = placement.split("/");
      for (let fenRank = 0; fenRank < 8; fenRank += 1) {
        let file = 0;
        const boardRank = 7 - fenRank;
        for (const char of ranks[fenRank]) {
          if (/[1-8]/.test(char)) {
            file += Number(char);
          } else {
            board[boardRank][file] = char;
            file += 1;
          }
        }
      }
      return board;
    }

    function applyMove(board, move) {
      if (!move || move.length < 4) return;
      const from = squareIndex(move.slice(0, 2));
      const to = squareIndex(move.slice(2, 4));
      const promotion = move.length >= 5 ? move[4] : null;
      const piece = board[from.rank][from.file];
      if (!piece) return;

      const target = board[to.rank][to.file];
      const isPawn = piece.toLowerCase() === "p";
      const isKing = piece.toLowerCase() === "k";

      if (isPawn && from.file !== to.file && target === null) {
        board[from.rank][to.file] = null;
      }

      board[from.rank][from.file] = null;
      board[to.rank][to.file] = promotion
        ? (pieceColor(piece) === "white" ? promotion.toUpperCase() : promotion.toLowerCase())
        : piece;

      if (isKing && Math.abs(to.file - from.file) === 2) {
        if (to.file === 6) {
          board[from.rank][5] = board[from.rank][7];
          board[from.rank][7] = null;
        } else if (to.file === 2) {
          board[from.rank][3] = board[from.rank][0];
          board[from.rank][0] = null;
        }
      }
    }

    function boardAfterMoves(fen, moves) {
      const board = parseFen(fen);
      for (const move of moves || []) {
        applyMove(board, move);
      }
      return board;
    }

    function formatClock(ms) {
      if (ms === null || ms === undefined) return "--:--";
      const total = Math.max(0, Math.ceil(ms / 1000));
      const minutes = Math.floor(total / 60);
      const seconds = String(total % 60).padStart(2, "0");
      return `${minutes}:${seconds}`;
    }

    function formatScore(value) {
      if (value === null || value === undefined) return "0.0";
      return Number(value).toFixed(1);
    }

    function renderBoard(current) {
      const boardEl = document.getElementById("board");
      boardEl.innerHTML = "";
      const fen = current?.start_fen || "8/8/8/8/8/8/8/8 w - - 0 1";
      const moves = current?.moves || [];
      const board = boardAfterMoves(fen, moves);
      const last = current?.last_move;
      const lastFrom = last ? squareIndex(last.slice(0, 2)) : null;
      const lastTo = last ? squareIndex(last.slice(2, 4)) : null;

      const ranks = state.flipped ? [0,1,2,3,4,5,6,7] : [7,6,5,4,3,2,1,0];
      const visibleFiles = state.flipped ? [7,6,5,4,3,2,1,0] : [0,1,2,3,4,5,6,7];

      for (const rank of ranks) {
        for (const file of visibleFiles) {
          const square = document.createElement("div");
          const dark = (rank + file) % 2 === 1;
          square.className = `square ${dark ? "dark" : "light"}`;
          if (lastFrom && lastFrom.rank === rank && lastFrom.file === file) square.classList.add("last-from");
          if (lastTo && lastTo.rank === rank && lastTo.file === file) square.classList.add("last-to");

          if ((state.flipped ? rank === 7 : rank === 0)) {
            const coord = document.createElement("span");
            coord.className = "coord file";
            coord.textContent = files[file];
            square.appendChild(coord);
          }
          if ((state.flipped ? file === 7 : file === 0)) {
            const coord = document.createElement("span");
            coord.className = "coord rank";
            coord.textContent = String(rank + 1);
            square.appendChild(coord);
          }

          const piece = board[rank][file];
          if (piece) {
            const pieceEl = document.createElement("div");
            pieceEl.className = `piece ${pieceColor(piece)}`;
            pieceEl.textContent = pieceLabels[piece.toLowerCase()];
            square.appendChild(pieceEl);
          }
          boardEl.appendChild(square);
        }
      }
    }

    function renderMoves(current) {
      const movesEl = document.getElementById("moves");
      const moves = current?.moves || [];
      if (moves.length === 0) {
        movesEl.innerHTML = '<div class="empty">No moves yet.</div>';
        return;
      }
      const rows = [];
      for (let index = 0; index < moves.length; index += 2) {
        const whiteMove = moves[index] || "";
        const blackMove = moves[index + 1] || "";
        const whiteLast = index === moves.length - 1;
        const blackLast = index + 1 === moves.length - 1;
        rows.push(`
          <div class="move-row">
            <div class="move-number">${Math.floor(index / 2) + 1}.</div>
            <div class="move ${whiteLast ? "last" : ""}">${whiteMove}</div>
            <div class="move ${blackLast ? "last" : ""}">${blackMove}</div>
          </div>
        `);
      }
      movesEl.innerHTML = rows.join("");
      movesEl.scrollTop = movesEl.scrollHeight;
    }

    function renderResults(data) {
      const resultsEl = document.getElementById("results");
      const results = data.results || [];
      if (results.length === 0) {
        resultsEl.innerHTML = '<div class="empty">Completed games will appear here.</div>';
        return;
      }
      resultsEl.innerHTML = results.slice().reverse().map(result => `
        <div class="result-row">
          <div class="pill">${result.game}</div>
          <div>
            <div>${result.white} vs ${result.black}</div>
            <div class="reason">${result.reason}</div>
          </div>
          <div class="pill">${result.result}</div>
        </div>
      `).join("");
    }

    function render(data) {
      const current = data.current;
      const summary = data.summary || {};
      const totalGames = data.total_games || 0;

      document.getElementById("engineA").textContent = data.engine_a || "Engine A";
      document.getElementById("engineB").textContent = data.engine_b || "Engine B";
      document.getElementById("scoreA").textContent = formatScore(summary.score_a);
      document.getElementById("scoreB").textContent = formatScore(summary.score_b);
      document.getElementById("games").textContent = `${summary.games || 0}/${totalGames}`;
      document.getElementById("cleanGames").textContent = String(summary.clean_games || 0);
      document.getElementById("eloDiff").textContent =
        summary.elo_diff_a === null || summary.elo_diff_a === undefined ? "--" : `${Number(summary.elo_diff_a).toFixed(1)}`;
      document.getElementById("updatedAt").textContent = data.updated_at || "--";

      const scorePct = summary.games ? Math.max(0, Math.min(100, (summary.score_a || 0) * 100 / summary.games)) : 0;
      document.getElementById("scoreBar").style.width = `${scorePct}%`;

      document.getElementById("gameLabel").textContent = current
        ? `Game ${current.game} of ${totalGames}`
        : "No active game";
      document.getElementById("opening").textContent = current?.opening || "--";
      document.getElementById("plies").textContent = String(current?.plies || 0);
      document.getElementById("result").textContent = current?.result || current?.outcome || "ongoing";
      document.getElementById("lastMove").textContent = current?.last_move ? `Last move ${current.last_move}` : "Last move --";

      document.getElementById("whiteName").textContent = current?.white || "White";
      document.getElementById("blackName").textContent = current?.black || "Black";
      document.getElementById("whiteClock").textContent = formatClock(current?.clocks?.white);
      document.getElementById("blackClock").textContent = formatClock(current?.clocks?.black);
      document.getElementById("whitePlayer").classList.toggle("active", current?.side === "white");
      document.getElementById("blackPlayer").classList.toggle("active", current?.side === "black");

      renderBoard(current);
      renderMoves(current);
      renderResults(data);
    }

    function setConnection(status, message) {
      const dot = document.getElementById("statusDot");
      dot.className = `status-dot ${status}`;
      document.getElementById("connection").textContent = message;
    }

    function setError(message) {
      const errorBox = document.getElementById("errorBox");
      if (!message) {
        errorBox.classList.remove("visible");
        errorBox.textContent = "";
        return;
      }
      errorBox.classList.add("visible");
      errorBox.textContent = message;
    }

    async function refresh() {
      try {
        const response = await fetch(`/state.json?ts=${Date.now()}`, { cache: "no-store" });
        if (!response.ok) {
          throw new Error(`state request failed with ${response.status}`);
        }
        const text = await response.text();
        if (text !== state.lastStateText) {
          state.lastStateText = text;
          render(JSON.parse(text));
        }
        setConnection("live", "Live state connected");
        setError(null);
      } catch (error) {
        setConnection("error", "Live state unavailable");
        setError(`Could not read live state: ${error.message}`);
      }
    }

    document.getElementById("flipButton").addEventListener("click", () => {
      state.flipped = !state.flipped;
      localStorage.setItem("gauntlet-live-flipped", String(state.flipped));
      if (state.lastStateText) render(JSON.parse(state.lastStateText));
    });

    document.getElementById("refreshButton").addEventListener("click", refresh);
    refresh();
    setInterval(refresh, 1000);
  </script>
</body>
</html>
"""


def empty_state() -> dict[str, Any]:
    return {
        "schema": 1,
        "updated_at": None,
        "total_games": 0,
        "engine_a": "Engine A",
        "engine_b": "Engine B",
        "summary": {
            "games": 0,
            "clean_games": 0,
            "score_a": 0.0,
            "score_b": 0.0,
            "wins_a": 0,
            "wins_b": 0,
            "draws": 0,
            "elo_diff_a": None,
        },
        "current": None,
        "results": [],
    }


class LiveHandler(BaseHTTPRequestHandler):
    state_path: Path

    def do_GET(self) -> None:
        if self.path == "/" or self.path.startswith("/?"):
            self.send_text(INDEX_HTML, "text/html; charset=utf-8")
            return
        if self.path.startswith("/state.json"):
            self.send_json(self.read_state())
            return
        self.send_error(HTTPStatus.NOT_FOUND)

    def log_message(self, format: str, *args: Any) -> None:
        return

    def read_state(self) -> dict[str, Any]:
        try:
            return json.loads(self.state_path.read_text(encoding="utf-8"))
        except FileNotFoundError:
            return empty_state()
        except json.JSONDecodeError as error:
            state = empty_state()
            state["error"] = f"Could not parse live-state.json: {error}"
            return state

    def send_text(self, body: str, content_type: str) -> None:
        encoded = body.encode("utf-8")
        self.send_response(HTTPStatus.OK)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(encoded)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(encoded)

    def send_json(self, payload: dict[str, Any]) -> None:
        encoded = (json.dumps(payload) + "\n").encode("utf-8")
        self.send_response(HTTPStatus.OK)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(encoded)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(encoded)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Serve a live web viewer for tools/gauntlet.py.")
    parser.add_argument(
        "--state",
        type=Path,
        default=Path("gauntlet-results/live-state.json"),
        help="Path to the live-state.json file written by gauntlet.py.",
    )
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8765)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    LiveHandler.state_path = args.state
    server = ThreadingHTTPServer((args.host, args.port), LiveHandler)
    url = f"http://{args.host}:{args.port}"
    print(f"Serving gauntlet viewer at {url}")
    print(f"Reading live state from {args.state}")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nStopping gauntlet viewer")
    finally:
        server.server_close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
