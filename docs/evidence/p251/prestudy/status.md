# P251 · 前置实验：三个否掉的假设，一个真瓶颈

日期：2026-11-10
设备：K40 `b6e950ee`（arm64-v8a / Android 13 / MIUI）
服务器：小米路由 `192.168.5.1`，匿名，共享 `XiaoMi`
片源：`视频/电影/The.Furious.2026.2160p...mkv`（20,905,428,916 B / HEVC）
性质：**前置研究，不是实现**。结论推翻了 250 评审时给 251 定的方向。

---

## 零、结论先行

251 原计划是"单连接异步流水线（多个 READ 同时在途）"。**三个实验把它否掉了，
并用数据指出了真正的瓶颈：JNI 的大数组搬运。**

| 假设 | 实验结果 | 判定 |
| --- | --- | --- |
| 并发不够 → 加在途请求 | 4 在途 vs 单在途：10.3 → 10.8 MB/s | ❌ 只涨 5% |
| 单次读太小 → 放大 chunk | 纯 C++ 下 1 MiB→4 MiB：13.2 → 53.9 MB/s | ✅ 对库成立 |
| 同上，经 JNI 也成立 | 经 JNI 下 1 MiB→4 MiB：7.8 → 10.4 MB/s | ❌ 只在 JNI 侧不成立 |

**真瓶颈**：同样的 4 MiB 单次读，纯 C++ 程序 53.9 MB/s，经 `nativeReadInto` 只有 10.4 MB/s，
**差 5 倍**。这正对上 250 评审里记的 P1-4：
`GetByteArrayElements` + `ReleaseByteArrayElements(buf, dst, 0)` 跨一次阻塞网络调用、
在 4 MiB 量级上等于一次全量拷贝加长时 pin。

---

## 一、实验 1：同一连接上 4 个请求在途

程序：`kivo_async_spike.cpp`（一次性，未入库）。同步 8 × 1 MiB 与异步 8 × 1 MiB（4 在途）对比。

```
open ok: size=20905428916
sync  : 8 x 1MiB in 776 ms -> 10.3 MB/s
async : 8 x 1MiB in 742 ms -> 10.8 MB/s (inflight=4, done=8)
```

**结论**：并发读同一文件在这台 NAS 上没有并行收益。
与 247 记录的"两条连接互相拖慢，1 MiB 92 → 543 ms"同源 —— 服务端对同一文件串行化。
所以"异步流水线"这个方向不成立，至少对家庭 NAS 不成立。

## 二、实验 2：单次读大小扫描（纯 C++ / 直调 libsmb2）

程序：`kivo_chunk_spike.cpp`。每个尺寸读 16 MiB，从文件中段开始。

| chunk | 吞吐 | 每次耗时 |
| --- | --- | --- |
| 64 KiB | 2.3 MB/s | 26.7 ms |
| 128 KiB | 8.1 MB/s | 15.5 ms |
| 256 KiB | 9.5 MB/s | 26.2 ms |
| 512 KiB | 12.4 MB/s | 40.3 ms |
| 1 MiB | 13.2 MB/s | 75.9 ms |
| 2 MiB | 26.3 MB/s | 76.1 ms |
| 4 MiB | **53.9 MB/s** | 74.2 ms |
| 8 MiB | **94.7 MB/s** | 84.5 ms |

> **⚠️ 2026-11-10 晚更正：这张表全部作废。**
> `kivo_chunk_spike.cpp:91` 写的是 `bytes = ok * chunk` —— 算的是**请求**字节，
> 不是**返回**字节。而服务端把单次 READ 封在 1 MiB（见下），于是 2/4/8 MiB 三行的
> 吞吐被等比放大 2/4/8 倍。13.2 × 4 = 52.8 ≈ 53.9，正是这个错误的比例。
> 真实上限是 **8–13 MB/s**，与 chunk 大小基本无关。详见
> [window-double-fill](./window-double-fill/status.md)。

**当时的（错误）读法**：1 MiB 以上单次耗时几乎恒定（74–85 ms）——"延迟主导，吞吐 ∝ chunk"。
这个"延迟恒定"本身是真的，但恒定的原因是**每次只回 1 MiB**，不是"传了 4 MiB 只用 74 ms"。

**服务端上限**：`libsmb2.c:1122` 用协商出来的 `rep->max_read_size` 作为上限，
`smb2-cmd-read.c:399` 超过就报错。小米路由的 Samba 协商结果是 **1 MiB**，
所以 `smb2_pread(len=4 MiB)` 只会返回 1 MiB —— 客户端任何 chunk 配置都突破不了它。

**顺带纠正一个 247 的隐含前提**：247 说"1 MiB/92 ms ≈ 11 MB/s，码率 3.6 MB/s，余量 3 倍"。
那个 3 倍是按 1080p 算的。**4K 原盘要 15 MB/s，13.2 MB/s 连余量都没有。**

## 三、实验 3：同样的扫描，走生产 JNI 路径

工具：`SmbReadAheadInstrumentation.kt`（已入库，可复算）。

```
RAW chunk=64K   read=24MiB in 7.34s ->  3.3 MB/s (19.1 ms/req)
RAW chunk=256K  read=24MiB in 3.05s ->  7.9 MB/s (31.4 ms/req)
RAW chunk=512K  read=24MiB in 2.62s ->  9.2 MB/s (53.5 ms/req)
RAW chunk=1024K read=24MiB in 3.06s ->  7.8 MB/s (122.3 ms/req)
RAW chunk=2048K read=12MiB in 1.14s -> 10.5 MB/s (163.0 ms/req)
RAW chunk=4096K read=6MiB  in 0.58s -> 10.4 MB/s (287.9 ms/req)
```

对比（**1 MiB 这一行才是有效的**，更大的 chunk 走 JNI 时用的也是实际返回字节）：

| chunk | 纯 C++ | 经 JNI（零拷贝前） | 判定 |
| --- | --- | --- | --- |
| 64 KiB | 2.3 | 3.3 | 基本一致 |
| 1 MiB | 13.2 | 7.8 | **真实差距 1.7×，来自 JNI 拷贝** |
| 4 MiB | ~~53.9~~ | 10.4 | 纯 C++ 一侧是错数，**不可比** |

**已被后续实验更正**：所谓"JNI 把大块读放大 4 倍"不成立。`2048K read=12MiB` 这一行
说明 24 MiB 预算只读出 12 MiB —— 12 次请求全部短读，每次 1 MiB。
JNI 在 1 MiB 上 12.4 MB/s、纯 C++ 13.2 MB/s，差 6%，与实现无关。

## 四、被数据否掉的一次临时改动（已回滚）

按实验 2 的推论，临时把 `SmbReadTuning.aheadChunk` 从 1 MiB 改到 4 MiB
（`aheadBytes` 12 MiB），预期软解吞吐提升 3~4 倍。实测：

```
AHEAD prefetch=off 5.2 MB/s / on 3.9 MB/s / 放大 0.75x
```

**开预读反而慢 25%**，与 247 在硬解上观察到的"24.00 → 20.27 fps"一致。

原因就是实验 3：4 MiB 经 JNI 要 288 ms，比 1 MiB 的 122 ms 只多读 4 倍数据却多花 2.4 倍时间，
加上第二条连接抢链路，净收益为负。

**该改动已 `git checkout` 回滚**，`SmbReadTuning` 保持 247 的验证值（6 MiB 环 / 1 MiB chunk）。
没有证据的改动不入库 —— 这条在这次守住了。

## 五、实验 4：零拷贝 JNI（已落地）

按 §五 旧清单的第 1 条做了最小改动：`nativeReadInto` 不再用
`GetByteArrayElements` + `Release(...,0)`，改为读进**线程本地** `std::vector`
再 `SetByteArrayRegion` 拷一次。

| chunk | 改前 | 改后 | 纯 C++ |
| --- | --- | --- | --- |
| 64 KiB | 3.3 | 4.7 | 2.3 |
| 256 KiB | 7.9 | 10.4 | 9.5 |
| 512 KiB | 9.2 | 11.6 | 12.4 |
| 1 MiB | 7.8 | **12.4** | 13.2 |
| 2 MiB | 10.5 | 11.2 | 26.3 |
| 4 MiB | 10.4 | 11.8 | 53.9 |

两个结论：

1. **1 MiB 已追平纯 C++**（12.4 vs 13.2）。JNI 拷贝确实是一部分代价。
2. **4 MiB 仍差 3.4 倍**（253.4 ms vs 74.2 ms）。说明还有一块 3 倍的开销没定位。
   最可能是 libsmb2 内部按 `max_read_size` 把大读拆成多个 READ 请求 ——
   若拆成 4 × 1 MiB，4 × 74 = 296 ms，与实测 253 ms 同量级。
   **这一条没有证据**（`struct smb2_context` 不透明，读不到 `max_read_size`），
   列为 251 的下一个待查项。

**这条同时证伪了"读放大"**：改后 1 MiB（12.4）仍优于 4 MiB（11.8）。
所以 `aheadChunk` 保持 1 MiB 是对的，之前的回滚是对的。

**回归**：seek 4/4 PASS，落点 `27105 ms`（改前 `27106`），`first_frame=3.9ms`，
`frames=67 dropped=0` —— 零拷贝没有引入任何回归。

### 顺带拿到生产路径的数字（补权限后终于能跑）

```
WINDOW 64KiB sequential -> 5.4 MB/s      （关预读，走块窗口）
AHEAD  64KiB sequential -> 5.0 MB/s      （开预读）
```

裸读 1 MiB 是 12.4 MB/s，**走窗口只剩 5.4** —— 掉了一半。
24 MiB 实测 4.43 s，而理论值（12 次 2 MiB 填充 × 153 ms）只有 1.84 s。
**窗口层的额外开销是下一个靶子**，而且它才是直接影响播放的那个数：
4K 原盘要 15 MB/s，现在只有 5.4。

---

## 六、251 应该做什么（2026-11-10 晚最终版）

1. ~~JNI 零拷贝~~ —— **已完成**，1 MiB 12.4 vs 纯 C++ 13.2。
2. ~~查窗口层为什么掉一半~~ —— **已完成，是块被读了两次**。
   24 MiB 逻辑读实际拉了 50 MiB，修复后 WINDOW 5.4 → 7.2 MB/s。
   详见 [window-double-fill](./window-double-fill/status.md)。
3. ~~查 4 MiB 的"3 倍额外开销"~~ —— **前提不存在**：§二 的纯 C++ 数字算的是请求字节，
   服务端单次只回 1 MiB。JNI 没有额外开销。
4. **前面三条都关掉之后，剩下的就是服务端上限**：单连接 8–9 MB/s，由
   `max_read_size = 1 MiB` + 约 120 ms 往返决定。客户端已吃到 83–90%。
5. **预读的去留**：仍略低于不预读（6.9 vs 7.2），但它的价值是遮挡抖动，
   不是提吞吐。**保留**，且不要再为它加连接。

## 七、复算

```bash
# 实验 1、2 的程序是一次性的（不进仓库），编译方式：
NDK=/c/Android/sdk/ndk/27.2.12479018/toolchains/llvm/prebuilt/windows-x86_64/bin
$NDK/aarch64-linux-android26-clang++.cmd -std=c++23 -O1 -static-libstdc++ \
  -I "C:/kivo video/third_party/libsmb2-src/include" \
  -I "C:/kivo video/third_party/libsmb2-src/include/smb2" \
  -o kivo_chunk_spike kivo_chunk_spike.cpp \
  "C:/kivo video/build/libsmb2-android/arm64/lib/libsmb2.a"
adb push kivo_chunk_spike /data/local/tmp/ && adb shell chmod 755 /data/local/tmp/kivo_chunk_spike
adb shell /data/local/tmp/kivo_chunk_spike

# 实验 3（已入库，可反复跑）：
cd android && JAVA_HOME=C:/Android/jdk-17.0.13+11 ./gradlew.bat :app:packageDebugAndroidTest \
  -Pkivo.testRunner=com.kivo.cinema.smb.SmbReadAheadInstrumentation
adb shell am instrument -w -e smbHost 192.168.5.1 -e smbShare XiaoMi \
  com.kivo.cinema.test/com.kivo.cinema.smb.SmbReadAheadInstrumentation
```

## 八、这一轮没做到的

- 实验 3 的 B/C 段（走窗口的 64 KiB 顺序读）被 `SecurityException` 打断，见
  `../../p250/s6-link-permission/status.md`。补上权限后可以通过同一个工具复跑。
- "为什么 64 KiB 经 JNI 反而比纯 C++ 略快"没解释（可能是批读/缓存的偶然），
  但它不影响结论：**大块读经 JNI 严重退化**。
