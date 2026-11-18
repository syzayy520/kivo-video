# D12-H 状态：二分定论 —— VC-1 能播，`.ts` 不能

日期：2026-11-18
上游：`D12G-STATUS.md` §二「33.8 GB `.ts` 仍读不出来」
方法：同目录找另一个 VC-1 片源做二分

---

## 一、二分结果

| 片源 | 容器 | 编码 | 结果 |
|---|---|---|---|
| `A Beautiful Mind 2001 BluRay REMUX 1080p VC-1 ...` （33.8 GB） | **`.ts`** | VC-1 | ❌ `Failed to recognize file format` |
| `[眩晕].Vertige.2009.FRA.BluRay.REMUX.1080p.VC-1 ...` | **`.mkv`** | VC-1 | ✅ **正常播放** |

两者**同为 VC-1、同为 1080p、同为 REMUX、同一 NAS 同一共享**，唯一差别是容器。

⇒ **VC-1 编码没问题，`.ts` 容器有问题。**

---

## 二、真 VC-1 实测（补 D12F 未覆盖项 #1、#3）

D12F 当时只能用 `wmv2` 做等价验证（ffmpeg 无 VC-1 编码器），并标注"不等于真 VC-1 能播"。
这次用 NAS 里的真 VC-1 片源补上了：

```
● Video  --vid=1  --vlang=fr  'QSD' (vc1 1920x1080 24 fps) [default]
Codec list:
    vc1 - SMPTE VC-1
Opening decoder vc1
Not trying to use hardware decoding: codec vc1 is not on whitelist.
Selected decoder: vc1 - SMPTE VC-1
```

### 性能（91.5 s 长稳，K40）

| 指标 | 值 |
|---|---|
| `estimated_vf_fps` | 恒 **24.0**（源帧率） |
| `vo_drop_count` | **恒 0** |
| `decoder_drop_count` | 恒 0 |
| `avsync_s` | −0.003 ~ −0.023（全程 <25 ms） |
| SF 帧间隔 p50 / p99 | 16.52 / 16.70 ms |
| **SF >20 ms 次数** | **0** |
| 等效上屏 | **60.54 fps**（24 fps 源经插值） |

⇒ **VC-1 1080p 软解在 K40 上完全够用**：零丢帧、零可见卡顿、插值到 60 fps 满帧。
"软解能力不足"这个顾虑可以排除。

---

## 三、对 D12-B3 的最终裁决

现在三条证据齐了：

1. 默认 mpv 路径的软解通路正常（D12F，`wmv2`）；
2. **真 VC-1 软解实测零丢帧**（本批）；
3. legacy 回退路径给的是明确失败 + 可读原因，不是黑屏。

⇒ **D12-B3 维持 P2**。它不是"能力缺失"，而是"legacy 回退路径没接软解会话"，
而 legacy 只在 `debug.kivo.engine=legacy` 或开「后台继续播放音频」时启用。

---

## 四、`.ts` 问题的新边界（仍未解，但范围缩小了）

现在可以确定排除的：**VC-1 编码**（§一）、**文件大小**（3 Idiots 38.4 GB 能播）、
**续播远 seek**（D12G 已修，`options=""` 仍失败）。

剩下只有两种可能：

1. **该 `.ts` 文件本身在 NAS 上不可读**（损坏 / 被占用 / 权限）；
2. **`.ts` 容器在本项目的 SMB 链路上有问题**（无索引 ⇒ mpv 要连续读一大段才能确认格式，
   而 SMB 侧 296 ms 就 RST）。

区分办法（下一步）：**在 NAS 上放一个自己生成的合法 `.ts`**（`ffmpeg -c copy` 或直接录一段 mpegts），
用它走同一条链路播 —— 能播就是"那个文件的问题"，不能播就是"`.ts` + SMB 链路的问题"。

> 注意：不要用本机 `ffmpeg` 生成完直接推设备本地测，那会绕过 SMB 链路，测不出问题。

---

## 五、一个附带发现（文件名）

`MpvHealthSoak` 的文件名清洗规则 `[^A-Za-z0-9._-] → _` 会把中文片名整段抹掉：

```
[眩晕].Vertige.2009...  →  ____.Vertige.2009...-health.ndjson
```

不算 bug（文件仍可定位），但同一个中文片名的两部影片会撞名。**记为待改**，本批未动。

---

## 六、证据清单

| 文件 | 内容 |
|---|---|
| `d12h-vc1-health.ndjson` | VC-1 软解 91.5 s 时间序列（19 点，`hwdec_current=no`） |
| `d12h-sf.txt` | 同场 SF 6 轮 |
| `d12h-vertige.png` | VC-1 出画截图 |

---

## 七、未覆盖

1. `.ts` 问题**未解**，区分实验（自造合法 `.ts` 放 NAS）未做。
2. 只测了 1080p VC-1，**4K VC-1 未测**（NAS 里未见）。
3. 中文片名清洗撞名未修。
