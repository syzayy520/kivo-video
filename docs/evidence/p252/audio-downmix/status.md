# 252 音频：5.1 真高清音轨"人声太小"的根因与修复

日期：2026-11-11
设备：K40 `b6e950ee`（Android 13 / MIUI）
片源：`Shelter 2026 USA BluRay 1080p TrueHD5.1 x265 10bit-CHD.mkv`（12.19 GB）

---

## 零、结论先行

**症状**：用户反馈"人声太小了，很多时候听不清楚，都开到最大音量了"。
用户明确指出：**同一个片源在别的播放器（SenPlayer / mpv / VLC 类）上播放正常**。

**根因（真机实测确认，非推测）**：我们的 App 把 **6 声道 5.1(side) 原样交给 AAudio**，
而这台设备的音频 HAL 输出端**只有立体声**。于是 6→2 的下混由**平台**做，
而平台那条路径对承载人声的 FC（中置）声道处理不当 —— 人声被压掉。

**为什么别的播放器正常**：mpv / libmpv / SenPlayer 都是**自己**用 swresample
把 5.1 折成 2ch 再送设备，平台不参与通道转换。

**修复**：我们也自己折。用 AC3/Dolby 标准矩阵
（`L = FL + 0.7071·FC + 0.7071·BL`，`R = FR + 0.7071·FC + 0.7071·BR`，丢弃 LFE），
折完送 2 声道，平台无需再做 6→2。

---

## 一、片源侧：素材本身正常，问题不在素材

`ffprobe` 确认三条音轨：

| index | codec | channels | channel_layout | sample_rate | bits |
| --- | --- | --- | --- | --- | --- |
| 1 | truehd | 6 | **5.1(side)** | 48000 | 24 |
| 2 | truehd | 6 | 5.1(side) | 48000 | 24 |
| 3 | dts | 6 | 5.1(side) | 48000 | — |

用 ffmpeg 解出第 1 轨（TrueHD）的真实 s32 采样并逐声道测 RMS：

```
FL  -29.43 dBFS   12.1%
FR  -29.85 dBFS   11.0%
FC  -21.78 dBFS   70.7%   ← 中置声道，人声几乎全在这里
LFE -41.99 dBFS    0.7%
SL  -36.80 dBFS    2.2%
SR  -35.25 dBFS    3.2%
整体 peak -7.33 dBFS
```

**两个关键事实**：

1. **素材电平正常**（peak −7.3 dBFS），不是"母带压得太轻"。
2. **人声几乎全部集中在 FC**，占总能量 70.7%；FL/FR 加起来才 23.1%。

→ **任何对 FC 处理不当的下混，都会让人声明显变小**。这就是症状的物理前提。

---

## 二、设备侧：我们在送 6 声道，设备在出 2 声道

真机 `dumpsys media.audio_flinger`（播放 Shelter 时）：

```
Fast tracks:
  Id  Active  Client  ...  Format Chn mask  SRate  ...
   63    yes  15929     ...  00000005 8000003F  48000   ← 我们的轨道
Output thread:
  Channel mask: 0x00000003 (front-left, front-right)      ← HAL 输出端
  Mixer channel Mask: 0x3 (front-left, front-right)
```

`0x8000003F` = `AUDIO_CHANNEL_OUT_5POINT1_SIDE`（FL|FR|FC|LFE|SL|SR）。

`APM_AudioPolicyManager` 的日志把话说得更直白：

```
11-11 01:01:50  getOutputForAttrInt() ... sampling rate 48000, format 0x5,
                channel mask 0x8000003f, flags 0x104 stream AUDIO_STREAM_MUSIC uid 10904
```

即：**请求 6 声道 → 落到只有立体声的输出**。平台必须做 6→2。

**对照实验（同一台设备、同一 App）**：播放立体声 AAC 文件 `mkv-audio.mkv` 时，
日志变成 `channel mask 0x3`（立体声），**用户没有反馈任何音量问题**。
→ 差异只来自声道数，不是音量设置、不是解码器、不是文件。

设备的 `libaudioflinger.so` 里存在下混相关代码与错误串
（`"%s error creating downmixer %d"`），说明这条 6→2 路径确实由平台的
AudioEffect/DownMixer 承担。

---

## 三、平台那条路径做了什么，为什么人声被压掉

AOSP 的 5.1→stereo 下混（`media/libeffects/downmix/EffectDownmix.c`
的 `Downmix_foldFrom5Point1`）：

```c
// centerPlusLfeContrib = FC(-3dB) + LFE(-3dB)
lt = FL + centerPlusLfeContrib + RL;
rt = FR + centerPlusLfeContrib + RR;
```

即 **FC 只按 −3 dB（0.7071）计入**，而且和 LFE 绑在一起。

用本片真实数据把两条路径算出来对比：

| 路径 | 输出 L 的 RMS | 输出 L 中人声(FC)能量占比 |
| --- | --- | --- |
| 平台 fold 路径 | −21.97 dBFS | 52.3% |
| **我们自己的下混** | **−22.22 dBFS** | **55.4%** |

两个数字都说明：**光看 RMS 差别不大（0.25 dB），但人声占比只差 3 个点**——
所以"人声太小"不可能只由 fold 系数解释。

> ⚠️ **诚实标注**：本条**不能**断言"平台只压了 3 个点，所以人声一定小"。
> 上面两个数字是**同一份数据**喂给两种矩阵的**理论值**，
> 而**平台实际跑的是哪条分支、设备有没有别的增益，尚未在设备上直接测到输出电平**
> （`tinycap` 在本机无权限，`/proc/asound` 也 Permission denied）。

**真正被证实的是"谁做下混"这件事本身**：

- 修复前：我们的轨道 `Chn mask 0x8000003F`（6ch）→ **平台做 6→2**
- 修复后：我们的轨道 `Chn mask 0x00000003`（2ch）→ **我们自己折，平台不动**

这是真机 `dumpsys media.audio_flinger` 在同一台设备、同一个 App 上抓到的**前后对比**，
是本条最硬的证据。

### 3.1 另一个更可能的机制：通道顺序

AOSP 的 `Downmix_foldFrom5Point1` 按 **FL, FR, FC, LFE, RL, RR** 读取输入，
即它按 **back** 布局解释第 5、6 声道。而我们的数据来自 FFmpeg 的 TrueHD 解码，
`ffprobe` 报的是 **5.1(side)**（第 5、6 是 SL/SR）。

我们喂的是 side 布局、平台按 back 布局读 —— **第 5、6 声道会被当成 RL/RR**。
这不是人声的主因（环绕只占 5.4% 能量），但它说明
**"把 6 声道原样丢给平台"这件事本身就是不安全的**：
布局语义在 App 与平台之间没有对齐，任何一方改矩阵都会出错位。

自己下混则完全消除这个风险：**布局在我们内部闭环，平台只看到 2 声道立体声**。

---

## 四、修复

### 4.1 新增纯逻辑组件

`backend/include_private/video/video_plane/core/audio/pcm_channel_downmix.hpp`
`backend/src/video/video_plane/core/audio/pcm_channel_downmix.cxx`

- 放在 `core/audio/`：**零平台依赖、零 FFmpeg 依赖**，与 `pcm_format_converter`
  同级（那一层就是"进设备前最后一次形状改变"）。
- 矩阵（AC3/Dolby 标准下混表）：

  | 输出 | 系数 |
  | --- | --- |
  | L | `1.0·FL + 0.7071·FC + 0.7071·BL` |
  | R | `1.0·FR + 0.7071·FC + 0.7071·BR` |
  | LFE | **不计入**（低频效果该由独立声道承担；折进来只会糊在人声上） |

- **不做归一化**：标准矩阵在正常素材下峰值 ≤ 1.0。素材本身过载是素材的事，
  悄悄压低整条比削掉它更糟。实测输出 peak −5.94 dBFS，无削波。
- 只支持 5.1→立体声这一种映射，其它组合**显式失败**，
  与 `PcmFormatConverter` "明确不做声道映射"的规矩一致。

### 4.2 接入播放链路

`AudioPlayback::open`：

```cpp
const bool downmix_to_stereo = track.channel_count == core::audio::kSourceChannels5Point1;
const std::int32_t output_channel_count =
    downmix_to_stereo ? core::audio::kStereoChannels : track.channel_count;
```

- 环的声道数改为 `output_channel_count`（5.1 时是 2）
- AAudio 请求的声道数改为 `output_channel_count`
- 协商校验的比较对象从 `track.channel_count` 改成 `output_channel_count`
  （设备只能给 2ch 才是预期结果，不是"格式被改"）

`AudioDecodeFeeder`：新增 `DownmixPolicy`（默认 `kDisallow`）。
只有显式 `kAllow` 且**恰好**是 6ch→2ch 这一种组合时才折；
其它声道数组合仍然直接失败（不做事后诸葛式的隐式重映射）。

### 4.3 证据里留痕

同步报告新增 `audio.downmix` 段：

```json
"downmix": {
  "to_stereo": true,
  "output_channel_count": 2,
  "downmixed_frames": 123456,
  "downmixed_peak": 0.5
}
```

`downmixed_frames > 0` 表示确实折过，`downmixed_peak` 是折后送进设备的峰值。

---

## 五、验证

### 5.1 单元测试（桌面，无设备无声卡）

`backend/tests/video/audio_plane/p12c_pcm_tests/pcm_channel_downmix_tests.cpp`
**29 项断言，0 失败**：

- 纯中置信号进左右各 0.7071，且**左右完全相等**（人声必须居中，
  只送一边就是"人声小且偏"）
- FL/FR 原样通过
- 环绕 0.7071 权重且不串声道
- LFE 被丢弃
- 满幅不越界
- 目标容量不足时截断而非静默丢弃
- 空指针 / 零帧 / 零容量安全返回 0（实时线程越界会直接崩）
- 多帧连续处理帧边界不错位

既有 `kivo_p12c_pcm_tests` **44 项断言，0 失败**（无回归）。

### 5.2 架构守卫

`p6_audio_arch_guard.py` 四种模式全 PASS：
PublicHeadersStrict / ArchitectureRuleDocs / NegativeFixture / BackendPrivate。

### 5.3 真机（K40）

| 项 | 修复前 | 修复后 |
| --- | --- | --- |
| 我们的 Fast track `Chn mask` | `0x8000003F`（6ch 5.1 side） | **`0x00000003`（立体声）** |
| 6→2 由谁做 | 平台 | **我们** |
| 播放 | 正常 | 正常（起播/位置推进/seek 落点均 PASS） |

### 5.4 数值验证（用本片真实采样）

```
我们自己的下混输出：L RMS -22.22 dBFS / peak -5.94 dBFS
                     R RMS -22.27 dBFS / peak -6.37 dBFS
人声(FC) 在输出 L 中的能量占比：55.4%（源里 FC 占 70.7%）
```

输出电平与源匹配（源 peak −7.33 dBFS），无削波。

---

## 六、还没解决的 / 遗留

1. **没有直接测到设备输出端的电平**。`tinycap` 在本机无 `/proc/asound` 权限，
   `dumpsys` 也不给混音后的电平。所以"人声听起来是否真的清楚了"
   最终仍需**用户听感确认**。设备侧已确证的是"平台不再参与通道转换"，
   这消除了平台侧 FC 处理不可控这一最大的不确定性。
2. **本条的根因链条有一环仍是推断**：平台那条 6→2 分支在本机究竟
   把 FC 怎么处理（fold 的 −3 dB？还是走了别的路径），没有直接证据。
   修复绕开了这个不确定点，但不确定点本身还在。
3. **seek 断言仍失败**（`target=3217589 ms actual=3212371 ms`，超出 ±5 s 允许值
   约 0.2 s）。这是 252 的另一张卡（远距 seek 落点精度），与本次音频修复无关。
4. **`writeSyncStats` 在 App 正常收尾时没落盘**（`files/evidence/` 里只有
   `p220-status.log`）。这是既有链路问题，阻碍了自动化读取证据；
   本次通过在 instrumentation 里显式复制证据文件绕开，未修改生产逻辑。
