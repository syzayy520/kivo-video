# P12-C2 · SPSC 无锁音频环形缓冲

> 状态：**已闭环** —— 9 项单测双平台全过（x86_64 + arm64 真机），并已接入真实 AAudio 会话联调
> 证据：`docs/evidence/p12c/ring-buffer-tests-arm64-k40.txt`、
> `p12c2-ring-realtime-redmi-k40.json`、`p12c2-ring-underrun-halfspeed-redmi-k40.json`
> 设备：Redmi K40（alioth / SM8250 / Android 13 API 33）
> 日期：2026-10-29

---

## 一、做成了什么

```
解码线程（未来：AMediaCodec）
   │  write()  短写即报欠载，不阻塞不覆盖
   ▼
SpscAudioRingBuffer          ← core/audio/，零平台依赖，三端共用
   │  read()
   ▼
RingBufferSampleSource       ← backends/aaudio/buffer/，薄适配器
   │  SampleSource::render()
   ▼
AaudioOutputSession ──回调──→ AAudio ──→ 扬声器
```

三层各自的理由：

| 层 | 位置 | 为什么在这 |
| --- | --- | --- |
| 缓冲 | `core/audio/spsc_audio_ring_buffer` | 零平台依赖，iOS 的 AudioUnit 回调同样需要它 |
| 适配器 | `backends/aaudio/buffer/ring_buffer_sample_source` | 依赖只能从后端指向核心，不能反向 |
| 会话 | `backends/aaudio/session/`（C1 产物） | **一行都没改** —— C1 留的注入点生效了 |

**落位修正**：C2 卡原本写的是 `backends/aaudio/buffer/audio_ring_buffer`。
实现前改了：缓冲本体移入 `core/audio/`，`backends/aaudio/buffer/` 只放适配器。
理由是铁律「可移植逻辑归 core」——一个不含任何 Android 类型的组件放在 `backends/aaudio/` 下，
iOS 端要么复制一份、要么反向依赖 Android 目录，两条都是死路。台账与家族树已同步。

---

## 二、单元测试（R1–R9）

| 编号 | 内容 | 断言要点 |
| --- | --- | --- |
| R1 | 基本写读 | 容量 64 写 100 → **必须短写 64**，不静默丢 |
| R2 | 环绕顺序 | 8 轮"写满→读空"+ 错位 7 帧，强制索引多次跨过物理末尾 |
| R3 | 写满短写 | 满时返回 0；只剩 16 帧空间时写 100 返回 16；不阻塞 |
| R4 | 空读不改输出 | 空读返回 0 且输出缓冲逐元素未变；空指针/零帧返回 0 而非崩溃 |
| R5 | 不覆盖未读 | 满后狂写 8 帧全被拒，读回的旧数据完整 |
| R6 | **并发 100 万帧** | 生产者块 100 / 消费者块 37（故意不同粒度），逐样本校验序号 |
| R7 | 热路径零分配 | 1 万轮读写期间全局 `operator new` 调用 **= 0** |
| R8 | 容量取整 | 请求 100 → 128；非 2 的幂/非法参数抛 `invalid_argument` |
| R9 | 立体声不串道 | L=i、R=−i，逐帧校验左右声道未错位 |

结果：

| 平台 | 构建 | 用例 | 断言 | 连跑 |
| --- | --- | --- | --- | --- |
| Windows x86_64 | MSVC 19.51 `/W4` Debug | 9/9 PASS | 433 项 0 失败 | CTest #237 通过 |
| Windows x86_64 | MSVC 19.51 **Release**（有优化） | 9/9 PASS | 433 项 0 失败 | **20/20 通过** |
| **Redmi K40 arm64** | **NDK r30 clang `-O2`** | 9/9 PASS | 433 项 0 失败 | **20/20 通过** |

**为什么一定要在 arm64 真机上跑**：x86 是强内存模型（TSO），
少写一个 `release`、把 `acquire` 写成 `relaxed`，在 x86 上大概率照样通过；
ARM 是弱内存模型，同样的错误会真实暴露。只测 x86 等于没测这条路径。

**为什么 Release 也要跑**：Debug 不优化，编译器不会把读写搬到索引发布之后；
优化器一旦重排，错误的屏障才会现形。

---

## 三、真机联调：把"计数器有效"也变成证据

单测通过只说明缓冲本身对，不说明**接到真实音频流上之后欠载能被如实记下来**。
所以我加了 `ring` 供给模式（`ToneRingProducer` 模拟解码线程，按倍速灌数据），
并刻意做了一次**负向实验**：

| 组 | 供给倍速 | 消费帧数 | 供给帧数 | `frames_silenced` | `xrun` | 结论 |
| --- | --- | --- | --- | --- | --- | --- |
| A | 1.0（实时） | 242,496 | 258,496 | **0** | 0 | 正常供给零欠载 |
| B | **0.5（半速）** | 242,496 | 138,400 | **104,192** | 0 | 供不上必须被记下来 |
| C | direct（C1 基线回归） | 146,112 | — | **0** | 0 | 加缓冲层没弄坏原路径 |

B 组的意义在于**恒等式闭合**：

```
frames_silenced = frames_rendered − frames_pushed
        104,192 ≈ 242,496 − 138,400 = 104,096    （差 96 帧 = 结束时缓冲里未消费的余量）
```

若这个计数器是个永远输出 0 的摆设，B 组就不可能得到这个数。
**A 与 B 必须成对存在**：只有 A，无法排除"计数器根本没在计"；只有 B，无法排除"一直欠载"。

---

## 四、这轮抓到的两个错误，都在探针自己身上

### 错误 1：预灌放在了 `start()` 之后 → 50ms 假欠载

第一版顺序是 `open → start → prefill → 起供给线程`。
音频流一开始工作，环形缓冲还是空的，前 13 个 burst（2496 帧 = 52 ms）被如实计成欠载。
数字**每次都是 2496**，正好 13×192，这个"过于整齐"才是破案线索——
真正的抖动不会每次一模一样。

改成 `open → prefill → 起供给 → start`，`silenced` 立刻归零。

> 附带结论：`frames_silenced` 抓到了我自己的错，这比"它显示 0"更能证明它有效。

### 错误 2：`sleep_for` 的相对睡眠会累积漂移

第一版供给线程用 `sleep_for(10ms)`。每次唤醒实际略长于 10ms，
误差一轮轮累加，5 秒就攒出约 40ms 的供给缺口，被误记成"环形缓冲欠载"。
改成 `sleep_until(绝对截止时刻)`，单轮超时不再累积到下一轮。

> 这条对 C3 有直接约束：**真实解码线程也必须按绝对时间轴供数**，
> 不能"解码完睡 10ms"，否则音频会持续性地缓慢欠载，而且看起来像缓冲容量不够。

---

## 五、复现

```bash
# 单测（桌面）
cmake --build build/vs-debug --target kivo_p12c_ring_buffer_tests --config Release
./build/vs-debug/backend/Release/kivo_p12c_ring_buffer_tests.exe

# 单测（arm64 真机）
CXX=/c/Android/sdk/ndk/30.0.16248370/toolchains/llvm/prebuilt/windows-x86_64/bin/aarch64-linux-android26-clang++
$CXX -std=c++23 -O2 -static-libstdc++ -I backend/include_private \
  backend/src/video/video_plane/core/audio/spsc_audio_ring_buffer.cxx \
  backend/tests/video/audio_plane/p12c_ring_buffer_tests/spsc_audio_ring_buffer_tests.cpp \
  -o build/p12c/kivo_ring_buffer_tests
ADB=/c/Android/sdk/platform-tools/adb.exe
MSYS_NO_PATHCONV=1 $ADB push build/p12c/kivo_ring_buffer_tests /data/local/tmp/
MSYS_NO_PATHCONV=1 $ADB shell "chmod 755 /data/local/tmp/kivo_ring_buffer_tests && /data/local/tmp/kivo_ring_buffer_tests"

# 真机联调（探针第 3 参 = 供给模式，第 4 参 = 供给倍速）
$ADB shell "/data/local/tmp/kivo_audio_probe 5 440 ring 1.0"   # 应 silenced=0
$ADB shell "/data/local/tmp/kivo_audio_probe 5 440 ring 0.5"   # 应 silenced>0
```

> 构建踩坑：本机 Git Bash 里 `HTTP_PROXY` 与 `http_proxy` 同时存在，
> .NET 的 MSBuild 会因大小写重名报 `MSB6001`。构建前 `unset http_proxy https_proxy` 即可。

---

## 六、人耳验收

`ring 1.0` 模式在真机上出声，**用户当场确认"有声音"**（2026-10-29）。

这一条的边界要写清楚：自动化只能证明"样本从环形缓冲被取走并交给了 AAudio"
（`silenced=0`、`xrun=0`、两侧计数闭合），证不了"出口没被静音、也没走错声道"。
所以 C2 的验收口径和 C1 一致，仍是两段：**客观指标由脚本断言，是否出声由人断言**。

至此 C1 的直连路径（`./kivo_audio_probe 5 440`）与 C2 的缓冲路径
（`./kivo_audio_probe 5 440 ring 1.0`）**两条路径都有人耳证据**。

---

## 七、已知未做（归属明确）

| 项 | 归属 / 理由 |
| --- | --- |
| 多生产者 / 多消费者 | 刻意不支持。SPSC 才有无锁实现的空间，多线程队列是另一个问题（且实时线程等锁必炸） |
| 音频解码供 PCM（现在是正弦） | P12-C3 |
| 采样格式转换（S16 → Float） | P12-C3 |
| 缓冲容量的自适应调整 | 不做。固定 16384 帧（341ms）已够吸收抖动；自适应会引入自身的不稳定 |
| 首帧出声时刻的精确测量 | P12-C4（用 `AAudioStream_getTimestamp`） |
