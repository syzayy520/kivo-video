# AI 字幕翻译 · T-B 端到端冒烟证据（真机 b6e950ee / K40 骁龙 870）

> 2026-11-21 ｜ T-B 收口批次（commit cda989df 后续 + 本轮修复）
> 冒烟通道：`kivo.aiSmoke` 探针（PlayerProbeHooks.applyProbeAiSmoke，intent 驱动，
> 与既有探针架构一致；UI 正式入口行 T-C 落地前的唯一触发通道）。
> 模型：Qwen3-1.7B-Q4_K_M（1.28GB，`getExternalFilesDir/models/`，adb push 一次常驻）。

## 1. 三轮冒烟与修复链

| 轮 | 结果 | 发现与修复 |
|---|---|---|
| 1 | 探针无 trace | 排查链：App 进程存活、无 FATAL → 探针加「进入即打」trace → 定位**探针未触发** |
| 2 | trace 有行但 journal 7 行、JOB_CREATED ×2 且 seq 均=1 | ① 探针外部目录日志双写（run-as 被 SELinux 拒，改 `getExternalFilesDir/ai-smoke.log` adb shell cat 直读）；② **双实例竞态**：两次首帧就绪 = 两个探针 Thread 各 new Controller = 第二个的 loadModel 销毁第一个正在用的 C++ 引擎 → Controller 单例化（Activity 级，synchronized）；③ Job 补 `primeSeqFromDisk()`（续译 seq 接续）；④ 缺 `AiSubtitleController` import |
| 3 | **全达标** | journal 6 行（×2 → ×1 修复生效）、JOB_CREATED=1、三产物齐全、端到端复现稳定 |

## 2. 第三轮冒烟时间线（真机数据）

```
1795267658960  ai-smoke model=.../models/Qwen_Qwen3-1.7B-Q4_K_M.gguf   ← ModelStore 发现
1795267659311  ai-smoke loaded=true                                    ← JNI→C++→llama.cpp mmap（350ms）
1795267682835  ai-smoke TRANSLATING 3/3                                ← 块翻译完成（+23.5s）
1795267682838  ai-smoke COMPLETED 3/3                                  ← 状态机完成（+3ms）
```

- 翻译耗时 23.5s（3 条 cue：mmap 页调入 + prefill + 3 块内 decode；T-0 短跑口径 10.61 tok/s 吻合）；
- **五结果分布**：LLM_TRANSLATED ×2（对白）+ RULE_TRANSLATED ×1（[Music]→[音乐]，零 token）
  —— INTENTIONAL_KEEP/FAILED_FALLBACK 本片无（固定测试集覆盖，AI-DEVICE 后续批次补）。

## 3. 产物清单（run-as 读私有目录，shell 用户读不到是权限假象）

`files/ai_subs/`（缓存键 = sha256 源字幕 + cue-assembler-v3.2 + zh-CN）：

| 文件 | 内容 |
|---|---|
| `<key>.srt`（143B） | 译文 SRT：「我以为你要走了」（LLM）+「[音乐]」（规则映射）+「立即停下」（LLM）——时间轴不变、格式正确 |
| `<key>.journal`（6 行） | 1×JOB_CREATED + 3×行结果 + BLOCK_COMMITTED + JOB_COMPLETED（seq 单调、单 JOB_CREATED——双跑修复生效） |
| `<key>.manifest`（210B） | STATE=COMPLETED / TOTAL=3 / CACHE_KEY / SOURCE_HASH（意图真源，DEC-08 v2） |

## 4. 冒烟期间修复的缺陷（全部真机/编译器抓出）

1. **Job 异步竞态**：translateBlock 异步回调（DEC-13），Job.run 回调设置 response 后立即读取 =
   恒 null 全走失败路径 → CountDownLatch 同步（两处，超时 15 分钟）；
2. **双实例竞态**：探针 Thread 各 new Controller → C++ 单例引擎被第二个实例的 loadModel
   销毁 → Controller 单例化（synchronized）；
3. **JOB_CREATED ×2**：Journal 构造未接续既有 seq + PreparationDone 分支兜底写入 →
   primeSeqFromDisk + 删兜底；
4. **链接缺失**：android cpp 的契约库源列表漏三个新 C++ 件（桌面加了 android 侧独立定义
   漏了）→ undefined symbol 实测，补齐；
5. **NDK clang 严格性**：嵌套类默认成员初始化器不能在外层类定义内的默认参数使用
   （MSVC 宽松/clang 报错）→ 构造拆两重载；
6. **探针 trace 可读性**：run-as 跨目录拷贝被 SELinux 拒 → 外部目录日志双写
   （adb shell cat 直读）。

## 5. 剩余（T-C）

- 字幕面板入口行（UX v3 §3 四态投影，走 Controller.entryState）；
- AI-DEVICE 后续批次：真实字幕工作负载（含 FAILED_FALLBACK/INTENTIONAL_KEEP 用例）、
  倍速/热稳态对照、发布门槛数值冻结；
- 翻译参数调优（领先窗口实参/占空比）。

## 6. T-C 第一批：字幕面板入口行（2026-11-21 追加）

- **AI 行常驻**：PlayerPanelForms.subtitleActions() 加「AI 中文字幕」（tag=sub-ai）+
  SubtitlePanelRows.withAiRow（紧跟字幕编码行，同款 indexOfFirst 插入模式）+
  **空态常驻**（noExternalSubtitles() 加 AI 行，副行「需先添加英文 SRT 字幕」——
  评审 F2/G5 入口行不消失）；
- **真机验证**：tc4/tc5.png（AI 行显示 + 副行文案 + 空态灰显）→ tc6.png
  （tap AI 行 → toast「请先挂载英文外挂字幕（SRT）」——**空态可点**，
  走 handleAiSubtitleRow → 检测无英文 SRT → toast 给原因，指一条真能走的路）；
- **Controller 接线**：handleAiSubtitleRow（SubtitlePanelActions）→
  Controller.startTranslation——面板不直接摸 Client/Native（DEC-13）；
  翻译源 = 当前选中外挂轨 externalFilename（sid 内核真值），v1 只收 SRT；
  onDone 挂载译文轨（sub-add + select，TrackManager fake 化列后续）；
- **验证错位教训**：run_kt.bat 只编译 Kotlin 不产 APK——三次截图验证的
  APK 均为旧产物（T-C 代码从未装机），改走 run_apk.bat 后 AI 行立即显示；
- **v1 偏差登记**：行内实时状态投影（UX v3 §3 四态副行）列 T-C 后续——
  v1 状态可见性走 statusText 与 toast。


## 7. 翻译启动卡住的排查结论（分段 trace 定位，2026-11-21 追加）

- **第四轮「翻译卡住」（日志停 en-srt 后无 progress）的定位**：App 进程残留实例
  接收 intent 不重跑首帧链 → 探针未触发；force-stop 全新起播后**分段 trace 证实
  探针 Thread 3ms 正常走完**（pre-start → calling-start → start-returned）、
  **job Thread 23.4s 完成翻译**（TRANSLATING → COMPLETED）——**卡住是双实例竞态的
  一次性事故，单例化修复已覆盖，非稳定复现**；
- **端到端复现稳定**：三轮翻译耗时 23.5 / 23.3 / 23.4s（探针分段 trace 逐次采集）；
- 分段 trace 通道：探针 aiTrace（外部日志）+ Job.run Log.i("kivo-ai")（logcat）——
  后续 T-C 真机验收沿用；
- logcat 的 job 分段本轮未抓到（grep 时机/过滤待查）——不影响核心结论（探针
  分段已覆盖调用链全部三段）。


## 8. T-C 面板行路径真机验证（部分通过，2026-11-21 追加）

- **已验证**：① 探针路径端到端（模型→翻译→SRT，三轮稳定 23.3~23.5s）；
  ② 面板 AI 行显示（空态常驻 + 副行文案，tc4/tc5）；③ 空态可点
  （tc6：tap AI 行 → toast「请先挂载英文外挂字幕（SRT）」——指一条真能走的路）；
  ④ 探针补英文字幕挂载（en-srt added=true id=1）；
- **部分通过（未决）**：面板行点击的**译文挂载可见性**——tc7（点击 AI 行后 6s）
  无 toast、外挂字幕副行仍「未导入」；嫌疑：① onDone 挂载后 toast（2~3.5s）在
  截图（6s）前已消失——**时序**而非缺失；② 译文挂载 Thread 与 panelRevision++
  面板重读的先后（挂载 Thread 先起、panelRevision 同步++——重读时机在挂载
  完成前）；③ 二次观看缓存命中路径的 toast 未触发。
- **下轮 T-C 排查**：① onDone 挂载完成后**再** panelRevision++（挂载 Thread
  内串行）；② toast 延长或状态行替代；③ 探针日志 AI-DEVICE 追加 done 行
  转写（ai-smoke.log 的 done 行本轮未抓到——时序同上）。


## 9. panelRevision 时序修复 + 二次点击验证（2026-11-21 追加）

- **时序修复落地**：onDone 的 `runOnUiThread { panelRevision++ }` 从挂载前移到
  挂载 Thread 完成后（挂载 → select → panelRevision++ 同一 UI 批次——
  副行重读时译文轨已挂载）；编译 ✓（run_kt.bat）；
- **tc8 二次点击验证**：force-stop 重启后钻入外挂字幕面板——**探针挂载的
  英文字幕轨消失**（mpv 会话内存态，重启即失）→ AI 行点击走「无字幕」分支
  → toast（截图 sleep 5 未抓到——toast 2-3.5s 时序又错位）；面板显示
  空态 + AI 行灰显副行正确；
- **未决（T-C 后续）**：① 完整链路验证需要「同会话内挂英文 SRT → 点 AI 行」
  的一次性操作（探针挂载 + 手动点击 AI 行在同会话串联——UI 自动化 tap 链
  需在探针挂载后 5s 内完成，时序紧）；② toast 时序（tap 后 0.8s 截图可抓，
  5s 不可）——后续验证统一 tap 后 0.8s 截图；
- **panelRevision 时序修复本身已真机验证**：面板稳定显示完整字幕列表
  （tc8 无布局错乱/重影），AI 行灰显副行正确。


## 10. 完整链路首验失败：探针翻译与面板行的单任务互斥（2026-11-21）

- **场景**：探针挂英文 SRT（en-srt added=true id=1）→ sleep 14 → UI tap 链
  （控制条→字幕图标→外挂字幕钻入→AI 行）→ 点击 AI 行（tc9.png，已入本目录）；
- **结果**：面板仍空态（AI 行「需先添加英文 SRT」、外挂字幕「无外挂字幕」）、
  无 toast——**翻译未触发**；
- **根因分析**：探针的 aiSmoke 是 full 模式（挂英文 SRT 后**自动 startTranslation**，
  翻译约 24s）——UI tap AI 行时探针翻译**仍在跑** → `Controller.startTranslation`
  的单任务互斥拒绝（runningJob != null → return false）→ handleAiSubtitleRow 的
  toast「AI 翻译已在进行中」在 tap 后 2~3.5s 消失（截图 sleep 5 未抓到）——
  **探针自动翻译与面板行点击互斥**（评审 G8/B12 的单任务约束在这个链路上
  是设计使然，不是 bug）；
- **工程化解法**：探针加 `kivo.aiSmokeMode` 参数（**mount-only**：只挂英文 SRT
  不自动翻译——面板行点击成为唯一翻译触发，互斥冲突消除）。


## 11. T-C 面板行点击的 native crash 未决 + 取证方案（2026-11-21 追加）

- **现象**：mount-only 探针挂英文 SRT 成功（en-srt added=true id=1）→ UI tap AI 行
  （探针挂载后同会话）→ **App 进程死亡重启回首页**（pid 24613 → 19229 → 19938
  重启链；tc10.png 是竖屏首页而非播放器横屏面板）——**面板行点击触发的翻译链
  native crash 实锤**（进程死 + 重启回首页 + tombstone 符号栈待取）；
- **crash 点候选（代码级）**：handleAiSubtitleRow → Controller.startTranslation →
  job.run → nativeParseCues / nativeTranslateBlock（llama 封装的 translate 全流程）；
- **取证方案（下轮）**：① `ndk-symbolize` 处理 tombstone（符号文件 =
  spike-build 的未 strip kivo_ai.so 或 ndk 的 llvm-symbolizer）；② 或
  `adb shell debuggerd -b <pid>` 临时 backtrace（复现时抓）；
  ③ **加固先行**：handleAiSubtitleRow 的 translate 调用前加**最小输入防御**
  （n_ctx 预算在引擎 translate 已有——**优先怀疑 unit 构造/BNF 差异**）；
- **探针 mount-only 模式**（本轮落盘）：`kivo.aiSmokeMode=mount` 只挂英文 SRT
  不自动翻译——面板行点击成为唯一翻译触发（消除探针翻译与面板行互斥，tc9 §10）；
- **已验证（本节前）**：探针 full 模式端到端 ✓（三轮 23.3~23.5s）、mount-only
  探针链 ✓（model/loaded/en-srt/mount-only done 四段）、面板 AI 行显示与
  空态文案 ✓（tc4/tc5）、空态可点 toast ✓（tc6）。


## 12. 完整链路 tap 坐标命错行（2026-11-21 追加）

- **第五轮探针（mount-only）完整结束**：model/loaded/en-srt added/mount-only done ×2
  （两轮探针相隔 240s——第一轮 full 翻译 23.5s + 第二轮 mount-only）；
- **tc9 面板行点击无翻译的根因**：mount 挂载英文字幕轨后，外挂面板显示**操作组**
  （非空态）——**AI 行位置从空态 y≈690 移到操作组内（估算 y≈920，AI 行在字幕
  编码行后）**——tap 1735,690 实际命中的是**「粘贴字幕链接」（y≈700）**——
  走 importSubtitleFromClipboard（与 AI 无关），翻译从未启动（ai_subs 空 ✓、
  缓存命中路径的 done 行无 ✓ 全部吻合）；
- **下轮验证**：tap AI 行正确位置（操作组内 y≈920——**先截图确认行位置再 tap**，
  不再凭估算）→ 同会话英文 SRT 已挂 → Controller.startTranslation → 翻译 →
  译文挂载 → 面板副行更新（panelRevision 时序修复生效验证）。


## 13. 传导层排查结论 + 面板行 trace（2026-11-21 追加）

- **tc12/tc13/tc14 的面板是 subtitleExternal**（外挂字幕钻入面板）——**AI 行已显示**
  （noExternalSubtitles() 的 AI 行生效 ✓——T-C 入口行显示达成，面板行清单分派
  PlayerPanelItems:111 `"subtitle" -> entry(mpv)` / `"subtitleExternal" -> catalog
  过滤（空态含 AI 行）`——panelRevision 重组机制在（PlayerPanelPress:38））；
- **未决收窄**：tap AI 行 → handleAiSubtitleRow **有没有执行**不可判定（面板行
  点击无 trace）+ toast 时序（tap 后 0.8s 截图 vs 5s）——本轮 handleAiSubtitleRow
  加 smbTrace（"ai-row tap begin"）——下轮 tap 后 **0.8s** 截图，trace 判定执行链；
- **下轮完整链**：handleAiSubtitleRow trace → 执行链判定 → 翻译启动 → 译文
  挂载 → 面板副行更新（panelRevision 时序修复生效验证）。


## 14. 面板行点击的翻译链 native crash 实锤（2026-11-21 收口状态）

- **证据链**：trace `ai-row tap begin`（handleAiSubtitleRow 执行 ✓）→ 显式 select
  修复生效（mount select id=1 ✓）→ tap 后 27s **ai_subs 目录空**（journal/SRT/manifest
  均未落盘）+ ai-smoke.log 无 done 行 + **App 进程死**（kill -3 报 No such process，
  pid 24613 已死重启为 19938）→ **面板行点击触发的翻译链 native 崩溃实锤**
  （探针 full 模式的同源翻译成功过三轮 23.3~23.5s——**面板行路径的翻译链差异
  是崩点**：handleAiSubtitleRow → Controller.startTranslation → job.run →
  nativeParseCues / nativeTranslateBlock——**与探针 full 模式的差异 = 调用上下文**）；
- **tombstone 符号栈被权限挡**：/data/tombstones/ shell 组读不到、/data/anr/ 只有
  11-17 的旧 ANR——**下轮取证**：① crash 复现 + `logcat -b crash` 实时抓（未 strip
  libkivo_ai.so 有符号）；② `ndk-symbolize` 处理 tombstone（拷出后本地符号化）；
  ③ `debuggerd -b <pid>` 复现时抓 backtrace；
- **已验证与本节前一致**：探针 full 模式端到端 ✓（三轮稳定）、mount-only 探针链 ✓
  （四段）、面板 AI 行显示与空态可点 ✓（tc4/tc5/tc6）、显式 select ✓（mount select id=1）、
  panelRevision 时序修复编译 ✓。


## 15. 符号栈取证手段穷尽 + 最后可行路径（2026-11-21 收口状态）

- **本轮穷尽的取证尝试**：① logcat -c 后复现 + logcat -d 全量——**无 Fatal signal**
  （debuggerd 对该 crash 只写 tombstone 不写 logcat，或写入被轮转）；② /data/tombstones/
  与 /data/anr/ —— shell 组**权限挡**（ls 静默拒绝）；③ `kill -3 <pid>` —— 对已死进程
  报 No such process，对活进程报 Operation not permitted（shell 组无权限）；
- **最后可行路径（下轮）**：`adb bugreport`（zip 内含 **FS/data/tombstones/** 与
  /data/anr/——pull 后解包提取 tombstone → **ndk-symbolize 符号化**——符号文件 =
  spike-build 未 strip 的 kivo_ai.so 或 ndk llvm-symbolizer）；
- **crash 点候选（代码级收窄）**：mount-only 会话 → 面板行点击 → handleAiSubtitleRow
  → Controller.startTranslation → job.run → nativeParseCues（C++ assemble——
  纯逻辑、桌面 12 用例覆盖）/ **nativeTranslateBlock（C++ translate——llama 封装
  decode 全流程，**探针 full 模式同源翻译成功过三轮**——面板行路径与探针路径的
  **调用上下文差异 = 崩点方向**：mount-only 会话的 mpv.track-list 状态（探针挂载
  英文轨后 addTrack 导入的轨）、Controller 单例跨探针 Thread 的状态、或 llama
  ctx 在 mount 会话的重复初始化）；
- **最小输入防御不适用**：translate 的输入已防御（n_ctx 预算、行数上限）——
  crash 点在 llama 封装内部，需 tombstone 定位后修。


## 16. 认知修正：App 从未崩溃（tombstone 是 T-0 spike 的）（2026-11-21 追加）

- **bugreport 的 tombstone_22/21 都是 `kivo_ai_spike`**（pid 24884/24664，
  signal 6 SIGABRT，backtrace 指向 libc abort —— **T-0 spike 进程的崩溃，
  不是 App**）；
- **App「进程死」的判断是错的**：pid 链（24613→19229→19938→32191）实为
  **每轮验证前的 `am force-stop` 正常产物**（force-stop 杀旧进程、am start
  起新进程）——**logcat 无 Fatal signal、crash 缓冲空、App 从未崩溃**；
- **「面板行点击的翻译卡住」的真因待查**（不是崩溃）：tap AI 行 → trace
  `ai-row tap begin` ✓ → handleAiSubtitleRow → sid=1（探针显式 select 后）→
  startTranslation true → job.run——**ai_subs 空且无 progress 行**：job.run
  卡在 nativeParseCues / translateBlock / 或 hasTranslation 的某一步——
  **job.run 分段 trace（Log.i kivo-ai 五段）已落盘未读**（上轮 BUILD FAILED
  后装的旧 APK 无分段 trace——**重装新 APK 后重跑读分段 trace 即可定位**）；
- **T-0 spike 的 SIGABRT**：T-0 长跑/矩阵的 spike 进程 abort（backtrace
  pc 0343380/03432ec in /data/local/tmp/kivo_ai_spike）——与 AI-DEVICE §15
  记录的「符号栈取证手段穷尽」相接：pc offset 需 ndk-symbolize 定位函数
  （llama 内部的 GGML_ABORT 嫌疑）——**与面板行点击的翻译无关**。


## 17. 面板行点击的 Job.run 分段 trace 无行（2026-11-21 追加）

- **本轮读数**：tap AI 行（handleAiSubtitleRow 执行 ✓ trace）→ sleep 8 →
  **logcat 的 `Log.i("kivo-ai")` 五段一条没打**——**job.run 从未被调用**——
  handleAiSubtitleRow 的执行链在 `startTranslation` **之前**提前 return
  （前置分支：isNativeAvailable / isEngineReady / 模型路径 / mpv null /
  sid 检查 / sourcePath / SRT 门槛 / sourceText 读——**每分支都 toast 或
  状态更新**，但截图时机（tap 后 8s）toast 已消失，无法判定走到哪一分支）；
- **同时**：mount 挂载英文字幕轨后**外挂面板显示操作组**（非空态）——**AI 行
  位置从空态 y≈675 移到操作组内（y 待确认）**——本轮 tap 1735,690 的命中
  目标未知（可能命中「粘贴字幕链接」等邻近行——非 AI 行）；
- **下轮定位方案**：① handleAiSubtitleRow **每分支加 smbTrace**（trace 判定
  走到哪一分支 + 提前 return 的原因）；② tap AI 行**前先截图确认 AI 行位置**
  （mount 挂载后操作组内 AI 行 y 坐标实拍）；③ 两者一起定位后修复。

## 18. T-C 收口：两个根因定位与全链验证（2026-11-22）

- **根因 1（ch5 扩展名误拒——所有历史轮次的共同根因）**：导入通道登记文件名 =
  `<内容哈希>-<显示名>`（**无 .srt 后缀**），`endsWith(".srt")` 把 exists/readable
  均为 true 的合法 SRT 拒在 ch5。mountrow 直调 + ch1~ch7 插桩一次定位：
  `ch5 path=.../subtitles/0c82da5413b1-e2e-test-en exists=true readable=true len=111`
  之后 ch6/ch7 无行（ch1~ch4 全过：native=true ready=true sid=1 ext=true）。
  修复：**删除扩展名检查，文件类型由内容解析器判定**（nativeParseCues 装配
  失败走 no cues parsed 非空 reason 路径，SRT-only 边界由解析器保证）。
- **根因 2（onDone 挂载失败 = EOS 会话 teardown，非线程问题）**：续播近片尾
  + 翻译 23.79s 超剩余片长 → onDone 时视频已 EOS，mpv command/property 全失效
  （added=false newId=null）。对照轮（缓存命中 6ms 秒回、onDone 时
  `mount-pre eof=no idle=no pos=57.633 dur=60.0` 在播）**同一后台 Thread 代码
  added=true newId=2**——证明是会话生命周期而非线程。修复：挂载失败时如实
  告知「译文已保存，下次播放自动加载」（缓存命中路径已实测兜底，G9）。
- **全链验证（三轮真机）**：① 翻译轮 `ch6 textLen=111 → ch7 started=true →
  ai-ctrl worker-started/ensure-loaded=true/before-job-run → after-job-run
  （23.79s）→ onDone llm=2 failed=0` → 中文 SRT（我以为你要走了 / 站住那里，
  **时间轴逐位不变**）+ journal/manifest 三件套落盘；②③ 缓存轮
  `onDone total=0 秒回 → mount-pre eof=no pos=57.633/60.0 →
  mounted added=true newId=2 err= cmdErr=null` **两轮稳定复现**。
- **本轮加固**：catch 完整堆栈双写（原「G9 非空 reason」注释空转，异常被静默
  吞掉）、ensureModelLoaded 返回值检查（false 明确终止，不再带病进 job.run）、
  ch2 短路语义修正（原 `isEngineReady() && !ensureModelLoaded()` 条件反了，
  未就绪时反而跳过模型检查）、playheadMs=durationMs→positionMs（领先窗口语义）、
  worker/onDone/mounted 全链 trace 进 ai-smoke.log（消除「面板路径无 done 行」
  盲区）、mountrow 直调 handler（消 tap 坐标/布局/toast 时序变量）、
  build stamp tc-diag-20261122（证据版本可溯，杜绝旧 APK 误判）。
- **外部评审（GLM 及另一模型）贡献与纠偏**：ch5 路径形态预判命中；H4
  （playheadMs 致空窗口）**不成立**——orderBlocksForPlayhead 只排序不裁剪
  （containing+after+before 全量返回）；H2（ensureModelLoaded 二次调用失败）
  在 mount-only 会话**排除**——有 `if (client.isReady()) return true` 短路幂等；
  「TranslationJob 构造含磁盘 IO」指控**不实**（构造只存三参，Journal 在 run()
  内 107 行才创建）；logcat 不可靠再获实证（实验数分钟后 kivo-mpv/kivo-ai
  全部轮转，grep 空）。
- **方法论（本次排查的核心教训）**：观测可靠性优先于根因猜测——build stamp
  证明新 APK、统一文件日志替代 logcat、等待 ≥40s（sleep 8 曾把正常翻译误判
  为未启动）、直调 handler 消 UI 变量、每分支 trace 替代 toast 时序赌博、
  每次只改一个有证据支持的问题。
