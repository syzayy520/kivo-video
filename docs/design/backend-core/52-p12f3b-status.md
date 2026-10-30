# P12-F3B · 加密轨识别收口（F3 由 3/4 变为 4/4）

- 版本：V1.0
- 日期：2026-10-30
- 上游：`49-p12f3-design.md`（主设计）、`50-p12f3-status.md`（3/4，加密轨 `NOT_VERIFIED`）
- 设计：`51-p12f3b-design.md`
- **结论：F3 达成 4/4。原判定「平台不报加密」是错的 —— 平台一直报，我们读错了位置。**

---

## 一、判据与结果

| 用例 | 结果 | 证据 / 关键读数 |
| --- | --- | --- |
| `bad-file` | ✅ | `container reports 0 tracks — the file is likely truncated or corrupt` |
| `no-video-track` | ✅ | `no video track among 1 tracks` |
| `no-audio-track` | ✅ | `source has no audio track; there is no audio timeline to compare against` |
| **`encrypted`** | ✅ **本轮补上** | `stage=open`、`return_code=12`、`track declares crypto-mode=1 (aes-ctr); sample-level encrypted tracks are not supported` |

`python tools/verify_p12f3_errors.py docs/evidence/p12f3` → **4 条 PASS，退出码 0**。

变异测试（把加密证据的 `return_code` 改成 5）：**FAIL**、退出码 1 —— 校验器不是"永远通过"。

---

## 二、根因：三层实测把一句话推翻了

`50-p12f3-status.md` 原文：

> CENC 片源在 K40 上 `is_encrypted=false` ⇒ `submit()` 的拒绝分支不可达 ⇒ G9 盲点（平台没告诉我们它加密了）。

前半句对，后半句错。补三步实测：

### 第 1 步：`pssh` 不是原因

本机没有 MP4Box / Shaka，ffmpeg 的 `-encryption_scheme cenc-aes-ctr` 只写
`encv/sinf/tenc/senc`，**从不写 `pssh`**。于是先写 `tools/inject_pssh.py` 手工注入
（插在 `moov` 内 `udta` 之前 —— 这份文件 `mdat` 在 `moov` 前，`stco` 偏移不用改）：

| 变体 | 容器形态 | 首包 `is_encrypted` |
| --- | --- | --- |
| `negative_cenc_h264.mp4` | 非分片，无 `pssh` | false |
| `negative_cenc_pssh.mp4` | 非分片，**注入** `pssh` | **仍 false** |

⇒ 「缺 `pssh`」被证伪。

### 第 2 步：不是"分片才支持"

| 变体 | 形态 | 首包 `is_encrypted` |
| --- | --- | --- |
| `negative_cenc_frag.mp4` | 分片（4 × `moof`，`senc` 在 `traf` 里） | **仍 false** |

⇒ 「非分片不支持」被证伪。（`schm` 的 `scheme_type` 也核过：三份都是 `cenc`。）

### 第 3 步：也不是我们读错了标志位

`AMEDIAEXTRACTOR_SAMPLE_FLAG_ENCRYPTED = 2`，对着 NDK 30 的 `NdkMediaExtractor.h` 核过，
`flags & 2` 的读法没问题。

### 第 4 步：真正的原因 —— 平台把加密信息放在**轨道格式**里

给探针加了 `track_format_dump`（`AMediaFormat_toString`，API 28，走 `dlopen` +
按句柄 `dlsym` —— E3 那次 `RTLD_DEFAULT` 返回 null 的坑不重踩）：

```
mime: string(video/avc), durationUs: int64(3100000),
crypto-mode: int32(1), crypto-default-iv-size: int32(8),
crypto-encrypted-byte-block: int32(0), crypto-skip-byte-block: int32(0),
crypto-key: data, ...
```

**平台报得清清楚楚。** 只是不往 sample flags 上设 `ENCRYPTED` 位。

> 这也纠正了 F3 主设计里那句"读 `crypto-key` 没有" —— 实测**有**。
> 当时没查，是因为推理代替了观测；这次查了，结论就反过来了。

---

## 三、改了什么

### 判据：sample flag → 轨道格式 `crypto-mode`

| 候选 | 结论 |
| --- | --- |
| `getSampleFlags() & ENCRYPTED` | 弃（K40 恒 0，正是原 bug） |
| `crypto-key`（data） | 只作旁证：读它要 `AMediaFormat_getBuffer`，**API 28**，minSdk = 26 |
| **`crypto-mode`（int32）** | **主判据**：API 21 起就有，三份样本全给 |

### 拒绝时机：`open()` 里，`configure()` 之前

不在 `submit()` 里挡（那里也有一道，保留作第二防线）的理由：

原来那条路的结果是「配好解码器 → 解出 25/90 帧垃圾像素 → `completed=true`、`error=""`」。
**那比拒绝更糟** —— 它看起来像播放成功。加密轨根本不值得配解码器。

### 三态判定（这是本卡最重要的设计）

| `crypto-mode` | verdict | 行为 |
| --- | --- | --- |
| `1` / `2` / 其它非 0 | `encrypted` | **拒绝**，带 reason |
| `0` | `clear` | 放行 |
| 键缺席 | **`unknown`** | 放行，但证据里留 reason |

`unknown` 这一态是必须的：K40 上**明文片源同样没有这个键**（`probe_seek_av_1080p30.mp4`
实测 `crypto_mode_present=false`）。把它写成 `clear` 会让"平台改键名"静默变成"可播"；
把它写成拒绝会让所有明文片源全挂。

---

## 四、实测数据

### 加密：开门即拒

```json
{
  "schema": "kivo.p12f.negative/1",
  "case": "encrypted",
  "stage": "open",
  "return_code": 12,
  "media": "neg_cenc.mp4",
  "reason": "open(neg_cenc.mp4): track declares crypto-mode=1 (aes-ctr); sample-level encrypted tracks are not supported — no key/DRM session is set up, default IV size 8 bytes"
}
```

### 明文对照：不受影响（防"一刀切"）

| 项 | 值 |
| --- | --- |
| `source.encryption.crypto_mode_present` | false |
| `source.encryption.verdict` | `unknown` |
| `source.encryption.reason` | `track format carries no crypto-mode key…playback continues, but this run cannot prove the track was clear` |
| 帧 | 300 送 / 301 收 / **300 上屏** / 0 丢帧 |
| `outcome.completed` | true |

只测加密样本会得到"全部拒绝"，那也是假通过 —— 所以这一轮必须跑。

---

## 五、诚实清单（六条，不许省）

1. **只覆盖 sample 级 CENC**（`cbc1/cens/cbc2` 也走 `crypto-mode`）。HLS 的 AES-128
   整段加密不走这套键，本卡**不覆盖**，也没有样本。
2. **`crypto-key` 只记"键在不在"，没读它的值** —— 读 data 要 API 28，minSdk 是 26。
   若某平台给 `crypto-key` 却不给 `crypto-mode`，会漏判（记为 `unknown`，仍可播）。
   这是已知边界，字段名写的是 `key_declared` 而不是 `has_key`。
3. **`unknown` 是"证不了"，不是"清白了"**。明文片源的证据里会长期带这句话 ——
   这是刻意的，别把它当噪声删掉。
4. **拒绝是整条轨拒**。对"部分样本加密"的文件，我们会连明文样本一起拒 —— 保守选择。
5. **`inject_pssh.py` 只在 `mdat` 位于 `moov` 之前的布局上安全**（脚本里写死了这个前提）。
   换别的布局会算错 `stco` 偏移，脚本不做通用化。
6. **没有真 DRM 样本**（Widevine/PlayReady 的许可证流程）。本卡证明的是"声明了就拒"，
   不是"能播 DRM"。

---

## 六、复算步骤（第三方可独立重跑）

```bash
# 1. 容器层：确认三份变体的 box 结构与 schm
python - <<'EOF'
import pathlib
for n in ('negative_cenc_h264','negative_cenc_pssh','negative_cenc_frag'):
    d = pathlib.Path(f'tests/real/{n}.mp4').read_bytes()
    print(n, 'pssh=', d.count(b'pssh'), 'moof=', d.count(b'moof'),
          'senc=', d.count(b'senc'), 'encv=', d.count(b'encv'))
EOF

# 2. 设备层：轨道格式里到底有没有 crypto-mode
adb push tests/real/negative_cenc_h264.mp4 /data/local/tmp/cenc.mp4
adb shell "/data/local/tmp/kivo_hdr_probe /data/local/tmp/cenc.mp4"   # 看 track_format_dump

# 3. 应用层：加密拒绝 + 明文对照
adb shell "am start -n com.kivo.cinema/.ui.PlayerActivity \
  --es kivo.media $MEDIA/neg_cenc.mp4 --es kivo.case encrypted \
  --es kivo.evidence p12f3-encrypted.json"
adb shell "run-as com.kivo.cinema cat files/evidence/p12f3-encrypted-failure.json"

# 4. 四条用例总校验（退出码必须为 0）
python tools/verify_p12f3_errors.py docs/evidence/p12f3
```

---

## 七、F4 交接（总收口要注意的）

- **证据 schema 已是 `decode/12`**。F4 若再加字段，继续升号，别复用 `/12`。
- **`docs/evidence/p12f3/superseded/` 里的两份是"加密轨被当明文播完"的旧证据**，不要删 ——
  它们是 F3B 全部改动的理由。任何人质疑"为什么要读 `crypto-mode`"，看这两份即可。
- **`unknown` 会出现在每一份明文证据里**。F4 汇总时别把它当异常统计；
  要统计的是 `verdict != "encrypted"` 且 `open` 成功的比例。
- **`return_code` 12 是加密专用**，5 是普通打开失败。F4 的错误分类表要按这个分。
