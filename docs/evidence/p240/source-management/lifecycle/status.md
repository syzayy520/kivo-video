# 源授权生命周期 · 平板阶段验证

2026-11-09，Huawei MRR_W29 / API31 / CFE6R21B12000179。L2继续DOING。此轮补证据，生产代码未改。

## 已验证

新增真实宿主测试 SourceLifecycleInstrumentation，6项通过，见 [输出](instrumentation.txt)。MainActivity→无源ID的Emby空连接页→返回保留前台授权；真实Activity重建后保留；三轮回桌面均撤销授权，返回不自动解锁。测试在内存授予合成授权，不能据此声称系统凭据成功流程或隐藏媒体内容已通过。测试运行前检查正式存储无隐藏源，否则拒绝运行；不改变账号和偏好，结束锁定。

真实资源库顶部锁按钮调出系统“输入锁屏密码”页，连续两次系统返回后回到资源库，显示“已取消解锁”，图标仍为“解锁隐藏资源库”，未出现已解锁图标。未输入设备密码。另以不存在的合成源ID直达EmbyActivity，出现读取失败及返回入口，没有解锁按钮或连接表单；返回正常。脱敏断言及命令见 [UI检查](ui-checks.json)。本轮无新增崩溃，平板最终回资源库。

## 构建与复现

在android/，显式JAVA_HOME=C:/Android/jdk-17.0.13+11、JAVA_TOOL_OPTIONS=-Djdk.net.unixdomain.tmpdir=C:/Windows/Temp，运行 `./gradlew.bat '-Pkivo.testRunner=com.kivo.cinema.sources.SourceLifecycleInstrumentation' assembleDebug assembleDebugAndroidTest --no-daemon`，BUILD SUCCESSFUL，29s。测试APK固定签名覆盖安装Success，未卸载或清数据。生产APK与上一轮一致，无需再次安装。

- 生产APK SHA256：`efcc66159be058401780cc1b519f9bf76b53c3322a4f172aff554d600d2e257b`
- 测试APK SHA256：`82aa44f637cbc40f69a13322d676fd0add58f8795381a3985f5be981ad66bb43`

复现测试命令见instrumentation.txt；先按上述runner参数构建并 `adb install -r android/app/build/outputs/apk/androidTest/debug/app-debug-androidTest.apk`。测试会启动真实宿主、重建页面及桌面往返，要求设备解锁、没有隐藏源且没有默认首页自动跳转。测试不验证网络登录、页面视觉或播放器。

## 仍未过闸

实际隐藏源在系统验证成功后的显示/编辑/后台内容清除、隐藏源直达与隐藏默认首页、锁屏与多窗口、身份验证中配置重建、手机矩阵仍待验证。无源ID空连接页的内部跳转不等于隐藏源媒体播放链验收。连接取消竞态、完整粘贴UI、几十源及分屏亦未全测。不提前进入L3，不称L2完成。
