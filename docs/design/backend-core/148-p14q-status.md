# P14-Q 状态卡 · 节拍侧的账要能加

> schema：`kivo.p12c.sync/27` → `/28`
> 一张卡只答一个问题：**一帧被决策之后，它去哪了？**
> 结论：现在报告自己回答这个问题，而且有判据守着。**顺带推翻了上一张卡的一个论断。**

---

## 一、改了什么

| 位置 | 改动 |
|---|---|
| `core/pace/pace_stats.hpp` | `PaceSummary` 加 `undelivered_frames`（不带 `measured`） |
| `core/pace/frame_pacer.cxx` | `summary()` 里算 `decided − presented − dropped`，**不钳零** |
| `backends/avsync/report/av_sync_report.cxx` | 紧跟 `dropped` 印一格；`kSchema` → `/28` |
| `tools/verify_p13d_drift.py` | S0–S4；`SCHEMA_LATEST` → 28；`partial_note` 机制 |
| `tools/verify_p12_all.py` | 两张表登记 `/28` |
| `tests/.../frame_pacer_tests.cpp` | P14Q-1…4（四种走向） |
| `tools/run_p14q_frame_accounting.sh` | 新取证脚本 |

**核心层这一格是纯派生量，不是测量** —— 所以不配 `measured` / `reason`。
给一个纯算术配"我很确定"，是 P14-L 反的那类东西。

### 1.1 判据的版本闸门与别人不同

S0–S3 **对全部版本生效**，不设 `_schemas_from`：

```
213 份 sync 证据，三格（decided / presented / dropped）100% 齐全，
等式 213/213 闭合 —— 它从 `/1` 就成立。
```

按版本闸门会把 213 份覆盖白白丢掉，而 P14-M 整张卡的教训就是"别丢覆盖"。
只有 S4（"`/28` 起这一格必须在场"）按版本闸门。

### 1.2 顺带补了一个会吞结论的洞

`/1`、`/2` 原来在 `check_file()` 开头就 `return`，标 `not_applicable`。
而汇总里是：

```python
if report.not_applicable: skipped...; continue   # ← rows 整个被丢掉
```

于是"判据跑了、结论被吞"—— 比不跑更坏，输出上看不出区别。
新增 `Report.partial_note`：`/1`、`/2` 跑 S 组，并在输出里注明"本份只跑了这一组"。
**那 27 份 `/1` 里就有 4 份差额非 0 的合法样本**，S2 的"非零但合法"分支全靠它们。

---

## 二、真机证据

同一设备（Redmi K40 / b6e950ee）、同一片源（1080p30 / 60 s）、`cpu_paced`：

| 字段 | 完整播放 |
|---|---|
| `frames_decided` | **1800** |
| `presented` | **1800** |
| `dropped` | 0 |
| **`undelivered_frames`** | **0** |
| `frames_discarded_no_surface` | 0 |
| `frames_rendered` | 1800 |
| `lateness_ns.samples` | 1800 |

`1800 = 1800 + 0 + 0`，S0–S4 全绿，整份证据 `结论：全部通过`。

### 2.1 非零分支：锁屏那一轮（留档在 `superseded/`）

第一次取证时设备处于锁屏（`mDreamingLockscreen=true`），App 起来了但窗口不可见：

| 字段 | 值 |
|---|---|
| `decided` | 1800 |
| `presented` | **1** |
| `dropped` | 0 |
| `undelivered_frames` | **1799** |
| `frames_discarded_no_surface` | **1799** |

**`S2` 在这份上是绿的**：差额 1799 全部由"没有面可上"交代。

它留在 `superseded/` 的唯一原因是 `I1` 红（`Σ window_samples = 0` vs 序列长度 1，
呈现帧只有 1 个时分窗循环分不到点）—— 那是另一件事，记进遗留清单。

---

## 三、上一张卡的一个论断被推翻了

`146-p14p-status.md` §四写过：

> `84 + 0 ≠ 1800`，1716 帧凭空消失，而没有任何判据看 `presented + dropped == frames_decided`。

前半句（没有判据）**是对的，本卡补上了**。
后半句（"凭空消失"）**是推断，不是实测** —— 那份证据被 `rm` 掉了，`frames_discarded_no_surface`
当时到底是多少，**没人知道**。

而 §2.1 这一轮说明：**窗口不可见时，`frames_discarded_no_surface` 是会记账的**
（1799 帧一帧不差）。所以 P14-P 那轮的 1716 很可能也在 `frames_discarded_no_surface` 里 ——
**账本来就能加，只是没人算过**。

这不是给上一张卡开脱，恰恰是它最该记的一课：
**"判据缺席"的真实代价不是"一定有 bug"，而是"没有人知道有没有 bug"** ——
包括写状态卡的人自己。

### 3.1 所以"第三种出口"没有找到

本卡原本想找的是"决策了、没交付、且 `dns = 0`"的帧（会话在交付前结束）。
两次取证都没造出来：完整播放 `dns = 0`（差额也是 0），锁屏 `dns = 1799`（差额也是 1799）。

**没有反例不等于没有洞。** 现在的证据只能说：在已测的两种形态上，每一帧未交付都有原因。
判据 S2 从今天起会在出现第三种出口时红 —— 但它至今**没有在真机上红过**，
可杀死性只有 `build/mutate_p14q_verifier.py`（不进仓库）自证。

---

## 四、变异自测

`build/mutate_p14q_verifier.py`，两个底座（`p12d` 换面轮次 `/1` + `/28` 真机）：

| 编号 | 变异 | 结果 |
|---|---|---|
| V0 | `p12d` 原样（差额 15 有原因） | 仍绿 |
| V7 | `/28` 真机原样 | 仍绿 |
| V1 | 删掉 `presented` | S0 红，**未牵连其他** |
| V2 | 印出的 `undelivered` 加 1（源数不动） | S1 红，未牵连 |
| V3 | 抹掉 `frames_discarded_no_surface` | S2 红，未牵连 |
| V4 | 复刻 P14-P 的 1800/84/0 | S2 红，未牵连 |
| V5 | `presented` 抬到 181（超 `decided`） | S3 红，未牵连 |
| V6 | `/28` 删掉 `undelivered_frames` | S4 红，未牵连 |
| V8 | 负差额时 S2 是否让位并指名 S3 | 让位了 |

**6 红 / 2 绿 / 1 语义守卫，每条判据都有唯一可抓样本。**

### 4.1 第一版设计里 S3 和 S3b 是重复的

原本写了 `S3 已交付 ≤ 决策` 和 `S3b 未交付非负` —— 这两条是同一件事
（`delivered > decided` ⇔ `diff < 0`）。更糟的是 S2 会抢先红：负差额必然
`!= dns`（`dns >= 0`），于是 S3 永远无法单独红，报错还会指向"少记了原因"而不是"多记了帧"。

改法：**S2 在 `diff < 0` 时让位**，明说"病因在 S3"。
判据的重叠区必须写清 —— 这是 P14-P 那条教训（"唯一性不能靠巧合"）的第二次应用。

---

## 五、预言核对

| 预言 | 设计卡写的 | 实测 |
|---|---|---|
| 213 份历史 S2 全绿 | 213/213 | ✅ 213/213 |
| 差额非 0 的份数 | 4 | ✅ 4（全在 `p12d`） |
| 正常轮 `undelivered` | 0 | ✅ 0 |
| 切后台轮 `undelivered` | > 0 | ❌ **未取得**（报告没写出来） |
| 切后台轮 S2 | 不预设 | ❌ **未取得** |

第 4、5 条没兑现的原因不在预言本身：**切后台后进程被回收，报告没落盘**
（`cat: No such file or directory`）。锁屏那一轮替代它回答了"非零分支"，
但走的是换面路径而不是"会话提前结束"。**设计卡没改。**

---

## 六、数字

| 项 | 值 |
|---|---|
| `kivo_p12c5_pace_tests` | **479 项 / 0 失败**（新增 4 个用例） |
| `ctest -C Debug -R "kivo_p1[234]"` | **25/25** |
| 全量 `verify_p12_all` | **654 份 / 四层通过 / 20 子校验器 exit 0** |
| 覆盖 | 186 → **214**（+27 份 `/1`、+1 份 `/28`） |
| 变异 | 6 红 + 2 绿 + 1 守卫，全按预期 |
| Android | `assembleDebug` BUILD SUCCESSFUL |

---

## 七、诚实清单

1. **S2 从未在真机上红过。** 可杀死性只有变异脚本自证。"第三种出口"没找到 ——
   但那可能是"没造出来"，不是"不存在"。
2. **切后台那轮没写出报告**（进程被回收），所以"会话在交付前结束"这个场景
   仍然**没有真机证据**。锁屏那一轮走的是 `discarded_no_surface` 路径，不是这个。
3. **第二台设备装不上**：CM311-1A 只有 `armeabi-v7a`，工程只编 `arm64-v8a`。
   要覆盖它得加 32 位 ABI + 32 位 FFmpeg —— 那是独立的一项。
4. **设备锁屏是硬障碍**：`input swipe`、`wm dismiss-keyguard`、`input keyevent 82`、
   `locksettings set-disabled`、重启全部无效（MIUI 拦注入事件）。本次靠人工解锁才跑通，
   下次取证前必须先确认 `mDreamingLockscreen=false`。**脚本没有检查这一点** ——
   应该加，但加了也只是提前失败，不能自动解决。
5. **`superseded/p14q-locked-1080p30-sync.json` 的 `I1` 是红的**：
   `Σ window_samples = 0` vs 序列长度 1。呈现帧只有 1 个时分窗循环一个点都分不到。
   这是独立缺陷（很可能是判据对"样本 < 窗口数"没有分支），本卡不修。
6. **S1 在 `/1`–`/27` 上恒为"不可判"**（没有那一格）。所以它真正被验证过的证据
   只有本卡的 1 份 `/28`。覆盖率是 1 而不是 214 —— 那个 214 是 S0/S2/S3 的。
7. `undelivered_frames` 与 `closure_gap_ns` 是两个域：帧数与纳秒。别混读。

---

## 八、下一步（按价值排序）

1. **切后台的证据怎么拿到** —— 现在 `onStop` 不落盘。要么让会话收尾时强制写报告，
   要么用 `am start` 回前台触发。前者是产品行为，后者是取证技巧。
2. **`I1` 在样本极少时的分支** —— `superseded/` 那份已经有一个可复现的样本。
3. **32 位 Android** —— 第二台设备是现成的验证机，但要先有 32 位 FFmpeg。
4. **取证脚本加前置检查** —— `mDreamingLockscreen` 为 true 就别开始，
   否则会产出一批"窗口不可见"的假证据（本卡浪费了两轮）。
5. 桶宽细化（P14-O 遗留，让 D2 真正可判）。
