# SenPlayer 拆解（真实反编译）：它的播放内核是 **定制版 libmpv**

素材：`F:\Files\实用工具\苹果相关\ipaDown20261030\App\syzayy91@gmail.com\SenPlayer-1.0.0-b8-Android-arm64-v8a.apk`
（87.9 MB，`arm64-v8a` 单 ABI）—— 注意它是 **APK，不是 IPA**。

## 一、拆解手段（都是真做的，不是看字符串猜）

| 手段 | 做了什么 | 结果 |
| --- | --- | --- |
jadx 1.5.2 | 全量反编译 | 6141 个类 |
AES/GCM 解密 | 自己写出解密器，把 `u54.a("s1:…")` 全部解成明文 | **8633 条**字符串 |
NDK llvm-readelf | 符号表/节表 | 两个 .so 的归属与导出 |
**Ghidra 12.2 headless** | 导入 `libFFmpegKit.so` 并自动分析 | 16 分钟，导出 9 个 seek 相关函数 |

### 字符串加密（自己解出来的）

`defpackage/u54.java`：

```java
key[i]  = b[i] ^ c[i]                      // 两组 32 字节常量
nonce   = base64(payload)[0..12]
aad     = "MLibKit Android strings v1"
cipher  = AES/GCM/NoPadding
明文     = UTF-16BE                        // 注意是 BE：Java 侧手工按 (high, low) 组装 char
```

（第一次按 UTF-16LE 解会得到一堆 CJK 乱码；按 BE 解立刻是正常英文/中文。）

## 二、结论：**`libFFmpegKit.so` 是改过的 libmpv**

证据一 —— mpv 的选项表原样出现在二进制里：

```
hr-seek  revert-seek  default-precise  osd-on-seek  demuxer-cache-duration
demuxer-max-bytes  cache-secs  playlist-pos  secondary-sid  vd-lavc  ao-*  gpu-next
demuxer-thread  libmpv
```

证据二 —— 版权行：

```
Copyright © 2000-2025 mpv/MPlayer/mplayer2 projects      → 2025 年的 mpv 分支
```

证据三 —— **他们自己的定制项**（只有一条，但足以定性）：

```
senplayer-export-thumb-packets
```

证据四 —— mpv 的 seek 相关报错串全在：

```
"hr-seek, skipping to %f%s%s"        "Cannot seek in this file..."
"discarding unseekable range due to stream %d"   "Cannot seek backward in linear streams!"
"Too many frames, seeking is not possible"      "preload-demux start reading=%d ... byte-seeks=%l"
```

证据五 —— Ghidra 反编译导出的 9 个函数（`C:\re-tools\sen_seek_probe.c`）：

| 地址 | 命中的串 | 属于 |
| --- | --- | --- |
`FUN_0198850c` | `hr-seek, skipping to %f%s%s` | **mpv `player/seek.c` 的 seek 主流程** |
`FUN_0193296c` | `Cannot seek in this file` | mpv `demux` |
`FUN_01939d64` | `discarding unseekable range due to stream %d` | mpv `demux` |
`FUN_019301a0` | `preload-demux start reading=%d … byte-seeks=%l` | mpv `demux` 预载统计 |
`FUN_031e3fa0` | `Cannot seek SMB file` | **他们自己的 SMB 客户端** |
`FUN_031e3a2c` | `Invalid SMB native buffer` | 同上 |
`FUN_01995010` | `http-lavf seek-cache from=%ld to=%ld` | FFmpeg http 协议 |

## 三、它的 SMB 是怎么走的

- `libFFmpegKit.so` 与 `libMLibKit.so` **都**带整套 `SMB2_STATUS_*` 错误表
  （libsmb2 的符号被静态链进去）；FFmpegKit 那个里面还有 FFmpeg 的
  `avformat_open_input` / `avformat_find_stream_info`。
- `libFFmpegKit.so` 里有 FFmpeg libsmbclient 的原始报错串
  `"URL does not start with 'smb://'."` ⇒ **播放时是把 `smb://…` URL 直接交给
  FFmpeg**，由 FFmpeg 的协议层做随机读。
- `libMLibKit.so` 是他们自研的工具库（导出 `Java_com_mlibkit_smb_SMBClient_*`：
  `create / destroy / invoke / fileRead / fileWrite / readBlock / writeBlock / cancelCore`），
  用来**浏览目录**。注意它有 `fileRead/fileWrite/readBlock/writeBlock` 与
  `Cannot seek SMB file` ⇒ 他们的 SMB 客户端**原生支持 seek**。

Java 侧（解密后）：

```java
// com.mplaykit.Player
public final CompletableFuture G(String cmd, long sourceId, String[] args, double... vals)
    → N.n3(handle, cmd, sourceId, args, vals)      // native 通用命令入口

// defpackage.so3 —— 拖进度条真正发出去的东西
Integer num = (Integer) player.G("seek", op3Var.k1, new String[0], d, flag).get();
//   cmd="seek"，sourceId=op3Var.k1，d=目标秒，flag=精确/不精确
```

**一条命令、一个目标时间、作用于整个源** —— 没有"视频 seek 一次、音频再 seek 一次"。

## 四、它为什么不会卡：mpv 的三条结构性差异

1. **单一解复用器**。mpv 只有一个 `AVFormatContext`、一个 demux 线程，
   音频与视频的包都从**同一条顺序流**里出来（`demuxer-thread` / `demuxer-cache-duration`
   / `demuxer-max-bytes` 都在）。消费者之间**根本不存在"互相排队读"**。
   我们现在是：视频一套 FFmpeg demux + MediaCodec，音频另一套 FFmpeg demux，
   两个 demux 各自 seek、各自读**同一条 SMB 连接**。
2. **seek 由 mpv 统一发起**：`mp_seek` → `demux_seek` → `av_seek_frame(ic, -1, ts, …)`
   （stream index = -1，用默认流即视频轨的索引），一次 seek 同时定位音频与视频。
3. **SMB 由 FFmpeg 自己管**：`smb://` URL 直接进 `avformat_open_input`，
   随机读由 libsmbclient 提供（原生支持 seek），不需要我们自造 fd 代理 + 块窗口。

我们这两天的两个硬骨头 —— "远距离 seek 跳不动"（音频轨没有索引 → FFmpeg 在 12 GB 文件里二分）
与 "偶发杂音"（两个消费者在同一条读路径上排队）—— **在 mpv 的架构里都不存在**。

## 五、给我们的两条路

- **短期**（保持现有架构）：把音频并到视频那一个 demux 上（同一 `AVFormatContext`），
  音频包从同一条流里分出来；这同时解掉 seek 与争用两件事。
- **长期**：直接用 libmpv 内核（SenPlayer 就是这么做的）。mpv 自带 demux 缓存、
  `hr-seek`、精确 seek、多轨、字幕、硬件解码，与我们的 UI/资源库拼装即可。
  代价是要放弃现有的 `backend/` 播放核心。

## 六、复算

```bash
# 字符串解密（8633 条）
python decode_strings.py out/sources

# Ghidra：导入 + 分析 + 导出 seek 函数
analyzeHeadless.bat C:\re-tools\ghidra-projects SenFF \
  -import libFFmpegKit.so -overwrite \
  -scriptPath C:\re-tools\scripts \
  -preScript sen_slim.java -postScript sen_seek_probe.java
# → C:\re-tools\sen_seek_probe.c
```
