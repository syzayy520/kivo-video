# W3-R3 · 网络字幕（粘贴字幕链接）· 真机取证（2026-11-22）

团队批次 `subtitle-remote-url`（T1–T4 并行），编译绿，覆盖安装 `Success`。

## 1. 验收结果（5/5 PASS）

| # | 项 | 实测 |
| --- | --- | --- |
| 1 | **成功路径** | `sub-remote: fetching …` → `sub-import ok: …/7b9dadb5500a-remote.srt deduped=false size=55` → `sub-remote ok` |
| 2 | **字幕真的显示** | 截图 `URL-11-lan-ok.png`：画面底部「**网络字幕 已导入**」（`analyzeImage` 逐字确认） |
| 3 | **导入库落盘** | `files/subtitles/7b9dadb5500a-remote.srt`（55 B，命名 `<hash12>-remote.srt`） |
| 4 | **HTML 拒收** | 指向目录列表 → `链接不是字幕文件`（`sub-remote failed`） |
| 5 | **UI 行 + 空剪贴板** | 字幕卡「导入字幕」**下面**紧邻「粘贴字幕链接」；点它且剪贴板无链接 → `sub-remote: no url in clipboard` |

## 2. 三个把验收卡住的环境坑（都不是产品 bug，但要记）

1. **PC 上开着代理**（`HTTP_PROXY=http://127.0.0.1:10808`）：
   用 `curl http://192.168.5.142:8900/...` 会**走代理并返回 503**，一度让我误判"服务没起/防火墙拦"。
   ⇒ 测本机服务必须 `curl --noproxy '*'`。
2. **子 shell / `Start-Process` 起的测试服务活不过几十秒**（`netstat` 里端口直接消失）：
   这就是最初 `IOException: 下载失败：超时`（connect 5s）的真实原因 —— 服务早没了。
   ⇒ 必须用**工具级后台**（`background=true`）常驻，并 `netstat` 确认 `0.0.0.0:8900 LISTENING`。
3. **`adb reverse` 与 Java 的 OkHttp 不兼容**：设备走 `127.0.0.1:8899/8900` 时，
   异常发生在 **`connection.responseCode`**（响应头都没收全）：`unexpected end of stream on com.android.okhttp.Address@…`；
   而 native 的 ffmpeg（App 播同一 HTTP 视频）走 reverse **完全正常**。
   ⇒ 真机验收必须走**局域网 IP**（临时开 TCP 入站规则，测完已删除）。

## 3. 顺带补的诊断（保留）

`downloadBytes` 的读体异常现在会带**已读字节数与异常类名**：
`下载失败：读到 N 字节后中断（IOException）`。这条把"连接层没通"和"读体中截断"区分开，
上面第 3 个坑就是靠它定位的。

## 4. 边界（不夸大）

1. **剪贴板路径未在真机端到端跑通**：MIUI 没有 `cmd clipboard`，adb 无法设剪贴板值。
   验收用同一条执行入口 `importSubtitleUrl(url)`（产品剪贴板路径与取证探针共用它），
   **空剪贴板分支**则用真实空状态验证（`no url in clipboard`）。
2. **https 未测**（夹具只有 http；契约里 https 走同一分支）。
3. 大文件上限（8 MiB）与超时未做"真实超限"注入，只做了代码路径核对。
4. 测试夹具服务在 PC 临时目录（`%TEMP%\webfix`），**已停**；设备上保留 `FONT-test.*` 字形夹具。

## 5. 收尾

临时防火墙规则 `kivo-test-8900-tmp` 已删除（`show rule` 报"没有匹配规则"）；后台服务已 kill；
设备临时截图已清；未 commit / push。
