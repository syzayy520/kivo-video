# 待派发任务清单（供其他助手领取）

> 用途：主助手专注 **设置页 UI 设计**（REX 1:1，见 `docs/design/ui/replica/settings-page.md`）。
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

---

## 主助手自留（不派发）

- **设置页 UI 设计 + 实现**（REX 1:1）：`docs/design/ui/replica/settings-page.md`。用户指定主助手负责。