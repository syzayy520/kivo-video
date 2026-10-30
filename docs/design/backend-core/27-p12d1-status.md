# P12-D1 · Surface 重建 · 验收与收口

- 版本：V1.0（结题）
- 日期：2026-10-29
- 上游：`26-p12d1-design.md`（设计，§3.2 已被本文 §三 的实测修订）
- 下游：D2 / D3 / D4

---

## 一句话结论

**切后台再回来，画面 2–29 ms 就回来了，一帧都没等。**

改之前不是"恢复得慢"，是**根本没有恢复** —— `onSurfaceLost()` 直接销毁整个会话，
回来时从第一帧重新打开、重新解码。C 组收口时那套"能播 180 帧"的东西，
用户按一次 Home 就全丢了。

三档空窗（0.47 s / 1.02 s / 2.02 s）实测：`resume_ms` = 29 / 15 / 2 ms，
`frames_lead` 恒为 0，0 丢帧、0 崩溃、播放速率 0.99998 不动。

---

## 一、判据（照卡片，未放宽）

卡片：**重建后 3 帧内恢复，不崩、不黑屏超 500ms。**

| 量 | 定义 | 判据 | 本次 |
| --- | --- | --- | --- |
| `resume_ms` | `rebind` 调用 → 重建后第一帧真正交出 | ≤ 500 ms | **2 / 15 / 29 ms** ✅ |
| `frames_lead` | `rebind` 后到首帧之间交出去的帧数 | ≤ 3 | **0 / 0 / 0** ✅ |
| 不崩 | 进程存活、`completed=true` | 硬性 | ✅ |
| 帧数闭合 | `rendered + dropped + discarded + 1 == received` | 硬性 | ✅ 181 = 181 |

`gap_ms`（面消失到面回来）单独记，**那是用户的时间**，不算我们的账。
三档分别是 470 / 1019 / 2020 ms —— 它只影响"空窗期丢了多少帧"，不影响恢复速度。

---

## 二、主证据（K40 `b6e950ee`，`h264_1080p8.mp4`，`cpu_paced`）

| 轮 | `gap_ms` | `resume_ms` | `frames_lead` | 上屏帧 | 空窗丢弃 | 节拍丢帧 | 收帧 | 播放速率 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| control（不切后台） | — | — | 0 | 180 | 0 | 0 | 181 | 0.999977 |
| r1 | 470 | **29** | 0 | 165 | 15 | 0 | 181 | 0.999979 |
| r2 | 1019 | **15** | 0 | 149 | 31 | 0 | 181 | 0.999979 |
| r3 | 2020 | **2** | 0 | 120 | 60 | 0 | 181 | 0.999978 |

四条自洽性检查，任何一条对不上都说明计数在骗人：

1. **空窗丢弃帧数 = 空窗时长 × 30fps**：470 ms→15、1019 ms→31、2020 ms→60。
   误差 ≤ 1 帧。这条最能说明"丢的帧确实是空窗期丢的"，不是别处漏的。
2. **`frames_dropped_late` 全程 0**：换面没有让节拍器判任何一帧"太晚"。
   这是本次最关键的一条 —— 它证明**时间轴没有被重建打断**。
3. **播放速率不动**：0.999977（对照）vs 0.999978–0.999979（换面）。
   切一次后台不会让片子变慢或变快。
4. **帧数闭合**：`rendered + discarded + 1 == received`，四轮全部成立。

---

## 三、真机踩的两个坑（都改了，都写进设计稿）

### 3.1 `setOutputSurface(nullptr)` 之后再切回 Surface，直接失败

设计稿第一版的做法是"摘面 = 切到 ByteBuffer 模式"，理由是 NDK 文档支持。
K40 实测：

```
rebind surface: surface: setOutputSurface failed (status=-10000)
```

`-10000` 是 `AMEDIA_ERROR_UNKNOWN`。那颗 `OMX.qcom.video.decoder.avc` **切过去就切不回来**，
会话只能作废 —— 也就是"切后台"变成了"结束播放"。

修正：**摘面改成本地行为**（置一个原子标志，收帧段一律 `render=false`），
平台侧只在 window → window 之间直换。那才是 ExoPlayer 换面走的同一条路。

> 这次失败之所以值钱：它是**精确可复现**的，而且失败被如实记进了
> `p12b-failure.json`（`stage=rebind`、`return_code=13`、带原始 status）。
> 没有这条通道，这个 bug 会表现成"切后台偶尔黑屏"，然后被归因到别处。

### 3.2 摘面时不能放掉 `ANativeWindow*`

第一版在摘面分支里顺手 `window.reset()` 了。但那一瞬间 codec 还指着这个窗口，
`release` 完就是悬空指针。

修正：旧窗口**留到新面挂上之后**再放。现在的顺序是
"先 `set_output_surface(new)` 成功 → 再替换 binder"，中间任何一步失败都保持摘面状态。

### 3.3 计数：空窗丢帧不能算进"节拍丢帧"

第一版把空窗期强制 `render=false` 的帧也算进了 `frames_dropped_late`，
于是 500 ms 的切后台凭空多出 15 个丢帧，读起来像"换面之后节拍崩了"。

修正：新增 `frames_discarded_no_surface` 单独记账，并把闭合关系改成四项。
`tools/verify_p12c_sync.py` 的闭合检查同步更新（缺这一项时按 0 处理，旧证据照样能过）。

---

## 四、判据里那句"3 帧内恢复"是怎么落实的

不是靠"感觉很快"，是靠两个数：

- `frames_lead`：rebind 之后、首帧交出之前，一共交出去了几帧（都是 `render=false`）。
  实测 0 —— **第一帧就直接上屏了**，没有"先丢几帧暖机"这回事。
- `resume_ms`：从 `rebind` 调用到那一帧交出。2–29 ms，比一帧时长（33 ms）还短。

为什么能这么快：换面**没有重开会话**。帧计数、节拍锚点、时钟全部连续，
所以不存在"重新建锚 → 前几帧判迟到 → 连续丢帧 → 触发追帧"这条链。
`DecodeSession::start()` 里有 `counters_.reset()` 和 `pacer_->reset()`，
走一趟 stop/start 就会把这条链完整触发一遍 —— 这就是当初决定热换面的原因。

---

## 五、这条改动动了哪些既有行为

| 改动 | 影响面 | 是否影响已收口的结论 |
| --- | --- | --- |
| `DecodeStats` 新增 2 个字段 | 证据 schema `/5` → `/6` | 只加字段，C 组证据仍可读 |
| 帧数闭合关系由三项变四项 | `tools/verify_p12c_sync.py` | 已同步，C 组五份复跑仍 OK |
| `release_frame*` 加锁 | 每帧一次无竞争 lock/unlock | 不影响：C 组 `av_drift` p99 仍是 0.125 ms 量级 |
| `PlayerActivity` 不再在 `onSurfaceLost` 销毁会话 | 切后台/回前台 | 这是本次要改的 |

桌面回归：`ctest -R kivo_p12c` **5/5**，26 例 / 109 断言不变
（D1 改的是 Android 侧会话与 UI，不碰节拍器与音频核心）。

---

## 六、这次没做到的（诚实清单）

1. **只测了"切后台再回来"这一种触发。** 卡片写的还有"旋转"，但本 App 是
   `screenOrientation="landscape"` + `configChanges` 全包，转屏根本不会重建 Surface ——
   也就是说**旋转这条路径在本工程里不存在**，不是没测，是没有可测的东西。
   真要做旋转，得先让 Activity 允许竖屏，那是产品决策不是本卡范围。
2. **没测多窗口 / 分屏 / 画中画下的 Surface 尺寸变化。**
   `surfaceChanged` 目前被刻意忽略（`PlayerSurfaceView` 里写着"避免重复初始化"）。
   尺寸变化会不会导致 `setOutputSurface` 需要重配，没验。
3. **空窗期靠丢帧度过，没有"暂停解码"这条路径。**
   2 秒空窗丢 60 帧，如果用户切走 5 分钟，会白解 9000 帧。
   省电语义要等 D4（生命周期收敛）。
4. **音频在空窗期照常出声。** 切到后台音乐会继续放 —— 这是 D1 范围外的行为，
   但真机上确实如此，写在这里免得被当成"已经处理了"。
5. **只验了 K40 一颗 AVC decoder。** HEVC 4K 没跑换面；
   4K 上 `setOutputSurface` 的代价可能不是几十毫秒量级。
6. **`resume_ms` 的分辨率是毫秒**，而且从 `rebind` 调用算起，不含 JNI 之前
   `ANativeWindow_fromSurface` 的开销（那一小段在 Kotlin 侧，没记）。

---

## 七、证据清单

`docs/evidence/p12d/`（K40 `b6e950ee`，schema `kivo.p12c.decode/6` + `kivo.p12c.sync/1`）：

| 文件 | 内容 |
| --- | --- |
| `p12d1-control-r1{,-sync}.json` | 对照：不切后台，`rebuilds=0`，字段保持 −1 |
| `p12d1-rebuild-r1{,-sync}.json` | 空窗 470 ms，`resume_ms=29` |
| `p12d1-rebuild-r2{,-sync}.json` | 空窗 1019 ms，`resume_ms=15` |
| `p12d1-rebuild-r3{,-sync}.json` | 空窗 2020 ms，`resume_ms=2` |

**复跑**：

```bash
M=/sdcard/Android/data/com.kivo.cinema/files/media
adb shell "am start -n com.kivo.cinema/.ui.PlayerActivity \
  --es kivo.media $M/h264_1080p8.mp4 \
  --ei kivo.pace 1 --ei kivo.drop 1 --ei kivo.fps 30 \
  --es kivo.present cpu_paced --es kivo.evidence p12d1-rebuild-r1.json"
sleep 2
adb shell input keyevent 3                     # HOME：面销毁
sleep 1
adb shell am start -n com.kivo.cinema/.ui.PlayerActivity   # 回来：换面
```

**复算**：

```bash
python tools/verify_p12c_sync.py docs/evidence/p12d/p12d1-*-sync.json
```

四份全过（含 `frames_discarded_no_surface` 参与的帧数闭合检查）。
