# L2 连接取消与粘贴组件验证 · 2026-11-09

范围：Huawei MRR_W29 真机 CFE6R21B12000179，Android 12/API31，2560×1600，density400。L2仍DOING，不进入L3。本轮未发现新的生产行为缺陷；SourceManagerModel只增加internal Repository/认证注入点，公开Application构造仍绑定正式加密存储与EmbyClient。

## 结果与证据

- [模型测试](instrumentation.txt)：5组PASS。取消后迟到成功/失败不落盘、不覆盖新草稿；失败保留草稿；识别保留缺失/空密码/特殊字符区别；重复点击仅认证并保存一次，重复账号在认证前拒绝。
- [生产组件点击测试](paste-ui.txt)：5项PASS。预览隐藏密码且确认前不改草稿；显式填表不认证不落盘；多地址必须选择；冲突阻止填表，取消保留草稿；继续编辑保留、放弃修改清草稿且不保存。
- 稳定截图：[识别预览](preview.png)、[字段冲突](conflict.png)。已人工查看，文案清楚，冲突时提交禁用，密码未显示。
- 初次脚本未等待按钮启用，后续截图等待暴露对话框关闭同步缺口；失败原始输出保留在paste-ui-first-attempt.txt、paste-ui-second-attempt.txt、paste-ui-animation-attempt.txt。最终脚本等待按钮启用和前一个对话框关闭，未放宽功能断言。
- 最终运行后查询crash缓冲区（11-09 06:29起）为空；已重新启动正式MainActivity，未停留在隔离测试界面。

测试使用独立cache记录和临时Keystore别名，结束删除记录与别名；未修改正式服务器账号。UI在真实MainActivity中挂载生产SourcesScreen和隔离模型，以CompositionLocal提供合成剪贴板，不读取或改写系统剪贴板。截图只包含合成内容。

## 复现

在android目录，JAVA_HOME=C:/Android/jdk-17.0.13+11，JAVA_TOOL_OPTIONS=-Djdk.net.unixdomain.tmpdir=C:/Windows/Temp：

```powershell
./gradlew.bat '-Pkivo.testRunner=com.kivo.cinema.sources.SourceConnectionInstrumentation' assembleDebug assembleDebugAndroidTest --no-daemon
```

根目录依次覆盖安装生产APK与测试APK（adb install -r，禁止卸载），运行：

```powershell
& C:/Android/sdk/platform-tools/adb.exe -s CFE6R21B12000179 shell am instrument -w com.kivo.cinema.test/com.kivo.cinema.sources.SourceConnectionInstrumentation
```

UI组须先在android目录以-Pkivo.testRunner=com.kivo.cinema.sources.SourcePasteUiInstrumentation重新assembleDebugAndroidTest，再覆盖安装测试APK，运行对应SourcePasteUiInstrumentation。必须核对输出PASS/FAIL，不能只认adb退出码。两组构建均BUILD SUCCESSFUL。

SHA256：

- 生产APK：D355387AD562F86BAD4B176FD4C10FBCBD5314D93664043EEA551B85F759613F
- 模型组测试APK：141CD1570B2EEC8FC5CFBFAD653166B8A0CE35C144FB9E76F24841F71BD936F9
- 最终UI测试APK：10176D4B61D1754F378A75F586D328BFCA4EAF8441B2D9AA336D239A5597FA59

## 未过闸门

模型测试用CountDownLatch延迟认证完成，只证明协调层取消时序，不证明HTTP socket立即中断。UI组是生产组件点击测试，不覆盖完整宿主导航或系统剪贴板权限。未验真实隐藏源成功验证、后台内容清除、锁屏/多窗口、认证中重建、隐藏默认首页、手机矩阵、几十源、保存期间阻止退出。上述仍需独立真机验证，不将本轮结果外推为L2完成，也不修改后端/JNI/Surface/解码链。
