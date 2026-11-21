# 字幕 AI 翻译工具链 · 落位验收

> 2026-11-17 ｜ 仓库 `C:\kivo video`，分支 `codex/kivo-video-p0-ui-01-playback-overlay-v13`，HEAD `82e98e91`
> 来源：`F:\Files\实用工具\AI工具\翻译工具链\staging\`（12 文件）

## 一、落位（10 文件，路径照抄）

```
backend/include/kivo/cinema_engine/subtitle_translation_core/
  subtitle_translation_engine.hpp / subtitle_translation_prompt.hpp
  subtitle_translation_types.hpp / translation_result_writer.hpp
backend/src/subtitle_translation_core/            # 无 cinema_engine 层
  subtitle_translation_prompt.cpp / translation_result_writer.cpp
backend/tests/subtitle_translation/
  subtitle_translation_prompt_test.cpp / translation_result_writer_test.cpp
third_party/llama.cpp-android/
  build.sh / README.md
```

落位前核验：四个目标目录均不存在；两份 CMake 未在未提交列表中 → 无覆盖、无冲突。

## 二、CMake 接线（两份都改）

| 文件 | 内容 |
| --- | --- |
| `backend/CMakeLists.txt` | `kivo_core_subtitle_translation` 静态库（`/utf-8 /W4`）+ 两个测试 target（`TIMEOUT 60`）；插在 `kivo_subtitle_runtime_test` 之后 |
| `android/app/src/main/cpp/CMakeLists.txt` | 同一份源注册为 `kivo_core_subtitle_translation`（`${KIVO_BACKEND}`，NDK 无 `/utf-8`）；随后 llama.cpp IMPORTED target（`dist` 不存在则 STATUS 跳过） |

## 三、验收结果

### 3.1 新增测试：18 PASS ✅

```
kivo_subtitle_translation_prompt_test          Passed
kivo_subtitle_translation_result_writer_test   Passed
直接运行两 exe：PASS 计数 = 18（prompt 9 + writer 9），两 exe EXIT=0
```

### 3.2 全量回归：294 → 296，其中 2 红（均非本次引入）

| 项 | 实测 |
| --- | --- |
| 测试总数 | **296**（README 写 286，**快照已过期**；本机基线实测 294） |
| 通过 | 294 中除下列 2 条外全绿，83 条 Skipped |
| ❌ #172 `kivo_redline_gate_production_scan` | 历史存量 23 条：**文档里的绝对路径被误判为凭据**（`/home/`、`/Users/`、`\\NAS\`）。本次新增的 1 条（`rex-dna-browse.md`）已修掉，违规数 **24 → 23**，报告不再包含本次文件 |
| ❌ #277 `kivo_p14z_step_repay_tests` | `discontinuity_trace().events` 断言失败；该文件最后提交 `df7040bf`（10-31），**与字幕翻译无关**，属既有红 |

### 3.3 Android arm64 ✅

| 项 | 结果 |
| --- | --- |
| `./gradlew :app:assembleDebug` | **BUILD SUCCESSFUL**，无 `undefined symbol` |
| 显式编译 arm64 库 | `libkivo_core_subtitle_translation.a` **289,266 B**（NDK 下两源文件均编译、链接通过） |
| llama dist | `C:/re-tools/llama.cpp-android/dist` 不存在 → CMake 打印 STATUS 并跳过，不影响构建 |

### 3.4 构建环境注意

`build/ninja-debug` 必须**先 `vcvars64.bat` 初始化 MSVC 环境**再构建；否则报 `C1083 "string"/"cassert" 找不到`（INCLUDE 为空，非代码问题）。

## 四、红线遵守

1. 未碰 `playback_graph` / `player_runtime_adapter` / `mpv` / `libass`。
2. 未做播放中实时翻译。
3. 未改 `SettingsAiSub.kt`（连灰开关也未点亮，属未做部分）。

## 五、未做（规格已在 README §4/§5）

- `SubtitleTranslationEngine` 实现（llama.cpp 封装 + 调度 + 兜底）
- JNI 桥（`AiTranslationNative`）
- Kotlin 控制器 + 设置页点亮
- 块级 few-shot 的 C++ 契约
- Vulkan 产物（缺 LunarG `glslc`）

## 六、结论

落位与接线**完成**：18 个用例全 PASS、两份 CMake 均注册、arm64 真实编译通过。
全量「296 全绿」**未达成**，但两条红均可在本次改动之外复现：redline 是历史文档路径误报（23 条存量），p14z 是既有失败。要全绿需另开两张卡（清理历史文档路径 / 排查 p14z），不在本次落位范围。