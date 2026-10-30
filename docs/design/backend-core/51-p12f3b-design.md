# P12-F3B · 加密轨识别：从 sample flag 改判轨道格式键

- 版本：V1.0
- 日期：2026-10-30
- 上游：`49-p12f3-design.md`（F3 主设计）、`50-p12f3-status.md`（3/4，加密轨 `NOT_VERIFIED`）
- 结论先行：**F3 的加密轨不是"平台不报"，是我们读错了位置。平台一直报，报在轨道格式里。**

---

## 一、为什么会判错（事实链）

`50-p12f3-status.md` 当时写的是：

> CENC 片源在 K40 上 `is_encrypted=false` ⇒ `submit()` 的拒绝分支不可达 ⇒ G9 盲点（平台没告诉我们它加密了）。

这句话的前半段对，后半段错。实测补全（`hdr-probe/3` 的 `track_format_dump`）：

```
mime: string(video/avc), durationUs: int64(3100000),
crypto-mode: int32(1), crypto-default-iv-size: int32(8),
crypto-encrypted-byte-block: int32(0), crypto-skip-byte-block: int32(0),
crypto-key: data, ...
```

**平台把加密信息完整地给了我们** —— 只是没放在 `AMediaExtractor_getSampleFlags()` 的 `ENCRYPTED(2)` 位上。
我们只读了那一位，于是把一条加密轨读成了明文轨。

三种形态都验证过，全部一致：

| 变体 | 容器形态 | `crypto-mode` | sample flag `ENCRYPTED` |
| --- | --- | --- | --- |
| `negative_cenc_h264.mp4` | 非分片，**无** `pssh` | 1 | false |
| `negative_cenc_pssh.mp4` | 非分片，**手工注入** `pssh` | 1 | false |
| `negative_cenc_frag.mp4` | 分片（4 × `moof`） | 1 | false |

⇒ **「缺 `pssh`」被证伪，「分片不支持」被证伪。** 唯一成立的是「sample flag 不设 ENCRYPTED 位」。

> 顺带排除了一次自身错误：`AMEDIAEXTRACTOR_SAMPLE_FLAG_ENCRYPTED = 2` 已对着
> NDK 30 的 `NdkMediaExtractor.h` 核过，`flags & 2` 的读法没问题。不是我们算错了常量。

---

## 二、判据选择

| 候选 | 可取性 | 结论 |
| --- | --- | --- |
| `AMediaExtractor_getSampleFlags() & ENCRYPTED` | 实测恒 false | **弃**（正是原 bug） |
| `crypto-key`（data） | 需要 `AMediaFormat_getBuffer`，**API 28**，而 minSdk = 26 | 只在 API ≥ 28 上加佐证，不作主判据 |
| **`crypto-mode`（int32）** | API 21 起就有，实测三份样本全给 | **主判据** |
| `crypto-default-iv-size` | 同上，作佐证 | 记录，不单独判 |
| 自己解析 `stsd` 找 `encv` | 要绕过 NDK 读原始盒子，等价于自写解封装 | 弃（F3 已否决过启发式） |

**为什么不是启发式**：`crypto-mode` 是平台给的**显式声明**，不是我们从帧数/码率猜的。
F3 主设计里拒绝的是"按帧数偏少判加密"那种，不是这个。

---

## 三、落位（家族树）

| 文件 | 唯一职责 | 明确不做 |
| --- | --- | --- |
| `source/encryption_key_names.hpp` | `crypto-*` 键名常量 + `crypto_mode_name()` | 不解读语义 |
| `source/encryption_facts.hpp` | `TrackEncryption`（纯数据）+ `EncryptionAssessment` | 不含 `AMediaFormat` |
| `source/encryption_facts.cxx` | `assess_encryption()` —— 纯判定，桌面可单测 | 不读平台 |
| `source/mediaformat_encryption.{hpp,cxx}` | `read_track_encryption(const AMediaFormat*)` | 不判定，只取值 |

改动点：

- `MediaExtractorSource`：新增 `encryption_` 成员 + `track_encryption()` getter；open 选定轨道后读一次
  （和 `info_` 同一时刻，理由相同：**轨道格式只在 open 期间可靠**）。
- `DecodeSession::open()`：`validate_decoder_format()` 之后立刻检查加密 → 拒绝。
  **必须在 `configure()` 之前**：加密轨根本不值得配解码器，配了只会得到垃圾帧。
- `submit()` 里原有的 sample flag 拒绝**保留**，作为第二道防线；注释补一句
  "K40 上这位不设，真正起作用的是 open 时那次检查"。
- `SessionOutcome::Code` 新增 `kEncryptedUnsupported = 11`。
- 证据：新增顶层 `source` 段（`encryption.present / mode / verdict / reason`），schema → `decode/12`。

---

## 四、判据（F3 第 4 条怎么算达成）

1. 加密样本 `open()` 返回 **非 0**，`reason` 非空；
2. `reason` 必须**点名加密**（含 `encrypt` 或 `crypto`），不能只说"打不开"；
3. 证据文件 `stage` 必须是 `open`（不是 `play`），证明是**开门就挡**而不是放到一半出错；
4. 明文样本（`probe_seek_av_1080p30.mp4`）必须**不受影响**：`encryption.present=false`、播放正常。

第 4 条是防"一刀切"的：只测加密样本会得到"全部拒绝"，那也是假通过。

---

## 五、已知边界（写清楚，不掩饰）

1. **只覆盖 CENC 家族**（`cenc`/`cbcs` 都走 `crypto-mode`）。HLS 的 AES-128（整段加密，不是 sample 级）
   不走这套键，本卡不覆盖。
2. **`crypto-key` 存在但 `crypto-mode` 缺席**的容器会漏判。实测三份样本未出现，但这是平台实现差异，
   所以判定结果里保留 `key_present` 字段供复算。
3. **拒绝时机是"打开即拒"**，不是"播到加密包才拒"。对纯 CENC 文件两者等价；对"部分样本加密"的文件
   我们会整条拒掉 —— 这是保守选择，写在这里以备后查。
