# 264 操控设置卡 · 真机证据 README

设备：Redmi K40 `b6e950ee`（HyperOS/MIUI，竖屏 1080×2400 / 横屏 2400×1080）。
构建：`build-controls.log`（BUILD SUCCESSFUL）；安装：`install.log`。
设计门闸：`docs/design/ui/replica/controls-settings.md` §五.1–6。

## 一、方法

- **UI 对位**：竖屏截屏 c0–c5（设置根 → 操控子页顶/中/底）。
- **持久化**：`run-as com.kivo.cinema cat shared_prefs/playback_prefs.xml` →
  `prefs-after-edit.xml`（测试值）/ `prefs-restored.xml`（恢复默认）。
- **消费定量**：设备 `cat /proc/uptime` 取 Δreal（0.01s 分辨率）+ 左上调试角标
  （媒体时间戳 ms + 帧数）取 Δmedia；ratio = Δmedia / Δreal。
  片源：SMB XiaoMi `192.168.5.1` `/视频/kivo-test/mpv-test-3min.mkv`（24fps、180s），mpv 内核。
- **播放器区点击**一律单条 adb shell 链式：显栏 tap + sleep 0.4–0.5 + 目标 tap
  （栏 3s 自动隐藏，PlayerSessionBridge.kt:108/133/168/185）。
- MIUI 过滤应用 logcat ⇒ `logcat-local-playback.log`（3001 行）无应用日志；
  内核范围证据 = 屏上占位文案 + 源码阅读（见披露 1）。

## 二、闸门对位

| 闸门 | 证据 |
| --- | --- |
| §五.1 视觉 4 组 20 行（14 真 + 6 灰） | c1–c4：节头/卡/行形态/脚注逐行对位 |
| §五.2 无假状态 | c2–c4 六灰行均带非空原因；c13 本地倍速面板诚实占位 |
| §五.3 接线复算 | 下表 7 项消费证明 |
| §五.4 行数 | SettingsControlsSub.kt 132 行；PlaybackPrefs.kt 265 行（均 ≤300） |
| §五.5 真机不回归 | c19 mpv 起播正常；各 ratio 轮播放连续、帧数单调 |
| §五.6 证据目录 | 本目录 |

### §五.3 消费复算（7 项）

| # | prefs | 证据 | 复算 | 结论 |
| --- | --- | --- | --- | --- |
| 1 | defaultSpeed=1.5 | c21 a/b | (24.917−8.458)/10.99 = **1.4976** | 起播即 1.5x |
| 2 | seekForwardSeconds=30 | c22 a/b | (163.625−132.208) − 1.5×1.24 = **29.56 ≈ 30**，伴 ▶▶ 闪现 | 步长消费 |
| 3 | longPressBaseSpeed=2.0 | c23 / c32 | 按住期间中央 HUD「2x」 | 起步倍速消费 |
| 4 | longPressRestore=previous | c33 a/b | (32.292−21.833)/7.02 = **1.490 ≈ 1.5**；帧 251/7.02 ≈ 35.8fps ≈ 24×1.5 交叉 | 松手回长按前速度 |
| 5 | showNextButton false→true | c12/c19 vs c29 | false 时底栏仅暂停；true 时暂停 + ▶| | 按钮显隐消费 |
| 6 | doubleTapSides=none | c30 a/b | Δmedia 12.542−10.708 = 1.834 ≈ 1.5×1.19 = 1.785，**无 +30 跳变** | 两侧双击门生效 |
| 7 | verticalSwipe=false | c31 | 竖滑后屏上**无音量/亮度 HUD** | 竖滑门生效 |

面板选择侧证：c20 a/b ratio **2.0005**（操作者在已开面板上选中 2.0x，见披露 2）。

## 三、持久化与恢复默认

- 测试值（`prefs-after-edit.xml`）：seek_forward=30、sides=none、vertical_swipe=false、
  default_speed=1.5、show_next_button=true。
- 恢复序列 c37–c43：c37 顶（30秒）→ c38 picker 选 15秒 → c39 中 → c40 sides picker
  选「快进 / 快退」→ c41 speed picker（背后可见 sides=快进/快退、竖滑绿）选 1.0x →
  c42/c43 终态屏显。
- 终态 XML（`prefs-restored.xml`，权威）：`seek_forward_seconds=15`、`double_tap_sides=seek`、
  `vertical_swipe=true`、`default_speed=1.0`、`show_next_button=true`（出厂默认）；
  262/263 既有五项（audio_language=zh、independent_brightness、ignore_ass_style、
  subtitle_language=zh、show_remaining）原样保留。

## 四、披露

1. **内核范围**：本地文件恒走 legacy `PlaybackSession`（PlayerSessionStart.kt:38-39），
   mpv 仅 SMB/URL 路径。倍速/长按消费在 SMB mpv 路径验证；本地倍速面板显示
   「内核尚未就绪 / 起播后即可切换」（c13）——既有内核范围，非 264 回归。
2. **c20 的 2.0005** 是操作者在已开倍速面板上同坐标二次 tap 选中 2.0x 所致
   （面板锚 trailing 16 / top 43 覆盖该点），证明面板选择消费；defaultSpeed 由 c21 全新重启隔离。
3. **c24 作废**：3 分钟片源到 EOS（179.958s ≈ 180s）帧冻结，ratio 无效；以 c33 新轮重测替代。
4. **c19 观察**：倍速文字绿但面板未入帧（截屏时机/锚位），如实记录，不作回归结论。
5. **c34 shelter.mkv 起播**为操作者导航失误：X 退出回到 SMB 浏览页（起播来源），
   后续 tap 落在 shelter.mkv 行；非 app 缺陷。c35/c36 为状态核查帧。
6. **logcat 无应用日志**（MIUI 过滤），内核证据以占位文案 + 源码替代。
7. **下一集钮**显隐为真，动作仍为 stub toast（剧集未接入，设计 §四 已披露）。

## 五、文件索引

- 构建/安装：build-controls.log、install.log、logcat-local-playback.log
- UI：c0–c5、c25–c28、c37–c43
- 播放消费：c11–c24、c29–c33（c24 作废，见披露 3）
- 持久化：prefs-after-edit.xml、prefs-restored.xml