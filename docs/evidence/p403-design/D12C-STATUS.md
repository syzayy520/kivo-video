# D12-C 状态：新宿主 mpv 指标契约（B1/B2）+ ui_gate 回归修复

日期：2026-11-18
上游：`D12B-STATUS.md` 的 B1（探针契约）/ B2（证据 schema）未完成项
设计：`docs/design/playback/345-mpv-metric-contract.md`

---

## 一、本轮做了什么

| # | 内容 | 结果 |
|---|---|---|
| 1 | **修 ui_gate FAIL**：`PlayerGlassMenu.kt` 304 行（G2-C 加语义造成） | 拆出 `PlayerGlassMenuParts.kt`（120 行），主文件 205 行 |
| 2 | **修存量超限**：`PlaybackMetrics.kt` 312 行 | 拆出 `MetricsJson.kt`（60 行），主文件 297 行 |
| 3 | **新建 `MpvHealthProbe`**（130 行） | 10 个 mpv 真实属性 + `incomparable` 声明 |
| 4 | 接线 `PlaybackMetrics` / `MpvCacheDiag` / `MpvStartSequence` / `PlayerSessionBridge` | `mpv_health` 块落盘 |
| 5 | 契约文档 | `docs/design/playback/345-mpv-metric-contract.md` |

`assembleDebug` = **BUILD SUCCESSFUL**；`tools/ui_gate.py` = **PASS（无回归）**。

---

## 二、验证结果

### 2.1 字段完整性（真机 K40 b6e950ee）

| 样本 | 内容 | 实采 | 说明 |
|---|---|---|---|
| `d12c-w1.json` | 25fps + 音轨 | **10/10** | 全字段有值 |
| `d12c-wa.json` | 无音轨（**修复前**） | 3/10 | 空快照污染缓存，复现偶发缺陷 |
| `d12c-y1~y4.json` | 无音轨（修复后，4 次） | **9/10 × 4** | 唯一缺 `avsync_s`，符合预期 |

连续 7 次（`wb`/`wc`/`x1-x3`/`y1-y4`）稳定，无抖动。

### 2.2 诚实性（不补 0）

无音轨片断：`avsync_s = "not_measured: mpv 属性读不到"` —— mpv 确实不提供该属性，契约如实写清，**未写 0**。

### 2.3 口径对照（最有价值的一条）

| 量具 | 25fps 源 + 帧插值开 |
|---|---|
| `estimated-vf-fps` | **25.0** |
| SurfaceFlinger 上屏 | **60.54 fps**（p50 16.52 ms，0 次 >25 ms，5 轮去重 99 帧） |

⇒ **插值确实生效**，但该 property 反映的是 vf 链（≈源帧率），**不能用于插值验收**。
证据：`d12c-sf-summary.txt`、`d12c-sf-raw.txt`。

---

## 三、修掉的两个真缺陷（不是本轮引入，是暴露的）

### ① 收尾现读必然全空
`onStop` 里 `onPlayerStop()` 先销毁内核，`finish()` 再读属性 → 全 `not_measured`。
四次复现（`s1`/`s2`/`t1`/`t2`）。改为"主线程活着时采快照 + 收尾用快照"。

### ② 后台线程读 vo 属性恒 null
`KivoCacheDiag` 线程读 `estimated-vf-fps` 连续 6 次全空，主线程有值
（`demuxer-cache-time` 两边都能读）。采样因此移到主线程 `sampleTelemetry()`。

### 附带发现
无音轨片起播后约 6 s 内 `container-fps` 读不到；一次早采样会把已有快照冲成半空（`wa` 3/10）。
修复：空快照只在 `cached == null` 时占位，不覆盖有效快照。

---

## 四、不改了什么（以及为什么）

原计划"迁移 24 探针"**未执行**，因为审计证明做不到：

- 6 个时钟探针（`clock_offset_ms` / `clock_rate_ppm` / `drift_gate` / `drift_band_ms` / `step_repay` / `clock_offset_at_ms`）
  改的是**同一个变量** `ClockPositionTracker::position_ns_`，mpv 无该接缝；
  `audio-delay` 只能造恒定 A/V 偏移，造不出"钟比墙快" ⇒ 结论不可互换。
- 旧契约最硬的 7 个高频字段（`pace`/`clock_health`/`drift_gate`/`lateness_ns`/
  `drift_rebases`/`clock_injection`/`frames_dropped_late`）**全部属不可迁移类**。

⇒ 改为"立新契约 + 声明不可比"，而不是"存下来但不生效"。

---

## 五、证据清单

| 文件 | 内容 |
|---|---|
| `d12c-w1.json` | 25fps + 音轨，10/10 |
| `d12c-y1.json`~`y4.json` | 无音轨 ×4，9/10（缺 avsync） |
| `d12c-wa.json` | 无音轨失败样本（3/10），证明竞态存在 |
| `d12c-s1.json`/`s2.json`/`t1.json`/`t2.json` | 收尾现读全空样本 ×4 |
| `d12c-sf-raw.txt` / `d12c-sf-summary.txt` | SurfaceFlinger 上屏帧原始数据与结算 |
| `d12c-25fps.mp4` / `d12c-noaudio.mp4` | 测试片（**未入库**，`.gitignore:64` 忽略 `docs/evidence/**/*.mp4`；用下面两行复现） |
| `build-d12c.log` / `build-d12d.log` / `build-d12c-final.log` | 编译日志 |

测试片生成（ffmpeg，12 s testsrc）：

```bash
ffmpeg -y -f lavfi -i "testsrc=duration=12:size=1280x720:rate=25" \
       -f lavfi -i "sine=frequency=440:duration=12" \
       -c:v libx264 -preset ultrafast -pix_fmt yuv420p -c:a aac -shortest d12c-25fps.mp4
ffmpeg -y -f lavfi -i "testsrc=duration=12:size=1280x720:rate=30" \
       -an -c:v libx264 -preset ultrafast -pix_fmt yuv420p d12c-noaudio.mp4
```

---

## 六、未覆盖（如实标注）

1. `soak` 周期快照与 `kivo.case` 结构化失败证据**未迁移**。
2. `mistimed_frame_count` 与 SF `>25ms` 的口径换算**未建立**（实测 SF 0 次 vs mistimed 3）。
3. 采样线程约束（vo 属性只在主线程可读）**只验了 K40**，未跨 SoC 复验。
4. 旧宿主**未删除**：自研内核仍是 legacy/后台音频的产品回退路径，取证面要跟着内核走。
   退役门禁 E1–E8 已按事实修订，写在 345 文档 §六。
