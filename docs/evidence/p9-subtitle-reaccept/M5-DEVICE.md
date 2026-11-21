# SUB-M5 · 次字幕延迟（按片源记忆）· 真机取证（2026-11-21）

- 设备：Redmi K40 `b6e950ee`；APK 两次覆盖安装均 `Success`（`M5-00/01-install*.log`）。
- 设计：p9 家族树 **§6.8**；用户拍板口径 = **按片源记忆**（与主 `sub-delay` 一致，键 `SubtitleSourceKey`）。

## 1. 落点

| 文件 | 改动 |
| --- | --- |
| `playback/subtitle/SubtitleDelayMemory.kt` | 主键前缀 `d:`（不变）、**次键 `s:`**；新增 `loadSecondary` / `saveSecondary`，夹取与空键语义同主延迟 |
| `playback/subtitle/SubtitleStyleApplier.kt` | `applyAtStart(..., secondaryDelaySeconds)`；非 0 才写 `secondary-sub-delay`，失败并入同一 failures |
| `playback/MpvStartSequence.kt` | 读同片源键的两个延迟（一次 `fromIntent`）并传入 |
| `ui/compose/player/subtitle/SubtitlePanelRows.kt` | `display()` 增读 `secondary-sub-delay`（缺失即 notReady）+ 在「次字幕位置」后新增「次字幕延迟」行（0.1 s 步进） |
| `ui/compose/player/subtitle/SubtitleStyleActions.kt` | `secondary-sub-delay` 分支**只记忆**（不重复写内核） |
| `playback/subtitle/SubtitleSecondaryControl.kt` | 删除随之无调用点的 `setDelay`（不留空壳） |
| `ui/compose/player/subtitle/SubtitlePanelActions.kt` | 重置在 `subtitleStepLock` 内一并写 `secondary-sub-delay=0`，两项都成功才落盘，主/次记忆同清 |

## 2. 读数

| # | 场景 | 读数 | 判定 |
| --- | --- | --- | --- |
| 1 | 探针 `--es kivo.subWrite "secondary-sub-delay=0.30"` | prefs `s:file:/sdcard/Movies/Kivo/kivo-panel-mid.mp4 = 0.30`；trace `probe sub-write secondary-sub-delay=0.30 ok=true`；内核 `Set property: secondary-sub-delay="0.30"` | PASS |
| 2 | 重启进程、同片源复播 | 内核 `[0.013] Set property: secondary-sub-delay="0.30"`（恰好一次） | PASS（回放） |
| 3 | 换片源（`kivo-panel-dark`） | 内核**无** `secondary-sub-delay` 写入 | PASS（不串味） |
| 4 | UI 行 | `uiautomator dump`：`次字幕延迟` bounds `[1429,435][1652,496]`、值 `0.2 s`（与 prefs 0.20 一致） | PASS |
| 5 | 点 `+` 一步 | 行 `0.3 s`；内核**只一行** `secondary-sub-delay="0.3"`；prefs `0.30` | PASS |
| 6 | 点「重置」 | 内核 `sub-delay="0"` 与 `secondary-sub-delay="0"` **同时**写入；prefs `<map />` 清空 | PASS |

## 3. 顺带修正

- 首版 `persistNumeric` 在次延迟分支又调了一次 `SubtitleSecondaryControl.setDelay`，
  导致同值被写两次（实测内核 `0.1` + `0.10` 两行）。已改为**只记忆**，
  `setDelay` 因此无调用点，一并删除（不留空壳）。复验：步进后内核**只有一行**。

## 4. 边界（不夸大）

1. 只测**本地路径**片源键；URL / SMB 三元组键未单独取（三者共用 `SubtitleSourceKey` 同一实现）。
2. 未截"次字幕画面位置/时间轴"的视觉证据 —— 本项只改时间轴偏移，不改位置（位置由 `secondary-sub-pos` 既有验收覆盖）。
3. 「显示次字幕」开关与次字幕延迟的交互（关掉次字幕时改延迟）未单独取证。

## 5. 收尾

设备已重置回默认（prefs 清空）；未 commit / push。
