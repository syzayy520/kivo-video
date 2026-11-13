# 待派发任务清单（供其他助手领取）

> 用途：主助手自留的 **设置页 UI 设计+实现**（REX 1:1）已于 261 收口（见文末「主助手自留」）。
> 本清单是**可独立领取**的剩余工作卡。领取前先读 `agents.md` + `.codex/memory/kivo-video-iron-gate.md`
> + `docs/handoff/CURRENT.md`；状态以 `docs/handoff/TASKS.md` 为准，收口后回写 TASKS.md 并推送。
> 每张卡自包含：目标 / 范围 / 闸门 / 证据 / 红线 / 依赖。**未完成家族树设计不得进入实现。**

## 公共红线（所有卡适用）

- 只在 `C:\kivo video` 工作；不新建项目包；`.kt` ≤300 行；单文件单职责；禁止平铺堆函数。
- 固定签名 `kivoDebug`；`adb install -r` 覆盖安装；**禁止 `adb uninstall`**（会删证据/片源）。
  HyperOS 安装弹「USB安装提示」9s 自动拒绝：安装同时 `adb shell input tap 347 2111`。
- 真机取证：K40 `b6e950ee`（或 `dd137cf7`，同为 K40）；模拟器数字无效。证据落 `docs/evidence/<stage>/`。
- 不静默降级：能力缺失必须非空 reason 且用户可见；禁软解回退（尤其 10bit）。
- 凭据/token 仅运行时，**不入仓库/证据/文档**；Emby token 走 `X-Emby-Token` 头，永不进 URL。
- 证据脱敏：个人家庭视频帧模糊；含密码表单态不截图。FLAG_SECURE 页 screencap 为空→改 uiautomator dump。
- 构建：`cd android && JAVA_HOME="C:/Android/jdk-17.0.13+11" ANDROID_HOME="C:/Android/sdk" ./gradlew :app:assembleDebug --offline --no-configuration-cache`。
- git 需 `-c safe.directory="C:/kivo video"`。**并发领取者注意：同一 `android/app/build` 与同一真机不可同时构建/装机**，请串行或错开设备。

---

## D1 · 拆分 MainActivity（任务 #37，既有行数债）

- **目标**：`ui/MainActivity.kt` 357 行 → ≤300，纯重构不改行为。
- **范围**：把内聚子职责抽到既有家族的新文件（如导航/入口编排），不新建包；调用方不变。
- **闸门**：① ≤300 行；② 编译过；③ 真机回归首页/底栏/入口路由不变（截图对照重构前）；④ 不新增超闸文件。
- **证据**：`docs/evidence/p260/mainactivity-split/`（行数 + 回归帧 + 复算）。
- **依赖**：无。**适合度：高**（机械重构，边界清晰）。

## D2 · 拆分 MediaLibraryActivity（任务 #45，既有行数债）

- **目标**：`ui/MediaLibraryActivity.kt` 334 行 → ≤300，纯重构不改行为。
- **范围/闸门/证据**：同 D1，证据 `docs/evidence/p260/medialibrary-split/`。
- **依赖**：无。**适合度：高**。可与 D1 同一助手串行做（避免 build/设备并发）。

## D3 · 播放页 REX 遗留交互（字幕菜单/锁定/缩放/长按倍速）

- **目标**：补齐播放页相对 REX 的遗留交互，交互动效对齐 REX（极致丝滑）。
- **范围**：`ui/compose/player/` 既有家族；先对照 `docs/reference/rex-teardown/`（`Rex_动效与交互全量拆解.md`、
  `Rex_动效参数表.csv`、`05-ui-spec-from-recording.md`）定家族树与参数，再实现。
- **闸门**：每项交互真机可用 + 动效参数对照 REX + 不破坏已验收播放基线（零丢帧/不叠播）。
- **证据**：`docs/evidence/p260/player-rex-leftovers/`。
- **依赖**：需 REX 参考与真机；**适合度：中**（需动效判断，建议先出设计再实现）。

## D4 · 慢链降码/转码清晰度档（卡顿根因，**用户明确暂缓**）

- **现状**：真链 10Mbps remux 在 ~292KB/s 测试链上必然缓冲卡成 PPT（带宽饥饿，非解码缺陷）。
  加大缓冲救不了持续缺口；唯一解是降 delivered 码率（服务器转码 `MaxStreamingBitrate` 或选低码版本）。
- **前置验证**：该测试服 `System/Info` 未 advertise 转码、影片仅单一 10Mbps 版本 ⇒ 先实测封顶码率转码请求是否被接受。
- **范围**：Emby 起播加"链路测速→自动/手动降码转码"清晰度档；依赖服务器开转码。
- **状态**：**DEFERRED（用户：之后再来想办法）**。领取前须用户重新拍板 + 完成前置验证。
- **适合度：中**（需服务器侧验证 + 起播链路改动）。

## D5 · 预览远_seek grab 窗与慢链矛盾（#40 披露 D2 转卡）

- **现状**：慢链远位置预览 grab 在 6s 等待窗超时（`docs/evidence/p259/.../thumb/p2-emby-farseek-timeout.json`），
  走非空 reason 不静默；近_seek 正常。属既有独立限制，非冷启动预算问题。
- **目标**：让慢链远_seek 预览也能出帧（如提高 grab 窗/异步出帧后刷新浮窗/降预览分辨率），不阻塞主播放。
- **闸门**：慢链远_seek 出帧或明确非空 reason；不引入主播放卡顿；≤300 行。
- **证据**：`docs/evidence/p260/preview-farseek/`。
- **依赖**：复用 #40 受控注入源（`docs/evidence/p259/.../logs/p4-range-server.js`）。**适合度：中**。

## D6 · 清除长期代码里的参考产品名痕迹（rex / senplayer）　✅ 已收口 2026-11-13

> **✅ 已收口**（前助手执行，codex 接管复核+提交推送）。54 文件纯注释/字符串+等价改名、零逻辑改动；
> 闸① 18 白名单/0 产品名、闸② codex 在线重编译 BUILD SUCCESSFUL（compose 2.1.21 从真实仓库补齐、无代理）、
> 闸③ 真机 4 页静态对照、闸④ 净 0 行。证据 `docs/evidence/p260/code-derex/`（含 §九收口补记）；详见 `TASKS.md` 顶部 D6 条。
> 下方为原始派发口径（保留存档）。

- **背景**：本项目 UI 复刻过一款参考播放器，但用户明确要求**长期留下的代码**里不得出现参考产品名
  （rex / Rex / REX / SenPlayer）或"逐帧复刻 REX"这类出处署名——这不是开源项目，没有保留出处的义务。
  此前助手把参考产品名写进了大量注释/字符串/标识符，需彻底清除。
- **范围（只清长期代码）**：`backend/`（.cxx/.hpp）、`include/`、`android/app/src/main/`（.kt/.cpp）、
  `tools/*.py`（活跃脚本）。**不清理** `docs/`、`artifacts/`、`docs/reference/rex-teardown/`、`tools/reference/rex/`
  ——这些是文档/参考素材/冻结归档，用户明确**之后要删除**，不必清理。
- **清什么**：① 注释/字符串里的 rex/Rex/REX/SenPlayer 及绑定参考录屏的帧号/动效编号出处
  （REX 0160、REX-EMBY 0178/0179、M03-S01、sk_16、IMG_0161、0172 等）——去掉出处，保留技术事实（尺寸/线宽/时长数值）。
  ② 标识符：`rexTextWeight` 已由主助手改名 `barTextWeight`（PlayerBarText/PlayerTopBar，已完成，勿重复）。
  ③ rex 命名的代码文件：`tools/measure_rex_playback.py` → 重命名中性名（如 `measure_baseline_playback.py`），
  同步更新 `tools/measure_kivo_playback.py` 的 `import measure_rex_playback as rex` 及所有 `rex.` 调用、docstring。
- **保留**：`mpv`/`libmpv`/`FFmpeg`/`swresample`/`MediaCodec`/`TMDB`/`Emby`/`SMB`/`PingFang`/`SF Pro`/`Noto`/`AirPlay`/`DLNA`
  等开源内核/技术/字体/协议名（事实依赖，非竞品）。**勿动**含 `Execution`/`ShaderExecution` 的词（子串误报，与参考产品无关）。
- **改写要求**：自然通顺、保留工程含义，不留空括号/残句；只改注释/字符串/上述标识符与文件名，**不改任何代码逻辑**；单文件 ≤300 行。
  例："REX 0160 实测 ink 21×18.3pt"→"实测 ink 21×18.3pt"；"SenPlayer / Rex 用的那颗内核"→"mpv 内核"；
  "RexDesign Assets.car 硬参数"→"浅色态语义色板硬参数"；"与 REX 一致"→去掉出处从句。
- **已完成（主助手，勿重复）**：`ui/design/settings/` 5 文件已清（含 About 页用户可见 REX 文案已删）；
  `rexTextWeight`→`barTextWeight` 已改。另有少量 player/compose/detail/emby 文件可能被部分清理过——以 grep 现状为准补全。
- **闸门**：① 长期代码 grep `rex|senplayer`（忽略大小写）零残留（Execution 误报与保留的参考目录除外）；
  ② `assembleDebug` 编译过；③ 真机回归播放/设置/详情不回归（纯文字清理，行为应零变化）；④ 不新增超闸文件。
- **证据**：`docs/evidence/p260/code-derex/`（清理前后 grep 计数 + 编译日志 + 回归帧 + 复算命令）。
- **依赖**：无。**适合度：高**（机械清理，边界清晰；需细心改写避免残句、避免误改 Execution/mpv）。

---

## 主助手自留（不派发）

- **设置页 UI 设计 + 实现**（REX 1:1）：**✅ 261 已收口**（K40 `b6e950ee` 真机，S1–S6 全 PASS）。
  规格 `docs/design/ui/replica/settings-page.md`；证据 `docs/evidence/p261/settings-page/README.md`。
  新家族 `ui/design/settings/` 8 文件（最大 126 行）；退役 `Pages.kt` 骨架；MainActivity 净 0（357 行既有 #37 债未增）。
  唯一真实开关复用 `HomePreviewSetting`（改 accentGreen 绿轨）；播放/字幕只读策略行（不放假开关）；
  缓存清理真实（磁盘图缓存 + 进程内 `TmdbImageCache.clearMemory` + 评分 json，du 119→0 文件复算）。
  **动效披露**：push/pop slide+fade 350ms 为候选值，screencap 延迟>过渡未拿到真 mid 帧，丝滑度运行时未测。
  > 262 播放 / 263 音画 / 264 操控 / 265 AI 字幕 / 266 数据与资源 子页均已收口（2026-11-13，K40 真机，见 TASKS.md 顶部）；
  > 264 = `SettingsControlsSub.kt` 132 行 + PlaybackPrefs 14 操控键，7 项消费复算 + 设备恢复默认，
  > 证据 `docs/evidence/p264/controls-settings/README.md`。
  > 265 = `SettingsAiSub/AsrModels/SysLangs.kt`（81/48/43 行），审计 0 后端 ⇒ 全置灰带原因；
  > 真实读回 3 项（目标语言联动 zh→en→zh / 模型目录实测 0/0 B / 用量空态），无新增持久化键，
  > 证据 `docs/evidence/p265/ai-subtitles/README.md`。
  > 266 = `SettingsDataRoot/DataSources/DataTmdb.kt`（82/142/43 行）+ 重写 `SettingsDataSub.kt`（49 行，ROOT+8 来源屏+内层 BackHandler）；
  > 每源聚合/搜索开关真写回 accounts.bin + 重读复算 + 退出再进持久化（c06–c11），计数为真、TMDB Key/代理/地址只读真值，
  > 片段标记来源行首版不可钻入已修可钻入（p266b），证据 `docs/evidence/p266/data-resources/README.md`。
  > 267 = `AppearancePrefs.kt`(73) + `SettingsAppearanceSub/Feedback/Nav/Badge.kt`（82/71/39/23 行）+ `SettingsRows.kt` PlainCard；
  > 6 真接线（autoplay 共享键同步 / 声音挂载门 / 剧照 / 内容提示 / 触感三态门 / 隐藏标题），触感三态运行时日志+同手势锁屏帧，
  > 修 manifest 缺 VIBRATE 静默降级真 bug；证据 `docs/evidence/p267/appearance-interaction/README.md`。其余设置子页等用户逐块发图。
  > **D6 已收口**（2026-11-13，codex 接管复核+提交推送，见 TASKS.md / `docs/evidence/p260/code-derex/`）；D1–D5 仍待其他助手领取（D4 用户暂缓）。