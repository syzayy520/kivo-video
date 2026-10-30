# P13-E：口径前缀 —— 让裸 `p99` 在结构上不存在

> 卡片来源：`63-p13d-status.md` §九-1。
> P13-D 证明了抽稀能跨帧率比，却把"这是哪套口径"留在了 `sampling.mode` 一个字段里 ——
> 而引用 p99 的地方，从来没人在三行之外去找它。

---

## 一、问题：写进一个字段，不等于写得进去

P13-D 的产出是对的：抽稀后 30/60/4K 都是 160 点，采样率差 0.21%。
但证据长成这样：

```json
"av_drift": {
  "sampling": { "mode": "media_time_decimated", ... },
  "samples": 160,
  "p50": 375000, "p95": 625000, "p99": 625000, "max": 3750000,
  "per_frame": { "p99": 1875000, ... }
}
```

口语里我会说"这份证据 p99 是 0.625 ms"。这句话省掉了最关键的一半。
而且**同一个键名装过两种东西**：抽稀不成立的那一轮，`av_drift.p99` 会变成逐帧的值，
路径不变、字段不变，只有三行之外的 `sampling.mode` 变了。任何按路径取数的脚本、
任何复制粘贴进表格的人，都拿到一个"看起来正常"的数。

这跟 `audio.opened` 恒为 false 是同一类错：**恒错/歧义的字段比缺失的字段更危险**，
因为它长得像观测值。上一轮是靠"两个字段放一起看"抓到的；这一轮不打算再靠这个 ——
只要还能靠"记得去看"，就一定会有人不记得。

判据只有一句：**想取一个 p99，必须先说出是哪套口径。**

---

## 二、结构：`quantiles.<口径>`

```json
"av_drift": {
  "measurable": true,
  "measured_at": "present",
  "definition": "...",
  "sampling": {
    "mode": "media_time_decimated",
    "cross_frame_rate_set": "quantiles.media_time_decimated",
    "interval_ns": 50000000, "target_hz": 20,
    "frames_observed": 480, "accepted": 160, "skipped": 320,
    "unobserved_cells": 0, "observed_hz": 20.13,
    "reason": ""
  },
  "quantiles": {
    "media_time_decimated": { "samples": 160, "p50": ..., "p99": ..., "max": ..., "histogram": {...} },
    "per_frame":            { "samples": 480, "p50": ..., "p99": ..., "max": ..., "histogram": {...} }
  },
  "trend": { ... }
}
```

三处刻意的设计：

1. **`av_drift` 顶层没有任何分位数。** `samples` / `p50` / `p95` / `p99` / `max` /
   `histogram` / `per_frame` 出现在顶层就是错 —— 校验器判红（判据 E1）。
   这不是命名约定，是"取不到"：`json["av_drift"]["p99"]` 直接 KeyError。
2. **`cross_frame_rate_set` 是一个路径，不是一个布尔。** 布尔只回答"有得比"，
   路径回答"比的是哪一份"。它要么精确等于 `quantiles.media_time_decimated`
   且该路径解析得到，要么是 `null`。
3. **`per_frame` 永远在。** G6 的 `over_budget_frames` 必须逐帧数（抽稀会漏掉
   恰好落空的那几帧），所以抽稀成立时两份都写，不成立时只剩逐帧那份。

`sampling.mode` 仍然保留，但语义收窄成"这一轮**产出了**哪个口径"，
取值只有 `media_time_decimated` / `per_frame` / `not_measured` 三个，
由 `core/pace/sampling_mode` 唯一一张名字表给出。P13-D 里的 `per_frame_fallback`
这个名字废掉了：它把"降级"和"口径"混在一个词里，读的人分不清说的是数据还是过程。

### 为什么升 `/3` 而不是在 `/2` 上打补丁

在 `/2` 上同时保留顶层 `p99` 和 `quantiles` 会有两条通往同一个数的路，
那就等于又造了一次"两套口径"——正是这张卡要拆的东西。
`/2` 的证据不重写（重写等于伪造），校验器对 `/2` 报"不适用"而不是"失败"；
`docs/evidence/p13d/` 的 26 份全部**重新采**，因为它们和代码必须对应同一个构建。

---

## 三、不可比时：缺席，不是顶替

| 情况 | `sampling.mode` | `cross_frame_rate_set` | `quantiles` |
| --- | --- | --- | --- |
| 正常 | `media_time_decimated` | `"quantiles.media_time_decimated"` | 两份都在 |
| 抽稀不成立（没传 pts） | `per_frame` | `null` | 只有 `per_frame` |
| 根本量不出来 | `not_measured` | `null` | **`{}`（空对象）** |

不可测时给空对象而不是给 0：`{}` 说的是"一份都没有"，`0` 说的是"量到了零"——
后者是个能算进平均值的假观测。写表格的人拿到 `{}` 会空一格；拿到 0 会算出一个
看起来很正常的比值，然后据此得出一个结论。

这是本卡唯一"宁可少一个数"的地方，也是刻意的：**表格空一格会被发现，假的 0.23×
不会。**

---

## 四、校验：三个新判据 + 一次补漏

`tools/verify_p13d_drift.py` 新增：

| 判据 | 内容 | 失败意味着 |
| --- | --- | --- |
| E1 | `av_drift` 顶层无裸分位数 | 有人把口径从结构里退回了字段里 |
| E2 | `cross_frame_rate_set` 指得到真实存在的那一份，或为 `null` | 有人声称可比，但没有那一份 |
| E3 | `accepted > 0` ⇔ 抽稀那一份存在 | 有样本却没落盘 / 没样本却凑了一份空分布 |

另外补上一个**真实的漏洞**：`verify_p12_all.py` 的 `card_checks` 分发里只写了
`kivo.p12c.sync/1`，于是 P13-D 的 26 份 `/2` 证据**完全跳过了层 2**——
能在白名单里、能解析，但"可测却没有样本""不可测却无理由"一条都没查过。
改成分发 `kivo.p12c.sync/` 前缀 + 版本感知取样本数。

`verify_p13d_drift.py` 同时**接进总收口的层 4**。一个"要靠人记得跑"的判据等于没有判据：
证据可以一直躺在盘上，没人复算过。

---

## 五、家族树

```
core/pace/
├── sampling_mode.hpp/.cxx        # 三个线格式名字 + 「能不能跨帧率比」的唯一答案
```

新增单测 `tests/video/pace/p13e_sampling_mode_tests/`：4 例 17 断言。
守的是名字逐字不变（线格式）、三个名字互不相同（不许合并成一个 `lateness`）、
只有抽稀口径可比、未登记枚举值必须**明显**非法（不许返回一个像正常口径的字符串）。

两处退出点同步改口径前缀：
- `av_sync_report.cxx`（证据 JSON，见 §二）
- `jni_session_handle.cxx` 的 logcat 摘要行：`p99=0.1ms` → `pace_lateness_p99[per_frame]=0.1ms`。
  那条日志是 `adb logcat` 里最常被复制的一行，复制出去只剩一个裸数；
  而且它和 `av_drift` 的抽稀 p99 不是同一个数，对着比会得出"两处矛盾"的假结论。

---

## 六、不做什么

- **不给 `present_jitter` 套同样的结构。** 它的 p99 是帧间间隔，不存在"两种口径"，
  加一层嵌套只会让读者多跳一次。歧义在哪里，手术就做到哪里。
- **不把 D1/D2 接进总收口。** 它们需要同一批证据**成对**喂进去；
  总收口按目录逐份遍历，凑不齐一对会误判成失败。手动跑，命令写在层 4 的注释里。
- **不重写 `/1`、`/2` 的证据。** 那是伪造。
- **不动 `decode/13` 的 `pace` 块。** 那个 p99 属于解码平面，与 `av_drift` 无关；
  只在 logcat 那一行加了口径后缀，避免两处数字被并排比较。
