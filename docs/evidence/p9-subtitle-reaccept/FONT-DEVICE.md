# W3-R2 · SC 字体 · 真机三次反证与结论（2026-11-22）

原假设（勘察记录 255 §2）：候选族名写死 `Noto Sans CJK JP`，把简中字体提上去即可（"单文件可修"）。
**真机三次尝试全部失败，结论被推翻** —— 这条路在现有 libass/FreeType 构建下走不通。

## 1. 取证方法

造字形夹具：`FONT-test.mp4`（1280×720 纯深底 40 s）+ **同名外挂** `FONT-test.srt`
（文本「直 骨 含 关」）。同名 ⇒ `sub-auto=exact` 自动挂载；srt 不带字体名 ⇒ 走我们的
`sub-font` / `sub-fonts-dir`（才测得到族名选择逻辑）。`--ez kivo.freeze true` 后截图，
统计字幕区亮像素（>120）。

## 2. 三次尝试（全部回滚）

| 尝试 | 族名 / 文件 | 结果 |
| --- | --- | --- |
| 基线 | `Noto Sans CJK JP` / `NotoSansCJK-Regular.ttc` | 字幕正常：**3133 亮像素**，`max 255` |
| ① | `MiSans` / `MiSansVF.ttf`（20 MB） | **字幕整段消失**：亮像素 **0**。原因：**可变字体（fvar）**，libass/FreeType 打开后无默认实例 |
| ② | `MiSans L3` / `MiSansL3.otf`（14.7 MB，60340 字形、无 fvar） | 仍 **0 亮像素**，截图与 ① **逐字节相同**。原因：**CFF/OTTO**，FreeType 无 CFF 驱动 |
| ③ | `Droid Sans Fallback` / `DroidSansFallback.ttf` | `adb pull` 失败：**本机 `/system/fonts` 里没有该文件**（候选行是历史遗留） |
| 回滚 | `Noto Sans CJK JP` | **3133 亮像素 / max 255，与基线逐像素一致** |

字体事实（fonttools 直读）：`MiSansVF.ttf` = 可变；`MiSansL3.otf` = `sfntVersion=OTTO`、60340 字形、
family `MiSans L3`；`MiSansC_3.005.ttf` = 仅 136 字形（不是完整 CJK）。

## 3. 结论（重要）

- **SC 字形不是"改一行族名"能解决的**：libass 只取 TTC 的 0 号 face（JP），而我们构建的
  FreeType 又**不支持 CFF/OTF**、**不支持可变字体** ⇒ 设备上所有"简中字形"候选都不可用。
- 真出 SC 字形只有两条路，代价都不小：
  1. **重编 libass + FreeType**（启用 CFF、可选 VF）——与 Wave2 的重编是同一套工程，可合并排期；
  2. **把一份 TrueType 简中字体打进 assets**（约 +10 MB 包体），再用 `sub-fonts-dir` 指向它。
- 现有状态**保持 JP 字形**（不做降级、不静默）：`[subfonts]` trace 的族名字段仍是
  `Noto Sans CJK JP`，可查。

## 4. 未取证

- 未验证"重编出 CFF 支持后 MiSansL3 是否真的出 SC 字形"（属第 3 节方案 1 的验证前提）。
- 未在其它 ROM 上重复三次反证（可能某些 ROM 自带 TrueType 简中字体）。
- 未做「直/骨/含/关」的逐字轮廓对比（因为三次尝试都没出画面，无从对比）。

## 5. 收尾

设备上保留夹具 `FONT-test.mp4` + `FONT-test.srt`（后续重编 FreeType 时直接复用）；
APK 已回滚到基线字形；未 commit / push。
