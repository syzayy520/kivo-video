# D376 冷启动端到端冒烟：发现并修复「继续观看」整行消失（真回归）

## 一、怎么发现的

D375 终局回归只验了「编译 + 单测 + 门禁」，没有真机端到端。本轮补冒烟：`am force-stop` → `am start -W` →
抓 net-diag → uiautomator dump → 截图。

**首轮冷启就抓到**：首页 hero 下方直接是「电影」NAS 行，**「继续观看」整行不见了**。
对照 `docs/evidence/ui-shot/verify-365-home-nomovie.png`（11-18 19:41，同一台 K40）——那时该行正常显示
（A Beautiful Mind 看到 8:32、读书会 27:42）。**确认是真回归，不是设计如此。**

再触发一次 `am start`（走 onResume）→ 该行又出现了。**冷启动首次不显示、二次 onResume 才出现**，指向
首次刷新线程被什么卡住。

## 二、根因（日志实证，不是猜）

先加 `[home]` 分段日志，冷启复现：

```
[home] continue 进入
[home] smb=1
（此后无任何 [home] 日志，持续 >22s）
```

同一窗口内 `[emby-hero] 完成 n=6` 出现了两次——说明网络请求本身是成功的，卡在**合并/等待**环节。

读 `EmbyHomeResume.load()` 找到病灶：

```kotlin
records.map { pool.submit(Callable { fetchOne(...) }) }
    .flatMap { future -> runCatching { future.get() }.getOrElse { emptyList() } }   // ← 无超时
```

`future.get()` **没有超时参数**。三个 Emby 源里只要有一个在 `client.login()` 阶段 hang
（`EmbyClient.conn` 是 connectTimeout 10s + readTimeout 20s，且 DNS/TCP 层还可能更久），
`flatMap` 就永远不返回 → `refreshContinue` 线程永久阻塞 → `homeEntries` 永不赋值 →
`ContinueRow(entries.isEmpty()) → return` → **整行消失**。

**它违反了自己类注释里的承诺**：「失败一律静默降级为空——首页不能因某个源不可达而报错或白屏」。
实际行为是"一个源不可达 → 整行连坐，且零日志线索"。

### 归因（诚实）

`git log -S "future.get()"` → 引入于 **`9ecb5df4`「首页 Emby 续播多源并发 + 会话复用(3.7s→2.1s)」**。
即这不是老代码，而是**性能优化为了并发提速引入的回归**：串行改多线程池后，忘了给等待加边界。
D373 冷启动计时段没抓到，因为它只量了 TotalTime 和"数据就绪日志"，没验证"该渲染的行到底渲没渲"。

## 三、修复（两处，均在铁律限内）

### 1. `EmbyHomeResume.load()`：先到先显示 + 共享总预算

```kotlin
val ecs = ExecutorCompletionService<Pair<String, List<HomeEntry>>>(pool)
records.forEach { rec -> ecs.submit(Callable { fetchOne(rec, limitPerSource) }) }

val deadline = System.currentTimeMillis() + FETCH_TIMEOUT_MS   // 6s，全部源共享
for (i in records.indices) {
    val remain = deadline - System.currentTimeMillis()
    if (remain <= 0) break
    val done = ecs.poll(remain, TimeUnit.MILLISECONDS) ?: break   // ← 有界
    val (name, got) = runCatching { done.get() }.getOrNull() ?: ("" to emptyList())
    if (got.isEmpty()) continue
    out += got
    onPartial?.invoke(out.toList())                                // 每源到达即回调
}
pool.shutdownNow()   // 不等卡死线程
```

三个要点：
- **`poll(timeout)` 有界**：坏源占住自己的线程，但不阻塞主流程；
- **共享总预算**而非每源串行各付一次超时（否则 3 个坏源就是 18s）；
- **`onPartial` 先到先显示**：一个源回来就刷一次 UI，不攒到全部完成。

### 2. `refreshContinue()`：SMB 先出 + 增量刷新 + 必须留痕

- SMB 结果一到就先写 `homeEntries`（不等 Emby 网络）；
- Emby 每次 `onPartial` 就 `runOnUiThread` 合并刷新；
- 整个 `Thread` 补 `catch (Throwable)` + `[home]` 日志——**静默早退不再可能**。

## 四、实测（K40 b6e950ee，3 轮冷启）

| 轮次 | TotalTime | 继续观看可见 | 首次增量耗时 |
|---|---|---|---|
| 1 | 696ms | ✅ | 1582ms |
| 2 | 675ms | ✅ | 925ms |
| 3 | 670ms | ✅ | **828ms** |

第 3 轮完整时序（说明先到先显示的价值）：

```
[home] continue 进入
[home] smb=1                                           +21ms
[home] emby 源数=3 地址=bangumi / lovehd / hohai
[home] 源完成 nl2.lovehd.de n=6                        +828ms   → 增量 homeEntries=7  ← 用户此时已看到
[home] 源完成 emby.bangumi.ca n=6                      +1285ms  → 增量 homeEntries=10
[home] 源完成 emby.hohai.eu.org n=1                    +1798ms  → emby 完成 成功源=3/3 条=13
```

同一台机器上，**修复前**那次 hohai 卡满 6s 超时；**修复后**这轮 hohai 只用 1.8s 就回了。
源站抖动不可预测 ⇒ 先到先显示是唯一正确解，不能靠"源都很快"的假设。

视觉证据：`docs/evidence/ui-shot/smoke376-fixed2.png` —— 继续观看行含
A Beautiful Mind（看到 8:32）与 空枪（看到 1:34:17），无灰块。

## 五、回归验证

| 项 | 结果 |
|---|---|
| `assembleDebug` | BUILD SUCCESSFUL |
| `testDebugUnitTest --rerun-tasks` | 115 例 / 0 失败 / 0 错误 |
| `tools/ui_gate.py` | PASS（无回归；MainActivityHomeFeed 159 行、EmbyHomeResume 113 行，均 ≤300） |
| 真机 3 轮冷启 | 3/3 立即显示，0.83–1.58s |

## 六、诚实边界

- **未做**：坏源冷却/熔断。hohai 若长期卡死，每次冷启仍会白占一个线程至 6s（**不影响显示**，因为已先到先显示）。
  属可选优化，不阻塞；真要做需要跨进程记住失败源，收益/复杂度比不划算。
- **未做**：SenPlayer 同场景对照不可得（同 D373，无源码级启动时序）。
- 冒烟只覆盖「首页冷启动 → 继续观看」这条链；详情页/播放链路本轮未重跑，D375 的编译+单测仍成立。
- 截图 `smoke376-home.png`（修复前，无继续观看）与 `smoke376-fixed2.png`（修复后，有）成对归档，可自证。

## 七、教训（已写入 MEMORY）

1. **`Future.get()` 永远带超时**——并发优化的默认代价就是"一个坏源连坐全部"。
2. **性能审计不能只量"快不快"，还要验"该显示的东西显示没有"**：D373 量出 TotalTime 690ms 很漂亮，
   但一个整行悄悄消失，指标完全反映不出来。
3. **try/finally 无 catch 的线程 = 取证黑洞**：必须成对加 `catch (Throwable)` + 日志。
