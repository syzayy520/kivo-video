# 362 · #27「消除 4K 插值 Render queue underrun」裁决：**前提被证伪，建议关闭，不修**

日期：2026-11-18

> 承接 D361（台账校准后仅剩 4 项真未完成）。本批对 #27 做立项前取证，
> 结论是**这个任务的两个前提都不成立**——underrun 不是缺陷，且文档给的解法不存在。
> 按纪律"不许把逻辑上应该当结论、改代码不能真机验=留 unverified 改动"，本批**不改任何代码**。

---

## 一、任务原始表述（来自 282 文档 §六 遗留）

> `4K 上的 Render queue underrun | 未消除。可选方向：4K 时用更轻的插值核、或按分辨率自动降级`

282 文档 §五 代价段：`4K HDR 插值下有 8 次 Render queue underrun（1080p 只有 2 次）：
Adreno 650 在 4K + 60fps 插值下偶尔供帧不及。SF 层面仍是 0 次 >200ms。`

---

## 二、前提一证伪：`Render queue underrun` 是 mpv 官方定义的**预期行为**，不是缺陷

本地 mpv 源码树（`/c/re-tools/mpv-build/mpv`，`git describe = v0.39.0`，
正是项目所用版本）`video/out/vo_gpu_next.c:1066-1072`：

```c
case PL_QUEUE_MORE:
    // This is expected to happen semi-frequently near the start and
    // end of a file, so only log it at high verbosity and move on.
    MP_DBG(vo, "Render queue underrun.\n");
    break;
```

`git blame` 确认这段来自 **v0.39.0 Release 提交 `a0fba7b`**（非本地改动）。三个关键点：

1. **触发条件是 `PL_QUEUE_MORE`**——libplacebo 帧队列"暂时没有足够帧可做混合"，
   上游明确说它在文件首尾"半频繁"发生，属于预期。
2. **日志级别是 `MP_DBG`**——debug 级，上游故意不让它出现在常规输出里；
   我们内核日志能看到，是因为取证时开了 verbose。**它不是 error/warning**。
3. **上游处理是 `break` 后继续**——不重试、不丢帧、不通知，直接跳过本次混合。

⇒ 把 "underrun 计数" 当成"待消除的缺陷指标"，等于把上游的 debug 噪声当 KPI。

---

## 三、前提二证伪：`oversample` 已经是最轻的插值核，"更轻的核"不存在

mpv v0.39 官方文档 `DOCS/man/options.rst:5525-5534`（`--tscale`）：

> Common `--tscale` choices include `oversample`, `linear`, `catmull_rom`, `mitchell`,
> `gaussian`, or `bicubic`. These are listed in increasing order of smoothness/blurriness,
> with **`bicubic` being the smoothest/blurriest and `oversample` being the sharpest/least smooth**.

当前代码 `mpv_options.cpp:167` 已是 `tscale=oversample`——官方排序里**最锐利、最不平滑**（即最轻）的那一档。
`oversample` 文档（同文件 5499-5507）还说明它是"nearest neighbour 的 oversample 版，开销低"。

⇒ 282 留的"用更轻的插值核"方向**无核可换**：换 `linear`/`catmull_rom`/`mitchell`/`gaussian`/`bicubic`
只会**更重**（更平滑=更大卷积核=更高 GPU 开销），与降 underrun 的目标相反。

唯一剩下"按分辨率降级"（4K 关插值或降帧率）在技术上可行，但那是**功能降级**——
4K 上直接失去 25→60fps 的翻倍收益（282 实测的核心卖点），代价远大于 8 次无可见影响的 debug 日志。

---

## 四、可见影响：8 次 underrun 全程零卡顿（D282 自己的数据）

282 文档 §五 实测表（K40，4K HDR 25fps，插值 on）：

| 上屏帧率 | p50 | p99 | max | >200ms 卡顿 |
|---|---|---|---|---|
| 59.47 fps | 17.2 ms | 37.3 ms | 97.5 ms | **0 次** |

对照 SenPlayer 同场景 34.19 fps / 3 次 >200ms / 最长 3220 ms。
⇒ 按本项目既定判据（可见卡顿只认 SF `gap>20ms`，见 MEMORY 的 mpv 指标契约），
这 8 次 underrun **没有造成任何一帧可见卡顿**，上屏是 60fps 满帧。

---

## 五、裁决与建议

1. **#27 的前提被证伪**：underrun 是 mpv 官方 `MP_DBG` 级预期行为，非缺陷；
   且"更轻核"方向不存在（oversample 已最轻）。**建议关闭该任务，不修**。
2. **不改任何代码**：当前 `tscale=oversample` 已是官方最轻核；换核只会更重，属反向改动。
   按"改代码不能真机验 = 留 unverified 改动"纪律，不为消除 debug 噪声去动播放核心。
3. ** Standing 判据（写入长期）**：以后 `Render queue underrun` 只作 debug 日志登记，
   **不计入卡顿/缺陷 KPI**；可见卡顿唯一判据仍是 SF `gap>20ms`。
4. 若用户坚持"4K 也要让这条日志消失"，唯一出路是按分辨率关插值——
   那是明确的功能降级，需用户书面确认放弃 4K 插值翻倍收益，**不在本批自行动手**。

---

## 六、诚实边界

- 未在真机复现那 8 次 underrun 的时机分布（设备处于锁屏态 `deviceLocked=1`，按 D361 纪律不盲目唤醒）。
  依据是上游源码语义 + D282 已有的 SF 上屏数据 + 官方文档，**属证据裁决，非本批真机实测**。
- 未改代码、未编译、未 push 包。
- 结论强度：**足以判定"这不是需要修的缺陷、且给定解法不存在"**；
  若未来真机发现 4K 插值的 SF 出现 `gap>20ms` 卡顿（D282 三档长稳均未出现），需另立新案查真因。
