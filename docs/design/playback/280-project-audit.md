# 280 · 接管审查（2026-11-15 06:3x）—— 全项目扫描与问题清单

> 触发：用户「你看完不能完美接管是吗？还需要了解什么？不行的话你重新审查下整个项目，
> 顺便看看还有哪些问题一并列出来优化」
>
> 本文件是**实地扫描结果**，每条都有代码位置或命令输出作证据；不是推测，不是旧结论搬运。
> 标注：`[已实证]` = 我跑命令/读代码确认；`[待裁决]` = 需要用户决定方向。

## 一、接管结论

**能接管干活**（编译通过、git 干净、设计文档与工作记忆齐备），**但"完美"不成立**，
原因有两类：**① 项目存在 4 个真实缺陷（其中 1 个直接阻碍"对齐 SenPlayer"的目标）；
② 有 3 个方向需要用户裁决，我不该替你拍。**

## 二、缺陷清单（按严重度）

### P0-1 本地文件播放**完全没吃到** gpu-next / HDR 的成果 `[已实证]`

**证据链**：
```
MainActivity.kt:83            .putExtra("kivo.media", item.path)
MediaLibraryActivity.kt:151   .putExtra("kivo.media", item.path)
LocalResultCard.kt:45         .putExtra("kivo.media", item.path)
DetailContent.kt:45           .putExtra("kivo.media", it.item.path)
PrivateActivity.kt:98         .putExtra("kivo.media", item.path)
        ↓ 全部汇入
PlayerSessionStart.kt:46      val localSession = PlaybackSession(0, media, media.name)   ← 自研旧内核
```

对比：**只有 Emby/URL 与 SMB 走 mpv**
```
PlayerUrlStart.kt:95          if (engine == MPV) startMpvPlayback(...)
PlayerSmbStart.kt:151         MpvPlaybackSession(this, source)
```

**后果**：本地文件播放没有 gpu-next、没有完整 libplacebo、没有 HDR 峰值计算、
没有 aimagereader 零拷贝、没有 3 音轨/5 字幕轨的控制面。
⇒ **"最终形态对齐并超过 SenPlayer"这个目标，在本地播放场景一条都没落地。**
而 HiFier（音频平面对标对象）主打的就是本地高保真播放 —— 这一块恰好是空白。

**好消息：技术上现成，改动很小** `[已实证]`
`startMpvPlayback(holder, source: ByteSource, gate)` 吃的是 `ByteSource`，
而 `net/FileByteSource.kt` **已存在**且实现同一接口：
```kotlin
class FileByteSource(path: String) : ByteSource
```
⇒ 只需在 `maybeStartPlayback` 的本地分支按 `PlaybackEngine.select()` 分流，
MPV 时走 `startMpvPlayback(holder, FileByteSource(media.absolutePath))`，legacy 保留回退。
预计新增/改动 **≤ 40 行**（新建一个分流文件 + 原分支加两行判定）。

### P0-2 libmpv 构建链断裂 —— 整个成果**不可复现** `[已实证]`

- `cpp/CMakeLists.txt:481` 注释写：「交叉编译脚本见
  `android/app/buildscripts/mpv/build-libmpv.sh`」。
- 实际 `buildscripts/mpv/` 只有三个文件：`patch-mpv-es3.py`、`patch-mpv-hdr-surface.py`、
  `stage-libmpv.sh`。**`build-libmpv.sh` 不存在**（`find . -name build-libmpv.sh` 返回空）。
- `stage-libmpv.sh` 里写死本机绝对路径 `/c/re-tools/mpv-build/mpv/build/libmpv.so`，
  且第一句就要求「先跑 build-libmpv.sh」。
- `libmpv.so` 入 `.gitignore`（33 MB），仓库里没有二进制。

⇒ **换一台机器、或新人 clone 下来，无法重建 libmpv**：mpv 源码版本、meson 配置参数、
NDK 交叉编译开关、ES3 补丁与 HDR 补丁的应用顺序，**全都没有脚本记录**。
这意味着我这轮做的 gpu-next / HDR 全部成果，**只在这台机器上存在**。

### P1-1 HDR 开关存在**两份独立决策**，取值会打架 `[已实证]`

| 位置 | 输入 | 结果去向 |
| --- | --- | --- |
| `PlayerComposeActivity.kt:262` | `intent.getStringExtra("kivo.renderMode")` | 决定 Surface 是 10bit 还是 8bit |
| `MpvStartSequence.kt:110` | `intentRenderMode()`（**还兼容 `kivo.gpuNext`**） | 决定 mpv 要不要 `egl-output-format=rgb10_a2` |

**冲突场景**：`--ez kivo.gpuNext true` + 设置项 `renderMode=legacy`
- Activity 侧：resolve → LEGACY ⇒ `wantHdr=false` ⇒ Surface 留在 **8bit**
- 内核侧：`intentRenderMode()` 返回 GPU_NEXT ⇒ `hdrSurface=true` ⇒ 请求 **10bit EGL config**

⇒ 8bit Surface + mpv 要 10bit config。本机被 `Hdr10BitProbe` 闸门挡住所以没炸，
**在支持 10bit 的设备上可能出现挂死**（正是本机踩过的 `setBuffersGeometry` 卡死形态）。

**修法**：只留**一个**决策点，把 `hdrSurface` 与 `renderMode` 一起算好传下去，
两处都不许各算一遍。（这也直接命中"单文件单职责 / 单一决策入口"。）

### P1-2 B5 守卫是**注释**，不是检查 `[已实证]`

`cpp/CMakeLists.txt:477` 只有一行注释 `# B5 每个 .cpp ≤ 300 行`，
其后没有任何 `foreach` / `string(LENGTH)` 之类的校验逻辑 ⇒ **违反不报错**。
Kotlin 侧更彻底：**完全没有守卫**，当前 **17 个文件超 300 行**：

| 行数 | 文件 |
| --- | --- |
| **901** | `ui/PlayerActivity.kt` |
| 403 | `bridge/FfmpegPlaybackSession.kt` |
| 402 | `net/OriginPrefetchKit.kt` |
| 398 | `ui/design/DiscoverPage.kt` |
| 395 | `ui/compose/PlayerActionRouter.kt` |
| 387 | `playback/NetworkPlaybackSession.kt` |
| 379 | `net/HttpOriginSource.kt` |
| 365 | `bridge/PlaybackSession.kt` |
| 352 | `ui/compose/PlayerComposeActivity.kt` |
| 343 | `mpv/MpvThumbnailReader.kt` · `playback/PlaybackPrefs.kt` |
| 341 | `net/HttpOriginClient.kt` |
| 334 | `library/MediaLibraryActivity.kt` |
| 320/319/309/307 | `NativePlayer` / `MpvPlaybackSession` / `EmbyResource` / `OriginWindowCache` |

（C++ 侧目前**全部合规**，最大 279 行 —— 这部分没问题。）

### P1-3 mpv 缺失时**静默跳过**，编译照样成功 `[已实证]`

```cmake
if(EXISTS "${KIVO_MPV_PREBUILT}/libmpv.so")
    add_library(kivo_mpv SHARED ...)
```
注释明说「缺失时这个目标整体跳过 —— 旧内核照常构建」。
⇒ `BUILD SUCCESSFUL` 但**新内核根本没编进去**，运行期才在 `PlaybackEngine` 那层
掉回 legacy。**违反铁律「不许静默降级」**，而且极易误判成"gpu-next 没生效"。
应在缺失时给 `message(WARNING)` 或在产物里留标记。

### P2-1 死代码：3 个 Activity **0 处引用**，但都在 Manifest 注册 `[已实证]`

| Activity | 行数 | 引用数 |
| --- | --- | --- |
| `ui/PlayerActivity` | **901** | **0** |
| `ui/MultiPlayerActivity` | 290 | 0 |
| `ui/FramerateDumpActivity` | — | 0 |

`PlayerActivity` 是 **AppCompat + 手写 View 的旧播放页**（`import androidx.appcompat.app.AppCompatActivity`），
而线上用的是 `PlayerComposeActivity`（Compose）。⇒ 旧 UI 残留，按铁律应删。
（后两个是取证探针，可保留但要注明"只能 adb 显式启动"，避免下一个人以为是死的。）

### P2-2 **5 套播放会话**并存 `[已实证]`

| 会话 | 行数 | 谁在用 |
| --- | --- | --- |
| `bridge/PlaybackSession` | 365 | 本地文件、`NetworkPlaybackSession` |
| `bridge/FfmpegPlaybackSession` | 403 | SMB 软解、`PlayerActivity` |
| `bridge/NativePlayer` | 320 | 底层 JNI |
| `playback/NetworkPlaybackSession` | 387 | Emby 的 legacy 分支 |
| `playback/MpvPlaybackSession` | 319 | Emby / SMB 的 mpv 分支 |

铁律第 5 条「新代码落地必须彻底移除被替代的旧代码」在这里是明确违反的（属历史包袱，
不是我这轮引入的）。但**只要本地文件还走旧的，这 5 套就都得留着** ⇒ P0-1 不解决，这个也解不掉。

### P2-3 网络层 **5311 行 / 35 文件 / 0 单测** `[已实证]`

`net` 包是 274 战役**所有 bug 的发源地**（超买 8 倍、carry 饿死、统计口径错），
而现有 6 个单测全部在 `smb` 包，`net` 与 `playback` 一个都没有：
```
android/app/src/test/java/com/kivo/cinema/smb/*.kt   ← 6 个，仅此
```
`Origin*` 前缀文件 15 个（Window / WindowCache / WindowDownloader / PrefetchPump /
PrefetchBuffer / PrefetchKit / ConnectionLease / HttpConnection / LatencyStats / WarmupPool …），
划分是否过度拆分**无法从命名判断**，需要一次依赖图梳理。

### P3-1 两个内核选择器语义重叠 `[已实证]`

- `PlaybackEngine.select()` ← `debug.kivo.engine`：**mpv vs legacy**（选哪条链路）
- `PlaybackRenderMode.resolve()` ← `debug.kivo.renderMode`：**gpu-next vs render API**（链路内选哪条出画）

命名接近（`Engine` / `RenderMode`）、都是"debug 属性 + intent + 默认"三层结构，
**但没有统一入口**，非常容易像 P1-1 那样各算一遍。建议合并成一个
`PlaybackStack.resolve()` 返回 `(engine, renderMode, hdr)` 三件套。

### P3-2 交接文档过期 `[已实证]`

`docs/handoff/CURRENT.md` / `TASKS.md` 的当前指针停在 **253-P6-2 字幕上屏**，
实际已推进到 278/279。下一个人照它接手会去修一个**已经不是缺口**的问题。

### P3-3 文档编号堆积

`docs/design/backend-core/` 下 100+ 编号文档无索引；`playback/` 下 273–279 尚可读。
需要一个 `docs/design/INDEX.md`（一页纸：哪个编号是哪条线的现状）。

## 三、建议的执行顺序（按"收益 ÷ 风险"排）

| 顺序 | 事项 | 理由 | 预估 |
| --- | --- | --- | --- |
| **1** | 补 `build-libmpv.sh` | 一切成果的**底座**。不补，换机即失。工作量小 | 小 |
| **2** | 修 HDR 双份决策（P1-1） | 防挂死；顺带把 `PlaybackStack` 统一入口建起来（P3-1 一起解） | 小 |
| **3** | **本地文件迁 mpv**（P0-1） | **"对齐 SenPlayer"真正的空白**；技术上现成（`FileByteSource` 已有） | 中 |
| **4** | 删死代码 `PlayerActivity` + 加 300 行守卫（P1-2/P2-1） | 铁律合规；守卫防再退化 | 中 |
| **5** | 补 `net` 层单测（P2-3） | 274 战役的教训：这层没测就是靠真机长稳兜 | 大 |
| 6 | 更新 `CURRENT.md` / `TASKS.md` + 建 INDEX | 防止下一个接手者走错路 | 小 |
| — | 卡顿两臂复验 | 独立于以上，需**下午时段** + 同源 senplayer 对照 | 待时段 |

## 四、需要用户裁决的三个点 `[待裁决]`

1. **本地文件要不要迁到 mpv？**（P0-1）
   迁 = 本地播放一次性拿到 gpu-next/HDR/零拷贝/多轨，且能顺势删旧内核；
   不迁 = 保留旧内核在本地场景的稳定性，但"对齐 SenPlayer"永远有个大洞。
   **我建议迁，且先在设置项里做成可选（默认新），留 legacy 一键回退。**

2. **旧内核最终去留。** 若迁完本地文件，`PlaybackSession` / `FfmpegPlaybackSession` /
   `NetworkPlaybackSession` 这条链路是否整体删除？（删了 P2-2 一并解决；
   留则永久承担双份维护成本。）

3. **下一步优先级**：构建可复现性（1）vs 本地迁移（3）—— 两者都能立刻开工，
   我按上表顺序做，除非你要调整。
