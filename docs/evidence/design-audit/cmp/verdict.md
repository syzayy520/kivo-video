# 第一波 P0 疑点 · 真机取证判定表

> 取证工程师：寇豆码（Kou）｜日期：2026-11-19｜设备：K40 `b6e950ee`（已解锁、非 reboot）
> 截图方式：全程 `adb exec-out screencap -p > x.png`（PNG 头 `89 50 4e 47` 校验通过）
> **本轮只取证 + 判定，未改任何业务代码，未提交。**

---

## 一、取证过程与设备状态

### 第 1 轮（P0-05）
- G0 设备核对通过（`b6e950ee`、`topResumedActivity=com.kivo.cinema/.MainActivity`、`mWakefulness=Awake`、非黑帧 2.28MB）。
- G2 清缓存成功（`force-stop` + 删 `cache/tmdb-images-v2`/`nas-poster` + monkey 重启）；其后设备陷入熄灭 + MIUI 锁屏通知帘循环，约 8 次恢复未果，按铁律停止，丢弃全部黑帧/0 字节帧。此轮**未进入任何详情页**，P0-01/P0-02 保持「未完成」。

### 第 2 轮（P0-01 / P0-02 补取证）
- 主理人只读核验设备已恢复（Awake、前台 MainActivity、非黑帧 2,083,584B）。遵循收紧后的纪律：**跳过清缓存/重启**，从当前前台 App 直接导航；全程无电源键/reboot/反复上滑；设备始终 `mWakefulness=Awake`、焦点正常，**未再遇锁定/通知帘/黑帧**。
- 导航路径全部合法：电影/剧集/SMB 卡片均为首页分类行点击 → `MovieDetailActivity`；**未走"继续观看"（会起播）、未用 am start**。
- 共取 4 张详情页有效帧：SMB 犯罪都市4、SMB 魔鬼天使、AHS 剧集、TMDB 电影（狂野雄心）。清缓存已跳过，串图 bug 由上游唯一 token 修复，标题叠加是 `TitleLayer` 渲染逻辑、与缓存海报无关，故不影响判定。
- 背景色用 ImageMagick 逐像素采样（`magick -format %[pixel:p{x,y}]`），不凭肉眼印象。

---

## 二、判定表

| 疑点 | 当前真机事实 | 判定 | 建议 |
|---|---|---|---|
| **P0-01 标题三重叠加**（海报美术字 + 自绘白字 + 自绘黑字同屏） | 四类详情页逐一实拍：① **SMB 魔鬼天使**（`p01-p02-smb-devil.png`）：海报内**自带紫色竖排美术字「魔鬼天使」**与界面**自绘黑色横排「魔鬼天使」**两块字**同屏**（放大图 `p01-p02-smb-devil.png` 可见，标题带完整保留，未被 `trimTitleBand` 裁掉）→ **≥2 套字，命中 FAIL**；② SMB 犯罪都市4（`p01-p02-smb-movie.png`）：海报无烧字带，仅自绘白字一套，PASS；③ AHS 剧集（`p01-p02-ahs-series.png`，封面不可用态）：仅自绘白字一套，PASS；④ TMDB 电影狂野雄心（`p01-p02-movie-tmdb.png`，封面不可用态）：TMDB logo 图一套，未再叠加自绘 Text（logo-first 防线生效），PASS。 | **真实存在（部分 case）**——`魔鬼天使` 这类**海报自带美术字 + 无 logo 标题**的 SMB 条目仍两字同屏；主理人「旧图拍于防线生效前」的前置推断**被推翻**：当前版本在该 case 仍复现。其余三类已正常。 | **建议改码**（仅针对失效 case）：`TitleLayer`/`SmbDetail` 的唯一化分支需覆盖「海报自带烧字 + 无 logo」场景——`trimTitleBand` 只裁底部 15%，魔鬼天使紫色字在竖排中部/右侧**未落入裁切带**，需补「海报含字则不画自绘 Text」判定或调整裁带策略（按设计 §1.1 G6'）。改动落 `DetailHeroContent.kt:30-48` / `SmbDetail.kt:269`，注意两文件均 300 行临界、净增 0。 |
| **P0-02 详情背景脏灰**（低饱和脏灰 / 米灰，与海报主色割裂） | 逐像素采样内容区背景：① **魔鬼天使**（浅色/米白海报）背景 = `srgb(199,188,171)` ≈ **#C7BCAB 低饱和米灰/卡其**，与其海报紫色美术字明显割裂，命中「米灰」投诉（见 `p02-devil-bg-crop.png` 特写）；② 犯罪都市4（暖调海报）背景 = `srgb(119,87,67)` ≈ #775743 暖褐，海报派生、正常；③ AHS/④ TMDB 封面不可用态背景为深色（海报暗化退化），非中性灰。即 `PosterPalette.ambientOf` 已接入但**对浅色海报取底部 15% 线性光平均 → 饱和度偏低显米灰**，与设计 §7-6 预判一致。 | **真实存在（浅色海报 case）**——`ambientOf` 对米白类海报产出低饱和米灰 #C7BCAB，与海报主色割裂；非「未接入」，而是取色饱和度不足。 | **建议改码（谨慎）**：仅当确认要修时调 `PosterPalette.ambientOf`（`PosterPalette.kt:9-35`）饱和曲线——**失败退化为"海报暗化版"，不用中性灰**（§7-6）；替换实现、不新增平行函数。改动前请主理人确认优先级（P0-01 更明确、应优先）。 |
| **P0-05 胶囊深态**（发现页 hero 深底是否「深灰底黑字」；浅色页是否浅态）〔第 1 轮已判定，本轮不变〕 | 发现页取得两态有效帧：① 深底 hero 时胶囊深态——深灰药丸底 ≈#2C2C2E、浅灰/白图标 + 浅色字、选中「发现」更浅灰药丸，**无深底黑字、标签清晰**；② hero「封面加载中…」浅色态时胶囊自动转浅态——白药丸 + 深色字，正常。 | **问题不存在／已消失**——主理人前置结论被当前真机证实，深/浅两态均正确。 | **不动（就「黑字」缺陷）**。D1 `onDark` 判定来源重构属另一议题，需主理人单独拍板。 |

---

## 三、证据图路径

| 图 | 路径 | 说明 |
|---|---|---|
| **P0-01 关键 FAIL** | `docs/evidence/design-audit/cmp/before/p01-p02-smb-devil.png` | SMB 魔鬼天使详情：海报紫色竖排美术字 + 界面自绘黑字「魔鬼天使」两套同屏 |
| P0-01 PASS（SMB） | `docs/evidence/design-audit/cmp/before/p01-p02-smb-movie.png` | SMB 犯罪都市4：仅自绘白字一套；背景暖褐 #775743 |
| P0-01 PASS（剧集） | `docs/evidence/design-audit/cmp/before/p01-p02-ahs-series.png` | AHS 剧集封面不可用态：仅自绘白字一套 |
| P0-01 PASS（电影） | `docs/evidence/design-audit/cmp/before/p01-p02-movie-tmdb.png` | TMDB 电影狂野雄心封面不可用态：TMDB logo 一套，无自绘叠加 |
| **P0-02 关键证据** | `docs/evidence/design-audit/cmp/before/p02-devil-bg-crop.png` | 魔鬼天使内容区背景特写：低饱和米灰 #C7BCAB，与海报主色割裂 |
| P0-05 深态 / 浅态 / 全景 | `.../p05-nav-dark-hero-crop.png`、`p05-nav-light-hero-crop.png`、`p05-discover-nav.png` | 第 1 轮证据，胶囊深/浅两态均正常 |
| 设备核对 | `.../g0-confirm.png`、`g2-relaunch.png` | 第 1 轮非黑帧与清缓存后截图 |

> 所有路径相对 `docs/evidence/design-audit/cmp/`；黑帧/0 字节帧已全部删除，未污染证据目录。

---

## 四、结论与建议下一步

1. **P0-01 真实存在（明确）**：`魔鬼天使` 类「海报自带美术字 + 无 logo」SMB 条目仍两字同屏，前置「旧图是修复前」推断被推翻。建议改码，落 `DetailHeroContent.kt`/`SmbDetail.kt` 唯一化分支（两文件 300 行临界，净增 0）。
2. **P0-02 真实存在（浅色海报 case）**：米白海报产出低饱和米灰 #C7BCAB、与海报割裂。属「已接入但饱和度不足」，是否调 `ambientOf` 请主理人定优先级（建议排在 P0-01 之后）。
3. **P0-05 无问题**，不动。
4. 本轮未改业务代码、未提交；设备全程健康，无黑帧/锁屏中断。
