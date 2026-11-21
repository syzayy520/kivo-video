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
