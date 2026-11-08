# R2 播放入口阶段 · 2026-11-09

基线 `2400ac26`；Redmi K40 `b6e950ee`，1080×2400。R2 仍 DOING。本轮仅改 `MovieDetailActivity.kt` 主播放导航目标，未改后端、JNI、Surface、会话生命周期或海报视觉参数。

## 结果与证据

- 旧入口点击“播放”先到旧 `DetailActivity`，还需再次点击：`android/legacy-before.*` → `legacy-intermediate.*`。
- 最终入口直接到 `PlayerComposeActivity`。4 连点只新增一个播放器，原详情栈保留：`release-before.*` → `release-player.*` → `release-return.*`。
- 第二次启动后色条测试视频烧录帧号从128到268、时码从4.267秒到8.933秒：`release-player-second.png` / `release-player-later.png`，裁剪对照为 `android/playback-frame-comparison.png`。这是已有测试视频，不是片名对应的商业影片。只证明画面推进，不证明音画同步、帧时或性能。
- 两次播放器返回，保存的可见文本、描述、bounds、clickable 和 Activity 栈恢复。最终包无资源入口点击前后节点与栈相同：`release-no-resource-before/after.*`。原因文字存在，未抓到 Toast，不称 Toast 实测。
- 从首页本周趋势进入《再婚皇后》；`trailer-ready/idle.*` 节点与栈相同，预告为静态图片。点击后 `trailer-external.*` 显示外部浏览器的 `www.youtube.com`。静态截图不证明没有网络请求；外部视频成功播放未验。
- 外部浏览器返回不是稳定的一次 Back 路径。四连点那次稍后焦点已回详情，额外 Back 回到首页（`trailer-return.*`，不是成功返回对照）。单次点击重测后通过多次 Back 回 Kivo，`trailer-single-before.*` 与 `trailer-final-return.*` 的节点和栈相同。不能据此声称浏览器任务唯一或四连点只打开一个外部页。
- `build.log` 成功，`install.log` 为 Success；固定签名覆盖安装，未卸载/清数据，未改测试片源。

## 复算与限制

运行 `python tools/reference/rex/verify_detail_playback.py`，19 项保存证据断言通过，结果 `verification.json`。取证入口为 `tools/reference/rex/detail_playback_device.py`，复用已有导航工具；`operations.jsonl` 保存操作和失败记录。源码差异见 `source.patch`，源码/APK/证据哈希见 `manifest.json`。

日志抓取未获得有效 KivoCompose 输出：`playback.log` 仅空行、`playback-live.log` 空文件，不能作为启动证据。操作中日期格式被远端 shell 拆开，仅得到11-09，不是真正时间戳；以操作自身 epoch 和截图为准。显式启动非 exported 详情 Activity 被系统拒绝，未改 manifest，最终预告使用正常首页入口。一次中文参数等待失败后改用 Unicode 转义。外部浏览器出现无关界面的两组临时截图/XML已删除，不纳入交付。

后台播放服务是否停止、无泄漏、音频、无外部应用的异常、外部弱网失败提示、用户字号/辅助功能、全页像素与帧时性能仍未验。本轮未改变旧详情类其他入口。观察到深滚内容仍可能接近系统状态栏，列入下一阶段系统栏/字号布局回归，不在本轮盲调。
