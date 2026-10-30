# 115 · P14-A 设计：补三条没有尺子的目标（G2 / G3 / G4）

- 日期：2026-10-31
- 前置：进度对账（G1–G10）—— **G2、G3 在 Android 上完全没有仪器，G4 只有外部 RSS 采样**
- 目标：三条计数器，把"不知道"变成"知道"，**不承诺它们一定达标**

---

## 一、为什么先做这三条

对账表里，G6/G7/G8/G10 是达标的，G1/G9 各有明确的缺口（175 vs 250 fps；加密不可检测）。
而 G2/G3/G4 的状态是**"未测"**——它们既不是达标也不是不达标，是**我们没装尺子**。

项目有一条规矩：**没有实测数字不许说"完成"**。那么反过来也成立：
**没有仪器的目标，永远只能写"未测"，写一百张功能卡也不会变成"达标"。**

这三条恰好都是"计数"性质，不需要改变播放行为，风险极低、收益是三条判据直接可判。

---

## 二、三条尺子分别是什么

### G2 · CPU 效率 → `cpu_time_facts`

| | |
| --- | --- |
| 判据原文 | 硬解 CPU 时间 ≤ 同片源软解的 **10%** |
| 能测的 | `CLOCK_THREAD_CPUTIME_ID` / `CLOCK_PROCESS_CPUTIME_ID` 的会话起止差 |
| 出版 | `thread_ns` / `process_ns` / `wall_ns` / `thread_ratio` / `process_ratio` |
| **分母缺失** | 这台机器上**没有软解路径**（Android 后端只做硬解），"相对软解 10%"算不出来 |

所以本卡出版的是**绝对占比**（CPU 时间 ÷ 墙钟），并把 G2 拆成两句：
`G2a` = CPU 占比是多少（本卡可判）；`G2b` = 相对软解是否 ≤10%（需要软解对照，留给后续卡）。
**把一句话拆成两句，比给一个算不出来的数更诚实。**

### G3 · 零堆分配 → `alloc_probe`

| | |
| --- | --- |
| 判据原文 | 解码+呈现稳态循环内，`operator new` 调用数 = 0 |
| 做法 | 全局替换 `operator new/delete` 的 8 个变体，转 `malloc/free` 并原子计数 |
| 稳态起点 | **首帧渲染之后**（`mark_steady()`）。启动期的 vector 扩容、编码器字符串复制不算"稳态" |
| 出版 | `calls_total` / `calls_steady` / `bytes_total` / `bytes_steady` |

为什么必须替换全局 `operator new` 而不是 `mallinfo`：`mallinfo` 只能看出"堆涨了多少"，
一个反复 new/delete 但总量不变的热路径在它眼里是**干净的**。判据要的是**调用次数**。

风险与约束（写进实现注释）：
- 只链进 **Android 的原生库**（`alloc_probe_hook.cxx` 单独一个静态目标），
  桌面单测不链它 —— 否则每个单测都会被迫带上这把尺子。
- 8 个变体必须齐（含 `nothrow` 与 `aligned`），少一个就是不同 TU 看到不同的分配器。

### G4 · 表面池不泄漏 → `buffer_lease_facts`

| | |
| --- | --- |
| 判据原文 | 1000 帧后池内 `in_use` 归还为 0，`allocated` 收敛 |
| **平台事实** | Android 的输出缓冲池**属于 codec**，不属于我们。我们看不到它的 `allocated` |
| 能测的 | `received / released / in_flight = received − released`；`leaked = in_flight` |
| 出版 | 三元组 + `pool_owner: "codec"` + `external_rss_source` 指向 soak 的 `dumpsys meminfo` 采样 |

**不能假装测到了平台的池。** 我们能保证的是自己的契约：
`released == received`（每个槽都还回去了），以及 `leaked == 0`。
平台的 `allocated` 用外部 RSS 曲线旁证，并在证据里写明那是外部采样、不是池读数。

> 写实现时踩过一次：第一版把 `leaked` 写成 `in_flight − 1`（想扣掉末尾 EOS），
> 于是**每一轮正确的会话都会报"泄漏 −1"**。EOS 在归还账里两边都在、正好抵消；
> 那个 +1 属于**帧账**（`received = rendered + dropped + 1`），不是归还账。
> 两个账混起来的代价是"仪器的默认输出就是错的"——那比没有仪器更坏。

---

## 三、落位

```
video_plane/core/metrics/            # 新族：三把尺子
├── cpu_time_facts.hpp/.cxx          # 读时钟 + 算占比（纯算术部分可单测）
├── alloc_probe.hpp/.cxx             # 计数与 arm/steady/mark（不含 hook）
├── alloc_probe_hook.cxx             # 全局 operator new 替换（**单独目标**，只在 Android 链）
└── buffer_lease_facts.hpp/.cxx      # 三元组 + 不变量
```

| 文件 | 单一职责 | 不做什么 |
| --- | --- | --- |
| `cpu_time_facts.cxx` | 采两次样、给三个数与两个比值 | 不判定是否达标 |
| `alloc_probe.cxx` | 原子计数与稳态标记 | 不替换分配器 |
| `alloc_probe_hook.cxx` | 替换 8 个分配/释放变体，只转发 + 计数 | 不含任何业务逻辑 |
| `buffer_lease_facts.cxx` | 由计数算在途与泄漏 | 不声称平台的池状态 |

接线：`DecodeStats` 增 `metrics` 块；`DecodeSession` 在 `start()` 采 CPU 起点并 `arm()`
分配计数、在首帧渲染时 `mark_steady()`、在 `snapshot()` 里算三组事实。
decode 证据 schema `decode/13` → `/14`，新增 `metrics` 对象。

---

## 四、判据（本卡验收）

| 编号 | 判据 | 说明 |
| --- | --- | --- |
| M1 | `metrics.cpu.wall_ns > 0` 且 `thread_ratio > 0` | 仪器接上了。比值为 0 只可能是没采样 |
| M2 | `metrics.cpu.process_ratio < 1.0` | 单线程播放的进程 CPU 不该超过墙钟；超过说明在忙等或多次重复解码 |
| M3 | 4K60 上 `alloc.calls_steady` **报出确定值** | 报 0 是结论，报 3000 也是结论。**本卡不要求它是 0** |
| M4 | `buffers.in_flight == 0` 且 `leaked == 0` | 每个输出槽都还回去了 |
| M5 | 三条都在**同一份 decode 证据**里 | 分三份文件就会被读成三次不同的运行 |

**M3 特意不写"= 0"**：写了就是先假定结论。G3 的判据是"= 0"，
但那是**验收**标准，不是**测量**标准。测出来多少先如实报，够不够 0 下一张卡再说。

---

## 五、诚实清单（开工前）

1. **G2 的分母仍然缺**。没有软解对照，`process_ratio` 只能自证"硬解占了 CPU 的百分之几"，
   证不了"是软解的 10%"。本卡不给 G2 打勾。
2. **分配计数在 Android 上可能数到别人的分配**。`operator new` 是进程级的，
   Kotlin/JNI 那一侧的 native 分配（如果有）会被一起数进来。
   缓解：只在 `arm()` 之后计数，且 `arm()` 发生在会话 start、`mark_steady()` 在首帧之后。
   但这仍然是"进程内的稳态"，不是"解码循环内的稳态"。
3. **aligned new 的替换有平台风险**。C++17 的 `operator new(size_t, align_val_t)` 若不替换，
   部分容器的分配会绕过计数（少报）。实现里全部替换，并在单测里各走一遍。
4. **G4 的三元组是"我们的账"，不是平台的池**。平台的 `allocated` 我们看不到，
   用 RSS 旁证 —— 而 RSS 里混着解码器自己的 native 堆，涨了也不能直接归因给池。
5. **三条都在 Debug 构建上测会偏大**（Debug 的分配比 Release 多）。
   证据必须写 `build_type`，否则下一轮换构建会得到不可比的数。
