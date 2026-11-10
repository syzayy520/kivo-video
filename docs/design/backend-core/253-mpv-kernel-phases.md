# 253 实施阶段：六个可独立验收的阶段

状态：**设计稿**
职责：**只回答一件事**：每个阶段交付什么、验收判据是什么、以及这一阶段的产物落在家族树的哪个位置。
家族：253 换内核到 libmpv（总纲见 [253-mpv-kernel-plan.md](253-mpv-kernel-plan.md)，家族树见 [253-mpv-kernel-family-tree.md](253-mpv-kernel-family-tree.md)）
前置证据：[p252/senplayer-arch](../../evidence/p252/senplayer-arch/status.md)

---

## 六、实施计划（6 阶段，每阶段有验收判据）

### P1 内核打通（先证明 libmpv 能在我们的 App 里活）

**做什么**
1. 用 `mpv-android` 的构建脚本交叉编译 libmpv（arm64-v8a，`--enable-shared`）。
2. 写 `KivoMpvBridge`（JNI）：`create / setOption / command / 事件回调 / 渲染到 Surface`。
3. 只播**本地文件**，视频走 `vd=mediacodec-copy`，音频走 `ao=android`。

**验收**
- 本地 1080p H.264 起播 ≤ 2 s，画面上屏。
- 拖进度条可 seek，误差 ≤ 1 帧。
- 声音正常（不含我们自研链路的任何环节）。

**风险**：交叉编译在 Windows 上的坑（mpv-android 的脚本面向 Linux/macOS）。
**退路**：改用 meson cross-file 直接驱动 NDK（meson 1.12 已就位）。

### P2 可播版本（解燃眉）

**做什么**
1. 走 §五 方案 C：本地 range 代理把 SMB 暴露给 mpv。
2. 接入现有播放页：`kivo.playurl` → `smb://` 或 `http://127.0.0.1`。
3. 保留自研内核的入口（feature flag 二选一）。

**验收**
- Shelter（12.19 GB / HEVC 10bit / 2×TrueHD）能起播、能拖、能出声。
- 恐怖废墟（VC-1 / TrueHD）能起播（mpv 全格式支持，无需退软解那套）。
- 人声正常（mpv 的 swresample 下混，与我们已验证的矩阵等价）。

### P3 正式版（把协议层换成我们的）

**做什么**：§五 方案 B —— mpv 注册自定义协议，走我们的 libsmb2。
同时把读画像接进证据 JSON。

**验收**：与方案 C 同上的全部判据，且 SMB 读画像可见、命中率 ≥ 90%。

### P4 断网恢复与体验对齐

**做什么**：把 `NetworkLinkMonitor` 的"局域网可达"判据接到 mpv 侧；
mpv 报错/EOF 时自动重连并回到原位置。

**验收**：断网 12 s → ≤ 4 s 恢复播放且位置不丢。

### P5 品质超越（§四 的 9 条逐条落地）

落位可见、字幕平面接管、弹幕、证据链扩展。

### P6 收尾

- 自研内核标记为"备用"，写清切换开关与回退步骤。
- 归档 §一~§五 的复算脚本。

---
