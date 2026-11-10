# 250 · SMB 读取链路健壮性收口（设计稿 v1）

> 上一卡：243（SMB 重建，已收口）/ 247（顺序预读，已收口）。
> 本卡来源：2026-11-10 对 SMB 全链路（Kotlin → JNI → libsmb2 2.3.0 源码）的逐层评审。
> 状态：**规划设计 / 家族树已出，等闸门批准，未动实现代码。**

---

## 零、结论先行

243 把 SMB 从"不能播"做到"能播"，247 把软解从"周期性顿帧"做到"19 个窗口全部 ≤42.7 ms"。
两条卡都成立。但**这两次优化都停在"参数与补丁"层面，没有回到读取链路的结构上**，于是留下三类问题：

| 类 | 一句话 | 用户怎么感知 |
| --- | --- | --- |
| 断链 | 全工程没有网络状态感知，断网只能等 PDU 超时 15 s | 断网后卡 16 s 才提示（设计 AC8 写的是 5 s） |
| 结构 | `SmbByteSource` 664 行 6 职责，窗口/预读/重连/统计/契约/常量挤在一个类 | 预读线程到文件尾退出后永不复活；改任何一处都在动同一个文件 |
| 编码 | JNI 用 `NewStringUTF` / `GetStringUTFChars`（modified UTF-8）承载 SMB 的标准 UTF-8 | 文件名含 emoji 打不开；含换行的目录名让整目录列举失败 |

本卡只做一件事：**把 `SmbByteSource` 拆成 6 个单职责单元，并把断链、超时、编码这三处从"能跑"改成"对"**。
**不做**异步流水线（那是 251），**不做** Kerberos/DFS（产品决策未定）。

一句话架构：

> **一个读取源 = 一个参数档 + 一个块窗口 + 一个预读环 + 一条可重连的读通道 + 一份画像；
> 契约层只负责编排顺序，不负责实现任何一件。**

---

## 一、现状诊断（按代码定位，不按感觉）

行号均为**改动前**。

### 1.1 P0 · 用户一定撞上

**P0-1 · 断链感知为零，断网必然卡 15–16 s**

| 事实 | 位置 |
| --- | --- |
| 读超时 15 s | `jni_smb2_bridge.cxx:46` `smb2_set_timeout(ctx, 15)` |
| fd 层耐心 20 s，退避 250/500/1000 ms | `net/RangeFd.kt:158`、`:135` |
| 全工程 0 处 `ConnectivityManager` / `NetworkCallback` | `grep -rn` 已确认 |

推演（不是猜测）：断网时 TCP 不报错，`smb2_pread` 阻塞到 PDU 超时 **15 s** 才返回；
`RangeFd` 拿到异常后 `sleep 250 ms` 重试；第二次再等 15 s，累计已超 20 s 耐心 → 判死。
**所以 20 s 预算只够一次重试，用户看到的是"卡 15 s 然后报错"。**

243 的 AC8 要求"断网 → 5 s 内 UI 出现连接中断"，实测 16 s（`p243/smb-rebuild/status.md` §S3 补测
`P243 RESUME 停 16002 ms`），收口时改用另一套判据（stall/recovered/alive）通过。
**这不是造假，是判据漂移**——但账要记在这里。

**P0-2 · 硬解路径关掉了预读**

`PlayerComposeActivity.kt:550` `opened.setPrefetchEnabled(false)`。
命中率 91–95%，剩下 5–9% 是 87–140 ms 的同步读（`:546-549` 注释自述）。
硬解是 4K 唯一实际路径，所以**默认路径没有 read-ahead**。

根因不在"预读有害"，在"预读用了第二条 TCP 连接"：`p247/smb-prefetch-ring.md` §二.2 记录
"两条连接同时拉同一台服务器，1 MiB 单次从 92 ms 恶化到 543 ms"。这是服务端对同 session /
同文件的 IO 串行化，**加连接只会把 NAS 打得更抖**。本卡不解决它（归 251），但要把它从
"按解码路径开关"这种补丁形态里分离出来，让 251 只需换一个实现。

**P0-3 · 打开即分配 12 MiB，且只按常量推导**

`SmbByteSource.kt:43-45` 构造即 `Array(6) { ByteArray(2 MiB) }`；`:621` 预读环再 6 MiB。
视频 + 软解音频各一路 → **36 MiB**。
`p247` §五 记的是"7 MiB × 2 = 14 MiB"，与代码实际差 2.5 倍。

后果不只是内存：`FfmpegRouting.probe(range)` 会先开一个源只读文件头，**探测也要吃 12 MiB**。

### 1.2 P1 · 确定是 bug

| # | 问题 | 定位 | 为什么确定 |
| --- | --- | --- | --- |
| P1-1 | **IPv6 假支持** | `SmbAddress.kt:56-65` 正确剥离 `[ ]`；`jni_smb2_bridge.cxx:76` 拼 `h + ":" + port` | libsmb2 `socket.c:1843` 只在 `host[0]=='['` 时按 IPv6 解析，否则按第一个 `:` 分割 → `::1:445` 解析失败 |
| P1-2 | **预读线程 EOF 退出后永不复活** | `SmbByteSource.kt:348` `return`；`:293` `if (pfThread == null)` | 线程退出但字段非 null、`pfStop` 不复位 → 播到结尾再拖回中间，预读永久失效 |
| P1-3 | **`setPrefetchEnabled(false)` 不可逆** | `:429-439` | 置 `pfStop = true` 后无任何路径复位 |
| P1-4 | **JNI 读路径持数组指针跨阻塞调用** | `jni_smb2_bridge.cxx:170-174` `GetByteArrayElements` + `Release(...,0)` | 大数组下等于全量 memcpy + 长时 pin；JNI 规范里跨可能长时间阻塞的调用持数组指针是反模式 |
| P1-5 | **JSON 转义不全** | `jni_smb2_bridge.cxx:105-108`、`:214-217` | 只转义 `"` `\`；`\n` `\t` 及 `<0x20` 直接进 JSON → `JSONArray()` 抛异常 → 报成"无法读取文件夹" |
| P1-6 | **modified UTF-8 ↔ 标准 UTF-8 混用** | `jni_string_convert.hpp:16`（`GetStringUTFChars`）、`:27`（`NewStringUTF`） | SMB 文件名是标准 UTF-8（4 字节 emoji 合法）；JNI 这对函数走 modified UTF-8（CESU-8 代理对）。**两个方向都错**：从 Kotlin 传路径给 libsmb2 会得到 CESU-8 字节；从 libsmb2 传文件名给 Kotlin 在 CheckJNI 下会 abort |
| P1-7 | **"多 fd 互不阻塞"与实现矛盾** | `net/RangeFd.kt:23` 注释 vs `SmbByteSource.kt:137` `synchronized(lock)` | 锁覆盖整段含网络读（最长 15 s），两路 fd 全串行 |
| P1-8 | 超时归因污染 | `sync.c:121` 超时由 `smb2_timeout_pdus` 触发、无回复报文 | `smb2_get_nterror` 保留上一次值 → 若该 context 之前失败过，超时被译成那个旧错误 |

### 1.3 P2 · 工程

| # | 问题 |
| --- | --- |
| P2-1 | 无 `src/test`，`SmbAddress.parse`（12 例）、`SmbError`（4 场景）全跑 instrumented，反馈周期 40 s+ |
| P2-2 | `build/libsmb2-android/arm64/lib/libsmb2.a` 在 `.gitignore` 的 `build/` 下，不入库；`AGENTS.md` §0.3「换机器必补的三样」未列此项 → clone 后**无法直接出 APK** |
| P2-3 | 参数散落：`BLOCK_BYTES`/`BLOCK_COUNT`/`PF_CAPACITY`/`PF_CHUNK`/`PF_WAIT_NS`/`smb2_set_timeout(15)` 分布在两个语言三个文件，没有一张表能一眼看完 |

---

## 二、为什么选这条解法（行业对照）

不引用未核实的产品实现，只讲形态：

| 问题 | 通行形态 | 本卡对应 |
| --- | --- | --- |
| 读失败该不该重试 | 把错误分成"可重试 / 终态"，退避只在可重试类上叠加（Media3 的 `LoadErrorHandlingPolicy`、OkHttp 的 `retryOnConnectionFailure` 同形态） | `SmbRetryGate`：把退避状态从读取逻辑里剥出来，成为可单测的状态机 |
| 断网要不要等 socket 报错 | 不等。查询系统网络状态，断了就当"已知不可达" | `NetworkLinkMonitor`：静态查询 `activeNetwork` 能力，**不注册回调、不管生命周期** |
| 路径/文件名编码 | 边界上一次转换，内部统一 UTF-8 | `utf8_codec.hpp`：JNI 边界做 UTF-8 ↔ UTF-16，中间层只见 UTF-8 |
| 读窗口多大 | 按码率/文件规模推导 + 上下限夹紧，不写死常量 | `SmbReadTuning`：一次推导、集中夹紧 |
| libsmb2 怎么提吞吐 | 官方 examples 走 async API（多条 PDU 同时在途）；`sync.c` 的封装是单 PDU 阻塞 | **251**（本卡只把结构腾出来，不动传输策略） |

---

## 三、目标设计

### 3.0 一句话架构

```
                     ┌──────────── 契约层（唯一对外）────────────┐
解码器 ──fd──► RangeFd ──► SmbByteSource（≤180 行：只编排顺序）  │
                     └───┬────────┬────────┬────────┬───────────┘
                         │        │        │        │
                 SmbReadAhead  SmbReadWindow  SmbPreadChannel  SmbReadStats
                 （预读环+线程）（块 LRU）    （读+重连）      （画像）
                         │        │        │
                         └────────┴───► SmbReader ──JNI──► libsmb2
                                        │
                                  SmbRetryGate（退避状态机）
                                        ▲
                                  NetworkLinkMonitor（系统网络可达性）
```

**关键边界**：`SmbByteSource` 不认识 libsmb2，只认识 `SmbPreadChannel` 暴露的
`readAt(offset, buf, off, len): Int`。窗口与预读都只依赖这个接口 → 可用假通道跑 JVM 单测。

### 3.1 决策

**D1 · 退避从读取逻辑里剥出来，成为状态机**

`SmbRetryGate`（纯 Kotlin，无 Android 依赖）：

```kotlin
class SmbRetryGate(
    private val online: () -> Boolean = { true },
    private val baseMs: Long = 400,
    private val capMs: Long = 4_000,
)

fun canAttempt(nowMs: Long): Boolean      // 未到点 → false
fun noteSuccess()
fun noteFailure(nowMs: Long)              // 退避翻倍封顶；离线时不翻倍（省一次无意义指数）
fun denyReason(): String                   // "网络已断开" / "连接已断开，正在等待重连"
```

- 退避序列保持 243 已验证的 400 → 800 → 1600 → 3200 → 4000 封顶，**不改数值**（那组数有真机证据）。
- 新增：`online() == false` 时 `canAttempt` 立即 false，且 `noteFailure` 不翻倍。
  恢复后第一次尝试不再等 4 s。
- **判据可测**：给定时间序列断言 `canAttempt` 的布尔序列，纯函数。

**D2 · 读超时与连接超时分离**

| 阶段 | 超时 | 依据 |
| --- | --- | --- |
| connect（negotiate + session setup + tree connect） | 保持 15 s | 慢网络认证真的要几秒；15 s 是 243 起就在用的值，不动 |
| 单次 READ | **3 s** | 真机 1 MiB 单次最坏 300 ms 量级（`p247` §五），3 s = 10 倍余量；一次抖动不再吃掉 15 s |

实现：JNI 增 `nativeSetReadTimeout(ctx, seconds)`，`SmbReader.open()` 在 `connect` **成功之后**调用。
不新增结构，只在 `SmbReader` 里加一次调用。

代价诚实说明：断网瞬间正在飞的那次读仍要等满 3 s（libsmb2 没有 cancel API，
跨线程 `smb2_destroy_context` 会 free 掉正在被读的内存，不能碰）。
**所以本卡的目标是"最坏 3 s"，不是 0。**

> **实测结论（2026-11-10，S3，与预期不符，必须记下来）**：
> `smb2_set_timeout(ctx, 3)` 调用返回 0，但断网后 `smb2_pread` 仍 **15 s** 才返回。
> 也就是说 **libsmb2 的 PDU 超时在"连接半死"时不按设定值生效**。
> 想回读 `smb2->timeout` 验证，但 `struct smb2_context` 是不透明类型（定义在私有头里），
> 拿不到。继续往 libsmb2 内部挖的收益不明，因此**改用下面的 D3 看门狗作为主手段**，
> 这一条只作为双保险保留（它确实让"连接层面的 PDU 有上限"，只是断网这条路上不起作用）。
> 恢复仍要 15 s + 退避 ≈ 17 s —— 那属于 251（异步流水线 / 可中断读）的范围。

**D3 · 链路可达性：静态查询，不注册回调**

`NetworkLinkMonitor`（`net/`，唯一 Android 依赖点）：

```kotlin
object NetworkLinkMonitor {
    fun isOnline(context: Context): Boolean   // activeNetwork + NET_CAPABILITY_INTERNET
}
```

为什么不用 `registerDefaultNetworkCallback`：

- 回调要配对 `unregister`，要跟 Activity 生命周期绑定，而 SMB 读发生在 fd 回调线程 →
  多一个生命周期面，多一类泄漏。
- 我们只在**读失败时**问一次"现在还有网吗"，查询是同步的、微秒级，频率 = 失败频率。
- 没有回调 = 没有"注册了忘注销"这种 bug 面。

接法：`RangeFd(source, storage, online = { NetworkLinkMonitor.isOnline(ctx) })`，
由 `RangeFd` 起一个 **1 秒轮询的看门狗线程**（`KivoSmbLink`，daemon）：
查不到网就立刻置 `stalled = true`，读成功时清掉。

为什么要"看门狗"而不是只靠读失败：实测证明读超时在断网时不按设定值返回（见 D2 的实测结论），
**"有没有网"是系统事实，本来就不该等 IO 报错才知道**。看门狗把提示延迟从
15 s 压到 1 s，且完全不依赖 libsmb2 的内部行为。

`online` 默认为 null（不监视），保证 `HttpRangeFile` 等既有调用方零改动、零额外线程。

用户感知：断网 → **约 1 s 内** `RangeFd.stalled = true` → 245 的"网络中断，正在重连…"出现。
真机实测（K40 / 4K HEVC 硬解）：`位置停止后 1000 ms 首次上报重连中`，比测试自身的
`STALL`（5 s 判定）还早 4 秒。

**D4 · `SmbByteSource` 拆成 6 个单职责单元**

现状 664 行、6 职责。拆分依据不是"按行数切"，是按**改动理由**切：谁会在什么情况下被改。

| 单元 | 会被谁改 | 职责 |
| --- | --- | --- |
| `SmbReadTuning` | 换设备、调内存 | 参数推导与夹紧 |
| `SmbReadWindow` | 换缓存策略 | 块对齐 LRU + 惰性分配 |
| `SmbReadAhead` | 换预读策略（**251 就在这里**） | 环形缓冲 + 自有连接 + 代际校验 + 可重启 |
| `SmbPreadChannel` | 换传输（**251 也可能在这里**） | 一次读 + 重连 + 退避 |
| `SmbReadStats` | 换诊断口径 | 画像采集与上报 |
| `SmbByteSource` | 换编排顺序 | 只编排：预读 → 窗口 → 等预读 → 填窗 → 直读 |

**这不是"把大文件切成小块"，是"把六个会独立变化的理由分开"。** 判据：251 只需改
`SmbReadAhead`（或 `SmbPreadChannel`）一个文件，其余五个一行不动。

**D5 · 预读可重启**

`SmbReadAhead` 的生命周期改成显式三态：

```
Idle ──start(offset)──► Running ──EOF/stop()──► Idle ──start(offset)──► Running
                          │
                          └── open 失败 ──► Disabled（终态，本实例不再启用）
```

- `stop()` 把 `thread` 与 `stopFlag` 一起复位；`start()` 时若 `thread == null` 才建线程。
- EOF 退出属于"自然停止"，回到 `Idle`，不是 `Disabled`。**播到结尾拖回去，预读会重新干起来。**
- `Disabled` 只留给"服务端限制并发句柄"这类真降级，且必须带 reason（G9）。

**D6 · 参数集中，且由文件规模推导**

`SmbReadTuning`：

```kotlin
data class SmbReadTuning(
    val blockBytes: Int, val blockCount: Int,
    val aheadBytes: Int, val aheadChunk: Int, val aheadWaitNs: Long,
) {
    companion object {
        fun forFile(sizeBytes: Long): SmbReadTuning   // 小文件降档
        val default: SmbReadTuning                    // 大文件：6×2 MiB + 6 MiB（= 247 实测档，不改）
    }
}
```

夹紧规则（全部可单测）：

| 输入 | 结果 | 理由 |
| --- | --- | --- |
| `size < 256 MiB` | `blockCount = 2`，`aheadBytes = 2 MiB` | 短片/剧集轨道少，6 块纯浪费 |
| `size < 2 GiB` | `blockCount = 4` | 单层 MKV 交织跨度小 |
| 其它 | `blockCount = 6`、`aheadBytes = 6 MiB` | **= 247 实测档，`Shelter` 2×TrueHD + PGS 需要 4 条轨常驻** |
| 下限 | `blockCount ≥ 2`、`aheadBytes ≥ 2 MiB` | 再小必然回到锯齿 |
| 上限 | `blockCount ≤ 6`、`aheadBytes ≤ 6 MiB` | 内存红线 |

**不做运行时 `onTrimMemory` 缩窗**（理由见 §五）。降低内存靠 D7 的惰性分配。

**D7 · 惰性分配**

`SmbReadWindow` 用 `arrayOfNulls<ByteArray>`，**第一次真的要填某一块时才 `ByteArray(BLOCK_BYTES)`**。

- 探测场景（`FfmpegRouting.probe` 只读文件头）→ 只分配 1 块 = 2 MiB，而不是 12 MiB。
- 结果：**软解双路从 36 MiB 降到"实际用到才付"**；长片播放稳态仍是 18 MiB/路（这个代价是合理的，
  不是浪费，247 的证据支持它）。

**D8 · 编码在 JNI 边界一次做对**

新增 `jni/utf8_codec.hpp`（纯函数，无 JNI 类型）：

```cpp
namespace kivo::jni_bridge {
std::u16string utf8_to_utf16(std::string_view);   // 非法序列 → U+FFFD，不抛
std::string    utf16_to_utf8(std::u16string_view);
}
```

`jni_string_convert.hpp` 的两个函数改为走它：

| 函数 | 现在 | 改为 |
| --- | --- | --- |
| `to_utf8` | `GetStringUTFChars`（modified） | `GetStringChars`（UTF-16）→ `utf16_to_utf8` |
| `to_jstring` | `NewStringUTF`（modified） | `utf8_to_utf16` → `NewString` |

**为什么改共用头而不是给 SMB 单独造一套**：`jni_string_convert.hpp` 已经是这个族唯一的转换入口
（`jni_player_bridge`、`jni_session_handle` 都在用）。再造一套 = 两个真相源。改它，全部调用方
从"对 BMP 正确"变成"对全部 Unicode 正确"，方向单一。

**D9 · 读失败的归因用 rc，不用"上一次的 nterror"**

libsmb2 `sync.c:373` 返回 `cb_data->status`（NT status，高位 0xC0…）或 `-errno`。
`nativeReadInto` 现在把两者都压成 `-1`，信息丢失。

改法：`nativeReadInto` 在 `n < 0` 时**返回该值本身**（jint 已有符号，不丢位）。

```
0xC0000121 (IO_TIMEOUT) 作为 int32 = -1073741535  →  Kotlin: read.toLong() and 0xFFFFFFFFL 还原
```

`SmbError` 增 `fromReadCode(code: Int): SmbFailure?`：

| 判定 | 结论 |
| --- | --- |
| `(v and 0xFFFF0000L) == 0xC0000000L` | 是 NT status，按表翻译（**含新增 `IO_TIMEOUT` 单列**） |
| 其它 | 是 `-errno`，按 errno 语义词翻译（`EIO`/`ECONNRESET`/`ETIMEDOUT`） |
| 拿不到 | 退回 `nativeNtStatus`（保持现状） |

**D10 · IPv6 地址在 Kotlin 侧规范化**

`SmbAddress` 增 `val authority: String`（`host:port` 的唯一正确形态）：

| host | authority |
| --- | --- |
| `192.168.5.1` | `192.168.5.1:445` |
| `::1` / `fe80::1%wlan0` | `[::1]:445` / `[fe80::1%wlan0]:445` |
| `nas.local` | `nas.local:445` |

JNI 只 `to_std(authority)`，**不再拼字符串**。`SmbDiscovery` 删掉 `substringBefore('%')`
（链路本地丢 zone id 就不可路由——这行注释写反了）。

### 3.2 家族树（自然结构，单文件单职责）

**工作目录约束（不可协商）**

- 一律在 `C:\kivo video` 内改动。**不新建 Gradle module、不新建项目包、不新建 source set
  （`src/test` 见 §四 闸门 F，需单独批准）、不把任何文件写到仓库外。**
- 不新增空目录。`smb/`、`net/`、`jni/` 都是已存在的目录，本卡**不新建目录**。
- 行数预算：新建 ≤220 行；存量改动后不得比改动前更长（除标注的迁移例外）。

```
android/app/src/main/java/com/kivo/cinema/
  smb/
    SmbByteSource.kt      改  664→≤180  契约实现：只编排顺序（预读→窗口→等预读→填窗→直读）+ 生命周期
    SmbReadTuning.kt      新  ≤100      参数推导与夹紧（纯数据，无 Android/无 libsmb2）
    SmbReadWindow.kt      新  ≤180      按块对齐的 LRU 窗口 + 惰性分配；填充走注入的"读满"回调
    SmbReadRing.kt        新  ≤140      预读环纯数据：复制/消费/前移/代际校验（可单独单测）
    SmbReadAhead.kt       新  ≤220      顺序预读：线程生命周期 + 网络读 + 代际交接 + 可重启
    SmbPreadChannel.kt    新  ≤170      一次读 + 重连 + 退避；对外只暴露 readAt
    SmbReadStats.kt       新  ≤120      画像采集（命中/填充耗时/块号分布）+ 4 s 上报
    SmbRetryGate.kt       新  ≤110      退避状态机（含 online 闸门），纯逻辑可测
    SmbReader.kt          改  82→≤95    connect 后设读超时；失败带 rc 归因
    SmbError.kt           改  82→≤111   + IO_TIMEOUT 单列；+ fromReadCode（只判 NT status 家族）
    SmbErrno.kt           新  ≤70       系统 errno → 文案（与"服务端说什么"分家，见 §3.2 修订）
    SmbAddress.kt         改  93→≤110   + authority（IPv6 方括号/端口唯一形态）
    discovery/SmbDiscovery.kt 改 104→104 删 substringBefore('%')（保留 zone id）
    ui/SmbBrowserActivity.kt  改 279→279 断链时 error 文案换"网络已断开"（同行走另一分支）

  net/
    NetworkLinkMonitor.kt 新  ≤100      系统网络可达性静态查询（唯一 Android 依赖点）
    RangeFd.kt            不改          耐心/退避逻辑保持；只由 D2 的 3 s 超时受益
    ByteSource.kt         不改          契约不变

  ui/compose/
    PlayerComposeActivity.kt 改 1000→1001  open(...) 传 online provider（**迁移例外：+1 行**）

android/app/src/main/cpp/jni/
    unicode_limits.hpp    新  ≤40       码点边界与 U+FFFD（两个方向共用，避免真相分裂）
    utf8_codec.hpp        新  ≤120      标准 UTF-8 → UTF-16（libsmb2 → Java 方向）
    utf16_codec.hpp       新  ≤110      UTF-16 → 标准 UTF-8（Java → libsmb2 方向）
    jni_smb2_json.hpp     新  ≤70       JSON 字符串转义（含 <0x20 → \u00XX）
    jni_smb2_context.hpp  新  ≤40       ctx_of / fh_of / user_or_null（桥与 list 共用）
    jni_string_convert.hpp 改 54→≤73    to_utf8 / to_jstring 改走 utf8_codec（两个方向都修）
    jni_smb2_bridge.cxx   改 242→≤175  通道原语：连/开/读/关/错误；+ nativeSetReadTimeout；
                                        + 读失败返回 rc；删 nativeReadAt（死接口）
    jni_smb2_list.cxx     新  ≤95       两个 JSON 生产者：nativeList / nativeListShares
    CMakeLists.txt        改            + jni_smb2_list.cxx（唯一一处构建改动）

docs/design/backend-core/
    250-smb-io-hardening-design.md  本文
```

**依赖方向（单向，不许回环）**

```
ui/*  ──►  smb/*  ──►  net/ByteSource
             │              ▲
             │              └── SmbByteSource 实现
             ├──► smb/SmbReader ──JNI──► libsmb2
             └──► net/NetworkLinkMonitor（只经 `online: () -> Boolean` 这一个 lambda）

禁止：
  smb/SmbReadWindow  ← 不许 import SmbReader / 不许 import android.*
  smb/SmbRetryGate   ← 不许 import android.*（online 是注入的 lambda）
  smb/SmbReadAhead   ← 不许 import SmbReader 具体类（只认 readAt 接口）
  net/*              ← 不许 import smb/*（除 SmbByteSource 实现 ByteSource）
```

**每个文件"说不清职责就该拆"的自检**

| 文件 | 用一句话说职责 | 说不清时 |
| --- | --- | --- |
| `SmbReadTuning` | 决定"这次用多大窗口" | 若开始出现"怎么读"，越界 |
| `SmbReadWindow` | 记住"哪一块已经在内存里" | 若开始出现"怎么连"，越界 |
| `SmbReadAhead` | 提前把后面的字节搬进来 | 若开始出现"重试几次"，越界（那是 Gate） |
| `SmbPreadChannel` | 把一次读发出去，失败了再来 | 若开始出现"缓存哪一块"，越界（那是 Window） |
| `SmbReadStats` | 记录"刚才读得好不好" | 若开始"决定"任何事，越界 |
| `SmbRetryGate` | 回答"现在能不能再试" | 若开始"执行"重试，越界 |
| `SmbByteSource` | 按什么顺序问上面五个 | 若开始"实现"任何一件，越界 |

**实施期修订（2026-11-10 · S1）**

S1 落文件时有三个单元超预算。按"超出即拆"处理，拆法不是按行数切，是按**改动理由**切：

| 原计划 | 实际 | 为什么这样拆 |
| --- | --- | --- |
| `SmbError.kt` ≤115（实测 133） | 拆出 `SmbErrno.kt`（37 行），`SmbError` 回到 111 | 两者上游不同：一个翻"服务端报文说什么"（账号/共享/权限），一个翻"本机 socket 出了什么事"（网线/DNS/重置）。合成一个 `when` 就是两个上游共用一个修改点 |
| `utf8_codec.hpp` ≤120（实测 163） | 拆成 `unicode_limits.hpp`（25）+ `utf8_codec.hpp`（108）+ `utf16_codec.hpp`（80） | 两个方向**不共享辅助函数**（一个 decode_utf8+append_utf16，一个 decode_utf16+append_utf8），只有常量共用。共用常量单独成文件，避免"一边改了一边没改" |
| `SmbReadTuning.kt` ≤100（实测 101） | 合并两行常量注释，99 行 | **不拆**。它只有一个职责（参数推导），101 行只是注释偏长，拆开会出现两个都叫 Tuning 的文件 |

**实施期修订（2026-11-10 · S2）**

| 原计划 | 实际 | 为什么这样拆 |
| --- | --- | --- |
| `SmbReadAhead.kt` ≤220（首版 241） | 拆出 `SmbReadRing.kt`（101 行），`SmbReadAhead` 回到 214 | 缓冲"怎么存"与"谁去填"是两个修改理由；拆出后代际校验/前移/边界可以在 JVM 上断言（S2 新增 10 个用例），以前只能靠真机播放 |
| `setPrefetchEnabled(false)` = 永久关闭 | 改为 `ahead.pause()`，可再次开启 | 这是 D5 的内容（修 P1-3）。**有意变更**，不是搬运 |
| `SmbByteSource` 内 `directRead` | 并入 `SmbPreadChannel.readFully` | "读满 / 读到 0 当异常 / 真 EOF 返回 -1"这条规则只该有一个实现；窗口与直读共用它 |
| `fillWindow` 填完才写偏移 | `fill` 进入填充前先标废该槽 | 顺带修掉的**原缺陷**：填充中途抛异常时，槽位仍指旧偏移而内容已被写半截 → 下次"命中"返回错位数据（花屏级） |

**不新增的东西**

- 不新建 `smb/read/`、`smb/io/`、`smb/core/` 等子包（**加一层包 = 加一层"这个文件该放哪"的猜谜**）。
- 不换 SMB 库，不动 `libsmb2-src`，不加 Gradle 依赖。
- 不在本卡引入 async API（251）。
- 不把 `SmbEntry` 排序抽成文件（3 行，抽出即过度拆分）。

### 3.3 与既有卡的关系

| 卡 | 关系 |
| --- | --- |
| 243 | 契约（`ByteSource`/`RangeFd`）、错误分阶段、加密存储全部沿用；本卡只改实现内部结构 |
| 244 | 无交集（H.264-in-MKV 已闭环） |
| 245 | "缓冲态提示"已实现（`RangeSource.stalled`）；本卡让它的触发从 15 s 提前到 3 s |
| 247 | 预读的性能结论（6×2 MiB + 6 MiB）**原样沿用**，本卡只改它的可重启性与归属 |
| 251（建议） | 单连接异步流水线。本卡把 `SmbReadAhead` 与 `SmbPreadChannel` 的边界腾出来，251 只换这两个中的一个 |

---

## 四、分阶段与闸门

| 阶段 | 工作 | 完成判据 |
| --- | --- | --- |
| **S1 纯函数族** | `SmbReadTuning`、`SmbRetryGate`、`SmbAddress.authority`、`SmbError.fromReadCode`、`utf8_codec.hpp` | 5 个单元可独立编译；`assembleDebug` 通过；行为零变化（不放开关） |
| **S2 拆 `SmbByteSource`** | `SmbReadWindow`、`SmbPreadChannel`、`SmbReadStats`、`SmbReadAhead`、瘦身契约层 | 6 个文件行数全部达标；**现有 instrumentation 全量复跑 4/4 PASS**；`KivoSmbRead stats` 输出字段与改动前三项一致（hits/fills/fill_max） |
| **S3 断链与超时** | `NetworkLinkMonitor`、读超时 3 s、闸门接线、浏览页文案 | 真机：断网后 **≤5 s** UI 出现"网络中断，正在重连…"且会话不判死；恢复后自动续播 |
| **S4 JNI 健壮性** | `jni_smb2_json.hpp`、`jni_string_convert` 改走 codec、`nativeSetReadTimeout`、读失败返回 rc、删 `nativeReadAt`、IPv6 authority | 真机：含 emoji 的文件名可列举可播放；含换行的目录名不再整目录失败；IPv6 地址（若有环境）能连 |
| **S5 收口** | 参数表、`AGENTS.md` §0.3 补 libsmb2 构建项、`TASKS.md`/`CURRENT.md` 记账 | 文档里的常量与代码逐条对得上（见 §四 闸门 G） |

### 闸门（每条都要能自己复算）

| # | 闸门 | 复算方式 |
| --- | --- | --- |
| G1 | 不新建项目包 | `git status --porcelain` 里新增路径只出现在 `smb/`、`net/`、`jni/`、`docs/` |
| G2 | 单文件单职责 | 每个新文件 ≤ 上表预算；超出即拆，不允许"先堆着" |
| G3 | 存量不变长 | `git diff --stat` 逐条核；`PlayerComposeActivity` 的 +1 行是唯一例外且已在 §3.2 标注 |
| G4 | 行为零回归 | `SmbPlaybackSeekInstrumentation`、`SmbReconnectInstrumentation`、`SmbShareEnumInstrumentation` 全跑 |
| G5 | 断链达标 | 断网 → `adb logcat -s KivoRangeFd` 出现 `read stalled`，时间戳与 `svc wifi disable` 相差 ≤5 s |
| G6 | 编码达标 | 共享上放 `测试😀.mkv` 与 `换\n行.mkv`，浏览页能列、能播 |
| G7 | 文档不漂移 | 参数表逐行 grep 代码常量，不一致即失败 |
| **F（需单独批准）** | 引入 JVM 单测 | 新增 `android/app/src/test/java/com/kivo/cinema/smb/`（Gradle 标准 source set，非新项目包）+ `testImplementation` 1 项。**不批准则 S1 的判据改为"instrumented 覆盖"，本卡不做单测** |

---

## 五、明确不做（本卡）

| 不做 | 为什么 |
| --- | --- |
| 单连接异步流水线（多 outstanding READ） | 1–2 周量级，会重写 `SmbReadAhead` 与传输策略；是本卡的结构前提，不是本卡内容 → **251** |
| 硬解路径重新启用预读 | 现在的双连接预读确有害（24.00 → 20.27 fps 有实测）。**等 251 换了传输形态再开**，本卡不靠调参硬开 |
| 运行时 `onTrimMemory` 缩窗 | 要跨层接线到 Activity；收益未验证（目标设备 8–12 GB）。先用 D7 惰性分配拿到确定收益 |
| Kerberos / 强制签名 / DFS referral | 产品决策未定（企业场景）。`build.sh` 里 `ENABLE_LIBKRB5=OFF` 是现状，不擅自改 |
| SMB1 / 32 位 ABI | 243 已明确不做；`abiFilters = arm64-v8a` 保持不变 |
| `smb2_pread` 的可中断（cancel） | libsmb2 无 cancel API；跨线程 `destroy` 是 UB。**用 3 s 超时兜，不用关闭 socket 兜** |
| 目录列举改 JNI 回调（消灭 JSON） | 本卡只把转义做对；改回调会动 3 个文件的协议面，收益是"少一层序列化"，不属健壮性 |

---

## 六、风险

| 风险 | 影响 | 对策 |
| --- | --- | --- |
| 拆分引入并发 bug（窗口/预读/统计三处状态） | 花屏、错帧、偶发卡顿——比原问题更糟 | S2 **只搬运不加逻辑**：先 1:1 搬出、跑全量 instrumentation 绿，再加 D5 的可重启 |
| `jni_string_convert` 改动波及非 SMB 调用方 | 播放器/字幕路径字符串行为变化 | 改动只影响非 BMP 字符（原来就错）；S4 后跑一次本地视频 + 字幕回归 |
| `smb2_set_timeout` 降到 3 s 后，慢存储误判 | 老 NAS 冷启动首读可能 >3 s | 3 s 只作用于 READ；connect 仍 15 s；闸门 G4 里加一条"NAS `N1` 冷启动首读不报错" |
| `NetworkLinkMonitor` 在 `activeNetwork` 切换窗口内误报在线 | 断网第一秒仍走 3 s 超时 | 可接受（最坏 3 s）；不为此引入回调复杂度 |
| 惰性分配让首次填块多一次 `ByteArray` 分配 | 起播首帧多一次 ~2 MiB 分配（毫秒级） | 可接受；不用对象池（池化要把生命周期管到进程级，得不偿失） |

---

## 七、证据归档

- 本卡设计稿：`docs/design/backend-core/250-smb-io-hardening-design.md`（本文）
- 阶段证据：`docs/evidence/p250/`
  - `s1-unit/`（若 F 闸门批准：单测输出）
  - `s2-split/`（拆分前后 instrumentation 对照 + `git diff --stat`）
  - `s3-link/`（断网时间戳对照：`svc wifi disable` vs `read stalled`）
  - `s4-encoding/`（emoji / 换行文件名真机截图 + logcat）
- 前置评审：本轮对话的逐层分析（P0/P1/P2 清单 + 双连接退化 6 倍的推演）

---

## 八、实施纪律（对齐 `.codex/memory/kivo-video-iron-gate.md`）

1. **同一工作目录**：全部落在 `C:\kivo video`；不新建 module / source set（除 F 闸门批准）/ 项目包。
2. **一阶段一提交**：S1…S5 各自一个提交，收口前跑 G1–G4。
3. **先搬后改**：S2 的拆分必须是 1:1 搬运（含注释），跑绿之后才允许改行为（D5）。
4. **只删不加**：`nativeReadAt`、内联 JSON 转义、`smb2_set_timeout(15)` 的读路径含义，先删干净再用新的。
5. **没有证据不说完成**：每个阶段在 `docs/evidence/p250/` 留下可复算产物。
6. **不静默降级**：`SmbReadAhead.Disabled` 必须带非空 reason 且进 logcat，用户可见态由现有 `stalled` 承担。

---

## 九、参数表（G7 闸门：逐条对代码）

六个常量原本散在两个语言三个文件里，没有一张表能一眼看完。这里集中列出，
**改动任何一个都必须同步本表**，CI 里用 `grep` 比对（见 §四 G7）。

| 参数 | 值 | 位置 | 依据 |
| --- | --- | --- | --- |
| `DEFAULT_BLOCK_BYTES` | 2 MiB | `SmbReadTuning` | 1 MiB 时命中率 82%（MKV 交织距离超过 1 MiB） |
| `DEFAULT_BLOCK_COUNT` | 6 | `SmbReadTuning` | `Shelter` 2×TrueHD + PGS 需 4 条轨常驻，留 2 格机动 |
| `DEFAULT_AHEAD_BYTES` | 6 MiB | `SmbReadTuning` | 3.6 MB/s 码率下 ≈ 提前 1.7 s，吸收两次抖动 |
| `DEFAULT_AHEAD_CHUNK` | 1 MiB | `SmbReadTuning` | 1 MiB/92 ms ≈ 11 MB/s；再小被往返次数拖住 |
| `DEFAULT_AHEAD_WAIT_NS` | 350 ms | `SmbReadTuning` | 覆盖"预读在路上、就差这一下"；秒级会白耗帧预算 |
| `SMALL_FILE_BYTES` | 256 MiB | `SmbReadTuning` | 以下降到 2 块 / 2 MiB |
| `MEDIUM_FILE_BYTES` | 2 GiB | `SmbReadTuning` | 以下降到 4 块 / 4 MiB |
| `BASE_MS` (退避起点) | 400 | `SmbRetryGate` | 243 真机验证值，不改 |
| `CAP_MS` (退避封顶) | 4 000 | `SmbRetryGate` | 同上 |
| `INTERVAL_MS` (画像上报) | 4 000 | `SmbReadStats` | ≈96 帧，够看出锯齿、不刷爆日志 |
| `READ_TIMEOUT_SECONDS` | 3 | `SmbReader` | 单次 1 MiB 最坏 300 ms → 10 倍余量。**实测断网时不生效**，见 D2 |
| connect 超时 | 15 s | `jni_smb2_bridge.cxx` `nativeCreate` | 慢网络上的认证 + 树连接真要几秒 |
| `DEFAULT_PATIENCE_MS` | 20 000 | `RangeFd` | 覆盖 Wi-Fi 重连（2–5 s）与 NAS 短时重启 |
| `LINK_POLL_MS` | 1 000 | `RangeFd` | 看门狗间隔；实测提示延迟 1000 ms |
| fd 层重试退避 | 250 / 500 / 1000 | `RangeFd.onRead` | 不打爆服务器，恢复后 1 s 内接上 |
| `TIMEOUT_MS` (mDNS) | 8 000 | `SmbDiscovery` | 浏览窗口，到点自动停 |

**单路常驻内存**：`blockBytes × blockCount + aheadBytes` = 2 MiB × 6 + 6 MiB = **18 MiB**
（大文件档）。软解双路 = 36 MiB。这是 247 实测档，不是浪费：
四条轨常驻换来命中率 100%、同步读 0 次。

---

## 十、本卡结题时仍然开着的口子

| 口子 | 归谁 |
| --- | --- |
| 断网后**恢复**仍要 17 s（12 s 断网 + 5 s 重连） | 251：可中断读 / 异步流水线 |
| 硬解路径仍关闭预读（命中率 91–95%，5–9% 同步读 87–140 ms） | 251：换掉"第二条连接"这个形态后再开 |
| 含 emoji 的文件名、IPv6 真能连上 —— 只有自检与单测，没有整链路真机证据 | 待共享可写 / 路由开 IPv6 时补 G6 |
| `jni_string_convert.hpp` 73 行（预算 70） | 注释超 3 行，不拆（§六 已记） |
