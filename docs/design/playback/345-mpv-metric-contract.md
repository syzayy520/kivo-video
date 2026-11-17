# 345 · 新宿主 mpv 指标契约（替代旧 24 探针）

> 结论先行：**不迁移旧探针，另立新契约。**
> 旧 24 个探针里有 6 个改的是同一个变量（`ClockPositionTracker::position_ns_`），
> mpv 没有那条接缝；照搬只能得到「存下来但不生效」或「伪造同名字段」。
> 本文定义新宿主（mpv/gpu-next）能**真实回答**的量，并显式声明哪些旧指标**不可比**。

- 上游：`docs/evidence/p403-design/D12B-STATUS.md` 的 B1/B2 未完成项
- 代码：`playback/MpvHealthProbe.kt`（采集与契约）、`playback/MetricsJson.kt`（渲染）
- 证据：`docs/evidence/p403-design/d12c-*.json` / `d12c-sf-summary.txt`

---

## 一、旧链路是什么（只读审计结论）

| 事实 | 依据 |
|---|---|
| 探针入口只有一条：`PlayerActivity.readProbeExtras()` → `PlaybackSession` → JNI | `ui/PlayerActivity.kt:723-772` |
| 实际读 **24 个** key（不是文档里写的 20） | 同上 |
| 证据四份 schema：`kivo.p12c.decode/21`、`kivo.p12c.sync/45`、`kivo.p12f.negative/1`、`kivo.p211.decode/1` | `decode_stats_renderer.cxx`、`av_sync_report.cxx` 等 |
| `files/evidence/` 由 Kotlin 定路径 + C++ 写内容；`files/metrics/` 只有新宿主写 | `EvidencePaths.kt:20`、`PlaybackMetrics.kt` |
| **6 个时钟探针改的是同一个变量** | `clock_offset_source.cxx:27-52`、`clock_position_tracker.cxx:203-241` |
| 自研内核**仍在产品路径**（legacy 内核 / 后台音频回退），不是死代码 | `ui/compose/PlayerSessionStart.kt:55-73` |

> 最后一条推翻了"D12-B 是把死代码删掉"的理解：`maybeStartPlayback` 默认走 mpv，
> 只有 `debug.kivo.engine=legacy` 或开了「后台继续播放音频」才落到自研链路。
> ⇒ **旧宿主退役 ≠ 自研内核退役**，两者要分开裁决（见 §五）。

---

## 二、24 探针三分类

分类口径：
- **A** = mpv 能真实提供等价物；
- **B** = 有对应物但口径必须重写（否则会拿新数字对旧阈值做假判定）；
- **C** = mpv 无反制能力，只能标注来源。

| # | 探针 | 旧链路落点 | 实际改了什么 | 类 | 新宿主等价手段 |
|---|---|---|---|---|---|
| 1 | `kivo.media` | `openVideo` | 选片源 | **A** | `MpvStartSequence` 的 `ByteSource` |
| 2 | `kivo.pace` | `DecodeSession::attach_pacing` | 建/不建 `FramePacer` | **B** | 无 per-frame 迟到量通道 |
| 3 | `kivo.drop` | `PacePolicy::allow_drop` | 永丢帧 vs 只睡 | **B** | `--framedrop`（初始化选项，非运行时） |
| 4 | `kivo.fps` | `default_frame_duration_ns` | 首帧兜底帧长 | **A** | `container-fps` |
| 5 | `kivo.threads` | `PipelineMode::kSplit` | 送包/收帧是否分线程 | **B** | `vd-lavc-threads`（口径不同） |
| 6 | `kivo.render` | `present_frames_` | 不碰 Surface，只解码 | **B** | 无等价（`--vo=null` 会改时钟） |
| 7 | `kivo.present` | `PresentSchedule` | 谁等 due_at | **B** | 无 1:1（`video-sync` 换主时钟，不是换"谁等"） |
| 8 | `kivo.audio` | `session_open_audio` | 主时钟来源 | **A**(avsync) / **B**(口径) | `avsync`、`audio-pts` |
| 9 | `kivo.evidence` | `installEvidenceSinks` | 文件名 | — | `kivo.label` → `files/metrics/<label>.json` |
| 10-11 | `kivo.soak` / `.ms` | `soakRunnable` | 周期快照 | — | 无（本批未做） |
| 12 | `kivo.sync` | `SessionFinalizer(writesSync)` | 合并证据名 | — | 无 |
| 13 | `kivo.case` | `FailureEvidence.write` | 失败证据名 | — | `metrics.failure` 字段 |
| 14 | `kivo.clock_offset_ms` | `ClockOffsetSource::now()` | `position_ns += offset`（**持续性**） | **C** | `audio-delay` 只能造 A/V 偏移，**不是时钟注入** |
| 15 | `kivo.clock_offset_at_ms` | `arm_after_ns_` | 注入触发点 | **C** | 无 |
| 16 | `kivo.clock_rate_ppm` | `position += span*ppm/1e6` | 钟比墙快 | **C** | `--speed` 同时改音视频，墙钟比恒 1 ⇒ **不可比** |
| 17 | `kivo.drift_band_ms` | `PacePolicy::drift_band_ns` | 复位目标 | **C** | 无 |
| 18 | `kivo.drift_gate` | `policy.drift_gate_enabled` | 门开/关 | **C** | 无（mpv 同步策略内建，无计数） |
| 19 | `kivo.step_repay` | `policy.step_repay` | 台阶是否进 cum | **C** | 无 |
| 20-21 | `kivo.pause.at.ms` / `.ms` | `session_set_paused` | 定时暂停/恢复 | **A** | `set pause yes/no` |
| 22-23 | `kivo.seek.ms` / `.at.ms` | `session_seek` | seek 目标/时刻 | **A** | `MpvSeekControl` + `PlaybackMetrics.recordSeek` |
| 24 | `kivo.finalize` | `SessionFinalizer` | 进后台即收尾 | **B** | 无（新宿主 `onStop` 覆盖写） |

**统计：A 类 6 个 / B 类 8 个 / C 类 6 个 / 纯记账 4 个。**

---

## 三、真契约字段（谁在真的读）

31 个 `tools/` 脚本里，只有约 20-30 个字段被 ≥4 个脚本共同读取；
**237 个字段只有 1 个消费者**（为一次实验长出来的）。

| 字段 | 消费脚本数 |
|---|---|
| `schema` | 22 |
| `pace`（整块） | 20 |
| `clock_health` | 11 |
| `frames_dropped` | 11 |
| `drift_gate` / `drift_rebases` / `lateness_ns` / `p99` | 9 |
| `av_drift` / `drift_max_correction_ns` / `max` / `offset_ns` / `samples` | 8 |
| `clock_injection` / `duration_us` / `frames_dropped_late` / `reason` / `source` | 7 |
| `completed` / `counters` / `drift_discontinuity` / `enabled` / `error` / `events` / `frames_received` / `quantiles` | 6 |

> ⚠️ 这张表里 **7 个高频字段（`pace`/`clock_health`/`drift_gate`/`lateness_ns`/`drift_rebases`/
> `clock_injection`/`frames_dropped_late`）全部属 B 或 C 类** —— 也就是说，
> 旧契约最"硬"的部分恰好是 mpv 最给不出的部分。这正是不能"照搬字段名"的量化理由。

---

## 四、新契约：`mpv_health`

落盘位置：`files/metrics/<label>.json` 的 `mpv_health` 块（与 `metrics` 并列）。
两者分开是因为读者不同：`metrics` 是本次会话的业务指标，`mpv_health` 是内核健康度。

### 4.1 字段定义与实测（K40 b6e950ee，2026-11-18）

| 字段 | mpv property | 实测值（25fps 带音轨） | 实测值（无音轨） |
|---|---|---|---|
| `clock_source` | 常量 | `"mpv"` | `"mpv"` |
| `video_sync` | `video-sync` | `display-tempo` | `display-tempo` |
| `container_fps` | `container-fps` | `25.0` | `30.0` |
| `estimated_vf_fps` | `estimated-vf-fps` | `25.0` ⚠️ | `30.0` ⚠️ |
| `display_fps_override` | `options/display-fps-override` | `60.000004` | `60.000004` |
| `mistimed_frame_count` | `mistimed-frame-count` | `3` | `0` |
| `vo_drop_count` | `frame-drop-count` | `3` | `0` |
| `decoder_drop_count` | `decoder-frame-drop-count` | `0` | `0` |
| `avsync_s` | `avsync` | `-0.003964` | **`not_measured`** ✅ |
| `hwdec_current` | `hwdec-current` | `mediacodec` | `mediacodec` |

> ⚠️ `mistimed_frame_count` **是累计值，且不等于卡顿次数** —— 见 §5.4 的 240 s 实测对照。

**诚实性验证**：无音轨片 9/10 字段有值，唯一缺失的 `avsync_s` 写的是
`not_measured: mpv 属性读不到` —— **没有补 0**。这是本契约最核心的约定。

### 4.2 时间序列：`files/metrics/<label>-health.ndjson`

单份 `<label>.json` 只回答"最后是什么样"，回答不了"是不是在持续累积"。
`MpvHealthSoak` 每 **5 s** 追加一行（NDJSON，一行一个采样点）：

```json
{"t_ms":5598,"health":{ "clock_source": "mpv", "mistimed_frame_count": 6, ... }}
{"t_ms":10683,"health":{ ... "mistimed_frame_count": 9, ... }}
```

- 每场播放重开文件（`PlaybackMetrics.begin` 转发），不跨场混；
- 4 MB 上限，超限写 `{"truncated":"file_over_4194304_bytes"}` 一行再停 —— **截断留痕，不静默停**；
- 只有主线程采得到值（同 §5.2），所以它挂在同一条 `sampleTelemetry()` 路径上，只是节奏不同。

复算：`python tools/p345_soak_report.py <label>-health.ndjson [sf-raw.txt]`。

### 4.3 不可比项（写进 JSON 的 `incomparable` 数组）

```
pace.lateness_ns / pace.schedule_lead_ns / pace.drop_ratio
clock_injection.* / drift_gate.* / counters.frames_dropped_late
output_fps（上屏帧率，须 tools/p274_sf_latency 量）
```

输出这些名字是**给读报告的人踩刹车**：看到 `mpv_health` 就不该再去 `pace` 里找阈值。

---

## 五、实测推翻的三个假设（本轮最大收获）

### ① `estimated-vf-fps` **不能**用于帧插值验收

| 量具 | 25fps 源 + 插值开 |
|---|---|
| `estimated-vf-fps`（property） | **25.0** |
| SurfaceFlinger 上屏（`--latency`，5 轮去重） | **60.54 fps**，p50 间隔 16.52 ms，0 次 >25 ms |
| `display-fps-override` | `60.000004` |

原因：`estimated-vf-fps` 是 **vf 链**估值（≈源帧率），插值发生在 VO/display-sync 层，不经过 vf。
⇒ 拿它验收会得出"插值没开"的**错结论**。上屏帧率只能靠 SF 量具。

**这条是实测推翻的，不是推理**：我原本在代码注释里写的是"实际输出帧率：帧插值验收的主指标"，
被 SF 数据打掉后改成了现在的措辞。证据：`d12c-sf-summary.txt` + `d12c-w1.json`。

### ② 后台线程读 vo 属性恒 null

同一内核、同一时刻：
- `KivoCacheDiag` 后台线程读 `estimated-vf-fps` → **null**（连续 6 次采样全空）；
- 主线程读 → 有值。

`demuxer-cache-time` 两个线程都能读 ⇒ 不是"线程不安全"，是**vo 相关属性只在主线程可读**。
⇒ 采样必须挂在主线程路径（`sampleTelemetry()`，1 s 节流）。证据：label `u1` vs `v1`。

### ③ 收尾现读必然全空

`onStop` 里 `onPlayerStop()` 先销毁内核，之后 `finish()` 再读属性 → 全 `not_measured`。
真机 `s1`/`s2`/`t1`/`t2` 四次复现。

⇒ 修法是"活着时采快照、收尾用快照"，且**空快照不得覆盖有效快照**：
无音轨片起播后约 6 s 内 `container-fps` 读不到，一次早采样会把已有快照冲成半空
（label `wa` 3/10）。加上"仅在有实质内容时覆盖"后，连续 7 次（`wb`/`wc`/`x1-x3`/`y1-y4`）稳定 9-10/10。

### ④ `mistimed_frame_count` 不是卡顿次数

240 s 长稳（25fps 源 + 插值开，K40，`label=soak`），soak 序列与 SurfaceFlinger 同步采：

| 指标 | 240 s 内的值 |
|---|---|
| `mistimed_frame_count` | **6 → 145（+139，≈0.54 次/秒）** |
| `vo_drop_count` | **恒 2**（只在起播时发生，稳态不增长） |
| `decoder_drop_count` | 恒 0 |
| `avsync_s` | −0.003 ~ −0.027（全程 <30 ms） |
| `estimated_vf_fps` | 恒 25.0 |
| **SF 帧间隔**（3007 个轮内样本） | p50 **16.52** / p99 **16.71** / max **16.94 ms** |
| **SF >20 ms 次数** | **0** |
| 等效上屏 | **60.55 fps** |

⇒ 同一段时间里 mpv 记了 139 次 mistimed，而画面**没有任何一帧超过 20 ms**。
`mistimed` 记的是"没落在理想 vsync 的次数"（约占总帧数 1%），
**不是用户可见卡顿**；把它写成"卡了 139 次"是彻底误导。
判断可见卡顿仍以 SF 的 `gap > 20 ms` 为准，`mistimed` 只能作为"vsync 对齐严格度"的相对指标。

> 这也顺手回答了长稳判据：稳态 `vo_drop` 不增长 ⇒ 240 s 内无累积性丢帧。
> 证据：`d12d-soak-health.ndjson`（51 点）、`d12d-sf-raw.txt`（24 轮）。

---

## 六、旧宿主退役门禁（修订）

D12-STATUS 原来写"31 个脚本会断 ⇒ 阻塞"。审计后强度大幅下调：

| 事实 | 结论 |
|---|---|
| 30 个 `run_p1*.sh` + `probe_present_timing.py` 最后改动 **2026-10-31/11-01** | 冻结 |
| CI（`.github/workflows/p2-ci-build-test.yml`）**零引用** | 不在门禁里 |
| 无聚合入口（顶层无 `.sh`，脚本间无互调） | 不会被批量触发 |
| 11 月活跃实验（`p273`/`p274`/`p282`/`p291`/`p292`/`p300`）**全部用 `PlayerComposeActivity`** | 已迁走 |
| 历史证据 29 个目录已入库 `docs/evidence/p13*~p15*` | 可复算 |

⇒ 退役阻塞从"脚本会断"降级为"历史实验不可原位复现"，后者**可接受**。

但仍**不删**旧宿主，理由换成真实的两条：
1. 自研内核仍是产品回退路径（legacy / 后台音频），取证面要跟着内核走；
2. C 类 6 个探针测的是自研组件的质量，mpv 替代不了；只要组件还在产品里，实验装置就该在。

**退役前提（E1–E8 修订版）**：
- E1–E5 不变（本地软解路由、同名外挂字幕已 ✅、探针契约、证据 schema、行为 parity）
- **E6 新增**：`mpv_health` 契约稳定（本文件）+ 连续 ≥5 次真机采样无 `not_measured` 抖动
- **E7 新增**：明确 legacy 内核的下线时间表（否则旧宿主永远不能退役）
- E8：31 个脚本改为指向取证宿主或标记 `archived`

当前状态：E3 ✅（本文件）、E4 ✅（同名字幕）、E6 ✅（7 次连续稳定）；E1/E2/E5/E7/E8 未完成。

---

## 七、未覆盖项（如实标注）

1. ~~**`soak` 周期快照未迁移**~~ → ✅ 已完成，见 §4.2（`MpvHealthSoak`，5 s/点，240 s 实测 51 点）。
2. **`kivo.case` 失败证据未迁移**：新宿主只写 `metrics.failure` 字符串，没有 `kivo.p12f.negative/1` 的结构化 stage/return_code。
3. **C 类 6 探针在新宿主完全无法复现**，本文件只做到"声明不可比"，没有做到"提供替代实验"。
4. ~~**`mistimed_frame_count` 口径未与 SF 对齐**~~ → ✅ 已判定：**它不是卡顿次数**，240 s 内 +139 而 SF 0 次 >20 ms，见 §5.4。
   仍未建立的是**换算公式**（例如"1 次 mistimed ≈ 多少 ms 偏差"），因为 mpv 不暴露单次 mistimed 的偏差量。
5. 采样线程约束（§五②）只验证了 K40 一台设备，未做跨 SoC 复验。
6. 长稳只跑了 **1 场 240 s**，没有做多场景（4K / 高码率 / SMB）对照；`vo_drop` 恒 2 是否与片源无关尚未验证。
