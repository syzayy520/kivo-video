# 263 · 音画设置子页（内核 radio 卡 / 画面 / 音频 / 画质增强）真机证据

设备：Redmi K40 `b6e950ee`（真机，非模拟器）。APK：`:app:assembleDebug`（kivoDebug 签名，`adb install -r` 覆盖）。
参考：用户「音画」页 2 帧。设计：[`docs/design/ui/replica/av-settings.md`](../../../design/ui/replica/av-settings.md)（家族树先行）。

## 闸门与判定

| # | 闸门 | 判定 | 证据 |
| --- | --- | --- | --- |
| A1 | 固定居中页头 + 圆返回 + 内核 radio 卡（✓ 当前 + Beta） | PASS | s2 |
| A2 | 画面/音频/画质增强 分组 + 发丝分隔 + 脚注 | PASS | s2/s3 |
| A3 | 未接入能力置灰 + 非空原因（内核/方向/适配/HDR/音频引擎/SDR↔HDR） | PASS | s2/s3 |
| A4 | 音频首选语言弹层（白圆角 + 蓝✓ + 可滚）+ 写回 | PASS | s4/s5 + prefs `audio_language=zh` |
| A5 | 单独调整播放亮度两态持久化 | PASS | s6(off)/s7(on) + prefs `independent_brightness` false→true |
| A6 | 行数闸 ≤300（新文件） | PASS | logs/line-counts.txt（SettingsAvSub 102） |

## 复算

- prefs：`adb shell "run-as com.kivo.cinema cat shared_prefs/playback_prefs.xml"` ⇒ logs/prefs-after.xml
  （`audio_language=zh` / `independent_brightness=true`，另含 262 遗留键）。
- 音频语言：s4 弹层蓝✓=媒体默认 → 选中文 → s5 行值=中文。消费点 `MpvPlaybackSession` 起播设 `alang`。
- 亮度：s6 灰(off)→s7 绿(on)，prefs 两态复算。消费点 `PlayerGestureRouter.onVerticalDragStart` 门控。

## 披露（诚实清单）

- D1 内核 radio 卡三行均 disabled（引擎切换未产品化，PlaybackEngine.select 仅读 debug 属性），✓ 标当前 mpv，不放假交互。
- D2 起播方向/画面适配/HDR 映射/增强/音频引擎/SDR↔HDR 均置灰+原因（P12-E4 映射本体未实现；manifest 固定横屏；keepaspect/捏合无）。
- D3 独立亮度默认 true（保持现有手势亮度行为），与参考默认 OFF 不同，刻意适配；关闭时左半屏竖滑不调亮度。
- D4 alang/亮度门控为 mpv/手势路径代码接线，本卡真机验证设置侧写回；网络源 alang 自动选轨 exercising 待后续。
- D5 MpvPlaybackSession 445 行为既有超闸债（非本卡引入，同 #37 类），新文件均 ≤300。
- D6 截图均为设置页，无个人画面/密码态；prefs 仅布尔/字符串，无凭据。