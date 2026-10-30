# P12-A 真机能力证据（Redmi K40）

> 状态：**已落盘、已在真机复跑、已进版本库**
> 证据文件：`docs/evidence/p12a/p12a-capabilities-redmi-k40.json`（36,349 字节 / 117 条 / JSON 合法）
> 采集时间：2026-10-28 23:57 (UTC+8)
> schema：`kivo.p12a.capabilities/1`

---

## 一、这份证据证明了什么

不是"跑通了 Hello World"，而是这条链路的每一环都被真机数据验证过：

| 环节 | 验证方式 | 结果 |
| --- | --- | --- |
| NDK r30 交叉编译 C++23 | `aarch64-linux-android26-clang++` 编译 11 个 `.cxx` | ✅ 零告警 |
| Gradle + AGP 9.4.1 打包 | `assembleDebug` 40 任务 | ✅ BUILD SUCCESSFUL |
| APK 安装到 HyperOS | `pm install`（绕过 adb 安装限制） | ✅ Success |
| JNI 桥接 | Kotlin `MediaCodecInventory` → `nativeExplore` | ✅ 返回 rc=0 |
| 平台能力枚举 | Java `MediaCodecList` 全量遍历 | ✅ 117 条 / 0 条解析失败 |
| C++ 侧解析与聚合 | 去重、排序、profile/位深推导 | ✅ 117 条全解析成功 |
| 结构化证据落盘 | `/data/user/0/com.kivo.cinema/files/evidence/` | ✅ 36,349 字节 |

**关键数字**：原始 117 条 → 去重后 117 条 → **解析失败 0 条**。
`rejected_records: 0` 是这份证据可信度的核心指标——任何一条解析失败都会让后面的能力统计失真。

---

## 二、设备实测基线

| 属性 | 实测值 |
| --- | --- |
| 厂商 / 型号 | Xiaomi / M2012K11AC |
| 设备代号 | `alioth` |
| SoC | SM8250（骁龙 870，`board_platform = kona`） |
| Android | 13 / **API 33** |
| ABI | `arm64-v8a` |
| GPU | Adreno 650（`ro.hardware.vulkan = adreno`） |
| 视频解码器条目 | **77**（含编码器共 117） |
| 其中硬件解码器 | **41** |
| 其中 10bit 解码器 | **11** |
| 覆盖 mime | 8 种：`3gpp` `av01` `avc` `hevc` `mp4v-es` `mpeg2` `vp8` `vp9` |

---

## 三、硬件解码器能力矩阵（本设备真实可用部分）

### H.265 / HEVC —— 主力格式

| codec 名 | profile | 位深 | 上限 | level |
| --- | --- | --- | --- | --- |
| `OMX.qcom.video.decoder.hevc` | `hevc-main` | 8 | 8192×8192 | high-tier-6.1 |
| 同上 | `hevc-main10` | **10** | 8192×8192 | high-tier-6.1 |
| 同上 | `hevc-main10-hdr10` | **10** | 8192×8192 | high-tier-6.1 |
| 同上 | `hevc-main10-hdr10plus` | **10** | 8192×8192 | high-tier-6.1 |
| 同上 | `hevc-mainstill` | 未知 | 8192×8192 | high-tier-6.1 |
| `OMX.qcom.video.decoder.hevc.secure` | 同上五种 | 同上 | 4096×4096 | high-tier-6.1 |

**结论**：G9（10bit 保真）在 Android 端**可以真机验收**。Main10 / HDR10 / HDR10+ 三条 profile 全部有硬件路径。

### H.264 / AVC —— 兼容格式

| codec 名 | profile | 位深 | 上限 | level |
| --- | --- | --- | --- | --- |
| `OMX.qcom.video.decoder.avc` | baseline / main / high / constrained-baseline / constrained-high | 8 | 8192×8192（**报告值虚高**） | avc-6 |
| `c2.qti.avc.decoder` | 同上五种 | 8 | **4096×2304**（真实工程上限） | avc-6 |
| `OMX.qcom.video.decoder.avc.secure` | 同上五种 | 8 | 4096×4096 | avc-6 |

**注意**：OMX shim 报告 8192×8192，但同一硬件的 Codec2 组件报告 4096×2304。
`/vendor/etc/media_codecs_c2.xml` 里 `size max="4096x2304"` 也印证 4096×2304 才是真上限。
→ **能力上限不许只看一个来源**，取同一硬件各组件的最小值才安全。

### VP9

| codec 名 | profile | 位深 | 上限 |
| --- | --- | --- | --- |
| `OMX.qcom.video.decoder.vp9` | `vp9-profile0` | 8 | 8192×8192 |
| 同上 | `vp9-profile2` | **10** | 8192×8192 |
| 同上 | `vp9-unknown(4096)` / `vp9-unknown(16384)` | 未知 | 8192×8192 |

`0x1000` / `0x4000` 不在 AOSP 的 `CodecProfileLevel` 公开常量表内，是高通私有扩展。
本层**不做猜测**，原样保留数值（`vp9-unknown(4096)`），是否可用留给 decode 阶段实测。

### AV1 —— 无硬件，只有软解

| codec 名 | profile | 位深 | 上限 |
| --- | --- | --- | --- |
| `c2.android.av1.decoder` | `av1-main8` | 8 | 2048×2048 |
| 同上 | `av1-main10` | 10 | 2048×2048 |

**骁龙 870 没有 AV1 硬解**。AV1 在本机只能软解，且上限 2048×2048、8 位主路径。
→ 产品决策：AV1 不作为 Android 端首发支持格式；若要支持，必须显式提示"软解、耗电"。

### 软解能力对照（用于降级路径）

| mime | 软解 codec | 位深 | 上限 |
| --- | --- | --- | --- |
| `video/hevc` | `c2.android.hevc.decoder` | **仅 8** | 4096×4096 |
| `video/avc` | `c2.android.avc.decoder` | 8 | 4080×4080 |
| `video/x-vnd.on2.vp9` | `c2.android.vp9.decoder` | 8 / 10 | 2048×2048 |
| `video/av01` | `c2.android.av1.decoder` | 8 / 10 | 2048×2048 |

**硬约束**：10bit HEVC 只有硬件路径，软解 `c2.android.hevc.decoder` **只声明 8bit**。
→ 硬解初始化失败时**不允许**静默退到软解，否则 10bit 片源会被降成 8bit 而用户毫无感知。
→ 这正是 G9「不许静默降级」在 Android 端的第一个真实约束点。

---

## 四、三条从数据里长出来的架构铁律

这三条不是推演出来的，是被真机数据逼出来的。写进代码之前先写进文档。

### 铁律 1：**绝不按 codec 名选解码器**

`/vendor/etc/media_codecs_c2.xml` 里明确声明了：

```xml
<MediaCodec name="c2.qti.hevc.decoder" type="video/hevc">
    <Limit name="size" min="128x128" max="4096x2304" />
    ...
```

但运行时 `MediaCodecList` **根本没有枚举出 `c2.qti.hevc.decoder`**。
HEVC 硬解实际只走 `OMX.qcom.video.decoder.hevc`。

而同一个文件里的 `c2.qti.avc.decoder` **确实被枚举出来了**。

同时，`media_codecs.xml` 的 `Include` 列表里**没有** `media_codecs_c2.xml`——
它引的是 `media_codecs_google_c2*.xml`、`media_codecs_system_default*.xml`。
也就是说 vendor 的 C2 配置在本 build 上是**部分失效**的。

**推论**：XML 声明的 ≠ 运行时存在的。任何"我知道这台机器用 c2.qti.xxx 就对了"的硬编码，
在换机型、换 ROM 版本、换系统更新后必然失效。
→ **选解码器只能靠运行时枚举 + 能力匹配，名字只用于日志。**

### 铁律 2：**拒绝"Codec2 就比 OMX 新"的偏见**

本机 HEVC 硬解只有 OMX 路径。如果按"只用 c2.* 才是现代做法"来筛，
HEVC 硬解会直接归零，10bit 保真彻底没法做。

OMX 在 Android 13 + Codec2 栈上仍可能是厂商的主通路——它是 C2 组件的兼容层，不是"过时技术"。

→ 筛选条件只写能力：`hardware == true` + `bit_depth` 达标 + 分辨率/帧率达标。
**不写实现框架、不写命名前缀。**

### 铁律 3：**能力上限取同硬件各组件的最小值**

同一颗 HEVC 核，OMX shim 报 8192×8192，Codec2 组件报 4096×2304，XML 声明 4096×2304。
若信 8192，遇到 5K 片源会"以为能解"→ 初始化成功 → 首帧解码 `AVERROR_INVALIDDATA`，
表现为"黑屏但没报错"，比直接拒绝更糟。

→ 协商时按 `min(所有同硬件声明)` 判定，超限时**在打开前就明确报错**。

---

## 五、复现步骤

```bash
# 1. 构建
cd "/c/kivo video/android"
JAVA_HOME=/c/Android/jdk-17.0.13+11 ./gradlew.bat assembleDebug --no-daemon

# 2. 安装（HyperOS 会拦 adb install，必须走设备内 pm）
adb push app/build/outputs/apk/debug/app-debug.apk /data/local/tmp/kivo.apk
adb shell "pm install -r -t /data/local/tmp/kivo.apk"

# 3. 启动（App 启动即自动探测，无需交互）
adb shell "am start -n com.kivo.cinema/.ui.PlayerActivity"

# 4. 取证据
adb shell "run-as com.kivo.cinema cat files/evidence/p12a-capabilities.json > /data/local/tmp/p12a.json"
adb pull /data/local/tmp/p12a.json
```

每次改动后必须复跑，并对比 `summary` 六个数字：`117 / 117 / 0 / 77 / 41 / 11`。
数字变了就说明解析层或聚合层被改坏了——这是本阶段最强的回归护栏。

---

## 六、本次暴露并修掉的问题

| 问题 | 性质 | 处置 |
| --- | --- | --- |
| `mediacodec_inventory_record.cxx` 一个文件装了解析 / profile 命名 / 位深 / 实现分类 4 个职责 | **违反铁律** | 拆成 11 个单职责单元 |
| `mediacodec_capability_probe.cxx` 装了 JSON 转义 / 属性读取 / 聚合 / 渲染 / 落盘 5 个职责 | **违反铁律** | 同上 |
| AVC 的 Constrained 档（`0x10000` / `0x80000`）未映射，证据里出现 `unknown(65536)` | 证据可读性 | 补齐常量表 |
| MPEG-2 / VP8 / H.263 / MPEG-4 没有族名，输出 `unknown-unknown(1)` | 证据可读性 | 补齐 mime→族名映射 |
| `kCodeWriteFailed` 定义了却用不上——打开失败与写入失败无法区分 | 真实缺陷 | 改 `WriteOutcome` 三态枚举 |
| 截断重复，`total_records` 语义混乱 | 语义错误 | 拆成 `total_records`（原始）与 `unique_records`（去重后） |

---

## 七、下一步

P12-A 到此闭环。接下来 **P12-B：真实硬解 + 上屏**，要打的目标是 G1（解码吞吐）、G3（表面池）、G4（零拷贝）。

必须先定的一条：**用 Surface 直出，不做 CPU 中转**。
K40 的 1080p@120 和 4K@60 只有在 Surface 输出模式下才成立，
一旦走 `MediaCodec.getOutputImage()` 拿 ByteBuffer，性能立刻掉到软解水平。
