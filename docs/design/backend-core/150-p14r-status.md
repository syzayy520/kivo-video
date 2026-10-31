# P14-R 状态卡 · 那份报告说不出「我为什么停在这里」

> schema：`kivo.p12c.decode/18` → `/19`，`kivo.p12c.sync/28` → `/29`
> 一张卡只答一个问题：**一个没有播完的会话，能不能留下一份说得清的账？**
> 结论：**能**，而且账是闭的。**顺带在判据层挖出三个洞。**

---

## 一、改了什么

| 位置 | 改动 |
|---|---|
| `session/session_end_reason.hpp` | 新增：三值枚举 + 派生函数 + 名字（纯 constexpr，两处共用） |
| `session/decode_stats_renderer.cxx` | `outcome` 加 `end_reason`（紧跟 `completed`）；`kSchema` → `/19` |
| `avsync/report/av_sync_report.cxx` | 顶层加 `end_reason`；`kSchema` → `/29` |
| `android/.../evidence/EvidencePaths.kt` | 新增：证据目录与文件名策略 |
| `android/.../evidence/SessionFinalizer.kt` | 新增：收尾顺序 + 封口标志（从 Activity 搬出） |
| `android/.../evidence/FailureEvidence.kt` | 新增：失败证据（输出逐字节不变） |
| `android/.../ui/PlayerActivity.kt` | `onStop` 接上中断收尾；收尾改走 `SessionFinalizer` |
| `tools/session_end_reason.py` | 新增：取值 + 四条自洽关系（两个校验器共用） |
| `tools/verify_p12_all.py` | 白名单两条；层 2 加 `check_end_reason`；层 3 按 `end_reason` 豁免并计数；`check_sync` 判 `/29` 在场 |
| `tools/verify_p12c_sync.py` | 扩到 `/29`；**EOS 的 `+1` 改成只看 `end_reason`**；分块结构复算；速率上界对非 `completed` 不成立 |
| `tools/verify_p13d_drift.py` | `SCHEMA_LATEST` → 29；新增 S5/S6；**修 S2 读错块** |
| `tests/.../p14r_end_reason_tests/` | 12 checks（派生四条路 + 三个词是协议 + 未知值回落 + 名字两两不同） |
| `tools/run_p14r_end_reason.sh` | 新取证脚本（含锁屏前置检查、对照组说明） |

### 1.1 收尾顺序统一成「先停，再写」

上一版是「写解码证据 → `stop()` → 写合并证据」。两个写之间夹一个 `stop`，理由是**只有**合并证据要读"节拍器停下来之后的完整直方图"。

打断路径没有这个奢侈：写解码证据时线程还在跑。所以改成一条**更强的规则** —— **写之前先让所有线程停下**，它对"为什么写"不作假设。

**这条改动必须用真机回归**，因为顺序变了。见 §二 第 1 行：`1800 / 1800 / 0`，与 P14-Q 逐位相同。

---

## 二、真机证据（Redmi K40 / b6e950ee，1080p30 / 60 s，`cpu_paced`）

| | 完整播放 | 播 6 秒切后台 |
|---|---|---|
| `end_reason` | `completed` | **`interrupted`** |
| `completed` | true | false |
| `error` | `""` | `""` |
| `frames_decided` | 1800 | **197** |
| `presented` | 1800 | **191** |
| `dropped` | 0 | 0 |
| `undelivered_frames` | 0 | **6** |
| `frames_discarded_no_surface` | 0 | **6** |
| `frames_received` | 1801 | **197** |
| `received − (rendered+dropped+dns)` | **1**（EOS 帧） | **0** |
| S2 | 绿 | **绿** |

### 2.1 三个预言当场兑现

- **打断轮 `received − (rendered+dropped+dns) = 0`，不是 1。** 那个 `+1` 就是 EOS 缓冲，
  而被打断的会话根本没走到 EOS。这是本卡给判据的那条规则的实测依据。
- **`undelivered == dns`（6 == 6）。** `stop()` 落在帧边界上（收帧循环里 decide → release →
  记账在一圈内完成），所以它**不制造**未交付帧 —— 那 6 帧是因为面已经摘了（HOME 触发
  `surfaceDestroyed`），去向由 `frames_discarded_no_surface` 交代。
- **S2 在真机上第一次面对"被打断"的证据，是绿的。** 上一张卡没找到的"第三种出口"，
  在这一轮里仍然不存在 —— 但这次是**有报告可查**的不存在，而不是"没有报告所以不知道"。

### 2.2 对照组：默认档仍然什么都不写

同样的 6 秒切后台，**不带** `--es kivo.finalize on_stop`：

```
run-as com.kivo.cinema ls -l files/evidence/ | grep p14r-control
(没有 p14r-control 文件)
```

这条对照比正向那条更重要：它证明**产品行为没变**（后台继续播、不落盘），
落差全部来自那个探针开关。没有它，“我们加了 onStop”和“我们改了产品行为”在证据上分不开。

---

## 三、判据层挖出的三个洞（都不是本卡要找的，但都是本卡照出来的）

### 3.1 S2 读错了块 —— 解码报告的 `dns` 在 `counters` 里

S2 原来只读 `video.frames_discarded_no_surface`（节拍报告的布局），读不到就**按 0 处理**。
解码报告没有 `video` 块，那一格在 `counters.frames_discarded_no_surface` ——
于是第一份被打断的解码报告（差额 6）被判成"6 帧连去向都没有"：

```
✗ S2 …… 差额 6（197 决策 − 191 呈现 − 0 丢弃），而 frames_discarded_no_surface = 0
```

而那 6 帧的去向**就写在同一个 JSON 里**。这正是 S0 自己那句注释警告的事
（"缺格会被下游读成 0"）—— 只不过缺的是这一格，而且从来没人踩到，
因为所有解码报告的 `diff` 在"播完了"的时候本来就是 0。

修法：两处都找（`video` → `counters`），并在**两处都没有**时单独报"缺格"而不是"帧丢了"。
两种病因的修法完全不同，报错要说对方向。

### 3.2 `verify_p12c_sync.py` 只认 `/1` —— `/2` 之后的证据它一份都没判过

它的 schema 检查是 `if schema != "kivo.p12c.sync/1": raise`。也就是说
**四项闭合式（`rendered + dropped + dns + EOS == received`）在 187 份新证据上没有任何人判**。

扩到 `/29` 时它先报了两条，都不是证据坏了：

| 报错 | 真因 |
|---|---|
| `av_drift: 缺少 histogram` | `/3` 起分布搬进了 `quantiles.<口径>.histogram`，脚本还在读旧位置 |
| `KeyError('p50')` | 收尾摘要那一行还在摸顶层的 `drift['p50']` |

改成"每一份口径块都复算"（不只挑一份：两份分布若有一份被手改，另一份看不出来）。
代价与收益都写在这里：**这是第一次有判据真的复算 `/29` 的两份分布**。

### 3.3 播放速率那条上界，被打断的报告结构性地顶穿

`[0.98, 1.02]`。打断轮的实测是 **1.0316**，超界。但那不是"跑快了 3%"：

```
video_pts_span_ns    量的是「收到的」第一帧到最后一帧   → 197 帧
present_wall_span_ns 量的是「交付的」第一帧到最后一帧   → 191 帧
比值 = (197-1)/(191-1) = 196/190 = 1.0316   ← 与证据逐位相同
```

分子多算了 6 帧。所以**上界对非 `completed` 的报告不成立**，
但**下界仍然成立**：截尾只会让比值偏高，不会偏低 —— 跑慢了照样要红。
这条不是"放行"，是"把结构性的偏置说清楚，然后只保留有意义的那一侧"。

---

## 四、预言核对

| 预言 | 设计卡写的 | 实测 |
|---|---|---|
| 既有 sync 证据 `completed` 全为 true | 214 / 214 | ✅ 214 / 214 |
| 既有 decode 的 `completed=false && error==""` | 30 份，全是 `-t####` | ✅ 30 份，全是快照 |
| 打断轮 `received − (rendered+dropped+dns)` | **0** | ✅ **0** |
| 打断轮 `undelivered` | 等于 `dns` | ✅ 6 == 6 |
| 打断轮 `end_reason` | `interrupted` | ✅ |
| 打断轮 S2 | 绿（不预设） | ✅ 绿 |
| 完整轮改序之后 | `1800/1800/0/0` | ✅ 逐位相同 |
| 全量校验 | 656 份 | ❌ **658 份**（两轮各出两份报告，我少算了一倍） |

最后一条是我算错了，不是实现错了：一轮取证 = 解码 + 合并两份文件，两轮 = 4 份。

### 4.1 设计卡写了一条 S5，实现时拆成 S5/S6

理由：**在场**与**自洽**是两个不同的病，修法也不同（一个去补字段，一个去查派生逻辑）。
合成一条会让两种病报同一句话 —— 而 P14-Q 刚立过这条规矩（S2 让位给 S3）。
拆开之后，V1 只在 S5 上红、V2–V4 只在 S6 上红，各自有唯一可抓样本。

---

## 五、变异自测

`build/mutate_p14r_verifier.py`（不进仓库），**15 条全部按预期**：

| 编号 | 变异 | 红在哪 |
|---|---|---|
| V0 | 两轮原样 | 全绿 |
| V1 | `sync/29` 删掉 `end_reason` | S5，未牵连 |
| V2 | `sync/29` `completed=false` 却写 `completed` | S6，未牵连 |
| V3 | `sync/29` 写 `failed` 但 `error` 为空 | S6，未牵连 |
| V4 | `sync/29` 跑完却写 `interrupted` | S6，未牵连 |
| V5 | `sync/29` 写一个不存在的词（`paused`） | S5，未牵连 |
| V6 | `decode/19` 删掉 `counters.frames_discarded_no_surface` | S2（报"缺格"，不是"帧丢了"） |
| V7 | `decode/19` 从 `counters` 扣掉一帧 | S2 |
| V8 | `decode/19` 删掉 `outcome.end_reason` | 层 2「收尾方式缺场」 |
| V9 | `decode/19` 跑完却写 `interrupted` | 层 2「收尾方式自相矛盾」 |
| V10 | 打断的证据去掉证词 | 层 3「静默失败」+ 层 2「缺场」 |
| V11 | `sync/29` 打断轮谎称 `completed` | 四项闭合（EOS 的 `+1` 不成立） |
| V12 | `sync/29` 打断轮从 `dns` 扣一帧 | 四项闭合（差 1） |
| V13 | 打断轮速率跌到 0.95 | 速率下界 |
| V14 | 完整轮速率涨到 1.05 | 速率上界 |

V10 红了两条 —— 它不是一个改动牵连两条判据，是**一个改动造成了两处事实**
（字段没了、证词也没了）。这一点写清楚，免得下次有人拿它当"判据重叠"的证据调松一条。

---

## 六、数字

| 项 | 值 |
|---|---|
| `kivo_p14r_end_reason_tests` | **12 checks / 0 失败**（新目标） |
| `ctest -C Debug -R "kivo_p1[234]"` | **26/26**（原 25） |
| 全量 `verify_p12_all` | **658 份 / 四层通过 / 20 子校验器 exit 0** |
| `verify_p13d_drift` 覆盖 | **217 条**（213 份 sync 文件 + 4 组配对判） |
| `verify_p12c_sync` 回归 | 27 份 `/1` 证据全部仍通过 |
| 变异 | **15 条按预期，0 条不符** |
| Android | `assembleDebug` BUILD SUCCESSFUL |

---

## 七、诚实清单

1. **"切后台就收尾"不是产品行为。** 要写一份闭合的账就得先 `stop()`，而 `stop()` 结束会话 ——
   与 P12-D1 的"后台继续播"（`rebuilds=1` / `frames_discarded ≈ 空窗×30`）互斥。
   本卡只提供取证档 `--es kivo.finalize on_stop`，**要不要后台停播是产品决策**，
   而且必须先解决"回来从哪继续"（native 现在不对外暴露播放位置）。
2. **进程被杀仍然写不出报告。** 被杀就是没有边界、没有报告。那种情况只能靠
   `kivo.soak` 的中途快照留痕。本卡不碰。
3. **打断轮只跑了一轮**（n=1）。6 帧 `dns` 是"面摘掉到 stop 之间"的窗口，这个窗口
   随 HOME 与 `onStop` 的间隔而变 —— n=1 不足以断言它总是个位数。
4. **"第三种出口"仍然是"没找到"，不是"证明不存在"。** 现在能说的是：在 stop 路径上
   不存在（帧原子保证），在换面路径上由 `dns` 交代。**进程被杀那条路径没有证据**（见第 2 条）。
5. **`verify_p12c_sync.py` 只扩到 `/29`，没回头判 `/2`–`/28` 的 187 份。**
   那些证据现在仍然只有层 2 的粗筛（`rendered > received + 1` 那种），
   没有四项闭合。回头补判是一个独立决定（可能要动一批历史结论），不在本卡。
6. **速率那条的上界对非 `completed` 报告不判。** 这是**放行**，虽然理由是可复算的
   （`(received-1)/(presented-1)`）。若将来有判据需要它，得改成"减去尾部未交付帧再比"，
   而不是把区间放宽。
7. **`end_reason` 与 `completed` 是同一件事的两种说法。** 它们由同一个函数派生，
   所以"报告内部自洽"这件事在生成侧是免费的 —— 只有手改过的报告才可能矛盾，
   而 S6 抓的正是那种。

---

## 八、下一步（按价值排序）

1. **切后台的"回来从哪继续"** —— 现在打断收尾之后会话不再复活（`sessionEnded`）。
   要做成产品行为，得先给 native 一个 `position_us`，那是独立一项。
2. **把四项闭合回补到 `/2`–`/28`** —— 187 份证据的账目前只有粗筛。
   风险：可能翻出历史结论的问题；收益：那本账终于有人看。
3. **`I1` 在样本极少时的分支** —— `superseded/p14q-locked` 仍是一个可复现样本。
4. **32 位 Android** —— CM311-1A 是现成验证机，但需要 32 位 FFmpeg。
5. 桶宽细化（P14-O 遗留，让 D2 真正可判）。
