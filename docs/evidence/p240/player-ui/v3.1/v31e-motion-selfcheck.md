# v3.1e · 播放页交互动效 REX 对照自检（丝滑度总调校）

日期：2026-11-12　设备：Redmi K40 `b6e950ee`（真机，横屏 2400×1080）
内核：libmpv（253）　片源：SMB `XiaoMi/视频/kivo-test/shelter.mkv`
参考：`REX/rex/rex/Rex_动效参数表.csv` + `Rex_动效与交互全量拆解.md`
动效唯一来源：`android/.../ui/compose/player/PlayerMotion.kt`

> 等级口径：✅ = 时长/缩放/阈值与 REX 数值一致；◐ = 结构一致但缓动或个别参数为推断/自定；
> ❌ = 未实现（明示，不静默）。REX 的 ease/spring 全部为 ◐ 推断（FairPlay 包无动画源码），
> 本工程统一用 Compose `FastOutSlowInEasing` 近似 easeOut/spring 族，**只对时长与缩放负责**。

## 一、逐项对照

| 交互 | REX 行 | REX 参数 | 本工程实现 | 等级 |
| --- | --- | --- | --- | --- |
| 控制条显隐 | 无直接行 | —（v3 自定） | `PlayerMotion.barEnterMs=180 / barExitMs=140`，fade+位移 ±1/6；G1 仅 α | ◐ 自定 |
| 浮层面板弹入 | M06-S01 | 0.2s，菜单 0.94→1 | `panelEnterMs=200` + scaleIn 0.94；出 `panelExitMs=150` | ✅ 时长+缩放 / ◐ 缓动 |
| 面板钻入换内容 | M06-S02 | 交叉 0.2s | `panelSwapEnterMs=150 / ExitMs=120` crossfade | ◐ |
| 轻提示 Toast | M08-03 | 0.2 入 / 0.25 出 | `toastEnterMs=200 / toastExitMs=250`；停留由调用方计时 | ✅ 时长 / ◐ 无 Δy8→0 |
| 轨道选中变粗 | 无直接行 | —（v3.1 自定） | `trackMs=150`，6→10pt | ◐ 自定 |
| 拖动预览浮窗 | M03-S06 | α 0→1 / 1→0，d=0.15s | `previewEnterMs=150 / previewExitMs=150` + scale 0.92 | ✅ 时长 / ◐ 缩放自定 |
| 亮度/音量 HUD | M03-S03 | 0.15 入 / 0.3 出 / 停 1.0s | `hudEnterMs=150 / hudExitMs=300 / hudHoldMs=1000` | ✅（本轮修复，见二-D1） |
| 长按快进标签 | M03-S02 | 0.2 入 / 0.15 出，0.9→1，上滑 −60pt，2 触觉 | `PlayerLongPressHud` tween200/150 + scale0.9；`LOCK_UP_THRESHOLD_DP=60`；LONG_PRESS+VIRTUAL_KEY | ✅ |
| 双击闪示图标 | M03-S01 | 图标 0.5s，识别窗 0.3s | `PlayerGestureFlash` show 500ms + tween150 进出（总≈0.65s） | ◐ 总时长略长 |
| 进度条平滑 | 进度类 d=0.2 linear | 仅宽度动画 | `PlayerSeekTrack` `withFrameNanos` 按帧插值 ⇒ 60fps | ✅ 自定增强 |
| Reduce Motion | M08-01 | 仅 α，0 或 0.1s | bars/panel 降为 `reduceMotionMs=150` 仅 α；HUD/flash 本就纯 α | ✅ / ◐ 0.15 vs 0.1 |
| 双指缩放/旋转 | M03-S04 | 旋转 0.3，pinch 1–2 | 未实现（无对应功能） | ❌ 明示不做 |
| 换集交叉溶解 | M03-S05 | 0.3–0.5s | 未实现（无选集资源库） | ❌ 明示不做 |

## 二、本轮修掉的三个丝滑度缺陷

- **D1 · HUD 硬闪进硬闪出。** 旧 `PlayerGestureHud` 用 `animateFloatAsState(targetValue=1f)`：
  常量 target 首帧即等于目标 ⇒ **不产生任何动画**；宿主又用裸 `if (hudKind!=null)` 挂载，
  退出时整棵子树被直接摘掉 ⇒ 进/出都是硬切，与 M03-S03 的 0.15/0.3 淡入淡出完全不符。
  现改为组件自带 `AnimatedVisibility`（与 `PlayerLongPressHud` 同构），
  `fadeIn(tween(hudEnterMs))` / `fadeOut(tween(hudExitMs))`，宿主常挂、以 `visible` 驱动。
- **D2 · 停留计时器不可撤销。** 旧 `onVerticalDragEnd` 每次 `new Handler().postDelayed(…,1000)`
  且从不撤销：连续两次竖滑时，第一次的计时器会在第二次还在滑时把 HUD 提前收掉。
  现复用 `decorView` 队列 + 单令牌 `hudHideToken`，开始/结束都先 `removeCallbacks`；
  时长改读 `PlayerMotion.hudHoldMs`（不再散落魔法数 1000）。
- **D3 · 预览浮窗时长偏离 REX。** 旧 120/100ms；REX M03-S06 明确 d=0.15s 双向 ⇒ 对齐 150/150。

## 三、真机取证（渐隐不是硬切）

文件（本目录）：
- `hud-v31e-pill-full.png`：竖滑后 HUD 全 alpha 渲染（太阳图标 + 百分比药丸）。
- `hud-v31e-hold-and-gone.png`：三行拼图。A=松手+0.5s **全 alpha 停留**；B=+1.3s、C=+1.6s **已消失**
  ⇒ 停留≈1s 后收掉，与 `hudHoldMs=1000` 一致。
- `hud-v31e-midfade.png`：松手+≈1.1s 抓到**半透明中间帧**——glyph 变灰、药丸底近透明，
  明显介于"全亮"与"消失"之间 ⇒ **渐隐**（若硬切则只会有全亮/消失两态）。
- `hud-v31e-luma-raw.txt`：一次录屏的 ROI 逐帧 YAVG 原始值，**仅留作参考，不作判据**（见下）。

复算路径：
```bash
adb -s b6e950ee shell input swipe 600 800 600 300 400   # 左半屏竖滑=亮度
adb -s b6e950ee exec-out screencap -p > hold.png        # +0.5s
# +1.1s 再抓一张 ⇒ 半透明中间帧；+1.6s 再抓 ⇒ 消失
ffmpeg -i mid.png -vf "crop=600:180:900:220" mid-crop.png
```

### 测量限制（诚实声明，不伪造曲线）
1. **本机 HyperOS 的 `screenrecord` 不录 Compose 覆盖层**（只录视频层）：录屏里找不到 HUD，
   而同时刻 `screencap` 有 ⇒ 不能用录屏测 fade。已用三次录屏 + 对照 screencap 证实。
2. `screencap` 连拍仅 ≈5–10fps，分辨不了 150/300ms 的完整 ramp；40 帧连拍只抓到 1 帧全 alpha。
3. 跨帧 YAVG 受**移动视频背景**污染（药丸底是 70% 黑、透 30% 视频），不同帧背景不同 ⇒
   跨帧亮度不可比。**故不以亮度曲线标称 fade 毫秒数**；fade 时长以代码 tween 为准，
   真机只证明"存在半透明中间态 + 停留≈1s 后消失"这两件可观测事实。

## 四、结论

丝滑度缺陷 D1/D2/D3 已修；其余交互与 REX 时长/缩放/阈值对齐（✅），缓动为统一近似（◐），
未实现项明示（❌）。HUD 渐隐与停留已在真机以半透明中间帧 + 时序截图证实，非硬切。