# 平板双栏与资料页闪退修复

2026-11-09，MRR_W29 / CFE6R21B12000179，真机2560×1600，fontScale=1.0。L2保持DOING。

## 闪退原因与修复

导航移入SourceWorkspace后，祖先navigationSource把导航自身纳入毛玻璃捕获，形成RenderNode递归绘制，系统RenderThread发生SIGSEGV。原始崩溃见navigation-render-crash.txt。

MainActivity在资源库页关闭祖先捕获，仅向SourcesScreen的overview传入navigationSource；导航与捕获内容成为兄弟节点，消除循环。修复包已assembleDebug成功并用adb install -r覆盖安装，未卸载或清数据。

## 真机结果

- navigation-regression.json：发现→资源库→管理连续5轮，同一PID6374。
- equal-management.png：左右等分、右页背景全高，底部导航仅属于左页。
- 补测添加表单输入合成名称LayoutTest后点击发现，出现“放弃未保存的修改？”及继续编辑/放弃修改。选择放弃，返回资源库并重新打开Library-A管理，PID仍6374；没有连接或保存测试账号。
- adb logcat -b crash -d -T '11-09 06:00:00.000' 返回空；旧崩溃保留在修前日志。此结论仅覆盖本次修复后的短时回归。

## 未验范围

分屏、键盘完整矩阵、长期稳定性及原L2隐私/生命周期闸门仍待验证。不能据本轮称整卡完成或像素级验收通过。K40未连接，平板窄窗不等于手机真机验收。

## 操作区与适配补测

SourceConnectionContent滚动区改为weight(1f)，操作区固定于宿主可用底边。最终包构建、摘要及逐项结果见layout-regression.json。平板横屏、键盘弹出/滚动至密码、竖屏窄窗、1.5倍字号窄窗及横竖切换草稿保留均已观察；继续编辑与放弃修改两条分支通过。仅输入合成草稿，没有连接或保存。最终停在final-management.png，左侧可见服名与统计，右侧管理页独立全高。

系统font_scale已恢复1.0，accelerometer_rotation=1、user_rotation=1恢复原值。最终包再次进入服务器并返回，无新崩溃。
