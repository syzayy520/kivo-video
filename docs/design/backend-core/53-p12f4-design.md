# P12-F4 · 证据总收口（设计）

- 版本：V1.0
- 日期：2026-10-30
- 上游：`TASKS.md` F4、「`docs/evidence/p12c~p12f/` 齐备，可被第三方复算」
- 结论先行：**F4 的交付物不是一份总结，是一个能自己失败的校验器。**
  总结会过期，校验器不会。

---

## 一、现状盘点（收口前的真实状态）

| 项 | 数量 |
| --- | --- |
| 证据目录 | 13 个（p12a/b/c/d/d5/e/e3/e4/f1/f2/f2a/f3/f3b） |
| JSON 证据 | **235 份**（不含 `superseded/`） |
| schema 种类 | **24 种** |
| 已有校验器 | 6 个，**各自只校验自己那一组** |
| 一次跑完全部 6 个的地方 | **没有** |

这就是收口的真实痛点：**没有任何一次动作能回答"P12 整体还算不算数"**。
单看每组都是绿的，但没人知道把这 235 份放在一起是否自洽 —— 而"自洽"正是
可交付播放器与"跑通了一堆实验"的分界线。

---

## 二、为什么不是"写一份总结文档"

总结的问题在于：**它不会随着代码变化而变红。**

这个项目已经有过三次教训（E1 的 `color-format`、F2 的"落点对但画面冻住"、
F3 的"平台不报加密"），每一次都是**文档说对了、判据说错了**。所以 F4 的判据必须是
可执行的：跑一遍，绿就是绿，红就是红，红在哪一条一目了然。

推论：**F4 的核心产物是 `tools/verify_p12_all.py`**，`54-p12f4-status.md` 只是它的输出快照。

---

## 三、四层结构（每层各自能被证伪）

### 层 1 · 结构层（对全部 235 份，无例外）

| 检查 | 为什么 |
| --- | --- |
| 可解析为 JSON 对象 | 半截写入的文件要能被发现 |
| `schema` 存在且在白名单里 | 白名单 = 24 种已知 schema；**出现新 schema 就报错**，防止"悄悄改了格式没人知道" |
| 有实测时间戳（`generated_at_unix_ms` / `captured_at_host_ms` / `tag`） | 区分"真机产物"与"手写的示意片段" |
| 无占位符（`TODO` / `PLACEHOLDER` / `FIXME` / `xxx`） | 防止把草稿当证据 |

### 层 2 · 卡片层（12 组不变量）

每张卡一条，判据从各自的验收文档里抄，不新造：

| 卡 | 不变量 |
| --- | --- |
| A | `summary.decoder_records > 0` 且 `codecs` 非空 |
| B | `outcome.completed` 且 `frames_rendered > 0` 且首帧时延 `> 0` |
| C | decode 帧闭合（收到 ≥ 上屏）；sync 侧 `measurable=false` **必须**带 `unavailable_reason` |
| D | `surface_rebuild.rebuilds ≥ 1` 时 `resume_ms` 有值；多路证据三路齐全 |
| D5 | `summary.samples > 0`；`delta_unavailable_reason` 与 rows 的 delta 一致 |
| E1 | `color.verdict` 三态之一；`indeterminate` **必须**带 reason |
| E2 | `hdr.kind` 与 `dataspace.applied` 一致（HDR ⇒ 设了；SDR ⇒ 没设） |
| E3 | 图层 dataspace 名字与 `hdr.kind` 一致（HDR ⇒ BT2020+PQ） |
| E4 | `tonemap.implemented == false` **且** reason 非空（不假装支持） |
| F1 | `seek.total_us ≤ 150000` 且 `requests ≥ 1` |
| F2/F2A | `av.gap_us` 绝对值 ≤ 40000、`av.pass`、`catch_up_frames == 0`、`fallback_frames == 0` |
| F3 | 四条负向用例 PASS（复用 `verify_p12f3_errors.py`，不重写） |

### 层 3 · 红线层（跨全组的四条"不说瞎话"）

这四条与具体卡片无关，**任何一份证据违反就算整体红**：

1. **零静默失败**：`completed == false` 或 `open.ok == false` 时，`error` / `reason` 必须非空；
2. **零无解释**：`measurable == false`、`applicable == false`、`ok == false` 时必须带原因；
3. **零含糊**：三态字段（`color.verdict` / `hdr.kind` / `encryption.verdict` / `seek.pass`）
   必须是已知取值，不许出现 `null` 或空串；
4. **零恒真字段**：`landing_us == target_us` 精确相等**且**片源关键帧间隔未知时，给出提示
   （F1 踩过的坑：`seek_to` 恒返回请求值，那个字段永远是"对的"）。

### 层 4 · 复用层（6 个既有校验器）

**不重写判据**，直接子进程调用并收集退出码。重写会制造"两套判据"，而两套判据
必然在某天分叉 —— 那时没人知道该信哪一套。

参数各不相同的处理：文件列表型的传 glob 展开结果；参数型的两条（`multi` / `soak`）
按固定命名拼参数，命名对不上就**报错而不是跳过**（"跳过"会让收口悄悄变松）。

---

## 四、自检（`--self-test`）：证明它能打红

一个从不失败的总校验器比没有更糟。所以内置 8 个变异，全部作用在**内存深拷贝**上，
不碰磁盘：

| 变异 | 必须被哪一层抓到 |
| --- | --- |
| 删掉 `schema` | 层 1 |
| `schema` 改成 `kivo.unknown/9` | 层 1 |
| 塞一个 `"reason": "TODO"` | 层 1 |
| `completed=false` 且 `error=""` | 层 3 |
| `av_drift.measurable=false` 且 `unavailable_reason=""` | 层 3 |
| `hdr.kind="hdr10"` 但 `dataspace.applied=false` | 层 2 |
| `seek.total_us=999999` | 层 2 |
| `tonemap.implemented=true` | 层 2 |

`--self-test` 的判据：**8 个变异必须全部被抓到**；抓到 <8 个就以非 0 退出。
这是"校验器的单测"。

---

## 五、输出

```
=== P12 证据总收口 ===
层 1 结构      235 份 / 235 通过
层 2 卡片层    12 组 / 12 通过
层 3 红线      4 条 / 4 通过
层 4 复用      6 个校验器 / 6 退出码 0
---
结论：P12 可交付
```

失败时逐条打印 `[FAIL] 文件: 判据: 实际值`，并给非 0 退出码。
`--json` 输出机读版，供 CI 或后续自动化消费。

---

## 六、边界（写清楚）

1. **它证明"证据自洽"，不证明"画面好看"。** 花屏/色彩正确性仍是人眼判据（E4 §五 第 1 条）。
2. **层 2 的阈值来自各卡验收文档**，改判据要同步改这里，否则收口会与卡片脱节。
   这条是维护成本，写在文档里而不是藏起来。
3. **白名单是硬编码的 24 种 schema**。新增卡片必须显式加进 `KNOWN_SCHEMAS` ——
   这是刻意的摩擦：让"格式变更"必须被看见一次。
