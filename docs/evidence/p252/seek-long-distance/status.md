# 长片源「远距离 seek 失效」（新发现，未修）

日期：2026-11-10 夜
设备：K40 `b6e950ee`；服务器：小米路由 `192.168.5.1` 共享 `XiaoMi`
测试：`SmbPlaybackSeekInstrumentation`（新增 `-e seekTargetMs` 可指定落点）

## 一、怎么发现的

在 `Shelter` 与 4K 原盘上跑既有 seek 验收，两条都失败：

```
4K  : target=3395600 ms landing=0 ms actual=2900 ms
Shelter: target=3217589 ms landing=0 ms actual=2906 ms
```

同一个测试在 `local-seek.mkv`（2 MB / 60 s）上 **4/4 PASS**（落点 27106 ms）。

## 二、bisect：不是"seek 坏了"，是"跳远了才坏"

在 Shelter 上按落点扫描：

| 目标 | 落点 landing | 实际位置 actual | 判定 |
| --- | --- | --- | --- |
| 60 s | 50 050 ms | 63 530 ms | 成功（关键帧对齐偏 10 s，未超 ±5 s 判据的边界） |
| 100 s | 90 090 ms | 111 564 ms | 落点偏 10 s、实际又跑过头 21 s → 判据失败 |
| 180 s | **0 ms** | 2 844 ms | **完全没跳**，位置停在起播附近 |
| 600 s | **0 ms** | 2 857 ms | 同上 |
| 中段（1 074 min 的一半） | **0 ms** | 2 906 ms | 同上 |

规律：**近距可跳，远距返回 landing=0 且画面停在 2.8–2.9 s**（起播后位置）。

## 三、排除了什么

- 不是传输能力问题：同一文件顺序读 24 MiB 实测 **7.4–11.3 MB/s**，
  在 13 GB 偏移处随机读也正常（`SmbWindowDiag` 读 size/2 成功）。
- 不是小文件特例：2 MB 的文件能跳，是因为它整份落在 12 MiB 块窗口里、
  **根本不发生网络读** —— 所以它证明不了 SMB 上的 seek。
- 不是文件坏了：这两个文件在别的播放器上跳转正常。

## 四、下一步（要动核心，不在本卡）

需要在 `session->seek_to()` 这条链上打点，把三件事分开：

1. demux 层 `seek` 是否返回错误（MKV 索引 / 同步点）；
2. 视频 `landing_us` 为 0 是"没跳"还是"跳了但没记录"；
3. 音频 seek（TrueHD / DDP，走 FFmpeg）是否**失败后把整体 seek 一起判失败**
   （`jni_session_handle.cxx:265` 之后有 `audio->seek_to`）。

判据：Shelter / 4K 上 seek 到任意位置，`|actual - target| ≤ 5 s`，且 landing 非 0。

## 五、复算

```bash
P="视频/电影/Shelter 2026 USA BluRay 1080p TrueHD5.1 x265 10bit-CHD/Shelter 2026 USA BluRay 1080p TrueHD5.1 x265 10bit-CHD.mkv"
adb shell "am instrument -w -e smbHost 192.168.5.1 -e smbShare XiaoMi \
  -e smbPath '$P' -e seekTargetMs 60000 \
  com.kivo.cinema.test/com.kivo.cinema.smb.SmbPlaybackSeekInstrumentation"
# 依次把 seekTargetMs 换成 100000 / 180000 / 600000 观察 landing 与 actual
```

## 六、附：本轮同时确认的好消息

同一晚在 4K 原盘上做的持续播放基准（`SmbPaceBenchInstrumentation`，15 s × 4 窗）：

```
#1 prefetch=off  26.46 fps  dropped=0  speed=1.00x
#2 prefetch=on   26.33 fps  dropped=0
#3 prefetch=off  26.47 fps  dropped=0
#4 prefetch=on   22.60 fps  dropped=56   ← 开预读才丢帧
```

**关预读（App 的实际策略）两窗都是 0 丢帧、1.00x。** 与 `Shelter` 上的结论一致。
