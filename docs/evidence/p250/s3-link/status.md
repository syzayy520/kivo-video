# P250 · S3 · 断链感知：提示延迟 15 s → 1 s

日期：2026-11-10
设备：K40 `b6e950ee`（arm64-v8a / Android 13 / MIUI）
服务器：小米路由 `192.168.5.1`，匿名，共享 `XiaoMi`
片源：`视频/电影/The.Furious.2026.2160p.iTunes.WEB-DL.DDP.5.1.Atmos.HDR10+.H.265-CHDWEB.mkv`
　　　（20,905,428,916 B / HEVC 硬解 / 有音轨）
基线：`7c536dd9`（S2）

## 一、判据（设计 AC8）

> 播放中断网 → 5 s 内 UI 出现"连接中断，已暂停"；恢复网络后可继续（不退出重来）。

改动前的实测值：**15002 ms**（S2 阶段同一测试、同一片源、同样断网 12 s）。

## 二、改了什么

| 文件 | 改动 |
| --- | --- |
| `net/NetworkLinkMonitor.kt` | 新增。静态查询 `activeNetwork` + `NET_CAPABILITY_INTERNET`，无回调、无生命周期 |
| `net/RangeFd.kt` | 新增可选参数 `online`；非 null 时起 1 s 轮询看门狗（`KivoSmbLink`，daemon），掉线立刻置 `stalled` |
| `smb/SmbReader.kt` | `open()` 末尾调 `nativeSetReadTimeout(ctx, 3)`，失败即抛（不静默退回 15 s） |
| `jni/jni_smb2_bridge.cxx` | 新增 `nativeSetReadTimeout`（`smb2_set_timeout`） |
| `ui/compose/PlayerComposeActivity.kt` | 视频 + 软解音频两处 `RangeFd` 传 `online`（+3 行，迁移例外） |
| `smb/ui/SmbBrowserActivity.kt` | 三处失败文案改为断链优先（+13 行，迁移例外）：<br>没网时说"网络已断开"，而不是"账号或密码不正确" |
| `androidTest/.../SmbReconnectInstrumentation.kt` | 按生产接线传 `online`；新增 `BUFFERING` 打点，直接度量"位置停止 → 提示"的间隔 |

## 三、实测结果：1000 ms（判据 5000 ms）

```
adb -s b6e950ee shell am instrument -w \
  -e smbHost 192.168.5.1 -e smbShare XiaoMi -e windowSec 45 \
  com.kivo.cinema.test/com.kivo.cinema.smb.SmbReconnectInstrumentation
```

宿主编排：测试启动后第 15 秒 `svc wifi disable`，第 27 秒 `svc wifi enable`（断 12 秒）。

```
P243 OPEN 视频/电影/The.Furious...mkv size=20905428916
P243 START codec=OMX.qcom.video.decoder.hevc dur=6791201000 audio=true
PASS 起播（准备 + 首帧会话就绪）
P243 BUFFERING 位置停止后 1000 ms 首次上报重连中
P243 STALL 位置停在 18466 ms（已 5001 ms 未推进）
P243 RESUME 停 17002 ms 后从 18466 ms 接上，现在 35828 ms
P243 TAIL state=PLAYING pos=44951 ms buffering=true
PASS 断网时上报重连中状态
PASS 断网期间会话没有判死
PASS 观察到断网造成的停顿
P243 RECOVERED 停顿 17002 ms 后恢复
PASS 网络恢复后继续播放
5/5 PASS
```

| 指标 | 改前（S2） | 改后（S3） |
| --- | --- | --- |
| 位置停止 → 上报"重连中" | **15002 ms** | **1000 ms** |
| 与判据（5000 ms） | ❌ 3 倍超标 | ✅ 5 倍余量 |
| 是否早于 `STALL` 判定 | 否（晚 10 s） | **是（早 4 s）** |

`BUFFERING` 出现在 `STALL` 之前，说明看门狗比"位置 5 秒不动"这条被动判据更早发现 ——
这正是"网络状态是系统事实，不该等 IO 报错"的直接体现。

## 四、过程中发现的事实（三条，都很关键）

### 4.1 libsmb2 的读超时在断网时**不按设定值生效**

按设计 D2 把读超时从 15 s 降到 3 s：

```kotlin
// SmbReader.open() 末尾
val timeoutRc = Smb2Native.nativeSetReadTimeout(created, READ_TIMEOUT_SECONDS)  // 3
require(timeoutRc == 0) { "无法设置 SMB 读超时（rc=$timeoutRc）" }
```

`require` 通过（rc = 0，说明 `smb2_set_timeout(ctx, 3)` 真的执行了），
但断网后 `smb2_pread` 仍 **15002 ms** 才返回。

源码依据：`smb2_set_timeout` 是简单赋值（`lib/init.c:1017`），
PDU 排队时 `pdu->timeout = time(NULL) + smb2->timeout`（`lib/pdu.c:180`），
`sync.c:120` 每秒 `smb2_timeout_pdus` 检查一次 —— 按代码应该 3 s 生效。
**实际不生效。** 想回读 `ctx->timeout` 验证时发现 `struct smb2_context` 是不透明类型
（定义在私有头），拿不到。

**处置**：不在这一层继续挖（收益不明、且即使修好也只是 3 s，仍不如看门狗）。
把 D2 保留为"连接层面的双保险"，主手段改为 D3 的看门狗。已同步回设计稿 D2 的实测结论。

### 4.2 恢复仍要 17 秒，本卡不解决

`RESUME 停 17002 ms` 里，12 s 是断网本身，**剩下约 5 s 是"恢复后到读重新成功"**。
根因就是 4.1：那条在飞的读仍要等满 15 s 才回，退避再等一等。

归 **251**（单连接异步流水线 / 可中断读）。本卡只保证"用户知道断了"，不保证"多快恢复"。

### 4.3 MIUI 的 logcat `main` 环形缓冲只有 64 KiB，应用日志基本抓不到

诊断 4.1 时曾以为 `nativeSetReadTimeout` 没被调用，因为 `adb logcat -s KivoSmbJni` 一直为空。
实际原因：

```
$ adb shell logcat -g
main: ring buffer is 64 KiB (23 KiB consumed, 34 KiB readable)
```

4K 硬解播放几十秒就把 64 KiB 冲干净。即便 `logcat -G 8M` 放大后，
应用自身打的 `Log.i` 仍抓不到（同一次会话里只能看到系统进程日志与 15 条含包名的系统记录）。

**教训**：在这台设备上，**不要用 logcat 作为唯一证据来源**。
S3 因此改为"让测试自己把度量打印出来"（`P243 BUFFERING ... ms`），
这个数直接进 instrument 输出，比日志可靠得多。

## 五、复算

```bash
cd android && JAVA_HOME=/c/Android/jdk-17.0.13+11 \
  ./gradlew.bat assembleDebug :app:packageDebugAndroidTest \
  -Pkivo.testRunner=com.kivo.cinema.smb.SmbReconnectInstrumentation

adb install -r app/build/outputs/apk/debug/app-debug.apk
adb install -r app/build/outputs/apk/androidTest/debug/app-debug-androidTest.apk

# 后台起测，前台在第 15 秒断网、第 27 秒恢复
adb shell am instrument -w -e smbHost 192.168.5.1 -e smbShare XiaoMi -e windowSec 45 \
  com.kivo.cinema.test/com.kivo.cinema.smb.SmbReconnectInstrumentation &
sleep 15 && adb shell svc wifi disable && sleep 12 && adb shell svc wifi enable
```

判据：输出里 `P243 BUFFERING 位置停止后 N ms`，N ≤ 5000。

## 六、回归

- JVM 单测 42 例仍全绿（S3 未改纯逻辑单元）
- `assembleDebug` 通过
- S2 的 seek 回归未在本阶段重跑（S3 没动读路径，只加了看门狗线程）；
  提交后如需可复跑 `SmbPlaybackSeekInstrumentation` 4/4
