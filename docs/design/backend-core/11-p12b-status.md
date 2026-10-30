# P12-B 真实硬解与零拷贝上屏

> 状态：**已闭环，真机验证通过**
> 证据：`docs/evidence/p12b/{avc-1080p8,hevc-1080p10,hevc-4k10}.json`
> 设备：Redmi K40（alioth / SM8250 / Android 13 API 33）
> 日期：2026-10-29

---

## 一、做成了什么

从「有能力清单」到「真的把视频放出来」：

```
文件 → MediaExtractor 解封装 → MediaCodec 同步解码 → ANativeWindow → Surface 合成
                                        ↑
                                  全程不经 CPU
```

**零拷贝成立**：`AMediaCodec_configure` 直接绑定 `ANativeWindow`，
解码输出由硬件写入图形缓冲，`releaseOutputBuffer(idx, true)` 只是把这块缓冲交给合成器。
像素数据从未落到 Java 堆或 CPU 可读内存。

这条路径的意义：K40 的 4K@60、1080p@120 只在这个模式下成立。
一旦改成 `getOutputBuffer` 拿 ByteBuffer，性能立刻塌到软解水平。

---

## 二、实测数据

| 片源 | 解码器 | 帧数 | 墙钟 | fps | 相对实时 | 首帧延迟 |
| --- | --- | --- | --- | --- | --- | --- |
| H.264 1080p 8bit | `OMX.qcom.video.decoder.avc` | 180/180 | 1.060 s | 169.8 | **5.66×** | 103.7 ms |
| HEVC 1080p **10bit** | `OMX.qcom.video.decoder.hevc` | 180/180 | 1.027 s | 175.3 | **5.84×** | 90.8 ms |
| HEVC 4K **10bit** | `OMX.qcom.video.decoder.hevc` | 120/120 | 0.942 s | 127.4 | **4.25×** | 113.0 ms |

### 零丢帧不变量

三个片源全部满足：

```
samples_submitted == frames_rendered == 轨道总帧数
samples_rejected == 0
completed == true, error == ""
```

这不是巧合，是设计约束的结果：输入侧用「挂起包 + 重试」而不是「失败就丢」。
`dequeueInputBuffer` 返回 TRY_AGAIN 时包留在 `pending`，绝不会因为输入队列满而丢数据。
**以后任何改动都必须保持这条不变量**——它比任何性能数字都更能说明正确性。

### G9（十位保真）在 Android 端已可真机验收

HEVC Main10 / 4K 10bit 全部走硬件解码器 `OMX.qcom.video.decoder.hevc`，
输出直送 Surface，中间没有任何 8bit 转换环节。

---

## 三、四个卡点，每个都改变了实现

### 卡点 1：NDK 的 codec 枚举 API 要 API 36，我们用不了

NDK r30 确实新增了 `AMediaCodecStore`（`AMediaCodecStore_findNextDecoderForFormat` 等），
但每个函数都标 `__INTRODUCED_IN(36)`——Android 16。

而 minSdk 是 26，测试机是 API 33。

**结论**：枚举与能力查询**仍必须走 Java 层 `MediaCodecList`**。
P12-A 的判断没有过时，只是把「NDK 永远没有」修正为「NDK 有了，但要等 API 36 普及」。
这条约束写死在家族树里，避免以后有人再翻一遍。

### 卡点 2：`AMediaExtractor_setDataSource(路径)` 返回 MALFORMED

传 `/storage/emulated/0/Android/data/com.kivo.cinema/files/media/a.mp4` 得到
`AMEDIA_ERROR_MALFORMED(-10002)`。

排查过程：
1. 校验设备上文件 md5 与宿主完全一致 → 文件没坏
2. `ffprobe` 确认编码正常（H.264 High 1080p yuv420p）→ 内容没坏
3. 换 `AMediaExtractor_setDataSourceFd(extractor, fd, 0, length)` → **一次成功**

**结论**：NDK 的解封装入口在生产代码里只该用 fd。
它同时解决了三个问题：路径语义不明确、分区存储限制、以及「路径能不能读」的模糊失败。
路径从此只作为诊断用的显示名存在。

### 卡点 3：这台机器的 logcat 对 adb 完全不可见

`adb logcat -d` 返回 **0 行**——不是过滤问题，是整个 buffer 取不到。
HyperOS 把 logd 对 adb 关掉了。

这件事的后果比想象中大：所有 `__android_log_print` 都等于写进黑洞。
如果只靠日志，失败时我们「什么都看不到」。

**处置**：
- 失败原因通过句柄回传（`nativeSessionLastError`），显示在屏幕上
- 失败同时落盘成 `p12b-failure.json`（`kivo.p12b.failure/1`）
- 成功与否都以 JSON 证据为准，不以日志为准

这恰好强化了 G10（必须有证据产物）。**在没有日志的平台上，证据文件不是可选项，是唯一的观测手段。**

### 卡点 4：一堆 NDK 接口是 API 28 的，minSdk 是 26

| 接口 | 引入版本 | 处置 |
| --- | --- | --- |
| `AMediaCodec_getName` / `releaseName` | 28 | `dlsym` 运行时解析，取不到就返回空串 |
| `AMediaFormat_copy` | 29 | **不用**——configure 不接管格式所有权，直接用轨道格式即可 |
| `AMEDIAFORMAT_KEY_CSD_0` | 28 | 用字面量 `"csd-0"`（键名自 API 21 稳定） |
| `AMEDIAFORMAT_KEY_ROTATION` | 28 | 用字面量 `"rotation"` |

**没有为此抬高 minSdk。** 三个都是「能用就多用一点，不能用就优雅缺省」，不该为了它们把 8.0/9.0 的设备排除掉。

`AMediaFormat_copy` 那条特别值得记：原本打算写一个「克隆格式再改」的 builder，
查证后发现 minSdk 26 上根本没有拷贝接口——**差点写出一段在 API 26/27 上必然崩的代码**。
最终把那个单元改成实打实需要的「格式校验 + 可读描述」，而不是假装能克隆。

---

## 四、瓶颈在哪：一个反直觉的数字

看这两行：

| 片源 | 像素量 | 每帧耗时 |
| --- | --- | --- |
| HEVC 1080p 10bit | 2.07 MP | 5.71 ms |
| HEVC 4K 10bit | 8.29 MP | **7.85 ms** |

像素翻 **4 倍**，每帧只慢了 **1.37 倍**。

如果瓶颈在解码（像素吞吐），4K 应该慢接近 4 倍。实际不是。
说明当前限制来自**每帧固定开销**：送包/收帧的往返、`releaseOutputBuffer` 与 BufferQueue 的交互。

已经做的一轮优化（把「一包一收」改成「批量灌 8 包再统一收」）：

| 片源 | 优化前 | 优化后 | 变化 |
| --- | --- | --- | --- |
| HEVC 1080p 10bit | 144.3 fps | 175.3 fps | **+21%** |
| HEVC 4K 10bit | 114.0 fps | 127.4 fps | **+12%** |

同一轮还去掉了热路径上的每帧加锁与 `std::string` 复制——
原来每帧都要锁一次互斥量并复制两个字符串，等于自己给解码器挂了个秤砣。
改为无锁原子计数。

**但这条线索没走完。** 继续压固定开销的方向：
1. 进一步加大批量、按平台输入槽数量自适应
2. 收帧与送包分线程（生产者/消费者）
3. 用 `releaseOutputBufferAtTime` 做精确呈现时刻，交给 SurfaceFlinger 排程

这三条属于呈现节拍问题，和 A/V 同步是同一件事，放到 **P12-C** 一起做。

---

## 五、家族树落位（实际实现）

```
backends/mediacodec/
├── device/          13 个单元（P12-A 能力证据链路，未改动）
├── source/
│   ├── extract_outcome.hpp           解封装结果码
│   ├── media_track_info.hpp          轨道静态信息（纯数据）
│   ├── sample_view.hpp               中立样本视图（含标志位语义翻译）
│   ├── mediaextractor_source.{hpp,cxx}   fd 打开 / 选轨 / 取包 / seek
├── decode/
│   ├── mediacodec_format_guard.{hpp,cxx}          配置前校验（不克隆）
│   ├── mediacodec_codec_name_resolver.{hpp,cxx}   API 28 符号的运行时降级
│   ├── mediacodec_video_decoder.{hpp,cxx}         送包 / 收帧 / EOS / flush
├── present/
│   └── surface_window_binder.{hpp,cxx}            ANativeWindow RAII
└── session/
    ├── decode_stats.hpp                  统计事实（纯数据）
    ├── decode_stats_renderer.{hpp,cxx}   统计 → JSON（纯函数）
    └── decode_session.{hpp,cxx}          线程 + 流水线编排

android/app/src/main/cpp/jni/
├── jni_player_bridge.cxx     全部门口的转发（薄）
├── jni_session_handle.{hpp,cxx}  句柄：窗口与会话的生命周期绑定
└── jni_string_convert.{hpp,cxx}  字符串转换（唯一转换点）
```

**一个刻意的设计**：`SessionHandle` 同时持有 `SurfaceWindowBinder` 和 `DecodeSession`。
原因是从 Java Surface 换来的 `ANativeWindow` 必须在 Java 侧 Surface 消亡前释放，
而解码器又依赖它。绑在同一个句柄上，释放顺序只有一处（先停会话、再放窗口），
不会出现「会话还在跑但窗口已被回收」的悬空指针。

---

## 六、已知未做（明确记录，不算遗漏）

| 项 | 归属 |
| --- | --- |
| A/V 同步、按节拍呈现、丢帧决策 | **P12-C**（C5–C7，见 `docs/handoff/TASKS.md`） |
| 音频输出（AAudio）+ 音频主时钟 | **P12-C**（C1–C4） |
| seek 后重同步（`seek_to` 已实现但未接入会话） | P12-F（F1–F2） |
| 加密轨道（MediaCrypto） | 显式拒绝并报错，未实现 |
| 多实例并发、Surface 重建 | P12-D |
| 软解回退 | 刻意不做：10bit 软解只声明 8bit，静默回退等于偷偷降质 |

---

## 七、复现

```bash
# 1. 构建安装
cd "/c/kivo video/android"
JAVA_HOME=/c/Android/jdk-17.0.13+11 ./gradlew.bat assembleDebug --no-daemon
adb push app/build/outputs/apk/debug/app-debug.apk /data/local/tmp/kivo.apk
adb shell "pm install -r -t /data/local/tmp/kivo.apk"

# 2. 放片源（文件名排序取第一个）
adb push clip.mp4 /sdcard/Android/data/com.kivo.cinema/files/media/a_clip.mp4

# 3. 启动（自动探测 → 自动播放 → 播完写统计）
adb shell "am start -n com.kivo.cinema/.ui.PlayerActivity"

# 4. 取证据
adb shell "run-as com.kivo.cinema cat files/evidence/p12b-decode.json"
```

屏幕左上角实时显示 `codec / fps / frames / speed / first_frame / completed`，
用于现场核对；滚动完毕后统计自动落盘。
