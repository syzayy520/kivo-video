"""Output-slot and whole-session pace contracts; archived versions retain their rules."""

from typing import Any


def stop_accounting(holder: dict[str, Any], required: bool) -> tuple[int, int, list[str]]:
    """Return total/subset without letting undecided or dropped slots explain pace loss."""
    if not isinstance(holder, dict):
        raise ValueError("停止归还账必须位于对象中")
    keys = ("frames_discarded_on_stop", "frames_discarded_on_stop_undelivered")
    values: list[int] = []
    notes: list[str] = []
    for key in keys:
        if key not in holder:
            if required:
                raise ValueError(f"停止归还账缺字段 {key}")
            values.append(0)
            notes.append(f"旧证据缺 {key}，按 0 兼容；不据此证明停止归还能力")
        else:
            value = holder[key]
            if type(value) is not int or value < 0:
                raise ValueError(f"{key} 必须是非负整数，实际 {value!r}")
            values.append(value)
    total, undelivered = values
    if total and keys[1] not in holder:
        raise ValueError("停止归还总数非零却缺未交付子集，不能用总数解释节拍差额")
    if undelivered > total:
        raise ValueError(f"停止未交付子集 {undelivered} 超过归还总数 {total}")
    return total, undelivered, notes


def session_pace_accounting(pace: dict[str, Any], recent: dict[str, Any]) -> None:
    """Validate sync/45 and decode/21 without mixing a seek run with a session."""
    if not isinstance(pace, dict) or pace.get("accounting_scope") != "session":
        raise ValueError("pace.accounting_scope 必须为 session")
    keys = ("frames_decided", "presented", "dropped", "undelivered_frames",
            "discarded_on_stop", "discarded_no_surface")
    for key in keys:
        value = pace.get(key)
        if type(value) is not int or value < 0:
            raise ValueError(f"pace.{key} 必须是在场的非负整数，实际 {value!r}")
    _, recent_stopped, _ = stop_accounting(recent, required=True)
    recent_no_surface = recent.get("frames_discarded_no_surface")
    if type(recent_no_surface) is not int or recent_no_surface < 0:
        raise ValueError("最近段 frames_discarded_no_surface 必须是在场的非负整数")
    if (pace["discarded_on_stop"] < recent_stopped or
            pace["discarded_no_surface"] < recent_no_surface):
        raise ValueError("会话累计归属不得小于最近段归属")
    missing = pace["frames_decided"] - pace["presented"] - pace["dropped"]
    if pace["undelivered_frames"] != missing:
        raise ValueError("累计 undelivered_frames 与决策/呈现/丢弃不符")
    if missing != pace["discarded_on_stop"] + pace["discarded_no_surface"]:
        raise ValueError("会话累计帧账未闭合：未交付必须等于停止归还加无窗口归还")
