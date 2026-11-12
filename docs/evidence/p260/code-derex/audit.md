# D6「清除长期代码里的参考产品名痕迹」独立审计报告

- 审计员：独立审计员（不采信执行者任何自我认证/判词）
- 审计时点：2026-11-13 约 04:20–04:26（本地），设备无关，只读
- 仓库根：`C:\kivo video`；所有 git 命令均带 `-c safe.directory="C:/kivo video"`
- HEAD（清理前基线提交）：`f97dd2ea`
- 只读铁律：本次审计**唯一写入文件即本 `audit.md`**；未改源码、未 `git add/commit`、未触碰两个 WIP、未执行任何会覆写证据文件的命令
- 扫描范围（用户拍板「目录全量」）：`backend/`、`include/`、`android/app/src/main/`（含 `CMakeLists.txt`、`.cxx`）、`tools/`（排除 `tools/reference/`）；`docs/`、`artifacts/` 为冻结归档，不在清理范围
- 语义判据：产品名/出处署名零残留；`barExit`/`ShaderExecution`/`Execution` 子串误报与冻结参考功能路径（`docs/reference/rex-teardown/...`、`artifacts/rex-measurements.txt`）列入白名单

> 复算一律用 `PYTHONUTF8=1 python - <<'PY' ... PY` **内联执行、零落盘**；`git archive HEAD` 经 `tarfile` 在内存解包，不导出到磁盘。
> import 测试加 `PYTHONDONTWRITEBYTECODE=1` 以免生成 `__pycache__`；语法检查用内存 `compile()` 替代会写 `.pyc` 的 `py_compile`。

---

## 0. 结论速览

**最终 verdict：`FAIL`**

D6 主体（140 条注释/字符串改写 + 两类允许例外）本身抽查质量总体尚可，17 行白名单干净；但按用户「语义判据 + 目录全量 + 全部 diff 分类」独立复算，存在**确定性的清理遗漏**与**证据链整体不可复算**：

1. **P0（阻断）**：`PlayerBottomBar.kt:38 （0170 实测）` 是绑定参考录屏的帧号/出处编号残留，清理前就在、清理后仍在，且 `0170` 与已清除的 `0169`/`0171` 同属 REX 录屏帧号系列（被 `player-ui-v3-rex-replication.md` L11 `rex-grid-0160/0163/0169/0170/0171.png` 佐证）。执行者 `FRAME_IDS` 正则集合缺 `0170` 导致「58→0」在其窄口径下才成立，语义闸门「出处署名零残留」被违反。
2. **P0（阻断）**：清理前基线口径不实。README 标「仓库 HEAD（清理前）137/120」，但 137 是「HEAD + 2 个 WIP 取工作区版本」的混合态；纯 `git archive HEAD` 实为 **144 HIT / 127 product**。两个 WIP 的工作区版本已含主助手 WIP 与 D6 注释清理，污染了「清理前」基线。
3. **P1（高）**：当前 `git diff --find-renames HEAD` 是 **67 文件 / 447+ / 232-**（且审计期间持续漂移），含 **275 行 Kotlin 可执行改动**（函数签名、依赖、常量、条件逻辑），既非注释/字符串，也不属两类允许例外；当前 diff **不是 D6 隔离提交集**。README「60 文件 / 160 / 159」无法复算。
4. **P1（高）**：`derex_apply.py verify` 在当前工作区**会 FAIL**（独立复算 140 条规则得 `new_text_missing=1`）；执行者 `verify_result.json: PASS` 已过期、不可复算。
5. **P2（中）**：改写质量若干确凿问题（残句、去主语、语义反转）；目录全量口径下另有 1 行 `rex-design` 引用被 `.json` 扩展名白名单漏扫；工作区并发漂移使几乎所有「清理后」结论无法稳定复算。
6. **P3（披露/未验证）**：`assembleDebug` 重编译、K40 四页对照、像素差/md5/语义树全等等均为静态证据，无法独立复现，一律标「未验证」。

---

## 1. 独立复算 grep（要点 1）

### 方法与实际命令
- 清理后：内联 Python 遍历 4 个范围目录的**全部文件**（不限扩展名），逐行 `re.search(r"rex|senplayer", line, re.I)`；另用宽帧号正则 `\bM\d{2}-S\d+\b|\bsk_\d+\b|\bIMG_\d+\b|\bframes\d+/|(?<![\d.])0(?:1(?:6\d|7\d|8\d))(?![\d])` 单独扫出处编号。同时用执行者的 `SCAN_EXTS`（`.kt/.cpp/.cxx/.hpp/.h/.py/.txt` + `CMakeLists.txt`）跑一遍做口径对比。
- 清理前：`subprocess.check_output(["git","-c","safe.directory=C:/kivo video","archive","--format=tar","HEAD"])` → `tarfile` 内存解包 → 逐文件逐行扫。三种口径：
  - A 纯 HEAD / SCAN_EXTS；B HEAD + 2 WIP 取工作区版本 / SCAN_EXTS（= 执行者 `head_snapshot()` 定义）；C 纯 HEAD / 目录全量。
- 分类**不采信** `derex_apply.py` 的 `classify()`：自定义 `myclass()`（剥离 `barexit|shaderexecution|execution|docs/reference/rex-teardown|rex-package|artifacts/rex-measurements.txt|rex-measurements|tools/reference/rex` 后若仍含 `rex|senplayer` 判 product，否则 whitelist），并与执行者 `PRODUCT/WHITELIST` 正则结果对撞找分歧。
- 逐行核对：把重建 B 的命中集合与 `grep_before.txt`（剥离 `[bucket]` 标签后）做集合差。

### 实测数字

| 口径 | 扫描文件 | HIT 行 | product | whitelist | FRAME_IDS(执行者正则) | 我的宽帧号正则 |
|---|---|---|---|---|---|---|
| 清理前 A 纯 HEAD / SCAN_EXTS | 2333 | **144** | 127 | 17 | 60 | 67 |
| 清理前 B HEAD+2WIP / SCAN_EXTS（执行者 before） | 2333 | **137** | **120** | 17 | **58** | 65 |
| 清理前 C 纯 HEAD / 目录全量 | 2446 | **145** | 128 | 17 | 60 | 67 |
| 清理后 当前工作区 / SCAN_EXTS | 2341 | **17** | 0 | 17 | 0 | 见下 |
| 清理后 当前工作区 / 目录全量 | 2452 | **18** | 1* | 17 | 0 | 见下 |

\* 该 1 行按执行者自己的 `classify()` 会判 `unknown`（非 product）。

- `grep_before.txt`（137 行）与重建 B **逐行完全一致，0 差异** → 137/120/17/58 在其自定义口径下可复算。
- 口径 A 与 B 差 **7 行**，正是纯 HEAD 中两个 WIP 文件的命中：
  - `PlayerBarText.kt:71 fontWeight = rexTextWeight(text)`（调用）、`:95 REX 实测横笔`（注释）、`:99 internal fun rexTextWeight(...)`（定义）
  - `PlayerTopBar.kt:47 实测 REX 顶栏白图标`（注释）、`:98 rexTextWeight(title)`（调用）、`:113 REX 0172 也隐掉它`（注释）、`:181 REX 0172 的反白圆`（注释）
  - 执行者把这 2 个文件替换成工作区版本（已改 `barTextWeight` 且已清注释），故这 7 行从基线消失。
- 我的 `myclass()` 与执行者 `classify()` 在 SCAN_EXTS 口径下 **0 分歧**；仅目录全量口径多出 1 行分歧（见下）。

### 清理后「非白名单命中」逐行列出

**（a）目录全量口径多出、被扩展名白名单漏扫的产品名痕迹：**
```
backend/tools/governance/redline_gate/artifacts/redline_gate_result.json:39:
    "file": "C:\\kivo video\\docs\\design\\backend-core\\236-ui-align-rex-design.md",
```
该文件位于 `backend/` 范围内，含 `rex-design`（产品名 Rex 连字符形式）。执行者 `SCAN_EXTS` 不含 `.json` → 未扫。若纳入，按执行者自己的 `PRODUCT` 正则 `\bRex\b(?![-/])` 对 `rex-design` 不匹配、`WHITELIST` 也不匹配 → 判 **`unknown`**，会使 `verify` 的 `unknown=0` 判词失效。它指向 `docs/` 冻结归档文档名、且是治理闸门生成物，属**口径差异/披露级**，非 D6 注释改写遗漏。

**（b）出处编号残留（不含 rex，故不进 HIT，但属语义判据要清除的帧号/动效编号）：**
```
android/app/src/main/java/com/kivo/cinema/ui/compose/player/PlayerBottomBar.kt:38:
    * · 选集态（点选集）：整条换成横滑选集带（0170 实测）。
```
`git show HEAD:.../PlayerBottomBar.kt` 证实该行清理前即如此（与第 37 行 `（0169 实测）` 并列）；`derex_apply.py` 的 140 条 RULES 中只有 `（0169 实测）→（实测）`，**从未处理 0170**；`frame_ids_before.txt`（58 行）也不含 0170。执行者 `FRAME_IDS` 集合为 `0160|0162|0166|0169|0171|0172|0178|0179`（`re.I`），**缺 0170**（以及 0161/0163–0168/0173–0177 等），故 `frame_ids_after.txt` 为空、`verify_result.json: frame_id_lines: []`。**这是 D6 清理本身的确定性遗漏，与工作区漂移无关。**

**（c）清理后其余帧号/动效编号（判定为项目自有，按 README 口径保留，非违规）：**
```
SourceOptionalFields.kt:25  M04-S03（243 自有）
TmdbHome.kt:9              IMG_0136（README 明列保留的自有截图号）
DiscoverPage.kt:180/321    M04-S04（自有）
rewrite_discover.py:14/120 M04-S01（自有）
```
`M04-*` 非用户列举的 `M03-Sxx`/`M08` 系列，`IMG_0136` 为 README 明确保留项。执行者 `FRAME_IDS` 亦不覆盖 M04 系列。审计无法完全独立确证其非参考来源，标 **低风险/待确认**，不计为违规。

### 核对 README「137→17 / 120→0」
- `137 / 120 / 17 / 58`：**可复算，但仅在执行者自定义的「HEAD + 2 WIP 工作区版本」口径下**；且该口径污染基线（真实纯 HEAD = 144 / 127）。README 第 3 行「仓库 HEAD（清理前）`f97dd2ea`」与第 30 行「前后同一口径」**不成立**（before 用 HEAD 树、after 用工作区树，文件集不同）。
- 清理后 `17`（SCAN_EXTS）：可复算，逐行一致。
- `120→0` 中的 `0`：SCAN_EXTS 口径成立；**目录全量口径不成立**（`rex-design` 一行 + `0170` 出处编号残留）。

### 问题（按严重度）
- **P0**：`0170 实测` 出处编号残留 → 语义判据「出处署名零残留」失败；「58→0」结论不成立。
- **P0**：清理前基线非纯 HEAD（144 vs 137），README 标注误导；「目录全量」被收窄为扩展名白名单。
- **P2**：`rex-design` 一行在目录全量口径下未被清理/未被列入白名单，且按执行者自身正则会判 `unknown`。

---

## 2. diff 分类（要点 2，最关键）

### 方法与实际命令
- `git -c safe.directory="C:/kivo video" diff --find-renames HEAD --stat` / `--numstat`
- 内联 Python 调 `git ... diff --find-renames HEAD -U0 --no-color`（`stderr=DEVNULL` 抑制 CRLF 警告），解析每个 `diff --git` 段，收集全部 `-`/`+` 变更行，按前缀分类：`comment`（`// | * | /* | */ | """ | ''' | #` 开头）/ `string`（含引号）/ `code`（其余），并据 `grep_before.txt` + `frame_ids_before.txt` 提取 D6 文件集（63 个）区分 D6 / 非 D6 文件。
- 抽看 22 个文件的 `-`/`+` 配对（`git diff HEAD -U0 -- <file>`）。

### 实测数字
- `git diff --find-renames HEAD --stat` 当前 **67 files changed, 447 insertions(+), 232 deletions(-)**。
- **审计期间工作区并发漂移**（关键环境事实，见 §4）：审计开始为 65 文件 / 422+ / 228-；中段 `git diff`（unstaged）67 文件 / 446+ / 232-；后段 `diff HEAD` 67 文件 / **447+** / 232-；`status --porcelain` 条目 62 → **74**。`find ... -printf "%T..."` 显示 04:22:35–04:25:09 有外部进程连续写入 `PlayerComposeRoot.kt`、`PlayerOverlay.kt`、`PlayerBarsHost.kt`、`PlayerBottomBar.kt`、`PlayerSeekTrack.kt`、`PlayerPip.kt`、`PlayerActionRouter.kt`、`PlayerStartPolicy.kt`、`PlayerLifecycle.kt`、`PlayerSmbStart.kt`、`PlayerComposeActivity.kt`、`SettingsRows.kt`。
- 因此 **README「`git diff --stat HEAD`：60 文件，160 insertions / 159 deletions」无法复算**。
- `code`-ish 变更行 **275 行**；其中 8 个文件**完全不属于 D6 文件集**：

| 非 D6 文件 | -/+ |
|---|---|
| `SettingsPlaybackSub.kt` | -12 / **+136** |
| `SettingsRows.kt` | -46 / **+96** |
| `PlayerComposeActivity.kt` | +14 |
| `SettingsPage.kt` | -2 / +2 |
| `PlayerPip.kt` | -1 / +2 |
| `PlayerSmbStart.kt` | +2 |
| `PlayerBarText.kt` | -3 / +3 |
| `PlayerTopBar.kt` | -4 / +4 |

- **超出「注释/字符串 + 两类允许例外」的 Kotlin 可执行改动（实测逐行，节选）**：
```
MpvPlaybackSession.kt  + fun seekTo(targetUs: Long, exact: Boolean = false): Long {      ← 函数签名变更
                       + private val prefs by lazy { PlaybackPrefs(app) }                ← 新依赖
                       + val slang = prefs.subtitleSlang()
                       + native.seek(targetUs / 1_000_000.0, exact = exact)
PlayerActionRouter.kt  + mpvNetSession?.seekTo(ms * 1000, exact = playbackPrefs.seekExact)
                       + internal fun PlayerComposeActivity.showToast(message: String, durationMs: Long = 1_800) {   ← 签名变更
                       + Handler(mainLooper).postDelayed({ toastText = "" }, durationMs)
PlayerComposeRoot.kt   + showRemaining = playbackPrefs.showRemaining / scrubPreview = playbackPrefs.scrubPreview
PlayerBarsHost.kt      + showRemaining: Boolean, / scrubPreview: Boolean,              ← 新增形参
PlayerBottomBar.kt     + text = if (showRemaining && durationMs > 0) ... else formatPlayerTime(durationMs)
PlayerOverlay.kt       + showRemaining: Boolean, / scrubPreview: Boolean,
PlayerSeekTrack.kt     + visible = (dragging || previewMs >= 0) && scrubPreview,
SettingsPlaybackSub.kt / SettingsRows.kt  大段功能代码（+136 / +96）
PlayerBarText.kt       - internal fun rexTextWeight(...) → + internal fun barTextWeight(...)   ← 函数定义改名
PlayerTopBar.kt        - rexTextWeight(title) → + barTextWeight(title)                        ← 调用改名
```

### 两类允许例外核对（逐条）
- **例外①（`tools/measure_kivo_playback.py` 等价标识符重命名）**：`import measure_rex_playback as rex` → `import measure_baseline_playback as baseline`；10 处 `rex.clusters/longest_run` → `baseline.`；`rex.P = D` → `baseline.P = D`。**实测全部为标识符替换，未改算法/数值，合规 ✓**。附带 docstring 中 `REX 基线帧`→`基线帧`、`measure_rex_playback`→`measure_baseline_playback`（同为字符串/注释，合规）。
- **例外②（文件重命名与复算命令维护）**：`tools/measure_rex_playback.py` → `tools/measure_baseline_playback.py`（`git mv`，暂存态 `RM`）；更名后脚本内 4 处命令名同步；`PlayerTokens.kt:10` 复算命令改新名；`docs/design/ui/replica/playback/player-ui-v3-rex-replication.md` 第 9 行命令改新名 + 第 10 行补文件名映射。**均落在字符串/文档，合规 ✓**。
- **D6 的 140 条 RULES 本身**（ast 静态解析实测 `len(RULES)=140`、`distinct files=58`）：抽查 22 文件显示均落在注释/KDoc/字符串/docstring，未发现 D6 规则改动可执行语句的证据。

### 问题（按严重度）
- **P1**：按「取全部变更」口径，当前 `git diff HEAD` 含 **275 行 Kotlin 可执行改动**（函数签名、常量、依赖、条件逻辑），**不属于两类允许例外**，违反 D6「只改注释/字符串」的范围约束。
- **P1**：8 个非 D6 文件（含 `SettingsPlaybackSub.kt +136`、`SettingsRows.kt +96`）混入工作区 diff，当前 diff **不是 D6 隔离提交集**；若按 README「隔离提交」口径提交，会把功能 WIP 一并提交。
- **P1（归因说明）**：这些功能改动 mtime 集中在审计期间 04:22–04:25、且 README 的 60/160/159 与实测不符，判断为**并发的工作区 WIP（262 播放设置）**，非执行者 140 条规则所为；但铁律要求「无法复算的一律标未验证」，故不能替执行者认定其「不属本卡」，只能如实记录其存在于全部变更中。
- **P1**：两个 WIP 文件里 `rexTextWeight → barTextWeight` 是**函数定义 + 调用的可执行标识符改动**，不在允许例外内；且两文件还混入 D6 注释清理，与用户「两个 WIP 不碰」冲突（README 披露 4 自承）。这构成 D6 的**未解决阻塞**：WIP 与 D6 改动纠缠，无法干净隔离（详见 §4）。

---

## 3. 改写质量抽查（要点 3）

### 方法与实际命令
`git -c safe.directory=... diff HEAD -U0 --no-color -- <22 个文件>` 取 `-`/`+` 配对，人工逐条读；另用精准正则（仅注释行）扫空括号/连续标点/残句：`（）|\(\s*\)|，。|；）|。）|：。|【[^】]*】。|与\s*一致`。

### 抽查覆盖（22 个，≥15 ✓）
`PlayerBarText.kt`、`PlayerTopBar.kt`、`PlayerGestureLayer.kt`、`PlayerGlyphFeature.kt`、`PlayerTokens.kt`、`PlayerPanelOcclusion.kt`、`PlayerWindow.kt`、`PlayerActionRouter.kt`、`PlaybackEngine.kt`、`LocalRangeServer.kt`、`EmbyBrowseScreen.kt`、`DesignTokens.kt`、`DetailPage.kt`、`PlayerGestureFlash.kt`、`PlayerMotion.kt`、`PlayerBottomBar.kt`、`PlayerSeekPreview.kt`、`PlayerSourceOrigin.kt`、`PlayerStatusRow.kt`、`PlayerStepperRow.kt`、`measure_kivo_playback.py`、`rewrite_discover.py`、`player-ui-v3-rex-replication.md`。

### 问题行（按严重度）
- **P2｜`DesignTokens.kt:23`** 叠词残句：
  `// 语义色板（RexDesign Assets.car 硬参数，浅色态；深色态 v2）` → `// 语义色板（浅色态语义色板硬参数；深色态 v2）` ——「语义色板」重复两次，去出处后主语叠加、语义不通。
- **P2｜`PlaybackEngine.kt:11-12`** 语义反转 + 凭空新增技术断言：
  `与 SenPlayer / Rex 同路。` → `链路独立，不与旧内核共用读路径。` —— 原意「与参考产品同路」被改写成「独立、不共用」，且引入无来源概念「旧内核」，非等价去出处。
- **P2｜`PlayerGestureLayer.kt:69`** 残句（无谓语）：
  `// 【竖向拖动：亮度/音量】参考 M03-S03。` → `// 【竖向拖动：亮度/音量】。` —— 去出处后只剩名词短语 + 句号（已由精准扫描确证当前仍存在）。
- **P2｜`PlayerGestureFlash.kt:26`** 主语丢失：
  `* REX 用一个居中的图标闪 0.5 秒来确认` → `* 用一个居中的图标闪 0.5 秒来确认` —— 「谁用…来确认」无主语。
- **P3｜`PlayerWindow.kt:27`** 语气/语义改变：`REX 播放时**也是全屏**` → `播放时**必须全屏**`（「也是」→「必须」）。可接受但非纯去出处。
- **P3｜`PlayerActionRouter.kt:247`** 注释被并发 WIP 二次改写（见 §6）：`/** 轻提示：默认 1.8s 后自动消失（…）；起播规格等可指定停留时长。 */`，同时 `showToast` 新增 `durationMs` 形参——属功能 WIP 混入，非 D6 改写。
- **空括号**：精准扫描未发现 D6 改写留下的 `（）` 空括号（其余 21 条命中均为 `start()`/`getLoopbackAddress()`/`stop()` 等正常代码括号误报）。
- **技术数值**：抽查中 `304.8pt / 55pt / 17pt / 21×18.3pt / 287dp / spring(0.25s, 0.8) / 淡入0.15·停止1s·淡出0.3 / 0.7–1.0pt@16pt / 24×24` 等数值**均保留，未发现误删 ✓**。
- 其余抽查行（`PlayerTokens.kt` 12 处、`PlayerGlyphFeature.kt` 15 处、`PlayerPanelOcclusion.kt`、`PlayerMotion.kt`、`PlayerBottomBar.kt`、`PlayerSeekPreview.kt`、`PlayerSourceOrigin.kt`、`PlayerStatusRow.kt`、`PlayerStepperRow.kt`、`measure_kivo_playback.py`、`rewrite_discover.py` 等）改写基本通顺、数值保留，无明显残句。

---

## 4. 闸门复核（要点 4）

### 方法与实际命令
`git status --porcelain=v1`；遍历 `android/app/src/main/java/**/*.kt` 统计行数；`git diff HEAD --numstat`；`build.log` 全文读；全量 `.kt` 搜 `rexTextWeight|barTextWeight` 定位定义/调用。

### 实测数字
- **新增 `.kt`**：工作区 **5 个未跟踪 `.kt`** —— `PlaybackPrefs.kt`、`PlayerLifecycle.kt`、`PlayerStartPolicy.kt`、`SettingsPicker.kt`、`SettingsScaffold.kt`，另有 `artifacts/p262-playback-settings/`、`docs/evidence/p260/` 两个未跟踪目录。README 称「新增 `.kt` = 0」。这些是并发 WIP（262 播放设置/播放页），非 D6 规则，但工作区确实存在。
- **300 行闸门**：超 300 行的 `.kt` 共 **10 个**（`PlayerActivity.kt` 901、`MpvPlaybackSession.kt` 442、`FfmpegPlaybackSession.kt` 403、`DiscoverPage.kt` 401、`NetworkPlaybackSession.kt` 387、`PlaybackSession.kt` 365、`LocalRangeServer.kt` 360、`MainActivity.kt` 357、`MediaLibraryActivity.kt` 334、`NativePlayer.kt` 320）。
  - D6 的 140 条规则是同行替换（净 0 行），**不新增行** → 就「本卡（D6）新增行」而言，未导致 `.kt` 超闸。
  - 但 `MpvPlaybackSession.kt`（442 行，已超闸）在 `diff HEAD` 中 **+13 行**（`PlaybackPrefs`/`seekTo(exact)` 等并发 WIP）；`SettingsPlaybackSub.kt +136`、`SettingsRows.kt +96` 亦为新增行。故 README「既有超闸文件未动 / 行数净 0」与**当前工作区 diff** 不符。
- **`build.log`**：L79 `> Task :app:compileDebugKotlin`（**无 UP-TO-DATE**，紧随 2 条 deprecation 警告 → 真实执行）；L92 `BUILD SUCCESSFUL in 22s`；L93 `40 actionable tasks: 6 executed, 34 up-to-date`。**日志文本层面成立 ✓**。但为静态证据，无法独立复现（审计不允许跑 gradle；且工作区已漂移，日志对应的代码非当前工作区）→ 标 **「未验证（编译复现）」**。
- **两个未提交 WIP 的改动内容与成对关系**：
  - `PlayerBarText.kt`：`rexTextWeight(text)` 调用 → `barTextWeight(text)`；`REX 实测横笔` 注释去 REX；`internal fun rexTextWeight(...)` 定义 → `barTextWeight(...)`。
  - `PlayerTopBar.kt`：`实测 REX 顶栏白图标` 注释去 REX；`rexTextWeight(title)` 调用 → `barTextWeight(title)`；`REX 0172 也隐掉它` → `实测也隐掉它`；`（REX 0172 的反白圆）` → `（反白圆）`。
  - 全仓 `.kt` 中 `barTextWeight` 共 3 处：`PlayerBarText.kt:99`（**定义**）、`PlayerBarText.kt:71`（本文件调用）、`PlayerTopBar.kt:98`（**跨文件调用**）。HEAD 三处均为 `rexTextWeight`。
  - **构成「定义/调用」必须成对提交关系 ✓**：只提交其中一个文件，另一文件仍引用旧名 → `barTextWeight`/`rexTextWeight` 未定义 → 编译断裂。且两文件都含 D6 注释清理，不纳入提交则提交态残留 `REX 0172`/`rexTextWeight`（后者在执行者 `PRODUCT` 正则里被列为违规）。**这是 D6 无法干净隔离的死结**（README 披露 4 自承，审计确证）。

### 问题（按严重度）
- **P1**：两个 WIP 与 D6 注释清理纠缠，且含可执行标识符改名；无论「提交 / 不提交」都违反 D6 判据或用户「WIP 不碰」，**提交口径未解决**。
- **P1**：工作区 5 个未跟踪 `.kt` + 超闸 `.kt` 被新增行，与 README「新增 .kt = 0 / 未动超闸文件」不符（受并发漂移影响，标「无法复算」）。
- **P3**：`assembleDebug` 编译与真机闸门仅静态证据，未验证。

---

## 5. 白名单审查（要点 5）

### 方法与实际命令
逐行读 `grep_after.txt`（17 行），按语义判据人工判定，并与我自己全量扫描的 18 行 HIT 对撞。

### 逐行判定（17 行，全部合规 ✓，无夹带产品名/出处署名）

| 行 | 文件:行 | 命中片段 | 判定 |
|---|---|---|---|
| 1–5 | `PlayerBarsHost.kt:61/78/113/115/116` | `barExit(...)` / `barExitMs` | `barExit` 子串误报（b-a-**rEx**-it）✓ |
| 6 | `PlayerMotion.kt:28` | `const val barExitMs = 140` | 同上 ✓ |
| 7 | `PlayerTokens.kt:11` | `artifacts/rex-measurements.txt` | 冻结参考功能路径（复算链依赖）✓ |
| 8–10 | `p5h_color_hdr_protected_tests.cpp:83/86/98` | `ShaderExecutionOrder` / `FakeShaderExecutionPlanner` | `ShaderExecution`/`Execution` 子串误报 ✓ |
| 11–16 | `include/.../shader_execution_order.hpp:19/25/28/39/52/60` | `ShaderExecutionStep` / `FakeShaderExecutionPlanner` | 同上 ✓ |
| 17 | `tools/dump_home_spec.py:6` | `docs/reference/rex-teardown/rex-package/strings/` | 冻结参考功能路径（脚本功能依赖）✓ |

计数与 README §三 白名单表（`PlayerBarsHost/PlayerMotion` 6 + `shader_execution_order.hpp` 6 + `p5h` 3 + `PlayerTokens` 1 + `dump_home_spec` 1 = 17）一致。**17 行全部属于已批准白名单，无夹带。**

### 问题（按严重度）
- **P0（白名单之外）**：`PlayerBottomBar.kt:38 （0170 实测）` 是**出处编号残留**，既未被清理、也未被列入白名单（且不含 `rex` 故不进 HIT、不被 `grep_after.txt` 收录）。白名单审查本身 PASS，但「清理后仅剩白名单命中」这一结论因该行而**不完整**。

---

## 6. 复算链（要点 6）

### 方法与实际命令
- 读 `docs/design/ui/replica/playback/player-ui-v3-rex-replication.md` 第 9–11 行；正则提取 `tools/[\w./-]+\.py` 并检查存在性。
- 内存 `compile(open(f,'rb').read(), f, 'exec')` 做语法检查（零落盘，等价 `py_compile`）。
- `PYTHONDONTWRITEBYTECODE=1 python -c "sys.path.insert(0,'tools'); import measure_baseline_playback; import measure_kivo_playback"`。
- `ast.parse` + `ast.literal_eval` 静态取 `derex_apply.py` 的 `RULES`，逐条独立核对当前工作区「old 是否残留 / new 是否落地」，以判定 `verify` 能否 PASS。

### 实测数字
- `player-ui-v3-rex-replication.md` L9：`> 复算：\`python tools/measure_baseline_playback.py {icons|bottom|panel|text} <png> …\`` → **指向的文件存在 ✓**（`tools/measure_baseline_playback.py exists=True`）；旧名 `tools/measure_rex_playback.py` **不存在 ✓**；L10 补的文件名映射存在 ✓。L11 `rex-grid-0160/0163/0169/0170/0171.png` 反证 0170 属 REX 帧号系列。
- 语法（内存 `compile()`）：`tools/measure_baseline_playback.py` **OK**；`tools/measure_kivo_playback.py` **OK**；`derex_apply.py` **OK**。
- import（`PYTHONDONTWRITEBYTECODE=1`，`sys.path` 含 `tools/`）：`import measure_baseline_playback` **OK**（导出 `P/clusters/longest_run/icons/bottom/panel/text` 等）；`import measure_kivo_playback` **OK**。两脚本 `import numpy as np` / `from PIL import Image` **依赖齐备，无环境限制** ✓。
- **README 三条复算命令**（§二）：
  ```
  python docs/evidence/p260/code-derex/derex_apply.py snapshot before
  python docs/evidence/p260/code-derex/derex_apply.py snapshot after
  python docs/evidence/p260/code-derex/derex_apply.py verify
  ```
  `snapshot before/after` 会**覆写** `grep_before.txt`/`grep_after.txt`/`frame_ids_*.txt`，`verify` 会**覆写** `verify_result.json`。按本次审计「除 `audit.md` 外不得修改任何文件」的只读铁律，**未原样执行**；改以上述等价只读内联脚本复算（结果见 §1、§7）。
- **即便抛开铁律，`verify` 在当前工作区也会 FAIL**：独立复算 140 条规则得 `file_missing=0`、`old_text_still_present=0`、**`new_text_missing=1`**。缺失规则为：
  ```
  FILE: android/app/src/main/java/com/kivo/cinema/ui/compose/PlayerActionRouter.kt
  OLD : '/** 轻提示：1.8s 后自动消失（REX M08-03 停留 1.0–2.0s）。 */'
  NEW : '/** 轻提示：1.8s 后自动消失（落在 1.0–2.0s 停留区间）。 */'
  当前 L247: /** 轻提示：默认 1.8s 后自动消失（落在 1.0–2.0s 停留区间）；起播规格等可指定停留时长。 */
  ```
  该 D6 新文已被并发 WIP 二次改写（同时 `showToast` 新增 `durationMs` 形参），故 `verify` 会报「新文未落地」→ `rule_problems` 非空 → `verdict=FAIL`。**README「`verify` 判词 PASS（可重跑）」在当前工作区不可复算。**

### 问题（按严重度）
- **P1**：`verify` 当前会 FAIL；README 三条复算命令因会覆写证据文件，按只读铁律未原样执行（等价复算见 §1）。
- **P3**：L9 复算命令、两脚本语法/import、依赖均通过 ✓（无环境限制需披露）。

---

## 7. 证据齐备性与一致性（要点 7）

### 方法与实际命令
`find docs/evidence/p260/code-derex -type f`；逐文件 `Path.exists()`；读 `verify_result.json`；逐行比对 `grep_after.txt` 与独立扫描。

### 实测数字
- README §七 声称的 7 个文件**全部存在 ✓**：`derex_apply.py`、`grep_before.txt`、`grep_after.txt`、`frame_ids_before.txt`、`frame_ids_after.txt`、`verify_result.json`、`build.log`。
- `device/before/`、`device/after/` 各 4 页 `png` + `xml`（共 16 个）**全部存在 ✓**。
- `grep_after.txt`（17 行）与「执行者 SCAN_EXTS 口径」独立实测**逐行一致 ✓**；与用户「目录全量」口径**不一致**（18 行，见 §1a）。
- `verify_result.json`：
  ```
  {"rule_count":140,"rule_problems":[],"hit_lines":17,
   "counts":{"product":0,"whitelist":17,"unknown":0},
   "violations":[],"frame_id_lines":[],"verdict":"PASS"}
  ```
  - `rule_count=140` ✓（ast 独立复算一致）。
  - `counts` 在 SCAN_EXTS 口径下一致 ✓；**目录全量口径下 `unknown` 应为 1**（`redline_gate_result.json` 的 `rex-design`，按执行者自身正则即判 unknown）。
  - **`frame_id_lines: []` 与实测不符**：`PlayerBottomBar.kt:38 （0170 实测）` 为残留参考录屏帧号；`frame_ids_after.txt` 仅 1 字节（空行）也因此不成立。
  - `verdict: PASS` 仅在「执行者窄口径 + 时刻 T 工作区」成立；当前工作区不可复算（§6）。
- **真机对照（K40 四页、像素差 0.063%、md5 `7bd86c…`、语义树节点全等、`adb install -r Success`）**：**未验证** —— 二进制 `png`/`xml` 仅能确认存在；缺独立基线 APK 与可复算的像素/语义树比对链；README §六 自承「基线 APK 不是 261 记录的那一个」「取证过程自身改变了应用状态」「播放页 dump 无文本节点」，A/B 非同一构建基线。审计无权、也无法重建该对照，按铁律标「未验证」。

### 问题（按严重度）
- **P1**：`frame_id_lines: []` / `frame_ids_after.txt` 空与实测（0170 残留）不符；`verify_result.json: PASS` 不可复算。
- **P2**：`grep_after.txt` 的 17 行仅在收窄口径下成立，与用户「目录全量」口径差 1 行。
- **P3**：真机像素/语义树对照、编译复现均为静态证据，标「未验证」。

---

## 8. 不认可或无法复算的执行者结论（单列）

| # | 执行者结论 | 审计判定 | 依据 |
|---|---|---|---|
| 1 | 「参考录屏帧号/动效编号 **58→0**，出处编号零残留」 | **不认可** | `PlayerBottomBar.kt:38 （0170 实测）` 确定性残留；`FRAME_IDS` 集合缺 0170（L11 `rex-grid-…0170…` 佐证同系列）；清理前 `frame_ids_before.txt` 也无 0170，从未处理 |
| 2 | 「语义 grep **137→17 / product 120→0**，仓库 HEAD（清理前）」 | **部分不认可** | 137/120 仅在「HEAD + 2 WIP 工作区版本」混合口径下成立；纯 `git archive HEAD` = 144/127。2 个 WIP 工作区版本已含 D6 清理，基线被污染、少算 7 行 |
| 3 | 「`git diff --stat HEAD`：**60 文件 / 160 / 159**」 | **无法复算** | 当前实测 65→67 文件 / 422→447 insertions / 228→232 deletions；工作区审计期间并发漂移 |
| 4 | 「140 条改写规则**全部落在注释/KDoc/字符串/docstring**」 | D6 规则本身抽查成立，但**当前全部变更不成立** | `git diff HEAD` 含 275 行 Kotlin 可执行改动（函数签名/依赖/常量），8 个非 D6 文件混入；当前 diff 非隔离提交集 |
| 5 | 「`verify` 判词 **PASS**（0 问题 0 违规），可重跑」 | **当前不可复算，且会 FAIL** | 独立复算 `new_text_missing=1`（`PlayerActionRouter.kt` 新文被并发改写）；三条复算命令均会覆写证据文件，按只读铁律未原样执行 |
| 6 | 「新增 `.kt` = **0**，行数净 0，既有超闸文件未动」 | **与当前工作区不符** | 5 个未跟踪 `.kt`（`PlaybackPrefs`/`PlayerLifecycle`/`PlayerStartPolicy`/`SettingsPicker`/`SettingsScaffold`）；`MpvPlaybackSession.kt` 442 行 +13（并发 WIP） |
| 7 | 「**assembleDebug 真实重编译通过**（compileDebugKotlin 非 UP-TO-DATE、BUILD SUCCESSFUL）」 | 日志文本层面成立，**编译复现未验证** | `build.log` L79/L92 属静态证据；不允许跑 gradle，且工作区已漂移，日志对应时刻 T 的代码 |
| 8 | 「K40 四页**零回归**（设置 0.063%、播放逐字节全等）」 | **未验证** | 二进制截图仅能确认存在；无独立基线 APK，README 自承 A/B 非同一构建、取证改变应用状态 |
| 9 | 「扫描 **2333 个文件，前后同一口径**」 | **口径不实** | 2333 = SCAN_EXTS 口径；before 用 HEAD 树、after 用工作区树（文件集不同），且「目录全量」被收窄为扩展名白名单，漏 `.json` 等 |

---

## 9. 最终判定

**`FAIL`**

判定基准是用户 2026-11-13 拍板的「语义判据（产品名/出处署名零残留）+ 目录全量 + diff 逐条分类」，而非执行者自定义的 `HIT/PRODUCT/WHITELIST/FRAME_IDS` 窄正则与「时刻 T」自认证。

**阻断项（任一即构成 FAIL）**：
- 语义闸门硬失败：`0170 实测` 参考录屏帧号确定性残留（D6 自身 `FRAME_IDS` 集合缺项），「出处署名零残留」不成立。
- 清理前基线口径不实：README 标注的「仓库 HEAD（清理前）137/120」实为混合态，纯 HEAD 为 144/127；「目录全量」被收窄为扩展名白名单。
- 当前 `git diff HEAD` 全部变更含 275 行超出允许例外的 Kotlin 可执行改动，且两个 WIP 与 D6 纠缠、不可隔离提交，不满足「只改注释/字符串」与「隔离提交」约束。
- `verify_result.json: PASS` 在当前工作区不可复算（会 FAIL）；diff 数字、新增 `.kt` 数、帧号 0 等关键结论不可复算或错误。

**公允说明**：`140 条 / 58 文件`、17 行白名单、两类允许例外、L9 复算命令、`measure_*.py` 语法/import、RULES 自身落在注释/字符串、`build.log` 日志文本等，在独立复算下成立；`DesignTokens.kt`/`PlaybackEngine.kt`/`PlayerGestureFlash.kt`/`PlayerGestureLayer.kt` 等改写质量问题属 P2 级。工作区在审计期间（04:22–04:25）被外部进程并发写入（262 播放设置 WIP），是多项「清理后」数字不可稳定复算的重要背景，但**不能**据此免除 `0170` 残留、基线口径污染、FRAME_IDS 缺项等 D6 自身缺陷。

**建议（不构成写入操作）**：补齐 `FRAME_IDS` 对 0160–0189 全系列（至少 0170）并清理 `PlayerBottomBar.kt:38`；以纯 `git archive HEAD` 重建并如实标注清理前基线；把两个 WIP 与 D6 注释改动解耦或由用户重新拍板提交口径；隔离非 D6 的并发 WIP 后再取 `git diff HEAD` 复核；修正改写残句/语义反转；重跑并在**静态工作区**下重取 `verify`/`build.log`/真机证据。

---

*审计结束。除本文件外未修改任何文件；未执行 `git add`/`commit`；未触碰两个 WIP；未原样执行会覆写证据文件的 `snapshot/verify` 命令。*
