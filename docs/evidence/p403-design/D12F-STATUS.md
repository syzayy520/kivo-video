# D12-F 状态：非硬解编码的软解能力验证 + 一个 SMB 大文件缺陷

日期：2026-11-18
上游：`D12E-STATUS.md` §三「VC-1 样片可解 D12-B3 阻塞」
目的：在动手做 D12-B3（legacy 软解接线）**之前**，先确认默认路径到底缺不缺这个能力

---

## 一、结论先行

| 路径 | 平台无硬解的编码（WMV8 / VC-1 类） | 影响面 |
|---|---|---|
| **默认 mpv（gpu-next）** | ✅ **软解正常**：`hwdec=no`、25 fps、`mistimed=0`、`vo_drop=0`、画面正常 | 99% 用户 |
| legacy 回退（`debug.kivo.engine=legacy` 或开「后台继续播放音频」） | ❌ 明确失败 + 可读原因（D12-B 阶段 1 已做） | 极窄 |

⇒ **D12-B3 从"P1 阻塞"降为 P2**：它只影响 legacy 回退路径，
而默认路径的软解是好的。真正会踩到的场景是「开了后台播放 + 播 VC-1/WMV」。

---

## 二、怎么验的（以及一个诚实的替代）

**ffmpeg 没有 VC-1 编码器**（只有 `wmv1` / `wmv2`），拿不到"真 VC-1"测试片。
但 `wmv2`（WMV8）与 VC-1 在这一点上等价：**都不在平台硬解白名单**，
都走同一条 ffmpeg 软解通路。

生成 30 s 测试片：

```bash
ffmpeg -y -f lavfi -i "testsrc=duration=30:size=1280x720:rate=25" \
       -c:v wmv2 -b:v 4M -an d12f-wmv2.wmv
```

真机（K40）内核日志：

```
[cplayer] ● Video  --vid=1  (wmv2 1280x720 25 fps)
[vd] Codec list:
[vd]     wmv2 - Windows Media Video 8
[vd] Opening decoder wmv2
[vd] Not trying to use hardware decoding: codec wmv2 is not on whitelist.
```

`mpv_health`（起播 25.8 s 时）：

```
hwdec_current = "no"      ← 确认软解
estimated_vf_fps = 25.0
mistimed_frame_count = 0
vo_drop_count = 0
```

画面：标准彩条测试图完整显示，无花屏/黑屏。

**证据边界**：这证明"mpv 软解通路对非硬解编码是通的"，
**不等于**"真 VC-1 一定能播"（VC-1 解码器本身可能有别的问题）。要严格证明仍需真 VC-1 片源。

---

## 三、顺带发现的真实缺陷：33.8 GB `.ts` 无法起播

`12T/REMUX/A Beautiful Mind 2001 BluRay REMUX 1080p VC-1 DTS-HD MA5.1 2Audio-CHD.ts`（33.8 GB）

点播放后黑屏、进度 `00:00 / 00:00` 持续 40 s 以上。日志：

```
Set property: hwdec="mediacodec" -> 1
Run command: loadfile, ... options="start=3574.582"      ← 从 59:34 续播
PROXY REQ range=bytes=0- cap=0
PROXY conn fail: ConnectionResetException: Connection reset
[ffmpeg/demuxer] matroska,webm: EBML header parsing failed
[lavf] avformat_open_input() failed
[cplayer] Failed to recognize file format.
```

**关键判读**：

- `EBML header parsing failed` 本身是探测器的正常输出（ffmpeg 会依次试），
  但后面紧跟 **`Failed to recognize file format`** ⇒ 最终没识别成任何格式；
- 同时 `PROXY conn fail: Connection reset` ⇒ **读到的数据本身就是断的**；
- `start=3574.582` ⇒ 该文件有续播点（此前播过），本次要 seek 到 59:34。

⇒ **这不是 VC-1 的问题，也大概率不是 `.ts` 的问题**：它死在 demuxer 之前，
根因落在「SMB 大文件 + 远 seek + 本地代理」这个组合上。

**未定型**（三个候选，本批没有足够证据区分）：
1. 代理层对 `bytes=0-` 的 range 请求在"要先 seek 到 3574 s"时的处理；
2. SMB 服务端对 33.8 GB 文件的远偏移读取限制/超时；
3. `.ts` 无索引（无 cues），必须扫全文件才能 seek ⇒ 33.8 GB 扫不动。

**建议单独开卡**：`SMB 大文件(.ts) 续播起播失败`。判据至少要有：
`wireshark/代理日志里 range 请求的实际偏移` + `同文件从头播是否成功`。

---

## 四、对 D12-B3 的裁决

原计划"补 legacy 软解完整接线（起播/暂停seek/进度/收尾四个控制点）"**本批不做**，理由：

1. 默认路径软解已正常（§二），缺口只在 legacy 回退；
2. legacy 回退现在给的是**明确失败 + 可读原因**，不是黑屏或崩溃 —— 静默降级已被消除；
3. 完整接线要动 4 个控制点，风险不小，而收益人群是「开后台播放 + 播非硬解编码」的交集，极窄。

⇒ 记为 **P2**，等有实际用户反馈或 legacy 内核下线计划时再做。
若要做，前提是先把 §三 的 SMB 缺陷解决（否则连"从 0 播 VC-1"都验不了）。

---

## 五、证据清单

| 文件 | 内容 |
|---|---|
| `d12f-vc1-kernel.log` | `.ts` 失败时的完整内核日志（85 行） |
| `d12f-vc1-netdiag.log` | 同次 `net-diag`（749 行，含 `PROXY conn fail`） |
| `d12f-sw.png` | WMV8 软解出画截图（彩条测试图） |
| `d12f-wmv2.wmv` | 软解测试片（未入库，`.gitignore:64` 忽略 `docs/evidence/**/*.mp4`；`.wmv` 也按同类处理，生成命令见 §二） |

---

## 六、未覆盖

1. **真 VC-1 未测**（ffmpeg 无 VC-1 编码器）。§三 的 `.ts` 恰好是 VC-1，但它死在 demuxer，
   没进解码器，无法反证解码能力。
2. §三 的根因三个候选**都未验证**，本批只做到"定位到 demuxer 之前 + 连接被重置"。
3. 软解性能未测（WMV8 720p 30 s 太短，`vo_drop=0` 只说明没崩，不说明软解性能足够播 1080p VC-1）。
