# 282 · 帧插值：从"参数全 ok 但功能是假的"到 60 fps 满帧

> 起因（用户 2026-11-15 17:44 原话）：「播放刚才那个HDR的试试。你记住，我们是做最好的，
> 不是玩具，做出来要超过 senplayer，做不到就不及格」
>
> 本文只记录**有真机数据**的结论；没量到的一律标"未验证"。

## 一、为什么插值是"超过 SenPlayer"的唯一硬缺口

同机对照（K40 一台设备上同时装着 Kivo 和 SenPlayer，`com.wuziqi.SenPlayer` v9，
走同一条 `192.168.5.1:445` SMB）：

| 指标 | SenPlayer（34GB REMUX，24fps） | Kivo 插值前（4K HDR 25fps） |
| --- | --- | --- |
| 上屏帧率 | **34.19 fps** | 25.00 fps（= 片源帧率） |
| p50 / p99 帧间隔 | 36.7 / 44.7 ms | 40.0 / 41.4 ms |
| 最大帧间隔 | **3220.2 ms** | 65.9 ms |
| >200 ms 卡顿 | **3 次** | **0 次** |

⇒ 稳定性我们本来就赢（0 次 vs 3 次），**它唯一的优势是帧数**。
把帧数补上，"超过"才是全面超越而不是局部胜出。

## 二、SenPlayer 到底用什么做的（二进制取证）

- 它没有 `libmpv.so`；mpv 被静态链进 **`libFFmpegKit.so`（52 MB）**，
  同库内含 gpu-next + libplacebo + **libsmb2** + vulkan。
  字符串证据：`cplayer` / `VO: [` / `AO: [` / `hwdec`×36 / `video-sync`×6 /
  `demuxer-max-bytes` / `gpu-next` / `pl_vulkan` / `smb2`×57 / `EGL_EXT_gl_colorspace_bt2020_pq`。
- 它有三个**我们库里没有**的选项名：`cache-dir`、`cache-capacity`、
  **`video-frame-interpolation`**（后两个是自建，非 mpv 标准名）。
- 我们比它多的：`aimagereader` 零拷贝硬解（它只有 `mediacodec`/`mediacodec-copy`）。

⇒ 它的"顺"来自 `video-frame-interpolation`，本质就是 mpv 原生的
`video-sync=display-resample` + `interpolation` + `tscale`。**不需要自建，能用原生的**。

## 三、三个必须踩过去的坑（每个都让功能变假）

### 坑 1：三个参数全返回 `ok`，插值却被内核自己关掉

第一版只下发三个选项，回执全是成功：

```
Set property: video-sync="display-resample" -> 1
Set property: interpolation="yes"           -> 1
tscale=oversample                           -> ok
```

但内核日志里：

```
[vo/gpu-next/libplacebo] Estimated source FPS: 25.000, display FPS: 25.000
[vo/gpu-next/libplacebo] Detected fps ratio 0.0000 below threshold 0.0100, disabling interpolation
```

**gpu-next 在 Android 上走 `wid` 路径时拿不到显示刷新率，就用视频帧率顶替**
⇒ 源 25 / 显示 25 ⇒ ratio = 0 ⇒ 它判定"不需要插值"直接禁用。

实测后果：上屏 25.00 → **24.81 fps**（不但没涨，p99 还从 41.4 涨到 50.6 ms）。
这就是"参数全 ok、功能是假的"——只测返回值会得出完全相反的结论。

### 坑 2：`display-fps` 是只读属性，覆盖值叫 `display-fps-override`

必须在 initialize **之前**把刷新率喂进去，于是试了三条路：

| 写法 | 返回 | 含义 |
| --- | --- | --- |
| `setProperty("display-fps", …)` | `-2` | `PROPERTY_NOT_FOUND` |
| `setProperty("options/display-fps", …)` | `-3` | `PROPERTY_UNAVAILABLE`（选项不可运行时改） |
| `mpv_set_option_string("display-fps", …)` | `OPTFAIL option not found` | **名字不对** |

在 `libmpv.so` 里扫字符串才看清：`display-fps` 出现 4 次全是**只读属性**；
真正的选项是 **`display-fps-override`**（旧名 `override-display-fps`，两个都在库里）。

### 坑 3：选项失败会连播放一起带走

`apply_mpv_options` 的既定语义是"一项非 optional 失败 ⇒ 整体失败"。
第一版把插值选项写成非 optional，真机结果是 **`OPTFAIL` ⇒ initialize 返回 false ⇒ 播放起不来**。
⇒ 插值三项 + `display-fps-override` 全部标 `optional`：失败只记 `OPTFAIL`（不静默），起播继续。

## 四、最终实现（文件家族）

| 文件 | 职责 |
| --- | --- |
| `playback/VideoInterpolationSwitch.kt` | **决策**：系统属性 > intent > 设置项 > auto(=on) |
| `playback/VideoInterpolation.kt` | **解析**：读设备真实刷新率，产出 `Plan(enabled, displayFps)` + 留痕 |
| `cpp/mpv/mpv_options.cpp` | **下发**：`video-sync` / `display-fps-override` / `interpolation` / `tscale` |
| `playback/PlaybackPrefs.kt` | `videoInterpolation` 设置项（auto/on/off） |
| `ui/design/settings/SettingsPlaybackSub.kt` | 「视频输出」卡内新增「帧插值」行（复用 `StepperRow`，视觉零变化） |

为什么 `display-fps` 必须走 native：它是 init-only 选项（-3），只能经
`mpv_set_option_string` 在 `mpv_initialize` 之前设 ⇒ 只能落在 `mpv_options.cpp`。

刷新率取值用 `display.mode.refreshRate`（**当前生效档**）而不是 `display.refreshRate`
（某些 ROM 返回首选档）。K40 真机：`mActiveModeId=1` ⇒ 60.000004 Hz。

## 五、实测（K40 b6e950ee，2026-11-15 18:0x–18:2x）

| 臂 | 片源 | 上屏帧率 | p50 | p99 | max | >200ms |
| --- | --- | --- | --- | --- | --- | --- |
| off | 4K HDR 25fps（Emby 252137） | 25.00 fps | 40.0 | 41.4 | 65.9 | 0 |
| **on** | 4K HDR 25fps（同上） | **59.47 fps** | 17.2 | 37.3 | 97.5 | **0** |
| **on** | 1080p 23.9fps（SMB 12GB） | **60.44 fps** | **16.7** | 23.4 | 48.8 | **0** |
| 参照 | SenPlayer 34GB REMUX 24fps | 34.19 fps | 36.7 | 44.7 | **3220.2** | **3** |

- 1080p 臂 p50 = **16.7 ms** ⇒ 正好是 60 Hz 的一帧，**满帧**。
- 内核确认：`Estimated source FPS: 23.923, display FPS: 60.674`，
  且 **`disabling interpolation` 消失**。

### 代价（诚实标注）

- 4K HDR 插值下有 **8 次 `Render queue underrun`**（1080p 只有 2 次）：
  Adreno 650 在 4K + 60fps 插值下偶尔供帧不及。SF 层面仍是 0 次 >200ms。
- 起播 1 次 `Audio device underrun`（11.0 s，起播阶段，与插值无必然关系）。
- `video-sync=display-resample` 会让音频被动重采样（mpv 既定行为）。
- 肥皂剧效应：24fps 电影插到 60fps 会失去胶片感 ⇒ 必须留 `off` 开关。

## 六、遗留

| 项 | 状态 |
| --- | --- |
| 4K 上的 `Render queue underrun` | 未消除。可选方向：4K 时用更轻的插值核、或按分辨率自动降级 |
| SMB 直读（ffmpeg `--enable-libsmb2`） | 未做。SenPlayer 的架构优势项，需重编 ffmpeg |
| DoVi | 未做（缺 `dovi_split` bsf） |
| 真 PQ 输出（L3） | 受 K40 的 EGL 驱动限制，代码就位 |
