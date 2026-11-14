# 276 · HDR→SDR 色调映射（L2）落地

状态：**已实施并真机验证**
上游：[274-hdr-rendering.md](../../design/ui/replica/274-hdr-rendering.md) §10.0（R2 路径）
证据：`docs/evidence/p274-hdr/l2-ab/`
工具：`tools/p274_hdr_ab.sh`（三臂 A/B + 装置自检）

---

## 一、结论先行

**在现有架构（`vo=libmpv` render API + legacy RA）下，HDR→SDR 色调映射是可行的，
且已经落地并真机验证。**

| 项 | 结论 |
| --- | --- |
| 真 HDR 输出（G2） | ❌ 仍被架构闸门挡住（无 compute shader ⇒ libplacebo 关掉 HDR 峰值计算） |
| **HDR→SDR 色调映射（L2）** | ✅ **可用**：同一段 PQ 画面，亮度 mean 75.0 → **93.7**，高光 max 159 → **181** |
| SDR 片源 | ✅ **零改动**（非 HDR 时一个参数都不下发；实测 mean 102.3 → 102.2） |
| 真机判定依据 | 内核回报的 `video-params`：`gamma=pq primaries=bt.2020` ⇒ 判为 HDR |

## 二、为什么之前没人发现"HDR 偏暗"

- 274 的 G1 取证用的是 **h264** 片源（`g1-hdr10/kernel.log`：`Video --vid=1 (h264 1920x1080)`），
  **不是真正的 HDR10 素材** ⇒ 从来没有真正把 PQ 画面放到屏上看过。
- 本轮第一次用真素材（`build/avtmp/hdr10_1080p.mp4`：
  hevc Main10 / bt2020nc / **smpte2084(PQ)** / bt2020）播放，
  内核日志确认：`reconfig to 1920x1080 mediacodec bt.2020-ncl/**bt.2020/pq**/limited`
  ⇒ 输入侧识别正确，问题在**输出侧没有做 tone-map**。

## 三、三臂对照（2026-11-15 04:2x，K40 `b6e950ee`）

素材：`build/avtmp/hdr10_1080p_x4.mp4`（6 秒原片循环 4 次 ⇒ 24 秒，
理由：6 秒片源在冻结截图时会撞 EOF）。
装置：`tools/p274_local_origin.mjs`（本地文件 ⇒ 速率确定）。
截屏：**设备截屏**（SurfaceFlinger 合成结果 = 用户眼睛看到的那一帧），
口径 = Rec.709 系数 / sRGB 编码值。

| 臂 | 下发的 mpv 参数 | mean | max | p95 |
| --- | --- | --- | --- | --- |
| A 基线 | 无 | 75.0 | 159 | 144 |
| **B L2** | `target-trc=bt.1886,target-prim=bt.709,target-peak=100,tone-mapping=bt.2390,target-colorspace-hint=yes` | **93.7** | **181** | **169** |
| C 可视化 | `tone-mapping-visualize=yes` | 75.0 | 159 | 144 |
| **D 自动（落地后）** | 由 App 自己判定并下发 | **93.7** | **181** | **169** |

- C 与 A **逐像素相同** ⇒ `tone-mapping-visualize` 在没有 tone-map 时不画曲线（不能拿它当判据）。
- D 与 B **完全一致** ⇒ 自动化路径与手工参数等价。
- E 回归（SDR 片源 `hevc_1080p10.mp4`，改动后）：mean **102.2**（改动前 102.3）⇒ 零差异。

## 四、实现（家族树）

```
android/app/src/main/java/com/kivo/cinema/playback/
├─ MpvHdrToneMap.kt      【新增】只做一件事：读 video-params → 是 HDR 就下发 L2 参数、否则不动
└─ MpvStartSequence.kt   改：首帧之后调 MpvHdrToneMap.applyIfHdr(native)
```

**为什么只有一个新文件**：这不是"新机制"，是"一组 VO 选项的按条件下发"，
判定与下发必须在一起（拆开就会出现"判了不落 / 落了不判"）⇒ 一个文件一个责任。

**为什么放在首帧之后**：`video-params` 要等解封装报出轨道才有值；
"是不是 HDR"必须依据**真实回报**，不许按扩展名/文件名猜。

**为什么对 SDR 一个参数都不设**：`target-*` / `tone-mapping` 是 VO 级选项，
改动会触发 VO 重新初始化（内核日志里 `Disabling HDR peak computation` 会重新打印一次）
⇒ 对 SDR 内容是"白折腾一次 VO 重建"。

## 五、参数组与依据

| 参数 | 值 | 依据 |
| --- | --- | --- |
| `target-trc` | `bt.1886` | 输出到 SDR 屏的传递函数（≈ gamma 2.4），不是 PQ |
| `target-prim` | `bt.709` | 输出色域回到 SDR  primaries |
| `target-peak` | `100` | SDR 参考白（nit）；**这是关键**：不设它，RA 会按内容峰值（PQ 常见 1000 nit）映射 ⇒ 画面偏暗 |
| `tone-mapping` | `bt.2390` | BT.2390 EETF 曲线（电视观看推荐） |
| `target-colorspace-hint` | `yes` | 让 RA 知道输出色彩空间 |

⚠️ **`target-colorspace` 这个选项在 mpv 0.39 不存在**（回执 `-3`）；
正确的是 `target-colorspace-hint`（布尔）。这一点在二进制里查过：
`libmpv.so` 的选项表里有 `target-colorspace-hint`，没有 `target-colorspace`。

## 六、诚实边界

1. **这不是 HDR 输出**：面板走的仍是 SDR 路径，颜色被压回 bt.709/bt.1886。
2. **真 HDR（G2）仍然不成立**：要 compute shader ⇒ 要换 `vo_gpu_next`（架构级，另立卡）。
3. **没有做逐像素色彩正确性验收**：274 §10.1 要求灰阶/亮度阶梯/广色域色块的标准素材，
   本轮只用 SMPTE 彩条做了"有没有被压暗"的判据 ⇒ **色彩准确度未测**。
4. **HLG 未验**：素材只有 PQ（smpte2084）；HLG 的 `gamma=hlg` 判定分支写好了但**没测过**。
5. **首帧之后才下发** ⇒ HDR 片源的第一帧可能是"未映射"的亮度，随后被修正
   （实测修正发生在首帧后约 0.3 秒内，肉眼不可见；未做逐帧录像）。
6. 装置日志/统计文件（`origin*.log`）由装置在退出时写出，脚本中途读会看到空文件 —— 正常。

## 七、验收判据（已全部通过）

| # | 判据 | 结果 |
| --- | --- | --- |
| 1 | HDR10 片源自动判为 HDR | ✅ 日志 `gamma=pq primaries=bt.2020` |
| 2 | L2 参数全部下发成功 | ✅ 5 项回执全 `ok` |
| 3 | 画面亮度回到正常 SDR 观感 | ✅ mean 75.0 → 93.7 |
| 4 | SDR 片源零改动 | ✅ mean 102.3 → 102.2，参数一个不下 |
| 5 | 真 HDR 宣称 | ❌ 不宣称（架构闸门未过） |
