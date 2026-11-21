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
