# 统一源管理 · L1证据

2026-11-09，Huawei MRR-W29 / API31 / CFE6R21B12000179。按用户“先用平板”指示验证；K40离线，不替代K40后端性能矩阵。

## 接手自检

SDK路径存在，JAVA_HOME显式使用C:/Android/jdk-17.0.13+11，固定仓库签名覆盖安装成功，未卸载或清数据。设备原有files/evidence可读。桌面五个P12c target先clean-first重建（98步），CTest 5/5通过，原始输出见[selfcheck-desktop-tests.log](selfcheck-desktop-tests.log)。本地a212mp4.mp4实际出画见[selfcheck-playing.png](selfcheck-playing.png)，独立报告见[selfcheck-decode.json](selfcheck-decode.json)：completed=true，300帧呈现，0丢帧。本次仅环境冒烟，不宣称10bit保真或音频验收。

## L1实测

[构建](l1-build.txt)及[真机Instrumentation报告](l1-instrumentation.txt)。十组真实Android Keystore/AtomicFile测试，全部使用隔离cache目录和唯一临时密钥别名；退出删除测试数据与测试密钥。覆盖地址/密码语义、URL userinfo、歧义、随机密文、重建读取、重复账号、修订冲突、并发写、写失败保留、认证损坏、未知schema、缺密钥不重建。账号均为合成数据，未读取或更改用户的真实源。

复现：显式JAVA_HOME与JAVA_TOOL_OPTIONS=-Djdk.net.unixdomain.tmpdir=C:/Android/no-unix-sockets；在android/运行 `./gradlew.bat '-Pkivo.testRunner=com.kivo.cinema.sources.SourceStorageInstrumentation' assembleDebug assembleDebugAndroidTest`；两包均adb install -r，然后 `adb -s CFE6R21B12000179 shell am instrument -w com.kivo.cinema.test/com.kivo.cinema.sources.SourceStorageInstrumentation`。

范围：L1基础层已验证，未接入总览UI；不代表跨进程真实登录、多服务器切换或SMB闭环。此前分页WIP已随当前工作区APK安装，仍未真机验收分页。没有修改后端/JNI/Surface代码。
