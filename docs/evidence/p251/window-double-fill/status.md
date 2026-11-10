# 251 第二步：块窗口的"双倍读取"（已修）

日期：2026-11-10 晚
设备：K40 `b6e950ee`（Android 13 / kona）
服务器：小米路由 `192.168.5.1` 共享 `XiaoMi`（匿名只读）
片源：`视频/电影/The.Furious.2026.2160p...H.265-CHDWEB.mkv`（20.9 GB）

## 一、为什么查这里

上一轮（[prestudy](../prestudy/status.md) §五）拿到生产路径吞吐：

```
WINDOW 64KiB 顺序读  5.4 MB/s
AHEAD  64KiB 顺序读  5.0 MB/s
```

而裸读 1 MiB 是 12.4 MB/s。走窗口掉一半，而窗口层做的事只有"把整块读进内存、
后面 31 次直接拷"——**它只该更快，不该更慢**。24 MiB 顺序读在块 2 MiB 时
理论上只需 12 次填充，按裸读 2 MiB 的耗时算约 2 秒，实测 4.43 秒。

差的 2.4 秒必须落到某处，三种可能的修法完全不同，所以先做诊断而不是改代码。

## 二、诊断工具

新增 `androidTest/smb/SmbWindowDiagInstrumentation.kt`。它**绕过** `SmbByteSource`，
直接用 `SmbReadWindow` 复刻窗口路径（命中就拷、不命中就填一整块），把四个计数分开报：

- 填充次数、其中"一次填充发了多次 pread"的次数；
- pread 总次数与总字节；
- 每次填充的耗时（avg / max）；
- 跨块边界的直读次数与耗时。

最后一项在上一版诊断里**漏掉了**，导致总时间比生产少 1.4 秒、数字凭空好看 —— 本轮补上。

## 三、修复前的输出

```
OPEN  size=20905428916 block=2048K blocks=6 ahead=6MiB
RAW-2MiB req=2048K read=12MiB in 1.54s -> 7.8 MB/s 128.5 ms/req short=12
WINDOW   64KiB x 24576K in 6.07s -> 4.0 MB/s
FILL     count=25 multi_pread=25 preads=50 bytes=50MiB pread/块=2.00 avg=242.6ms max=261.9ms
COPY     hit_copy_total=6.8ms straddle=12
```

三个事实：

1. **`short=12`**：12 次 2 MiB 请求**全部短读**，每次都只回 1 MiB。
   服务端协商的 `max_read_size` 就是 1 MiB，客户端改不了。
2. **`count=25`、`bytes=50MiB`**：24 MiB 的逻辑读，实际从网上拉了 **50 MiB**。
   一个块被填了两次。
3. **`straddle=12`**：每块末尾都有一次跨块边界的请求。

### 双倍读取的机制

`SmbByteSource.readAt` 的顺序是：命中就返回，不命中就 `fill(offset)`。
而 `fill` 用 `align(offset)` 决定填哪一块 —— 跨块边界的那一次请求，`offset`
**仍落在前一块内**，于是 `fill` 把刚读进来的整块又读了一遍。填完之后
`copyIfPresent` 依然不命中（请求确实跨块），再退化成一发直读。

净效果：每 2 MiB 读了 4 MiB，外加一次 64 KiB 直读。

## 四、修复

`SmbReadWindow.fill` 开头加一个前置判断：**该块已完整驻留就直接返回结束偏移，不发网络读。**

```kotlin
residentEnd(aligned, sizeBytes, length)?.let { return it }
```

`residentEnd` 只认两种"完整"：读满整块，或读到了文件尾（尾部块本来就短）。
**中途短读留下的半块不算** —— 那种情形必须重读，否则会把残数据当有效数据。

改动 22 行，`SmbReadWindow` 90 → 112 行（预算 180）。`SmbByteSource` 一行未动。

## 五、修复后的输出

```
RAW-2MiB req=2048K read=12MiB in 1.46s -> 8.2 MB/s 121.7 ms/req short=12
WINDOW   64KiB x 24576K in 3.22s -> 7.4 MB/s
FILL     count=25 multi_pread=13 preads=26 bytes=26MiB avg=121.0ms max=260.6ms
COPY     hit_copy_total=4.9ms straddle=12 straddle_total=193ms avg=16.0ms
```

- `bytes` **50 MiB → 26 MiB**（24 MiB 逻辑 + 起点未对齐多出的一个块）；
- `multi_pread=13`：13 次真填充，`count=25` 里另 12 次是驻留直接返回（不再发读）；
- **跨块直读只有 16 ms/次**（总 193 ms），不是之前推断的 120 ms；

> 上一版把 `fill_avg` 除以 `count=25`（含 12 次空操作）算出 121 ms 的"平均"，
> 又拿它反推"12 次跨块读吃掉 1.4 秒"。那是算错的：121 × 25 ≈ 3.02 s 已经
> 基本等于总时间，跨块读只占 193 ms。**追这个不存在的缺口会白做一次拼接优化。**

## 六、生产路径（走 `SmbByteSource`，同一把尺子）

`SmbReadAheadInstrumentation`，同一片源同一设备：

```
RAW chunk=64K   read=24MiB in 12.75s -> 1.9 MB/s (33.1 ms/req)
RAW chunk=256K  read=24MiB in  3.16s -> 7.6 MB/s
RAW chunk=512K  read=24MiB in  3.02s -> 7.9 MB/s
RAW chunk=1024K read=24MiB in  2.81s -> 8.6 MB/s (112.3 ms/req)
RAW chunk=2048K read=12MiB in  1.35s -> 8.9 MB/s   ← 只读出 12 MiB：每次短读 1 MiB
RAW chunk=4096K read= 6MiB in  0.71s -> 8.5 MB/s   ← 只读出  6 MiB：同上
WINDOW 64KiB sequential -> 7.2 MB/s
AHEAD  64KiB sequential -> 6.9 MB/s
```

| 指标 | 修复前 | 修复后 | 裸读上限 |
| --- | --- | --- | --- |
| WINDOW（关预读） | 5.4 MB/s | **7.2 MB/s** | 8.6–8.9 |
| AHEAD（开预读） | 5.0 MB/s | **6.9 MB/s** | 8.6–8.9 |

窗口路径从裸读的 61% 提到 **83%**；预读那条 79%。

## 七、这一轮真正学到的

1. **这台 NAS 的单连接上限是 8–9 MB/s**，由服务端协商的 `max_read_size = 1 MiB`
   与约 120 ms 往返共同决定。客户端 chunk、块大小、预读开不开都突破不了它。
   4K 原盘要 15 MB/s —— **这台路由器给不了**，不是播放器的问题。
2. **"加并发"在这台机器上无效**（[prestudy §一](../prestudy/status.md)：4 个在途只涨 5%），
   所以上限就是单连接的 8–9 MB/s。想更快只能换服务端（真 NAS / 有线）。
3. **预读仍略低于不预读**（6.9 vs 7.2）。它的价值从来不是吞吐而是抖动：
   把 120–250 ms 的同步读挡在解码线程之外。这条数据反过来也说明——
   **凡是靠"再开一条连接换吞吐"的设计，在这台机器上都是负收益。**

## 八、剩余可做（按证据）

| 项 | 预期收益 | 判断 |
| --- | --- | --- |
| 跨块边界拼接（头部拷窗口、尾部填下一块） | +6%（193 ms / 3.2 s） | **不做**。要多 20 行编排代码，换 0.5 MB/s |
| 换块大小 1 MiB（对齐服务端上限） | 0 | 每次填充 1 个往返而不是 2 个，但总往返数不变 |
| 单连接异步流水线 | 0（已证伪，+5%） | 不做 |
| 服务端换真 NAS / 走有线 | 可能 3–10× | **不是客户端能解决的问题**，写进交接 |

## 九、复算

```bash
export JAVA_HOME="C:/Android/jdk-17.0.13+11"
export PATH="$PATH:/c/Android/sdk/platform-tools"

cd "C:/kivo video/android"
./gradlew.bat testDebugUnitTest \
  --tests "com.kivo.cinema.smb.SmbReadWindowTest" --console=plain
./gradlew.bat :app:packageDebugAndroidTest \
  -Pkivo.testRunner=com.kivo.cinema.smb.SmbWindowDiagInstrumentation --console=plain

adb install -r app/build/outputs/apk/androidTest/debug/app-debug-androidTest.apk
adb shell am instrument -w -e smbHost 192.168.5.1 -e smbShare XiaoMi \
  com.kivo.cinema.test/com.kivo.cinema.smb.SmbWindowDiagInstrumentation
```

JVM 侧另有 5 个用例（`SmbReadWindowTest`）把这条修复钉住：已驻留不重读、
24 块只发 24 次读、中途短读必须重读、EOF 短块算完整、命中字节与写入一致。
