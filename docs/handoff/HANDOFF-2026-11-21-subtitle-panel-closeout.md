# 交接 · 面板材质 v2 + 字幕修复轮（2026-11-21 收口）

> 写给人：接手本线的下一位助手。本文件是**自足**的：不依赖上一会话记忆。
> 上一棒：软件开发专家团（项目经理协调，调研/全栈/测试/审查分工完成）。
> 状态一句话：**本轮全部工作已收口且真机全绿，无进行中的半成品；待办均为登记项，开工前按 §5 优先级与用户确认。**

---

## 0. 铁律（用户多次重申，违反即返工）

1. **同一工作目录 `C:\kivo video`，不新建项目包**；单文件单职责、先设计文件家族树再动手、不堆砌代码（长期记忆 `kivo-working-contract.md`）。
2. **设备只用 `b6e950ee`（Redmi K40）**；`dd137cf7` 属其他助手，**禁止对它执行任何 adb**；**禁止 `adb uninstall`**（会删私有证据与媒体）。
3. 工作区有 **127 项其他助手的未提交改动**（`git status --porcelain | wc -l` = 127，2026-11-21 02:30 基线）：不覆盖、不回退、不 reset。**未经用户授权不 commit / push / 发布。**
4. 验收只认实测证据（像素读数、日志原文、run-as 文件清单），逐项 PASS/FAIL/未取证 + 数字；静态审查 ≠ 测试通过。
5. 签名固定 `android/keystore/kivo-debug.jks`，不改任何 gradle/SDK/签名配置。

## 1. 本轮完成了什么（全部已验证，可直接引用）

### 1.1 面板材质 v2（P10）
- 6/6 PASS。判据经用户拍板修订：**保透光（α 顶 62%→底 65%，底色 #151B23），亮场标题 ≥4.5 / 副题 ≥3.5 / 组头 ≥2.8，暗场仍 ≥6:1**（spec §6.1）。
- 实测：亮场标题 4.91 / 副题 3.75 / 组头 2.80；步进圆底 7%+描边 12%、按下 +12%；开关绿轨 #34C759。
- 证据：`docs/evidence/visual-polish/panel-v2/ACCEPTANCE.md`（§7 判据总表、§8.1–8.3 像素读数）。
- 规范：`docs/design/subtitle-plane/p10-panel-material-spec.md`（§15 参数表；唯一参数源 `ui/compose/player/PlayerPanelStyle.kt`，材质唯一入口 `playerPanelSurface`）。
- TEMP-DIAG 埋点已全部移除（grep `TEMP-DIAG|KivoPanelGeom|p9-panel.log` 零命中）。

### 1.2 字幕修复轮（P9 后续，11 项 + 4 项返工）
- 第一轮 11 项：SUB-M1…M4 / L1…L5 / C中-2 / C低-3（原始旧编号记录已丢失，这是重新审计的新编号，定义见家族树与审查报告）。
- 独立审查发现 4 项返工 R1–R4 + M4 取证，已全部修复：
  - **R1**：导入复用分支补主/次互斥（`ui/compose/PlayerSubtitleImport.kt`，先关次字幕，失败中止）。
  - **R2**：重置整段纳入 `subtitleStepLock`（`ui/compose/player/subtitle/SubtitlePanelActions.kt:140`，与步进同锁；锁序 stepLock→ioLock，无反向）。
  - **R3**：`playback/subtitle/SubtitleImportStore.kt` 类级 `lock`（:181），import 查重临界区 :77 / remember :104 / forget :136。
  - **R4**：`subtitlePathKey`（同文件 :16，canonicalFile，异常回落原串）统一三处路径比较：M1 匹配、sharedBefore 计数、findRegisteredByPath。
- **真机复验 ROUND2 全绿**（2026-11-21 02:00–02:22，b6e950ee）：M1 改名复用不增文件/不增轨；R1 无主次同轨时刻；M2 唯一文件 disk=true 双清 / 跨片源共享 disk=false 保留；L1 重置内核+prefs 回 40/100/40/默认色；L5「底部边距/仅文本字幕」无裁剪；面板 #4 步进 12% / 控制栏 8%（纯黑区 (20,20,20)）。
- 证据：`docs/evidence/p9-subtitle-reaccept/ROUND2-DEVICE.md` + 29 个 `R2-*` 文件（安装原文、p220-status.log、mpv-kernel.log、prefs、像素采样）。
- **M4 已裁定：不扩展兜底**（删次轨无 secondary-sid 残留、删唯一主轨正确 sid=no；剩余场景登记为 O2，见 §5）。
- 收口登记：`docs/design/subtitle-plane/p9-subtitle-perfection-family-tree.md` **§6.5（发现）+ §6.6（销项与裁定）**——这是本线的权威状态文档，接手先读它。

### 1.3 被测产物
- APK：`android/app/build/outputs/apk/debug/app-debug.apk`，**2026-11-21 01:55:59 (+0800)，67,880,299 字节**，已覆盖安装到 b6e950ee（lastUpdateTime 02:00:50，firstInstallTime 2026-11-10 18:25:08 不变）。
- 本轮改动全部在 127 项脏区内，未新增/删除文件（除 docs 证据与本交接）。

## 2. 关键契约速查（改字幕/面板前必读）

- 家族树与契约：`docs/design/subtitle-plane/p9-subtitle-perfection-family-tree.md` §2/§3。要点：
  - 字号单位 scaled pixel（720 基准），默认 40 三处同源（C++ `cpp/mpv/mpv_sub_options.cpp` / Kotlin `SubtitleStyle.DEFAULT_FONT_SIZE` / 面板重置）。
  - `sub-margin-y` 只作用非 ASS 文本字幕（文案必须写「仅文本字幕」）；`sub-pos` 写侧夹 0–100。
  - 全局样式落 `subtitle_prefs`，起播 `MpvStartSequence` 在 loadFile 前回放；**sub-delay 按片源记忆**（`SubtitleSourceKey`），不随全局样式。
  - 导入字幕：`filesDir/subtitles/<md5前12>-<消毒名>`，登记 `subtitle_imports`（片源键→路径集合）；**导入轨是会话级**（设计边界，非缺陷）。
  - **主次不许同轨**（sid == secondary-sid 会叠印）；所有选主轨路径必须先处理次字幕冲突。
  - 删除外挂：`sharedBefore != 1` 不删盘；跨片源引用保护在 `forget` 内。
- 面板材质（**v3 2026-11-21，SenPlayer 对齐**）：唯一参数源仍是 `PlayerPanelStyle.kt`；底=白 40%→37%、圆角 13.5dp、文字纯白/77%/70%、面板内控件全部反相为黑（行按压 4%/步进按压 16%/步进圆底 12%+描边 20%/分隔线黑 10%）。⚠️ `pressCircleColor=0x14FFFFFF`(白 8%) **只用于画面上的控制栏**（底栏文字钮），不许改成黑、不许与 `stepperPressColor`(黑 16%) 合并。`PressIndicator` 全仓 **2 处**（PlayerBarText:95 底栏 8% / PlayerStepperRow:250 步进 16%）；**顶栏已按 SenPlayer 实测去掉按压圆**（只保留 glyph 变暗）。详见 spec §16。
- 并发：字幕步进与重置共用 `subtitleStepLock`（SubtitlePanelActions.kt:83 顶层 private val）；样式落盘 `SubtitleStyleStore.ioLock`、导入登记 `SubtitleImportStore.lock` 均为**类级**（调用方每次 new 实例，实例锁无效）。锁序 stepLock→ioLock，禁止反向、禁止嵌套 ImportStore.lock 与 ioLock。

## 3. 环境与操作手册（照抄可用，都是本轮实测过的）

### 构建
```
cd "C:/kivo video/android" && JAVA_HOME="C:/Android/jdk-17.0.13+11" ANDROID_HOME="C:/Android/sdk" \
  MSYS_NO_PATHCONV=1 ./gradlew.bat :app:assembleDebug --console=plain --no-configuration-cache
```
（PATH 无 java，必须显式 JAVA_HOME；约 50s。）

### 安装（关键坑）
- adb：`C:/Android/sdk/platform-tools/adb.exe`，Git Bash 加 `MSYS_NO_PATHCONV=1`。
- **HyperOS USB 安装弹窗 9 秒自动拒绝**：`adb -s b6e950ee install -r <apk>` 发起后立刻点「继续安装」（竖屏 1080×2400 约 **347,2111**，可间隔 1s 连点 2–3 次）。上一轮复验"无原因失败"即此坑，本轮按此一次成功。
- 成功判据：`Success` + dumpsys `lastUpdateTime` 更新、`firstInstallTime` 不变。

### 播放页取证
- 启动：`am start -S -n com.kivo.cinema/.ui.compose.PlayerComposeActivity --es kivo.media <路径> --ei kivo.seekSec 2 --ez kivo.freeze true`。
- 测试片源：`/sdcard/Movies/Kivo/kivo-panel-{bright,mid,dark}.mp4`（均无内嵌字幕轨）。
- 控制条 3 秒自动隐藏：tap/sleep/screencap 链在**同一条 shell**；首击可能只唤出控制条。顶栏 y≈180：音频 x≈2075、字幕 x≈2160。
- 按住截图：`input motionevent DOWN x y; sleep 0.3; screencap …; input motionevent UP` 单链。
- **坑**：① 本机 MIUI **logcat 全空**（`-d` 0 行），应用日志一律走文件：`run-as com.kivo.cinema cat files/evidence/p220-status.log`（smbTrace）与 `files/mpv-kernel.log`（`grep -a`）；② `uiautomator dump` 对面板返回陈旧节点，行定位用代码顺序+像素几何；③ Read 图片工具有缓存错位，**像素读数一律 Python/PIL 直接采样**；④ 长按底栏播放键会进「已锁定屏幕」浮层（`am start -S` 恢复），按压测量用「倍速」文字钮/顶栏图标替代。
- 私有目录：`run-as com.kivo.cinema ls -l files/subtitles/`；prefs：`run-as com.kivo.cinema cat shared_prefs/subtitle_prefs.xml`（及 `subtitle_imports.xml`）。

## 4. 已实现 vs 已验证的边界（不要夸大）

- R2/R3 并发锁：代码落点已复核+编译通过，**并发交错本身无注入通道，未真机取证**。
- 未取证（无构造手段，非阻塞）：同文件多轨保护（M1 复用使 UI 构造不出 shared>1；跨片源保护已实测）、失败注入类（内核拒绝/SAF 读取失败）、重置×步进并发。
- 亮场对比度只在白场测试片最不利条件取样；**真实雪景素材未复测**，组头 2.80 贴着修订判据 2.8 的下限。

## 5. 待办（全部登记未启动；开工前与用户确认优先级）

按建议优先级：

1. ~~**O1 · 次字幕面板锚点缺失**~~ —— **DONE 2026-11-21 03:0x（接手线）**：`PlayerPanelHost.anchorFor` 的 captions 分支补 `"subtitleSecondary"`（唯一改动点，1 个 kind + 注释）；真机 `b6e950ee` 实测字幕卡与次字幕面板顶边同为 **82.18dp**（期望 82）、左缘同为 **608.73dp**（期望 608.7）、宽 248dp，不再落 Center。设计与判据见家族树 §6.7，证据 `docs/evidence/p9-subtitle-reaccept/O1-DEVICE.md` + `O1-0{1..5}*.png`。
2. ~~**KivoSub 埋点迁移**~~ —— **DONE 2026-11-21（接手线）**：删除 `MpvSubtitleControl.kt` 的
   `Log.i("KivoSub", …)` 与 `import android.util.Log`；改由调用方 `SubtitlePanelActions` 的
   `sub-remove ok` trace 输出 `sid=` / `sec=`（smbTrace → `p220-status.log`，真机可读）。
   `grep -rn "KivoSub\"" android/app/src/main/java` 零命中。**今后字幕埋点一律 smbTrace。**
3. ~~**O2 · M4 剩余场景取证**~~ —— **DONE 2026-11-21（接手线）**：主 A（外挂 id=2）+ 次 B（内嵌 id=1）
   时删 A，trace `sub-remove ok id=2 … sid=1 sec=no`、内核 `Track removed` → `Set property: sid="1"`。
   **内核自选到 B 且次位清成 no ⇒ 无同轨叠印，裁定维持不扩展兜底。** 证据
   `docs/evidence/p9-subtitle-reaccept/O2-DEVICE.md` + `O2-0{0,1,2}*`。
4. ~~**O3 · 顶栏按压圆有效亮度 ≈5%**~~ —— **DONE 2026-11-21（接手线）**：按用户「参考 SenPlayer，他怎么做你就怎么做」实测：SenPlayer 顶栏按住 0.12 s / 0.4 s 与未按帧的图标区**完全一致**（亮像素 1859→1859）⇒ 顶栏**没有按压圆**。已删除 `PlayerTopBar` 的 28dp 圆，只留 glyph 变暗；5% vs 8% 的分歧随之消失。
5. **O4 · 长按底栏播放键进锁屏浮层**：行为是否合意待用户确认。
6. ~~**SUB-M5 · 次字幕延迟行**~~ —— **DONE 2026-11-21（接手线）**：口径拍板为**按片源记忆**（键前缀 `s:`，与主 `d:` 并列）。面板行「次字幕延迟」（0.1 s 步进）、写侧只记忆不重复写内核、起播回放、重置双清全部真机 PASS；读数见 `docs/evidence/p9-subtitle-reaccept/M5-DEVICE.md`、设计见 p9 家族树 §6.8。
7. ~~**面板 v2 审查低项**~~ —— **DONE 2026-11-21（团队批次 `panel-polish-v3`，T1–T4）**：
   描边改 `Stroke(2*hairline)` ⇒ 真机可见 **3px ≈1.09dp**（改前 1.4px）；两壳内容层 + 滚动条 clip 到 `radius`、按压覆盖改 `drawRoundRect` ⇒ 右下角逐行收口、无方角；`LaunchedEffect(state)` + `snapshotFlow` ⇒ 滚动期不再每帧重启；`CastSheet` 全部改取既有 token；规范 §15 标历史、§16.2 按代码回写（含 toggle 16%/headerWash/分隔线三处旧偏差 —— 已随 v3 重定）。
   证据 `docs/evidence/p9-subtitle-reaccept/R0-DEVICE.md` + `R0-0{1..6}*`。
   **本批发现并已修（2026-11-21，`stepper-split` T2）**：`CastSheet` 原本弹在**左上角**；已给 `CastSheet` 加末位 `modifier` 并由 `PlayerComposeRoot` 传 `Modifier.align(Alignment.BottomCenter)`，真机顶边 y≈756（70% 屏高）、底边贴 1080。
8. **已登记大项**（家族树 §6.2–6.4，仅登记）：AI 字幕第二阶段（SettingsAiSub + subtitle-translation-landing.md）；Wave2 minSdk 26→28 + libmpv 重编 `-Diconv/-Duchardet`（GBK 乱码）——**已出家族树草案 `docs/design/backend-core/254-wave2-minsdk28-iconv-family-tree.md`（待你拍板 §6 两项）**；Wave3 sub-codepage/SC 字体/网络字幕 —— **勘察记录已出 `docs/design/backend-core/255-wave3-subtitle-codepage-fonts-network.md`**：`sub-codepage` 未实现且**依赖 Wave2 的 iconv**；SC 字体根因=候选族名写死 `Noto Sans CJK JP`（单文件可修）；网络字幕只有本地路径入口（方案=下载进 cache 复用导入链路）。**R3 可先做；R2 经真机三次反证证伪了「改族名即可」——受限于 FreeType 无 CFF/VF 支持，只能重编 libass+FreeType 或打包 TrueType 简中字体（证据 `FONT-DEVICE.md`）。**。

## 6. 合规与收尾状态

- git 脏区 127 项保持原样；本轮**未 commit / 未 push / 未发布**（用户未授权）。
- 设备已收尾：临时截图/夹具清理、字幕样式重置回默认、`svc power stayon` 恢复 false。
- 本交接与 §6.6 收口登记为本轮最后写入；除此之外 docs 改动只有证据文件。
- 用户决策记录：亮场判据修订=用户拍板「保透光，修订判据」；资源卡 meta 图标随文字改色=项目经理裁定在 §15.1 范围内保留；M4 不扩展=依据 ROUND2 实测裁定。

—— 上一棒到此。接手后先读家族树 §6.5/§6.6 与 ROUND2-DEVICE.md，再按 §5 与用户对齐下一步。

9. **新登记（接手线 2026-11-21）· `PlayerStepperRow.kt` 450 行超铁律**：已出拆分家族树
   `docs/design/ui/replica/playback/player-row-family-tree.md`（5 文件 / 按变化原因拆 / 零行为改动），
   **代码未动**，等用户确认后再落。

10. **新登记（接手线 2026-11-21，已修）· 关闭面板中间闪一下**：根因 `anchorFor("") → Center`；改为由 `shownKind` 派生锚点 + title/items 快照，60fps 逐帧中间区跳变 +37.15 → +0.01。证据 `docs/evidence/p9-subtitle-reaccept/BUG-DEVICE.md`。
11. **新登记（未修，已做 12 轮探针）· 面板退场动画不生效**：800ms 仍 1 帧；Animatable 手动 alpha 仍 1 帧；`if(true)/if(false)` 证明路径正确；**延迟 260ms 清 kind、按 open 判快照、`key()` 钉组合身份** 全部仍 1 帧。结论=关闭同帧面板内容同步消失，**组件内方案无解**；只能走独立窗口或 Activity 层自绘，成本显著。本轮已全部回滚，最终态只保留"中间闪现"修复（复验 0.34）。证据 `BUG-DEVICE.md` §5 / §5.1。

12. **新登记（已修，2026-11-21 夜间）· 行数回归**：backdrop 参数把 `PlayerGlassMenu` 顶到 302 行 → `MenuHeader` 移入 `PlayerGlassMenuParts`（257/138），门禁「行数无新增超限」。
13. ~~**裸色 +33**~~ —— **已治理 2026-11-22**：`ui_gate.py` 的令牌判定从「`tokens/` 目录」扩展到「`*Style.kt` / `*Tokens.kt` / `*Palette.kt`」，并**明示豁免**（报告/JSON 都打印 `exempt` 清单与计数）：豁免 114 处（DesignPalette 54 / PlayerPanelStyle 35 / PlayerTokens 25）；计入裸色 299。baseline 已刷新，`python tools/ui_gate.py` ⇒ **RESULT: PASS**，复跑「裸色与 baseline 持平：299」。
14. **投屏设备行已取证**（`DLNA-DEVICE.md`）：真实设备小爱音箱-8245 / 192.168.5.145，卡片顶 82.2dp。
> 夜间全部收口见 `docs/handoff/OVERNIGHT-2026-11-22.md`。

15. **W3-R3 网络字幕已落地（2026-11-22）**：字幕卡「导入字幕」下加「粘贴字幕链接」（tag `sub-import-url`）；`SubtitleRemoteFetcher`（http/https、5s/10s、8 MiB、HTML 拒收）+ `SubtitleImportStore.importFile`（内容哈希去重）+ `importSubtitleUrl` 统一执行入口（剪贴板与取证探针共用）。真机 5/5 PASS，证据 `URL-DEVICE.md`。
    ⚠️ 验收环境三坑：PC 代理会让 curl 假 503、子 shell 起的夹具会退、**adb reverse 与 Java OkHttp 不兼容**（native 正常）。

16. **W2 iconv 已落位（2026-11-22，未真机验证）**：bionic iconv + `patch-mpv-iconv-android.py` + `-Diconv=enabled` + API 28；
    `minSdk=28`、`KIVO_API=28`、cross 26→28、`KIVO_BUILD=build-28`。二进制验证通过（`iconv_open@LIBC_P` UND / 无 libiconv NEEDED）。
    ⚠️ **测试机 b6e950ee 掉线**（`adb devices` 空），GBK 正向/反证与 HDR/4K 回归**都还没跑**；回滚与待验证清单见 `W2-iconv-LANDED.md`。

17. **W3-R1 字幕编码已达成（2026-11-22）**：`SubtitleEncoding`（UTF-8 严格 → UTF-16 BOM → GB18030 → BIG5）在 `SubtitleImportStore` 落盘前归一化；SAF/URL 两条入口共用，去重口径 = UTF-8 内容哈希。真机 GBK 字幕「中文字幕 GBK 转码测试」正确显示、同内容 UTF-8 版 `deduped=true`。**W2 的 iconv/抬 minSdk 路线已全部回滚**（bionic iconv 实测不含中文）。证据 `ENC-DEVICE.md`。
    ⚠️ 取证通道教训：Store 内的 trace **不要用 `android.util.Log`**（MIUI logcat 读不到），已改成构造注入 `{ smbTrace(it) }`。

18. **编码归一化单测 + 死分支清理（2026-11-22）**：`SubtitleEncodingTest` 9 例全绿；`big5BytesAreClaimedByGb18030` 证明 BIG5 不可达（首/尾字节区间均被 GB18030 包含）⇒ 已删 `BIG5`，只留 `GB18030`。繁体支持需手选编码或 uchardet，与「手选编码 UI」同批。证据 `ENC-UNIT.md`。
