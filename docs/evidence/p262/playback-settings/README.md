# 262 · 播放设置子页（行原语升级 + 真实接线）真机证据

设备：Redmi K40 `b6e950ee`（真机，非模拟器）。APK：`:app:assembleDebug`（kivoDebug 签名，`adb install -r` 覆盖）。
参考：用户提供的设置页截图（`C:\Users\Administrator\Pictures\设置页`，IMG_0205–0251），本卡只复刻**播放**子页。

## 闸门与判定

| # | 闸门 | 判定 | 证据 |
| --- | --- | --- | --- |
| P1 | 固定居中页头 + 圆形返回 + 分组 inset 卡 + 发丝分隔 | PASS | s2/s3/s4 |
| P2 | 行原语：标题+多行灰副标题+右控件（绿/灰 Switch、值+⇕ Stepper、chevron） | PASS | s2–s5 |
| P3 | 未接入能力置灰+非空原因（不假开关） | PASS | s3/s4（服务器预处理/来源/片段跳过 灰+原因） |
| P4 | 字幕语言选择弹层（白圆角+蓝✓+可滚动）+ 写回 | PASS | s6/s7 + prefs `subtitle_language=zh` |
| P5 | 开关真实持久化（忽略特效字幕样式） | PASS | s8 + prefs `ignore_ass_style=true` |
| P6 | 显示剩余时长：设置→prefs→播放器底栏右侧消费 | PASS | s10(off)→s11(on)→s12(右 `-00:15`) + prefs `show_remaining=true` |
| P7 | 行数闸 ≤300 | PASS | logs/line-counts.txt（最大 290） |

## 复算

- prefs：`adb shell "run-as com.kivo.cinema cat shared_prefs/playback_prefs.xml"` ⇒ logs/prefs-after.xml
  （`ignore_ass_style=true` / `subtitle_language=zh` / `show_remaining=true`）。
- 剩余时长：s9 基线右=总长 `00:20`（showRemaining OFF）；s12 右=`-00:15`、左=`00:04`（ON，20−4≈15）。
  消费点 `player/PlayerBottomBar.kt` TimeText（`showRemaining && durationMs>0` ⇒ `-remaining`）。

## 披露（诚实清单）

- D1 mpv-only 旋钮（缓冲 demuxer-max-bytes / slang / seekExact / 续播 / 起播规格）已接线到内核属性/seek 并编译过，
  但本卡真机走本地硬解测试片（非 mpv），**网络源 exercising 待后续卡**；不宣称已真机验证。
- D2 拖动预览开关 `scrubPreview` 为预览浮窗可见性的 AND 门（`PlayerSeekTrack.kt`）+ 关闭时不向预览内核发 seek
  （`PlayerComposeRoot.kt`）；预览出帧/跟随已由 #2/#12 真机证明，本卡不重复抓拖动帧（adb 拖动时序不稳定）。
- D3 后台音频/自动恢复/自动 PiP 经 `PlayerLifecycle.kt`/`PlayerPip.kt` 接线，生命周期路径代码核验，未单独真机取证。
- D4 截图均为设置页与合成彩条测试片，无个人家庭画面、无密码态；prefs 仅布尔/字符串，无凭据。