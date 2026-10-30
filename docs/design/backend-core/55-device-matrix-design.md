# 设备矩阵：哪些判据需要什么设备（覆盖面前置设计）

- 版本：V1.0
- 日期：2026-10-30
- 上游：`54-p12f4-status.md` §六（能力边界）、§七（P12 之后）
- 问题：P12 的结论只在一台 K40 上成立。要"打败市面播放器"，必须扩覆盖面。
  但**不是所有判据都需要真机** —— 分清这一点，成本能降一个数量级。

---

## 一、把判据拆成三类（这是全部结论的基础）

| 类别 | 依赖什么 | 换设备会变吗 | 模拟器可信吗 |
| --- | --- | --- | --- |
| **A. 平台契约** | AOSP 的解封装/格式解析逻辑 | **可能变**（厂商定制、API 版本） | **可信**（同一份 AOSP 代码） |
| **B. 硬件时序** | 真正的解码器、音频设备、GPU 合成 | **一定变**（每颗 SoC 都不同） | **完全不可信** |
| **C. 显示链路** | 面板能力、SurfaceFlinger、HDR 支持 | **一定变** | **不可信**（模拟器没有真 HDR 屏） |

具体落到 P12 的证据上：

### A 类（平台契约）—— 可以在模拟器上验证

| 判据 | 证据字段 | 为什么模拟器可信 |
| --- | --- | --- |
| 加密轨能被识别 | `source.encryption.crypto_mode` | 由 `MediaExtractor` 解析 MP4 盒子决定，与硬件无关 |
| 坏文件的 reason 正确 | `container reports 0 tracks` | 同上 |
| 无视频轨/无音轨的 reason | `no video track among N tracks` | 同上 |
| 颜色键能否读到 | `source_color_standard/transfer` | 由 extractor 从 SPS VUI 读，与解码器无关 |
| API 级别不崩 | 探针能否起来 | 这正是模拟器的强项 |

### B 类（硬件时序）—— 必须真机

| 判据 | 证据字段 | 为什么模拟器无效 |
| --- | --- | --- |
| 音画同步 | `av_drift.p99` | 模拟器音频是虚拟设备，时钟不真实 |
| 零丢帧 | `frames_dropped_late` | 模拟器软解性能与硬解完全不同量级 |
| seek 首帧时延 | `seek.total_us` | 同上 |
| 上屏时刻 | `p12d5` 的 delta | 模拟器无真实 VSYNC/合成器 |
| 长稳内存 | soak 的 PSS | 模拟器进程模型不同 |
| 硬解零拷贝 | `output_target` | **模拟器根本没有硬解器** |

### C 类（显示链路）—— 必须真机（且必须是 HDR 屏）

| 判据 | 证据字段 |
| --- | --- |
| 合成器认 dataspace | SF 图层 `dataspace=BT2020_ITU_PQ` |
| 显示状态切换 | `dynamic_range=HDR` |
| 位深协商 | `color-format` |

---

## 二、结论：模拟器能扩 API，扩不了 SoC

| 维度 | 模拟器 | 真机 |
| --- | --- | --- |
| **API 级别**（26/28/30/33/34） | ✅ **能，而且便宜** | 要买对应版本机器 |
| **ABI**（arm64 / x86_64） | ✅ 能（x86_64） | arm64 才是真实目标 |
| **SoC / 厂商定制** | ❌ 不能（永远是 AOSP 行为） | ✅ 唯一途径 |
| **硬件编解码** | ❌ 不能（软解） | ✅ 唯一途径 |
| **音频时钟** | ❌ 不能（虚拟设备） | ✅ 唯一途径 |
| **HDR 显示** | ❌ 不能 | ✅ 唯一途径 |

**所以正确的用法是分工，不是替代**：

- 模拟器负责回答"**这段代码在 API 26 上会不会崩、extractor 契约在低版本是否一致**"
- 真机负责回答"**这颗 SoC 上跑得怎么样**"

我们已经因为 API 级别踩过两次坑，都是真机上发现的，代价不小：

1. `ANativeWindow_setBuffersDataSpace` 在 `libnativewindow.so` 而不是 `libandroid.so`（E2）
2. `AMediaFormat_getBuffer` 是 API 28，而 minSdk 是 26（F3B，读 `crypto-key` 时放弃）

这两条如果能在模拟器矩阵上提前跑一遍，第一次就不会在真机上卡住。

---

## 三、真机的来源（按成本排序）

| 方案 | 成本 | 覆盖 | 适合 |
| --- | --- | --- | --- |
| **手头旧机**（自己/同事的备用机） | 0 | 通常 1–3 台，SoC 各异 | **第一优先**，立刻能做 |
| **云真机**（Firebase Test Lab / 阿里云 / 腾讯云移动测试 / AWS Device Farm） | 按分钟计费，有免费额度 | 几十到上百型号 | 需要"一次覆盖很多机型"时 |
| **二手采购**（闲鱼红米/一加旧款） | 200–600 元/台 | 可控，能长期占用 | 需要反复回归时 |
| **厂商合作/内测机** | 时间成本 | 新机型 | 后期 |

**建议的最小矩阵（3 台）**：

| 目标 | 为什么选它 |
| --- | --- |
| 高通中端（已有 K40 / SM8250） | 当前基线 |
| 联发科中端（天玑 800/900 系列） | MediaCodec 实现差异最大的一支 |
| 麒麟/Exynos 或较老机型（API 26–28） | 验证 minSdk 边界 + 老 extractor 行为 |

---

## 四、具体动作（模拟器部分，**实测卡在硬件加速**）

### 已完成的（2026-10-30 实测）

| 步骤 | 结果 |
| --- | --- |
| 装 `emulator` + `system-images;android-33;google_apis;x86_64` | ✅ 完成 |
| 建 AVD `kivo-api33`（pixel_5） | ✅ 完成 |
| 编 **x86_64 探针**（`-DANDROID_PLATFORM=android-26`） | ✅ 通过 —— 说明 **API 26 目标下编译期没有依赖 API 28 的符号** |
| 启动模拟器 | ❌ **失败** |

失败原文：

```
ERROR | x86_64 emulation currently requires hardware acceleration!
CPU acceleration status: Unable to open AEHD device: ERROR_ACCESS_DENIED
```

有意思的是它先说 `Ok: Hypervisor compatibility ... are met`，再说打不开设备 ——
探测能过、实际用不了。本机 `HypervisorPresent = False`，`aehd`/`gvm` 服务都不存在，
所以是**驱动没装**，不是权限问题。

### 本机条件（已核实）

- Windows 11 **专业版** build 22621（支持 WHPX）
- CPU `i3-10100`，虚拟化固件已启用，SLAT 支持，24 GB 内存 ✅
- 驱动包已下载到 `C:\Android\sdk\extras\google\Android_Emulator_Hypervisor_Driver\`

### 两条路都走不通（实测，2026-10-30）

**路线 A：x86_64 镜像 + 硬件加速** —— 需要管理员权限

```
ERROR | x86_64 emulation currently requires hardware acceleration!
CPU acceleration status: Unable to open AEHD device: ERROR_ACCESS_DENIED
```

`x86_64 emulation currently requires hardware acceleration!` 是硬门槛。
装 AEHD 驱动或启用 WHPX 都要写内核驱动/系统功能 —— **普通用户权限做不到**。
把 SDK 移到别的盘**解决不了这个问题**：加速能力是 CPU + 内核驱动的属性，与磁盘位置无关。

**路线 B：ARM 镜像 + 纯软件模拟** —— Google 已移除支持

```
FATAL | QEMU2 emulator does not support arm64 CPU architecture
```

emulator **37.2.13** 已经不再带 `qemu-system-aarch64`。老版本（30.x/31.x）可能还能跑，
但要手工下载旧包、且 ARM 在 x86 上全模拟的开机时间是几十分钟级 —— 成本远超收益。

**结论：本机（无管理员权限）无法运行 Android 模拟器。** 这不是配置问题，是权限与上游支持的双重限制。

### 但这条路上仍然拿到了两件事（零权限成本）

1. **静态 API 兼容性已证明**：`-DANDROID_PLATFORM=android-26` 下，x86_64 探针与
   arm64 全套都**链接通过**。这不是"看起来没问题"—— NDK 在 android-26 目标下用的是
   26 版的 stub 库，**代码若直接引用了 API 28 的符号，链接就会失败**。
   链接成功因此等价于"没有静态引用 28+ 符号"。这正好覆盖我们已经踩过的两个坑
   （`ANativeWindow_setBuffersDataSpace`、`AMediaFormat_getBuffer`）。
2. **验证脚本已就位**：`tools/check_emu_contract.py` —— 一旦有可用的模拟器或第二台设备，
   一条命令就能跑完"提取器契约"对比，输出与真机基线的差异表。
   它的证据固定写入 `docs/evidence/emulator/`，**总收口物理排除该目录**
   （B/C 类判据在非真环境下测出来的数字是假的，混进去会让"全绿"变成假的）。

跑法：

```bash
# 1. 建 AVD（每个 API 一个）
avdmanager create avd -n kivo-api33 -k "system-images;android-33;google_apis;x86_64" -d pixel_5

# 2. 启动（无窗口，省资源）
emulator -avd kivo-api33 -no-window -no-audio -no-boot-anim -gpu swiftshader_indirect &

# 3. 等开机
adb wait-for-device shell 'while [ "$(getprop sys.boot_completed)" != "1" ]; do sleep 1; done'

# 4. 推探针与片源，跑契约检查
adb push C:/tmp/kivo-x86/kivo_hdr_probe /data/local/tmp/
adb push tests/real/negative_cenc_h264.mp4 /data/local/tmp/cenc.mp4
adb shell "/data/local/tmp/kivo_hdr_probe /data/local/tmp/cenc.mp4"
```

**只跑 A 类判据**：`crypto-mode` 是否存在、错误 reason 是否正确、探针能否启动。
**不跑 B/C 类** —— 在模拟器上跑那些得到的数字没有意义，写进证据还会污染收口。

---

## 五、风险与边界

1. **WHPX/AEHD 未启用时模拟器起不来 —— 已实测确认。** 本机装了 emulator、建了 AVD、
   x86_64 探针也编过了，但启动时 `Unable to open AEHD device: ERROR_ACCESS_DENIED`。
   `x86_64 emulation currently requires hardware acceleration!` 是硬门槛，绕不过去。
   解法见 §四（两条，都需要管理员权限）。
   **ARM 镜像不是替代方案**：API 28 之后 Google 不再提供 x86 主机可用的 arm64 image；
   即使能跑，QEMU 全模拟的开机时间也是十分钟级。
2. **模拟器证据必须打标记。** 建议文件名带 `-emu`，并在收口脚本里**排除**
   —— 否则 B 类数字混进 `docs/evidence/` 会让"全绿"变成假的。
3. **x86_64 不是交付目标。** 真机是 arm64。x86_64 只用于契约验证，不进性能结论。
4. **API 33 模拟器 ≠ API 33 真机。** 厂商定制（MIUI 把 `ENCRYPTED` 位抹掉这类）模拟器永远复现不了。
   所以模拟器过了不代表真机过。

---

## 六、这条路能解决多少"打败市面播放器"的问题

诚实回答：**能解决一小部分，但它是唯一便宜的那部分。**

| 差距 | 模拟器有帮助吗 |
| --- | --- |
| API 兼容性崩溃 | ✅ 帮助很大 |
| extractor 契约一致性 | ✅ 有帮助 |
| 4K/HEVC/60fps 性能 | ❌ 完全没有 |
| 多 SoC 行为差异 | ❌ 完全没有 |
| DRM / 字幕 / 倍速 / 网络流 | ⚠️ 逻辑可测，效果不可测 |

真正的短板（4K、多 SoC）只能靠真机。模拟器的价值是把"低级错误"挡在真机之前，
让真机时间花在真问题上。
