# D12-D 状态：soak 时间序列 + mistimed 口径判定

日期：2026-11-18
上游：`D12C-STATUS.md` 未覆盖项 #1（soak）与 #4（mistimed 口径）
契约：`docs/design/playback/345-mpv-metric-contract.md` §4.2 / §5.4

---

## 一、做了什么

| # | 内容 | 结果 |
|---|---|---|
| 1 | 新建 `MpvHealthSoak`（70 行） | 每 5 s 把 `mpv_health` 追加成 NDJSON，每场重开，4 MB 截断留痕 |
| 2 | `MpvHealthProbe.flat()` | 单行渲染供 NDJSON；缩进版仍给人读 |
| 3 | `PlaybackMetrics.begin` 转发 `soak.begin` | 生命周期与单文件指标一致 |
| 4 | `sampleTelemetry` 接入 | 同一条主线程路径，只换节奏 |
| 5 | 复算工具 `tools/p345_soak_report.py`（~140 行） | 固化两个量具坑，见 §四 |

`assembleDebug` = **BUILD SUCCESSFUL**；`PlaybackMetrics` 299 行、`MpvHealthSoak` 70 行，均 ≤300。

---

## 二、240 s 长稳实测（K40 b6e950ee）

片源：`testsrc 1280x720@25fps + sine 440Hz`、300 s（16 MB，`d12d-soak300.mp4`）
同时采：soak NDJSON（5 s/点）+ SurfaceFlinger `--latency`（10 s/轮 × 24 轮）

### mpv 侧（51 个采样点，5.6 s → 259.7 s）

| t_s | mistimed | vo_drop | dec_drop | avsync (s) | vf_fps |
|---:|---:|---:|---:|---:|---:|
| 5.6 | 6 | 2 | 0 | −0.0118 | 25.0 |
| 56.5 | 33 | 2 | 0 | −0.0143 | 25.0 |
| 107.3 | 61 | 2 | 0 | −0.0036 | 25.0 |
| 158.2 | 89 | 2 | 0 | −0.0082 | 25.0 |
| 209.0 | 117 | 2 | 0 | −0.0087 | 25.0 |
| 259.7 | **145** | **2** | **0** | −0.0105 | 25.0 |

### SF 侧（3007 个轮内间隔样本，24/24 轮可用）

| 指标 | 值 |
|---|---|
| gap p50 / p90 / p99 / max | 16.52 / 16.56 / 16.71 / **16.94 ms** |
| >16.7 ms 次数 | 36 |
| **>20 ms 次数** | **0** |
| >25 ms / >33.4 ms | 0 / 0 |
| 等效上屏 fps | **60.55** |

---

## 三、结论

### ① `mistimed_frame_count` 不是卡顿次数

同一段时间：mpv 记 **+139 次 mistimed**，而画面**没有任何一帧超过 20 ms**。
它记的是"没落在理想 vsync 的次数"（约占总帧数 1%），属 vsync 对齐严格度，不是可见卡顿。
把它写成"卡了 139 次"是彻底误导 ⇒ 已写进契约 `incomparable` 与 §5.4。

### ② 稳态无累积丢帧

`vo_drop_count` 240 s 恒为 **2**（只在起播发生），`decoder_drop_count` 恒 0
⇒ 长稳窗口内没有累积性丢帧。这就是 soak 存在的意义：单份 JSON 只能看到"最后是 2"，看不出它是否在涨。

### ③ `avsync_s` 全程健康

−0.003 ~ −0.027 s，从未超过 30 ms；无音轨时该字段为 `not_measured`（D12-C 已验证）。

---

## 四、两个量具坑（已固化进脚本）

1. **`--latency` 有 `INT64_MAX` 量级哨兵**：不过滤会把 span 算成 92 亿秒。
   第一次分析就踩了，输出 "平均上屏 fps = 0.00"。
2. **环形缓冲只有 128 帧**，多轮采样之间必然有 10 s 级间隙 ⇒ 必须按轮分组算间隔。
   合并后直接差分会得到 `p100 = 8158.96 ms` 的假卡顿。

⇒ `tools/p345_soak_report.py` 两条都处理了，用法：
`python tools/p345_soak_report.py <label>-health.ndjson [sf-raw.txt]`

---

## 五、证据清单

| 文件 | 内容 |
|---|---|
| `d12d-soak-health.ndjson` | 51 行时间序列（5.6→259.7 s） |
| `d12d-sf-raw.txt` | 24 轮 SF 原始输出（3120 行） |
| `d12d-soak.json` | 同一场的单文件指标（收尾用） |
| `d12d-soak300.mp4` | 测试片（未入库，`.gitignore:64` 忽略 `docs/evidence/**/*.mp4`） |
| `build-d12d.log` | 编译日志 |

测试片生成：
```bash
ffmpeg -y -f lavfi -i "testsrc=duration=300:size=1280x720:rate=25" \
       -f lavfi -i "sine=frequency=440:duration=300" \
       -c:v libx264 -preset ultrafast -g 50 -pix_fmt yuv420p -c:a aac -shortest d12d-soak300.mp4
```

---

## 六、未覆盖（如实标注）

1. **只跑了 1 场 240 s、1 种片源**（720p25 合成片）。4K / 高码率 / SMB 未对照，
   `vo_drop` 恒 2 是否与片源无关**未验证**。
2. **mistimed 与可见卡顿的换算公式仍未建立**（mpv 不暴露单次 mistimed 的偏差量），
   只做到"不能当卡顿次数用"这个定性判定。
3. `kivo.case` 结构化失败证据仍未迁移；C 类 6 探针仍无替代实验。
4. 采样线程约束仍只验 K40。
