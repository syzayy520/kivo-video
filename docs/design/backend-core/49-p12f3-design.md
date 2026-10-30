# P12-F3 设计：错误路径全覆盖 —— 四条负向路径，每条都要把话说到证据里

- 版本：V1.0
- 日期：2026-10-30
- 卡片判据：**坏文件 / 加密轨 / 无音轨 / 无视频轨，均给非空 reason**
- 关联：`48-p12f2a-status.md` §六（F2A 给 F3 的三条）

---

## 一、先把"给了 reason"分成两种，只有一种算数

| 形态 | 算不算 | 为什么 |
| --- | --- | --- |
| `KIVO_LOGW("open failed: ...")` | ❌ 不算 | 日志会滚掉、不进证据、第三方无法复算 |
| 证据文件里的 `"reason": "..."` | ✅ 算 | 可提交、可比对、可归档 |

所以这张卡的核心不是"加 try/catch"，而是**让每一条负向路径都落到一份可命名的证据文件里**，
并且**文件名能区分是哪一条**。当前的失败证据只有一个固定名 `p12b-failure.json`，
四条路径会互相覆盖 —— 那等于只有最后一条有证据。

---

## 二、四条路径现状（先摸清，再动）

| 用例 | 触发 | 现状 | 缺口 |
| --- | --- | --- | --- |
| **坏文件** | 截断的 mp4 | `extractor.open_fd` 失败 → `open()` 返回 `kOpenFailed` → 应用写失败证据 | 证据名固定、schema 是 P12-B 的 `/1`、没有 `case` 字段 |
| **无视频轨** | 只有音轨的 mp4 | 同上（`TrackKind::kVideo` 找不到轨） | 同上 |
| **加密轨** | CENC 加密的 mp4 | 打开**成功**；`submit()` 里 `is_encrypted` 拒绝 → 失败槽 → 证据 `outcome.error` | **从未真机验证过**这条分支可达 |
| **无音轨** | 只有视频轨的 mp4 | 不算失败：降级到 `system` 时钟，证据里 `av_drift` 带 `unavailable_reason` | reason 在 sync 证据里，没和另外三条放在同一张表上 |

**结论**：代码里的判断大多已经存在，缺的是"**统一、可命名、可校验的负向证据**"。
F3 的活主要是取证工程，不是加分支。

---

## 三、做法

### 3.1 失败证据改造（唯一改代码的地方）

`PlayerActivity.writeFailureEvidence`：

```
文件名：p12f3-<case>-failure.json      （case 来自 --es kivo.case，缺省则退回旧名）
schema：kivo.p12f.negative/1
字段：case / stage / return_code / media / reason
```

**为什么要 `case`**：没有它，四条路径产出四份同名文件，跑完只剩最后一份；
而"只剩最后一份"这件事在证据目录里看不出来 —— 那是最坏的一种缺失。

### 3.2 四个用例的片源

| 用例 | 片源 | 生成方式 |
| --- | --- | --- |
| `bad-file` | `tests/real/negative_truncated.mp4` | 真片源截前 4 KB（容器头不完整） |
| `no-video-track` | `tests/real/negative_audio_only.mp4` | `-f lavfi -i sine` + `-c:a aac`，无视频轨 |
| `encrypted` | `tests/real/negative_cenc_h264.mp4` | `-c copy -encryption_scheme cenc-aes-ctr -encryption_key ... -encryption_kid ...` |
| `no-audio-track` | `tests/real/probe_seek_1080p30.mp4`（已有） | 纯视频，正常播放，看降级 reason |

**加密片源的诚实边界**：这是 **CENC 加密但没有许可证服务器**的样本 —— 我们想验证的是
"遇到加密样本会拒绝并说清原因"，**不是**"支持 DRM"。播放器本来就没有 MediaCrypto 路径
（`submit()` 里已写明"当前未实现"），这张卡只证明那条分支可达、且理由可读。

### 3.3 校验：`tools/verify_p12f3_errors.py`

输入四份证据 + 一句"期望表"（写在脚本里，不靠人眼），逐条断言：

| 用例 | 期望 `stage` | 期望 reason 关键词 |
| --- | --- | --- |
| `bad-file` | `open` | `extractor` 或 `malformed` |
| `no-video-track` | `open` | `video`（"no video track" 一类） |
| `encrypted` | 无（运行时失败，看 `outcome.error`） | `encrypted sample is not supported` |
| `no-audio-track` | 无（降级不是失败） | `no audio track` / `unavailable`（在 sync 证据的 `av_drift` 里） |

判据三条：
1. **每份证据的 reason 非空**（空串一律判失败 —— 那正是 G9 要防的"静默"）；
2. **关键词命中**：reason 要指出"是哪种问题"，不能只说 "failed"；
3. **四份文件都存在且 case 与文件名一致**（防"跑漏一条还显示通过"）。

### 3.4 家族树

| 改动 | 位置 |
| --- | --- |
| `kivo.case` extra + 证据命名 + 新 schema | `ui/PlayerActivity.kt`（`writeFailureEvidence` 与其调用点） |
| 四条负向片源 | `tests/real/negative_*.mp4` |
| 校验脚本（**新**） | `tools/verify_p12f3_errors.py` |
| 证据 | `docs/evidence/p12f3/` |

### 3.5 明确不做

- **不做真实 DRM**（无 license server、无 MediaCrypto）：只验证"拒绝且说清"。
- **不枚举所有损坏形态**：只做"容器头不完整"这一种，其余形态（错码流、坏索引）留给以后。
- **不改 `submit()` 的加密判断**：它已经是对的，F3 只补证据。
- **不做网络错误**（HTTP 404 / 超时）：本阶段没有网络源。

---

## 四、风险

| 风险 | 处置 |
| --- | --- |
| 截断文件恰好还能被解析 → 用例不成立 | 先本机 `ffprobe` 确认解析失败，再上机；不成立就换截断长度 |
| CENC 片源被解封装器直接判成坏文件 → 走成 `bad-file` 那条 | 那也算"说清了"，但**必须改口径**：证据里写清它其实是被当坏文件拒的，不能宣称"验证了加密分支" |
| 无音轨那条其实没有 `unavailable_reason` | 先在 sync 证据里确认字段名，再写断言；断言写错比没断言更糟 |
| 四条路径里有一条没有证据文件 | 校验脚本对"文件缺失"单独报错，不算通过 |
