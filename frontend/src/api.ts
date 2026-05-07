import type { GameState, NewGamePayload } from "./types";

async function request<T>(path: string, init?: RequestInit): Promise<T> {
  const response = await fetch(path, {
    ...init,
    headers: {
      "Content-Type": "application/json",
      ...(init?.headers ?? {})
    }
  });
  if (!response.ok) {
    throw new Error(`${response.status} ${response.statusText}`);
  }
  return (await response.json()) as T;
}

export function getState(): Promise<GameState> {
  return request<GameState>("/api/state");
}

export function newGame(payload: NewGamePayload): Promise<GameState> {
  return request<GameState>("/api/new-game", {
    method: "POST",
    body: JSON.stringify(payload)
  });
}

export function playMove(uci: string): Promise<GameState> {
  return request<GameState>("/api/move", {
    method: "POST",
    body: JSON.stringify({ uci })
  });
}

export function resetGame(): Promise<GameState> {
  return request<GameState>("/api/reset", { method: "POST", body: "{}" });
}

export function stopEngine(): Promise<GameState> {
  return request<GameState>("/api/stop", { method: "POST", body: "{}" });
}
