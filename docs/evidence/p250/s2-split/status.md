# P250 · S2 · `SmbByteSource` 拆分（已收口）

日期：2026-11-10
基线：`014e91f7`（S1）
设备：K40 `b6e950ee`（arm64-v8a / Android 13 / MIUI）
服务器：小米路由 `192.168.5.1`，匿名，共享 `XiaoMi`
片源：`视频/kivo-test/local-seek.mkv`（2.2 MB / H.264 1280×720 / 60 s / 无音轨）
状态：**代码、单测、两条真机回归全通过，已提交。**

## 一、拆分结果（664 行 → 6 个单元，全部达标）

| 文件 | 行数 | 预算 | 职责（一句话） |
| --- | --- | --- | --- |
| `SmbByteSource.kt` | **170** | ≤180 | 按什么顺序问下面五个 |
| `SmbReadAhead.kt` | **214** | ≤220 | 谁来填、何时填、何时停 |
| `SmbReadRing.kt` | **101** | ≤140 | 预读环怎么存（纯数据） |
| `SmbReadWindow.kt` | **90** | ≤180 | 记住哪一块已在内存 |
| `SmbPreadChannel.kt` | **91** | ≤170 | 把一次读发出去，失败再来 |
| `SmbReadStats.kt` | **95** | ≤120 | 记录"刚才读得好不好" |
| 合计 | 761 | —— | 拆分前 664（原文件含全部 6 职责） |

`git diff --stat`：`SmbByteSource.kt` **100 insertions, 594 deletions**。

拆 `SmbReadRing` 的理由：`SmbReadAhead` 首版 241 行超预算，且"缓冲怎么存"与
"谁去填"确实是两个修改理由。拆出后缓冲可单测（见下），以前只能靠真机播放验。

## 二、JVM 单测：42 用例全绿（新增 10 个覆盖环形缓冲）

```
$ cd android && JAVA_HOME=/c/Android/jdk-17.0.13+11 ./gradlew.bat testDebugUnitTest assembleDebug
BUILD SUCCESSFUL in 14s
```

| 测试类 | 用例 | 失败 |
| --- | --- | --- |
| `SmbAddressTest` | 12 | 0 |
| `SmbReadTuningTest` | 7 | 0 |
| `SmbReadRingTest`（S2 新增） | 10 | 0 |
| `SmbRetryGateTest` | 7 | 0 |
| `SmbErrorTest` | 6 | 0 |
| **合计** | **42** | **0** |

新增的 10 个里，四条是以前"只能用真机播放验"的：

- `stale epoch data is discarded` —— 消费者跳读后旧位置数据必须整批丢弃（花屏级）；
- `request crossing the filled end is not a hit` —— 部分命中不算，必须走同步读；
- `request before the consumed point is not a hit` —— 基点跑过头时不能从旧区间取；
- `compact does nothing before half is consumed` —— 前移阈值不能太小，否则频繁搬运。

## 三、真机回归 1：播放 + seek（硬解链路）4/4 PASS

```
adb -s b6e950ee shell am instrument -w \
  -e smbHost 192.168.5.1 -e smbShare XiaoMi \
  -e smbPath 视频/kivo-test/local-seek.mkv \
  com.kivo.cinema.test/com.kivo.cinema.smb.SmbPlaybackSeekInstrumentation
```

```
P243 OPEN 视频/kivo-test/local-seek.mkv size=2270964
P243 START codec=OMX.qcom.video.decoder.avc dur=60000000 audio=false
PASS 起播（准备 + 首帧会话就绪）
P243 POSITION 0 ms → 2931 ms
PASS 位置推进（3 秒窗口）
P243 SEEK target=30000 ms landing=25000 ms actual=27106 ms
P243 SUMMARY ... frames=67 dropped=0 first_frame=5.0ms completed=0 error=-
PASS seek 到中段并落位
P243 RESUME 27106 ms → 30106 ms
PASS seek 后继续推进
4/4 PASS
```

**落点 `27106 ms` 与 243 记录的 `27106 ms` 逐位一致** —— 拆分没有移动任何读取行为。
`frames=67 / dropped=0 / completed=0` 也与基线同量级。

## 四、真机回归 2：断链恢复（重连路径）5/5 PASS

重连逻辑在 S2 里被搬进了 `SmbPreadChannel` + `SmbRetryGate`（全新代码），所以这条必须跑。

```
adb -s b6e950ee shell am instrument -w \
  -e smbHost 192.168.5.1 -e smbShare XiaoMi \
  -e smbPath 视频/kivo-test/local-seek.mkv -e windowSec 85 \
  com.kivo.cinema.test/com.kivo.cinema.smb.SmbReconnectInstrumentation
```

宿主编排：测试跑到第 20 秒 `svc wifi disable`，第 42 秒 `svc wifi enable`（断 22 秒）。

```
P243 START codec=OMX.qcom.video.decoder.avc dur=60000000 audio=false
PASS 起播（准备 + 首帧会话就绪）
P243 STALL 位置停在 37955 ms（已 5001 ms 未推进）
P243 RESUME 停 18004 ms 后从 37955 ms 接上，现在 55997 ms
P243 TAIL state=ENDED pos=59855 ms buffering=true
PASS 断网时上报重连中状态
PASS 断网期间会话没有判死
PASS 观察到断网造成的停顿
P243 RECOVERED 停顿 18004 ms 后恢复
PASS 网络恢复后继续播放
5/5 PASS
```

### 附带得到的 P0-1 实证（本卡不修，S3 修）

第一次跑同一测试时，断网只持续 **12 秒**，结果：

```
P243 STALL 位置停在 37918 ms（已 5000 ms 未推进）
P243 TAIL state=PLAYING pos=37981 ms buffering=false
FAIL 断网期间 buffering 始终为 false，界面无法提示"正在重连"
```

**这不是拆分回归，是 P0-1 的现场证据**：读还卡在 `smb2_pread` 里等 15 秒 PDU 超时，
12 秒的断网窗口内根本没有异常抛出，所以 `RangeFd.stalled` 不动、UI 不会提示。
把断网拉到 22 秒（超过超时）后 5/5 PASS，恢复耗时 **18004 ms** —— 这个数字正好是
15 s 超时 + 退避重试，也就是设计 AC8 要求"5 秒内提示"与实际之间那 13 秒的差距。

这条记录同时说明：**重连代码本身在 S2 之后是好的**，坏的是"多久才知道断了"。

## 五、行为等价性人工核对（readAt 分支逐一对照）

| 原 `SmbByteSource.readAt` | 现在 | 等价 |
| --- | --- | --- |
| `closed` / `len<=0` / 越界 | 同 | ✅ |
| `++statReads` | `stats.onRead()` | ✅ |
| `copyFromPrefetch` → 命中 | `ahead.copyIfPresent` → `stats.onHit(true)` | ✅ |
| `copyFromWindow` → 命中 | `window.copyIfPresent` → `stats.onHit(false)` | ✅ |
| `waitPrefetch` | `ahead.waitFor(..., tuning.aheadWaitNs)` | ✅ 350 ms 不变 |
| `want >= BLOCK_BYTES/2` → 直读 | `channel.readFully(...)`，仍在 `try` 内，`finally` 仍记一次 fill | ✅ |
| `fillWindow` | `window.fill(offset, sizeBytes, channel::readFully)` | ✅ |
| `finally` 里 `(filledEnd-1)/BLOCK` | `window.blockIndex(offset)` | ✅ 数学等价：`filledEnd-1` 必落在 `align(offset)` 块内 |
| 顺序 → `restartPrefetchLocked(maxOf(offset+want, filledEnd))` | `ahead.start(同参数)` | ✅ |
| 乱序 → 暂停（不销毁） | `ahead.pause()` | ✅ |
| 填完再取一次 → 跨界直读 | 同 | ✅ |

### 顺带修掉的一个原缺陷

原 `fillWindow` 是"填完才写 `blockStart[slot] = aligned`"。若填充中途抛异常，
该槽位的 `blockStart` 仍指向**旧偏移**，但数据已被写入半截新内容 ——
下一次"命中"会返回错位数据，画面直接从错的地方开始。
现在 `SmbReadWindow.fill` 进入填充前就置 `starts[slot] = -1`。

## 六、与设计稿的偏差（已同步回 §3.2 实施期修订）

1. 新增 `SmbReadRing`。
2. `setPrefetchEnabled(false)` 由"永久关闭"改为"暂停"（D5，修 P1-3）。
3. `directRead` 并入 `SmbPreadChannel.readFully`（"读满"语义只留一个实现）。
4. `fill` 先标废槽位（修原缺陷）。

## 七、复算

```bash
cd android && JAVA_HOME=/c/Android/jdk-17.0.13+11 \
  ./gradlew.bat testDebugUnitTest assembleDebug

grep -h -o 'tests="[0-9]*" skipped="[0-9]*" failures="[0-9]*" errors="[0-9]*"' \
  app/build/test-results/testDebugUnitTest/*.xml

# 真机（先打开 MIUI 的「USB 安装」，否则首次安装会被拦）
./gradlew.bat :app:packageDebugAndroidTest \
  -Pkivo.testRunner=com.kivo.cinema.smb.SmbPlaybackSeekInstrumentation
adb install -r app/build/outputs/apk/debug/app-debug.apk
adb install -r app/build/outputs/apk/androidTest/debug/app-debug-androidTest.apk
adb shell am instrument -w -e smbShare XiaoMi -e smbPath 视频/kivo-test/local-seek.mkv \
  com.kivo.cinema.test/com.kivo.cinema.smb.SmbPlaybackSeekInstrumentation
```
