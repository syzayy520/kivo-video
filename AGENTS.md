# Kivo Video · 助手上手入口

> **任何助手进入本仓库，先读完本文件（约 1 分钟），再动任何文件。**

## 零、先把项目完整拿到手

#
## 0.0 装机铁律（2026-10-30 事故后加，两件都不许再犯）

### ① 签名：已固定在仓库里，不许换

`android/app/build.gradle.kts` 的 `signingConfigs.create("kivoDebug")` 指向
`android/keystore/kivo-debug.jks`。**任何账户、任何助手构建出来的 debug APK 签名都一样**，
`adb install -r` 永远能覆盖安装。

为什么必须这样：这台机器上有 **四个 Windows 账户**（Administrator / CodexAlt / codex / huyasi），
各自有一份 `~/.android/debug.keystore`。Android 的 debug 构建用**当前账户**那把私钥签名，
于是同一份代码、同一个 `applicationId`，换个人构建就 `INSTALL_FAILED_UPDATE_INCOMPATIBLE`。
现象看起来像"项目包被换了"，实际只是密钥换了 —— 用户为这个已经问过一次。

> **同一项目包 = 同一个目录 + 同一个 `applicationId`**，签名不是身份。
> 但签名不一致会逼着人 `uninstall`，而 `uninstall` 会**删掉应用私有证据**。

### ② 禁止为了装 APK 去 `adb uninstall`

`uninstall` 会连带删掉：

- `files/evidence/` —— 设备上还没归档的取证（可能就是别人几小时的活）；
- `/sdcard/Android/data/com.kivo.cinema/files/media/` —— 推上去的测试片源（几十 MB，要重推）。

真要 `uninstall`（例如签名仍未对齐），**顺序必须是**：

```bash
# 1) 备份私有证据
adb shell "run-as com.kivo.cinema ls files/evidence" > list.txt
while read -r f; do adb shell "run-as com.kivo.cinema cat files/evidence/$f" > backup/$f; done < list.txt
# 2) 备份外部片源
adb pull /sdcard/Android/data/com.kivo.cinema/files/media backup/media
# 3) 卸载安装
# 4) 装完先把片源推回去，再把证据 cp 回去（cp 比逐文件 cat 快）
```

另外：HyperOS 上 `adb install` 会弹「USB安装提示」并在 **9 秒后自动拒绝**，
所以安装常常返回 `INSTALL_FAILED_USER_RESTRICTED: Install canceled by user`。
处理办法是安装的同时点「继续安装」（`adb shell input tap <x> <y>`，
1080×2400 上约为 `347 2111`），不是去改系统设置。

## 0.1 成果已入库，**也已推送**（`git clone` 从 2026-10-31 起可用）

**这条 2026-10-31 22:20 变了。** 之前这里写的是"入库了但没推送、只能整目录拷" ——
那个状态已经结束：`7b8c291` 把 P14-L…X 共 13 张卡（146 个文件）一次性提交并推上去，
分支同时挂上了 upstream。现在 `git clone` 拿到的不再是 P10 基线。

| 做法 | 结果 |
| --- | --- |
| `git clone` + `git checkout codex/kivo-video-p0-ui-01-playback-overlay-v13` | ✅ 现在能拿到全部源码、证据与交接文档 |
| 整目录拷贝 | ✅ 仍然可以，但要排掉构建产物（见下）；好处是 `.git` 历史一起带走 |

三条命令自证（**不要照抄数字，跑一遍**；数字随提交变化）：

```bash
git rev-parse HEAD                        # 应包含 `7b8c291`（P14-L…X）或其之后的提交
git rev-list --count origin/codex/kivo-video-p0-ui-01-playback-overlay-v13..HEAD   # 0 = 已同步
git status --porcelain | wc -l            # 0 = 工作区干净
git ls-files | wc -l                      # 3990 = 库里跟踪的文件数
```

> ⚠️ **「已提交」不等于「已备份」。** 本地 `.git` 和源码在同一个磁盘上，
> 磁盘坏 / 目录误删，两者一起没。真正的备份只有两种：`git push`，或者整目录拷到另一块盘。
> 现在这条已经做到了（远端有 `7b8c291`），但**以后每张卡收口都要推**，别再攒成一次。

> ⚠️ **本机 `git` 会直接罢工**：仓库属主是 `DESKTOP-UFK7V16/CodexAlt`，而当前登录用户是 `codex`，
> 所以任何 `git` 命令都报
> `fatal: detected dubious ownership in repository at 'C:/kivo video'`。
> **这不是仓库坏了，是 Windows 账户换了。** 三种解法任选其一：
>
> ```bash
> # ① 只对当前命令生效（不改配置，推荐先这样确认能读）
> git -c safe.directory="C:/kivo video" status --porcelain | wc -l
> # ② 对本仓库永久生效
> git config --global --add safe.directory "C:/kivo video"
> # ③ 把目录属主改回当前用户（需要管理员权限）
> ```
>
> 不解决它，`git status` / `git diff` 全部不可用 —— 但**不影响编译、不影响 adb、不影响 ctest**。

| 内容 | 状态（`7b8c291`） |
| --- | --- |
| P12 / P13 / P14-A…X 的 C++ 源码（`backend/`） | ✅ 已入库并推送 |
| `docs/design/backend-core/`（G1–G10、阶段口径、家族树、162 篇设计/状态稿） | ✅ 已入库并推送 |
| `docs/evidence/`（698 份真机证据，另 3 份 `superseded/` 已作废） | ✅ 已入库并推送 |
| `android/` 整个 Gradle 工程 | ✅ 已入库并推送 |
| `docs/handoff/CURRENT.md`、`TASKS.md`（本文件旁边的接手文档） | ✅ 已入库并推送 |

拿到手之后核对"是不是全的"，**只认一条**：`git ls-files | wc -l` 必须等于 **3990**。

> 原来这里是一张"每个目录几个文件"的清单（2026-10-29 E1 收口后实测）。
> 它在 P14 的 13 张卡落地后立刻过期了 —— 现在 `docs/evidence` 早已不是 218。
> 清单没法自证，一条 `git ls-files` 可以。

拷贝而不是克隆时，**排掉构建产物**：`build/`、`android/build/`、`android/app/build/`、
`android/.gradle/`、`android/.kotlin/`、`android/app/.cxx/`
—— 它们占了 `android/` 51 MB 里的绝大部分，而且 configuration-cache 里存的是本机绝对路径，
带过去只会重算或给出误导性报错。

备份 / 交接用这一条（Git Bash 里也能跑，把 `D:\` 换成目标盘）：

```powershell
robocopy "C:\kivo video" "D:\kivo-video-backup" /E /XJ /XD build .gradle .kotlin .cxx
# /E  含空目录      /XJ 跳过 junction（新机器用 bootstrap.cmd 重建）
# /XD 排除构建产物   不要用 /MIR —— 它会删掉目标端多出来的文件
```

实测这条命令在本机的结果（`/L` 演练，2026-10-29 06:47）：
**9,590 个文件 / 1,185 个目录 / 305 MB**，其中 8 个 junction 被正确跳过。
交接量就是这么大，一个 U 盘装得下。

> **风险**：P12 的成果眼下只有本地这一份物理副本。换机器、磁盘故障、误删目录，
> A/B/C 三个阶段一起消失。这不是技术债，是数据安全 —— 建议至少做一次离线备份。

### 0.2 工作目录是仓库根，不是 `android/`

**工作目录是 `C:\kivo video` 整个仓库，不是 `C:\kivo video\android`。**
`android/` 只是 Android 壳，C++ 后端一行都不在里面。

| 目录 | 是什么 | 里面有什么 | 单独拿会怎样 |
| --- | --- | --- | --- |
| `backend/` | **跨平台后端核心（真正的代码）** | `src/video/video_plane/{core,backends,probe,runtime}` + 镜像的 `include_private/video/...` | 出不了 APK；桌面端也编不过（缺 `cmake/`、`third_party/`） |
| `android/` | **Android 壳** | Gradle（AGP 9.4.1）+ JNI 桥 2 个 `.cxx` / 2 个 `.hpp` + Kotlin 4 个文件 | **CMake 立刻 `FATAL_ERROR`**，见下 |
| `docs/` | 交接与验收事实来源 | `handoff/`（怎么接手）、`design/backend-core/`（口径与家族树）、`evidence/`（真机证据） | 不知道下一步做什么、不知道数字从哪来 |
| `.codex/memory/` | 项目铁律 | `kivo-video-iron-gate.md` | 会平铺文件、会堆大函数，然后返工 |
| `third_party/ffmpeg-sdk/` | 桌面端 FFmpeg 链接桩 | `include`/`bin` 为 junction，`lib` 由 DLL 导出表重生成 | 桌面 CMake 链接报 `LNK1104`（Android 端不用 FFmpeg，不受影响） |

`android/app/src/main/cpp/CMakeLists.txt` 第 10–14 行是这么定位根目录的：

```cmake
set(KIVO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../../..")   # cpp -> main -> src -> app -> android -> 仓库根
set(KIVO_BACKEND "${KIVO_ROOT}/backend")
if(NOT EXISTS "${KIVO_BACKEND}/src/video/video_plane")
    message(FATAL_ERROR "KIVO_ROOT resolved incorrectly: ${KIVO_ROOT}")
endif()
```

**它靠相对路径往上跳五层去拿 `backend/` 的源文件，没有 `add_subdirectory`。**
只拷 `android/` 时，实测（2026-10-29）直接停在配置阶段：

```
CMake Error at CMakeLists.txt:14 (message):
  KIVO_ROOT resolved incorrectly:
  .../cpp/../../../../..
```

注意报错里那个 `../` **没有被规范化解析**，所以它只会说"路径算错了"，
**不会提示"你少拷了 backend"**。要交接就给整个 `C:\kivo video`（含 `.git`）。

依赖方向是单向的：`android/` → `backend/`，反向不存在。
`backend/` 不许 include 任何 Android 头文件，`core/` 下更是一个都不许有（见 `docs/handoff/CURRENT.md` §六）。

### 0.3 换机器必补的三样（不是代码问题，是环境问题）

把整个仓库拷到另一台机器后，代码是全的、构建却起不来。这三样都属于"绑定本机"：

| 缺什么 | 为什么会缺 | 不补的现象 | 怎么补 |
| --- | --- | --- | --- |
| `android/local.properties` | 被 `android/.gitignore` 忽略；里面写的 `sdk.dir` 是本机路径 | 构建 18 秒后失败：`SDK location not found...`（实测 2026-10-29） | `echo 'sdk.dir=C:/Android/sdk' > android/local.properties` |
| `third_party/ffmpeg-sdk/{include,bin,lib}` | `include`/`bin` 是指向 `C:\ffmpeg-sdk\...` 绝对路径的 junction，`lib` 由本机 DLL 导出表生成 | 桌面端 `LNK1104: 无法打开文件 avformat.lib` | `bootstrap.cmd "C:\ffmpeg-sdk\ffmpeg-n7.1-latest-win64-gpl-shared-7.1"`（幂等；**需本机已装该源 SDK**） |
| adb 授权 | `~/.android/adbkey` 绑定本机用户 | `adb devices` 为空 / `unauthorized` | 重建 `~/.android/adbkey`，细节见 `docs/handoff/CURRENT.md` §五 |

> Android 构建**不需要** FFmpeg（`kivo_player` 只链 `android`/`log`/`mediandk`/`aaudio`），
> 所以只做 Android 阶段的话，中间那行可以先跳过。

---

## 一、必读（按顺序）

1. **铁律门闸** —— `.codex/memory/kivo-video-iron-gate.md`
   构思 → 规划设计 → 自然家族树 → 实现 → 单文件单职责 → 禁止平铺。
   **未完成家族树设计不得进入实现。** 改动架构、目录、模块前必须读。

2. **交接快照** —— `docs/handoff/CURRENT.md`
   现在在哪、怎么跑起来、环境与设备、会咬人的坑、架构上不许破的线、
   接手自检单。只给你 30 秒就读这一份。

3. **任务台账** —— `docs/handoff/TASKS.md`
   下一步做什么，以这里的任务卡为准，**不以聊天记录为准**。
   **P12 结题；P13-A/B/C 已收口**。下一卡：`av_drift` 采样语义统一。**P12-E 已结题（E1–E4）**。
   开工前**必须**读 `27-p12d1-status.md` §三 —— D1 在真机上踩到
   `setOutputSurface(nullptr)` 切不回 Surface（−10000），"摘面"因此改成本地行为；
   同一节还记了"摘面时不能放掉 ANativeWindow*"。这两条在任何 Surface 相关改动里都会再遇到。
   **P12-D 已结题**（D1–D5）。上屏时刻那一条的关键教训在 `34-p12d5-status.md` §一：
   `dumpsys SurfaceFlinger --latency` 的层名**必须带 `#id`**，少了它不报错、只返回刷新率 ——
   C7 就是把它当成了"平台不支持"。凡"某个命令拿不到数据"的结论，都要连同命令行一起留下。

4. **验收口径** —— `docs/design/backend-core/03-goals.md`（G1–G10）、
   `07-p12-plan.md`（P12 阶段划分与红线）、`08-p12-family-tree.md`（文件归位）。

**接管完成判据（两条都满足才算能动手）**：
① 上面 4 份读完，能不看文档说出"当前指针是哪张卡、它的闸门是什么"；
② `docs/handoff/CURRENT.md` §七 自检单**全绿**。
少一条就别写代码 —— 缺的是环境知识，不是代码能力。

## 二、三条不可协商

1. **没有证据路径，不许说"完成"。** 证据落在 `docs/evidence/<stage>/`，必须能在真机复算。
2. **模拟器数字无效。** 主测试机 Redmi K40（`b6e950ee`），无硬件 MediaCodec 的环境不产生有效证据。
3. **不许静默降级。** 任何能力缺失必须带非空 reason 且用户可见；软解回退（尤其 10bit）一律禁止。

## 三、当前状态

| 阶段 | 状态 |
| --- | --- |
| P12-A 真机能力矩阵 | 已闭环 |
| P12-B 硬解 + 零拷贝上屏 | 已闭环（1080p HEVC10 175.3fps / 4K HEVC10 127.4fps） |
| P12-C1 AAudio 输出骨架 | 已闭环（真机 xrun=0、零补静音、**人耳确认出声**） |
| P12-C2 SPSC 环形缓冲 | 已闭环（9/9 单测双平台、真机 20/20、欠载计数已验证有效） |
| P12-C3 音频解码与 PCM 供给 | 已闭环（AAC 真机 480,256 帧零丢弃、`xrun=0`、**人耳确认出声**） |
| P12-C4 音频主时钟 | 已闭环（7 次真机零回退、启动开销双峰 25/127–147ms、延迟 57/153ms） |
| P12-C5 呈现节拍器 | 已闭环（24/30/60fps 三档零丢帧，抖动 p99 小于 0.25ms 分辨率；关节拍对照 7.36× 实时） |
| P12-C6 送收分线程 | 已收口 2026-10-29，**判据未成立**（增益 1080p8 −0.9% / 1080p10 +2.0% / 4K +0.5%，全在噪声内），默认已回退 `kMerged`；顺带证伪「往返是瓶颈」 |
| P12-C6B 呈现回压边界 | 已结题 2026-10-29，**结论是不做优化**。2.01ms 不在 `release` 在 `dequeue` 空等；仅当解码快于显示时出现，真实节拍下 TRY_AGAIN 从 2.03 降到 0.02–0.31 次/帧、receive 占 wall 从 85.5% 降到 0.5–3%，全零丢帧 |
| P12-C7 精确呈现时刻 | 已收口 2026-10-29，**判据成立、已设为默认**。`releaseOutputBufferAtTime` 把 `due_at` 交给合成器；负载下提交提前量 12 次运行 spread 全为 0，旧结构交出错峰则从 0.18ms 涨到 2.12ms。⚠️ 买到的是"不受调度摆布"，不是"画面更流畅"，两者都 0 丢帧 |
| P12-C8 证据产出 | 已收口 2026-10-29，**C 组结题**。AudioVideo 首次同进程同时跑，`av_drift` p99 = 0.125–0.375ms（G7 阈值 40ms），0 丢帧/0 xrun/0 时钟回退；`tools/verify_p12c_sync.py` 从分布重算 p99 逐位相同。⚠️ **只有 `cpu_paced` 路径可测**，默认 `surface_at_time` 下证据写 `measurable:false` + 理由 |
| P12-D1 Surface 重建 | 已收口 2026-10-29。切后台再回来 `resume_ms` = 2–29 ms、`frames_lead` = 0，三档空窗全 0 丢帧 0 崩溃，播放速率不动。⚠️ 真机上 `setOutputSurface(nullptr)` 切不回 Surface（−10000），摘面改成本地标志 + 丢帧；空窗丢帧单列 `frames_discarded_no_surface` |
| P12-D2 多实例并发 | 已收口 2026-10-29。3 路 1080p 各自 130.8–132.9 fps = 单路的 **81.3–82.6%**（门槛 60%），3 路 @30fps 零丢帧。**关键**：4 路合计仍是 395 fps ⇒ 天花板 ≈396、单路只吃 41%，单路瓶颈是每实例往返不是算力。⚠️ 多路各占 1/3 屏，缩放开销更少，结论偏乐观 |
| P12-D3 长稳 30min | 已收口 2026-10-29。54000/54000 帧、`playback_rate 0.999990`、内存 **−0.62 MB**（判据 <5MB）、31 个采样点丢帧全 0。⚠️ 两个测量坑：停机瞬间 PSS 掉 45MB 必须裁窗口；**这台 K40 时钟比 UTC 快 8h**，证据时间戳不能跨机器相减 |
| P12-D4 生命周期收敛 | 已收口 2026-10-29。收尾顺序从"三处代码的先后"变成 `session_teardown_plan.hpp` 里一份 constexpr 表 + 一个执行器；`stop` 是 `destroy` 的**严格前缀**。单测 6 例/36 断言，两次变异分别打红 11/6 项。Kotlin 侧 `PlaybackSession` 成为句柄与 fd 的唯一入口，C8/D1 两条取证路径回归一致 |
| P12-D5 上屏时刻 | 已收口 2026-10-29，**P12-D 结题**。推翻 C7 的"不可测"：层名要带 `#id`。7 轮 `surface_at_time`（含 16 忙循环压到 **1.1% 空闲**）稳态 **0 次错过 vsync**、Δ max ≤ 16.47 ms（= 刷新周期）；`SIGSTOP` 150 ms ×3 把 max 推到 54–93 ms ⇒ 量灵敏。⚠️ 中途因"负载没真停"推倒重测过一次，现在每轮记录实测空闲率 |
| P12-E1 颜色协商 | 已收口 2026-10-29，**结论是否定的**：播放路径 `color-format` = 厂商私有 `0x7FA30C06`，8bit 与 10bit 片源同值 ⇒ 位深不可判定；探针三次 configure（含"Surface 无窗口"矛盾请求）全成功、start 后都是 NV12 ⇒ 组件不采纳 `color-format`。证据写 `indeterminate` + reason |
| P12-E2 HDR 标记下发 | 已收口 2026-10-30。HDR10 片源判 `hdr10`、`delivered_color_*=6/6/2`、`dataspace.applied=true`（`ADATASPACE_BT2020_PQ`）、无标记 SDR 不误判；四轮 300/301/300 零丢帧。⚠️ **`hdr-static-info` 未达成**（轨道格式里没有这个键，只在 SEI 里）。三处实测更正：`BT601_NTSC=4`/`SDR_VIDEO=3` 且无 `BT2020_CL`；`setBuffersDataSpace` 在 `libnativewindow.so`；色彩键来自 SPS VUI |
| P12-E3 色域标记 | 已收口 2026-10-30，**"可查"落到合成器侧**。HDR10 轮 SF 图层 `dataspace=BT2020_ITU_PQ(298188800)`、`hdr metadata types=3`、`dynamic_range=HDR`；SDR 轮 `V0_BT709`/`0`/`SDR`；3+2 轮一致。⚠️ 三处坑：`HDR Panel:true` 是能力不是状态；SF 数值 298188800 ≠ 我们设的 163971072（比名字不比整数）；`metadata types=3` 只证明类型位在，不证明值对 |
| P12-E4 tone mapping | 已收口 2026-10-30，**按字面判据达成、映射本体未实现**。三条路径全实测：① `set-user-disabled-hdr-types` 在 MIUI 上设完即被清回（造不出 SDR 屏）；② `color-transfer-request=3` 被解码器忽略（`output_transfer` 仍 6）；③ GL 路径=换架构，不做。对比截图（同像素只改标记）：中位亮度 71→61（−14%）；证据 `hdr.tonemap.implemented=false` + reason |
| P12-F1 `seek_to` | 已收口 2026-10-30。三次 seek 到 5 s，`total_us` 39.8/28.1/33.5 ms（G8 预算 150 ms），落点精确、seek 后每段 150/151/150 帧零丢帧。做法：停线程→定位→flush→与 `start()` 共用的 `begin_run()`。⚠️ 修了 `seek_to` 恒返回请求值的旧 bug；⚠️ `landing` 离 target 远先查片源关键帧间隔（第一版片源只有一个关键帧） |
| P12-F2A 音频 seek 修复 | 已收口 2026-10-30。**两处改动各解一个问题**：① 不重启 AAudio 流，改静音栅栏（`render()` 返回 0，不碰环）→ `fallback/catch_up` 归零；② 音频追视频落点（`defer_start` + `resume_after_seek`）→ 后向 seek 的 gap 490 ms→0。**14 轮全绿**：gap ≤ 8 ms、0 丢帧、total 19–60 ms。⚠️ 栅栏靠 10 ms 时间余量不是锁；静音期未单列计数 |
| P12-F3 错误路径 | **DONE 2026-10-30（4/4，经 F3B）**。`bad-file` / `no-video-track` / `no-audio-track` / `encrypted` 四条全部 PASS，`verify_p12f3_errors.py` 退出码 0，变异能打红 |
| P12-F3B 加密轨识别 | 已收口 2026-10-30。**原判「平台不报加密」是错的**：平台报在轨道格式的 `crypto-mode`（`int32(1)`）上，而 `AMEDIAEXTRACTOR_SAMPLE_FLAG_ENCRYPTED` 在 K40 恒不设。三步实测：注 `pssh` 无效 → fMP4 无效 → `track_format_dump` 看到 `crypto-mode`。改判后 `open()` 即拒（返回码 12）、reason 点名 `aes-ctr`；明文对照 `verdict=unknown` 且 300/301/300 帧零丢帧。⚠️ 三态：`encrypted`/`clear`/**`unknown`**，第三态必须存在（明文片源同样缺这个键） |
| **P13-C 4K+音频** | **DONE 2026-10-30**。12 轮零丢帧/零 xrun。**分辨率不是变量，帧率才是**：4K60 与 1080p60 的 `av_drift` 几乎相同（p99 0.6–1.9ms），而 30fps 基线是 0.125–0.375ms —— 60fps 让尾部涨 4 倍（p50 恒 0.125ms，说明是采样密度而非漂移变大）。⚠️ 修掉两个「证据字段恒错」bug：`audio.opened` 从未被赋值、`audio.started` 被 stop() 清掉；收口加了跨字段一致性检查（`clock_source=audio_timestamp` ⇒ opened/started 必须 true） |
| **P13-D 采样归一化** | **DONE 2026-10-30**。抽稀按媒体时间 20 Hz，30/60/4K 在 8 s 上都是 160 点（差 0.21%）；**同源对照下 60fps 的 p99 反而更低（0.23×）—— P13-C 的"帧率才是变量"没有复现**；漂移首次有斜率，35 s 片长分辨率约 7 ppm，只能说"< 7 ppm" |
| **P13-E 口径前缀** | **DONE 2026-10-30**。`av_drift` 顶层不再有裸 `p99`，分位数一律挂 `quantiles.<口径>`；`cross_frame_rate_set` 用路径指出唯一可跨帧率比的那一份（不可比时为 `null`，`not_measured` 时 `quantiles` 是 `{}` 不是 0）。schema 升 `sync/3`，13 轮证据全部重采 |
| **P13-B 冷启动/seek 分离** | **DONE 2026-10-30**。**P13-A 那句「4K 上 150ms 不够」是错的**（混了冷启动与 seek，且 132ms 本来就没超 150ms）。实测 seek 4K 只有 28–49ms，比 1080p 只慢 5–15ms。已拆成 G8a（冷启动 ≤300ms）+ G8b（seek ≤150ms），21 轮冷启动 + 9 轮 seek 全 pass。⚠️ 首版阈值 200ms 按单轮数据拍，真机首轮就 206ms 红 —— **阈值必须按已观测最大值定**。根因：两个场景共用一个计数器且 seek 会覆盖冷启动值，现已分开存 |
| **P13-A 4K/60fps/高码率** | **DONE 2026-10-30**。四组全零丢帧：1080p60 46.7Mbps（60.2fps/首帧 47ms）、4K60 HEVC8（60.0/66ms）、4K60 AVC8（59.4/116ms）、**4K60 HEVC10（59.2/133ms）**。**证伪 G8 的 150ms 预算在 4K 上不够**（光解码起跑就吃 66–133ms）→ 下一卡 P13-B。⚠️ 顺带修掉一个活了 6 张卡的 bug：renderer 有两个 `"source"` 键，JSON 重复键让 `source.mime/width/height` 静默消失；收口层 1 现在查重复键 |
| **P12-F4 证据总收口** | **DONE 2026-10-30，P12 结题**。交付物是 `tools/verify_p12_all.py`（不是总结）：230 份证据 / 四层 / 7 个子校验器 exit 0 / 46 项已声明豁免。自检 8 变异 **8/8 抓得到**。收口时修正了 3 处**我自己写错的判据**（中途快照 `completed=false`、负向样本 `reached_eos`、paced 传错证据集）。⚠️ 豁免必须明示 —— 靠隐藏换来的全绿比红更危险 |
| **P12-E HDR / 10bit** | **已结题（E1–E4 全部收口）**。能说的：识别/下发/合成器认账/三条 tone mapping 路径的实测结论；**不能说的**：位深可判定、静态元数据已下发、已实现 tone mapping |
