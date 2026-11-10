# 硬解到底该不该开预读（重测，未完成）

日期：2026-11-10 夜
设备：K40 `b6e950ee`（Android 13 / kona）
片源：`The.Furious.2026...H.265-CHDWEB.mkv`（20.9 GB，3832×1592 HEVC HDR10+，硬解）

## 一、为什么要重测

243 关掉了硬解路径的预读（`PlayerComposeActivity` 里 `setPrefetchEnabled(false)`），
理由是"实测帧率 24.00 → 20.27、末尾出现 567 ms 卡顿"。

那条数据是在**块窗口会双倍读取**的版本上测的（24 MiB 逻辑读拉 50 MiB，
见 [window-double-fill](../window-double-fill/status.md)）。双倍读取意味着预读那条连接
在跟主读抢同一份带宽，而抢的额度里有一半是浪费。现在浪费没了，结论必须重测 ——
不能拿旧数字继续关着一个可能有益的开关。

## 二、方法

新增 `androidTest/smb/SmbPaceBenchInstrumentation.kt`：

- 同一次运行内 **关→开→关→开** 交替，每窗 15–20 s，切换只翻一个开关（不解码器、不重建会话）；
- 每窗结束让上一段余量排空 1.5 s，避免边界混入；
- 从原生 summary 取累计 `frames` / `dropped`，取差值得每窗独立数字；
- 报 `speed`（媒体秒 / 实际秒）与 `pace_lateness_p99`。

为什么必须同一次运行内交替：这台 NAS 的吞吐有分钟级波动（裸读 8.2–8.9 MB/s 都出现过），
分两次跑无法区分"预读的差别"和"机器当时的状态"。

## 三、结果（健康段）

两次独立运行，前三个窗口全部健康：

| 运行 | 窗口 | 预读 | fps | dropped | speed |
| --- | --- | --- | --- | --- | --- |
| #1 | 1 | 关 | 25.80 | 0 | 1.00x |
| #1 | 2 | **开** | 25.75 | 0 | 1.00x |
| #1 | 3 | 关 | 25.80 | 0 | 1.00x |
| #2 | 1 | 关 | 26.40 | 0 | 1.00x |
| #2 | 2 | **开** | 26.33 | 0 | 1.00x |
| #2 | 3 | 关 | 26.46 | 0 | 1.00x |

**开与关的差别在噪声内（0.2%）。243 那个"24.00 → 20.27"不再复现。**

配合吞吐数据（[window-double-fill](../window-double-fill/status.md) §六）：
AHEAD 6.9 MB/s vs WINDOW 7.2 MB/s，**预读仍低 4%**，但在 0 丢帧、链路余量 2.6 倍的前提下，
这 4% 换不到任何可测的用户收益。

## 四、判定：**不翻开关**

理由不是"预读有害"，而是**没有证据表明它现在有益**：

- 吞吐略低（可测）；
- 帧率与丢帧无差别（可测）；
- 唯一可能的好处是"减少解码线程阻塞时长"，而现有观测器看不见它 ——
  `pace_lateness_p99` 六窗全 0，对这件事不敏感。

翻产品行为需要一个能证伪的判据（`RangeFd` 的 stall 次数、或解码线程单次读阻塞时长直方图），
现在没有。**留原样，把"243 的依据已失效"这件事记下来，让下一次有判据时再决定。**

## 五、顺带测到的异常（与本开关无关）

运行 #1 从第 4 窗（约 60 s 处）开始退化，运行 #2 从第 4 窗（约 45 s 处）开始退化：

```
#2 WINDOW #4 prefetch=on  speed=1.00x frames=235 dropped=154 fps=15.67
#2 WINDOW #5 prefetch=off speed=0.99x frames=205 dropped=190 fps=13.67   ← 关着也退化
#2 WINDOW #6 prefetch=on  speed=0.93x frames= 14 dropped=381 fps= 0.93
```

**第 5 窗是关预读的，照样退化** → 与开关无关。时钟是 `pace=audio_timestamp`，
音频时钟照常走、视频样本迟到就被丢，所以这种形态是"喂不上"而不是"时钟错"。
"喂不上"可能来自网络供不上，也可能来自解码器本身跟不上 4K HEVC —— 本轮**没有结论**。

为此加了 `SmbByteSource.deliveredBytes()`（每窗报 `net=… MB/s` 与 `need=… Mbps`），
用来把这两种可能分开。**这一项没跑到**，见下。

## 六、被设备状态打断

第三轮起解码器完全不出帧：

```
START codec=OMX.qcom.video.decoder.hevc dur=6791201 ms
WINDOW ... frames=0 dropped=0 net=0.00 MB/s
SUMMARY ... samples=1 frames=0 error=dequeueOutputBuffer returned -10000
```

同一个文件在既有 `SmbPlaybackSeekInstrumentation` 里也停在 `POSITION 0 ms → 0 ms`，
而 H.264 720p 的 `local-seek.mkv` 仍然 35.9 fps / 0 丢帧 —— **播放链路没坏，是 4K HEVC 那条路**。
同期 SMB 读取完全正常（RAW 11.8 MB/s、WINDOW 11.3 MB/s，见 window-double-fill §六复跑）。

判断是设备侧解码器实例进了坏状态（`ps` 里有僵死的 `Z [com.kivo.cinema]`），
重启后手机连到了另一个 Wi-Fi（`ZTE-G36ZdT-5G`，10.0.0.247/24），**访问不到 192.168.5.1**，
测量到此中断。`adb shell cmd wifi connect-network` 被拒（`SecurityException`：shell 无该权限）。

**待补**：把设备切回 NAS 所在网络后，重跑

```bash
adb shell am instrument -w -e smbHost 192.168.5.1 -e smbShare XiaoMi \
  -e windowSec 15 -e windows 6 \
  com.kivo.cinema.test/com.kivo.cinema.smb.SmbPaceBenchInstrumentation
```

看 `net=` 与 `dropped` 的关系：`net` 顶到 8–9 MB/s 仍在丢 → 网络供不上；
`net` 只有 3–4 MB/s 而丢帧 → 解码器跟不上。这两个结论对用户的意义完全不同
（前者该换 NAS，后者该换设备/降码率）。
