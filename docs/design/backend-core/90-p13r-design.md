# 90 · P13-R 设计：`due` 为什么会整体前移

> 卡片：P13-R
> 前置：P13-Q（`88-p13q-design.md` 出版了两侧累积，测出满载时 `cumD` 可到 −70 ms）
> 结论：把 `due = mono + (pts − anchor_pts − elapsed)` 的四个自变量做成四个跨度出版，
> "due 为什么前移"从推断变成一道减法；真机上答案是**音频时钟位置超前墙钟**
> schema：`kivo.p12c.sync/15` → `/16`

---

## 一、问题

P13-Q 把账做平了，但只说"目标时间轴整体前移 70 ms"，没说清**是谁在动**。
`due` 的定义式里有三个自变量：

```
due = mono + (pts − anchor_pts − elapsed)
```

- `mono`：读钟那一刻的单调时刻（墙钟）；
- `pts`：本帧媒体时刻；
- `elapsed`：音频时钟相对锚点走了多久。

三者任一超前/滞后，都会让 `due` 偏。要分开，就得在**整段**尺度上看它们的跨度 ——
单帧差分只会把误差摊平成噪声。

---

## 二、判据设计

### 2.1 四个跨度，一条定义式

| 字段 | 定义 |
|---|---|
| `arrive_mono_span_ns` | 末帧与首帧的 `mono` 之差（**到达窗口**） |
| `pts_span_ns` | 末帧与首帧的 `pts` 之差（**媒体时长**） |
| `elapsed_span_ns` | 末帧与首帧的 `elapsed` 之差（**音频时钟走了多久**） |
| `due_span_ns` | 末帧与首帧的 `due_at` 之差（**目标时间轴压成多长**） |

正常分支上严格满足：

```
due_span == arrive_mono_span + pts_span − elapsed_span
```

于是 `due_span` 比 `pts_span` 短多少，就是 `elapsed_span − arrive_mono_span`
（下文记作 **`clock_ahead`**）那么多。它只有一个含义：
**音频时钟在同样的到达窗口里多走了多少。**

### 2.2 最差帧上的同一条件

`worst` 块加四格：`clock_mono_ns` / `clock_elapsed_ns` / `anchor_pts_ns` / `due_at_ns`。
它们与已有的 `media_ns` 必须满足同一条定义式（判据 R2）——
这样"最差那一帧的 `due_step` 是从哪一项冒出来的"也能当场复算。

### 2.3 接线自证

四格全是 0 有两种读法："这一轮没动" 与 "参数没接上"。后者在 P13-P / P13-Q 各撞到过一次
（schema 升了、字段没写；结构体插字段插错位置）。加 `clock_supplied_frames`：
小于 `presented` 就说明没接上，判据 R1a 直接判红。

### 2.4 独立的第三方对照

`timeline.playback_rate = pts_span / present_wall_span` 是 P12-C8 留下的、
**不与节拍器结论循环**的检查。若 `clock_ahead` 真的意味着"时间轴被压短"，
`playback_rate` 必须同步大于 1 —— 两个数来自不同的时间戳，一起错的可能性很低。

---

## 三、字段与落位

| 文件 | 改动 |
|---|---|
| `core/pace/pace_stats.hpp` | `DueStep` +4 格钟读数（**只能往后加**）；`PaceSummary` +5 格跨度 |
| `core/pace/pace_stats.cxx` | 首末值 → 四个跨度；`clock_supplied_frames` |
| `core/pace/frame_pacer.cxx` | `arrive()` 存下本帧 `mono` / `elapsed`；`note_presented()` 带进 `DueStep` |
| `core/pace/pace_worst_episode.hpp` | `EpisodeSample` 结构体（十来个裸 int64 排成一串，传反了编译器不吭声） |
| `report/av_sync_report.cxx` | `sync/16`：`pace` 五格 + `worst` 四格 |
| `tools/verify_p13p_episode.py` | R1a 接线 / R1b 跨度闭合 / R2 最差帧定义式 / R3 归类 |

---

## 四、不做什么

| 不做 | 理由 |
|---|---|
| 不因为 `clock_ahead > 0` 就去改时钟代码 | 先把"是否重现、量级多大、与 `playback_rate` 是否一致"三件事测出来；改在没测准的地方等于蒙 |
| 不在丢帧轮判跨度闭合 | 与 P13-Q 同理：两个分母不同，账不平是已知的 |
| 不把 `anchor_pts` 换成一个"更稳"的常量 | 锚点就是定义的一部分；换了等于换了一把尺子，历史证据无法对读 |

---

## 五、验收

| 编号 | 判据 |
|---|---|
| R1a | `clock_supplied_frames == presented`（接线自证） |
| R1b | `due_span == mono_span + pts_span − elapsed_span`（零 free_run 时，容差 1 µs） |
| R2 | 最差帧：`due_at == mono + pts − anchor_pts − elapsed`（容差 1 µs） |
| R3 | `clock_ahead` 与 `playback_rate` 同向：前者为正 ⇒ 后者 > 1 |
| R4 | 单测：跨度恒等式、钟超前 10 ms/帧时 `due_span` 少 30 ms |
