# 302 · 4K AV1 选源闸门（301 的直接产物）

状态：**已落地并验证 15/15 PASS**
设备：K40 `b6e950ee`（SM8250）
时间：2026-11-16 03:15–03:30

## 一、为什么做这个

301 实测发现一个**会被现有选源逻辑选错**的组合：

| 版本 | 码率 | 实际表现 |
| --- | --- | --- |
| 4K AV1 10bit | **11.9 Mbps** | ❌ **10.87 fps**（dav1d 打满 CPU） |
| 1080p AV1 10bit | 约 8 Mbps | ✅ 60.44 fps |
| 4K HEVC HDR | 15–20 Mbps | ✅ 59.47 fps（硬解） |

`EmbySourcePicker` 原规则是「码率 ≤ 12 Mbps 的里面取最高」。**4K AV1 的 11.9 Mbps 正好落在阈值内**
⇒ 会被当成"可播版本里画质最好的"选中 ⇒ 用户点开就是 10 fps。

⇒ 判据必须从「只看码率」升级为「**codec × 分辨率 × 设备硬解能力**」。

## 二、改动（3 个文件，均在既有边界内）

### 2.1 新增 `playback/Av1HardwareProbe.kt`

- `hasHardwareAv1()`：查 `MediaCodecList` 里有没有 `video/av01` 的**硬件**解码器
  （`isHardwareAccelerated`，API 29+；minSdk 26 ⇒ 旧系统按 `OMX.google.`/`c2.android.` 前缀判软解）。
  **软解不算**——它正是"4K 跑不动"那条路径。
- `isLikelyUnplayable(codec, height, av1Hw)`：**只拦已实测确认跑不动的组合**
  （4K+ 的 AV1 且无硬解）。1080p AV1 不拦（实测 60.44 fps），高度未知不拦（不替用户拒绝）。
- 进程内缓存：设备能力不会变，只探一次。

### 2.2 `emby/EmbySourcePicker.kt` 加"编码闸门"

- `Item` 增加 `codec` / `height`，从**每个** `MediaSource` 自己的 `MediaStreams[Type=Video]` 读。
- `pick()` 先用闸门分组，再在原组内走原码率规则：
  - 有可播版本 ⇒ 原规则（码率最高），reason 追加"已跳过 N 个 4K+ AV1（无硬解）"；
  - 全是 4K AV1 ⇒ 选码率最小的，reason 写明"所有版本都是 4K+ AV1 且本机无 AV1 硬解"；
  - 单版本也是 4K AV1 ⇒ 仍选它，但 reason 带 ⚠️ 说明（**不静默**）。
- `pick()` 增加可注入参数 `av1Hw`（默认现探），让测试能覆盖两个分支而不依赖设备。

### 2.3 `ui/compose/PlayerStartPolicy.kt` 复用既有规格 Toast

`showStartupSpec()` 本来就会弹一行片源规格（分辨率 · codec · 码率）。
加一句条件文字：`isLikelyUnplayable(codec, h)` ⇒ 追加「本机无 AV1 硬解·可能无法流畅」。

**没有新增控件、没有改布局**，只是同一个 Toast 多一段文字（守"不许写 UI"）。

> 该 Toast 目前只在 **SMB 路径**调用（`PlayerSmbStart.kt:210`），也就是用户播 NAS 文件的实际路径。
> Emby/URL 路径没有这个 Toast —— **本改动不给它新增 UI 行为**，那边靠选源自动避开。

## 三、验证：15/15 PASS（`EmbySourcePickerInstrumentation`）

| # | 用例 | 结果 |
| --- | --- | --- |
| 1–6 | 原有 6 条（485 真实数据、全超阈值、单版本、空/null、多可播取最高） | ✅ 全部仍 PASS（**无回归**） |
| 7 | 4K AV1 11.9M + 1080p HEVC 8M，`av1Hw=false` ⇒ 必须选 **1080p HEVC** | ✅ |
| 7b | 原因写明"已跳过" | ✅ |
| 8 | 同输入 `av1Hw=true` ⇒ 选 **4K AV1**（**不误伤**有硬解的设备） | ✅ |
| 9 | 全部 4K AV1 + `av1Hw=false` ⇒ 选最小码率 | ✅ |
| 9b | 原因写明"无 AV1 硬解" | ✅ |
| 10 | 1080p AV1 不拦（实测 60.44 fps 可播） | ✅ |

**本机真实探测值**：`INSTRUMENTATION_RESULT: av1Hardware=false`
⇒ K40 确实无 AV1 硬解，闸门在真机上会生效（不是只在注入参数时成立）。

## 四、未闭合 / 边界

1. **Toast 未在真实 4K AV1 实播中目视确认**：NAS（192.168.5.36）可达，但 K40 上没保存该 SMB 源、
   凭据未知，实播需要用户在 UI 里添加源。逻辑链（codec/height 取值 → 判定 → 追加文字）已由
   单测 + 代码路径确认，**但"用户眼睛里看到那句话"这一步未取证**。
2. **Emby 侧没有真实 4K AV1 条目可测**：用例 7–10 用的是构造的 `MediaSources` JSON
   （字段取自真实 Emby 响应结构），不是线上条目。
3. 闸门只覆盖 **AV1**。其它 codec 的"4K 软解跑不动"（如 4K VP9）未纳入——没有实测证据，
   不猜。
4. 选源只影响 Emby 播放 URL；本地/SMB 直选文件时没有多版本可选，只能靠 Toast 提示。

## 五、复算命令

```bash
export JAVA_HOME="C:/Android/jdk-17.0.13+11"; export ANDROID_HOME="C:/Android/sdk"
export PATH="/c/Android/sdk/platform-tools:$PATH"; export MSYS_NO_PATHCONV=1
cd android
./gradlew :app:assembleDebug --offline
./gradlew :app:assembleDebugAndroidTest \
  -Pkivo.testRunner=com.kivo.cinema.emby.EmbySourcePickerInstrumentation --offline
adb -s b6e950ee install -r app/build/outputs/apk/debug/app-debug.apk
adb -s b6e950ee install -r app/build/outputs/apk/androidTest/debug/app-debug-androidTest.apk
adb -s b6e950ee shell am instrument -w \
  com.kivo.cinema.test/com.kivo.cinema.emby.EmbySourcePickerInstrumentation
```

期望输出：`pass=15 / total=15 / av1Hardware=false`（K40）。

## 六、改动文件

| 文件 | 改动 |
| --- | --- |
| `playback/Av1HardwareProbe.kt` | **新增**（能力探测 + 可播判定） |
| `emby/EmbySourcePicker.kt` | 加编码闸门、Item 带 codec/height、可注入 `av1Hw` |
| `ui/compose/PlayerStartPolicy.kt` | 规格 Toast 追加一句风险提示 |
| `androidTest/.../EmbySourcePickerInstrumentation.kt` | 新增用例 7–10（4 个断言组） |
