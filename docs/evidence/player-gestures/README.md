# player-gestures 真机证据判读

设备：Redmi K40 `b6e950ee`（横屏 2400×1080，density 2.75）。模拟器数字无效。
坐标体系（屏幕 px）：rail 心 y≈812；轨道盒 `[259,747][2219,879]`（48dp 命中高）；
双击分区 left<800 / center 800–1600 / right>1600；手势中心 y=540。

## 复现命令链

```bash
A="C:/Android/sdk/platform-tools/adb.exe"   # 两台设备在线，必须 -s b6e950ee
export MSYS_NO_PATHCONV=1                  # 含 /sdcard 参数时
"$A" -s b6e950ee shell am force-stop com.kivo.cinema
"$A" -s b6e950ee shell am start -n com.kivo.cinema/.ui.compose.PlayerComposeActivity \
    --es kivo.media /sdcard/Android/data/com.kivo.cinema/files/media/mpv-test-3min.mkv \
    --ei kivo.seekMs 60000
sleep 6; "$A" -s b6e950ee shell input tap 1200 540; sleep 0.6   # 冷启动后栏已自动隐藏，单击唤出
"$A" -s b6e950ee shell "input swipe 1200 812 700 812 2500 & sleep 1.2; screencap -p /sdcard/x.png"
```

装机：`install -r` 放后台，`sleep 3` 后 `input tap 347 2111` 两次（HyperOS USB 安装确认，9 秒自动拒绝）。
禁止 `adb uninstall`（删私有证据与片源）；签名固定 `android/keystore/kivo-debug.jks`。

## 测量方法

- 卡内帧框＝纯色 `(24,35,33)±4` 的 bbox（PIL 隔 2px 扫描）；卡外框＝帧框各边 ±6dp(16.5px)。
  卡外框高 462px(168.125dp)、宽 522px(190dp)，见 `seekPreviewWidth/Height`。
- rail 行＝最长近白 run。**坑**：片源彩虹斜带含近白像素，会在 y812 造成假阳性；
  显隐结论必须目视截图复核（38-regress-r5 即假阳性例）。
- **uiautomator semantics bounds 对轨道盒内溢出子节点不可信**：32 号 dump 的「卡节点」
  坐标与同帧像素矛盾。落位 ground truth 一律用像素。
- **screencap 时延**：拖拽中途截图可能抓到更早相位；夹紧/停留类判据在**抬手后 dwell 窗内**截图
  （target 保持终值），如 34/35 用 `sleep 1.35`（swipe 1200ms）。
- MIUI `uiautomator dump` 的 `theme_config` FileNotFoundException 是无害噪声，dump 仍写出。

## 编号判读

- 01–24：继承 golden path（双击光痕/同侧重播、横滑、滑轨、竖滑 HUD、长按倍速、后台恢复、EOS 前向 toast）。
- 25–32：**落位排查（已证伪的四种机制）**。卡放在轨道盒（BoxWithConstraints）内时，
  AnimatedVisibility 居中、Popup 绝对坐标、同高包裹盒+align+offset、align+padding、
  自布局 `Modifier.layout` 五种落位全部被恒定加偏（约 ±60dp，随机制变号），
  与像素预测差恒定 ⇒ 偏移来自轨道盒祖先放置层，不是卡自身几何。
- 33–37：**修复后判据（卡挂 overlay 根 Box）**。
  - 33：帧框顶 274＝预测 274；卡中心 960.5 vs 拖点 960（中心跟手）；
    卡底 719.5＝轨道盒顶 747−27.5（底边贴轨道上沿 10dp）。
  - 34：卡左 259.5＝轨道左 259（左夹紧）。35：卡左 1697.5＝2219−522（右夹紧）。
  - 36/37：抬手 +250ms 卡在、+950ms 卡无（400ms 停留 + 180ms 淡出）。
- 38-regress-r1…r5：回归。r4 单击唤出 / r5 单击隐藏（目视）；r2 右双击光痕；r3 竖滑音量 HUD 44% 真实读回。

## 修复结构

卡从轨道盒内移到 [PlayerOverlay] 根 Box（HUD/toast 同款 align+padding，实测正确）；
轨道经 `onGloballyPositioned` 回传窗口坐标到 `seek/PlayerSeekPreviewAnchor.kt`，
`seekPreviewLeftTop()` 是落位几何唯一来源（中心跟滑块、两端夹在轨道内、底边贴轨道上沿 10dp）。
轨道盒内不再渲染卡；`PlayerSeekTrack` 只留命中/手势/轨道本体。

## 未验与边界（不声称完成）

- 本地 mpv `thumbnailReader == null` ⇒ 预览卡显示「此片源暂不支持预览帧」（reason 可见，不伪帧）；
  真实帧预览与 thumbScan 未验。
- EOS 后回 seek 静默 no-op（k2）：`MpvSeekControl.awaitLanding` 无 eof 守卫，根因未定。
- 片头边界 toast 未真机复验。
- 减弱动画/多指中断本轮未单独取证（继承前轮 JVM 时序测试覆盖）。