# 339 · 从服返回资源库不再闪「连接未完成」（2026-11-16 21:00，K40 b6e950ee）

## 用户诉求
「从服返回资源库的时候会出现连接未完成，这个你看看是不是需要修」。需要修。

## 根因（代码实证）
三条退出路径（系统返回键 / 顶栏返回 / gate 页按钮）都执行 `model.back(); finish()`。
`back()` 同时把 `inLibrary=false`、`connecting=false` 且不清 `error`；
Activity 的 `finish()` 尚未生效前 Compose 重组一帧，落到：
```kotlin
else { Text(model.error ?: "连接未完成", ...); Button(onClick = 重试) }
```
⇒ 肉眼看到红字「连接未完成」（+ 重试 + 返回资源库按钮）闪一下才退回。
不是误报：这条重组路径在代码里确定存在，只是只有 1～2 帧，普通抽样容易漏。

**附带发现**：`BackHandler(enabled = model.inLibrary || model.connecting)` 在读源/连接早期
（`checked=false`）是关闭的 ⇒ 那段按返回键**完全无反应**（实测 sleep 1s 内点击无效）。

## 修复（EmbyActivity，171 行）
1. 退出只 `finish()`：新增 `fun exit()`，三条退出路径 + error 分支按钮统一走它。
2. 清理移到真正销毁时：`onDestroy() { if (isFinishing && ::model.isInitialized) model.back() }`；
   配置变更（`isFinishing=false`）不动会话。这样退出瞬间 UI state 完全不变，不可能重组到错误分支。
3. `BackHandler` 去掉 `enabled` 条件，始终可用 —— 连接早期也能返回。
4. model 提为类字段（供 onDestroy 清理）；无 UI 视觉改动、无新增文件。

## 真机证据（设备端同步录屏，消除跨命令时间漂移）
录屏命令在设备端单条执行：`screenrecord & sleep 2; input tap 返回; sleep 4; wait`。

`exit_closeup.png`（逐 0.05 秒 ×16 帧）：
- 第 1–12 格：稳定停在服首页（谷雨海报 / 继续观看）；
- 第 13 格：直接进入 Activity 切换动画（旧页半透明叠加资源库）；
- 第 14–16 格：资源库列表页。
**全程 0 帧出现「连接未完成」红字 / 重试按钮 / 白屏 / 转圈。**

`exit_grid.png` 为 8 秒每 0.25 秒整体宫格，`exit_record.mp4` 为原始录屏。
`early_exit_works.png`：进服后 2 秒（连接阶段）按返回，2 秒后顶层已是 `MainActivity`、页面为资源库列表
—— 修复前这一场景按键无效。

## 回归
- 338 原地切服未受影响：A→iLoveHD 内容正确（565KB B 服帧），切回 A 服 1 秒恢复（2.13MB A 服帧）。
- `BUILD SUCCESSFUL in 42s`（`:app:assembleDebug` 全量）；EmbyActivity 171 行 ≤300。
- 旧退出代码 `model.back(); finish()` / `model.cancel(); finish()` / `BackHandler(enabled=…)` 全无残留。

## 诚实边界
- 旧版闪帧太短（<1 帧 @0.25s 抽样），本轮未保留"修复前抓到红字"的截图；依据是确定的代码路径
  + 修复后 0.05s 逐帧无错误态。不宣称有修复前红字截图。
- 仅 K40（A13 / kona）验证；MatePad 未跑。
