# P250 · S1 · 纯函数族（设计稿落地）

日期：2026-11-10
设备：Redmi K40 `b6e950ee`（arm64-v8a / Android 13）—— 用于 UTF-8 编解码自检
基线：`a2134782`（P240–P249 收口提交）

## 一、这一阶段做了什么

只做**纯函数与纯数据**，不接线、不改任何现有行为：

| 文件 | 类型 | 职责 |
| --- | --- | --- |
| `smb/SmbReadTuning.kt` | 新 | 由文件大小推导读取参数（块大小/块数/预读环/等待上限），含夹紧 |
| `smb/SmbRetryGate.kt` | 新 | 重连退避状态机：`canAttempt` / `noteSuccess` / `noteFailure` / `denyReason` |
| `smb/SmbAddress.kt`（+`authority`） | 改 | `host:port` 的唯一正确形态：IPv6 方括号、保留 zone id |
| `smb/SmbError.kt`（+`fromReadCode`/`IO_TIMEOUT`） | 改 | 读失败用返回码归因，区分 NT status 与 `-errno` |
| `jni/utf8_codec.hpp` | 新 | 标准 UTF-8 ↔ UTF-16（无 JNI 类型、不抛异常） |
| `app/build.gradle.kts`（+`testImplementation`） | 改 | JUnit 4.13.2（**本地 Gradle 缓存已有，不联网**） |
| `src/test/.../smb/*Test.kt` × 4 | 新 | 32 个用例 |

**S1 不改变任何运行时行为**：新类无人调用，`authority`/`fromReadCode` 是新增 API，
`utf8_codec.hpp` 未被任何 .cxx include。所以本阶段不需要真机播放回归，只需要"能编 + 单测绿"。

## 二、JVM 单测：32 用例全绿

```
$ cd android && JAVA_HOME=/c/Android/jdk-17.0.13+11 ./gradlew.bat testDebugUnitTest \
    --tests "com.kivo.cinema.smb.*"
BUILD SUCCESSFUL in 1m 6s
```

按测试类统计（`app/build/test-results/testDebugUnitTest/*.xml`）：

| 测试类 | 用例数 | 失败 |
| --- | --- | --- |
| `SmbAddressTest` | 12 | 0 |
| `SmbReadTuningTest` | 7 | 0 |
| `SmbRetryGateTest` | 7 | 0 |
| `SmbErrorTest` | 6 | 0 |
| **合计** | **32** | **0** |

关键用例（都是"错一次就整条链路不通"的）：

- `ipv6 authority is always bracketed` / `link local zone id survives parsing` ——
  对应 P1-1：libsmb2 `socket.c` 只在首字符是 `[` 时按 IPv6 解析。
- `timeout is reported as a timeout not as the previous error` ——
  对应 P1-8：超时路径没有回复报文，`smb2_get_nterror` 是上一次的残值。
- `offline blocks attempts and does not escalate the backoff` / `link recovery allows an immediate attempt` ——
  对应 243 r1 的终态 bug 与 245 的"网回来了画面还停着"。
- `every tier keeps the ring able to hold two chunks` —— 预读环 < 2 chunk 会空转。

## 三、UTF-8 编解码：真机 12 项全 PASS

这一项不能进 JUnit（是 C++ 头），所以编成 arm64 可执行直接推手机跑，
而不是只做 `-fsyntax-only`——语法对不代表语义对。

```bash
# 编译（一次性自检程序放在系统临时目录，不进仓库）
NDK=/c/Android/sdk/ndk/27.2.12479018/toolchains/llvm/prebuilt/windows-x86_64/bin
TMP="C:/Users/Administrator/AppData/Local/Temp"
$NDK/aarch64-linux-android26-clang++.cmd -std=c++23 -O1 -static-libstdc++ \
  -I "C:/kivo video/android/app/src/main/cpp/jni" \
  -o "$TMP/kivo_utf8_check" "$TMP/kivo_utf8_check.cpp"

# 推真机运行
export MSYS_NO_PATHCONV=1
adb push "$TMP/kivo_utf8_check" /data/local/tmp/kivo_utf8_check
adb shell chmod 755 /data/local/tmp/kivo_utf8_check
adb shell /data/local/tmp/kivo_utf8_check
```

输出（原样）：

```
PASS cjk unit count
PASS cjk roundtrip
PASS emoji becomes a surrogate pair
PASS emoji roundtrip
PASS invalid byte becomes U+FFFD
PASS overlong rejected
PASS lone surrogate becomes U+FFFD
PASS u+ffff roundtrip
PASS u+10ffff takes two units
PASS u+10ffff roundtrip
PASS truncated sequence becomes U+FFFD
PASS empty roundtrip
failures=0
```

覆盖的四类风险：

| 用例 | 防的是什么 |
| --- | --- |
| cjk roundtrip | 3 字节 UTF-8（中文文件名）往返不变 |
| emoji becomes a surrogate pair + roundtrip | 4 字节 UTF-8 ↔ 代理对，**这正是 `NewStringUTF` 会 abort 的那类** |
| invalid byte / overlong / truncated → U+FFFD | 坏字节不让整目录失败（宽容策略） |
| lone surrogate → U+FFFD | 反向：Java 侧给了孤立代理，不能产出非法 UTF-8 |

### 一次写错并修正（保留，不粉饰）

首轮 `FAIL cjk unit count`：断言写成 7，实际"测试.mkv"是 **6** 个 UTF-16 单元
（测=1、试=1、`.`=1、m/k/v=3）。**roundtrip 是同轮通过的**，说明编解码正确、断言数错了。
改断言为 6 后 12/12 全 PASS。这也说明"往返相等"比"数单元个数"更能证明正确性——
S4 接真机时以行为为准，不以计数为准。

## 四、复算方式

```bash
# 单测（毫秒级反馈，不需要设备）
cd android && JAVA_HOME=/c/Android/jdk-17.0.13+11 ./gradlew.bat testDebugUnitTest --tests "com.kivo.cinema.smb.*"

# 统计实际用例数
grep -h -o 'tests="[0-9]*" skipped="[0-9]*" failures="[0-9]*" errors="[0-9]*"' \
  android/app/build/test-results/testDebugUnitTest/*.xml
```

## 五、没做的事（诚实记录）

- 本阶段**不动** `SmbByteSource`：新类虽然已存在，但仍然没有任何调用点。
  接线在 S2（拆分）与 S3（断链），那时才需要真机回归。
- `utf8_codec.hpp` 未被 `jni_string_convert.hpp` 引用：改共用头会波及
  `jni_player_bridge` / `jni_session_handle`，按设计稿放在 S4 一次做完并连带回归。
