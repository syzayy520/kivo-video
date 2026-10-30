# P12-F3 验收：错误路径全覆盖 —— **3/4 达成，加密轨一条 BLOCKED**

- 版本：V1.0
- 日期：2026-10-30
> ⚠️ **本卡的「加密轨 BLOCKED」已由 `51-p12f3b-design.md` / `52-p12f3b-status.md` 解开**：
> 平台一直在报，报在轨道格式的 `crypto-mode` 上（不是 sample flag）。改判后 4/4 达成。
> 本文那句「平台没告诉我们它加密了」**是错的**，保留原文以记录这次误判。
- 设计：`49-p12f3-design.md`
- 卡片判据：**坏文件 / 加密轨 / 无音轨 / 无视频轨，均给非空 reason**
- 结论：**三条达成，一条不可验证。** 校验脚本 `tools/verify_p12f3_errors.py` 输出
  3 PASS + 1 NOT_VERIFIED、**退出码 1** —— 没有把"三条过"粉饰成"四条过"。

---

## 一、四条用例的结果

| 用例 | 结果 | 证据 | reason（原文摘录） |
| --- | --- | --- | --- |
| `bad-file` | ✅ PASS | `p12f3-bad-file-failure.json` | `container reports 0 tracks — the file is likely truncated or corrupt, not merely missing a video track` |
| `no-video-track` | ✅ PASS | `p12f3-no-video-track-failure.json` | `no video track among 1 tracks` |
| `no-audio-track` | ✅ PASS | `p12f3-no-audio-track-sync.json` | `source has no audio track; there is no audio timeline to compare against`（`clock_source = system`） |
| `encrypted` | ❌ **NOT_VERIFIED** | `p12f3-encrypted-flags.json` | 设备端把 CENC 样本读成明文（`is_encrypted = false`）⇒ 拒绝分支不可达 |

---

## 二、改了一处代码：截断文件与"没有视频轨"必须说成两句不同的话

第一轮 `bad-file` 的 reason 是：

```
no video track among 0 tracks
```

这句话**技术上没错，但会误导人**：读的人会以为"这文件只有音频"，而事实是
"这根本不是一份能用的容器"（`ffprobe` 对同一文件报 `Invalid data found when processing input`）。
两者的处理方式完全不同 —— 前者换片源就行，后者要怀疑文件损坏/传输截断。

改成：

```cpp
if (track_count == 0) {
    return {kNoVideoTrack, label + ": container reports 0 tracks — the file is likely "
                                 "truncated or corrupt, not merely missing a video track"};
}
```

**这就是这张卡真正要的东西**：不是"有 reason"，是"reason 指向正确的故障类型"。

---

## 三、加密那条：为什么是"不可验证"而不是"通过"

### 3.1 先说结论

用仓库里的 FFmpeg 造了一份真 CENC 片源（`encv` / `sinf` / `senc` / `tenc` 四个盒子都在），
设备端读到的第一包是：

```json
"first_sample": { "read_ok": true, "is_encrypted": false, "is_sync": true, "size": 40681 }
```

`is_encrypted = false` ⇒ `MediaCodecVideoDecoder::submit()` 里那条
`if (sample.is_encrypted) return fail("encrypted sample is not supported yet")` **永远不会触发**。

### 3.2 更糟的是：它不会报错，它会"播出一堆垃圾"

同一份片源走完整播放链：

| 字段 | 值 |
| --- | --- |
| `samples_submitted` | 92 |
| `samples_rejected` | **0** |
| `frames_received` | 25 |
| `frames_rendered` | 23 |
| `outcome.completed` | **true** |
| `outcome.error` | **空** |

3 秒的片源应该出 ~90 帧，实际只出 25 帧、而且那 23 帧是解错的像素，**却一路 `completed=true`**。
这是 **G9（不许静默降级）的一个真实盲点**：不是我们写了"静默"，而是**平台没告诉我们它加密了**。

### 3.3 为什么不能"顺手加个检测"

- 读 `AMediaFormat` 的 `crypto-key`：本机没有这个键（探针没有输出，等于没有）。
- 自己解析 `stsd` 找 `encv`：那是容器解析，设计 §3.5 已列为不做，而且手写 box 遍历
  极易在偏移上出错 —— 错一次就是"把正常文件判成加密"，比漏判更糟。
- 用"帧数明显偏少"当启发式：VFR、坏片、结尾截断都会触发，误报率高，**不能当加密判据**。

所以本卡**不加**任何启发式检测，只把事实留下。

### 3.4 解卡条件（写进台账）

需要一个**能让 Android 的 `MediaExtractor` 真的把样本标成加密**的样本。按经验，
当前这份缺少 `pssh` 盒（`d.count(b'pssh') == 0`），而 `MediaExtractor` 往往要
`pssh` + `tenc` 一起才认。可行的下一步：

1. 用 **mp4box / shaka-packager** 生成带 `pssh` 的 CENC（或 CMAF/fMP4 片段）；
2. 或用一段真实的 Widevine/ClearKey 加密内容（无 license 也行，只要能读出 `is_encrypted=true`）；
3. 拿到之后**只需重跑一条用例** —— 代码不用改（拒绝分支已经在 `submit()` 里）。

---

## 四、产物

| 类型 | 路径 |
| --- | --- |
| 失败证据（新 schema） | `docs/evidence/p12f3/p12f3-{bad-file,no-video-track}-failure.json`（`kivo.p12f.negative/1`） |
| 无音轨降级证据 | `p12f3-no-audio-track.json` + `-sync.json`（`av_drift.unavailable_reason`） |
| 加密用例证据 | `p12f3-encrypted.json` + `-sync.json` + `p12f3-encrypted-flags.json`（探针的首包标志） |
| 负向片源 | `tests/real/negative_{truncated,audio_only,cenc_h264}.mp4` |
| 校验脚本（**新**） | `tools/verify_p12f3_errors.py` |

代码改动三处，都只影响"说清"这一层：

| 改动 | 位置 | 影响 |
| --- | --- | --- |
| `kivo.case` extra + 按用例命名 + 新 schema | `ui/PlayerActivity.kt` | 四条路径不再互相覆盖 |
| `0 tracks` 单独成句 | `source/mediaextractor_source.cxx` | 截断文件不再被说成"没有视频轨" |
| 探针输出首包标志 | `probe/hdr_probe_main.cxx` | 让"加没加密"从猜测变成观测 |

桌面单测不变：`ctest -R kivo_p12` **9/9**。

---

## 五、诚实清单（五条）

1. **加密那条没验证。** 不是"通过了没写"，是**分支不可达**：设备把这文件当明文。
   证据里 `is_encrypted=false` 是硬证据，任何人都能复算。
2. **花屏/垃圾帧无法自动判定。** 本卡能证明"帧数明显偏少"，不能证明"那 23 帧是垃圾"——
   后者要靠人眼（截图 `artifacts/p12f3/` 未提供，本轮没有把像素当证据）。
3. **只做了一种坏文件形态**（容器头截断）。错码流、坏索引、零字节文件都没测。
4. **"无音轨"用的是 sync 证据的 `unavailable_reason`**，不是失败证据 —— 它是**降级**不是失败，
   这个区别写在这里，避免后来人以为它该出现在 `p12f.negative` 里。
5. **`kivo.case` 是取证专用 extra**，产品路径不该依赖它；不带它时文件名退回 `p12b-failure.json`。

---

## 六、交接 F4（证据总收口）

- F3 的三条通过用例已经可以进 F4 的总表；**加密那条要标 `NOT_VERIFIED`，不许留空**。
- F4 要做的第一件事是**跨阶段核对**：`docs/evidence/p12{a,c,d,e,e3,e4,f1,f2,f2a,f3}/` 每个目录
  至少有一份能复算的产物，且 schema 版本一致（当前最终版：`decode/11`、`sync/1`、`hdr-probe/2`、
  `layer-dataspace/1`、`frame-luma/1`、`p12f.negative/1`）。
- 本卡的校验脚本可以当 F4 的工具模板：**判据与证据一起提交，退码不粉饰**。
