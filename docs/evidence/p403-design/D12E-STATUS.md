# D12-E 状态：三档片源长稳对照（含 38GB 真实 REMUX）

日期：2026-11-18
上游：`D12D-STATUS.md` 未覆盖项 #1（只跑 1 场 1 种片源）
契约：`docs/design/playback/345-mpv-metric-contract.md` §5.4

---

## 一、三档对照

| 场景 | 时长 | 源 | 码率 | hwdec | `vo_drop` | `dec_drop` | `mistimed` 增速 | SF p99 | SF max | SF >20ms | 上屏 |
|---|---:|---|---:|---|---:|---:|---:|---:|---:|---:|---:|
| 720p25 合成 | 254 s | testsrc | 1.2 M | mediacodec | **恒 2** | 0 | **0.55/s** | 16.71 | 16.94 | 0 | 60.55 |
| 4K25 合成 | 183 s | testsrc | 1.2 M | mediacodec | **恒 2** | 0 | **0.50/s** | 16.73 | **99.11** | **1** | 60.54 |
| **1080p24 真实 REMUX** | **213 s** | **3 Idiots 38.4 GB（SMB）** | **≈24 M** | mediacodec | **恒 0** | 0 | **0.53/s** | 16.74 | 16.96 | **0** | 60.54 |

真实片源：`\\192.168.5.36\12T\REMUX\3 Idiots 2009 BluRay REMUX 1080p AVC DTS-HD MA5.1 2Audio-CHD.mkv`
（38.4 GB，AVC 1080p，DTS-HD MA 5.1，SMB 播放，起播 `startup_total=2310 ms`）

---

## 二、三条结论

### ① `mistimed` 增速与内容完全无关 —— 它是固有比例，不是性能指标

三档片源的增速分别是 **0.55 / 0.50 / 0.53 次每秒**，覆盖 720p 合成片、4K 合成片、38 GB 真实 REMUX，
分辨率差 5 倍、码率差 20 倍，**增速几乎不变**。

这比 D12-D 的单场证据强得多：它证明 `mistimed` 反映的是「每帧是否落在理想 vsync」这种
与内容无关的固有对齐比例（约 1% 帧），**不能用来比较不同片源的性能**。

### ② `vo_drop` 不是"恒 2" —— 真实片源是 0

D12-D 里写的"`vo_drop` 恒 2"只是**合成片**的现象（testsrc 起播时丢 2 帧）。
真实 REMUX 全程 **0**。准确表述应为：

> `vo_drop` 只在起播阶段可能非 0（合成片为 2，真实片为 0），稳态一律不增长。

上一轮的"恒 2"是我从单一片源外推的，本轮修正。

### ③ 4K 合成片有 1 次 99 ms 真实卡顿，而 `mistimed` 反而涨得最少

| | 4K 合成 | 真实 REMUX |
|---|---|---|
| SF max 间隔 | **99.11 ms**（约丢 6 帧） | 16.96 ms |
| SF >20ms | **1** | 0 |
| `mistimed` 增速 | **0.50/s（最低）** | 0.53/s |

⇒ **`mistimed` 与可见卡顿不同向**。真要抓卡顿只能看 SurfaceFlinger `gap>20ms`。
这是对 D12-D 结论的加强版证据：不只是"不能当次数用"，而是**会给出方向相反的误导**。

---

## 三、顺带发现：D12-B3 的样片阻塞可以解除

`12T/REMUX` 下有 **VC-1 编码的真实影片**：

- `A Beautiful Mind 2001 BluRay REMUX 1080p VC-1 DTS-HD MA5.1 2Audio-CHD`
- `[眩晕].Vertigo.2009.FRA.BluRay.REMUX.1080p.VC-...`

VC-1 不在平台硬解列表（`hwdec-codecs` 只有 h264/hevc/mpeg4/mpeg2video/vp8/vp9/av1），
会触发 `FfmpegRouting.useFfmpeg = true` —— 正是 D12-B3「软解会话完整接线」缺的那个场景。

⇒ **D12-B3 的"阻塞：缺样片"不再成立**，可以排期。本批未做（属另一个改动面）。

---

## 四、一个必须知道的读数陷阱

`am force-stop` **不触发 `Activity.onStop`**，而单文件 `metrics/<label>.json` 的最后一次写
正是在 `onStop` 里。所以用 force-stop 收尾去读单文件，会得到**起播后 2.3 s 的早期快照**
（本次 `writes=1`、`mistimed=3`，而 soak 同期到 218 s 已是 118）。

⇒ **看趋势只认 `<label>-health.ndjson`**；单文件只代表"最后一次正常收尾时的状态"。
正常退出（返回键）才会补写。

---

## 五、证据清单

| 文件 | 内容 |
|---|---|
| `d12e-nas-health.ndjson` | 真实 REMUX 43 点（5.5→218.8 s） |
| `d12e-nas-sf.txt` | 真实 REMUX 18 轮 SF 原始数据 |
| `d12e-nas.json` | 单文件指标（force-stop 收尾，仅前 2.3 s，见 §四） |
| `d12e-4k-health.ndjson` | 4K 合成片 37 点 |
| `d12e-4k-sf.txt` | 4K 合成片 18 轮 SF |
| `d12e-4k180.mp4` / `d12e-1080p20m.mp4` | 合成对照片（未入库，`.gitignore:64`） |

复算：`python tools/p345_soak_report.py <health.ndjson> <sf.txt>`

---

## 六、未覆盖

1. 1080p 合成片（20 Mbps 目标）**没跑** —— `-b:v 20M` 对 testsrc 无效（实际只 1.3 Mbps），
   等价于又一个低码率样本，跑了也不增加信息，直接改用真实 REMUX。
2. 4K **真实** REMUX 未测（`12T/REMUX` 里只见 1080p AVC / VC-1；4K 可能在 `电影` 或其他目录）。
3. 音轨是 DTS-HD MA 5.1，本次只验证「不出错」，未验证多声道下混质量。
4. `mistimed` 的换算公式仍未建立（三档数据只能证明"与内容无关"，不能换算成毫秒）。
