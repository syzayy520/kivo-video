# D6 · 清除长期代码里的参考产品名痕迹（证据，含独立审计与修复）

日期：2026-11-13 · 设备：K40 `b6e950ee`
**清理前基线提交固定为 `f97dd2ea`**（不随 HEAD 漂移）；审计期间并发提交把 HEAD 推到 `1643a141`，见 §六。

## 一、拍板口径（用户 2026-11-13）

| 项 | 结论 |
| --- | --- |
| 闸门①判据 | **语义判据**：产品名 / 出处署名零残留；子串误报与冻结参考功能路径列入白名单，白名单与复算脚本一并入库 |
| 范围 | **目录全量**：`backend/`、`include/`、`android/app/src/main/`（含 `CMakeLists.txt`、`.cxx`）、`tools/`（含 `.py`/`.json`，排除 `tools/reference/`）；顶层 `docs/`、`artifacts/` 为冻结归档不清理 |
| 交付 | 隔离提交、暂不推送 —— **因并发提交，提交口径未解决，本卡未提交**（§六） |

> 措辞更正（复审 P3）：本卡扫描用的是 `SCAN_EXTS` 扩展名集合（`.kt/.cpp/.cxx/.hpp/.h/.py/.txt/.json`
> + `CMakeLists.txt`），并非真正「不限扩展名」。第二轮复审另做了 **FULL 口径（2455 文件，不限扩展名）**
> 对照，两口径结论一致（18 命中 / 0 product）。下文「目录全量」均指此扩展名集合口径。

字面 `grep -i rex|senplayer` 零残留不可达：`barExit`（b-a-**rEx**-it）、`ShaderExecution` 等为子串误报；
`docs/reference/rex-teardown/...`、`artifacts/rex-measurements.txt` 是脚本与复算链的功能路径。
强行零残留须改标识符/常量，与 D6「只改注释/字符串、不改逻辑」冲突。

## 二、复算命令（仓库根 `C:\kivo video`）

```bash
python docs/evidence/p260/code-derex/derex_apply.py apply              # 幂等；已应用则写 0 文件
python docs/evidence/p260/code-derex/derex_apply.py snapshot before    # 固定 f97dd2ea 纯提交态
python docs/evidence/p260/code-derex/derex_apply.py snapshot after     # 当前工作区
python docs/evidence/p260/code-derex/derex_apply.py verify              # 判据 + 144 条规则复算
```

`RULES` 为逐条人工拟定的「原文片段 → 新文片段」，每条断言唯一命中后才写盘；`apply` 已改为幂等，
可安全重跑。`BASELINE_COMMIT` 固定，避免 HEAD 前移污染清理前基线（审计 P0 修复项）。

## 三、grep 计数（**修订后**，宽帧号 + 含 `.json` 的目录全量口径）

| 指标 | 清理前 `f97dd2ea` 纯提交 | 清理后工作区 |
| --- | --- | --- |
| 扫描文件 | 2384 | 2396 |
| `rex|senplayer` 命中行 | **145** | **18** |
| ├ product（违规） | **127** | **0** |
| ├ whitelist（已批准） | 18 | 18 |
| └ unknown | 0 | 0 |
| 参考录屏帧号 / 动效编号 | **61 行** | **0 行** |
| `verify` 判词 | — | **PASS**（144 规则、1 条 soft、0 问题、0 违规） |

**两个「清理前」数字都要看**（审计 P0）：
- `145 / 127` = 纯提交 `f97dd2ea`（本表，可复算）；
- `137 / 120` = 清理前**实际工作区**（含主助手 2 个未提交 WIP，它们已自行清掉 7 行），
  这是第一版口径（`SCAN_EXTS` 未含 `.json`、帧号正则较窄，故另有 `58` 行帧号）。
  第一版 `audit.md` 中「137/120 污染基线」的批评，已由固定基线 + 双口径披露解决。

### 保留的 18 行白名单（逐行可查 `grep_after.txt`）

| 文件 | 行数 | 理由 |
| --- | --- | --- |
| `PlayerBarsHost.kt` / `PlayerMotion.kt` | 6 | `barExit` / `barExitMs` 子串误报 |
| `shader_execution_order.hpp` | 6 | `ShaderExecutionStep` / `FakeShaderExecutionPlanner` |
| `p5h_color_hdr_protected_tests.cpp` | 3 | 同上，测试符号名 |
| `PlayerTokens.kt:11` | 1 | `artifacts/rex-measurements.txt`，冻结证据归档路径 |
| `tools/dump_home_spec.py:6` | 1 | `docs/reference/rex-teardown/rex-package/strings/`，脚本功能依赖 |
| `backend/tools/governance/redline_gate/artifacts/redline_gate_result.json:39` | 1 | 治理闸门生成物，引用冻结设计稿名 `236-ui-align-rex-design.md`；改它会破坏既有证据（审计 P2 修复项：纳入扫描并列入白名单） |

保留的技术/协议/内核名：`mpv`、`libmpv`、`FFmpeg`、`swresample`、`MediaCodec`、`TMDB`、
`Emby`、`SMB`、`PingFang`、`SF Pro`、`Noto`、`AirPlay`、`DLNA`、项目自有编号
（`239/255/220/250`、`v3.1f`、`§1.5`、`IMG_0136`、`M04-Sxx`）均未改动。

## 四、独立审计 FAIL → 修复清单

第一次独立审计（`audit.md`，2026-11-13 约 04:20–04:26）判 **FAIL**。逐条处理：

| 审计发现 | 级别 | 处理 |
| --- | --- | --- |
| `PlayerBottomBar.kt:38 （0170 实测）` 帧号漏清；`FRAME_IDS` 缺 0170 | P0 | ✅ 已清为「（实测）」；正则放宽为 `\b0(?:16\d|17\d)\b` 覆盖 0160–0179 |
| 「清理前」用 `HEAD + 2 WIP` 混合态、随 HEAD 漂移 | P0 | ✅ 固定 `BASELINE_COMMIT=f97dd2ea` 纯提交；双口径数字全部披露 |
| 目录全量漏 `.json`，治理闸门 `rex-design` 一行未纳管 | P2 | ✅ `SCAN_EXTS` 加 `.json`，该行列入白名单 |
| diff 混入 262 卡功能改动 / 「60 文件 160/159」不可复算 | P1 | ⚠️ 属并发提交，非 D6 缺陷；改为如实披露（§六），不再引用旧 stat |
| `verify` 因 `PlayerActionRouter.kt` 注释被并发送改而 FAIL | P1 | ✅ 该规则标 `SOFT_RULES`：只校验原文消失，不强制本卡新文仍在；`verify_result.json` 明列 relaxed 规则 |
| 改写质量：`DesignTokens` 叠词、`PlaybackEngine` 语义反转并引入「旧内核」、`PlayerGestureLayer:69` 无谓语残句、`PlayerGestureFlash` 去主语 | P2 | ✅ 四处全部改写：实测硬参数 / 单一 demux 线程，单一读路径 / 左半屏调亮度右半屏调音量 / 交互反馈用居中图标 |
| 编译与真机为静态证据、审计未复现 | P3 | 如实标注；修复后已重跑编译（§五） |

修复后 `verify` 复算：**144 条规则（含 4 条修复规则）、0 问题、PASS**；`apply` 幂等复算写 0 文件。

## 五、闸门验收

| 闸门 | 结果 | 证据 |
| --- | --- | --- |
| ① 语义 grep 零残留 | ✅ PASS | `grep_before.txt` / `grep_after.txt` / `verify_result.json` / `frame_ids_*.txt` |
| ② `assembleDebug` 编译过 | ✅ PASS | `build.log`：改动后真实 `compileDebugKotlin`（非 UP-TO-DATE）+ `BUILD SUCCESSFUL`；审计标「静态证据未独立复现」 |
| ③ 真机播放/设置/详情不回归 | ✅ 单侧 PASS（见披露） | `device/before/`、`device/after/` 各 4 页 .png + .xml；`adb install -r` = `Success`；清理后 APK md5 `7bd86c36508b384c67fe99e161ca16ac` |
| ④ 不新增超闸文件 | ✅ PASS（本卡） | 本卡新增 `.kt` = 0、行数净 0；并发 262 卡的 5 个新 `.kt` 已随其提交，不属本卡 |

### 真机对照（K40 `b6e950ee`）

| 页面 | 语义树 | 像素差（排除状态栏 y<100） | 判定 |
| --- | --- | --- | --- |
| 设置 | **内容全等 / 节点全等**（51 节点） | **0.063%**（1566 / 2,484,000 px） | 零回归 |
| 播放 | **内容全等 / 节点全等**（2407 B 逐字节相同） | 35.0%（视频帧内容不同，属预期） | 零回归 |
| 首页 | 主视觉文本前 5 项一致；新增「继续观看」区 | 16.4% | 取证副作用，非代码差异 |
| 详情 | 前后进入不同详情页 | 97.0% | 点击路径差异，非代码差异 |

清理后前台校验：`MainActivity` → 设置 tab → `DetailActivity` → `PlayerComposeActivity`，四页正常渲染无崩溃。

## 六、披露（不掩饰的限制与未决项）

1. **重大环境事实：并发提交把 D6 成果劈成两半。** 本卡开工时 HEAD=`f97dd2ea`（工作区仅 2 个 WIP）；
   执行与审计期间，另一助手完成并提交/推送了 **262 播放设置子页（`1643a141`，`@{u}..HEAD = 0`）**，
   带走了本卡的一部分注释清理（如 `PlayerActionRouter.kt` 的 toast 注释被其二次改写），
   其余部分仍留在工作区。`TASKS.md` 顶部已被 262 卡改写。**因此当前 `git diff` 不是干净的 D6 隔离集**，
   「60 文件 / 160+159」等旧数字已作废，本文不再引用。
2. **提交口径未解决，本卡未提交、未推送。** 用户批准「只提交本卡 + 证据、两个 WIP 不碰」，
   但 `PlayerBarText.kt`（`barTextWeight` 定义）/ `PlayerTopBar.kt`（跨文件调用）是**必须成对提交**关系：
   只提交其一会编译断裂、且提交态残留 `rexTextWeight`。叠加并发提交后，
   隔离提交已无法在不替主助手决定其 WIP 的前提下完成。**需用户重新拍板后再动 git。**
3. **基线 APK 不是 261 记录的那一个**：清理前设备已装 APK md5 `4248a32e159bfe6aa6728c9f2a88d3bc`，
   与 `TASKS.md` 261 记录 `31a8f8f63ff2b747da0be2a0a329e85d` 不符（应为并发助手装的包）。
   故非严格「同一构建基线 A/B」；**行为零变化主要由静态等价证据支撑**（改写全部落在注释/字符串/
   等价标识符 + 真实重编译 + 安装成功），真机对照为辅助。
4. **取证自身改变了应用状态**：基线阶段点过一次播放 → 生成「继续观看」卡片 → 清理后首页布局下移，
   同一坐标落到本地文件详情页。首页/详情像素差由此产生。补取 TMDB 详情同一入口未成功，已停止强行造对照。
5. **播放页 dump 无文本节点**，以「逐字节全等」判零回归；像素差不作判据（视频帧随时间变化）。
6. **既有超闸 `.kt`**（`PlayerActivity.kt` 901 行等）为 D1/D2 既有债，本卡不扩 scope。
7. 独立审计对编译与真机只核到日志/截图层，**未独立复现**，按「静态证据」标注。

## 七、文件索引

```
derex_apply.py          改写映射(144) + 固定基线快照 + 判据复算（幂等可重跑）
audit.md                第一次独立审计报告（FAIL，保留原貌不改写）
grep_before.txt         清理前 145 行命中（f97dd2ea 纯提交）
grep_after.txt          清理后 18 行命中（全部白名单）
frame_ids_before.txt / frame_ids_after.txt   帧号出处 61 → 0
verify_result.json      修复后判词 PASS
build.log               改动后真实增量编译日志
device/before/*         清理前 4 页截图 + dump
device/after/*          清理后 4 页截图 + dump
audit.md                第一轮独立审计（FAIL，原貌保留）
audit-round2.md         第二轮独立复审（PASS_WITH_DISCLOSURE）
```

## 八、第二轮独立复审结论（2026-11-13）

报告 `audit-round2.md`（第一轮 `audit.md` 原貌保留）。复审独立复算，未采信本卡脚本判词：

- 语义零残留：FULL（2455 文件）与 EXTS（2396 文件）两口径均为
  **18 命中 / 0 product / 0 unknown / 18 whitelist**；宽帧号 0160–0179 零命中，`0170` 全仓 0；
  18 行白名单逐行无夹带。
- 基线：`git archive f97dd2ea` 内存解包得 **145 / 127 / 18 / 61**，与 `grep_before.txt` 逐行 0 差异；
  「清理前工作区 137/120」的双口径差异已如实披露。
- 第一轮 6 个 P0/P1/P2 **全部闭环**；`RULES=144 / 58 文件 / 1 soft`，独立逐条核对
  `rule_problems=0` → `verify` 可 PASS；实跑 `apply` 写 0 文件，前后 2530 文件聚合 hash、
  `git status`、`git diff` 三者完全一致；复算链三脚本可 import；`build.log` 日志层面成立。
- 残留 **P0/P1/P2 = 0，P3 = 6**（均为披露级：`PlaybackEngine` L10/L11 重复用词、
  「目录全量」措辞实为扩展名白名单、自有编号 `M04`/`IMG_0136`、编译与真机未独立复现等）。

复审同时确证：**收尾时工作区再次并发漂移**（新增 `av-settings.md`、
`MpvPlaybackSession.kt` / `PlaybackPrefs.kt` 被重写，diff 由 55 → 57 文件），属并发助手后续 WIP，非 D6。

> **最终状态：D6 技术闸门达标（PASS_WITH_DISCLOSURE）。用户 2026-11-13 拍板全量提交并推送；
> codex 接管后在线重编译 BUILD SUCCESSFUL、修掉 P3-1、回写台账，54 文件 + 本证据已随 D6 收口提交入库（见 §九）。**

## 九、收口补记（2026-11-13，codex 接管）

前助手额度耗尽停在 D6 复审中途；codex 接管，按用户拍板「全量提交并推送」收口。§一–§八 审计原貌保留，本节只记接管后的处置：

- **提交口径已解决**（核销 §六.2「两 WIP 成对提交死锁」）：`barTextWeight` 改名自洽——1 定义（`PlayerBarText.kt:99`）
  + 2 调用（`PlayerBarText.kt:71` / `PlayerTopBar.kt:98`），全仓 `rexTextWeight`=0；TASKS.md 262 条明确把该改名划归 D6。
  262/263 并发改动已在 HEAD（`ec554a96`），当前工作区 **54 个 M 文件即干净 D6 隔离集**（`git add -u` 干跑=54，
  逐条核为纯注释/字符串 + 等价改名）；`origin..HEAD=0`。
- **闸②本会话真实复现**（补 §六.7「编译未独立复现」）：codex 账户缓存缺 compose `2.1.21`（只有 2.0.21），
  首次 `--offline` 构建 **FAILED**（插件解析失败，纯环境非代码）；改用项目 `settings.gradle.kts` 已声明的真实仓库
  （google / mavenCentral / gradlePluginPortal，**无代理替换**）在线补齐 → `compileDebugKotlin` + `buildCMakeDebug[arm64-v8a]`
  实际执行、**BUILD SUCCESSFUL in 2m1s**（`build-codex-reverify.log`）。`packageDebug` UP-TO-DATE ⇒ 编译产物与既有
  05:01 APK 字节一致（md5 `eabe20fcf5a5771dcf2099d1cd1b6ca2`），佐证零逻辑改动。
- **P3-1 已修**：`PlaybackEngine.kt` L10/L11 重复用词改为「…全 FFmpeg 解码覆盖，单一读路径。」（余 P3=5 均披露级）。
- **残留判据复核**：codex 账户无可用 Python（WindowsApps 桩），改用 Grep 工具复核——android 侧 7 命中全白名单
  （`barExit`/`barExitMs` 6 + `PlayerTokens.kt:11` 冻结归档路径 1）/ 0 产品名，与 §三 18 白名单总数一致。
- **范围纪律**：`artifacts/p262-playback-settings/`、`artifacts/p263-av-settings/` 原始截图保留本地**不提交**
  （精选证据已在 `docs/evidence/p262`、`p263`）；本次只提交 54 文件 + `docs/evidence/p260/` + 台账回写（TASKS.md / DISPATCH.md）。
- **push 目标核对**：`origin = https://github.com/syzayy520/kivo-video.git`（真实 GitHub 源，fetch/push 同一，无代理）。
