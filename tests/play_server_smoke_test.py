#!/usr/bin/env python3

from __future__ import annotations

import importlib.util
import os
import sys
import tempfile
import textwrap
import time
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SERVER_PATH = ROOT / "tools" / "play_server.py"


def load_play_server():
    spec = importlib.util.spec_from_file_location("play_server", SERVER_PATH)
    module = importlib.util.module_from_spec(spec)
    assert spec is not None and spec.loader is not None
    sys.modules["play_server"] = module
    spec.loader.exec_module(module)
    return module


def write_fake_engine(directory: Path) -> Path:
    engine = directory / "fake_uci.py"
    engine.write_text(
        textwrap.dedent(
            """\
            #!/usr/bin/env python3
            import sys

            for raw in sys.stdin:
                command = raw.strip()
                if command == "uci":
                    print("id name FakeEngine", flush=True)
                    print("option name Hash type spin default 64 min 1 max 4096", flush=True)
                    print("uciok", flush=True)
                elif command == "isready":
                    print("readyok", flush=True)
                elif command.startswith("go "):
                    print("info depth 1 score cp 12 nodes 31 qnodes 4 nps 31000 time 1 pv e7e5", flush=True)
                    print("bestmove e7e5", flush=True)
                elif command == "quit":
                    break
            """
        ),
        encoding="utf-8",
    )
    engine.chmod(0o755)
    return engine


def wait_for(predicate, timeout: float = 3.0) -> None:
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if predicate():
            return
        time.sleep(0.03)
    raise AssertionError("timed out waiting for controller state")


def main() -> int:
    play_server = load_play_server()
    with tempfile.TemporaryDirectory() as tmp:
        engine_path = write_fake_engine(Path(tmp))
        controller = play_server.PlayController(engine_path)
        try:
            state = controller.new_game(
                {
                    "humanColor": "white",
                    "timeControl": {"mode": "depth", "depth": 1},
                    "engineOptions": {"Threads": 2, "Hash": 8},
                }
            )
            assert state["status"] == "playing"
            assert state["turn"] == "white"
            assert state["engineOptions"]["Threads"] == 2
            assert state["engineOptions"]["Hash"] == 8

            controller.apply_human_move({"uci": "e2e4"})
            wait_for(lambda: len(controller.snapshot()["moves"]) == 2)
            state = controller.snapshot()
            assert state["moves"][0]["uci"] == "e2e4"
            assert state["moves"][1]["uci"] == "e7e5"
            assert state["lastSearch"]["depth"] == 1

            state = controller.reset()
            assert state["status"] == "setup"
            assert state["moves"] == []
        finally:
            controller.engine.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
