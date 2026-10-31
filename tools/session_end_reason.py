"""收尾方式（`end_reason`）的取值与自洽规则 —— **一处定义，两处使用**（P14-R）。

被 `verify_p12_all.py`（解码侧 `outcome.end_reason`）与 `verify_p13d_drift.py`
（节拍侧顶层 `end_reason`）共用。

## 为什么单独一个模块

那三个词是**协议**，不是日志文案：判据按字符串比对，改一个字就是改协议。
而"与 `completed` / `error` 不矛盾"这四条关系，两边各写一遍就会出现同一天的两份记忆 ——
本工程为这类事付过账（`SYNC_QUANTILE_BLOCK_SCHEMAS` 的两张表只更新一张）。

## 为什么它不叫 `verify_*.py`

`verify_p12_all.py` 把 `tools/verify_*.py` 当成子校验器逐个执行。
这个模块是库，不是校验器 —— 名字必须让它不出现在那个枚举里。
"""

from __future__ import annotations

#: 合法取值。顺序就是"从最好到最坏"，但**不要**拿顺序做比较：
#: 这三个是并列的收尾方式，不是严重程度。
END_REASONS = ("completed", "interrupted", "failed")


def end_reason_problems(reason: object, completed: object, error: object) -> list[str]:
    """`end_reason` 与 `completed` / `error` 之间的矛盾，逐条列出来。

    四条关系，每条都能被一份手改过的报告单独触发：

    1. `completed=true` + `interrupted` —— 跑到 EOS 的会话不叫被打断。
    2. `completed=false` + `completed` —— 没跑完却说跑完了。
    3. `failed` + `error` 为空 —— G9：失败必须带证词。
    4. `interrupted` + `error` 非空 —— 有错就该是 `failed`，那个词专门留给"停了、没出错"。

    注意第 1 条**不**排除 `completed=true` + `failed`：视频跑到 EOS 而音频报错时，
    收尾方式确实是 `failed`（`decode` 侧 `session_end_reason` 里错误优先于完成）。
    把它当矛盾会误伤那一种真实现场。
    """
    problems: list[str] = []
    if reason not in END_REASONS:
        problems.append(f"end_reason={reason!r} 不在 {END_REASONS} 里")
        # 取值非法时后面四条都无从谈起：拿一个认不出的词去比，比出来的结论没有意义。
        return problems

    if completed is True and reason == "interrupted":
        problems.append("completed=true 却写 interrupted（跑到 EOS 就不叫被打断）")
    if completed is False and reason == "completed":
        problems.append("completed=false 却写 completed")
    if reason == "failed" and not error:
        problems.append("end_reason=failed 但 error 为空（失败没有证词）")
    if reason == "interrupted" and error:
        problems.append(f"end_reason=interrupted 但 error={error!r}（有错就该是 failed）")
    return problems
