# 274 · 选源逻辑验收（instrumentation）

命令：

```bash
./gradlew :app:assembleDebug :app:packageDebugAndroidTest \
  -Pkivo.testRunner=com.kivo.cinema.emby.EmbySourcePickerInstrumentation --offline
adb install -r app/build/outputs/apk/debug/app-debug.apk
adb install -r app/build/outputs/apk/androidTest/debug/app-debug-androidTest.apk
adb shell am instrument -w com.kivo.cinema.test/com.kivo.cinema.emby.EmbySourcePickerInstrumentation
```

输出（K40 `b6e950ee`）：

```
INSTRUMENTATION_RESULT: pass=9
INSTRUMENTATION_RESULT: total=9
INSTRUMENTATION_CODE: -1        # = Activity.RESULT_OK
```

9 个断言：
1. 485 真实数据（4K 排在前面）→ 选 1080p
1b. playSize 跟着版本（4,316,184,874）
2. 全部超阈值 → 取最小
2b. 原因写明"全部超阈值"（不静默）
3. 单版本 4K → 仍选它
3b. 原因标注"只有一个版本"
4. 空数组 → 回退旧行为
5. null → 回退旧行为
6. 多个可播（4K/1080/720）→ 取 1080（码率最高的可播项）

> 顺带修好了 androidTest 模块：5 个 mpv 验收测试的 `session.start(surface) { }`
> 因 Kotlin「尾随 lambda 只能绑最后一个参数」而编译失败
> （`start()` 最后一个是 `loadGate`）⇒ 改为 `onReady = { }` 命名传参。
