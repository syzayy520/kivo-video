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

**读法**：1 MiB 以上，单次耗时几乎恒定（74–85 ms）——**延迟主导，不是带宽主导**。
所以吞吐 ∝ chunk，放大 4 倍 chunk 就是 4 倍吞吐。

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

对比：

| chunk | 纯 C++ | 经 JNI | 差距 |
| --- | --- | --- | --- |
| 64 KiB | 2.3 | 3.3 | JNI 略快（噪声/缓存） |
| 1 MiB | 13.2 | 7.8 | 1.7× |
| 4 MiB | **53.9** | **10.4** | **5.2×** |

每次耗时：纯 C++ 4 MiB 是 74.2 ms，经 JNI 是 **287.9 ms**。

**结论**：库本身没问题，JNI 把大块读的开销放大了 4 倍。
`smb2_pread` 的 4 MiB 数据要写进 `GetByteArrayElements` 拿到的指针，再 `Release(...,0)`
拷回 —— 4 MiB × (pin + 回写) 在 ART 上代价极高。

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

## 五、251 应该做什么（方向已改）

不再做异步流水线。按证据排序：

1. **JNI 零拷贝**（最大头，实验 3 直指）
   `nativeReadInto` 改用 `NewDirectByteBuffer` + `GetDirectBufferAddress`，
   Kotlin 侧用 `allocateDirect` 做网络读缓冲，避免 `GetByteArrayElements` 的 pin/拷贝。
   这一步做完，实验 2 的 53.9 MB/s 才有机会在应用里兑现。
2. **读放大**（第 1 步之后才有意义）
   `aheadChunk` 1 → 4 MiB，`SmbReadWindow.blockBytes` 2 → 4 MiB。
   前提：JNI 单次 4 MiB 的耗时接近纯 C++ 的 74 ms。
3. **重新决定要不要预读**
   实验 4 表明"第二条连接"整体为负。若第 1、2 步把单条连接的吞吐做到 50 MB/s，
   预读线程存在的理由（"遮蔽同步读的长尾"）就消失了 —— 可能应该**删掉预读**，
   而不是继续优化它。

## 六、复算

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

## 七、这一轮没做到的

- 实验 3 的 B/C 段（走窗口的 64 KiB 顺序读）被 `SecurityException` 打断，见
  `../../p250/s6-link-permission/status.md`。补上权限后可以通过同一个工具复跑。
- "为什么 64 KiB 经 JNI 反而比纯 C++ 略快"没解释（可能是批读/缓存的偶然），
  但它不影响结论：**大块读经 JNI 严重退化**。
