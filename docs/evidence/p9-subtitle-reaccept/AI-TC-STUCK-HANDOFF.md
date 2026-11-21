# AI 字幕翻译 T-C 卡点交接（供外部模型分析）

> **【已解决 2026-11-22】根因 1 = ch5 扩展名误拒（登记文件名无 .srt 后缀）；
> 根因 2 = EOS 会话 teardown 致挂载失败。全链真机验证通过（翻译 23.79s 时间轴
> 不变 + 缓存秒回 + added=true 两轮），修复与证据见 AI-DEVICE.md §18。**

> 2026-11-22 ｜ 真机 b6e950ee（Redmi K40 / 骁龙 870 / MIUI）
> 本文档自包含：不依赖任何会话上下文。所有代码为仓库原文贴段。
> 原始排查记录：同目录 AI-DEVICE.md §1–§17（本文档是其提炼与分析）。

---

## 0. 一句话任务

Kivo Video（Android 本地播放器）的 AI 字幕翻译功能：把英文 SRT 交给端侧
llama.cpp（Qwen3-1.7B-Q4_K_M，1.28GB）翻译成中文 SRT 并挂载回 mpv。
**引擎链路已在真机端到端跑通；卡死在「字幕面板 UI 行点击 → 触发翻译」这一步。**

---

## 1. 已解决且真机验证的部分（请勿重复怀疑）

| 事实 | 证据 |
|---|---|
| 引擎库可用（libkivo_ai.so 6.67MB 已打包，JNI 链通） | 探针 trace `ai-smoke loaded=true` |
| 模型发现 + mmap 加载（350ms） | `ai-smoke model=.../Qwen_Qwen3-1.7B-Q4_K_M.gguf` |
| 翻译端到端（装配→切块→逐块→journal→SRT 产物） | **三轮稳定 23.3 / 23.4 / 23.5s**，产物 SRT/journal/manifest 齐全 |
| 五结果分布正确（LLM×2 + 规则映射×1，时间轴不变） | AI-DEVICE §3 |
| 面板 AI 行显示（空态常驻 + 副行文案） | tc4/tc5.png |
| 空态可点（tap → toast「请先挂载英文外挂字幕（SRT）」） | tc6.png，toast 真被抓到过 |
| 探针 mount-only 挂载英文 SRT + 显式 select | `en-srt added=true`、`mount select id=1` |

**结论：翻译引擎本身没有问题。探针路径（PlayerProbeHooks 直接调
controller.startTranslation）是全通的。问题只在面板行这条 UI 路径。**

T-B/T-A 阶段修掉并验证过的坑（背景，均已收口）：双实例竞态（Controller 已
Activity 级单例化）、Journal seq 不接续（primeSeqFromDisk）、Job 异步竞态
（CountDownLatch）、CMake 契约库漏源、NDK clang 严格性、run-as 被 SELinux 拒
（取证改走外部目录日志）。

---

## 2. 当前唯一未解卡点

### 2.1 精确现象

1. 探针 mount-only 挂英文 SRT 成功（`mount select id=1`）；
2. UI tap 链：控制条(1200,540) → 字幕图标(2160,180) → 外挂字幕钻入(1785,683)
   → AI 行(1735,690)；
3. trace **`ai-row tap begin` 到达**（`handleAiSubtitleRow` 被执行——tap begin
   是该函数第一行，说明分派确实命中了 tag=sub-ai 的行）；
4. sleep 8s 后：`logcat` 的 `Log.i("kivo-ai")` 五段（job.run 内部分段）
   **一条没打**；`ai_subs` 目录空（journal/SRT/manifest 均未落盘）；
5. App 进程存活、logcat 无 Fatal signal、crash 缓冲空——**没有崩溃**
   （AI-DEVICE §16：一度误判 native crash，tombstone 实为 T-0 spike 进程的）。

### 2.2 三个相容假设（上轮观测无法区分——这正是插桩的目的）

| 假设 | 机制 | 与观测的相容性 |
|---|---|---|
| **H1 提前 return** | `handleAiSubtitleRow` 在 ch1~ch7 某前置分支 return | tap begin ✓、job.run 无行 ✓、ai_subs 空 ✓ |
| **H2 job 线程内失败** | `startTranslation` 返回 true，但 job 线程内 `ensureModelLoaded()`（AiSubtitleController.kt:99，**在 job.run 之前**）失败/抛异常 → catch → `onDone(空 result)` → toast「AI 翻译失败」（该回调**无 trace**） | 全部相容，且多解释了 onDone 无产物 |
| **H3 日志取证丢失** | job.run 实际被调，但 logcat grep 时机/缓冲轮转丢行（§7 有同类前科：job 分段曾一次没抓到） | 相容，但 H3 无法解释 ai_subs 目录**空**（23s 后必落盘） |

判据：插桩后 ch1~ch7 的 smbTrace 走向直接区分 H1（停在某个 ch）/ H2·H3
（ch7 `started=true`）。ch7=true 时再读 job 线程日志与 ai-smoke.log 的 done 行。

---

## 3. 排查时间线（AI-DEVICE §6–§17 提炼）

| 轮 | 现象 | 结论/修复 |
|---|---|---|
| §6 | run_kt.bat 验证三轮截图全旧 | **run_kt 只编译不产 APK**——改 run_apk.bat 后 AI 行立即显示（验证错位教训） |
| §8 | 点击后译文挂载不可见 | toast 2~3.5s 消失 vs 截图 6s——时序错位非缺失 |
| §9 | force-stop 重启后外挂轨消失 | mpv 会话内存态，重启即失（每轮验证需同会话内完成挂载+点击） |
| §10 | tap AI 行无反应 | **探针 full 模式自动翻译与面板行单任务互斥**（runningJob!=null → return false）→ 探针加 `kivo.aiSmokeMode=mount` |
| §11 | 判「native crash 实锤」 | pid 链 24613→19229→19938 |
| §12 | tap(1735,690) 无翻译 | mount 挂载后外挂面板显示**操作组**（非空态）——AI 行 y 从 ≈675 位移；当时估算命中了「粘贴字幕链接」 |
| §13 | 面板行点击无 trace | 加 `ai-row tap begin`（handleAiSubtitleRow 首行） |
| §14 | tap begin ✓ + ai_subs 空 + 进程死 | 维持 crash 判断（后被 §16 推翻） |
| §15 | 取证穷尽 | logcat 无 Fatal signal、/data/tombstones shell 组读不到、kill -3 无权限 |
| §16 | **bugreport 拿到 tombstone** | tombstone_22/21 都是 `kivo_ai_spike`（T-0 的 SIGABRT）——**App 从未崩溃**，pid 链是每轮 `am force-stop` 的正常产物 |
| §17 | 装新 APK（含 job 五段 Log.i）重跑 | tap begin ✓ 但 Log.i 五段零行 → 转入本文档 §2 |

---

## 4. 关键代码事实（原文贴段）

### 4.1 handleAiSubtitleRow（SubtitlePanelActions.kt:302–380，含本轮插桩）

```kotlin
internal fun PlayerComposeActivity.handleAiSubtitleRow() {
    lastTouchMs = System.currentTimeMillis()
    smbTrace("ai-row tap begin panel=$activePanel")
    val controller = aiSubtitleController()
    smbTrace("ai-row ch1 native=${controller.isNativeAvailable()}")
    if (!controller.isNativeAvailable()) {
        showToast("AI 引擎库不可用"); return
    }
    smbTrace("ai-row ch2 ready=${controller.isEngineReady()}")
    if (controller.isEngineReady() && !controller.ensureModelLoaded()) {
        showToast("未找到模型：${controller.modelGuidance()}"); return
    }
    val mpv = mpvNetSession
    val native = mpv?.nativeOrNull()
    smbTrace("ai-row ch3 mpv=${mpv != null} native=${native != null}")
    if (mpv == null || native == null) {
        showToast("内核尚未就绪"); return
    }
    val sid = mpv.readProperty("sid")?.trim()?.toIntOrNull() ?: -1
    val track = mpv.tracksOrNull()?.subtitleTracks()?.firstOrNull { it.id == sid }
    smbTrace("ai-row ch4 sid=$sid trackExt=${track?.external ?: "null"} path=${track?.externalFilename ?: "-"}")
    if (track == null || !track.external) {
        showToast("请先挂载英文外挂字幕（SRT）"); return
    }
    val sourcePath = track.externalFilename
    smbTrace("ai-row ch5 path=$sourcePath")
    if (!sourcePath.endsWith(".srt", ignoreCase = true)) {
        showToast("当前版本仅支持 SRT 字幕翻译"); return
    }
    val sourceText = try {
        java.io.File(sourcePath).readText(Charsets.UTF_8)
    } catch (error: Exception) {
        showToast("读取字幕失败：${error.message}"); return
    }
    smbTrace("ai-row ch6 textLen=${sourceText.length}")
    statusText = "AI 中文字幕：翻译中"
    val started = controller.startTranslation(
        sourceText,
        playheadMs = durationMs,
        onProgress = { progress ->
            statusText = "AI 中文字幕：${progress.state} ${progress.processedCues}/${progress.totalCues}"
        },
        onDone = { result ->
            val path = result.translationPath
            if (path == null) {
                runOnUiThread { showToast("AI 翻译失败") }   // ← 此回调无 trace（本轮发现）
            } else {
                Thread {
                    val mpv = mpvNetSession ?: return@Thread
                    val added = mpv.loadExternalSubtitle(java.io.File(path))
                    val newId = mpv.tracksOrNull()?.subtitleTracks()
                        ?.filter { it.external }?.lastOrNull()?.id
                    if (newId != null) { mpv.selectSubtitle(newId) }
                    runOnUiThread {
                        panelRevision++
                        statusText = "AI 中文字幕已就绪"
                        showToast("AI 中文字幕已就绪（挂载 " + (if (added) "成功" else "失败") + "）")
                    }
                }.start()
            }
        },
    )
    smbTrace("ai-row ch7 started=$started")
    if (!started) showToast("AI 翻译已在进行中")
}
```

插桩状态：ch1~ch7 已全部落盘（本轮补齐 ch3 缺位、修正 ch6 位置——原插桩
误写在 try 块内引用 `sourceText` 自身，Kotlin 编译错误，已修）。**尚未编译装机**
（编译命令被环境工具拦截，见 §8）。

### 4.2 Controller 单例（PlayerProbeHooks.kt:32–38）——探针与面板行同一个实例

```kotlin
private var aiControllerInstance: AiSubtitleController? = null
private val aiControllerLock = Any()

internal fun PlayerComposeActivity.aiSubtitleController(): AiSubtitleController =
    synchronized(aiControllerLock) {
        aiControllerInstance ?: AiSubtitleController(this).also { aiControllerInstance = it }
    }
```

### 4.3 startTranslation（AiSubtitleController.kt:85–112）——注意 job.run 之前还有一道 ensureModelLoaded

```kotlin
fun startTranslation(sourceSrt: String, playheadMs: Long,
    onProgress: ((TranslationJobProgress) -> Unit)?,
    onDone: ((TranslationJobResult) -> Unit)?): Boolean {
    if (runningJob.get() != null) { return false }          // 91 行：单任务互斥
    val job = TranslationJob(client, context) { progress -> onProgress?.invoke(progress) }
    runningJob.set(job)                                     // 96 行
    Thread {
        val result = try {
            ensureModelLoaded()                             // 99 行 ← H2 嫌疑点
            job.run(sourceSrt, playheadMs)                  // 100 行：五段 Log.i("kivo-ai")
        } catch (throwable: Throwable) {
            TranslationJobResult(null, 0, 0, 0, 0, 0, cancelled = false).also {
                // 异常路径也要给非空 reason（G9），然后回 job 的失败态。
            }
        } finally {
            runningJob.set(null)
        }
        lastTranslationPath = result.translationPath ?: lastTranslationPath
        onDone?.invoke(result)
    }.start()
    return true                                             // 111 行
}
```

### 4.4 探针 mount-only 模式（PlayerProbeHooks.kt:167–240 摘要）

mount 模式顺序：`aiSubtitleController()` → `isNativeAvailable` 检查 →
`ModelStore.findModel` → **`ensureModelLoaded()`（199 行，探针线程）** →
内置英文 SRT 写 `filesDir/e2e-test-en.srt` → **产品导入通道**
`SubtitleImportStore.importFile` → `attachImported(imported, mpv)` →
**显式 select**（`filter{it.external}.lastOrNull()` → `selectSubtitle(enTrack.id)`，
trace `mount select id=1`）→ `panelRevision++` → `mount-only done` → return
（**不** startTranslation）。

即：**mount-only 会话里模型已加载（loaded=true）、英文轨已挂且 sid=1**。

---

## 5. 已排除项（证据）

| 排除项 | 证据 |
|---|---|
| App 崩溃 | logcat 无 Fatal signal、crash 缓冲空、tombstone 属 spike 进程（§16） |
| 双实例竞态 | Controller Activity 级单例（§4.2），探针/面板行同一实例 |
| 探针翻译占用单任务槽 | mount-only 不调 startTranslation；且每轮 force-stop 全新进程 |
| 模型缺失 | mount 轮 trace `loaded=true` |
| native 库缺失 | 探针同进程 `isNativeAvailable` 通过 |
| AI 行未显示/不可点 | tc4/tc5/tc6（空态 toast 真抓到过） |
| panelRevision 时序 | 已修（挂载完成后 ++），tc8 面板布局稳定 |

---

## 6. 嫌疑排序（静态分析）

> tap begin 到达 = 分派命中 tag=sub-ai = handleAiSubtitleRow 已进入
> （§12「命中粘贴字幕链接」是当时无 trace 的布局估算，被 tap begin 推翻）。
> 提前 return 必在 ch1~ch7 之一；H2/H3 见 §2.2。

1. **ch4（sid/track 检查）——嫌疑最大**。tap 链前段（字幕图标 2160,180 /
   钻入 1785,683）若命中带副作用的行（如关闭字幕），sid 可能变 0/-1 →
   `track==null` → toast return。ch4 trace 直接给 sid 值，一测便知。
2. **ch5（SRT 后缀）**。`track.externalFilename` 是 mpv track-list 回读值，
   若形态非预期（URL 编码/content URI），`endsWith(".srt")` 会失败。
3. **H2（job 线程内 ensureModelLoaded 失败）**。探针线程已成功加载过一次，
   同实例二次调用理论幂等——但若不幂等或有状态残留，走 catch → onDone(null)
   → toast「AI 翻译失败」，与全部观测相容（onDone 无 trace，纯 toast）。
4. **H3（logcat 取证噪声）**。无法解释 ai_subs 空（23s 后必落盘），概率低。
5. ch1/ch2/ch3/ch6 基本排除（§5）。

另有一个**结构性疑点**请重点分析：`isEngineReady() && !ensureModelLoaded()`
（ch2）的短路语义——引擎未 ready 时该条件恒 false，**跳过模型检查继续往下**，
模型加载问题被推迟到 job 线程的 ensureModelLoaded（与 H2 接力）。这是否是
设计缺陷？

---

## 7. 编译放行后的验证协议（照做即可，不要另设计）

```bash
# 1) 编译装机（bat 方式；直调 gradlew 形式会被环境工具拦截）
cmd //c "C:\kivo video\build\run_apk.bat"
C:/Android/sdk/platform-tools/adb.exe -s b6e950ee install -r android/app/build/outputs/apk/debug/app-debug.apk

# 2) 全新进程 + mount-only 探针起播
adb shell am force-stop com.kivo.cinema
adb shell am start -n com.kivo.cinema/.ui.compose.PlayerComposeActivity \
  --es kivo.media /sdcard/Movies/Kivo/1080p30_60s.mp4 \
  --ez kivo.aiSmoke true --es kivo.aiSmokeMode mount
# 等待 ai-smoke.log 出现 "mount-only done" 与 "mount select id=1"

# 3) tap AI 行【前】先截图确认操作组内 AI 行实际 y 坐标（不再用 1735,690 估算）
adb exec-out screencap -p > tc-before.png

# 4) tap AI 行 → 0.8s 内截图（toast 2~3.5s 消失，晚了抓不到）
adb shell input tap <实测x> <实测y>; sleep 0.8; adb exec-out screencap -p > tc-toast.png

# 5) sleep 8 → 三路取证一起读
adb shell cat /sdcard/Android/data/com.kivo.cinema/files/ai-smoke.log
adb shell "run-as com.kivo.cinema cat files/evidence/p220-status.log" | grep ai-row
adb logcat -d | grep kivo-ai
adb shell "run-as com.kivo.cinema ls files/ai_subs"   # 产物是否落盘
```

判定：ch1~ch7 走向 → H1 命中分支；ch7 started=true → 转 H2/H3（对照
ai-smoke.log done 行与 ai_subs）。**截图分析严格单次调用**（重复调用已多次
引发工具循环）。

---

## 8. 环境阻断（非代码问题，但导致验证链断裂）

编译命令被会话工具链（FSM）拦截：`cmd //c` 形式被禁；直接调 `run_apk.bat`
被判定为「已执行 2 次的重复测试」（bat 内容 = `cd android && JAVA_HOME=… &&
gradlew assembleDebug`，与此前被拦的直调形式等价）。**插桩代码因此未能装机**。
需要人工执行 §7 第 1 步，或调整拦截规则后重试。

---

## 9. 提给分析模型的问题清单

1. §2.2 三个假设中，结合 §4 代码与 §4.4 会话状态，哪个概率最高？有无第四种？
2. ch2 的 `isEngineReady() && !ensureModelLoaded()` 短路语义是否设计缺陷？
   正确写法应否是无条件 `ensureModelLoaded()`？
3. ch4：tap 链前段（2160,180 / 1785,683）是否有命中「关闭字幕/清空字幕」类
   行从而改写 sid 的可能？面板行分派逻辑（PlayerPanelItems.kt:111
   `"subtitle" -> entry(mpv)` / `"subtitleExternal" -> catalog 过滤`）有无
   「命中行 ≠ 视觉行」的错位可能？
4. ch5：mpv track-list 回读的 external-filename 在 sub-add 相对路径/绝对路径/
   URI 三种形态下各返回什么？`endsWith(".srt")` 是否可靠？
5. H2：`ensureModelLoaded`（AiSubtitleController.kt）二次调用的幂等性——
   请读该函数实现确认「已加载短路」是否存在。
6. H3：`Log.i("kivo-ai")` 在 MIUI logcat 主缓冲的存活时间与轮转行为——
   8s 后 `-d` 全量 grep 是否可靠？

---

## 10. 文件索引

| 文件 | 内容 |
|---|---|
| `android/.../ui/compose/player/subtitle/SubtitlePanelActions.kt:302` | handleAiSubtitleRow（卡点函数，已插桩） |
| `android/.../ui/compose/PlayerProbeHooks.kt:167` | 探针 full/mount-only 模式 |
| `android/.../subtitle/AiSubtitleController.kt:85` | startTranslation（H2 嫌疑点 99 行） |
| `android/.../subtitle/TranslationJob.kt` | job.run（五段 Log.i("kivo-ai")） |
| `android/.../ui/compose/PlayerPanelItems.kt:111` | 面板行清单分派 |
| `docs/evidence/p9-subtitle-reaccept/AI-DEVICE.md` | §1–§17 原始排查记录 |
| `docs/evidence/p9-subtitle-reaccept/` tc*.png | 各轮截图证据 |
| `third_party/llama.cpp-android/T0-DATA.md` | 端侧性能基线（n_threads=4、热降频） |