# P14-R · 那份报告说不出「我为什么停在这里」

> 一张卡只答一个问题。本卡答：**一个没有播完的会话，能不能留下一份说得清的账？**
>
> 上一张卡的预言第 4、5 条没兑现：切后台那轮**什么都没写出来**（`cat: No such file or directory`）。
> 当时的解释是"进程被回收"。本卡读了代码之后认为那个解释**不完整** ——
> 进程被回收只是最后一环，真正的堵点在前面：**今天的 schema 里没有"被打断"这个形状。**

---

## 一、为什么切后台那轮什么都没写出来

三层原因，一层比一层深：

| 层 | 事实 | 后果 |
|---|---|---|
| 接线 | `finishPlayback()` 只从 400 ms 轮询里被调，且只在 `summary` 出现 `completed=1` 时才写 | 被打断的会话 `completed` 永远是 0 → **永远不写** |
| 时机 | 轮询是 UI 线程的 400 ms 心跳，进程被回收时不保证还能跑一轮 | 就算条件成立，也可能差最后一拍 |
| **形状** | 报告里只有 `completed`（bool）和 `error`（字符串）。`completed=false` 且 `error==""` 在层 3 被**明确判为"静默失败"** | **写出来就是假故障** |

第三条是这一整张卡的由来。前两条是"没写"，第三条是"**没有可写的东西**"。

### 1.1 实测：这个状态从来没进过证据

| 事实 | 结果 |
|---|---|
| `kivo.p12c.sync/*` 全部证据的 `completed` | **214 / 214 为 `true`**，`false` 零份 |
| `kivo.p12c.decode/*` 里 `completed=false` 且 `error==""` | **30 份，全部是 `-t####` 长稳中途快照** |

那 30 份能过层 3，靠的是**文件名正则**：

```python
is_midrun_snapshot = bool(re.search(r"-t\d+\.json$", path.name))
```

也就是说：`completed=false && error==""` 这一格今天同时装着两种完全不同的东西 ——
"还没跑完"和"被打断了"，而区分它们的唯一手段是**文件名长什么样**。
第 31 份如果不是快照命名，它就会被读成静默失败。

**一个状态如果只能在文件名里表达，它就还没有进入协议。**

---

## 二、先量一遍：`stop()` 会不会制造未交付的帧

写判据之前先读实现，因为这里有一条容易想错的路。

`DecodeReceiveWorker::handle_frame()` 里，一帧的全部动作在**同一圈**内完成：

```
arrive() 决策 → release_frame() 交付 → frames_rendered / dropped_late / discarded_no_surface 三选一
```

而 `DecodeSession::stop()` 做的是 `gate_.request_stop()` 然后 join ——
`request_stop()` 只在**圈与圈之间**被看见。所以：

> **`stop()` 落在帧边界上。它不会把某一帧停在"决策了但没交付"的半路上。**

推论：`decided == presented + dropped + discarded_no_surface` 在**任何**收尾时刻都成立，
包括被打断的那一刻。**"会话在交付前结束"这件事本身不产生未记账的帧。**

### 2.1 但有一帧只在"自然结束"时存在

`frames_received` 里有一帧是 EOS 缓冲 —— 不是画面、不进节拍、但要 release 把槽还回去
（`decode_receive_worker.cxx:90` 那段注释）。所以解码侧那本账多一个常数：

```
frames_rendered + frames_dropped_late + frames_discarded_no_surface + 1 == frames_received
                                                                     ↑ 这一帧是 EOS
```

被打断的会话**没有走到 EOS**，所以那个 `+1` 不存在。于是同一份公式会在被打断的报告上差 1，
**而且只差这 1**。

这条不是缺陷，是本卡要给判据的那条规则的来源：

> **EOS 那一帧只在"自然结束"时存在 —— 所以 `+1` 只对 `end_reason == completed` 成立。**

`verify_p12_all.py` 的层 2 早就承认过同类窗口（`check_frames_closed` 写着"允许 1 帧在途
（交出与计数之间有一个观察窗口）"）。这次的 1 不是观察窗口，是**一帧根本不存在的缓冲**，
两者不能互相顶替。

---

## 三、自然家族树

### 3.1 原生侧：一个派生量，住在一个头文件里

```
backends/mediacodec/session/
├── session_end_reason.hpp      ← 新增：三值枚举 + 名字 + 派生函数（constexpr，可单测）
├── decode_stats_renderer.cxx   ← 印 outcome.end_reason；kSchema → /19
└── av_sync_report.cxx          ← 印顶层 end_reason；kSchema → /29
```

`end_reason` **是纯派生量**，和 P14-Q 的 `undelivered_frames` 同一条理由：

```
failed       ⟸ error 非空
completed    ⟸ completed 为真
interrupted  ⟸ 其余（停了、没报错、也没跑完）
```

**不新增任何状态、不配 `measured`。** 给一个纯函数配"我很确定"，是 P14-L 反的那类东西。

派生函数单独放一个头文件而不是各写一遍，是因为它要被**两处**用（decode 与 sync），
而两处各写一个 `if` 就是两份记忆 —— 本工程已经为这类事付过三次账
（`SYNC_QUANTILE_BLOCK_SCHEMAS` 的两表不同步、Q 组的 `enter` 位置、S2/S3 的重叠区）。

### 3.2 `interrupted` 会不会变成"静默失败"的免死金牌

会，如果不管。所以配一条**独立**的守卫：

> `interrupted` 免除的是"层 3 的静默失败"这一条，
> **不免除任何一本账**。S2（`decided = presented + dropped + dns`）对三种收尾一视同仁，
> 解码侧的四项闭合也一样严格 —— 只是 `+1` 换成 `+0`。

换句话说：**逃掉的是"你有没有报错"这一问，逃不掉的是"账对不对"这一问。**
而后者才是本工程真正在守的东西。

同时把份数**打印出来**（`另有 N 份 interrupted（显式中断，不是静默失败）`）。
P14-M 的全部教训是"不可见"比"判错"更坏。

### 3.3 宿侧：收尾这件事从 Activity 里搬出来

```
android/.../ui/PlayerActivity.kt          ← 只留生命周期接线与 UI
android/.../evidence/SessionFinalizer.kt  ← 新增：收尾顺序 + 封口标志
android/.../evidence/EvidencePaths.kt     ← 新增：文件名策略（stats / sync / 失败证据）
```

`PlayerActivity` 现在 590 行、八件事：生命周期、Surface、Intent 解析、起播、收尾、失败证据、
片源定位、状态投影。本卡只搬"收尾"与"文件名"两件 —— 因为本卡要给它加**第二个入口**
（被打断），而一个已经有 590 行的文件再加一个入口，就是第 N 次"顺手加一点"。

不搬的东西写明理由：起播链路（`maybeStartPlayback`）与失败证据留在原处，
前者是 Activity 的决策（要不要开音频、挂不挂节拍），后者是它的错误呈现。

### 3.4 收尾顺序统一成「先停，再写」

现在的顺序是：

```
写 stats → stop() → 写 sync        （finishPlayback，自然收尾）
```

两个写之间夹一个 `stop()`，理由是"sync 要读节拍器停下来之后的完整直方图"。
被打断的路径没有这个奢侈：写 stats 的时候**线程还在跑**，
`frames_received` 与 `frames_released` 会差 1 —— 那正是层 2 承认的"在途窗口"，
但它会让一份**本该闭合**的证据看起来没闭合。

所以统一成：

```
stop() → 写 stats → 写 sync
```

理由是一条**更强的规则**：**写之前先让所有线程停下。** 它对"为什么写"不作假设。
自然收尾路径改序之后数值应当逐位不变（`wall_micros` 由 `finish()` 的 CAS 决定，
早就在 stop 之前写好了）—— 这一点由真机一轮 1800 帧的复测来验，不由推理来保证。

---

## 四、判据

| 文件 | 改动 |
|---|---|
| `verify_p12_all.py` | 白名单加 `decode/19`、`sync/29`；层 3 的静默失败按 `end_reason` 豁免并计数；`check_sync` 对 `/29` 判"`end_reason` 在场且取值合法" |
| `verify_p12c_sync.py` | 接受 `sync/29`（原来只认 `/1`）；四项闭合改成 **`+1` 仅当 `end_reason == completed`** |
| `verify_p13d_drift.py` | `SCHEMA_LATEST` → 29；新增 **S5**：`/29` 起 `end_reason` 必须在场、取值三种之一，且与 `completed` 不矛盾（`completed=true` 时不许写 `interrupted`） |

### 4.1 为什么 S2 不为"被打断"开口子

这是本卡唯一需要顶住诱惑的地方。打断的报告上，`undelivered` 会明显大于 0
（切后台时面已经摘了，帧全进 `discarded_no_surface`）。最省事的写法是
"`interrupted` 时 S2 不判" —— 那等于在最需要它的证据上把判据关掉，
和 P13-C 那次"判据写在 return 之后"是同一个病。

§二 已经量过：`stop()` 落在帧边界上，账**必须**闭合。所以 S2 一个字不改。
如果真机上红了，那是本卡的收获，不是判据的错。

### 4.2 S5 为什么单独一条

`end_reason` 与 `completed` 是两个字段说同一件事。两者矛盾时（`completed=true` 却写
`interrupted`），任何一本账都还是对的 —— 错的是**叙述**。这种错不会让别的判据红，
只会让人读错，所以必须单独立一条。

---

## 五、实施顺序

1. `session_end_reason.hpp` + 单测（纯函数，桌面可测）。
2. 两个报告印出 `end_reason`，schema → `/19`、`/29`。
3. 宿侧：`SessionFinalizer` + `EvidencePaths` 抽出；顺序改成 stop 先；
   `onStop` 接上（探针开关 `kivo.finalize=on_stop`，默认不改变产品行为）。
4. 判据：三处改动。
5. 桌面：CMake + CTest；Android：`assembleDebug`。
6. 真机两轮：① 完整播放（复测顺序改了之后数值不变）② 打断（`end_reason=interrupted`）。
7. 变异自测：`+1` 分支、S5、层 3 豁免各一条能红的样本。

---

## 六、预言（写完不改，错了也不改回去迁就结果）

| 预言 | 值 | 依据 |
|---|---|---|
| 既有 sync 证据 `completed` 全为 true | 214 / 214 | 已实测 |
| 既有 decode 的 `completed=false && error==""` | 30 份，全是 `-t####` 快照 | 已实测 |
| 打断轮 `frames_received − (rendered+dropped+dns)` | **0**（不是 1） | 没有 EOS 帧 |
| 打断轮 `undelivered_frames` | **等于** `frames_discarded_no_surface` | §二：stop 落在帧边界 |
| 打断轮 `end_reason` | `interrupted` | 派生式 |
| 打断轮 S2 | **绿** | §二 的推论；若红是本卡最大收获 |
| 完整轮改序之后 | `1800 / 1800 / 0 / undelivered=0` | 与 P14-Q 逐位相同 |
| 全量校验 | 656 份 / 四层通过 | 214 + 2 份新证据 |

第 6 条我照旧不预设：**如果打断会制造未记账的帧，那必须在这里被看见。**

---

## 七、明确不做

1. **不把"切后台就收尾"设成产品默认。** 现在的产品行为是"后台继续播"（P12-D1 的
   `rebuilds=1` / `frames_discarded ≈ 空窗×30`）。要改成"后台停播"是产品决策，
   而且必须连带解决"回来从哪继续"（现在 native 没有对外暴露播放位置）。
   本卡只提供取证档：`--es kivo.finalize on_stop`。
2. **不修"进程被杀写不出报告"。** 被杀就是没有边界、没有报告 —— 那种情况只能靠
   中途快照（`kivo.soak`）留痕。本卡不碰。
3. **不给 `end_reason` 配 `measured`**（§3.1）。
4. **不动 `undelivered_frames` 的定义**，也不动 S0–S4。
5. **不给 `sync/2`–`sync/28` 补这一格。** 旧证据不重写。
   `verify_p12c_sync.py` 只扩到 `/29`，不回头把 200 多份中间版本重新判一遍 ——
   那是另一件事（要不要回头补判据），不该夹在本卡里做。
