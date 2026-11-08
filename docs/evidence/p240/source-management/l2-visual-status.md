# L2 视觉基础统一 · 有界记录

2026-11-09，Redmi K40 / b6e950ee / 1080×2400。按用户反馈将视觉工作从L5提前到L2；[设计与文件边界](../../../design/ui/library-rebuild/visual-network-followup.md)。

本次已将独立绿色Material主题替换为由DesignTokens派生的中性主题；背景继续由宿主AnimatedGradientBg提供。标题28sp，页边距13dp，卡片圆角13dp。三个本地/文件入口合并为一组，服务器卡保留名称、地址、账号、管理和进入动作。绿色只留在EMBY协议标签。

`assembleDebug assembleDebugAndroidTest -Pkivo.testRunner=com.kivo.cinema.tmdb.TmdbConnectivityInstrumentation` 构建通过。手机、平板均 `adb install -r` Success；未卸载、未清数据。手机重启应用后两条源保留，打开空白新增表单后返回，两源仍保留；原微信输入法已恢复。

- [手机总览](l2-visual-overview-phone.png)、[界面树](l2-visual-overview-phone.xml)：两条源、管理入口、尾箭头可见，地址和账号已遮盖。
- [手机空白表单](l2-visual-form-phone.png)、[界面树](l2-visual-form-phone.xml)：继承中性主题，提交和取消在无键盘首屏内可见。

这只是视觉基础统一，不是最终UI设计或像素验收。表单仍以标准控件为主，媒体浏览页未改；平板仅安装成功，未产生本轮视觉验收图。管理弹层、键盘、大字号、连续动效，以及前轮粘贴/重复/取消竞态待测项均未据此转绿。L2保持DOING；下一步先继续本卡的表单/卡片视觉设计及未测矩阵，再推进L3。
