# 225 · 下载管理 —— 状态稿（收口）

> 日期：2026-11-08 · 设备：Redmi K40（b6e950ee）· 服务端：Windows SMB 共享 `kivo` @ 192.168.5.142:445（kivosmb）
> 结论：**AC1–AC4 全部真机通过**，M2 第五卡收口。证据 `docs/evidence/p225/`，校验器 `tools/verify_p225_download.py`（6/6 变异自检）。

---

## 一、一句话结果

**SMB 浏览行点「下载」→ 前台服务经 libsmb2→本地 Range 代理把 58.2MB 视频落盘 `Movies/Kivo/` → 通知进度/完成 → 媒体库 157→158 可见；1GB 文件强杀进程后续传从断点接续（不归零）。**

## 二、与设计稿的差异（四条，都要知道）

| 设计稿 | 实际 | 为什么 |
| --- | --- | --- |
| D1 OkHttp + CoroutineWorker(WorkManager) | HttpURLConnection + CoroutineScope(IO) + 手动 Range | 单文件 v1 不需要队列/持久化调度；WorkManager 恢复链路复杂化，AC4 用"断点 + 用户重触发"达到同一判据 |
| AC1 落点 `getExternalFilesDir("downloads")/` | `Movies/Kivo/`（公共媒体集合） | **设计稿自相矛盾**：getExternalFilesDir 在 Android 10+ 不被 MediaStore 索引，AC3"出现在媒体库"根本查不到；落点让位给 AC3 |
| （未写下载 URL 来源） | SMB 必须先桥成本地 HTTP：`Smb2Native.Source → openForRead → LocalRangeProxy → http://127.0.0.1:port/kivo` | DownloadService 只说 HTTP，SMB 主机不听 HTTP——收口时发现的关键缺口，复用 220b 播放同款链路 |
| （未写） | 通知 ID 统一由 onStartCommand 算一次下传 | 原 `url.hashCode()` vs `title.hashCode()` 两处不一致会让进度更新另开一条通知，FGS 那条永远停在"准备下载…" |

落盘后 `MediaScannerConnection.scanFile` 入 MediaStore —— AC3 的可见性由此而来。

## 三、真机证据（AC 对照）

| # | 判据 | 实测 | 证据 |
| --- | --- | --- | --- |
| AC1 | 33MB 文件下载落盘 | `1080p30_60s.mp4` 58.2MB（**61,069,842 B**）落 `Movies/Kivo/`；拉回 PC ffprobe **60.000000s / h264 1920×1080 / aac 48000**，moov 完整无截断 | `p225-ac1-ffprobe.json`、`p225-summary.json`（含 sha256） |
| AC2 | 通知进度 | 下载中「1gb_720p.mp4 / 已下载 172MB」+ 进度条；完成后「1080p30_60s.mp4 / 下载完成 ✓」；`dumpsys` 证 `isForeground=true channel=download` | `p225-progress.png`、`p225-notification.png` |
| AC3 | 下载完成出现在媒体库 | 本地视频 **157→158**，下载片在海报墙首位（0:01:00 徽标） | `p225-library.png` |
| AC4 | 中断后续传 | 1GB 文件下到 **110,157,824 B** 强杀进程 → 重连重触发 → 首个观测点 **122,544,613 B > 断点**，全程单调不回落直到 **1,090,116,567 B**（两次 stat 间隔 4s 相同）+「下载完成 ✓」通知 | `p225-summary.json` ac4、`p225-resume-done.png` |

Range 续传的生效机制：重触发时下载器读 `localFile.length()` 作 `existingBytes`，带 `Range: bytes=N-` 发给本地代理（代理 206 支持），不重下前 N 字节。

## 四、本卡新踩的坑（下次直接用）

1. **MIUI 中文 IME 会吞自动化输入**：`adb shell input text` 的标点被全角化（`.`→`。`、`!`→`！`），字母进拼音组合框不上屏——之前 SMB 表单"填进去就乱"的根因就是它。**自动填表前必须 `ime set` 切 Gboard，填完切回**（`tools/p225_fill_form.py` 已内置流程）。
2. **密码框 dump 是圆点不是原文**（PasswordVisualTransformation）——断言按圆点数 == 密码长度判；曾见过 11 圆点 ≠ 9 位密码，是清空不彻底（DEL 次数不够），补 20×DEL 后归位。
3. **uiautomator dump 别走 `/dev/tty`**（shell 通道拿不到 xml），dump 到 `/sdcard/*.xml` 再 `cat`；且 Git Bash 忘带 `MSYS_NO_PATHCONV=1` 时设备路径会被改写成 Git 安装目录。
4. **这台 K40 现在 logcat 里应用日志全被抑制**（`logcat -b all | grep Kivo` = 0 条，此前 KivoSmbUi 是可见的）——本卡证据全部不依赖 logcat，用文件尺寸/dumpsys/截图/uiautomator 四类可复算状态。
5. `am start-foreground-service` 起不了未导出服务（`Requires permission not exported`）——UI 入口必须真走界面。

## 五、豁免与未做（明示，不藏）

- 批量下载 / 队列管理 / 速度限制 / Wi-Fi only / 自动删除：设计稿 §五已排除，未做。
- 1GB 文件**未**拉回 PC 做 ffprobe：58MB 参照物已证明链路字节完整；1GB 完整性由「下载器以服务器 Content-Length 为停止条件」+ 轮询单调 + 完成通知保证。
- 通知点击跳转媒体库（设计稿 D2 一句带过）：未做，v1 通知不带 contentIntent。
- 断点续传只验证了"进程死亡 + 手动重触发"路径；网络瞬断（TCP 断而不杀进程）路径未单测。
