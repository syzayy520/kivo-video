# 307 · WatchStore 原子写（观看进度不再可能被写坏）

状态：**已修复，4/4 PASS**
设备：K40 `b6e950ee`
时间：2026-11-16 04:08–04:20

## 一、问题

`stats/WatchStore.kt` 用 `file.writeText(...)` **直接覆写** `watch.json`：

```kotlin
file.writeText(JSONObject().put("progress", p).put("daily", d).toString())
```

而 `watch.json` 是**每秒级高频写**的文件（播放中每 10 秒 + 暂停/seek/退出各一次）。风险链条：

1. 写到一半掉电 / 进程被杀 / 存储满 ⇒ 磁盘上留下**半个 JSON**；
2. 下次 `load()` 里 `JSONObject(text)` 抛异常；
3. 异常被 `runCatching { ... }` **整个吞掉**（连日志都没有）；
4. 结果：**续播进度与观看统计全部归零，且没有任何痕迹**。

这同时违反两条铁律：数据不能静默丢、失败不能静默。

## 二、修法

### 2.1 改用 `AtomicFile`

与 `sources/storage/SourceStore`、`media/marks/FilmMarksStore` 同一写法：

```kotlin
private val disk = AtomicFile(file)

out = disk.startWrite()
out.write(bytes)
disk.finishWrite(out)   // 成功才原子改名
// 失败：disk.failWrite(out) —— 半成品删掉、回滚到上一份完整文件
```

`AtomicFile` 的保证是：**任何时刻磁盘上都有一份完整文件**（写 `.new` → fsync → rename）。

### 2.2 损坏时留证据（不静默）

`load()` 的解析失败不再被吞：

```kotlin
val backup = File(file.parentFile, "${file.name}.corrupt-${System.currentTimeMillis()}")
val moved = runCatching { file.renameTo(backup) }.getOrDefault(false)
NetDiag.log("[watch] watch.json 解析失败（${e.javaClass.simpleName}: ${e.message}）；" +
    if (moved) "已备份为 ${backup.name}，本次从空开始" else "备份失败（原文件保留），本次从空开始")
```

**为什么不照搬 `SourceStore` 的"永不覆盖"**：源列表是低频写、用户配置最重要；
`watch.json` 是高频写，若"永不覆盖"会导致**新进度也存不下**。折中为
「**坏文件改名备份 + 从空开始**」——历史可查，新进度能存。

### 2.3 构造加可选文件名（为测试隔离）

```kotlin
class WatchStore(context: Context, fileName: String = "watch.json")
```

默认行为不变（现有调用 `WatchStore(context)` 全部照旧）；测试用 `watch.test.json`，
**不碰用户真实的观看进度**。

## 三、验证：4/4 PASS（`WatchStoreAtomicInstrumentation`）

| # | 用例 | 结果 |
| --- | --- | --- |
| 1 | 正常写读（report → 新实例读回 pos=60000） | ✅ |
| 2a | 塞半截 JSON ⇒ 不崩、新进度能存能读 | ✅ |
| 2b | 坏文件**留下备份**（1 份 `watch.test.json.corrupt-*`） | ✅ |
| 3 | 连续写 50 次，**每次写完立刻解析**都成功 | ✅ |

## 四、必须说清的边界

1. **用例 3 不能证明"旧实现会坏"**。`writeText` 是单次小写入，50 次正常时序下
   不太可能正好被读到半个文件。它证明的是「**没有引入回归**」，
   而不是「旧实现必然失败」。原子性的真实价值在**崩溃/掉电窗口**——那无法在正常运行中复现。
2. **真正能证明的是**：`AtomicFile` 是 Android 官方的原子写保证（写 `.new` + rename），
   用例 2a/2b 证明损坏恢复路径本身正确。
3. 只改了**持久化方式**；`report` / `progressOf` / `recentKeys` / `dailyMs` 的语义与阈值
   （95% 视为看完）**一行未动**。
4. 未测"存储写满"（`ENOSPC`）场景 —— `failWrite` 会回滚，但真机没造出这个条件。
5. 未在真机上验证"写一半真的断电"——没有可靠的掉电注入手段。

## 五、复算命令

```bash
export JAVA_HOME="C:/Android/jdk-17.0.13+11"; export ANDROID_HOME="C:/Android/sdk"
export PATH="/c/Android/sdk/platform-tools:$PATH"; export MSYS_NO_PATHCONV=1
cd android
./gradlew :app:assembleDebug :app:assembleDebugAndroidTest \
  -Pkivo.testRunner=com.kivo.cinema.stats.WatchStoreAtomicInstrumentation --offline
adb -s b6e950ee install -r app/build/outputs/apk/debug/app-debug.apk
adb -s b6e950ee install -r app/build/outputs/apk/androidTest/debug/app-debug-androidTest.apk
adb -s b6e950ee shell am instrument -w \
  com.kivo.cinema.test/com.kivo.cinema.stats.WatchStoreAtomicInstrumentation
```

期望：`4/4 PASS`。

## 六、改动文件

| 文件 | 改动 |
| --- | --- |
| `stats/WatchStore.kt` | `writeText` → `AtomicFile`；`load()` 解析失败备份 + 记日志；构造加可选 `fileName` |
| `androidTest/.../WatchStoreAtomicInstrumentation.kt` | **新增**，4 个断言 |
