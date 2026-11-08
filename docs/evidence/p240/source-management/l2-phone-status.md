# L2 手机补验

2026-11-09。用户重新连接 Redmi K40 / b6e950ee，1080×2400。使用与平板相同的现有 debug APK，adb install -r 返回 Success；未卸载、未清数据。手机原有本地媒体记录保留，首次进入新源总览为0条源。

本轮实际执行：

1. 分别新增 Library-B（空密码）和 Library-A（有密码），两者真实认证成功，总览为2条。手机自适应为单列，第二条需滚动查看。
2. 进入 B，首批60条、总714，[界面树](l2-library-b-phone.xml)。
3. force-stop 后重开，进入资源库，A/B均保留，[脱敏截图](l2-two-sources-restarted-phone.png)、[脱敏界面树](l2-two-sources-restarted-phone.xml)。
4. 重启后进入 A，首批60条、总12157，[界面树](l2-library-a-phone-after-restart.xml)。首次采集仍在加载，后续采集才完成；不把异步等待误报成连接失败。
5. 编辑 B，仅名称追加 -Edited，重新认证并保存，显示 Library-B-Edited，[脱敏界面树](l2-renamed-phone.xml)。本轮没有在改名后单独重启，不能将该记录当作成功编辑跨进程恢复的独立证据。
6. 删除本轮刚添加的 B 测试记录，点名确认后只剩 A，[脱敏界面树](l2-removed-phone.xml)。未触碰平板两条记录，未删除服务器媒体。
7. 重新添加原名 Library-B，真实认证通过，手机最终恢复2条连接。

输入环境：原微信输入法会将 adb 英文输入转成候选词，第一次字段填充不正确，未提交错误值；临时切到设备已安装 Gboard 后逐项核对再登录。结束已恢复 com.tencent.wetype/.plugin.hld.WxHldService。没有更改字号、分辨率或系统键盘显示设置。

边界：手机连接/保存/重启/切源/成功编辑/真实删除已实测；软键盘完整遮挡矩阵、大字号、粘贴预览UI、重复添加UI、连接中取消竞态及动效连续录屏仍待验。L2继续DOING，当前先完成这些，不提前开始L3分类实现。数量为当轮服务端结果，不是固定产品值。
