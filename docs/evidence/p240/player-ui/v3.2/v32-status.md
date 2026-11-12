# v3.2 收口证据 · 拖动跟随 / 白度 / 图标 / 质感 / 按压与弹性（2026-11-12）

> 设备：Redmi K40 `b6e950ee`（真机，横屏 2400×1080）。构建：`assembleDebug --offline`，
> 签名 kivoDebug，`adb install -r` Success。分支 `codex/kivo-video-p0-ui-01-playback-overlay-v13`。
> 片源：SMB `视频/kivo-test/shelter.mkv`（1:47:15，35–44s 有弹幕）、SMB `kivo-test/local-seek.mkv`
> （60s SMPTE 彩条+烧录秒计数）、真本地 `media/mpv-test-3min.mkv`（3min 彩条+时码）。
> 对照基准：`../reference/`（REX 1.0.2 截图）与 `../v3/`、`../v3.1/`。

## 一、拖动预览帧跟随（#12）

**结论：跟随成立。** 连续滑动中预览窗画面逐段变化、窗位随手指移动、松手落点精确。

| 文件 | 观察 |
| --- | --- |
| `drag-smb-follow-a/b/c-*.png` | 同一次拖动三个时刻：预览画面三帧互不相同，弹窗 x 位随手指右移 |
| `drag-smb-land-1h36m04.png` / `drag-smb-land-35m27.png` | 松手落点 1:36:04 / 35:27，与拖停位置一致 |
| `drag-smb-follow-c-danmaku-hidden.png` / `drag-smb-danmaku-hidden-b.png` | 主画面处于 35–44s 弹幕窗口内但**无弹幕**：scrub 期间弹幕层隐去成立 |
| `drag-local-follow-a/b.png` | 真本地源同法复验：两帧不同（烧录秒计数可读差异） |

**机制（两处，均已真机验证）**
1. `thumbnail/ThumbnailReader.kt:221,245-248,287`：120ms 帧追逐预算——预算耗尽即交付
   「已解出的 ≤target 最近帧」，不再死等关键帧对齐；
2. `ui/compose/PlayerComposeRoot.kt:98-105`：120ms 媒体时间 + 40ms 墙钟双节流
   （快拖时单靠媒体节流会到 66Hz seek，墙钟底封顶 25Hz）。

**延迟实测**（`thumb-local-latency.json`，100 次请求）：
cold 76ms；warm **p50 13 / p95 29 / max 46ms**；decode_failures=1（如实记录）。

**SMB 口径（不藏）**：SMB 走 `MpvThumbnailReader`（第二 mpv 实例网络抓帧），
`thumb-smb-shelter-latency.json`：cold 229ms、warm p50 30 / **p95 283 / max 409ms**。
`drag-smb-popup-lag-a/b.png` 里弹窗帧落后手指约 0.3–0.6s 墙钟——**这是网络往返，不是队列积压**
（latest-wins 只折叠排队项，在飞抓帧必须跑完）。60s 短片源上尤其显眼（烧录秒计数放大观感）。
本轮不做 SMB 抓帧优化；用户可见行为＝预览帧稍滞后但始终跟随、落点精确。

## 二、白度 / 投屏与 PiP 图标 / 进度条质感（#13 / #14 / #16）

`visual-convergence.png` 为收敛后整屏：与 `../reference/` 同机位对照——
- 文字：CJK 走 `FontWeight.Light`（`PlayerBarText.kt` `rexTextWeight`），拉丁/数字 Normal；
  时间 13sp SemiBold Monospace。REX 实测横笔 0.7–1.0pt@16pt、字面填充 0.21，
  本机 Noto Regular 同字号 ≈0.30 偏粗发白 ⇒ Light 收敛（v3.1 已测，本轮目视复核）。
- 投屏＝通用 cast 符（去 AirPlay）、PiP＝双矩形+**↘ 箭头**（大屏转小屏语义），
  与 REX 拆解包 glyph 逐笔对齐（`PlayerGlyph*.kt`，v3.1d 起）。
- 进度条：已播 `0xFFFFFFFF`、缓冲 `0x8CFFFFFF`+描边 `0xBFFFFFFF`、
  未播 `0x1FFFFFFF`+描边 `0x59FFFFFF`（`PlayerTokens.kt`）——半透明白+提亮描边，
  不再是实心深色条；拖动加粗 6→10pt 保留。

## 三、按压反馈 + 面板弹性（#15）

**按压变暗（真机像素实证）**：按住无长按动作的「下一集」键（`PlayerBottomBar.kt:196`
`onLongClick=null`）期间截 `press-dim-held.png`，松手后 `press-dim-released.png`。
`press-metrics.txt`（ink 掩膜 G>100&B>100，背景红列 G≈0）：

| 状态 | next 键 ink mean_G | pause 键（对照）mean_G |
| --- | --- | --- |
| 按住 | 149.8 | 254.1 |
| 松手 | 235.5 | 254.1 |

背景扣除 alpha = (149.8−0.1)/(235.5−0.1) = **0.636**，目标 `PlayerTokens.pressAlpha=0.6`
（偏差来自抗锯齿边缘像素计入掩膜）；对照键恒定 ⇒ 变暗只发生在被按的本体，无涟漪。
实现：`PlayerTopBar.kt:174` / `PlayerBottomBar.kt:242` / `PlayerBarText.kt`
`onPress={pressed=true; tryAwaitRelease(); pressed=false}` + glyph `.alpha(0.6)`。

**面板弹性**：`PlayerMotion.kt:31-32` `spring(dampingRatio=0.8f, stiffness=632f)`
（stiffness=(2π/0.25)² ↔ REX §三「0.2s spring(0.25,0.8)」），收起 150ms EaseOut；
`PlayerOverlay.kt` 面板 enter=fade+scale(0.94→1) 同 spring。
**口径**：screencap 延迟 0.2–1s，静帧抓不住 0.2s 弹簧过程 ⇒ 弹性以代码参数对齐 REX 规格 +
`panel-danmaku-settled.png`（落定帧）+ `bars-fade-mid.png`（控制条渐隐中间帧，整屏 chrome
半透明）作证；逐帧动效对照待录屏补（screenrecord 不录 Compose 层，需外部摄录）。

**取证污染事故（记下来别再踩）**：前两轮「按住」静帧（p1/q1）里面板已开——不是注入合成 tap，
而是**长按动作**：长按播放键=锁屏、长按圈弹=弹幕面板（`PlayerTopBar.kt:124`），
hold 超过 ~0.5s 阈值就触发。隔离按压必须选 `onLongClick=null` 的键。

## 四、锁屏三态（顺带实证，§1.5）

| 文件 | 状态 |
| --- | --- |
| `lock-locked-toast.png` | 长按播放键加锁：chrome 全收 + toast「已锁定屏幕，点按左下挂锁解除」 |
| `lock-hint-summoned.png` | 锁定态单击＝仅唤出挂锁（2.5s 自隐，`PlayerActionRouter.kt:194`） |
| `lock-unlocked.png` |  hint 窗口内点挂锁＝解锁，控制条恢复 |

`lock-node-bounds.xml`：uiautomator 实测解锁节点 bounds `[105,896][226,1017]`
（中心 ≈165,956）。**目视截图估的 glyph 中心与命中盒中心差约 30px**——
点按取证一律以 uiautomator bounds 为准，别信眼睛。

## 五、测量协议更正（adb / HyperOS）

1. **swipe 必须在控制条可见窗内起手**：隐藏态起手不命中轨道（此前多轮「拖动失败」的真因）。
   起播自带 ≈3s 可见窗；之后中心点按唤出（中心是开关，可见时点＝隐藏）。
2. 同点 `input swipe x y x y N` 被 HyperOS 当立即 tap；真 hold 用 `input motionevent DOWN/UP`。
3. hold >~0.5s 触发 onLongClick（见 §三污染事故）。
4. screencap 延迟 0.2–1s：静帧只能证「状态」，不能证「过程」；过程靠参数+中间帧+落定帧。
5. **EOS 冻帧陷阱**：3min 片源播完后连截三张字节级相同（u0/u1/u2 第一轮），
   误以为按压无效；取证前先确认时码在走。
6. 命中区以 uiautomator dump 为准（§四）。

## 六、复算命令

```bash
export PATH="$PATH:/c/Android/sdk/platform-tools"; export MSYS_NO_PATHCONV=1
adb shell am start -n com.kivo.cinema/.ui.compose.PlayerComposeActivity \
  --es kivo.media /sdcard/Android/data/com.kivo.cinema/files/media/mpv-test-3min.mkv \
  --ei kivo.thumbDumpSec 20     # 延迟证据落 files/evidence/v31-thumb.json
# 按压：中心唤出 → motionevent DOWN 345 960 → screencap → UP → screencap
# 锁定：DOWN 200 960 hold>0.6s → 单击唤挂锁 → hint 2.5s 内点 (165,956)
```

## 七、未达成 / 待补（不静默）

1. SMB 抓帧 p95 283ms 未优化（网络 bound，见 §一口径）。
2. 弹簧/按压的逐帧动效对照待外部摄录（screenrecord 不录 Compose 层）。
3. 锁屏入口＝长按播放键为 ◐ 推断（REX 未录到），待 iPhone 补证（设计稿 §六清单）。
4. 本地 100 次请求中 1 次 decode_failure（json 如实记录，未复现定位）。