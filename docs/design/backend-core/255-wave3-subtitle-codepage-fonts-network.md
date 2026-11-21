# Wave3 · 字幕 codepage / SC 字体 / 网络字幕 · 勘察记录（未实施）

> 状态：**勘察完成，未实施**（2026-11-22）。交接 §5-8 只写了"需先出家族树"，
> 本文件把三项的**真实落点**钉住，并明确哪些已确认、哪些还没勘察 —— 不做推断性设计。

## 1. 三项现状（全部实测命中）

| 子项 | 现状 | 证据位置 |
| --- | --- | --- |
| `sub-codepage`（手选编码） | **完全未实现**：`grep -rn "sub-codepage\|codepage"` 在 kt/cpp 里 **0 命中** | — |
| SC 字体 | **已实现但选错 face**：候选族名是 `Noto Sans CJK **JP**`，代码注释自己写着"0 号 face 是 JP" | `playback/SubtitleFontSet.kt:56-64` |
| 网络字幕 | **只支持本地文件**：挂载唯一入口 `tracks.addExternalSubtitle(file.absolutePath)` | `playback/MpvSubtitleControl.kt:47` |

## 2. R2（SC 字体）—— 根因最明确，收益最直接

**现状原文**（`SubtitleFontSet.kt:55-58`）：

```
* 而 Android 的 `NotoSansCJK-Regular.ttc` 0 号 face 是 JP。
* 字形层面无损失：CJK 全字集每个 face 都带，中文照样显示。
```

这句话只对了一半：**字集**确实都有，但 **face 决定字形**。JP face 下「直／骨／含／关」等字是日文写法，
简体用户一眼能看出"这字幕怪怪的"。

**⚠️ 2026-11-22 真机推翻了这个假设**（见 `docs/evidence/p9-subtitle-reaccept/FONT-DEVICE.md`）：
三次尝试（MiSansVF 可变字体 / MiSansL3 CFF / DroidSansFallback 缺失）**全部整段不渲染**，
回滚 Noto 后逐像素恢复。根因不是族名，而是**构建层**：
libass 只取 TTC 的 0 号 face，而我们的 FreeType **不支持 CFF/OTF、不支持可变字体**。

**可行的两条路（都不是"单文件"）**：
1. 重编 libass + FreeType（启用 CFF / 可选 VF）——与 Wave2 同一套重编工程，建议合并排期；
2. 把一份 **TrueType 简中字体**打进 assets（约 +10 MB），`sub-fonts-dir` 指向它。

**验收**（必须真机字形对比，不是"应该能显示"）：
同一段含「直、骨、含、关」的字幕，改前改后各截一张，**逐字裁图对比**；
同时确认 `[subfonts]` trace 的"族名"字段已变成 SC。

**未勘察**：各机型 `.ttc` 里是否都存在 `Noto Sans CJK SC` 这个 family 名（K40 需实测；其他厂商 ROM 可能有裁剪）。
⇒ 所以必须保留回退链，且回退时**写 reason 不许静默**（沿用现有 `Prepared.reason` 机制）。

## 3. R1（sub-codepage）—— 未实现，且有一个必须先定的粒度问题

GBK 字幕乱码的正确修法有两条：
- **Wave2**（内核 iconv，见 `254-…md`）：自动/可选地解 GBK，但需 minSdk 28；
- **Wave3 这条**：让用户**手选**编码（`sub-codepage`），**不需要抬 minSdk**（mpv 侧转码仍要 iconv…）。

> ⚠️ 实测结论：`sub-codepage` 在**没有 iconv 的 libmpv 上大概率无效**（选项存在但转码器缺失）。
> 也就是说 Wave3-R1 **不是 Wave2 的替代**，而是 Wave2 之后的 UI 补充。
> 这一点必须在实施前用一次探针验证（设 `sub-codepage=GBK` + 挂 GBK 字幕，看是否仍乱码）。

**落点**（若 Wave2 已落地）：
- 面板行：`ui/compose/player/subtitle/SubtitlePanelRows.kt`（新增「字幕编码」选择行）
- 写侧：`SubtitleStyleActions.kt`（`sub-codepage` → 写内核）
- 记忆：`SubtitleDelayMemory` 同族新键？**未定** —— 编码是**文件属性**，应按"字幕文件指纹"记忆，
  而不是按片源键（同一片源换一条外挂字幕，编码可能不同）。

**待拍板**：编码记忆的粒度（按片源 / 按字幕文件 / 不记忆）。

## 4. R3（网络字幕）—— 方案已定，不需要改内核

现有唯一入口是 `addExternalSubtitle(file.absolutePath)`，所以最省事且可控的做法：

**下载到 cache → 走现有入口挂载**。新增一个抓取器（单文件）：
`playback/subtitle/SubtitleRemoteFetcher.kt`：HTTP(S) GET → 校验是文本（UTF-8/GBK/BOM）→ 落
`cacheDir/subtitles/<hash>.srt` → 返回 `File` 给 `addExternalSubtitle`。

**为什么不用 mpv `sub-add <url>`**：内核侧没有 `sub-add` 的调用点（`cpp/` 里 0 命中），
新增它等于给内核加一条网络取数路径，和 Wave2 的重编/回归风险叠加；而下载进 cache 可以复用
现有的"导入字幕"整条链路（含 SRT 校验、去重、UI 状态）。

**验收**：给一个能直连的 `.srt` URL → 面板"导入字幕"出现新入口 → 挂载成功且字幕正确；
断网/404/非文本 → **必须给可见原因**（不许静默）。

## 5. 依赖次序（重要）

```
Wave2（minSdk 28 + iconv） ──► R1 sub-codepage（手选编码，才有意义）
R2 SC 字体（独立，随时可做，单文件）
R3 网络字幕（独立，不改内核，可先做）
```

**R3 不依赖 Wave2，可以立刻排期**；R2 受限于 libass/FreeType 构建（见 §2 更新），与 Wave2 同批最划算；R1 卡在 Wave2 之后。

## 6. 建议的下一步（你来点）

1. **先做 R2（SC 字体）**：单文件、根因明确、真机可像素对比 —— 性价比最高
2. 再做 R3（网络字幕）：不改内核，复用导入链路
3. R1 与 Wave2 一起评（先跑一次 `sub-codepage` 探针，确认没有 iconv 时它到底有没有用）