# 253-P6-2 字幕平面：诊断结论与根因定位

状态：**通路已证明可用（文本字幕上屏 ✅）；PGS 位图字幕未上屏，根因已缩小到 mpv 的位图字幕缩放/合成环节**
日期：2026-11-11
设备：K40 `b6e950ee`（Android 13，MIUI / Adreno 650 / GLES 3.2）
片源：NAS `视频/kivo-test/shelter.mkv`（= `Shelter 2026 USA BluRay 1080p TrueHD5.1 x265 10bit-CHD.mkv`，12.19 GB）

> 本节取代 `docs/design/backend-core/253-mpv-kernel-p6-control-plane.md` §四 里关于根因的猜测。
> 那几处猜测已被本节的实测数据**证伪**，不要再照着做。

---

## 零、结论速览

| 问题 | 状态 |
| --- | --- |
| 字幕轨道能枚举/能选中 | ✅ 已证明（5 条 PGS 中文轨，`sid` 写进去生效） |
| **字幕渲染通路（OSD 合成 + 上屏）是否可用** | ✅ **已证明可用** —— 外挂 SRT 文本字幕清晰上屏（见 §三 的截图证据） |
| 字体匹配 | ✅ 已证明可用（`fontselect: (Noto Sans CJK JP, 400, 0) -> NotoSansCJKjp-Regular`） |
| **PGS 位图字幕为何不上屏** | ⚠️ 未闭环，但已缩小到 mpv 内部的**位图字幕缩放**环节（见 §五） |
| 旧交接文档 §四.2 第 2 步（`--osd-dimensions=video`） | ❌ **错的，不要执行**（见 §二） |

---

## 一、三个必须先知道的 mpv 事实

### 1.1 `--osd-dimensions` 不是选项，是**只读属性**

| 断言 | 实测（mpv v0.39.0 源码树 `C:/re-tools/mpv-build/mpv`） |
| --- | --- |
| 它是 mpv 的**选项** | ❌ 全树只有 `player/command.c:4119` 一处：`{"osd-dimensions", mp_property_osd_dim}` |
| 可以写 | ❌ 该属性只有 `m_property_read_sub`，**没有 SET 分支** |
| 默认值是 `window` | ❌ `DOCS/man/options.rst` 里 **0 处**提到它；只有 `input.rst`（属性文档）有 8 处 |

**为什么危险**：mpv 的 `player/client.c` 里 `mpv_set_option` 对未知选项返回
`MPV_ERROR_OPTION_NOT_FOUND`，**且不打印任何日志**。写它会**静默失败**。

⇒ 旧文档说"加一条 `--osd-dimensions=video`，一次重编就能判定根因"——
这会得到一个**假阴性**（什么都没变），从而把排查引向更贵的死路。

### 1.2 `osd-dimensions` 属性才是真正的几何读数

它报告 OSD 的目标尺寸与到视频的边距：

```
osd-dimensions/w, /h      OSD 目标尺寸（渲染单位）
osd-dimensions/par        像素宽高比
osd-dimensions/aspect     显示宽高比
osd-dimensions/mt,mb,ml,mr   OSD 到视频的边距（上下左右）
```

真机实测（`Window size: 2320x956`）：

```
OSD borders: l=19 t=0 r=19 b=0      ← 与 Video borders 完全相同
Video display: (0, 0) 1920x804 -> (19, 0) 2282x956
```

⇒ **OSD 跟着视频矩形走**（左右各 19px 是 pillarbox，上下为 0）。
⇒ **"字幕被画进窗口底部黑边"这个假设不成立** —— 上下根本没有黑边。
   旧文档 §四.2 第 2 步据此推出的整条推理链**全部作废**。

### 1.3 mpv-android（SenPlayer 同源）怎么配字幕

`C:/re-tools/mpv-build/mpv-android/app/src/main/java/is/xyz/mpv/Utils.kt:60`：

```kotlin
/** Write the 'fonts.conf' for fontconfig. */
private fun writeFontsConf(context: Context, configFile: File) {
    ... "<dir>/system/fonts/</dir>" ... "<cachedir>${context.cacheDir.path}</cachedir>"
    ... "<alias><family>sans-serif</family><prefer><family>Roboto</family>..."
}
...
fun copyAssets(context: Context) {
    ...
    // we used to ship this, but it's no longer needed
    File("$configDir/subfont.ttf").delete()
    writeFontsConf(context, File("$configDir/fonts.conf"))
}
```

两点要注意：
1. mpv-android **依赖 fontconfig**（libass 带 `CONFIG_FONTCONFIG`），
   靠 `fonts.conf` 指向 `/system/fonts`。
   **我们的 libass 是 `#undef CONFIG_FONTCONFIG`**（`libass-android/build/libass-0.16.0/config.h`）
   ⇒ 没有 fontconfig 提供器，只能走 `ass_set_fonts_dir` 那条路。
2. mpv-android **显式删除 `subfont.ttf`** ⇒ 它不需要那个文件。
   旧文档 §四.2 第 4 步建议"放一个 `subfont.ttf` 进去"——那是**已被上游否定的做法**。

---

## 二、旧交接文档 §四.2 第 2 步为什么必须跳过

旧文档写：

> **验证办法（成本极低）**：给 mpv 加一条 `--osd-dimensions=video`（让 OSD 按视频尺寸算坐标），
> 重编重装再截一张图。如果字幕立刻出现在视频底部 ⇒ 根因确认。

三条独立证据说明它错：

1. **选项不存在**（§1.1）⇒ 写了不生效，且**不报错**（假阴性）。
2. **几何假设被证伪**：真机 `OSD borders` 与 `Video borders` 逐项相同，
   且 `t=0 b=0` ⇒ 上下没有黑边可供"藏字幕"。
3. **逐像素扫描**（自己解 PNG，`docs/evidence/p253/mpv-kernel/subtitle/`）：

   | 截图 | 视频画面区（x） | 全黑行带 |
   | --- | --- | --- |
   | `pgs-6s-controls-hidden.png` | 99..2380 | 仅 0..21（顶部状态栏） |
   | `pgs-600s-controls-covering.png` | 99..2380 | 仅 0..21 |

   画面从第 99 列一直铺到 2380 列，**上下没有黑带**。

⇒ 这一步不但无效，还会让人以为"OSD 定位不是根因"，进而转向 §四.2 第 3 步（改 FBO 坐标系）——**那是错的**。

---

## 三、已经证明可用的部分（不要重做）

### 3.1 文本字幕通路：完全正常 ✅

做法：外挂一条 SRT，走 libass 通路。

```
[3.252] Subs --sid=6 'probe.srt' (subrip [Advanced Sub Station Alpha]) [external]
[3.253] Set property: sid="6" -> 1
[3.261] fontselect: (Noto Sans CJK JP, 400, 0) -> NotoSansCJKjp-Regular, 0, NotoSansCJKjp-Regular
```

真机截图（播放页 + `adb screencap`）里 **`KIVO-MPV-SUBTITLE-PROBE-AAAA` 清晰显示在画面底部**。

⇒ **OSD 合成通路、字体、上屏、位置，全部正常。**
⇒ 问题**只在 PGS 位图字幕这一条路上**。

### 3.2 字体匹配是成功的（旧文档说"族名全不匹配"是错的）

```
fontselect: (Noto Sans CJK JP, 400, 0) -> NotoSansCJKjp-Regular, 0, NotoSansCJKjp-Regular
```

`mpv/sub/ass_mp.c` 把 `opts->font` 直接设为 ASS 的 `FontName`；
libass 在 `/system/fonts` 里逐个 `ass_add_font`（`NotoSansCJK-Regular.ttc` 的 0 号 face = JP），
**匹配成功**。

旧文档 §四.1 那句"字体族名全部不匹配"**与实测不符**。

### 3.3 一条被隐藏了整整一轮的真 bug：`msg-level=all` 被 mpv 拒绝

旧代码 `mpv_session.cpp` 写的是 `msg-level=all`，但 mpv 的 `msg-level` 是
**keyvalue 列表**（`模块=级别`），裸 `all` 会被判成非法级别名：

```
[OPTFAIL] mpv option rejected: msg-level=all (error setting option)
```

而 mpv 对被拒的选项**不打印任何日志** ⇒ 旧代码里 13 处 `mpv_set_option_string`
**全部丢弃返回值** ⇒ 这个失败**从来没被任何人发现**。

后果：mpv 日志一直不是全量 ⇒ 字体相关信息（`fontselect` 那些）从未落盘 ⇒
上一轮查不到"为什么字幕不上屏"。

**已修**（见 §六）：`msg-level` 改为 `all=debug`
（级别名取自 mpv `common/msg.c`：fatal/error/warn/info/status/v/debug/trace/stats）。
mpv-android 用的也是 `all=v` 这个格式（`main.cpp:68`）。

---

## 四、取证方法上的两个坑（写下来免得重踩）

### 4.1 instrumentation 的虚拟 Surface 尺寸会退化成 1×1

instrumentation 用 `SurfaceTexture(0)`（无消费者）⇒ mpv 的 VO 窗口尺寸退化成 **1×1**：

```
Window size: 1x1 (Borders: l=0 t=0 r=0 b=0)
Video display: (0, 0) 1920x804 -> (0, 0) 1x1
```

后果：
- `screenshot-to-file ... window` 截出来是 **166 字节的 1×1 空 PNG**；
- 位图字幕按 OSD 尺寸缩放 ⇒ 在这个环境里**必然缩成一个点**。

⇒ **字幕渲染的验证必须在真实 Surface（播放页）上做**，instrumentation 做不到。
旧交接文档 §四.4 建议删掉那个测试，结论对，但归因写错了。

### 4.2 `screenshot-to-file ... window` 不依赖 `cur_frame`

旧文档说"render API 下 `cur_frame` 是空的 ⇒ 截图截不到画面"——**这是把两种模式混为一谈**：

- `MODE_VIDEO`（`video`）：取当前视频帧 ⇒ render API 下确实拿不到；
- `MODE_FULL_WINDOW`（`window`）：走 `player/screenshot.c:330`

  ```c
  bool window = mode == MODE_FULL_WINDOW;
  struct mp_osd_res res = window ? osd_get_vo_res(mpctx->video_out->osd) : ...;
  if (mode == MODE_SUBTITLES || window) {
      osd_draw_on_image(mpctx->osd, res, mpctx->playback_pts, OSD_DRAW_SUB_ONLY, image);
  }
  ```

  **独立地把字幕画进图片**，尺寸取自真实 VO。

⇒ 在真实 Surface 上，`window` 模式能拿到 **2320×956 的合成帧**（本次实测 326 KB）。
这是把"mpv 内部有没有生成字幕"与"有没有送到 Surface"分开的**唯一可靠判据**。

---

## 五、PGS 位图字幕：已定位到 mpv 内部的缩放环节

### 5.1 现象

| 事实 | 证据 |
| --- | --- |
| PGS 轨选中生效 | `select track 5 (sub)` → `Set property: sid="2" -> 1` |
| PGS 包确实被解码 | `stream 5: resize index to 128` |
| 位图确实进了 OSD 纹理 | `Reallocating OSD texture to **2048x512**` |
| 文本字幕的纹理是 | `Reallocating OSD texture to **1024x128**`（libass 单通道） |
| 解码**无任何错误** | 日志里没有 `unsupported subtitle type from libavcodec` / `Unable to pack subtitle bitmaps` |
| 但画面上**没有** | mpv 自己的 `screenshot window` 合成帧里也没有（326 KB 的图，只有 BBC logo） |

### 5.2 mpv 侧的机制（源码核对）

**位图字幕的画布基准**（`mpv/sub/sd_lavc.c:478`）：

```c
int w = priv->avctx->width;
int h = priv->avctx->height;
if (w <= 0 || h <= 0 || opts->image_subs_video_res) {
    w = priv->video_params.w;   // 视频尺寸：1920×804
    h = priv->video_params.h;
}
if (current->src_w > w || current->src_h > h) {
    w = MPMAX(priv->video_params.w, current->src_w);
    h = MPMAX(priv->video_params.h, current->src_h);
}
```

**缩放与定位**（`mpv/sub/osd.c:513 osd_rescale_bitmaps`）：

```c
int vidw = res.w - res.ml - res.mr;   // 2320-19-19 = 2282
int vidh = res.h - res.mt - res.mb;   // 956-0-0 = 956
double xscale = vidw / frame_w;
double yscale = vidh / frame_h;
if (compensate_par < 0) {            // PGS 走这一支（video_par = -1）
    compensate_par = xscale / yscale / res.display_par;
}
...
bi->y = lrint(bi->y * yscale) + cy + res.mt;
```

⇒ **字幕位图的纵向坐标是按 `frame_h`（字幕画布高）缩放的**，
而 `frame_h` 来自 `max(视频高, 位图包围盒高)`。

**关键的不匹配**：Shelter 是 **1920×804**（2.39:1 宽银幕，片源被裁过上下），
而 PGS 字幕画布是 **1920×1080**（Blu-ray 标准）。
`rect->y` 是 PGS **画布内**的坐标（FFmpeg `pgssubdec.c:554`：`rect->y = presentation.objects[i].y`），
通常是 1080 画布的下沿 ≈ 950。

**实测的反证**：试过 `--image-subs-video-resolution=yes`（把字幕画布改成视频分辨率），
结果**整帧变黑**（截图从 326 KB 掉到 65931 字节）—— 说明这条路径在本片源上更糟，
**已回退**。

### 5.3 还没排除的两条

1. **PGS 的 `endpts` 语义**：
   FFmpeg `pgssubdec.c:513` 设 `sub->end_display_time = UINT32_MAX`，
   而 mpv `sd_lavc.c:363` 算 `endpts = pts + end_display_time / 1000.0`
   ⇒ 理论上是 **49.7 天**，字幕应当一直显示。
   但实测 `sub-start`/`sub-end` 读出来是 **41 ms 宽**（如 `[14.014..14.056]`），
   连续播放扫描也稳定得到 41 ms 宽、间隔 208 ms 的包序列。
   ⇒ **这条矛盾尚未解释**，是当前最值得先解开的一环。

2. **libass 的 `fontselect` 失败**：
   `fontselect: failed to find any fallback with glyph 0x0 for font: (Noto Sans CJK JP, 400, 0)`
   出现在 PGS 场景（`osd/libass` 通道）里，而文本字幕场景却是匹配成功的。
   两个场景用的是同一套字体配置，**这个差异本身是线索**。

### 5.4 下一步建议（按此顺序）

1. **先解开 §5.3 第 1 条**：把 `sub-start`/`sub-end` 的 41 ms 与
   `end_display_time = UINT32_MAX` 的矛盾查清楚 ——
   重点看 mpv 0.39 的 `sd_lavc.c` 是否在 `sub_fix_timing` 之外还有别的路径改写了 `endpts`
   （`sd_lavc.c:371 prev->endpts = pts` 会**用下一个包的 pts 覆盖上一个包的 endpts**，
   这正好能解释"每个包只有 41 ms"）。
   **验证办法**：连续播放时逐包采样 `sub-start/sub-end`，
   看到包序列时**画面上到底有没有字**。
2. **再查 §5.3 第 2 条**：为什么 PGS 场景的 libass 会报 `failed to find any fallback`，
   而文本字幕场景不会。
3. **若前两条都通了仍不上屏**：再考虑 mpv 的位图合成路径
   （`video/out/gpu/osd.c` 的 `gen_osd_cb` / `upload_osd`）与
   `SUBBITMAP_BGRA` 分支的 `.bgra` swizzle 在 **GLES2 上下文**下的兼容性 ——
   我们的 EGL 上下文声明的是 `EGL_CONTEXT_CLIENT_VERSION, 2`，
   而 mpv-android 用的是 `wid`（mpv 自建 EGL）。这是**架构差异**，值得做对照实验。

---

## 六、本轮落地的代码改动

| 文件 | 改动 |
| --- | --- |
| `cpp/mpv/mpv_options.{hpp,cpp}` | **新增**。选项设置独立成文件，并**逐项检查 mpv 回执**（旧代码全部丢弃返回值）。失败写进 mpv 日志（`OPTFAIL` / `OPTWARN`），降级原因经 `Session::error()` 暴露给上层 |
| `cpp/mpv/mpv_session.cpp` | 选项代码搬走；`Session::error()` 原本**恒返回空串**，现在返回真实降级原因 |
| `cpp/mpv/mpv_session_impl.hpp` | 新增 `degraded_options` 字段 |
| `kotlin/mpv/MpvNative.kt` | `initialize()` 不再吞异常（原来把失败变成"起播失败"，原因全丢） |
| `kotlin/playback/MpvPlaybackSession.kt` | 新增 `native()`（读 mpv 原生属性）；错误信息带上 `e.message` |
| `kotlin/ui/compose/PlayerComposeActivity.kt` | 取证钩子：`kivo.seekMs`（毫秒级 seek）、`kivo.shot`（让 mpv 自己截合成帧）、`kivo.subFile`（外挂字幕）、`kivo.subPos`（运行时改字幕位置）；**并修掉一个真 bug**：选字幕轨会触发 mpv 的轨道刷新把位置打回去，顺序必须"先选轨 → 后 seek" |
| `androidTest/.../MpvSubtitleDiagnosticInstrumentation.kt` | **新增**。读 mpv 原生属性（`sub-start`/`sub-end`/`osd-dimensions/*`）、连续播放采样、让 mpv 自己截图 |

### 复算命令

```bash
export JAVA_HOME="C:/Android/jdk-17.0.13+11"
export ANDROID_HOME="C:/Android/sdk"
export PATH="$PATH:/c/Android/sdk/platform-tools"
cd "C:/kivo video/android"

./gradlew :app:externalNativeBuildDebug --offline          # 改了 .cpp 必须显式跑
./gradlew :app:assembleDebug --offline
./gradlew :app:assembleDebugAndroidTest \
  -Pkivo.testRunner=com.kivo.cinema.mpv.MpvSubtitleDiagnosticInstrumentation --offline

adb -s b6e950ee install -r -t app/build/outputs/apk/debug/app-debug.apk
adb -s b6e950ee install -r -t app/build/outputs/apk/androidTest/debug/app-debug-androidTest.apk

# 读字幕包的覆盖区间 + 连续播放扫描
adb -s b6e950ee shell am instrument -w \
  -e smbHost 192.168.5.1 -e smbShare XiaoMi -e smbPath "视频/kivo-test/shelter.mkv" \
  -e atSec 14 -e subId 2 -e scanFrom 0 -e scanTo 60 \
  com.kivo.cinema.test/com.kivo.cinema.mpv.MpvSubtitleDiagnosticInstrumentation

# 真实 Surface 上取证（字幕包正中 14.076 s）
adb -s b6e950ee shell am start -n com.kivo.cinema/.ui.compose.PlayerComposeActivity \
  --es kivo.sourceId seed-smb-xiaomi --es kivo.smbShare XiaoMi \
  --es kivo.smbPath "视频/kivo-test/shelter.mkv" \
  --ei kivo.subtitleId 2 --ei kivo.seekMs 14076 --ez kivo.freeze true \
  --es kivo.shot pgs-14076.png
adb -s b6e950ee exec-out run-as com.kivo.cinema cat files/p253-shots/pgs-14076.png > shot.png
```

> ⚠️ **PowerShell 下 `-P` 参数会被解析成任务名**（报 `Task '.testRunner=...' not found`）。
> 用 `--%` 让 PowerShell 原样传参：`./gradlew --% :app:assembleDebugAndroidTest -Pkivo.testRunner=... --offline`。
> 任务名是 `assembleDebugAndroidTest`，**不是** `packageDebugAndroidTest`
> （旧交接文档 §三 写错了任务名）。

---

## 七、给下一位接手者的一句话

**字幕通路是好的**（文本字幕已证明上屏），**别再查 OSD 合成、别再查字体、别再加 `--osd-dimensions`**。
要查的是 **PGS 位图字幕在 mpv 内部为什么没被画出来**，
入口是 §5.3 第 1 条那个矛盾（`end_display_time = UINT32_MAX` 却读出 41 ms 的包宽）。
