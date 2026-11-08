# R2 · 详情导航与加载反馈回归

2026-11-09；基线 `2d928bc1`；Redmi K40 `b6e950ee`，1080×2400、density 2.75。R2 保持 **DOING**。本次验证详情导航与加载反馈，不宣称整页像素验收完成。

已有内容刷新失败时，原来的中央错误文字盖在海报/片名上。现在使用底部浅色圆角提示，可重试或关闭，保留原内容与滚动位置；首次加载失败单独显示原因与重试入口。加载状态仍由 Activity 持有，展示拆到 `ui/detail/loading/DetailLoadFeedback.kt`。已有底部操作菜单沿用上一轮已安装版本，本次未重新设计。

## 最终安装包上的结果

| 场景 | 结果 | 原始证据（均在 android/） |
| --- | --- | --- |
| 简介展开→收起 | 全部可见文字/描述节点 bounds 与收起前完全一致 | `release-overview-collapsed/expanded/recollapsed.{png,xml}` |
| 电影相似作品连续点 4 次 | 《荒野丛林》→《地心记》仅新增一个 Activity；首载故障恢复后标题正确，返回父页节点与栈恢复 | `release-first-load-parent/failed/recovered/parent-return.{png,xml}` 及同名 `-stack.json` |
| 剧集相似作品连续点 4 次 | 《辛普森一家》→《干杯酒吧》仅新增一个 Activity，子页有集数元数据，返回节点与栈恢复 | `release-tv-parent/child/parent-return.{png,xml}` 及栈 |
| 已加载详情刷新失败 | 保留原全部可见节点，底部出现原因/重试/关闭，未盖住片名 | `release-injected-refresh-before/failed.{png,xml}` |
| 恢复配置后重试 | 25 秒观察窗内错误未再出现，全部节点坐标与刷新前一致；已有内容路径无加载完成指示，因此不作为请求计时证明 | `release-injected-refresh-retried.{png,xml}`、`operations.jsonl` |
| 关闭底部错误提示 | 关闭后所有原节点坐标恢复一致 | `release-dismiss-before/visible/after.{png,xml}` |

相似作品横滑后打开《圣斗士星矢：圣域传说》再返回，也已验证节点坐标完全相等，见 `movie-horizontal-before/return.{png,xml}`。该项采集在本轮首次安装包，最终包未再单独重复横滑；最终包代码变动仅清理无用 import。`movie-*`、`overview-*`、`final-*` 为前段探索记录，最终包优先读 `release-*`。

## 取证方法和失败记录

原始动作在 `operations.jsonl`，每次截图配 XML 与 Activity 栈。栈只计 `* Hist #...` 行，避免 dumpsys 其他段落重复计数。XML 只比较可见 text/content-desc/bounds/clickable，不包含系统状态栏像素；返回一致不等于海报加载完成或动画逐帧一致。

仅关 Wi-Fi 的两次尝试未得到错误提示；之后暂开飞行模式实测到了加载失败，但恢复后还失败，第二次重试及首载重试记录均保留。诊断发现应用走 loopback SOCKS，而 adb reverse 映射为空；主机沿原有 SOCKS 请求 TMDB `/configuration` 返回 HTTP 200，恢复 `tcp:10808` 转发后首载重试成功（`first-load-recovered-after-usb-route`）。归为取证环境恢复不完整，不算产品重试失败。

最终包移除 reverse 并断无线仍未出现重试入口，35 秒等待超时。已有连接可能存活，方法判为 **不确定**，没有伪造 failed/retried 截图。随后三次确定性故障注入临时写入不可用代理端点，每次 finally 原样恢复，并断言私有配置字节相等；无线与密钥不变，原配置内容不入库。此法验证请求失败 UI，不能冒称物理断网或所有网络错误均通过。

剧集加载时一次 UI dump 返回 137；当时可能存在并发 dump，根因未证实。另一个脚本等待了本页不存在的“播放”文字（本片无本地资源）。两项均是取证问题，改为串行抓取现有节点后通过，修正已写入操作日志。

## 构建、安装与复算

`build-final.log`：BUILD SUCCESSFUL；`install-final.log`：Success。使用仓库固定签名覆盖安装，未卸载、未清数据。安装包路径/哈希及生产源码哈希在 `manifest.json`；`source.patch` 保存基线上的本轮生产代码变更，包括新文件。补丁按 Git 文本行生成；精确字节复现须按 manifest 的逐文件 newline 恢复 LF/CRLF，复现已检查。APK 不入库。

接手自检首次只建两个目标，ctest 其余三个缺可执行文件，失败日志保留。补建五个目标后 `preflight-test-all.log` **5/5 passed**；本轮未改 backend/JNI/Surface，不宣称重新跑了全仓 287 项。

复算命令：`python tools/reference/rex/verify_detail_navigation.py`。读取已归档 XML、栈和动作记录，20 项断言；结果与临时副本的坐标/栈变异检查见 `verification.json`。它不连接设备、不修改证据，不代表色彩、动效或网络计时通过。

## 继续推进

下一步仍是 R2：本地资源匹配→播放→返回、预告失败/成功路径、缺图/长标题、1.3× 字体与辅助功能。R3 再处理首页数据更新/缓存/生命周期。预告默认关闭、用户选择后才展示的要求保留。全页像素、帧时性能、短暂启动黑屏尚未闭环。
