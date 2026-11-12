# D6「清除长期代码里的参考产品名痕迹」· 第二轮独立复审报告

- 复审员：第二轮独立复审员（**不采信** 执行者 `derex_apply.py` 的任何判词，也不采信 `README.md` 的任何数字，全部自己复算）
- 审计时点：2026-11-13（本地），设备无关，**只读**
- 仓库根：`C:\kivo video`；所有 git 命令均带 `-c safe.directory="C:/kivo video"`
- 基线：`f97dd2ea`（D6 开工提交，固定）；当前 HEAD：`1643a141`（262 卡，`@{u}..HEAD = 0`）
- **唯一写入文件：本 `audit-round2.md`**。未改任何源码、未改 `audit.md`（保留第一轮原貌）、未 `git add/commit`、未推送、未触碰两个 WIP 的内容、未替用户提交
- 复算方式：一律 `PYTHONUTF8=1 python - <<'PY' ... PY` **内联执行、零落盘**；`git archive --format=tar` 经 `tarfile` 在内存解包；import 测试加 `PYTHONDONTWRITEBYTECODE=1` 以免生成 `__pycache__`；语法检查用内存 `compile()` 而非会写 `.pyc` 的 `py_compile`
- 扫描范围（用户拍板「目录全量」）：`backend/`、`include/`、`android/app/src/main/`、`tools/`；排除 `tools/reference/`、顶层 `docs/`、顶层 `artifacts/`（冻结归档）；`backend/tools/governance/.../artifacts/` 属 backend 子树，纳入
- 独立口径：自己实现 `in_scope(rel)`（仅排除上述前缀），并同时跑两种口径：
  - **FULL**＝真·目录全量（不限扩展名，跳过不可 UTF-8 解码的二进制文件）
  - **EXTS**＝执行者 `SCAN_EXTS`（`.kt/.cpp/.cxx/.hpp/.h/.py/.txt/.json` + `CMakeLists.txt`）
  - 独立分类器 `myclass()` 用自写 `PRODUCT`/`WHITELIST` 正则，与执行者 `classify()` 对撞

---

## 0. 结论速览

**最终 verdict：`PASS_WITH_DISCLOSURE`**

就 D6 本卡的四条技术闸门而言，独立复算结果：

| 闸门 | 结果 | 关键实测 |
| --- | --- | --- |
| ① 语义零残留（产品名 / 出处署名） | **PASS** | 清理后 FULL 口径 2455 文件 **18 HIT / 0 product / 0 unknown / 18 whitelist**；宽帧号 `\b0(?:16\d|17\d)\b` **0 命中**；`0170` 全仓 0 命中；18 行白名单无产品名夹带 |
| ② 编译 | **PASS（日志层面）** | `build.log` L37 `> Task :app:compileDebugKotlin` 无 `UP-TO-DATE`（真实执行）、L47 `BUILD SUCCESSFUL in 6s`、L48 `6 executed, 34 up-to-date`；独立复现未验证（P3 披露） |
| ③ 不改逻辑 | **PASS（附披露）** | 复审 T1 快照 `git diff HEAD` = **55 文件 / 144+ / 143-**，全部 `M`；除两类允许例外与两个 WIP 的等价标识符改名外，**无函数签名 / 依赖 / 常量 / 条件逻辑改动**；第一轮的 275 行可执行改动已随 262 提交离开工作区。收尾 T2 因并发 WIP 再次写入变为 57 文件 / 177+ / 143-（新增 2 文件属并发 WIP 非 D6，见 §7 第 6 条） |
| ④ 证据可复算 | **PASS** | 纯提交基线 **145 / 127 / 18 / 61** 独立复算一致、`grep_before.txt` 逐行 0 差异；144 条规则 `rule_problems=0`；`verify_result.json` 与实际完全一致；`apply` 幂等写 0 文件且前后 hash 完全相同；复算链 3 脚本可 import |

**为何不是纯 `PASS`**：存在必须如实披露、但不构成 D6 技术闸门失败的环境项（并发 262 提交 / 提交口径未决 / 真机与编译独立复现未验证）与 6 条 P3 级披露项。**为何不是 `FAIL`**：第一轮的全部 P0/P1/P2 阻断项已逐条落地并经独立实测确认；用户明确要求不得因「并发提交 / 提交口径未决」把技术闸门判成 FAIL，本复审遵此，且**不替用户决定提交**。

残留问题分级：**P0 = 0，P1 = 0，P2 = 0；P3（披露级）= 6**（见 §11）。

---

## 1. 独立扫描清理后长期代码并按语义分类（要点 1）

### 方法与实际命令
内联 Python（零落盘）遍历 4 个范围目录全部文件，逐行：
- `HIT = re.compile(r"rex|senplayer", re.I)` 找命中；
- 自写 `PRODUCT = REX[ -]?\d|REX-EMBY|RexDesign|SenPlayer|\bRex\b(?![-/])|rex_browse|measure_rex|rex-replication|rex-playback|rexTextWeight`（`re.I`）判违规；
- 自写 `WHITELIST = ShaderExecution|barExit|Execution|docs/reference/rex-teardown|artifacts/rex-measurements\.txt|tools/reference/rex|rex-measurements|236-ui-align-rex-design\.md`（`re.I`）判白名单；
- 宽帧号 `WIDE = \b0(?:16\d|17\d)\b`（0160–0179）单独扫；另用更宽 `FRAME_ALL = M\d{2}-S\d+|\bsk_\d+\b|IMG_\d+|frames\d+/|(?<![\d.])0(?:16\d|17\d|18\d)(?![\d])` 扫全部出处编号；
- 单独统计含字面 `0170` 的行；
- 对 FULL 与 EXTS 两口径各跑一遍，并输出「FULL 含、EXTS 排除、且含 rex/senplayer」的文件集合。

等价复现命令：`git -c safe.directory="C:/kivo video" status`（确认工作区）、`PYTHONUTF8=1 python - <<'PY'`（内联遍历 + 上述 5 个正则）。

### 实测数字

| 口径 | 扫描文件 | HIT 行 | product | whitelist | unknown | WIDE(0160–0179) | 含 0170 行 | FRAME_ALL |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| **FULL**（真目录全量） | **2455** | **18** | **0** | **18** | **0** | **0** | **0** | 6 |
| **EXTS**（执行者 SCAN_EXTS） | **2396** | **18** | **0** | **18** | **0** | **0** | **0** | 6 |

- FULL − EXTS = 59 个文件，**其中含 `rex|senplayer` 的文件 = 0** → 即使用户拍板的「目录全量（不限扩展名）」口径，结论与执行者收窄口径完全一致，口径差异不影响闸门。
- EXTS 扫描文件数 **2396** 与 README 表头「清理后工作区 2396」一致。
- `grep_after.txt`（18 行）与独立 FULL/EXTS 命中**逐行完全一致，0 差异**。

### 18 行白名单逐行核对（无夹带）

| 组 | 行 | 文件:行 | 命中片段 | 语义判定 |
| --- | --- | --- | --- | --- |
| A | 1–5 | `PlayerBarsHost.kt:63/80/117/119/120` | `barExit(...)` / `barExitMs` | `barExit`（b-a-**rEx**-it）子串误报 ✓ |
| A | 6 | `PlayerMotion.kt:28` | `const val barExitMs = 140` | 同上 ✓ |
| B | 7 | `PlayerTokens.kt:11` | `artifacts/rex-measurements.txt` | 冻结证据归档路径（复算链依赖）✓ |
| C | 8–10 | `p5h_color_hdr_protected_tests.cpp:83/86/98` | `ShaderExecutionOrder` / `FakeShaderExecutionPlanner` | `ShaderExecution`/`Execution` 子串误报 ✓ |
| C | 12–17 | `include/.../shader_execution_order.hpp:19/25/28/39/52/60` | `ShaderExecutionStep` / `FakeShaderExecutionPlanner` | 同上 ✓ |
| D | 18 | `tools/dump_home_spec.py:6` | `docs/reference/rex-teardown/rex-package/strings/` | 冻结参考功能路径（脚本功能依赖）✓ |
| **E** | **11** | `backend/tools/governance/redline_gate/artifacts/redline_gate_result.json:39` | `"file": "...\\236-ui-align-rex-design.md"` | 治理闸门生成物，引用冻结设计稿名（P2 修复主动纳入白名单）✓ |

计数 6+1+3+6+1+1 = **18**，与 README §三 白名单表一致。**结论：18 行全部为子串误报或冻结参考功能路径，无产品名 / 出处署名夹带。** 第 11 行是唯一新增项：它是第一轮 P2 指出的 `.json` 漏扫行，执行者通过「`SCAN_EXTS` 加 `.json` + `WHITELIST` 加 `236-ui-align-rex-design\.md`」主动纳入并列入白名单，属**白名单扩展**而非夹带；该行 `rex` 来自设计稿文件名（冻结归档文档名），非独立产品名，且改它会破坏既有治理闸门证据——处理合理，但须持续披露（P3-3）。

### 0170 与宽帧号范围核对
- `PlayerBottomBar.kt:38` 当前为 `* · 选集态（点选集）：整条换成横滑选集带（实测）。`；L37 亦为 `（实测）`（原 `0169 实测`）。**（0170 实测）全仓 0 命中、`0170` 字面全仓 0 命中 → 0170 已清 ✓**
- `WIDE = \b0(?:16\d|17\d)\b`（0160–0179）在 FULL/EXTS 两口径均 **0 命中** → 宽帧号范围内零残留 ✓
- 执行者 `FRAME_IDS` 正则当前工作区 **0 命中**（见 §4），`frame_ids_after.txt` 为空（1 空行）与实际一致。
- `FRAME_ALL` 的 6 行（FULL 口径）逐行列出：
  ```
  SourceOptionalFields.kt:25   …（243 M04-S03，0.3s easeInOut）。
  TmdbHome.kt:9                …（239 §2；节序 = IMG_0136 全链实测）：
  DiscoverPage.kt:180          …（M04-S04，不可合并）
  DiscoverPage.kt:321          …（M04-S04）。
  rewrite_discover.py:14        …（M04-S01，自动 5s）
  rewrite_discover.py:120       …（M04-S01 动效参数…）
  ```
  全部为 `M04-Sxx` 与 `IMG_0136`——README §三 明确保留的项目自有编号，不含产品名、不属参考录屏帧号系列（`0160/0169/0170/0171/0172/0178/0179` 已清）；执行者 `FRAME_IDS` 亦不覆盖 `M04`。按第一轮一致口径标 **低风险 / 待确认（P3-4）**，不计违规。

### 残留问题（分级）
- **P3-2**：README §一 称「目录全量」，但 `derex_apply.py:eligible()` 实为扩展名白名单（`SCAN_EXTS`）过滤；真目录全量为 2455 文件、EXTS 为 2396。结论不受影响（两口径数字相同），属措辞不严谨。
- **P3-3 / P3-4**：见上（治理闸门 `.json` 白名单、6 行自有编号）。

---

## 2. 独立重建清理前基线并核对 README 数字（要点 2）

### 方法与实际命令
```
git -c safe.directory="C:/kivo video" archive --format=tar f97dd2ea
```
`subprocess.check_output` 取字节 → `tarfile.open(fileobj=io.BytesIO(raw))` **内存解包** → 对每个 `isfile()` member 规范化 `./` 前缀后过自写 `in_scope()` → `decode("utf-8","replace")` → 逐行用 §1 的 `HIT`/`PRODUCT`/`WHITELIST` 分类；帧号用执行者修订后的 `FRAME_IDS = M0[38]-S\d+|\bsk_\d+\b|IMG_0161|frames2/|\b0(?:16\d|17\d)\b` 与更宽 `FRAME_ALL` 各算一次。FULL / EXTS 两口径各算。另把重建 EXTS 命中集合与 `grep_before.txt`、`frame_ids_before.txt` 做集合差。

### 实测数字

| 口径 / 帧号正则 | 扫描文件 | HIT | product | whitelist | unknown | 帧号行 |
| --- | --- | --- | --- | --- | --- | --- |
| **EXTS / FRAME_IDS（修订口径，README 用）** | **2384** | **145** | **127** | **18** | **0** | **61** |
| EXTS / FRAME_ALL（第一轮宽正则） | 2384 | 145 | 127 | 18 | 0 | 67 |
| FULL / FRAME_IDS | 2446 | 145 | 127 | 18 | 0 | 61 |
| FULL / FRAME_ALL | 2446 | 145 | 127 | 18 | 0 | 67 |

- 与 README §三「清理前 `f97dd2ea` 纯提交：**扫描文件 2384 / 145 / 127 / 18 / 0 / 61 行帧号**」**逐项完全一致 ✓**
- FULL 口径（2446 文件）比 EXTS 多 62 文件，但 HIT 仍为 145 → 清理前基线的 4 个数字**不受口径影响**。
- 宽 `FRAME_ALL` 得 **67** 行，与第一轮 `audit.md` §1 的 67 一致；修订 `FRAME_IDS` 排除 `M04-Sxx` 自有编号后为 **61** → README 的 61 成立。
- `grep_before.txt` **145 行**，与重建 EXTS 命中集合「仅在重建 0 / 仅在文件 0」→ **逐行完全一致**；`frame_ids_before.txt` **61 非空行**，与重建 `FRAME_IDS` 61 行一致 ✓。

### 「清理前工作区 137/120」差异是否如实披露
**是，已如实披露。** README §三 第 42–46 行明确写出双口径：
- `145 / 127` = 纯提交 `f97dd2ea`（本表，可复算）；
- `137 / 120` = 清理前**实际工作区**（含主助手 2 个未提交 WIP，已自行清掉 7 行），属**第一版口径**（`SCAN_EXTS` 未含 `.json`、帧号正则较窄，故另有 `58` 行帧号）；
- 并写明「第一版 `audit.md` 中『137/120 污染基线』的批评，已由固定基线 + 双口径披露解决」。

补充核实：`derex_apply.py:30` 已 `BASELINE_COMMIT = "f97dd2ea"`（固定、不随 HEAD 漂移）；`head_snapshot()` docstring（L426–428）也写明纯提交 144/127 与清理前工作区 137/120 的由来。`137/120` 是**历史工作区态**（当时两个 WIP 的工作区版本此后又被改动），无法从当前仓库独立重建，但 README 已标注其口径、来源与差异原因，不构成隐瞒或基线污染。

### 残留问题（分级）
- 无 P0/P1/P2。P3-2（口径措辞）见 §1。

---

## 3. 逐条验证第一轮 P0/P1/P2 修复落地与改写质量（要点 3）

### 方法与实际命令
直接读实际源码文件（不只读 RULES 文本）：`Read` + 内联 `dump(rel, a, b)` 打印带行号原文；关键词全范围扫描（`旧内核 / 语义色板 / 链路独立 /（0170 实测）/ 0170 / 【竖向拖动：亮度/音量】。/ REX M08-03 / 1.8s 后自动消失 / rexTextWeight / barTextWeight / REX 0172`）；并用 `git show f97dd2ea:<file>` 与当前工作区逐行对比，判断术语来源。

### 第一轮问题逐条落地核验

| 第一轮问题 | 级别 | 执行者修复 | 独立实测（读实际文件） | 判定 |
| --- | --- | --- | --- | --- |
| `PlayerBottomBar.kt:38 （0170 实测）` 漏清；`FRAME_IDS` 缺 0170 | P0 | RULES 加 `（0170 实测）→（实测）`；`FRAME_IDS` 放宽 `\b0(?:16\d|17\d)\b` | L38 = `（实测）`；全仓 `0170` = 0；WIDE = 0 | ✅ 落地 |
| 清理前用「HEAD+2 WIP」混合态、随 HEAD 漂移 | P0 | `BASELINE_COMMIT="f97dd2ea"` 固定 | 重建 145/127/18/61 可复算（§2） | ✅ 落地 |
| `verify` 因 `PlayerActionRouter.kt` 注释被并发送改而 FAIL | P1 | 该规则标 `SOFT_RULES`，只校验原文消失 | 当前 L247 见下；`rule_problems=0`（§4） | ✅ 落地 |
| diff 混入 262 卡功能改动 /「60 文件 160/159」不可复算 | P1 | 改如实披露，不再引用旧 stat | T1 快照 diff 55 文件 / 144+ / 143−（收尾 T2 漂移为 57/177+/143−，见 §7 第 6 条） | ✅ 落地（披露） |
| 目录全量漏 `.json`、治理闸门 `rex-design` 一行未纳管 | P2 | `SCAN_EXTS` 加 `.json`；`WHITELIST` 加 `236-ui-align-rex-design\.md` | `redline_gate_result.json:39` 判 whitelist、在扫描内（§1） | ✅ 落地 |
| 4 处改写质量（叠词 / 语义反转 / 无谓语残句 / 去主语） | P2 | 4 处全部重改 | 逐条见下 | ✅ 落地 |

### 4 处改写后的实际文本（逐条读）

1. **`DesignTokens.kt:23`** → `// 语义色板（浅色态实测硬参数；深色态 v2）`
   第一轮叠词态为「语义色板（浅色态**语义色板**硬参数…）」。现全仓「语义色板」仅 1 处（行首标题），括号内改为「浅色态实测硬参数」，**叠词消除、语义通顺 ✓**。
2. **`PlaybackEngine.kt:10-11`** →
   ```
   *  · **libmpv 内核**（253）—— 单一 demux 线程 + 自己的时钟 + 全 FFmpeg 解码覆盖。
   *    单一 demux 线程，单一读路径。
   ```
   清理前 L11 = `与 SenPlayer / Rex 同路。`；第一轮中间态被改成「链路独立，不与旧内核共用读路径」（`同路 → 独立` 语义反转）。现文改为描述 libmpv 内核特性，**「链路独立」全仓 0 命中 → 语义反转已消除 ✓**。
   - **术语澄清（对第一轮表述的修正）**：第一轮称「凭空新增无来源概念『旧内核』」。实测 `git show f97dd2ea` 中「旧内核」已存在于 6 个文件（CMakeLists.txt 2 / LocalRangeServer.kt 3 / MpvNative.kt 3 / MpvSpeedController.kt 1 / MpvPlaybackSession.kt 4 / PlaybackEngine.kt 2 = 15 处），**清理前后计数完全相同** → 「旧内核」是项目 253 卡固有的架构术语（自研 FFmpeg demux 内核，与 libmpv 新内核相对），非 D6 新造、不含 `rex|senplayer`、不违反语义判据。第一轮批评的实质（语义反转）成立且已修复，但「凭空引入旧内核」的表述与事实不符，此处如实更正。
   - **P3-1（轻微）**：L10 已写「单一 demux 线程」，L11 又写「单一 demux 线程，单一读路径」，**重复用词**；L11 为并列名词短语（中文技术注释中作补充说明可接受）。**无残句、无语义反转**，不构成阻断。
3. **`PlayerGestureLayer.kt:69`** → `// 【竖向拖动：亮度/音量】左半屏调亮度，右半屏调音量。`
   第一轮残句 `// 【竖向拖动：亮度/音量】。` 全仓 0 命中；现为完整句（主语隐含「竖向拖动」+ 谓语「调」）✓。
4. **`PlayerGestureFlash.kt:26`** → `* 交互反馈用一个居中的图标闪 0.5 秒来确认 —— 这是"零成本的确认"，不打断观看。`
   第一轮去主语态 `* 用一个居中的图标闪 0.5 秒来确认` 已补主语「交互反馈」，语法完整 ✓。

**结论：4 处改写后无残句、无语义反转残留**；仅 `PlaybackEngine` 有 P3-1 轻微重复用词。

### `SOFT_RULES` 那条注释当前文本
`PlayerActionRouter.kt:247`：
```
/** 轻提示：默认 1.8s 后自动消失（落在 1.0–2.0s 停留区间）；起播规格等可指定停留时长。 */
internal fun PlayerComposeActivity.showToast(message: String, durationMs: Long = 1_800) {
```
逐项核对：**不含 `REX` / `SenPlayer` / `REX M08-03` / `0172` / 任何产品名或出处编号**（`REX M08-03` 全仓 0、`REX 0172` 全仓 0）。`落在 1.0–2.0s 停留区间` 为数值描述、`起播规格` 为项目术语。→ `SOFT_RULES`「并发改写后现文本满足语义判据」的前提**成立 ✓**。同时该函数带 `durationMs` 形参，是 262 卡功能改动，已随 `1643a141` 进入 HEAD（见 §6/§7），非工作区遗留。

### 残留问题（分级）
- **P3-1**：`PlaybackEngine.kt` L10/L11 重复「单一 demux 线程」。
- 其余无。

---

## 4. 独立判断 `verify` 能否 PASS / `apply` 幂等 / JSON 一致性（要点 4）

### 方法与实际命令
- 内联 `ast.parse(derex_apply.py)` + `ast.literal_eval` 静态取 `RULES` 与 `SOFT_RULES`（**不执行脚本、不 import 其判词**），逐条独立核对当前工作区：`old in text`（原文消失）、`new in text`（新文落地，soft 除外）；并预测 `apply` 幂等（列出所有 `old_count ≠ 0` 的规则）。
- 用执行者 `FRAME_IDS` 正则独立扫当前工作区，核对 `verify_result.json: frame_id_lines`。
- 实跑 `python docs/evidence/p260/code-derex/derex_apply.py apply`，跑前/跑后用内联 `hashlib.sha256` 对「4 范围目录 + code-derex 目录全部文件」算聚合 hash，并对比 `git status --porcelain` 与 `git diff` 的 hash。

### 实测数字
- `RULES len = 144`，`distinct files = 58`；`SOFT_RULES = 1` 条（`PlayerActionRouter.kt` 的 toast 注释）。
- 独立逐条核对：**`rule_problems = 0`**（原文残留 0、新文未落地 0）→ **`verify` 在当前工作区可 PASS**。
- 幂等预测：`old_count ≠ 0` 的规则 = **空集** → `apply` 应写 0 文件。
- **实跑 `apply`**：`exit code = 0`；stdout = `[OK] 144 条改写规则全部唯一命中，已写入 0 个文件`；stderr 空。
- **hash 快照（2530 个文件）**：

| 项 | BEFORE | AFTER | 一致 |
| --- | --- | --- | --- |
| 聚合 SHA256 | `eb2b8e0c0570b1683ef24f4b61b009d7a3d2a44fefaea91919c9573a2c737b05` | 同 | ✅ |
| `git status --porcelain`（57 行） | `f7c944e7…` | 同 | ✅ |
| `git diff` 内容 hash | `f15cc76d1ef84d70003bfc9864b6da17e65a7dfdbf9187467dd74c9e2a1ce2f0` | 同 | ✅ |
| 文件数 | 2530 | 2530 | ✅ |

→ **`apply` 幂等成立，写 0 文件、未改动任何源码 / 证据文件** ✓（本复审全程唯一写入仍为本报告）。

- 当前工作区 `FRAME_IDS` 独立扫描 = **0 命中** → `verify_result.json: "frame_id_lines": []` 与实际一致。
- `verify_result.json` 逐字段核对：

| 字段 | 文件值 | 独立复算 | 一致 |
| --- | --- | --- | --- |
| `rule_count` | 144 | 144 | ✅ |
| `soft_rules_relaxed` | 1 条（PlayerActionRouter.kt） | 1 条 | ✅ |
| `rule_problems` | `[]` | 0 | ✅ |
| `hit_lines` | 18 | 18 | ✅ |
| `counts` | product 0 / whitelist 18 / unknown 0 | 同 | ✅ |
| `violations` | `[]` | 0 | ✅ |
| `frame_id_lines` | `[]` | 0 | ✅ |
| `verdict` | `PASS` | PASS | ✅ |

**结论：`verify_result.json` 与当前工作区实际完全一致、可复算**；README「修复后 `verify` 复算：144 条规则（含 4 条修复规则）、0 问题、PASS；`apply` 幂等写 0 文件」独立复算成立。

### 残留问题（分级）
- 无 P0/P1/P2。`snapshot before/after`、`verify` 三条命令会覆写证据文件，按只读铁律未原样执行，改以上述等价内联脚本复算（结果一致）。

---

## 5. `build.log` 编译证据与复算链（要点 5）

### 方法与实际命令
`Read build.log` 全文；内联 `compile(p.read_bytes(), rel, "exec")` 做内存语法检查；`PYTHONDONTWRITEBYTECODE=1` 下 `sys.path.insert(0, 'tools')` 后 `import measure_baseline_playback` / `import measure_kivo_playback`；正则提取文档中 `tools/[\w./-]+\.py` 并查存在性。

### 实测数字
- **`build.log`（49 行，已被重写为修复后重跑的日志）**：
  - L37 `> Task :app:compileDebugKotlin` —— **无 `UP-TO-DATE` / `FROM-CACHE` / `NO-SOURCE` 后缀，按 Gradle 输出惯例为真实执行**；
  - L41 `dexBuilderDebug`、L42 `mergeProjectDexDebug`、L43 `packageDebug`、L45 `assembleDebug` 同样无 `UP-TO-DATE`（增量编译链真实重跑）；
  - L47 `BUILD SUCCESSFUL in 6s`、L48 `40 actionable tasks: 6 executed, 34 up-to-date`。
  - **日志文本层面成立 ✓**。与第一轮 `audit.md` §4 引用的「L79 compileDebugKotlin / L92 BUILD SUCCESSFUL in 22s / L93 6 executed」为**不同版本**：执行者在审计后重写了 `build.log`（mtime 04:47）并重跑编译，如实记录。
- **复算链**：
  - `tools/measure_baseline_playback.py`、`tools/measure_kivo_playback.py`、`docs/evidence/p260/code-derex/derex_apply.py` 三者 `exists = True`，内存 `compile()` 语法 **OK**；
  - `import measure_baseline_playback` **OK**（导出 `P / clusters / longest_run / icons / bottom / panel / text / np / Image` 等）；`import measure_kivo_playback` **OK**（导出 `baseline / D / ...`）；`numpy` / `PIL` 依赖齐备，无环境限制；
  - `player-ui-v3-rex-replication.md` L9 复算命令已指向 `tools/measure_baseline_playback.py`（存在）；L10 补的文件名映射说明提到旧名 `tools/measure_rex_playback.py`（已不存在，作为历史说明）；L11 `rex-grid-0160/0163/0169/0170/0171.png` 佐证 0170 属 REX 帧号系列——该文件在顶层 `docs/` 冻结归档，不在清理范围；
  - `tools/measure_rex_playback.py` 在 `f97dd2ea` 存在、在 `HEAD` 不存在（`R100` 重命名已随 262 提交进入 HEAD）。

### 残留问题（分级）
- **P3-5**：`assembleDebug` 编译为静态日志证据，本复审不跑 gradle、无法独立复现；且日志对应时刻 T 的代码非当前漂移后工作区。按铁律标「日志层面 PASS / 编译复现未验证」。
- **P3-6**（归属 §7 环境披露）：真机 K40 四页像素差 / 语义树全等 / `adb install -r` 等为二进制静态证据，README §六 自承 A/B 非同一构建基线、取证改变应用状态、播放页 dump 无文本节点 → 标「未验证」，不作为技术闸门判据。

---

## 6. 复核当前 `git diff` 分类与两个 WIP（要点 6）

### 方法与实际命令
```
git -c safe.directory="C:/kivo video" diff HEAD --name-status
git -c safe.directory="C:/kivo video" diff HEAD --numstat / --stat
git -c safe.directory="C:/kivo video" diff HEAD -U0 --no-color   # 内联解析，按 -/+ 行前缀分类
git -c safe.directory="C:/kivo video" diff f97dd2ea HEAD --name-status   # 262 提交范围
git -c safe.directory="C:/kivo video" cat-file -e <ref>:<path>            # 文件重命名核实
git -c safe.directory="C:/kivo video" show <ref>:<WIP>                    # 两个 WIP 提交态内容
```

### 实测数字（含时序注记）
- **T1（本项复核时）**：`git diff HEAD --stat` = **55 files changed, 144 insertions(+), 143 deletions(-)**；`--name-status` 全部为 `M`，**无 A / D / R**（重命名 `R100` 已随 262 提交进入 HEAD）。下文 (a)–(d) 的分类基于 T1 快照。
- **T2（复审收尾复核时）**：`git status --porcelain` 由 57 行增至 **60 行**，新增 `?? docs/design/ui/replica/av-settings.md`（04:57 创建，1973 字节）、`M MpvPlaybackSession.kt`、`M PlaybackPrefs.kt`；`git diff HEAD --stat` 变为 **57 files changed, 177 insertions(+), 143 deletions(-)**。新增的 2 个 `M` 文件是并发助手对 262 卡功能文件的再次写入，**非 D6 改动**，属环境漂移（§7 第 6 条）。
- 与第一轮审计时的「67 文件 / 447+ / 232−（含 275 行 Kotlin 可执行改动、8 个非 D6 文件混入）」相比，T1 diff 已大幅净化：262 提交把功能改动全部带走；T2 的再次漂移进一步印证「当前工作区非 D6 隔离集」。
- **本复审所有闸门结论以「D6 改动集」（144 条 RULES + 两类允许例外 + 两个 WIP）为对象**，不因 T2 并发 WIP 漂移而改变（用户指令：并发漂移作独立环境披露项，不判技术闸门 FAIL）。

### 三类变更逐项区分

**（a）D6 注释 / 字符串改写（约 50 文件）**
所有 `.kt / .cpp / .cxx / .hpp / CMakeLists.txt` 变更行 **100% 落在注释前缀（`//` / `*` / `#`）、KDoc 文档表格、或字符串字面量的行尾注释**，非注释变更行 = 0。典型：`CMakeLists.txt`、`jni_smb2_bridge.cxx`、`mpv_commands.cpp`、`DownloadService.kt`、`Emby*`、`LocalRangeServer.kt`、`PlaybackEngine.kt`、`SmbDiscovery.kt`、`PlayerGestureRouter.kt`、`PlayerSubtitleImport.kt`、`PlayerAudioOnly.kt`、`PlayerBottomBar.kt`、`PlayerCaptureButton.kt`、`PlayerGestureFlash.kt`、`PlayerGestureHud.kt`、`PlayerGestureLayer.kt`、`PlayerGlyph*.kt`、`PlayerLockLayer.kt`、`PlayerLongPressHud.kt`、`PlayerMotion.kt`、`PlayerPanel*.kt`、`PlayerResourcePanel.kt`、`PlayerSeekPreview.kt`、`PlayerSourceOrigin.kt`、`PlayerStatus*.kt`、`PlayerStepperRow.kt`、`PlayerToast.kt`、`PlayerTokens.kt`、`PlayerWindow.kt`、`DesignTokens.kt`、`Detail*`、`backend/*`、`tools/rewrite_discover.py`。均为 `-N/+N` 同行替换、净 0 行。

**（b）两类允许例外（合规 ✓）**
- **例外① 等价标识符重命名**：`tools/measure_kivo_playback.py`（-14/+14）：
  `import measure_rex_playback as rex` → `import measure_baseline_playback as baseline`；10 处 `rex.clusters / rex.longest_run` → `baseline.`；`rex.P = D` → `baseline.P = D`；另 2 处 docstring 去 `REX` / 改脚本名。**算法、常量、数值未变**（`import` 实测通过，§5）。
- **例外② 文件重命名与复算命令维护**：`tools/measure_baseline_playback.py` docstring 4 处命令名改新名、`PlayerTokens.kt:10` 复算命令改新名、`docs/design/ui/replica/playback/player-ui-v3-rex-replication.md` L9 命令改新名 + L10 补文件名映射——均落在字符串 / 文档。**重命名本身 `R100 tools/measure_rex_playback.py → measure_baseline_playback.py` 已随 262 提交进入 HEAD**（`cat-file -e` 核实：`f97dd2ea` 有旧名、`HEAD` 有新名、旧名无），故工作区 diff 中该文件仅为 `M`。这是「262 带走部分 D6 成果」的又一例证。

**（c）并发 262 卡改动：T1 时工作区 diff 中已无，T2 又被并发写入**
T1 时第一轮点名的 8 个非 D6 文件（`SettingsPlaybackSub.kt +136`、`SettingsRows.kt +96`、`PlayerComposeActivity.kt`、`SettingsPage.kt`、`PlayerPip.kt`、`PlayerSmbStart.kt`、`PlayerActionRouter.kt`、`MpvPlaybackSession.kt`）**均不在 55 文件清单中**，已随 `1643a141` 提交。`git diff f97dd2ea HEAD --name-status` = **35 文件**，其中含 `PlayerActionRouter.kt`（HEAD 版 toast 注释 = 当前工作区 L247 文本，确证 262 二次改写）、`R100` 重命名、5 个新增 `.kt`、`docs/handoff/TASKS.md`。**T2 收尾时** `MpvPlaybackSession.kt` / `PlaybackPrefs.kt` 又被并发助手写入（+33 行）、并新增未跟踪 `av-settings.md`——这是复审期间的第三次工作区漂移，属环境披露（§7 第 6 条），非 D6 改动。

**（d）两个 WIP（用户「不碰」）—— 当前 diff 中唯一超出「注释/字符串 + 两类例外」的可执行改动**
- `PlayerBarText.kt`（-3/+3）：`fontWeight = rexTextWeight(text)` → `barTextWeight(text)`（调用）、`internal fun rexTextWeight(...)` → `barTextWeight(...)`（**定义**），另 1 行注释清理。
- `PlayerTopBar.kt`（-4/+4）：`fontWeight = rexTextWeight(title)` → `barTextWeight(title)`（**跨文件调用**），另 3 行注释清理（`实测 REX 顶栏白图标` / `REX 0172 也隐掉它` / `（REX 0172 的反白圆）`）。
- 说明：`derex_apply.py` 的 144 条 RULES **不含** `PlayerBarText.kt`，且**不含** `rexTextWeight → barTextWeight` 映射；RULES 仅覆盖 `PlayerTopBar.kt` 的 3 条注释。即标识符改名与 `PlayerBarText.kt` 的清理属 WIP 自身内容 / 主助手划给 D6 的边界（见下 TASKS.md L12），非 D6 规则自动产物。该改动为**等价标识符重命名**（不改算法 / 行为），编译日志佐证其自洽（`barTextWeight` 全仓 3 处定义/调用、`rexTextWeight` 全仓 0）。

### 两个 WIP 成对关系是否仍成立
**成立 ✓**。全仓 `barTextWeight` 3 处：`PlayerBarText.kt:99`（定义）、`PlayerBarText.kt:71`（本文件调用）、`PlayerTopBar.kt:98`（跨文件调用）；`rexTextWeight` 全仓 0 命中。只提交其中一个文件 → 另一文件仍引用旧名 → 编译断裂；且两文件在 `f97dd2ea` 与 `HEAD` 的提交态**完全相同**（各含 3 / 4 行产品名/出处疑似行，从未进提交态）：
```
PlayerBarText.kt  L71 rexTextWeight(text) / L95 REX 实测横笔 / L99 internal fun rexTextWeight(...)
PlayerTopBar.kt  L47 实测 REX 顶栏白图标 / L98 rexTextWeight(title)
                 L113 REX 0172 也隐掉它 / L181 （REX 0172 的反白圆）
```
→ 若按「两个 WIP 不碰」口径提交，提交态将残留 `rexTextWeight` / `REX 0172` / `REX 实测横笔` 等；若 `git add -A` 全提交，则把用户明确「不碰」的 WIP 一并提交。**这是提交口径死结，属用户待决项（§7），不由本复审裁决，也不据此判技术闸门 FAIL。**

补充环境事实：`docs/handoff/TASKS.md` L12（262 卡写入）明确「**D6 边界**：`barTextWeight` 改名（PlayerBarText/PlayerTopBar）留 D6，本卡不带走；本卡文件不引用该符号，提交自洽」——即主助手把该标识符改名划归 D6，与用户「两个 WIP 不碰」的批准口径存在张力，进一步说明需用户重新拍板。

### 当前 diff 是否适合直接提交
**不适合，本复审不提交。** 理由：
1. 两个 WIP 与 D6 注释改动纠缠、且含可执行标识符改名；用户明确「两个 WIP 不碰」，全提交违背指令、排除则编译断裂且提交态残留产品名（见上）。
2. 262 卡已带走一部分 D6 成果（`PlayerActionRouter.kt` toast 注释、measure 脚本重命名、部分播放页注释），T1 的 55 文件 diff（T2 已漂移为 57 文件）只是 D6 **剩余部分**，非完整隔离集。
3. `docs/design/ui/replica/playback/player-ui-v3-rex-replication.md` 位于顶层 `docs/` 冻结归档、却因例外②被改动，是否纳入提交需用户确认。
4. 提交口径（两个 WIP 成对 / 是否随本卡提交 / 证据目录 `docs/evidence/p260/` 未跟踪）为**用户待决项**，本卡未提交、未推送。

---

## 7. 环境披露项（独立列示，**不因这些把技术闸门判 FAIL**）

1. **并发提交把 D6 成果劈成两半**（用户已知的「重大环境事实」，本复审独立确证）：D6 开工 HEAD=`f97dd2ea`；执行与审计期间另一助手完成并提交/推送 262 卡（`1643a141`，`@{u}..HEAD = 0`），`git diff f97dd2ea HEAD` = 35 文件，含 `PlayerActionRouter.kt`（toast 注释二次改写）、`R100` measure 脚本重命名、5 个新增 `.kt`、`docs/handoff/TASKS.md`（顶部被 262 改写，L1–13 为 262 卡内容）。故当前工作区 diff **不是干净的 D6 隔离集**，README §六 已如实披露、不再引用旧 stat。
2. **提交口径未决，本卡未提交、未推送**：用户批准「只提交本卡 + 证据、两个 WIP 不碰」，但两个 WIP 是「定义 / 调用必须成对」关系，叠加并发提交后隔离提交无法在不替主助手决定其 WIP 的前提下完成。需用户重新拍板后再动 git。
3. **基线 APK 不是 261 记录的那一个**（README §六 自承）：非严格同一构建 A/B；行为零变化主要由静态等价证据（注释/字符串/等价标识符 + 真实重编译 + 安装成功）支撑，真机对照为辅助。
4. **取证自身改变应用状态 / 播放页 dump 无文本节点**：首页、详情像素差由此产生；播放页以「逐字节全等」判零回归。
5. **编译与真机为静态证据**：`build.log` 日志层面成立但独立复现未验证；K40 四页像素差 / 语义树 / md5 均未能独立复现，标「未验证」。
6. **复审期间工作区再次并发漂移（本轮独立发现，非 D6 缺陷）**：收尾复核时 `git status --porcelain` 由 57 行增至 60 行，新增未跟踪 `docs/design/ui/replica/av-settings.md`（04:57）、`M MpvPlaybackSession.kt`、`M PlaybackPrefs.kt`；`git diff HEAD --stat` 由 T1 的 **55 文件 / 144+ / 143−** 变为 T2 的 **57 文件 / 177+ / 143−**。新增项均为并发助手的 WIP，非 D6 改动。本复审所有闸门结论以 D6 改动集为对象、以 T1 快照为基准；该漂移进一步佐证「当前 diff 非隔离集、不适合直接提交」，按用户指令作独立环境披露，**不据此判技术闸门 FAIL**。

---

## 8. 第一轮问题闭环表

| # | 第一轮（FAIL）发现 | 级别 | 修复 | 第二轮独立复算 | 闭环 |
| --- | --- | --- | --- | --- | --- |
| 1 | `PlayerBottomBar.kt:38 （0170 实测）` 帧号漏清；`FRAME_IDS` 缺 0170 | P0 | 清为「（实测）」+ 宽帧号 `\b0(?:16\d|17\d)\b` | L38 = （实测）；全仓 `0170` = 0；WIDE = 0；FRAME_IDS 清理后 = 0 | ✅ **闭环** |
| 2 | 清理前基线用「HEAD + 2 WIP」混合态、随 HEAD 漂移（144 vs 137） | P0 | `BASELINE_COMMIT="f97dd2ea"` 固定 + 双口径披露 | 纯提交 145/127/18/61 可复算，`grep_before.txt` 逐行 0 差异；137/120 差异如实披露（§2） | ✅ **闭环** |
| 3 | `verify` 因 `PlayerActionRouter.kt` 注释被并发送改而 FAIL（`new_text_missing=1`） | P1 | `SOFT_RULES` 放宽（只校验原文消失） | 当前 L247 文本不含产品名/出处编号；`rule_problems=0`；`verify_result.json` 与实际一致 | ✅ **闭环** |
| 4 | diff 混入 262 卡功能改动、275 行可执行改动、「60 文件 160/159」不可复算 | P1 | 改如实披露、不再引用旧 stat | T1 快照 diff 55 文件 / 144+ / 143−（收尾 T2 漂移为 57/177+/143−，见 §7 第 6 条）；三类变更可区分（§6） | ✅ **闭环（披露）** |
| 5 | 目录全量漏 `.json`，治理闸门 `rex-design` 一行未纳管（会判 unknown） | P2 | `SCAN_EXTS` 加 `.json` + 白名单加该行 | FULL/EXTS 两口径 18/0/18/0；该行判 whitelist；无夹带（§1） | ✅ **闭环** |
| 6 | 4 处改写质量：DesignTokens 叠词 / PlaybackEngine 语义反转+「旧内核」/ GestureLayer 残句 / GestureFlash 去主语 | P2 | 4 处全部重改 | 逐条读实际文件：叠词消除、语义反转消除、残句补全、主语补全；「旧内核」经 `git show` 证实为项目固有术语（§3） | ✅ **闭环** |
| 7 | `frame_id_lines: []` / `frame_ids_after.txt` 空与实测（0170）不符 | P1 | 0170 已清 | 当前工作区 FRAME_IDS = 0，空文件与实际一致（§4） | ✅ **闭环** |
| 8 | `grep_after.txt` 17 行仅在收窄口径成立、与「目录全量」差 1 行 | P2 | 18 行（纳入 `.json`） | FULL 2455 文件亦为 18 行，与 `grep_after.txt` 逐行一致（§1） | ✅ **闭环** |
| 9 | 编译 / 真机仅静态证据、未复现 | P3 | 修复后重跑编译、如实标注 | `build.log` 49 行日志文本成立；真机与编译独立复现仍未验证（§5 / §7） | ⚠️ **如实披露、维持未验证**（非阻断） |

**第一轮 6 个 P0/P1/P2 阻断项全部闭环；2 个 P3「未验证」项维持披露状态但不构成阻断。**

---

## 9. 仍不认可 / 需澄清的执行者结论（单列）

| # | 执行者结论（README / 脚本判词） | 第二轮判定 | 依据 |
| --- | --- | --- | --- |
| 1 | 「③ 真机播放 / 设置 / 详情不回归 ✅ 单侧 PASS（0.063% / 逐字节全等 / 语义树全等）」 | **仍不认可为「已验证」**，标未验证 | 二进制 `png`/`xml` 仅能确认存在；README §六 自承「基线 APK 不是 261 记录的那一个」「取证过程自身改变了应用状态」「播放页 dump 无文本节点」→ 非严格同一构建 A/B，无独立可复算比对链。仅作辅助证据，不作闸门判据 |
| 2 | 「② `assembleDebug` 编译过 ✅ PASS」 | **日志层面认可，独立复现不认可** | `build.log` L37/L47/L48 文本成立；本复审不跑 gradle、日志对应时刻 T 代码非当前工作区 → 标「日志 PASS / 编译复现未验证」（P3-5） |
| 3 | 「扫描 2384 / 2396 个文件，**目录全量**（含 `.json`）」 | **数字可复算并认可；措辞需澄清** | 2384/2396 是 `SCAN_EXTS` 扩展名白名单口径；真·目录全量为 2446 / 2455 文件。两口径下 HIT 数字相同（145 / 18），结论不受影响，但「目录全量」表述与 `eligible()` 实现不符（P3-2） |
| 4 | 「本卡新增 `.kt` = 0、行数净 0」 | **基本认可，需区分一处例外** | D6 的 144 条 RULES 为同行替换、净 0 行、无新增 `.kt`；但例外② `player-ui-v3-rex-replication.md` 净 **+1 行**（补文件名映射说明），属文档维护，宜单独标注 |
| 5 | 「`verify` 判词 PASS（0 问题 0 违规），`apply` 幂等写 0 文件」 | **认可 ✓** | 独立 ast 复算 `rule_problems=0`、144 条 old 消失 / new 落地；实跑 `apply` 写 0 文件、前后 hash 完全一致（§4） |
| 6 | 「修复后 `verify` 复算：144 条规则（含 4 条修复规则）、PASS」 | **认可 ✓** | `RULES len=144`、58 文件、1 条 soft；`verify_result.json` 逐字段与实际一致（§4） |

**附：对第一轮 `audit.md` 一处表述的更正（非执行者结论）**——§3 已证「旧内核」在 `f97dd2ea` 已存在（6 文件 15 处，清理前后计数相同），属项目固有架构术语；第一轮「凭空新增无来源概念『旧内核』」与事实不符，第一轮批评的实质是「语义反转」（已修复）。本复审保留 `audit.md` 原貌不改写，仅在此如实更正。

---

## 10. 最终判定

**`PASS_WITH_DISCLOSURE`**

判定基准：用户 2026-11-13 的「语义判据（产品名 / 出处署名零残留）+ 目录全量 + 编译 + 不改逻辑 + 证据可复算」，并按用户本轮指令把「并发提交 / 提交口径未决」作为**独立环境披露项**、不因它把技术闸门判 FAIL、也不替用户决定提交。

**四条技术闸门逐项结论**：
1. **语义零残留 —— PASS**：清理后长期代码 FULL（2455 文件）与 EXTS（2396 文件）两口径均 18 HIT / **0 product** / **0 unknown** / 18 whitelist；宽帧号 0160–0179 零命中、0170 已清；18 行白名单逐行核对无产品名 / 出处署名夹带；`FRAME_ALL` 6 行全为 README 明列保留的项目自有编号（M04-Sxx / IMG_0136）。
2. **编译 —— PASS（日志层面）**：`build.log` L37 `compileDebugKotlin` 无 `UP-TO-DATE`、L47 `BUILD SUCCESSFUL in 6s`、L48 `6 executed`；独立复现未验证（P3 披露）。
3. **不改逻辑 —— PASS（附披露）**：T1 快照 diff 55 文件 / 144+ / 143− 全为注释 / 字符串 + 两类允许例外（等价标识符重命名）+ 两个 WIP 的等价标识符改名；无函数签名 / 依赖 / 常量 / 条件逻辑改动；第一轮的 275 行可执行改动已随 262 提交离开工作区；`apply` 幂等写 0 文件、hash 未变，佐证「不改逻辑」可复算。T2 的 57 文件 / 177+ / 143− 含并发 WIP 再写入，属环境披露项（§7 第 6 条），不改变 D6 改动集的判定。
4. **证据可复算 —— PASS**：纯提交基线 145/127/18/61 与 `grep_before.txt` 逐行 0 差异；144 条规则 `rule_problems=0`；`verify_result.json` 逐字段一致；`apply` 幂等实跑验证；复算链三脚本存在、语法 OK、可 import。

**披露项（不判 FAIL，但必须随结论呈现）**：并发 262 提交（`1643a141`）带走部分 D6 成果、当前 diff 非隔离集；提交口径（两个 WIP 定义/调用成对、是否随本卡提交）未决，本卡未提交未推送；编译与真机为静态证据未独立复现；6 条 P3 级残留（§11）。

**分级残留清单（P0=0 / P1=0 / P2=0 / P3=6）**：
- P3-1 `PlaybackEngine.kt` L10/L11 重复「单一 demux 线程」（轻微文案冗余，无残句 / 语义反转）；
- P3-2 README「目录全量」措辞与 `eligible()` 扩展名白名单实现不符（真全量 2455 / 2446 文件，结论不受影响）；
- P3-3 白名单第 11 行 `redline_gate_result.json:39` 为治理闸门生成物、主动纳入白名单（非夹带），需持续披露；
- P3-4 `FRAME_ALL` 6 行项目自有编号（M04-Sxx / IMG_0136）按 README 口径保留，标「低风险 / 待确认」；
- P3-5 `build.log` 为修复后重跑日志（49 行 / 6s），与第一轮引用的 22s 版本不同；编译独立复现未验证；
- P3-6 真机像素 / 语义树 / md5 对照为静态证据，未独立复现（README §六 自承 A/B 非同一构建）。

**本复审不替用户提交**：当前 diff 因两个 WIP 纠缠 + 提交口径未决 + 262 已带走部分成果，**仍不适合直接提交**；建议由用户重新拍板提交口径后再动 git。本卡未 `git add/commit`、未推送。

---

*复审结束。除本 `audit-round2.md` 外，本复审未修改任何文件；`audit.md` 原貌保留；未执行会覆写证据文件的 `snapshot before/after` / `verify` 原命令（改以等价内联脚本复算）；`apply` 实跑为幂等、写 0 文件并经 hash 确认源码未改动；未触碰两个 WIP 的内容；未替用户提交。收尾复核时观测到的 `av-settings.md` 与 2 个 playback 文件写入为**外部并发进程**所为（§7 第 6 条），非本复审写入。*
