# SenPlayer 拆解：它为什么能在 SMB 上跳转（对比我们的 seek 缺陷）

素材：`F:\Files\实用工具\苹果相关\ipaDown20261030\App\syzayy91@gmail.com\SenPlayer-1.0.0-b8-Android-arm64-v8a.apk`
（87.9 MB，`arm64-v8a` 单 ABI，306 个条目）

## 一、它用什么做传输与解封装

```
lib/arm64-v8a/libFFmpegKit.so       54.4 MB   ← FFmpeg（解封装 + 解码 + 协议层）
lib/arm64-v8a/libMLibKit.so         15.7 MB   ← 自研媒体库
lib/arm64-v8a/libc++_shared.so       1.3 MB
（无 libsmb2.so、无独立 SMB 原生库）
```

两类证据：

1. `libFFmpegKit.so` 里出现整套 **`SMB2_STATUS_*`** 错误常量表
   （`SMB2_STATUS_ACCESS_VIOLATION`、`SMB2_STATUS_ACCOUNT_EXPIRED`…）。
   那是 **libsmb2 的错误码表**被静态链进 FFmpeg —— FFmpeg 的 `libsmbclient`
   协议实现底层就是 libsmb2。
2. `classes.dex` 里有 `Lcom/mlibkit/smb/SMBClient;` —— 自研 SMB 客户端（浏览/枚举用）。

结论：**它把 `smb://` 直接交给 FFmpeg 打开**，解封装、seek、读取全在 FFmpeg 的
avio 层完成；Java 侧只做目录浏览。

## 二、这解释了我们这边为什么跳不动

我们的路径是：`SmbByteSource → RangeFd → ParcelFileDescriptor → **系统 MediaExtractor** → MediaCodec`。
解封装用的是平台提取器，seek 也是它做的。真机实测（252）：

| 目标 | 平台提取器落点 |
| --- | --- |
| 60 s | 50 050 ms（可用） |
| 180 s | **0**（`seekTo` 仍返回 AMEDIA_OK） |
| 600 s | **0** |

即：平台提取器在这个 MKV 上**报成功但落点塌回开头**，上层把"跳到 0"当成功，
于是用户看到"拖了进度条，画面从头开始"。

SenPlayer 不会有这个问题，因为 FFmpeg 的 MKV demuxer 自己解析 Cues/SeekHead，
并通过 avio 的字节区间 seek 定位 —— 与我们这套 `RangeFd` 支持随机读的能力完全匹配。

## 三、我们的修法（已在仓库里有零件）

仓库里**已有** FFmpeg 解封装路径：
`backend/src/video/video_plane/backends/ffmpeg/demux/ffmpeg_demux_source.cxx`
（P240-B 引入，"只解封装，不解码"，并已注册进 `kivo_backend_ffmpeg`）。

所以正确的修法不是继续修 MediaExtractor，而是：

**大文件 / SMB 片源走"FFmpeg demux + MediaCodec 解码"**，seek 交给 FFmpeg。
平台提取器只保留给"确认它工作正常"的场景（本地小文件、平台强项封装）。

判据：Shelter / 4K 上 seek 到任意位置，`|actual − target| ≤ 5 s`，且不再出现 landing=0。

## 四、本轮已落地的两处修复（与此相关）

1. **CMake 变量顺序 bug（真 bug，阻断全量重建）**：
   `KIVO_LIBASS_ANDROID_DIST` 定义在文件末尾，却在 `kivo_backend_ffmpeg` 里先被使用，
   未定义时展开成 `/include` → `fatal error: 'ass/ass.h' file not found`。
   增量构建碰不到，一旦清 `.cxx` 缓存就再也编不出来。已把定义前置，全量重建 23 s 通过。
2. **平台提取器 seek 落点校验（已加，未验证生效）**：
   `PlatformExtractorSource::seek_to` 现在会在落点塌回开头（<2 s 而目标 >2 s）时
   换 `CLOSEST_SYNC` 重试，仍塌回则返回 -1，让上层明确报错而不是静默从头播。
   真机复测 landing 仍是 0、无错误 —— 说明要么这条分支没被触发，要么上层没把
   `-1` 变成可见错误，**需要下一步用日志确认**。不再盲改。

## 五、复算

```bash
cd /tmp && mkdir -p sp && cd sp
unzip -l "…/SenPlayer-1.0.0-b8-Android-arm64-v8a.apk" | grep '\.so$'
unzip -o -q "…/SenPlayer….apk" "lib/arm64-v8a/libFFmpegKit.so" "classes.dex"
grep -a -o -E "SMB2_STATUS_[A-Z_]+" lib/arm64-v8a/libFFmpegKit.so | sort -u | head
grep -a -o -E "L[a-zA-Z0-9/]*Smb[A-Za-z0-9/]*;" classes.dex | sort -u | head
```
