# 交接文档：kivo video 产品开发（2026-11-06 晚）

> 交接人：Agent（本会话）→ 下一任助手
> 总计划书：**`docs/plan/00-product-master-plan.md`（v1.0）——之后一切开发按它推进，先读它**
> 本文档只讲计划书之外的：决策背景、进行中任务、工具环境、坑。

---

## 0. 一句话现状

后端播放核心完成 P16 加固系列（21+ 张卡，8 个防回退门禁，pause-resume/seek/时钟全闭环），
两轮竞品拆解（Rex/SenPlayer）完成，产品蓝图与里程碑已定案，
**下一张卡 = 210（DV-A 识别+透传）**，UI 侧等 Ghidra 反编译产出与用户参照物。

---

## 1. 用户决策记录（最高优先级，勿违）

1. **产品定位**：UI 学 Rex（声明式、体验顶级），核心与播放能力学 SenPlayer（自研、播放优化公认好）；
2. **技术栈**：Android=Jetpack Compose（含 Compose for TV）、iOS=SwiftUI、逻辑层 KMP、核心=C++ backend；
3. **红线**：视频层永远 SurfaceView/CAMetalLayer 零拷贝；播放内核永远自研自控（可用开源库但必须源码可控——Rex 拿 KSPlayer 黑盒 dylib 导致播放问题修不了的教训）；
4. **DV 两档**：A 档（识别+透传，对齐 SenPlayer——它经判定**没有**自研映射层）先行；B 档（libdovi+libplacebo Vulkan）P2 攻坚；
5. 市场反馈背书：Rex UI 顶级/播放被吐槽，SenPlayer 播放好——我们的差异化 = SenPlayer 的核心 + 可验证性（8 门禁）+ Rex 级 UI。

---

## 2. 进行中的后台任务（接手第一件事）

**Ghidra 正在分析 SenPlayer 的 `libMLibKit.so`**（DV 优先级最高，用户指定 DV 挖 SenPlayer）：

- java 进程 PID 56140（启动 21:44 左右），预计 15–40 分钟完成；
- **进度日志：`C:\re-tools\sen.log`**（tail 它看进度；出现 "Analysis succeeded" 即完成）；
- 完成后执行提取：
  ```
  export JAVA_HOME="C:/re-tools/jdk-25.0.5+8"
  cd /c/re-tools
  ./ghidra_12.2_PUBLIC/support/analyzeHeadless.bat "C:/re-tools/ghidra-projects" SenProject \
    -process MLibKit -noanalysis -postScript dump_dovi_functions.py
  ```
  输出落在 `C:\re-tools\dovi_functions.c`——整理进 `docs/reference/rex-teardown/03` 并更新计划书 233 卡输入；
- **之后启动 Rex UI 分析**（严格串行！见 §4 内存红线）：
  ```
  ./ghidra_12.2_PUBLIC/support/analyzeHeadless.bat "C:/re-tools/ghidra-projects" RexProject \
    -import "C:/kivo video/build/rea-rex/raw/Payload/Rex.app/Rex" \
    -analysisTimeoutPerFile 5400 -processor AARCH64:LE:64:AppleSilicon \
    -preScript slim_analysis_options.py > /c/re-tools/rex.log 2>&1 &
  ```
  完成后 `-process Rex -postScript dump_rex_ui_functions.py`，输出喂 213 卡的 Compose tokens。
- **输出必须重定向到文件**（`> xxx.log 2>&1 &`），绝不能走管道 `| tail`（Ghidra 日志量大，管道缓冲写满 java 会阻塞假死——已踩坑两次）。

---

## 3. 下一张卡开工指引：210（DV-A 识别+透传）

- 目标：`classify_hdr()` 加 DV 识别（解析 `dvcC`/`dvh1` box：`dv_profile`/`bl_signal_compatibility_id`），
  `HdrKind` 加 `kDolbyVision`；profile 8.x → 复用既有 `apply_surface_dataspace()` 按 HDR10 透传；
  profile 5 → 显式拒绝 + 非空 reason（G9）；
- 已有基建：`surface_dataspace.cxx`（dataspace 设置，真机验证过）、`hdr_kind.hpp`（kSdr/kHdr10/kHlg）、
  `video_color_facts.cxx`（`classify_hdr`）；
- 参考字段定义：KSPlayer swiftinterface 的 `DOVIDecoderConfigurationRecord`
  （`C:\kivo video\build\rea-rex\ksplayer\Sources\KSPlayer.xcframework\.../arm64-apple-ios-simulator.swiftinterface` 875 行起）；
- 纪律：按 P16 风格先写设计稿（`docs/design/backend-core/210-*.md`）+ 门禁（变异自检）；
- 测试片：`tests/real/` 下现有片源无 DV 样本——需自制或找 DV profile 8 样本（可用 ffmpeg 给 HEVC 塞 dvcC box 模拟，或下公开 DV 样片）。

---

## 4. 工具环境与血泪坑（全部实测）

### Ghidra（`C:\re-tools\ghidra_12.2_PUBLIC\` + JDK 25.0.5 `C:\re-tools\jdk-25.0.5+8\`）
- **内存红线**：24GB 机器**严禁并行两个 headless**——已发生内存耗尽双死（2h+ 分析全丢，Ghidra 中断不保存！headless 是最后才 commit）；
- **管道红线**：输出必须 `> file.log 2>&1 &`，走 `| tail` 会缓冲写满假死；
- 大二进制（90MB）全量分析 1.5–2h；用 `slim_analysis_options.py`（preScript 关 Decompiler Parameter ID 等）可砍半；
- 孤儿进程识别：`decompile.exe` 父进程退出即孤儿，`Stop-Process` 安全杀。

### rea（`F:\Files\实用工具\AI工具\rea`，v6.8.0）
- Windows 限制：深度反编译四 provider 全不可用（Ghidra-Win 只认 PE / Hopper 无 win / IDA、Kuna 未配）；
  Apple 命令（inspect-macho/plist）要 macOS；
- **可用**：`decode-interface-builder`（storyboard/nib，Windows OK）；
- Android 命令需 npm tarball 的 native 构件（本地 git clone 缺 `native/windows/build/rea-windows-x64.node`）——
  `npm i -g rea-agents` 可补，未装；
- 目标参数必须传**文件**（APK/IPA 本体或 .app 目录），解压目录会拒。

### 网络
- **大文件下载必须 `curl --noproxy "*"`**——系统代理（127.0.0.1:24668）对 GitHub release 资产 CONNECT 502，
  直连反而 89MB/s（Ghidra 570MB 20 秒下完）。

### 真机
- 设备：`b6e950ee`（kona，主测试机，**默认禁止**，`KIVO_ALLOW_PRIMARY=1` 才放行——用户已口头授权过本轮使用）、
  `dd137cf7`（默认目标机，当前不在 USB）、华为 MatePad（offline，adbd 无响应需重启平板）；
- **无 uninstall**；安装用 `install -r`；压测前 `svc power stayon usb`（息屏会冻住 sleep 和播放）；
- seek/压测实验用 `--es kivo.finalize on_stop`（45s→6s/轮）。

### 其他
- `F:\kivo`（用户音乐播放器）**只读**，永不写入；
- JNI 改签名**四处联动**：Kotlin 调用方、`NativePlayer.kt` external 声明、bridge 实现、native 实现；
- `docs/handoff/CURRENT.md`、`backend/CMakeLists.txt`、`pace_stats.*`、`av_sync_report.cxx`、
  `tools/verify_p12_all.py`、`docs/evidence/` **他人主线占用，不碰**；
- Gradle 需要 `export JAVA_HOME="C:/AndroidTC/jdk-17.0.2"`；NDK 在 `C:/Android/sdk/ndk/30.0.16248370`。

---

## 5. 关键文件索引

| 文件 | 内容 |
| --- | --- |
| `docs/plan/00-product-master-plan.md` | **总计划（宪法）**：M1–M4 卡片清单、验收口径、卡号看板 |
| `docs/reference/rex-teardown/01` | Rex 拆解：DV 数据流（FFmpeg RPU→DoviMatrix Metal）、KSPlayer 血统 |
| `docs/reference/rex-teardown/02` | Rex UI 拆解第一部分：rea 能力边界、SwiftUI 结构性限制、功能面证据 |
| `docs/reference/rex-teardown/03` | SenPlayer 拆解：三层架构、DV 透传判定、UI 排除法 |
| `docs/reference/rex-teardown/04` | 产品蓝图：S1–S8/R1–R9 能力矩阵 |
| `docs/handoff/P16-CLOSEOUT.md` | P16 结题索引：五条结论、21 张卡索引、7 个接手坑 |
| `.workbuddy-ai/memory/2026-11-06.md` | 今日全量工作日志（最细粒度） |

## 6. 待办优先级（接手后）

1. **监控 Sen 分析完成 → 提取 DV 结果**（§2 命令现成）；
2. **开 210 卡**（§3 指引现成）；
3. Rex UI 分析（串行，Sen 完成后）→ Compose tokens 喂 213；
4. 向用户催 UI 参照物（他承诺提供）→ 213 前转设计规范；
5. M1 其余卡按计划书推进。
