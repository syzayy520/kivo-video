from __future__ import annotations

from pathlib import Path


ROOT = Path(__file__).resolve().parents[5]
OUT = ROOT / "docs" / "roadmap" / "p7_command_conflict_matrix.generated.md"


RULES = [
    ("Open", "NotCreated", "Accept"),
    ("Open", "BuildFailed", "Accept"),
    ("Start", "Ready", "Accept"),
    ("Pause", "Starting", "PendingTransportIntent"),
    ("Pause", "Playing", "Accept"),
    ("Resume", "Paused", "Accept"),
    ("Seek", "Ready", "Accept"),
    ("Seek", "Playing", "Accept"),
    ("Seek", "Paused", "Accept"),
    ("SwitchAudioTrack", "Playing", "Accept"),
    ("SwitchVideoTrack", "Playing", "Accept"),
]


def render() -> str:
    lines = [
        "# P7 Generated Command Conflict Matrix",
        "",
        "| Command | State | Decision |",
        "|---|---|---|",
    ]
    lines.extend(f"| {command} | {state} | {decision} |" for command, state, decision in RULES)
    lines.append("")
    lines.append("Generated from backend/tools/video/playback_graph/matrix_codegen/p7_matrix_codegen.py.")
    return "\n".join(lines)


def main() -> int:
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(render(), encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
