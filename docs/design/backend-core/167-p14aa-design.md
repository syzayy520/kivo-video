# 167 · P14-AA 设计：`surface_at_time` 档为什么没有跨度，该怎么补

- 日期：2026-10-31
- 前置：`CURRENT.md` 的"108 份不可判"，与 `verify_p13p_episode.py` 的 R1
- 状态：**设计稿，待批准**。本文不改代码。

---

## 一、先把数字说准（原文写得太宽）

`CURRENT.md` 写的是"108 份不可判里 `surface_at_time` 是默认档"，听起来像 108 份都该补。
按版本与呈现档拆开之后不是：

| 类别 | 份数 | 真实原因 |
| --- | --- | --- |
| `sync/1–/15` 旧格式 | **90** | 那时还没有 `clock_supplied_frames` 这个字段，**历史无法补** |
| `sync/24 · 25 · 27 · 30`，`surface_at_time` | **18** | 真的缺：这一档**从来没喂过跨度** |

所以要补的是 **18 份**，不是 108 份。这一条先改文档，再谈代码 —— 把 108 说成
"都是 at_time 的锅"是把一个格式史问题算成一个设计缺陷。

## 二、18 份为什么没有跨度

`decode_receive_worker` 按档分流：

```cpp
if (at_time) { deps_.pacer->note_scheduled(decision, present_at_ns); }
else         { deps_.pacer->note_presented(decision, present_at_ns); }
```

而 `FramePacer::note_scheduled()` 只做两件事：记"提交提前量"、记交付间隔。
它**不调用** `stats_.note_presented(...)`，于是 `DueStep` 从不进入 `PaceStats` ——
`clock_supplied_frames` 恒 0、四个跨度恒 0、`media_rate` 无从计算。

这不是漏写一行：`note_presented` 里的 `lateness = present_at − due_at`，
在 at_time 档上量到的是**提交提前量**（实测 p50 = −99.875 ms ≈ 3 帧），
把它当"迟到"记进去，G6 的 p99 会被一条根本没迟到的路径撑爆。

## 三、三条路，选 B

| 方案 | 做法 | 代价 |
| --- | --- | --- |
| A | 让 `note_scheduled` 直接调 `note_presented` | ❌ 污染 lateness / 间隔直方图，G6 数字变成假的 |
| **B** | **跨度单独一本账**：抽 `note_span(DueStep, media)`，present 与 scheduled 都喂它；`clock_supplied_frames` 保持"presented 的钟读数帧数"不动，另出 `span_frames` 与 `span_basis` | 要动 R1 判据（见 §五），但每一格都有明确定义 |
| C | 什么都不做，18 份继续判不了 | 这 18 份是 at_time 的**全部**证据，等于这一档没有量程 |

选 B：它让 at_time 档第一次有 `media_rate`，且**不改变任何已有字段的含义**。

## 四、家族树（不新增模块，只抽一个私有方法）

| 文件 | 改动 |
| --- | --- |
| `pace_stats.hpp/.cxx` | 把 `note_presented` 里那段跨度记账抽成私有 `note_span()`；新增公开 `note_scheduled_step()`（只喂跨度，不碰 lateness/间隔） |
| `frame_pacer.cxx` | `note_scheduled()` 里补一次 `note_scheduled_step(decision, decision.media_ns)` |
| `av_sync_report.cxx` | `pace` 块补 `span_frames` 与 `span_basis`（`presented` / `scheduled`），schema `/37` → `/38` |
| `verify_p12c_sync.py` | `/38` 白名单；`media_rate` 的"不可判理由"改口径（不再说"at_time 不提供") |
| `tests/video/pace/p14aa_scheduled_span_tests/` | 新目录：三条断言（见 §五） |

没有新模块、没有新顶层目录 —— 这是"补一条记账路径"，不是新功能。

## 五、判据（AA 组）

| 判据 | 内容 | 打红意味着 |
| --- | --- | --- |
| AA1 | at_time 档：`span_frames ≥ 2` 且 `pts_span / elapsed_span` 非 0 | 跨度还是没接上 |
| AA2 | 同一轮 `clock_supplied_frames` 仍是 0（presented 口径没被污染） | 两本账混了 |
| AA3 | `lateness` 直方图与 `over_budget_frames` 与改动前**逐位相同** | 顺手改了 G6 的语义 |
| AA4 | `span_basis` 必须出版，且与 `video.present_schedule` 一致 | 读者分不清跨度是哪来的 |
| AA5 | `media_rate` 可算时，其值与 `timeline.playback_rate`（第三方对照）之差 ≤ 1e-4 | 跨度算错 |

### 五之一、开工第一件事已经查了（结果比预想的严重）

拿一份真实的 at_time 证据（`docs/evidence/p14k/p14k-4k60-sync.json`，`/30`）跑
`verify_p13p_episode.py`，**当场就红**：

```
[FAIL] P1 due_step_supplied=0 != presented=480（这一路参数没接上）
```

也就是说：不止"跨度"没接上，**`DueStep` 整条路都没接**。同一份证据里的
`due_step_frames = 0` 不是"没有位移"，是"没有样本"—— 正是 P13-P 立 P1 时
要防的那种"沉默的零"。

而总闸是绿的。原因是 `verify_p12_all.py` 只把 **`p13*` 目录**的证据喂给
`verify_p13p_episode`（`p13_files`），`p14k` 这几份 at_time 证据从来没被它扫过。
**这是"白名单与分发是两张表"的第四次发作**：判据写好了，但没被分发到。

所以这一卡的范围要扩大一条：**不只补跨度，还要补 `due_step` 的记账**，
并且把 at_time 证据纳入总闸的 p13p 扫描（否则修好了也没人守）。

## 六、不做的事

- 不把 scheduled 的提交提前量写成 lateness（方案 A 的错）。
- 不给旧 90 份补字段：那要伪造时间戳，P12-C 早期已经定过不许补。
- 不顺手改 `av_drift`：它靠 lateness，at_time 档本来就没有，**继续没有**。
  这一卡只让"跨度"可判，不假装"音画偏差"可判。
