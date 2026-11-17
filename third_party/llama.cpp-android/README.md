# llama.cpp → Android arm64-v8a

AI 字幕翻译的端侧推理内核。产物是静态库，链接进播放器的原生库。

## 用法

从 MSYS2 bash 里跑（仓库 third_party 下的构建脚本一律是 build.sh）：

```bash
/c/msys64/usr/bin/bash -lc 'cd "/c/kivo video" && bash third_party/llama.cpp-android/build.sh'
```

## 前置

| 项 | 默认值 | 覆盖 |
|---|---|---|
| NDK | `/c/Android/sdk/ndk/30.0.16248370` | `NDK=` |
| llama.cpp 源码 | `/c/re-tools/llama.cpp-src` | `LLAMA_SRC_DIR=` |
| 产物目录 | `/c/re-tools/llama.cpp-android/dist` | `LLAMA_DIST_DIR=` |
| 构建目录 | `/c/re-tools/llama.cpp-android/build` | `LLAMA_BUILD_DIR=` |
| cmake | 自动找 | `CMAKE_EXE=` |
| Vulkan | 关 | `LLAMA_VULKAN=1`（需 LunarG SDK） |

源码不在就先 clone：

```bash
git clone --depth 1 https://github.com/ggml-org/llama.cpp.git /c/re-tools/llama.cpp-src
```

## 产物

```
dist/
├── include/   llama.h  ggml.h  ggml-*.h ...
└── lib/       libllama.a  libggml.a  libggml-base.a  libggml-cpu.a
               （开 Vulkan 时多一个 libggml-vulkan.a）
```

## 消费方式

`android/app/src/main/cpp/CMakeLists.txt` 里以 IMPORTED target 链接
（与 `ffmpeg-android` / `libass-android` 同一范式，详见 `staging/CMake.snippet.md` §③）。

**产物不进版本库**，目录不存在时整个目标静默跳过，其余构建不受影响。

## 几个必须知道的点

### GGML_BACKEND_DL=OFF 不是可选项

llama.cpp 默认开后端动态加载。静态链接进 App 后运行时找不到
`ggml-*-vulkan.so`，**Vulkan 会静默不可用** —— 不报错，只是没有 GPU 加速，
症状是「明明是旗舰机却和低端机一样慢」，极难归因。
静态嵌入必须关掉后端 DL，改为编译期注册。

### Vulkan 需要 LunarG Vulkan SDK 的 glslc

`ggml-vulkan/CMakeLists.txt` 是 `find_package(Vulkan COMPONENTS glslc REQUIRED)`，
硬依赖。本机没装 SDK 时默认关 Vulkan；装好后 `LLAMA_VULKAN=1` 打开。

Vulkan 对本项目的意义：1.7B 模型在 K40 上纯 CPU 只有 12.42 tok/s decode，
开 Vulkan 后预计 2-4 倍 —— 这是「1 小时视频字幕本地翻译」体验是否可接受的分水岭。

### 构建路径必须纯 ASCII

路径含中文时 ninja 报 `failed recompaction: Permission denied`，
CMake 直接 Generate 失败。默认路径都在 `/c/re-tools/` 下就是为此。

### GGML_NATIVE=OFF

NDK 交叉编译时宿主 CPU 特性探测没有意义。开着会按宿主（x86）选指令集，
产物在 arm64 上可能非法指令。

### `set -o pipefail` + `grep -q` 会假失败（build.sh 的坑）

`build.sh` 末尾有符号自检（`llama_model_load_from_file` / `llama_init_from_model` /
`llama_decode` / `llama_sampler_sample`，4 个都必须在产物里）。
**不要**写成 `llvm-nm "$lib" | grep -q "$sym"`：`grep -q` 命中后立刻退出，
管道写入端收到 SIGPIPE/EPIPE，`pipefail` 把整条管道判为失败 ——
符号明明在库里却报「不在任何库」（MSYS2 实测返回 74）。

写法：先把符号表抓进变量，再用 herestring 喂给 grep（不走管道，就没有 SIGPIPE）：

```bash
syms="$("$NM" "$lib" 2>/dev/null || true)"
grep -q "$sym" <<< "$syms"
```

## 真机实测（K40 / 骁龙 870 / Android 13）

| 模型 | prefill | decode | 质量 |
|---|---|---|---|
| Qwen3-0.6B-Q4_K_M | 67.96 tok/s | **32.09 tok/s** | 回吐英文 6%（逐行）/ 1%（块 20 行） |
| Qwen3-1.7B-Q4_K_M | 20.24 tok/s | **12.42 tok/s** | 回吐英文 1-2% |

均为 **CPU 后端**（未开 Vulkan）。

模型放哪：`filesDir/asr-models/`（`SettingsAsrModels.kt` 已经在实测那个目录的文件数与字节，
模型下载落位直接复用）。**不要打进 APK**。
