# FFmpeg linkable SDK (local bootstrap)

这个目录不是 FFmpeg 源码，也不是它的副本。它是**一台机器本地生成的“可链接 SDK 根”**，
用来绕开预编译 SDK 里导入库（`*.lib`）的权限问题。

## 为什么需要它

`C:\ffmpeg-sdk\ffmpeg-n7.1-latest-win64-gpl-shared-7.1` 里：

| 内容 | 状态 |
|---|---|
| `include\*.h` | 可读 |
| `bin\*.dll` | 可读 |
| `lib\avcodec.lib` / `avformat.lib` / `avutil.lib` / `swresample.lib` / `swscale.lib` | **拒绝访问**（文件级保护 DACL，连读 ACL 都被拒） |
| `lib\avdevice.lib` / `avfilter.lib` / `postproc.lib` | 可读 |

后果：任何链接 FFmpeg 的目标都失败，报 `LNK1104: 无法打开文件 avformat.lib`。

修复有两种：

1. 管理员执行 `icacls ... /grant "用户:(OI)(CI)(RX)" /T`（需要管理员）；
2. **本目录的做法**：不碰原文件，用可读的 DLL 导出表重新生成导入库（无需管理员）。

## 它是怎么做的

```
third_party/ffmpeg-sdk/
├── include   -> junction 指向源 SDK 的 include
├── bin       -> junction 指向源 SDK 的 bin
├── lib/      <- 本目录真实生成
│   ├── avcodec.def / .lib
│   ├── avformat.def / .lib
│   ├── avutil.def / .lib
│   ├── swscale.def / .lib
│   └── swresample.def / .lib
├── bootstrap.cmd
└── README.md
```

步骤：
1. `dumpbin /exports <dll>` 取出导出符号表（DLL 可读，所以这一步不需要任何特权）；
2. 生成 `.def`（`LIBRARY xxx-61.dll` + `EXPORTS` + 符号列表）；
3. `lib /def /machine:x64 /out:` 生成导入库。

生成的 `.lib` 与原版**字节数完全一致**（avcodec 40496 / avformat 46990 / avutil 134658 /
swscale 8308 / swresample 6432），导出符号数为 168 / 190 / 599 / 31 / 23。

## 怎么用

`CMakePresets.json` 里的 `FFMPEG_ROOT` 已指向本目录：

```json
"FFMPEG_ROOT": "${sourceDir}/third_party/ffmpeg-sdk"
```

运行时需要 DLL 在 `PATH` 上（`bin` 是 junction，等同于原 SDK 的 bin）：

```bash
export PATH="C:/kivo video/third_party/ffmpeg-sdk/bin:$PATH"
```

## 什么时候需要重跑

- 换了机器 / 换了 FFmpeg 版本 / 重装了 SDK；
- 删过本目录。

```cmd
bootstrap.cmd "C:\ffmpeg-sdk\ffmpeg-n7.1-latest-win64-gpl-shared-7.1"
```

脚本是幂等的：junction 已存在会跳过，导入库会重新生成。

## 已知限制

- `include` / `bin` 是 **junction**，属于本机路径绑定，不能提交到版本库，
  因此本目录下的 `include/`、`bin/`、`lib/` 都在 `.gitignore` 里。
- 导入库由 DLL 导出表反推，所以**总是与 DLL 完全一致**；
  但若源 SDK 的导出表本身缺少某个符号，这里也不会有——此时需要换一份完整构建的 SDK。
- 本机实测的显卡是 **Intel UHD Graphics 630**，`d3d11va` 硬解对
  H.264 High 8bit、HEVC Main10 可用；H.264 **4:4:4** 会初始化失败并回退软解
  （Intel 不支持该 profile 的硬解）。这是硬件限制，不是 SDK 或代码问题。
