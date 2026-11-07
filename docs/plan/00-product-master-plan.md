# kivo video 产品总体规划书（Master Plan）

> 版本：v1.0（2026-11-06）
> 性质：**总执行计划**——之后所有开发按本计划推进，变更需在此文件留痕。
> 依据链：P16 系列实测（21+ 张卡）→ Rex 1.0.2 拆解（`docs/reference/rex-teardown/01`）→
> SenPlayer 1.0.0 拆解（`03`）→ 用户市场反馈与拍板（本文 §1.3）。

---

## 1. 产品定位

### 1.1 一句话

**"Rex 的 UI + SenPlayer 的核心"**：声明式原生 UI 的顶级体验 + 自研可验证播放内核的扎实播放，加上两者都没有做透的差异化能力。

### 1.2 竞品定位（拆解实证）

| | Rex (iOS) | SenPlayer (Android) | **kivo 目标** |
| --- | --- | --- | --- |
| UI | SwiftUI，顶级口碑 | View 体系+混淆，平庸 | **对齐 Rex**（Compose/SwiftUI） |
| 播放 | KSPlayer 拿来主义，**常被吐槽** | 自研核心，**公认好** | **自研（已具雏形）+ 可验证性护城河** |
| 格式 | FFmpeg 全量软解 | FFmpeg 裁剪版软解 | 硬解优先 + 软解兜底（同覆盖） |
| DV | libdovi+自研 Metal 映射 | **仅识别+透传（无映射层）** | A 档透传先齐，B 档真映射超越两者 |
| 功能 | 媒体库/网盘/弹幕/iCloud/订阅 | DLNA/SMB/网盘/下载/缩略图/微信 | 两家功能面并集 |

### 1.3 用户拍板记录（2026-11-06，全部有效）

1. UI 学 Rex；核心与播放能力学 SenPlayer；
2. 技术栈：Android=Jetpack Compose（含 Compose for TV）、iOS=SwiftUI、逻辑层 KMP、核心=C++（现有 backend）；
3. 视频层永远 SurfaceView/CAMetalLayer 零拷贝，不过跨平台合成器；
4. 播放内核永远自研自控——可用开源库但必须源码可控、能自己修（Rex 教训）；
5. DV：A 档（识别+透传，对齐 SenPlayer）先行 1–2 天级；B 档（libdovi+libplacebo 真映射）P2 攻坚；
6. 优先改善维度：核心任务效率、信息组织、视觉品质、性能。

---

## 2. 现状盘点

### 2.1 已有资产（P16 系列，全部有证据路径）

**播放内核（backend/C++）**：
- MediaCodec 硬解（video/audio）+ AAudio 输出（low_latency、audio_timestamp 时钟）；
- 时钟三路健康度（fresh/extrapolated/free_run）+ 漂移门 + 频差估计器（`rate_ppm` 已出版到证据）；
  注入验证：2000/5000 ppm 线性吻合（`196`）；门阈值标定 1000–1500 ppm（`197`）；
- seek：显式凭据（`in_flight()==0` + deadline），无固定睡眠；真机 seek→0 = 23.5 ms；
- **pause-resume 全链路闭环**：`begin_run(preserve_stats)` + `FramePacer::resume_at` +
  `ClockPositionTracker::skip_pause`，真机 1049 帧零丢帧（`204`§8）；
- burst 兜底 G9 显式拒绝；JNI 17 导出异常边界全覆盖；分配钩子双 flavor；
- `position_us` / `publish_cost`（22 µs ≈ 0.067%/帧，已证非瓶颈 `202`）已出版。

**工程化**：
- 8 个防回退门禁（全部带变异自检）+ CI `hard-gates` 9 步、无 `continue-on-error`；
- 性能画像（470 份证据）：丢帧 0.0876%、冷启动 p50 29.1 ms / p99 128.8 ms、seek p50 36 ms（`201`）；
- 行为测试 21 项断言（T1–T17，`b6e950ee` 真机可复算）。

**拆解知识库（`docs/reference/rex-teardown/`）**：
- Rex：组件清单、DV 数据流（FFmpeg RPU→DoviMatrix Metal）、KSPlayer 血统与 swiftinterface API、
  Launch Screen 参数、Info.plist 能力面；
- SenPlayer：三层架构（壳/FFmpegKit裁剪/双 Kit）、JNI 模块全景、UI 框架排除判定、
  DV 透传策略判定、`PreviewReader$Still` 缩略图思路。

### 2.2 缺口（对齐 SenPlayer/Rex 的差距清单）

| 缺口 | 现状 | 目标 |
| --- | --- | --- |
| 软解 | FFmpeg 仅桌面（`KIVO_ENABLE_FFMPEG` avcodec-only 后端已有） | 进 APK，硬解优先/软解兜底路由 |
| 字幕 | 语义层 74 文件未接渲染 | libass 栈进 APK，srt/ass/webvtt/PGS |
| UI | `PlayerActivity` 手写 View 600 行 | Compose 全量重写 |
| 功能面 | 无缩略图/SMB/网盘/DLNA/下载/媒体库/弹幕 | 全清单见 §4 |
| DV | `classify_hdr` 仅有 SDR/HDR10/HLG | + DV profile 识别（A 档）→ B 档映射 |
| 机型 | 仅 kona 验证 | ≥3 SoC 矩阵 |
| iOS/TV/桌面 | 无 | M3/M4 期 |

---

## 3. 技术架构（定案，见 §1.3-2/3）

```
┌─────────────────────────────────────────────────────────┐
│              C++ 核心（一份，跨平台）                       │
│  解码路由(MediaCodec↔FFmpeg) · 时钟/节拍 · DV(A/B 档)     │
│  字幕渲染(libass) · 缩略图(ThumbnailReader)               │
├─────────────────────────────────────────────────────────┤
│  Kotlin Multiplatform：媒体库逻辑 / 队列 / 设置 / 同步状态  │
├───────────────────┬──────────────────┬──────────────────┤
│ Android: Compose  │ iOS: SwiftUI     │ TV: Compose for TV│
│ (手机/平板)        │ (M3)             │ (M4)             │
├───────────────────┴──────────────────┴──────────────────┤
│ 视频层零拷贝：SurfaceView / CAMetalLayer（不可妥协）        │
└─────────────────────────────────────────────────────────┘
```

**红线（不可妥协）**：
1. 视频帧不经过任何跨平台合成器；
2. 播放内核改动的每张卡必须带门禁/行为测试（P16 风格）；
3. 新 UI 不得阻塞 native 线程；所有失败带非空 reason（G9）；
4. 证据 schema 加字段不升版本（`verify_p12_all` 白名单策略）。

---

## 4. 里程碑与卡片清单

> 卡号延续 P16 编号（210 起）。每张卡照旧：先规划设计与家族树 → 实现 → 验证 → 收口。
> 【输入待补】标注的卡片依赖外部输入，先做不依赖项。

### M1「可用播放器」（Android，目标：从能播到能用）

| 卡 | 内容 | 验收口径 | 依赖 |
| --- | --- | --- | --- |
| 210 | **DV-A：识别+透传**——`classify_hdr` 加 `dvcC/dvh1` box 解析（profile/`bl_signal_compatibility_id`），`HdrKind`+`kDolbyVision`；profile 8.x→HDR10 dataspace 透传；profile 5→显式拒绝带 reason | DV 测试片：8.x 正确识别并显示；5 拒绝且 reason 非空；既有 SDR/HDR10/HLG 逐位不变 | 无（`apply_surface_dataspace` 已有） |
| 211 | **软解兜底**——FFmpeg avcodec 后端进 Android APK（仅解码），路由策略：硬解支持→硬解，否则软解 | RMVB/WMV 测试片可播；硬解片源路径与软解路径均出版 `decoder.backend` 字段；4K 硬解片不走软解 | 无 |
| 212 | **字幕进渲染**——libass 交叉编译进 APK，srt/ass 先行，接到 video_plane 叠加层 | srt/ass 显示正确；时间轴随 seek 同步；PGS 冲突提示（对齐 Rex `image_selection_conflict` 语义） | 无 |
| 213 | **Compose 播放页骨架**——`PlayerScreen`（Compose）+ `AndroidView` 包 `PlayerSurfaceView`；控制层：播放/暂停/进度条/时间；手势：双击暂停、滑动 seek/亮度/音量（对齐主流播放器） | 核心任务 ≤4 步；冷启动首帧 ≤300 ms 预算不回退；与现有 `PlayerActivity` 并存灰度 | 210/211/212 先或并行 |
| 214 | **后台播放**——MediaSession + Foreground Service（对齐 SenPlayer `BackgroundPlaybackService`）；接 `position_us` 通知栏进度 | 息屏 30 min 音频不断；回前台画面无缝（复用 P16 pause-resume 判据）；通知栏进度随 `position_us` 刷新 | 无 |
| 215 | **进度条缩略图**（学 `PreviewReader$Still`）——独立 `ThumbnailReader`（低分辨率 MediaCodec/FFmpeg 实例批量出帧 + LRU 缓存），与主链路完全解耦 | 1080p 片源拖动时缩略图首现 ≤500 ms【待验证假设】；主链路丢帧率不回退（门禁看） | 213 |

**M1 出口验收**：单机（kona）完成上述 6 卡；全部门禁绿；新 UI 与旧 `PlayerActivity` 并存可选。

### M2「生态与内容」（Android）

| 卡 | 内容 | 依赖 |
| --- | --- | --- |
| 220 | **SMB 直连**（smbj/JCIFS-NG 选型；KMP 化协议逻辑）——浏览/起播/断点 | 211 软解兜底 |
| 221 | **WebDAV**（kivo 云服务直连优先） | 同上 |
| 222 | **本地媒体库**——MediaStore 扫描 + 文件墙（缩略图/文件夹，对齐 SenPlayer `image×7` + Rex `library*`） | 215 |
| 223 | **弹幕**——B 站 xml/protobuf 格式解析 + Compose Canvas 渲染层（对齐 Rex danmu） | 213 |
| 224 | **Emby/Jellyfin 对接**（学 Rex Drivemby：扫描/入库/webhook 通知）——账号 + 媒体库浏览 + 流播 | 220/221 网络层 |
| 225 | **下载管理**（对齐 SenPlayer download+Service） | M1 |

**M2 出口验收**：SMB/WebDAV 片源 1GB 起播与本地差 ≤2s【待验证】；媒体库千级文件可流畅浏览；弹幕与播放零互相干扰。

### M3「进阶与 iOS」

| 卡 | 内容 | 说明 |
| --- | --- | --- |
| 230 | **DLNA 投屏**（SSDP+AVTransport，学 SenPlayer DLNAClient） | P2 |
| 231 | **私密区**（BiometricPrompt，对齐 Rex FaceID） | P2 |
| 232 | **订阅**（Play Billing，对齐 Rex StoreKit 模式） | P2 |
| 233 | **DV-B：libdovi + libplacebo Vulkan 真映射** | 【输入待补】Ghidra 反编译 Rex 曲线公式 |
| 240+ | **iOS SwiftUI 复刻**（信息架构 1:1，播放核心接同一 C++ backend） | 月级工程，启动需人力决策 |
| 241 | **机型矩阵**：≥3 SoC（kona + 中端 + 联发科），补齐 201 性能画像的多机版 | 持续 |

### M4「TV 与桌面」

| 卡 | 内容 |
| --- | --- |
| 250 | Compose for TV（焦点导航 + 遥控器键位） |
| 251 | Compose Desktop（JVM）——大部分 Compose 代码复用，视频层换平台 Surface 桥 |

### 持续项（不占里程碑）

- **Ghidra 拆解 Rex**（进行中）：完成后产出 ① DV 曲线公式（喂 233）② SwiftUI 布局常量（喂 213 的 tokens）；
  > **2026-11-07 变更（用户决定）**：Rex **UI** 拆解（rea）推迟到 213（Compose 骨架）真正开工时再做，
  > 不作为 211/212 期间的常驻后台任务。DV 曲线（libFFmpegKit 分析）不受影响，继续喂 233。
- **SenPlayer 深拆**（可选）：装 npm 版 rea（jadx provider）反编译 6811 混淆类——若 215/220 实现遇阻再取；
- **用户 UI 参照物**：✅ **两批全部交付并集成（2026-11-07）**：
  ① 真机录屏三段 + 45 截图 → `05-ui-spec-from-recording.md`（运行时像素规格）；
  ② Mac 端 REA v6.7.0 静态全量包（78 份文案 JSON + 资产元数据 + 交互状态机）→
  `rex-package/` + `06-rex-package-integration.md`（结构契约与三源合并裁决）。
  直接增量：220 选型 AMSMB2、223 弹幕=JS Widget、224 Drivemby 四态、232 免费额度、
  233 同栈 libdovi+libplacebo、213v2 色板+P01–P12 动效配方、Anime4K/AI 字幕两潜在新卡；
- **CI 真跑**：8 门禁推分支上 GitHub 实跑一次（需用户授权推分支）。

---

## 5. 风险与对策

| # | 风险 | 对策 |
| --- | --- | --- |
| 1 | MediaCodec Surface 模式拿不到帧 → 215 缩略图/233 DV 映射受阻 | 缩略图走独立低分辨率解码实例（SenPlayer 同款思路）；DV-B 走 FFmpeg 软解+libplacebo 全接管 |
| 2 | FFmpeg 进 APK 的体积/合规（LGPL 动态链接要求） | 动态库形式（SenPlayer/Rex 同款）；保留构建脚本与来源声明 |
| 3 | 混淆竞品的逆向结论置信度 | 全部标注【推断/未知】，关键决策不依赖单一拆解结论 |
| 4 | iOS 并行的人力 | M3 前不定档期；KMP 保证逻辑层届时可直接复用 |
| 5 | Compose 重写期间双 UI 并存的维护成本 | `PlayerActivity` 冻结功能只修不增；213 灰度切换后废弃 |

---

## 6. 执行纪律（不变）

1. 一张卡一份规划设计与家族树，先设计后实现；
2. 门禁先行：新能力必须带防回退判据（变异自检）；
3. 真机纪律：`b6e950ee` 需 `KIVO_ALLOW_PRIMARY=1` 才触碰；默认 `dd137cf7`；无 `uninstall`；
4. `F:\kivo`（用户音乐播放器）**只读**，永不写入；
5. 每卡收口更新 workspace memory 与本计划的卡号状态。

---

## 7. 卡号状态看板（随执行更新）

```
M1: [x] 210 DV-A（2026-11-07 收口，AC1-AC5 全绿）   [x] 211 软解（2026-11-07 收口，AC1-AC6 全绿）   [x] 212 字幕（2026-11-07 收口，AC1/2/4/6 全绿、AC3 结构性、AC5 代码接线+媒体待补）   [x] 213 Compose骨架（2026-11-07 收口，真机 4K HEVC 播放 ✓）   [x] 214 后台播放（2026-11-07 收口，AC1-AC4 全绿）   [x] 215 缩略图（2026-11-07 收口：冷启动 71ms/预算500、扫描期主链路 0 丢帧、LRU 9 淘汰封顶 50；8/8 变异打红）
M2: [x] 220 SMB（v1 收口 8bfd9f7：六 AC 实证 + SmbActivity UI 全链 + 节拍曲线仪表）  [x] 220b 硬解过代理（cleartext 修复 + pts 节拍 → 1.00× 恒定，12 采样实证）  [x] 221 WebDAV（硬解真机播放 + seek 实证 67aa223；ByteSource 接口复用——SMB/WebDAV 同管线）  [x] 222 媒体库（7cba7f7：MediaStore 157 视频扫描 + 海报墙 + 点播 K40 一次通过）  [x] 223 弹幕（c915649：B站XML解析 + Compose Canvas 滚动层 + 弹开关——K40 实证弹幕滚动渲染 ✓）  [ ] 224 Emby  [x] 225 下载（58.2MB 落盘 Movies/Kivo + ffprobe 完整 + 入库 157→158 + 1GB 强杀断点续传 → 7/7 变异自检）
M3: [ ] 230 DLNA  [ ] 231 私密区  [ ] 232 订阅  [ ] 233 DV-B  [ ] 240 iOS  [ ] 241 机型矩阵
M4: [ ] 250 TV  [ ] 251 桌面
```

**下一张卡：210（DV-A 识别+透传）**——不依赖任何外部输入，基建已有半程，按 P16 纪律开卡。
