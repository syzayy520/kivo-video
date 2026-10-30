# 06 - Android 工具链与设备基线（P12 前置）

> 状态：工具链已就绪并完成真机闭环验证
> 建立日期：2026-10-28
> 适用阶段：P12（Android 端后端核心）

---

## 一、工具链安装清单

全部安装在 `C:\Android`，**无需管理员权限**，不污染系统目录。

| 组件 | 版本 | 路径 | 用途 |
| --- | --- | --- | --- |
| JDK (Temurin) | 17.0.13+11 | `C:\Android\jdk-17.0.13+11` | Gradle / AGP 运行时 |
| Android SDK | cmdline-tools 11076708 | `C:\Android\sdk` | SDK 管理器 |
| Platform-Tools | 37.0.1 | `C:\Android\sdk\platform-tools` | adb / fastboot |
| Build-Tools | **37.0.0** | `C:\Android\sdk\build-tools\37.0.0` | aapt2 / d8 / zipalign |
| Platform | **android-37.0**（Android 17） | `C:\Android\sdk\platforms\android-37.0` | 编译期 API |
| NDK | **30.0.16248370 (r30)** | `C:\Android\sdk\ndk\30.0.16248370` | C++ 交叉编译 |
| CMake (SDK) | 3.22.1 | `C:\Android\sdk\cmake\3.22.1` | Gradle externalNativeBuild |
| CMake (系统) | 独立安装 | `C:\Program Files\CMake\bin` | 桌面构建复用 |
| Gradle | **9.6.1** | `C:\Android\gradle-9.6.1` | 构建系统 |
| AGP | **9.4.1** | 由 Gradle 拉取 | Android 构建插件（内置 Kotlin） |
| Ninja | winget 安装 | `%LOCALAPPDATA%\Microsoft\WinGet\...\ninja.exe` | 构建加速 |

版本基线说明：以 2026-10 官方主线为准（AGP 9.4 / Gradle 9.6 / Kotlin 2.3 / compileSdk 37 / NDK r30）。
早期安装的 r27c + Gradle 8.11.1 已升级，仅保留在磁盘上未删除。

> **AGP 9 的三处硬变更**（踩过才知道，写下来避免重犯）：
> 1. 内置 Kotlin 编译，**不能再 apply `org.jetbrains.kotlin.android`**，否则 duplicate extension 报错；
> 2. 新 DSL 生效，`kotlinOptions { jvmTarget }` 失效，改用 `android { kotlin { compilerOptions { ... } } }`；
> 3. Gradle 最低版本随小版本抬升，AGP 9.4 要求 **Gradle ≥ 9.6.0**。

SDK 总体积：**约 5.4 GB**（含 r27c 与 r30 两套 NDK）。


---

## 二、环境变量（用户级，已写入注册表）

| 变量 | 值 |
| --- | --- |
| `JAVA_HOME` | `C:\Android\jdk-17.0.13+11` |
| `ANDROID_HOME` | `C:\Android\sdk` |
| `ANDROID_SDK_ROOT` | `C:\Android\sdk` |
| `Path` 追加 | jdk\bin、platform-tools、cmdline-tools\latest\bin |

> 注意：环境变量写入后，**已打开的终端/IDE 需重启**才能读到。

---

## 三、真机闭环验证（已完成）

验证方式：NDK 交叉编译一个 C++23 arm64 程序，adb push 到 `/data/local/tmp` 执行。

```bash
aarch64-linux-android23-clang++ -std=c++23 -O2 -static-libstdc++ hello.cpp -o hello_arm64
adb push hello_arm64 /data/local/tmp/kivo_probe
adb shell chmod 755 /data/local/tmp/kivo_probe
adb shell /data/local/tmp/kivo_probe
```

真机输出：

```
kivo-backend probe: OK
sdk=23 abi=arm64-v8a ptr=8
```

结论：**编译 → 部署 → 运行 全链路打通**，64 位指针确认。

---

## 四、主测试机能力基线（Redmi K40，已实测）

序列号 `b6e950ee`，型号 `M2012K11AC` / `alioth`。

| 属性 | 实测值 | 评价 |
| --- | --- | --- |
| 型号 | Xiaomi M2012K11AC / `alioth`（Redmi K40） | 主开发机 |
| SoC | `ro.board.platform=kona`，`ro.soc.model=SM8250`（骁龙 870） | 强 |
| Android | **13 / API 33** | 满足全部现代 API |
| ABI | `arm64-v8a`（`zygote64_32`） | 64 位 |
| RAM | 11,783,376 kB ≈ **11.2 GB** | 压力测试可做 |
| 屏幕 | 1080×2400 @ 440dpi，60 / 90 / 120 Hz 三档可变 | 高刷可测 |
| GPU | Adreno 650，OpenGL ES **3.2**（196610），Vulkan 可用（`ro.hardware.vulkan=adreno`） | 双后端可选 |
| HDR | `mSupportedHdrTypes=[1,2,3,4]` = HDR10 / HDR10+ / HLG / Dolby Vision | **HDR 可验证** |
| 色域 | `supportedColorModes=[0,7,9]` = sRGB / Display-P3 / BT2020 | 广色域可测 |
| 亮度 | max 420 nits，avg 210 nits，min 0.32 nits | 够用 |
| H.264 解码 | `c2.qti.avc.decoder`（Codec2 硬件） | 有 |
| HEVC 解码 | `c2.qti.hevc.decoder`（Codec2 硬件） | 有 |
| VP9 解码 | `c2.qti.vp9.decoder` | 有 |
| 解码上限 | **4096×2304 / 120 Mbps**，frame-rate 1–480 | 4K 可测 |
| 性能点 | 4K@60、1080p@240、720p@480 | 余量充足 |
| 并发实例 | HEVC 最多 16 | 多路可测 |
| 编码器 | `c2.qti.avc.encoder` / `c2.qti.hevc.encoder` | 录制可测 |

实测运行我们的 arm64 探针：

```
kivo-backend probe: OK
sdk=33 abi=arm64-v8a ptr=8
```

> 说明：vendor XML 不含 `profile-levels`（Android 10+ 由 Codec2 组件运行时提供），
> HEVC Main10 是否可用需用 `MediaCodecInfo` 运行时查询确认，列为 P12 首项任务。

---

## 四之二、下限验证机基线（Coolpad C106，已退役为主机）

| 属性 | 实测值 | 评价 |
| --- | --- | --- |
| 型号 | Coolpad C106（`cool_c1`） | 2016 年机型 |
| SoC | `msm8952`，GPU 报 Adreno 510 | 骁龙 650/652 级 |
| Android | 6.0.1 / **API 23** | **过低** |
| ABI | `arm64-v8a`（zygote64_32） | 可跑 64 位 |
| RAM | 2.9 GB（实测空闲仅 55 MB） | 紧绷 |
| 屏幕 | 1080×1920 @ 480dpi | 够用 |
| GPU API | OpenGL ES 3.1（Adreno 510 驱动 V@139） | 无 Vulkan |
| 硬解上限 | **1920×1088 / 50 Mbps**（H.264 与 HEVC 同） | **不支持 4K** |

| H.264 解码器 | `OMX.qcom.video.decoder.avc` | 有 |
| HEVC 解码器 | `OMX.qcom.video.decoder.hevc` | 有，8bit 为主 |
| 其他解码 | VP8 / MPEG2 / MPEG4 / H263 / WMV / VC1 / DivX | 有 |
| VP9 | 无 | 缺失 |
| Vulkan | 无（`ro.hardware.vulkan` 为空） | 缺失 |
| 并发解码实例 | max 8 | 正常 |

---

## 五、为什么不拿 API 23 做主开发基线

| 缺失能力 | 需要 API | 对本项目的影响 |
| --- | --- | --- |
| `AHardwareBuffer` | 26 | **零拷贝上屏地基缺失**，只能退化到 SurfaceTexture + GL_TEXTURE_EXTERNAL_OES |
| Vulkan | 24（驱动） | 渲染后端选型被封死，只剩 GLES |
| `MediaCodecInfo.isHardwareAccelerated()` | 29 | 无法在运行时判定软/硬解，违反"禁止静默降级"条款 |
| `ImageReader` PRIVATE + 高精度时间戳 | 23~29 演进 | 帧时间戳精度不足，A/V 同步误差大 |
| HDR / 10bit 输出 | 26+ | P010 十位保真在 Android 端无法验证 |
| 4K 解码 | 硬件 | 上限锁死 1080p |

**结论：API 23 只能作为「下限兼容验证机」，不能作为主开发目标。**

它的价值在于：如果架构在 API 23 + 弱硬件上仍能不崩溃、且降级路径显式可观测，说明分层设计足够健壮。这正好对应 `03-goals.md` 中「禁止静默降级」的验收要求。

---

## 六、换机决策依据（已执行：2026-10-28 换为 K40）

| 维度 | Coolpad C106 | Redmi K40 | 差距 |
| --- | --- | --- | --- |
| SoC | 骁龙 650/652 | 骁龙 870 | CPU 约 4× |
| GPU | Adreno 510 | Adreno 650 | 约 5× |
| Android | 6.0.1 / API 23 | 11 起 / API 30+ | 跨 7 代 |
| 解码上限 | 1920×1088 | 4K@60 | 质变 |
| HEVC | 8bit | Main10 / HDR10 | 10bit 可验证 |
| Vulkan | 无 | 1.1 | 上屏架构可选 |
| AHardwareBuffer | 无 | 有 | 零拷贝可做 |
| RAM | 2.9 GB | 8/12 GB | 压力测试可做 |

K40 能覆盖本项目全部 Android 端目标（G3 表面池零泄漏、G4 零拷贝上屏、G5 A/V 同步、G9 10bit 保真）。

---

## 七、设备角色分工（已落地）

| 角色 | 设备 | 用途 |
| --- | --- | --- |
| 主开发机 | Redmi K40（`b6e950ee`） | 全特性开发、性能基准、目标验收 |
| 下限验证机 | Coolpad C106 | API 23 兼容性、弱硬件降级路径、极端内存压力 |

---

## 八、常用命令速查

```powershell
# 设备
adb devices -l
adb shell getprop ro.build.version.sdk
adb logcat -s KivoVideo:* KivoAudio:*

# SDK 管理
sdkmanager --list_installed
sdkmanager "platforms;android-34"      # 如需其他 API

# 构建
gradle assembleDebug
gradle :backend:externalNativeBuildDebug
```

> bash 环境注意：`adb push` 的目标路径会被 MSYS 转换，需用 PowerShell 执行，或设 `MSYS_NO_PATHCONV=1` 且本地路径写成 `C:/...`。

---

## 九、待办

- [x] 换机到 Redmi K40，重新采集能力基线（已完成，见第四节）
- [x] 用 `MediaCodecInfo` 运行时查询 HEVC Main10 / HDR profile，产出机器可读能力矩阵（P12-A 已闭环）
- [x] 建立 `android/` 工程骨架，落实家族树分层
- [x] 首次 `gradle assembleDebug` 全链路构建验证
- [x] 真实硬解 + 零拷贝上屏真机验证（P12-B 已闭环）

> 后续任务不在本文维护，见 `docs/handoff/TASKS.md`。
