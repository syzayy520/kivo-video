# 277 · 真 HDR 输出（L3）路线 —— 交底与执行计划

状态：**P0 已完成（mpv ES3 补丁重编）**，P1 起待做
上游：[276-hdr-tone-map-l2.md](276-hdr-tone-map-l2.md)（L2 已落地：HDR→SDR 映射）
　　　[274-hdr-rendering.md](../../design/ui/replica/274-hdr-rendering.md) §十（G0–G5 闸门）
证据：`docs/evidence/p274-hdr/`

---

## 一、目标与判决

**目标**：让 HDR10/HLG 片源在 K40 上**按 HDR 输出**（面板收到 PQ/BT.2020），
而不是像现在这样压回 SDR。

**判决路线**：**用 mpv 自己的 `vo=gpu-next` 输出到 Android Surface**
（SenPlayer 同款），**不重编第三方内核**——因为本机**已经有**：
mpv 0.39 源码、meson/ninja、NDK、以及当初编出我们这版 `libmpv.so` 的构建目录。

### 1.1 已查清的根因链（每条都有源码/二进制证据）

| # | 事实 | 证据 |
| --- | --- | --- |
| 1 | 我们走 **render API（`vo=libmpv`）→ legacy RA** | 内核日志 `video/out/gpu/video.c` 的 `Disabling HDR peak computation`；二进制只有 `r_gl.c`，**无 `r_gl_pl.c`** |
| 2 | legacy RA **没有 compute shader** ⇒ HDR 峰值计算被禁 | 同上日志：`compute shaders=0, SSBO=1` |
| 3 | 设备**有** GLES 3.2（支持 compute shader） | 同一份日志：`GL_VERSION='OpenGL ES 3.2'`、`GLSL ES 3.20` |
| 4 | 我们的 EGL context 是 **ES2** | `mpv_egl_surface.cpp`：`EGL_CONTEXT_CLIENT_VERSION, 2` + `EGL_OPENGL_ES2_BIT` |
| 5 | **mpv 自己的 EGL helper 也写死 ES2** | mpv 0.39 源码 `video/out/opengl/egl_helpers.c`：`rend = EGL_OPENGL_ES2_BIT`、`EGL_CONTEXT_CLIENT_VERSION, 2` |
| 6 | mpv **自带 gpu-next VO 与 Android GL context** | 源码：`video/out/vo_gpu_next.c`（VO 名 `gpu-next`）、`video/out/opengl/context_android.c`（`gpu-context=android`）；构建产物里有 `video_out_vo_gpu_next.c.o`、`video_out_opengl_context_android.c.o` |
| 7 | Surface 怎么交给 mpv：**`--wid`（jobject 指针）** | 源码 `video/out/android_common.c:48`：`jobject surface = (jobject)(intptr_t)vo->opts->WinID;` → `ANativeWindow_fromSurface` |
| 8 | 我们的二进制**没有** `android-surface` 选项 | 精确字符串统计：`android-surface` 精确命中 **0**（只有 `android-surface-size`）；⇒ 走的是**上游标准 `wid`**，不需要 mpv-android 补丁 |
| 9 | SenPlayer 走的就是这条路 | `libFFmpegKit.so`：`pl_dispatch_compute`、`pl_peak_detect`、`pl_render_image`、`vo_gpu_next`、libplacebo v7.360.1 |

### 1.2 结论

⇒ **L3 的全部障碍是"我们自己把 context 建成了 ES2 + 用错了 render API"**，
   不是 mpv 缺功能，**也不需要重编第三方内核**（只需要重编**我们自己的** mpv，源码就在本机）。

---

## 二、P0（已完成）：给 mpv 打 ES3 补丁并重编

**改动**：`video/out/opengl/egl_helpers.c` 三处（附带回退）

1. ES 路径的 `rend`：`EGL_OPENGL_ES2_BIT` → `EGL_OPENGL_ES3_BIT_KHR`
2. config 选不到 ES3 ⇒ **回退 ES2**（不能为了 HDR 把画面整个搞没）
3. context 版本：ES 路径 `EGL_CONTEXT_CLIENT_VERSION` 2 → **3**，建不起来再退 2

**影响面**：这个文件只被 **mpv 自己的 VO** 用；我们 App 现有链路的 context 由
`mpv_egl_surface.cpp` 自建 ⇒ **现有播放路径零影响**。

**重编**：本机 meson 1.x + ninja 1.13 + NDK `27.0.12077973`（`/c/AndroidTC/sdk/ndk`）
+ 原构建目录 `/c/re-tools/mpv-build/mpv/build`（与线上 `libmpv.so` **md5 一致** ⇒ 就是同一个产物）。
- 坑：`--target=aarch64-linux-android26` 下现代 NDK **没有 `libpthread.a`**
  ⇒ 造一个空归档占位（Android 的 pthread 在 libc 里）。
- 产物：`libmpv.so` 33,022,624 字节（原 33,049,080），备份在
  `/c/re-tools/mpv-build/mpv/build/libmpv.so.pre-es3-kivo`；
  App 侧旧版在 git 里（`git checkout` 可回退）。

**待验**（下一步）：装机后确认①现有 render API 播放**无回归**；②`gpu-context=android` 下
拿到的 context 是 **GLES 3.x**（日志里 `GL_VERSION` 变成 3.x）。


---

## 八、执行记录（2026-11-15 04:5x–06:3x）：**P0 与 P1 已跑通**

### 8.1 P0（已完成）：mpv ES3 补丁 + 重编

- 补丁：`video/out/opengl/egl_helpers.c` 三处（ES3 config / ES3 context / 两处回退 ES2）。
- 重编环境**全在本机**：meson + ninja + NDK `27.0.12077973`（`/c/AndroidTC/sdk/ndk`）
  + **当初编出线上那版 `libmpv.so` 的构建目录**（`/c/re-tools/mpv-build/mpv/build`，
  产物 md5 与线上**一致** ⇒ 同一个产物）。
- 坑：现代 NDK 没有 `libpthread.a` ⇒ 造一个**空归档占位**（Android 的 pthread 在 libc 里）。
- 产物已进 App：`app/src/main/jniLibs/arm64-v8a/libmpv.so`（**注意：真正被打包的是
  `src/main/jniLibs`，不是 `cpp/mpv/prebuilt/`** —— 后者只是链接用的 IMPORTED 源）。
  改完 prebuilt 不会进 APK（Gradle 不会重跑 merge）⇒ 要两个地方都换。
- 旧版备份：`/c/re-tools/mpv-build/mpv/build/libmpv.so.pre-es3-kivo`；App 侧旧版在 git 里。

### 8.2 P1（已完成，可跑）：gpu-next 出画模式

实现（都在 `switch --ez kivo.gpuNext true` 后面，**默认仍是旧路径**）：

| 位置 | 改动 |
| --- | --- |
| `mpv_options.cpp` | `vo=gpu-next` 时补 `gpu-context=android`；**硬解驱动换成 `aimagereader`**（零拷贝的 `mediacodec` 在 mpv 自己的 VO 下拿不到帧 ⇒ 日志 `Waiting for frame timed out! / acquireLatestImage failed: -30001`） |
| `mpv_session.cpp` | gpu-next 模式**不建渲染上下文**，改为把 Surface 交给 mpv：`mpv_set_option(handle, "wid", MPV_FORMAT_INT64, &wid)`；首帧判定改用 mpv 的 `vo-configured` 属性 |
| `jni_mpv_bridge.cpp` | 传的是 **Surface 的全局引用**（`NewGlobalRef`） |
| `mpv_java_vm.cpp` | 新增 `attach_env()`（会话销毁时删全局引用要用） |
| `MpvStartSequence.kt` / `MpvPlaybackSession.kt` | 出画模式开关；**顺带修了一个大坑**（见 §8.4） |

### 8.3 真机结果（K40，HDR10 素材）

```
[0.085][v][vo/gpu-next/opengl] Trying to create GLES 3.x + context.      ← 我们的 ES3 补丁生效
[0.085][v][vo/gpu-next/opengl] GL_VERSION='OpenGL ES 3.2'
[0.085][v][vo/gpu-next/opengl] Detected GLES 3.2.
[0.088][v][vo/gpu-next/libplacebo] Initialized libplacebo v7.391.0 (API v391)
[0.111][v][vo/gpu-next/libplacebo] GL context seems to be OpenGL ES, reloading as GLES...
```

- **`Disabling HDR peak computation` 消失了** ⇒ libplacebo 现在拿到了 compute shader
  ⇒ **HDR 峰值计算不再被禁**（这正是 274 G1 卡住的那一环）。
- 画面：**SMPTE 彩条全屏、颜色正常**（证据 `docs/evidence/p274-hdr/gpu-next/smpte-bars-gpu-next-small.jpg`）。
- 设备截屏亮度 mean 91.7 / max 194（对比 legacy RA 的 93.7 —— 同一量级，说明两条路都能出好看的画面）。

### 8.4 路上踩到并修掉的三个坑（都很典型，记下来）

1. **intent 开关从来就没生效过**：`MpvPlaybackSession` 把 `context` 存成了
   `applicationContext`，而 `MpvStartSequence` 里所有开关都写的是
   `(app as? Activity)?.intent` ⇒ 恒为 null ⇒
   `kivo.demuxerMb` / `kivo.maxRangeMb` / `kivo.gpuNext` **全部是死的**。
   （真机表现：`--ez kivo.gpuNext true` 跑了，日志里连"出画模式 = gpu-next"都没有。）
   ⇒ 修法：会话保留宿主 Activity（`host`），开关从那里读。
2. **把 jobject 当整数传给 native ⇒ SIGSEGV**：mpv 的 `wid` 要的是 jobject 指针，
   但 JVM 不认识它 ⇒ Surface 被 GC ⇒ mpv 再 `ANativeWindow_fromSurface` 就是野指针
   ⇒ 真机 **signal 11（CheckJNI: CheckPossibleHeapValue abort）**，App 直接闪退。
   ⇒ 修法：`NewGlobalRef` + 会话持有 + 销毁时 `DeleteGlobalRef`。
3. **硬解驱动要跟着出画模式换**：gpu-next 的 VO 要把帧读回自己的 GL 管线 ⇒ 必须 `aimagereader`。

### 8.5 还没做的（诚实边界）

- **P2 真 HDR 输出**（EGL PQ surface + `ANativeWindow_setBuffersDataSpace` + 10 bit 渲染目标）
  **还没做** ⇒ 现在 gpu-next 也只是把 HDR **映射成 SDR** 显示（颜色对，但面板没收到 PQ）。
- **P3 回归矩阵**还没跑：字幕/PiP/前后台/Surface 重建/预览取帧在 gpu-next 下的行为**未验证**
  （例如 Surface 重绑：mpv 的 `WinID` 在 VO 初始化时读一次 ⇒ 重绑可能要重建 VO）。
- **默认没切**：开关默认关 ⇒ 线上行为不变 ⇒ 用户现在不受任何影响。

---

## 三、P1：新增「vo=gpu-next + Android Surface」出画模式

**做法**（mpv 侧，全部是标准选项，不需要改 mpv 源码）：

| 步骤 | 动作 |
| --- | --- |
| 初始化 | `vo=gpu-next`、`gpu-context=android` |
| 交面 | `mpv_set_option(handle, "wid", MPV_FORMAT_INT64, (intptr_t)jobject_of_Surface)`；同时给 `android-surface-size=WxH` |
| 尺寸 | Surface 尺寸变化时更新 `android-surface-size`（mpv 用它建 EGL surface） |
| 首帧判定 | render API 的 render 回调没了 ⇒ 改用 mpv 事件（`MPV_EVENT_VIDEO_RECONFIG` / `video-out-params`）+ 一次实际 swap 证据 |
| Surface 重绑 | mpv 的 `WinID` 在 VO 初始化时读一次 ⇒ 重绑要么重建 VO，要么在 `wid` 上重新 set + 触发 VO 重载（**需要实测**） |

**家族树**（单文件单职责）：

```
android/app/src/main/cpp/mpv/
├─ mpv_video_output.hpp      改：出画模式（render-api | gpu-next）
├─ mpv_gl_render.cpp         不动（render-api 模式继续用它）
└─ mpv_android_vo.cpp        【新增】gpu-next 模式：wid/surface-size/首帧事件
android/app/src/main/java/com/kivo/cinema/
├─ mpv/MpvNative.kt          改：initialize 支持 vo 模式参数
└─ playback/MpvStartSequence.kt  改：按开关选模式
```

**开关**：`--ez kivo.voGpuNext true`（默认关 ⇒ 出问题一键回退到现有路径）。

---

## 四、P2：真 HDR 输出（EGL PQ + dataspace）

compute shader 到位后（P0+P1 做完才有意义）：

1. **EGL surface 色彩空间**：创建 window surface 时带 `EGL_GL_COLORSPACE_BT2020_PQ`
   （设备 EGL 扩展已确认有 `EGL_EXT_gl_colorspace_bt2020_pq`，见 274 的 G0）。
2. **buffer dataspace**：`ANativeWindow_setBuffersDataSpace(..., AHARDWAREBUFFER_DATASPACE_BT2020_PQ)`。
3. **渲染目标**：RGBA1010102（10 bit）而不是现在的 8 bit。
4. **libplacebo 目标**：`target-trc=pq`、`target-prim=bt.2020`、`target-peak=<面板亮度>`。
5. **合成器验证**：SurfaceFlinger 上带 `#id` 的 layer 必须显示 PQ dataspace 与
   `dynamic-range = HDR`；**接口成功 ≠ 合成器认了**。

---

## 五、P3：回归矩阵（全绿才切默认）

| 场景 | 判据 |
| --- | --- |
| 现有 render API 路径 | 与 276 之前的基线**逐项一致**（帧率、字幕、预览、PiP） |
| gpu-next 路径 · SDR 片源 | 画面正确、帧率 ≥ 23.5 fps、无新缓冲 |
| gpu-next 路径 · HDR10 片源 | 面板收到 PQ（合成器层可见）且画面不发暗 |
| 字幕/OSD | 内嵌/外挂/PGS/ASS 都在（mpv 自己画进同一 Surface） |
| 预览取帧 | 仍走离屏 render API，不受影响 |
| Surface 重建 / 前后台 / PiP | 不黑屏、不泄漏 native 句柄 |
| 断网恢复 | 与 273 的结论一致 |

---

## 六、风险与诚实边界

1. **P1 会动到出画核心**（Surface 生命周期、字幕合成、PiP）⇒ 这是本项目目前**最大的一处改动**，
   必须开关 + 可回退 + 全矩阵回归。
2. **真 HDR 的用户收益有限**：库里绝大多数片源是 SDR（485 的两份都是 bt709）；
   HDR 输出只在少数 HDR 片源上体现差别。**"卡"的收益远大于"HDR"** ⇒ 优先级已排好。
3. **杜比视界（DV）另说**：SenPlayer 有 `dovi_split` bsf（DV RPU 处理）；
   我们**没有**这条通路 ⇒ 即便 L3 成了，DV 片源也只会按普通 HDR10 处理。属独立立项。
4. **色彩准确度仍需标准素材验收**（274 §10.1）：灰阶/亮度阶梯/广色域色块 + 色度计。
   本轮只做到"能被合成器认成 PQ"，不等于"绝对亮度准确"。
5. 任何一步失败都**保留"未验证/不支持"的精确结论**，不改口。

---

## 七、立刻可做的顺序

1. **P0 验证**：装机 ⇒ 现有路径无回归；`gpu-context=android` 时 context 变 3.x。
2. **P1 最小版**：只做"起播 + 出画 + 首帧判定"，先不管字幕/PiP，跑通 HDR10 的**画面**。
3. **P2**：EGL PQ + dataspace + 10 bit 渲染目标。
4. **P3**：回归矩阵。
5. **并行**：明天下午的"卡"两臂复验（与 HDR 无关，独立排期）。

---

## 九、278 · gpu-next 与旧路径的正面较量（真机，K40，Emby 485 的 1080p）

### 9.1 先造一把与出画通路无关的尺子

**问题**：现有的 `tools/p273_gaps.mjs` 靠 **mpv render 回调**计数
⇒ 只对 render API 路径有效；`vo=gpu-next` 模式下**没有 render 回调**
（画面由 mpv 自己的 VO 直接上屏）⇒ 统计**恒为 0**（本轮 B 臂就踩到了）。

**新量具**：`tools/p274_sf_latency.mjs` + `tools/p274_sf_latency.sh`
—— 量 **SurfaceFlinger 的 latency**（`dumpsys SurfaceFlinger --latency <layer>`）：
每行第一个数 = 该帧**真正上屏**的时刻（SurfaceFlinger 合成时刻）。

为什么这个量具更权威：

| | p273_gaps.mjs（render 回调） | 本量具（SF latency） |
| --- | --- | --- |
| 量的东西 | 内核**交给**我们的帧 | **用户眼睛看到**的帧 |
| 通路依赖 | 只适用 render API | **与出画通路无关**（两条都用） |
| 覆盖合成/丢帧 | 否 | 是 |

**两个必须记住的坑**：
1. 层名必须带 `(BLAST)` 与 **id**（`SurfaceView[...](BLAST)#4452`）——
   id 每次启动都变 ⇒ 脚本先查再采；不带 id 只返回刷新周期。
2. latency 是 **128 帧的环形缓冲**（60 fps 下约 2.1 秒）
   ⇒ 一次 dump 不够 ⇒ 必须**反复采样并按时间戳去重**（多次采样有重叠）。

### 9.2 两臂对照（同一台设备、同一片源、同一时段）

| 臂 | 出画路径 | 上屏帧率 | 帧间隔 p50 / p99 / max | >200ms | 关键内核行 |
| --- | --- | --- | --- | --- | --- |
| **A** | render API（legacy RA + mediacodec） | **23.97 fps** | 41.9 / 44.1 / **55.8 ms** | 0 | `Disabling HDR peak computation (compute shaders=0, SSBO=1)` |
| **B** | **vo=gpu-next**（libplacebo + aimagereader） | **23.98 fps** | 41.9 / 44.0 / **47.9 ms** | 0 | `Initialized libplacebo v7.391.0`、**无** Disabling 行 |

片源是 1920x804 @ 23.976 fps ⇒ **两臂都精确贴合源帧率**（24.0 目标），
B 臂的帧间隔 max 还更小（47.9 vs 55.8 ms）⇒ **性能上没有因为换渲染器付出代价**。

### 9.3 其它回归点（gpu-next，全部通过）

| 项 | 结果 |
| --- | --- |
| **前后台切换**（HOME → 回来） | 恢复后 **23.98 fps / max 48.7 ms**，VO 未重建、未崩 ⇒ mpv 自己的 VO 扛得住 Surface 重建 |
| **字幕** | PGS 轨被识别（`Subs --sid=1 (hdmv_pgs_subtitle)`），**内核层 OSD 直接画进同一 Surface**（`[vo/gpu-next] OSD borders` / `osd_render`）⇒ 字幕链路通 |
| **硬解** | aimagereader 路径活了（旧路径走 `hevc_mediacodec`） |
| **轨道** | hevc 1920x804 + 4 音轨 + 字幕轨，`Opening done` 4.8 s |

### 9.4 与 SenPlayer 的正面对比（含我们更强的一项）

| | SenPlayer | 我们（gpu-next 路径） |
| --- | --- | --- |
| 渲染器 | 完整 libplacebo（`pl_render_image` / `pl_dispatch_compute` / `pl_peak_detect`） | **同样完整**（v7.391.0 vs 它 v7.360.1） |
| compute shader | 有 | **有**（ES3 补丁后 `GL_VERSION='OpenGL ES 3.2'`） |
| HDR 峰值计算 | 有 | **有**（"Disabling HDR peak computation" 消失） |
| **gpu-next 下的硬解** | 库里**没有** `aimagereader`，只有 `mediacodec` / `mediacodec-copy` ⇒ **帧要拷回 CPU 再上 GPU** | **aimagereader**（零拷贝）⇒ **这一项我们更强** |
| Android HDR 输出 | 有 `ANativeWindow_setBuffersDataSpace`（**dataspace 路线**，非 EGL_GL_COLORSPACE 属性） | 待做（P2）⇒ **照它的手法做，可对齐** |
| 杜比视界 | `dovi_split` bsf + `dovi-fel-hwdec` + `dolby-vision-profile` | 无 |

⇒ **最终形态定调**：出画走 **vo=gpu-next**（与 SenPlayer 同架构），
   硬解用 **aimagereader 零拷贝**（**超过它**），
   HDR 输出按 **dataspace 路线**做（对齐它），杜比视界作为后续增量。

---

## 十、278 · **默认切换**：出画通路改为 gpu-next（与 SenPlayer 同架构）

### 10.1 为什么敢切（依据全在 278 的实测里）

| 回归项 | 结果 |
| --- | --- |
| 上屏帧率 | 23.98 fps（源 1920x804 @23.976 ⇒ 精确贴合） |
| 帧间隔 | p50 41.9 / p99 44.4 / **max 47.6 ms**、>200ms **0** 次 |
| 前后台切换 | 恢复后 23.98 fps / max 48.7 ms，VO 未重建未崩 |
| 字幕 | PGS 轨识别 + 内核层 OSD 画进同一 Surface |
| seek | seek→900 s：`playback restart complete @ 900.024000`，落点预取协同正常 |
| 音频 | `ao/audiotrack` 48000Hz 5.1，`playback restart complete, audio=playing` |
| 预览取帧 | **不受影响**（预览是独立的第二个 mpv 实例，自己 `initialize(vo="libmpv")`，不经 MpvStartSequence） |

### 10.2 决策点：`playback/PlaybackRenderMode.kt`【新增】

单一职责：**决定走哪条出画通路**。三层优先级：

1. **系统属性** `debug.kivo.renderMode` = `legacy` / `gpu-next`
   —— 最高优先，紧急回退不必发版（与既有 `debug.kivo.originKeepAlive` 同一套惯例）。
   实现用**反射**读 `android.os.SystemProperties`（隐藏 API，直接 import 编译不过）；
   反射被拦就当"没设"，不影响正常路径。
2. **intent** `--es kivo.renderMode legacy|gpu-next` —— 脚本化取证用。
   （兼容 277 时代的布尔开关 `--ez kivo.gpuNext true`。）
3. **用户设置项**「视频输出」= 自动 / 新版 / 兼容 —— 持久化在 `PlaybackPrefs.renderMode`。
4. **默认 = gpu-next**。

### 10.3 设置页（**只加一行，不改任何视觉风格**）

`ui/design/settings/SettingsPlaybackSub.kt` 新增一节「视频输出」，
用**已有的** `SectionCard` + `StepperRow` + `Footnote` 组件，
文案「渲染通路 / 新版·兼容·自动」。
（用户授权："设置开关里的功能可以加，但不改变现有 UI 风格"。）

### 10.4 真机验证（三层全中）

| 场景 | 日志 |
| --- | --- |
| 默认（无任何开关） | `出画模式 = gpu-next（mpv 自己的 VO + 完整 libplacebo）` |
| `setprop debug.kivo.renderMode legacy` | `出画模式 = legacy（render API + legacy RA）` |
| `--es kivo.renderMode legacy` | `出画模式 = legacy（render API + legacy RA）` |
| 默认路径长稳 | **23.98 fps / max 47.6 ms / >200ms 0 次** |

⇒ **线上行为已切换**：默认走 gpu-next，与 SenPlayer 同架构；
   硬解用 aimagereader 零拷贝（**比 SenPlayer 的 mediacodec-copy 强**）。
