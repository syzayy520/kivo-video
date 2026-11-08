# L2 服务器偏好与隐私入口 · 阶段记录

2026-11-09，L2仍DOING。设计与家族树见 [source-controls](../../../design/ui/library-rebuild/source-controls.md)。沿用已归档平板接手自检；没有改后端/JNI/Surface。

新增隐藏/系统凭据解锁、唯一默认首页、首页聚合、聚合搜索独立设置。隐藏源即使解锁也不参与聚合。旧schema1记录默认兼容，编辑账号保留偏好，事务拒绝过期修订。默认首页只在冷启动跳转一次。管理菜单完整展开，避免平板半展开时下面操作不在首屏。

## 最终包与隔离测试

显式 JAVA_HOME=C:/Android/jdk-17.0.13+11 与 JAVA_TOOL_OPTIONS=-Djdk.net.unixdomain.tmpdir=C:/Android/no-unix-sockets，运行 android/gradlew.bat -p android -Pkivo.testRunner=com.kivo.cinema.sources.SourceControlsInstrumentation assembleDebug assembleDebugAndroidTest，BUILD SUCCESSFUL。两包均固定仓库签名 adb install -r 成功，未卸载/清数据。

Huawei MRR-W29 / CFE6R21B12000179，运行 am instrument -w com.kivo.cinema.test/com.kivo.cinema.sources.SourceControlsInstrumentation，七组PASS，见 [原始报告](l2-controls-instrumentation-tablet.txt)。测试使用隔离cache文件和临时Keystore别名，退出清理；覆盖旧记录、唯一默认/修订、聚合独立与重读、隐藏资格、编辑保留、撤销/过期写、无效隐私字段拒读。

## UI 与边界

[操作记录](l2-controls-ui-tablet.json)、[菜单脱敏裁切](l2-controls-menu-tablet.png)。真实两源中测试搜索关闭/重启保留/恢复、默认首页冷启动进入Emby/返回不循环/撤销。初版自动脚本在凭据取消断言失败，不能将其算作通过；实见测试源已隐藏，随后用户完成系统验证恢复显示，再取消隐藏并检查恢复原偏好。系统密码未被脚本获取或输入。裁切图仅保留动作与说明，无真实地址或账号。

未验：可靠的取消系统凭据验证路径、后台/锁屏重锁、隐藏源直达Intent及隐藏默认首页、配置重建、手机矩阵。隔离测试中直接调用内存授权不代表这些实际系统流程通过。K40离线，之前字号可能仍1.5，连接后须优先恢复1.0并确认。

当前没有跨服务器聚合首页/搜索，菜单明确说明接入后生效；本阶段只保存设置并提供统一资格函数，不能声称已过滤真实聚合搜索。搜索接线必须覆盖请求前筛源及历史/建议/缓存/延迟结果隔离，已列为 [L3硬闸门](../../../design/ui/library-rebuild/l3-content-scope.md)。这不等于TMDB发现页影片分级。
