# 操控设置子页（Controls）· 家族树设计（实现前置门闸）

> 来源：用户 2026-11-13 提供的「操控」页 2 帧截图（05:36 顶部 / 05:37 滚动）。
> 延续 261→262→263 设置家族节奏。本稿先定家族树与逐行真实能力，**未完成本设计不得实现**。
> 措辞遵守 D6：不写参考产品名，统一称「参考」。

## 一、页面结构（4 组，20 行 = 14 真 + 6 置灰）

| 组 | 行 | 参考形态 |
| --- | --- | --- |
| （无节头卡） | 跟随设备旋转 / 显示转屏按钮 | Switch |
| 播放器按钮 | 播放器按钮(›) / App 内画中画 / 下一集 按钮 / 快进与快退按钮 / 截图 按钮 / 快进时长 / 快退时长 | Chevron / Switch×4 / Stepper×2 |
| 手势操作 | 双击中间触发 / 双击两侧触发 / 全屏上下滑动调整音量和亮度 / 双指缩放画面 / 锁定时允许长按和双击操作 | Stepper×2 / Switch×3 |
| 倍速与快进 | 默认播放速度 / 长按快进 / 默认长按倍速 / 快进时左右滑动调整速度 / 快进时向上滑动锁定 / 长按松手时恢复至(+脚注) | Stepper×3 / Switch×3 |

## 二、家族树（新增 1 文件 + 既有家族接线，不新建包、不平铺）

| 文件 | 职责 | 动作 |
| --- | --- | --- |
| `ui/design/settings/SettingsControlsSub.kt` | 操控子页：只组合行原语 + 读写 PlaybackPrefs + picker | **新增**（≤300 行） |
| `ui/design/settings/SettingsRows.kt` | 行原语；ChevronRow 增 `enabled`（置灰钻入用） | 改（小升级） |
| `ui/design/settings/SettingsNav.kt` | 路由枚举 +`CONTROLS` | 改 |
| `ui/design/settings/SettingsPage.kt` | 根页 +「操控」入口 + sub 映射 | 改 |
| `playback/PlaybackPrefs.kt` | 播放策略唯一真值源；+14 个操控键 | 改（≤300 行） |
| `ui/compose/PlayerActionRouter.kt` | onDoubleTap 消费动作/步长 prefs | 改 |
| `ui/compose/PlayerGestureRouter.kt` | 竖滑/长按/调速/上滑锁定/松手恢复 消费 prefs | 改 |
| `ui/compose/PlayerComposeActivity.kt` | 退役 DOUBLE_TAP_SEEK_MS / LONG_PRESS_BASE_SPEED 常量（改读 prefs） | 改 |
| `ui/compose/PlayerSmbStart.kt` | startMpvPlayback 就绪回调应用 defaultSpeed（mpv 出厂 1.0，非默认才下发） | 改 |
| `ui/compose/PlayerComposeRoot.kt` | 向 Overlay 传 showNextButton / showCaptureButton | 改 |
| `ui/compose/player/PlayerOverlay.kt` | 截图钮显隐门 + 透传 showNextButton 给 BarsSlot | 改 |
| `ui/compose/player/PlayerBarsHost.kt` | BarsSlot 透传 showNextButton 给 BottomBar | 改 |
| `ui/compose/player/PlayerBottomBar.kt` | ButtonRow 下一集钮显隐门 | 改 |

依赖方向不变：settings UI → PlaybackPrefs ← player 消费（单向）。

## 三、逐行真实能力审计（file:line 证据 → 真 / 置灰+原因）

### 置灰 + 非空原因（6 行，无后端，不放假开关）
| 行 | 证据 | 原因文案 |
| --- | --- | --- |
| 跟随设备旋转 | AndroidManifest PlayerComposeActivity 固定 landscape | 起播固定横屏，旋转跟随尚未接入 |
| 显示转屏按钮 | PlayerGlyph/TOP_ACTIONS 无 rotate | 转屏按钮尚未接入（固定横屏） |
| 播放器按钮(›) | TOP_ACTIONS/ButtonRow 硬编码，无按钮配置子页 | 按钮显隐自定义尚未接入 |
| App 内画中画 | PlayerPip 仅系统 PiP（autoPip），无应用内小窗 | 应用内小窗尚未接入；系统画中画见播放设置 |
| 快进与快退按钮 | ButtonRow/TopBar 无 ff/rewind 钮（仅双击手势） | 快进/快退按钮尚未接入，当前用双击两侧 |
| 双指缩放画面 | 无 pinch/zoom 代码（263 披露捏合无） | 捏合缩放尚未接入 |

### 真控件（14 行，现硬编码 → 新增 prefs + 播放器消费）
| 行 | 现状证据 | 新 prefs 键 | 消费点 |
| --- | --- | --- | --- |
| 下一集 按钮 | ButtonRow `BarIconButton("next")` 恒显；onNextEpisode 为 stub(toast) | showNextButton | PlayerBottomBar 显隐 |
| 截图 按钮 | PlayerOverlay 暂停态恒显 PlayerCaptureButton | showCaptureButton | PlayerOverlay 显隐 |
| 快进时长 | DOUBLE_TAP_SEEK_MS=10_000 硬编码 | seekForwardSeconds | onDoubleTap FORWARD |
| 快退时长 | 同上共用 | seekBackwardSeconds | onDoubleTap BACKWARD |
| 双击中间触发 | DoubleTapZone.TOGGLE_PLAY 硬编码 | doubleTapCenterAction(playpause/none) | onDoubleTap |
| 双击两侧触发 | BACKWARD/FORWARD 硬编码 | doubleTapSidesAction(seek/none) | onDoubleTap |
| 全屏上下滑动调整音量和亮度 | onVerticalDragStart 恒接管（亮度半受 independentBrightness 门） | verticalSwipeEnabled | onVerticalDragStart |
| 锁定时允许长按和双击操作 | 锁定态双击/长按恒保留 | lockAllowGestures | onDoubleTap/onLongPressStart 锁定门 |
| 默认播放速度 | 起播恒 1.0 | defaultSpeed | 起播 ready setSpeed |
| 长按快进 | onLongPressStart 恒生效 | longPressEnabled | onLongPressStart |
| 默认长按倍速 | LONG_PRESS_BASE_SPEED=2.0 硬编码 | longPressBaseSpeed | onLongPressStart |
| 快进时左右滑动调整速度 | onLongPressDrag 横向调速恒生效 | longPressSwipeSpeed | onLongPressDrag |
| 快进时向上滑动锁定 | onLongPressDrag 上滑锁定恒生效 | longPressSwipeLock | onLongPressDrag |
| 长按松手时恢复至 | onLongPressEnd 恒恢复原速 | longPressRestore(previous/keep) | onLongPressEnd |

（双击中/侧、松手恢复 用「双选项 picker」表达真实可配置：播放/暂停|无、快进/快退|无、长按前速度|保持快进速度。）

## 四、默认值对齐参考出厂态 + 披露

- seekForward/BackwardSeconds 默认 **15**（参考 15秒；现硬编码 10s ⇒ 行为对齐参考，披露变更）。
- defaultSpeed=1.0、longPressBaseSpeed=2.0、showNext/showCapture=true、verticalSwipe/lockAllow/longPress*/swipe*=true、longPressRestore=previous、doubleTapCenter=playpause、doubleTapSides=seek。
- 披露：下一集钮显隐为真，但其动作仍为 stub（剧集列表未接入，toast 明示）；App 内画中画置灰但系统 PiP 真实存在（播放设置）。

## 五、闸门

1. 视觉：4 组 22 行与参考逐行对位（节头/卡/行形态/脚注）。
2. 无假状态：6 置灰行均带非空原因；14 真行读写真实 prefs 且播放器消费。
3. 接线复算：改 prefs 后真机验证消费（seek 步长/长按倍速/默认倍速/按钮显隐/手势门）。
4. 行数：新增 SettingsControlsSub ≤300；PlaybackPrefs ≤300；不新增超闸文件。
5. 真机：K40 b6e950ee 截图 + 持久化复算 + 播放不回归。
6. 证据：docs/evidence/p264/controls-settings/。