# ffmpeg-android（211 软解兜底的 FFmpeg 交叉编译）

## 这是什么

把 FFmpeg 7.1 交叉编译成 Android **arm64-v8a 静态库**（avformat/avcodec/swscale/avutil），
供 kivo 的软解兜底路径（`runtime/local_media/` 家族）链接。

- 源码：FFmpeg 7.1（`https://ffmpeg.org/releases/ffmpeg-7.1.tar.xz` 或
  GitHub `FFmpeg/FFmpeg` tag `n7.1`），下载到 `/c/re-tools/ffmpeg-src/`。
- 工具链：NDK r30（`/c/Android/sdk/ndk/30.0.16248370`）+ 自带 make。
- 运行：`bash third_party/ffmpeg-android/build.sh`（幂等；日志 `third_party/ffmpeg-android/build.log`）。
- 产物：`dist/{lib/*.a, include/}`（**gitignore**，本目录只留脚本与文档）。

## 配置要点（build.sh 的 configure 一行里都有）

- `--disable-everything` 策略的反面：显式白名单——只开
  `file` 协议 + 12 个 demuxer（rm/asf/avi/flv/mkv/mov/ts/mp3/wav/ogg/aac/mpegvideo）
  + 25 个 decoder（RealVideo 系 / WMV/VC-1 系 / 通用兜底 h264/hevc/aac/mp3 等）。
- 关掉 programs/doc/avdevice/postproc/avfilter/**network**——与 SenPlayer 同款
  "只做 demux+decode" 的裁剪思路（`docs/reference/rex-teardown/03` §一）。
- 静态库而非动态：Android APK 打不了带版本号的 soname
  （`libavcodec.so.61` 不过 `*.so` 包名过滤），动态化改由上层封装单一
  `libkivo_ffmpeg.so`。

## LGPL 合规（v2.1+，动态使用方式）

FFmpeg 以 `--disable-gpl`（默认）配置 = **LGPL v2.1+**。本工程以"静态库封装进
独立 `libkivo_ffmpeg.so`"的方式**动态链接**使用，为满足 LGPL §6：

1. FFmpeg 源码版本（7.1）与 configure 参数全文见 `build.sh`（无任何修改补丁；
   若未来打补丁，补丁文件必须进本目录）；
2. 应用发布时随分发提供本目录的 `build.sh` 与上述源码链接，
   使用者可据以重新链接等效产物；
3. 不修改 FFmpeg 本体；对 FFmpeg 的使用（demux/decode/swscale）通过
   `backend/src/third_party_adapter/ffmpeg_adapter/` 一层进行。

## 产物自检

`build.sh` 末尾会验证 `ff_rv10_decoder / ff_wmv3_decoder / ff_cook_decoder`
三个符号存在（对照 `tests/real/sd211/` 的两支测试片）。
