# 355 · 两臂复验尝试 + Emby 首页卡片不可点（已修）

> 背景：354 把本地文件改成直连后，下一个最高优先项是**卡顿两臂复验**
> （`debug.kivo.originKeepAlive`：默认"每块新建连接" vs `=1` "复用连接"），
> 记忆里标注"必须下午跑，夜间源站快判不了"。
>
> 本批：**修掉一个真实 UI 缺陷**，两臂复验**三次尝试都没能给出有效对照**，
> 原因逐条记录（避免下次重踩）。

---

## 一、核心成果：Emby 首页「继续观看」整排卡片不可点（已修）

### 现象

真机点「继续观看」里任意卡片（图片/文字都试）**无任何反应**。
`uiautomator dump` 里该区域**没有 clickable 节点**（整页只有 1 个可点节点 = hero）。

### 根因

`emby/ui/EmbyBrowseRows.kt` 的 `EmbyResumeRow` 签名里有 `onOpen: (EmbyHeroItem) -> Unit`，
调用方 `EmbyBrowseScreen.kt:190` 也正确传了实现：

```kotlin
onOpen = { h -> onItem(EmbyItem(h.id, h.name, h.year, h.heroUrl, h.isSeries)) },
```

但**卡片本身没有任何 `.clickable`** —— 参数传进来却从未被调用。
典型的"接口接好了、最后一米忘了接"。

### 修法

```kotlin
Column(
    Modifier.width(208.dp)
        .clip(RoundedCornerShape(10.dp))
        .clickable(role = Role.Button) { onOpen(h) },
)
```

`clip` 只影响 ripple 范围（图片本身已是圆角），视觉零变化。

### 验证

| 项 | 修复前 | 修复后 |
|---|---|---|
| 可点击节点数 | **1**（仅 hero） | **5** |
| 点「空枪」卡片 | 无反应，仍停在 Emby 页 | 进入 `MovieDetailActivity` |

---

## 二、两臂复验：三次尝试与各自的局限（都没能给出有效对照）

### 尝试 1 · 合成片 + 限速装置（作废）

- 装置：`--mbps 0.6` + `--file`（720p testsrc，6.2 MB）
- 问题：合成片码率仅约 **52 KB/s**，而限速 0.6 MB/s ⇒ **供给是需求的 12 倍**，
  连接大部分时间在空转。
- 结果自相矛盾：B 臂（复用）`TIME_WAIT` 反而更高（6→8），与预期相反。
- ⇒ **连接行为在"供给远大于需求"时不成立，指标失效。**

### 尝试 2 · 真实影片 + 限速装置（有数据，但测不出核心）

- 装置：`--mbps 0.8` + `--file`（真实拍摄视频，402 MB / 328.8 s / **10.25 Mbps = 1.28 MB/s**）
- 供给 62% ⇒ 持续分块请求成立（300 s 内 77 个 chunk，每块 1 MiB）
- A 臂（默认不复用）数据：

| 指标 | 值 |
|---|---|
| `origin open fail` / `eof unexpected` | 4 次，**全部是 `thread interrupted`**（主动放弃，非连接错误） |
| `vo_drop_count` | 0 → **496** |
| `mistimed` | 0 → 665 |
| `avsync` | 全程 < 0.21 s |
| `container_fps` | 恒 30.03 |

- **为什么测不出核心**：限速低于片源码率 ⇒ **两臂都必然大量丢帧**（`vo_drop=496`）。
  这个场景只能证明"供给不足会丢帧"，无法区分"新建连接 vs 复用连接"。
- 装置的 `--degenerate-every` 只让**单块**降速，模拟不了真实源站的**连接吞吐衰减**
  （同一条连接用久了整体塌，正是 124.1 s 冻结的机制）。

### 尝试 3 · 真实 Emby 源站（源站当前不可用）

- 影片：《空枪》（时长 1:34:17），探测到 `total=3688473195`（**3.69 GB**）
- 源站表头 **535 ms**，随后 **拿不到首帧**：

```
播放失败
网络视频无法播放：内核播放准备失败（IllegalStateException: 内核起播超时（position=-1.0））
[cache] fw=7.6M total=72.0M dur=2.0s underrun=false rate=0.0M/s
```

### 排除回归（重要）

为确认不是 354 引入的问题，用装置重跑同一条 URL 路径（`--mbps 3`）：
**正常出画**（真实拍摄视频）。

⇒ URL 起播链路没有回归，是 Emby 公网源站当时太慢。

---

## 三、结论

| 项 | 状态 |
|---|---|
| Emby 卡片不可点 | ✅ **已修并真机验证** |
| 354 是否引入 URL 起播回归 | ✅ **已排除**（装置验证） |
| 两臂复验 | ⏳ **未完成** —— 装置测不出核心，真实源站当前不可用 |

**两臂复验必须满足三个条件，缺一不可**：

1. **真实源站可用**（起播能出画）—— 当前不满足
2. **片源码率 ≥ 源站速率**（否则两臂都丢帧，差异被淹没）
3. **持续 ≥300 s 且文件远大于可播完量**（否则播完即空转）

下一次执行：等源站恢复后，用同一部 3.69 GB 影片，`originKeepAlive=0` / `=1` 各 300 s，
判据看 `origin open fail` 与 `eof unexpected` 的**次数与类型**（区分 `thread interrupted`
与真实连接错误）、`vo_drop` 增量、以及 demux cache 是否归零。

---

## 四、本批证据

| 文件 | 内容 |
|---|---|
| `v355/what.png` / `retry.png` | 真实源站两次起播失败（含 3.69 GB 探测后仍超时） |
| `v355/armA-progress.txt` | A 臂 300 s 逐 30 s 的异常/chunk 计数 |
| `v355/armA-netdiag.txt` | A 臂完整日志（4 次异常全为 `thread interrupted`） |
| `v355/armA-health.ndjson` | A 臂 61 点健康度时间序列（`vo_drop` 0→496） |
| `v355/origin.log` / `real.log` / `origin.stdout` | 装置侧供片记录（1 MiB / 1.25 s = 0.8 MB/s） |
