# 212 · 字幕进渲染 —— 设计稿

> 状态：**设计完成，实现未开始**。总计划 M1 第三张卡。
> 卡片原文：libass 交叉编译进 APK，srt/ass 先行，接到 video_plane 叠加层；
> 验收：srt/ass 显示正确、时间轴随 seek 同步、PGS 冲突提示（对齐 Rex `image_selection_conflict` 语义）。
> 纪律：本稿先于实现；实测不符时更正块留原位（210/211 先例）。

---

## 一、一句话目标

**srt / ass 字幕在播放画面上正确显示、随 seek 同步、PGS 图像轨给出可见的冲突提示——
视频主链路（两条 backend）一行不动。**

## 二、现状盘点（2026-11-07 实查）

| 资产 | 位置 | 对本卡的意义 |
| --- | --- | --- |
| 字幕面状态机（84+ 文件，P8 ABI） | `subtitle_plane/{abi,command,core,format,frame,generation,text,...}` | 语义/状态机层：分类、租约池、文本/字体 blob 库、证据摘要——**唯独没有光栅化器**（"零像素"诊断不变） |
| 契约词汇表 | `cinema_engine/subtitle_core/{entry,render_plan,overlay,...}` | 策略字段（字体解析/安全区/HDR 亮度……全是计划词汇），本卡按字段语义对齐出版 |
| ASS/SSA 语义分类 | `subtitle_plane/format/ass_semantics.*` | 分类器（是/否/样式语义报告），不是解析器——srt/ass 解析交给 libass |
| 呈现接缝先例 | 211 的 `IFfFrameSink` 接口注入模式 | 复用同款思路：渲染器与呈现解耦，桌面可测 |
| `position_us` | P16-P1B26 已出版 | 字幕时间轴的驱动源（**解耦关键**，见 D3） |
| libass / 依赖链 | 本机无 | 交叉编译：libass + freetype + fribidi + harfbuzz（D1） |
| 字幕测试片 | 无 | srt/ass 是文本格式，自造（D5）；PGS 场景用 mkv 容器装一条 PGS 轨 |

## 三、设计决策（五条）

### D1 光栅化器：libass + 依赖链交叉编译（M1 唯一承诺的渲染器）

libass 是 ASS/SSA/SRT 的事实的解析+光栅化标准（mpv/VLC 同款）。依赖链：
**freetype（必须）、fribidi（必须，双向文本）、harfbuzz（必须，复杂 shaping）**；
fontconfig / expat **不编**（Android 无 fontconfig，字体走内存注入，见 D4）。

- 构建：`third_party/libass-android/build.sh`（ffmpeg-android 同款模式：无空格产物目录、
  静态库、七坑经验直接继承）；freetype 走其官方 Android 编译法，fribidi/harfbuzz 走
  meson（MSYS2 `pacman -S meson ninja`，走代理）；
- 产物：`dist/lib/{libfreetype,libfribidi,libharfbuzz,libass}.a` + 头文件；
  许可证：libass ISC、freetype FTL、fribidi/harfbuzz 老版 LGPL/MIT——README 声明。

### D2 叠加路径：第二块透明 View，不碰视频主链路

P12-E4 已实测：走 GL 合成 = 架构变更（fps/上屏时刻全部重取证）。**M1 用 Android 合成器**：
视频 SurfaceView 之上叠一块透明 `SubtitleOverlayView`，libass 每帧产出 RGBA 位图 →
JNI 拷出 → Kotlin `Bitmap` → `onDraw` 画布绘制。合成由系统 SurfaceFlinger 完成，
视频主链路（两条 backend）一行不动——P12 红线零触碰，代价只是叠加层的合成开销
（文字面积小，实测进证据）。

### D3 驱动解耦：字幕渲染器只吃 `position_us`，不进解码会话

字幕的本质是 `(文件, 当前时间) → 位图`。**它不需要知道视频是谁解的**：

```
SubtitleOverlayController
  ├── 字幕文件（外挂 .srt/.ass，与视频同名；M1 不做内封轨提取）
  ├── 时间源：position_us（硬解/软解两条路径都已出版——同一控制器服务两条 backend）
  └── 输出：OverlayView.setBitmap(rgba, w, h) → invalidate
```

这带来两个直接收益：① 硬解与软解路径**自动同时获得字幕能力**（零额外接线）；
② seek 同步天然成立——seek 后 position_us 跳变，控制器下一帧渲染新时间点的字幕
（libass 对时间查询是状态无关的）。**M1 不把字幕接进解码会话**，避免动两条会话。

### D4 字体：内存注入 + 设备字体文件

Android 无 fontconfig。libass 走 `ass_set_fonts(..., "none")`（禁用系统提供者）+
`ass_add_font()` 注入字体字节。M1 从 `/system/fonts/` 运行时读
（Roboto-Regular 保证拉丁；NotoSansCJK 保证中文），不随 APK 打包字体（体积与许可）。
字体缺失时 libass 回退到默认字形——证据记录 `font_loaded` 列表，缺字体不静默。

### D5 测试片与 PGS 提示

- **srt/ass**：文本格式自造（与视频同名的 `.srt`/`.ass` 外挂文件，
  含中文+英文+样式行；ass 版带定位/颜色/多层事件）；
- **PGS**：`ffmpeg` 把一条 PGS 轨 mux 进 mkv（`-c copy -c:s hdmv_pgs_subtitle`）；
  路由探测时发现字幕轨为图像类（`video/x-pgs`/`hdmv_pgs_subtitle`）→
  叠加层显示 **"图像字幕（PGS）暂不支持——文本轨可正常显示"** 的可见提示
  （对齐 Rex `image_selection_conflict` 的"冲突告知"语义：不假装支持，也不静默）。

## 四、家族树（落位）

| 文件 | 新/旧 | 单一职责 |
| --- | --- | --- |
| `third_party/libass-android/build.sh` + `README.md` | **新** | 依赖链交叉编译（freetype→fribidi→harfbuzz→libass，顺序构建） |
| `subtitle_plane/render/libass_renderer.{hpp,cxx}` | **新** | libass 桥：load(path)/render(now_ms) → RGBA 位图 + 事件计数（唯一碰 `ass_*` 的层） |
| `subtitle_plane/render/overlay_bitmap.hpp` | **新** | 位图结构（w/h/stride/bytes，可移植，桌面可测） |
| `jni/jni_subtitle_bridge.cxx` | **新** | load/render/destroy 的 JNI（DirectByteBuffer 零拷贝出图） |
| `subtitle/SubtitleOverlayView.kt` | **新** | 透明叠加 View（onDraw 画位图） |
| `subtitle/SubtitleOverlayController.kt` | **新** | 轮询 position_us → 驱动渲染 → 投给 View |
| `PlayerActivity` + 布局 | 旧（小改） | 叠加 View 挂载 + 控制器接线 + PGS 提示 |
| `tools/verify_p212_subtitle.py` | **新** | 证据门禁 + 变异自检 |
| `tests/real/sd212/` | **新** | 自造 srt/ass/PGS 三件测试素材 |

## 五、验收判据

| # | 判据 | 通过条件 |
| --- | --- | --- |
| AC1 | srt 显示 | 外挂 srt：播放中叠加层出现字幕位图，文本事件数与文件一致，截图存证 |
| AC2 | ass 显示 | 外挂 ass（含样式/定位）：渲染不崩、位图非空、样式字段进证据 |
| AC3 | seek 同步 | seek 后 overlay 内容与新时间点一致（控制器渲染时间与 `position_us` 差 ≤ 一帧间隔，证据出版） |
| AC4 | 双后端 | 硬解（mp4+srt）与软解（wmv+srt）两路都出字幕（D3 解耦的实证） |
| AC5 | PGS 提示 | 含 PGS 轨的 mkv：提示出现且视频正常播 |
| AC6 | 门禁 | `verify_p212_subtitle.py` 自检变异全抓 |

## 六、明确不做（范围外）

内封字幕轨提取（M1 只做外挂文件）· ASS 特效（\t 动画/卡拉 OK）·
字幕样式设置 UI（213 的 Compose 层）· PGS 位图渲染（libass 不做图像轨）·
字体打包进 APK · 字幕面状态机（P8 ABI）的深度接线（后续卡，M1 只做分类查询级复用）。

## 七、开工顺序

1. 依赖链交叉编译跑通（freetype→fribidi→harfbuzz→libass，产物自检：`ass_library_*` 符号）；
2. `libass_renderer` + 桌面单测（真 srt/ass 渲染 → 位图非空 + 事件计数）；
3. JNI + OverlayView + Controller，真机 srt 轮；
4. seek 同步证据 → PGS 提示 → 门禁 → 双真机取证。
