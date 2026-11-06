# 210 · DV-A：识别 + 透传 —— 设计稿

> 状态：**设计完成，实现未开始**。总计划 M1 第一张卡（`docs/plan/00-product-master-plan.md` §4）。
> 决策依据：用户拍板 2026-11-06 —— DV A 档先行（识别+透传，对齐 SenPlayer 的实测判定：它没有自研映射层）；
> B 档（libdovi+libplacebo 真映射）是 233 卡，本卡一行都不做映射。
> 纪律：本稿先于实现；实现碰到的实测与本稿预期不符时，**更正块留在原位，不回写历史**（P12-E1 先例）。

---

## 一、一句话目标

DV 片源进播放器：**profile 8.x 按 HDR10 透传可播；profile 5 在 `open()` 显式拒绝且 reason 非空；
既有 SDR / HDR10 / HLG 判定逐位不变。**

## 二、现状（本卡站在哪里）

| 已有 | 位置 | 对本卡的意义 |
| --- | --- | --- |
| `HdrKind` 三值枚举 + `hdr_kind_name()` | `backends/mediacodec/decode/hdr_kind.hpp` | 加 `kDolbyVision`，枚举值只能往后加（P13-R 字段顺序教训同理） |
| `classify_hdr()` 纯判定 | `backends/mediacodec/decode/video_color_facts.cxx:113` | 判定与平台解耦、桌面可单测；DV 判定沿用这个形状（判定 + 依据串成对） |
| dataspace 下发 + 窗口回读 | `present/surface_dataspace.cxx`（`apply_surface_dataspace` / `read_surface_dataspace`） | 8.x 透传 = 复用 HDR10 的 BT2020_PQ 下发路径，E2/E3 已真机验证 |
| open() 早拒先例 + 专错误码 | `session/decode_session.hpp` `Code` 枚举（`kEncryptedUnsupported = 11`） | profile 5 拒绝照这个形状：**configure 之前拒绝**，专码，reason 点名 |
| 轨道格式 dump 探针 | `probe/hdr_probe_main.cxx` 的 `track_format_dump`（hdr-probe/3） | 本卡的"先测后定"工具：DV 片源轨道格式里到底有什么，打一份就知道 |
| 证据渲染 color 块 | `session/decode_stats_renderer.cxx:233`（`kind` / `kind_reason`） | DV 字段出版在相邻位置；**schema 不升版本**（红线 #4：加字段不升版本，`decode/19` 保持） |

## 三、检测机制 —— 设计假设，先测后定

**两条路，用一次实测取舍，两路的判定逻辑都保持纯函数：**

- **路 A（假设优先成立）**：平台直接报。Android 9+ 的框架 `MediaExtractor` 对 DV 轨可能报
  `mime = video/dolby-vision`（NDK `AMediaExtractor` 同框架）。若 K40 实测如此，
  识别 = 读 mime + 轨道格式键，**零容器解析**。
- **路 B（兜底）**：容器 box 解析。`stsd` 采样条目（`hvc1`/`dvh1`）里的 `dvcC` 配置盒。
  需要 fd 上的 MP4 box 游走，新单元，单职责"只读盒、不做判定"。

**实测步骤（实现期第一件事）**：用 `track_format_dump` 打一份 DV 片源，看轨道格式里有什么。
哪条路成立就走哪条，另一条的接口位不实现（YAGNI）。

### dvcC 载荷语义（对照来源：KSPlayer swiftinterface:875 `DOVIDecoderConfigurationRecord`）

`dv_version_major(8)` `dv_version_minor(8)` `dv_profile(7)` `dv_level(6)` `rpu_present(1)`
`el_present(1)` `bl_present(1)` `bl_signal_compatibility_id(4)`。
载荷逐位布局以 Dolby Vision MP4 规范为准，实现时与 Rex 主二进制里的 `ff_dovi_rpu_parse`
反编译产出（Sen/Ghidra 提取，见交接文档 §2）交叉对照。

| profile | 含义 | A 档行为 |
| --- | --- | --- |
| 8.x | BL 是 HDR10 兼容流（`bl_present=1`、`bl_signal_compatibility_id=1`），RPU 只是增强 | **透传**：`HdrKind::kDolbyVision` + dataspace 按 HDR10（BT2020_PQ）下发；RPU 忽略（无映射层就是 A 档的定义） |
| 5 | IPT 编码，**不向后兼容**（`bl_present=0`） | **拒绝**：`open()` 拒绝、错误码 `kDolbyVisionUnsupported = 12`、reason 点名 `dv_profile=5` |
| 0–4 / 6 / 7 / 9+ | 未在 A 档范围 | 拒绝 + reason 点名 profile（不静默当 SDR 播） |
| 键/盒缺席 | 不是 DV 片源 | **放行** + reason 留痕 —— F3B 的 `unknown` 第三态教训：明文片源同样缺键，第三态不能省 |

## 四、家族树（落位）

| 文件 | 新/旧 | 单一职责 |
| --- | --- | --- |
| `backends/mediacodec/decode/hdr_kind.hpp` | 旧（小改） | 加 `kDolbyVision = 3` + 名字 `"dolby_vision"` |
| `backends/mediacodec/decode/dv_config_facts.{hpp,cxx}` | **新** | dvcC 载荷结构 + 解析 + profile 判定（纯逻辑，无任何 Android 头，桌面可单测） |
| `backends/mediacodec/source/mp4_dvcc_reader.{hpp,cxx}` | **新（仅路 B 才建）** | 给 fd，找到 `dvcC`，返回载荷字节。只读盒，不判定 |
| `backends/mediacodec/session/decode_session.cxx` | 旧（接线） | DV 事实 → profile 5 走 open() 拒绝分支；8.x 设 `hdr.kind = kDolbyVision` |
| `backends/mediacodec/present/surface_dataspace.cxx` | 旧（小改） | `apply_surface_dataspace` 的 switch 加 `kDolbyVision → BT2020_PQ`（与 HDR10 同值，**证据里 kind 仍是 dolby_vision**，可区分） |
| `backends/mediacodec/session/decode_stats_renderer.cxx` | 旧（小改） | color 块出版 `dv` 子块：`detection`（mime/box/none）、`dv_profile`、`dv_level`、`bl_signal_compatibility_id`、`verdict`（passthrough/rejected/not_dv）、`reject_reason`。schema **不升版本** |
| `tests/video/decode/p210_dv_tests/` | **新** | 判定纯函数单测（AC1/AC3/AC4 的桌面部分）+ 变异 |
| `tools/verify_p210_dv.py` | **新** | 真机证据校验器 + `--self-test` 变异自检（P16 门禁风格） |
| `tools/inject_dvcc.py` | **新（若需合成片）** | 给 HEVC mp4 注入 dvcC（先例：P12-F3B `inject_pssh.py`） |

### 两颗雷（开工前必读）

1. **`backend/CMakeLists.txt` 属他人主线占用**（P16-CLOSEOUT §五.3）。本卡注册新测试目标绕不开它。
   规则：动它之前先 `git status` 确认它没有**新的**未提交改动（本日 `9bc721d5` 之后）；有 → 停下来问用户
   （两个助手会话共用同一个工作目录）。android 侧 `CMakeLists.txt` 不在占用清单，可正常加源文件。
2. **`verify_p12_all.py` / `docs/evidence/` 占用**。本卡靠"加字段不升版本"避开两者：
   schema 保持 `decode/19`，总收口两张表不用动；210 校验器独立成文件，不并进去。

## 五、测试片源

1. **首选**：公开 DV profile 8 样片（下载用 `curl --noproxy "*"`，网络坑见交接文档 §4）。
2. **合成兜底**：`tools/inject_dvcc.py` 往现有 HEVC / HDR10 mp4 注入 dvcC，profile=8.1 与 profile=5 各一。
   合成片对 AC2/AC3 **够用**：8.x 本来就按 HDR10 播（不需要 DV 像素），5 在 open() 就拒（根本不解码）。
3. **诚实清单条款**：若 AC2 用的是合成片，证据与状态稿必须写"识别与透传已验，DV RPU 元数据本体未验"
   —— 不许把合成片的结果说成"DV 能播"。

## 六、验收判据（全部可量化、有证据路径）

| # | 判据 | 通过条件 | 证据 |
| --- | --- | --- | --- |
| AC1 | 识别 | DV 片源解出 `dv_profile` / `dv_level` / `bl_signal_compatibility_id`，8.x 判 `dolby_vision` 且依据串非空 | 真机 decode 证据 `color.dv.*` ×3 轮 |
| AC2 | 透传 | 8.x 轮 `dataspace.applied=true` 且回读值与 HDR10 轮**逐位相同**（BT2020_PQ）；零丢帧、`completed=true` | 同上 + sync 证据 |
| AC3 | 拒绝 | profile 5 在 `open()` 拒绝：错误码 12、`reason` 点名 `dv_profile=5`、**没有 configure 过解码器** | 真机负向证据 ×3 轮 |
| AC4 | 回归 | 既有 SDR/HDR10/HLG 片源：改动前后 color 块**逐位相同**（桌面单测 + 真机各一轮对照） | 单测 + 回归证据 |
| AC5 | 门禁 | `verify_p210_dv.py --self-test` 变异全 CAUGHT（删字段/改判据/谎报透传各一条），正式跑 exit 0 | 校验器输出 |

## 七、真机步骤（需设备到位）

1. `track_format_dump` 打 DV 片源 → 定 路 A / 路 B（这一步之后才动 C++）。
2. 8.x 轮 ×3：播放取证（decode + dataspace 回读 + sync）。
3. profile 5 轮 ×3：open 拒绝取证。
4. SDR / HDR10 / HLG 回归轮 ×1–3：AC4 逐位对照。
设备：`dd137cf7`（插上即用）或授权 `KIVO_ALLOW_PRIMARY=1` 用主测试机 `b6e950ee`。

## 八、明确不做（范围外，防跑偏）

DV B 档映射（233 卡）· RPU/EL 元数据解析与下发 · profile 7 双层盘源 ·
平台 DV 解码器直解（若实测发现 K40 存在 `video/dolby-vision` 解码器，**只记录事实**，
要不要用它留给后续卡拿数据决策）· UI 侧 DV 标识（213 卡的 Compose 层）。

## 九、开工时间线（2026-11-06 定）

| 步骤 | 依赖 | 时点 |
| --- | --- | --- |
| 设计稿 | 无 | ✅ 本文 |
| 测试片（合成/下载） | 无真机 | 设计稿确认后即可，宿主机完成 |
| 实现 + 单测 + 门禁 | 设计稿确认 | 一轮会话内 |
| 真机取证 | 设备 | 设备到位当天收口 |

---

## 十、实测更正块（2026-11-06 夜，实现前实测；原稿 §一–§九 不回写）

实测工具：合成片（§五 的兜底路线，`tools/inject_dvcc.py`，ffprobe 独立验证逐位一致）
+ 现成探针 `kivo_hdr_probe`（arm64 构建产物，含 hdr-probe/3，**未重编**）。
证据：`docs/evidence/p210/p210-track-dump-{dv81,dv5}.json`。

1. **路 A 成立，路 B 不需要**：平台对带 `dvcC` 的 HEVC 轨直接报
   `mime = video/dolby-vision`，且给出 `profile` / `level` / `bl_compatibility_id`
   三个键 —— 零容器解析。§三 的 box 解析单元（`source/mp4_dvcc_reader`）**不建**
   （YAGNI 落实：另一条的接口位不实现）。
2. **平台键是位标志编码**（两支片交叉验证）：profile 8→`256=1<<8`、5→`32=1<<5`；
   level 1→`1=1<<0`、6→`32=1<<5`。解码规则：`log2(key)`（profile）、`log2(key)+1`（level）。
   非 2 的幂一律判"不认识"，**不猜**。
3. **这颗设备没有 dolby-vision 解码器**：探针三次 configure 全部
   `createDecoderByType returned null`。§一 的"8.x 透传可播"因此要求
   **configure 时重写 mime → video/hevc**（BL 本来就是标准 HEVC Main10；
   RPU NAL 留在流里由解码器自然丢弃）。原稿没有这一步，是实现期新事实。
4. **DV 判定必须先于 `classify_hdr`**：dv5 片源连颜色键都没有，现有分类会判
   `sdr` 放过去、最后死在模糊的"没有解码器"上 —— 拒绝前移到 `open()`
   （枚举 `kDolbyVisionUnsupported = 12`）。
5. 家族树更正：`decode/mediaformat_dv_io.{hpp,cxx}` 新增（DV 键读数层，
   与颜色家族同款分工）；`dv_config_facts.{hpp,cxx}` 落 `decode/`；§四 其余不变。
6. JNI 返回码定为 **13**（加密是 12）：取证要能按码分开统计，"DV 档位不收"与
   "文件打不开"不能共用一个码。
7. Sen 拆解更正（交接文档 §2 的目标）：`libMLibKit.so` 里 **0 个** dovi/tone 符号
   （460 函数全扫，dumped: 0）；DV/RPU 解析在 `libFFmpegKit.so`
   （字符串实测 dovi=9 / DOVI=16 / vivid=4）。DV 逆向价值落在 233 卡，
   夜间分析目标改为 libFFmpegKit（54MB）。
