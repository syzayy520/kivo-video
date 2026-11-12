# 交接 · 播放页 UI v3.1 收口（2026-11-12）

> 接手先读 `agents.md` → 本文件 → `docs/evidence/p240/player-ui/v3.1/v31f-status.md`
> → `docs/evidence/p240/player-ui/v3.2/v32-status.md`（最新一轮）。
> 分支：`codex/kivo-video-p0-ui-01-playback-overlay-v13`。工作目录＝仓库根 `C:\kivo video`。

## 一、现在在哪

播放页 UI 已按 REX 1.0.2 复刻（v3 像素基线）+ 功能调校（v3.1a–f）**全部收口**：

| 项 | 状态 | 证据（`docs/evidence/p240/player-ui/v3.1/`） |
| --- | --- | --- |
| 像素基线（顶栏 pitch50 / 轨道 6pt / 面板锚点） | ✅ | `docs/design/ui/replica/playback/player-ui-v3-rex-replication.md` + `artifacts/rex-*` |
| 投屏去 AirPlay 留 DLNA | ✅ | `topbar-v31d-icons.png`（首枚通用 cast 符） |
| 进度条选中变粗 6→10pt + 拖动实时画面帧 | ✅ | `09…14-drag-*`、`preview-v31d-*`、`thumb-v31d-frame40.png` |
| 第二 mpv 预览实例（网络/无索引源） | ✅ | `v31-thumb-v31d-scan.json`、`mpv-thumb-v31c-round1.log` |
| 画中画（含 MIUI 静默拒绝看门狗） | ✅ | `pip-v31d-enter-firstframe.png` / `pip-v31d-live.png` |
| 弹幕（设置真接渲染、无文件置灰+原因） | ✅ | `02…08-danmaku-*.png` |
| 丝滑度（PlayerMotion 唯一来源、HUD 渐隐） | ✅/◐ | `v31e-motion-selfcheck.md` + `hud-v31e-*` |
| BACK 层级（面板→sheet→控制条→退出） | ✅ | v3.1 设计 §九 |
| 清晰度分档统一（长边唯一规则） | ✅ | `origin-v31f-seed-panel.png` / `origin-v31f-realpath-panel.png` |
| 源卡显示来源名（REX 0169 语义） | ✅ | 同上 + `origin-v31f-sourcelist.png` |
| 进程内唯一播放页（修双音频叠播） | ✅ | `single-player-log.txt` / `single-player-stack.txt` |
| 字幕（libass 渲染/轨道/导入/设置真接内核） | ✅ | `sub-v31f-*.png`（v31f-status §五） |

设计稿：`docs/design/ui/replica/playback/player-ui-v3.1-features.md`（含每轮真机定论与换核决策）。
退役件：`player/retired/PlayerSubtitleLayer.kt`（Compose 假字幕层）。

## 二、未完成 / 诚实边界（不许静默，均已用户可见或文档明示）

1. **Emby 真服务器往返未测**：K40 源列表"媒体服务器 0 个"。`kivo.sourceName` 显式名分支
   （`EmbyActivity.kt:84`）仅代码核对。**用户添加 Emby 源后**补一轮源卡取证。
2. **ASS 统一样式观感对照未截帧**：内嵌轨是 PGS 位图轨，样式只作用文本字幕。
   外挂 ASS 已补证**特效渲染链路**（`sub-v31f-ass-imported.png` / `sub-v31f-ass-effects.png` /
   `sub-v31f-ass-clean.png`，fixture `kivo-fx-test.ass` 同目录）；
   `sub-ass-override=yes` 对 ASS 的"剥特效"视觉对照仍待补。
3. 字幕搜索、弹幕过滤关键词：无后端 ⇒ 置灰 + 非空 reason（设计如此，非缺陷）。
4. 双指缩放/旋转、换集交叉溶解：❌ 明示不做（无对应功能/资源库）。
5. 平板适配：用户 2026-11-09 明确"晚点再做"。
6. **字幕选择不跨会话持久化**：重开播放页读回 内嵌=关闭/外挂=无
   （`sub-v31f-nopersist.png`），代码无 sid 恢复逻辑，既有行为非本轮引入。

## 三、环境与命令（会咬人的坑）

```bash
# 构建（离线、无配置缓存）
cd "C:/kivo video/android" && JAVA_HOME="C:/Android/jdk-17.0.13+11" ANDROID_HOME="C:/Android/sdk" \
  ./gradlew :app:assembleDebug --offline --no-configuration-cache
# git 需：git -c safe.directory="C:/kivo video" …（账户属主不一致）
# adb：export PATH="$PATH:/c/Android/sdk/platform-tools"; export MSYS_NO_PATHCONV=1
```

- **安装**：HyperOS 9s 自动拒绝 ⇒ 后台 `adb install -r`，同时 `input tap 347 2111` 两次。
  **禁止 `adb uninstall`**（删证据与片源，agents.md §0.0）。
- **坐标**：横屏 screencap 2400×1080、竖屏 1080×2400，显示图均 ×1.2 换算。
- **控制条 ≈3s 自动隐藏**；起播会刷新 lastTouchMs ⇒ 取证用"中心唤出 + 目标点按"同一命令链。
  中心点是**开关**：控制条可见时点中心＝隐藏。
  **隐藏态起的第一下 tap 只唤出控制条，不 seek、不进面板**——前几轮"进度条死钮"
  误判即由此来（v31f-status §六.5）。面板不随自动隐藏收，但钻入/选轨不刷新
  lastTouchMs、选轨成功后面板自动关闭；此刻 BACK 直落退出层级。
- **screenrecord 不录 Compose 覆盖层** ⇒ UI 取证一律 `screencap`。
- `adb am start` 带 NEW_TASK ⇒ BACK 落点与用户手动导航不同；取证尽量走真实 UI 路径。
- 清晰度徽章横屏 (1932,960)；字幕图标 (2172,152)；中心 (1200,540)。

## 四、架构红线（本轮新增）

- 分档唯一规则 `player/PlayerResolution.kt`；来源推导唯一 `player/PlayerSourceOrigin.kt`
  （`from` 主线程 / `resolve` 仅 IO）。**不要再写第二份分档或来源逻辑。**
- 动效常量唯一 `player/PlayerMotion.kt`。
- `PlayerComposeActivity` companion `livePlayer` WeakReference：新页 finish 旧页。
  **不要加 onStop 暂停**（214 后台播放 + PiP 依赖会话存活）。
- 单文件 ≤300 行；改前看家族树（v3.1 设计 §七/§十）。

## 五、下一卡候选

1. 用户加 Emby 源 ⇒ 源卡显式名分支真机取证。
2. ASS `sub-ass-override=yes` 剥特效观感对照（渲染链路已证，见 `sub-v31f-ass-*`）。
3. 按文件记忆字幕选择（sid 不跨会话持久化，`sub-v31f-nopersist.png`）。
4. 平板适配（等用户时间）。
5. 后端指针仍以 `docs/handoff/TASKS.md` 为准（本卡只动播放页 UI）。
6. SMB 预览抓帧 p95 283ms 优化（网络 bound，v3.2 §一已明示口径）。
7. 弹簧/按压逐帧动效外部摄录对照（screenrecord 不录 Compose 层）。

## 六、v3.2 轮收口（2026-11-12 晚，#12–#16）

证据：`docs/evidence/p240/player-ui/v3.2/`（23 件 + `v32-status.md`）。

| 项 | 状态 | 关键证据 |
| --- | --- | --- |
| 拖动预览帧跟随（含弹幕 scrub 隐去、落点精确） | ✅ | `drag-smb-follow-*` / `drag-*-land-*` / `drag-local-follow-*` |
| 本地预览延迟 | ✅ warm p50 13ms | `thumb-local-latency.json`（120ms 帧预算 + 25Hz 墙钟节流） |
| SMB 预览滞后口径 | ◐ 明示不藏 | `thumb-smb-shelter-latency.json` p95 283ms + `drag-smb-popup-lag-*` |
| 白度 / cast / PiP↘ / 轨道半透明+描边 | ✅ | `visual-convergence.png` 对照 `../reference/` |
| 按压本体变暗 α0.6 | ✅ 像素实证 0.636 | `press-dim-held/released.png` + `press-metrics.txt` |
| 面板 spring(0.8,632) / 收起 150ms easeOut | ✅ 参数对齐 | `PlayerMotion.kt:31-32` + `panel-danmaku-settled.png` / `bars-fade-mid.png` |
| 锁屏三态（顺带） | ✅ | `lock-locked-toast / lock-hint-summoned / lock-unlocked.png` |

**本轮新增的坑（详见 v32-status §五）**：swipe 必须在控制条可见窗内起手；
hold >0.5s 会触发长按动作（长按播放=锁屏、长按圈弹=面板）污染按压静帧；
EOS 后连截字节级相同；命中区以 uiautomator bounds 为准（目视估坐标差过 30px）。