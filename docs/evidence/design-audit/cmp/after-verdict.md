# 第一波 P0 统一 after 真机判定

- 设备：K40 `b6e950ee`，Kivo Video debug 新包（T01/T02/T03/T04 全部落地后重装）。
- 取证纪律：`adb exec-out screencap -p`；装包后仅 monkey 起 App、**未清缓存**；全程 Awake、前台正常、非黑帧；未遇锁屏/通知帘，设备全程健康。
- 装包：`gradlew :app:assembleDebug` → `adb -s b6e950ee install -r app-debug.apk`（Success）→ monkey 起 MainActivity。
- 图路径：after 在 `cmp/after/`，before 在 `cmp/before/`，并排图在 `cmp/` 根目录。

## 判定表

| 疑点 | before 现象 | after 现象 | 判定 | 图路径 |
|---|---|---|---|---|
| **P0-01 标题叠加**（魔鬼天使 SMB） | 海报自带紫色竖排"魔鬼天使"美术字 + 自绘黑色横排大字同屏打架（34sp） | 海报紫色美术字仍是主角；自绘字退为 20sp 横排小标贴在 metaLine 上方，两套字主次分明、不再争视觉重心 | **改善**（重复大标题消除；小标仍保留作为 NAS 降级的次级标注） | before `before/p01-p02-smb-devil.png` → after `after/p01-devil.png`；并排 `side-by-side-p01-devil.png` |
| **P0-01 标题保留**（犯罪都市4 SMB） | 海报无烧字，唯一 34sp 自绘白字标题（视觉主角） | 白字标题仍在（本地 NAS 降级为 20sp，属已知代价），清晰可读、不裸掉 | **可接受**（标题未丢；20sp 是 scheme 判据下本地 NAS 的统一代价，TMDB/Emby 非 NAS 仍 34sp） | before `before/p01-p02-smb-movie.png` → after `after/p01-movie.png`；并排 `side-by-side-p01-movie.png` |
| **P0-02 详情背景脏灰**（魔鬼天使） | 低饱和米灰，实采样主色 ≈ `#C7BCAB`（crop 均值 `srgba(78.55%,75.27%,69.71%)`） | 海报同色系饱和暖橄榄褐，采样主色 ≈ `#A78A47` / 背景区均值 `srgba(65.95%,55.03%,31.65%)`；与海报黄色椅/紫字暖调同色系 | **改善**（米灰消失，换为同色系饱和色场） | before `before/p02-devil-bg-crop.png` → after `after/p02-devil-bg.png`；并排 `side-by-side-p02-devil-bg.png` |
| **P0-03 评分独立行** | meta 行 `日期 · 地区 · 时长` 无评分；"展开简介"下方居中独立一行 `TMDB徽章 4.4`（RatingBadges，耗 ~40dp） | 评分并入 meta 行 `日期 · 地区 · 时长 · ★ 4.4`；独立评分行整块消失 | **消失**（信息减负达成，纯文本 ★ 无胶囊徽章、零裸色） | before `before/p01-p02-smb-devil.png` → after `after/p03-meta-rating.png`；并排 `side-by-side-p03-meta-row.png`（meta 行）+ `side-by-side-p03-score-block.png`（独立评分块消失） |
| **P0-04 首页继续观看空占位** | 首卡渐变蓝底 + 白色首字 "C"（Cold Case Clean），并非纯深灰空框 | 与 before 逐像素一致（同一 `NasPosterPlaceholder`：渐变+首字），本就合格 | **不适用/未变化**（该位置用的是设计文档明确保留的 NasPosterPlaceholder，不是 T03 统一占位的覆盖范围；before 已无"纯深灰空框"问题） | before `before/p05-discover-nav.png` → after `after/p04-home-empty.png`；并排 `side-by-side-p04-home.png` |

## 补充事实

- **犯罪都市4 背景**：after ≈ `#B46B23` / `#844404`（饱和暖橙），比 before `#775743` 更饱和鲜亮，符合 P0-02 同色系色场方向（该 case 本就正常，属保持并增强）。
- **色值采样方法**：ImageMagick `-resize 1x1` 取区域均值 + `-format "%[pixel:p{x,y}]"` 定点采样；before/after 同尺寸 crop（各 1080x900）保证可比。
- **C2 消空白**：按设计 §7-5 门控跳过——C1 删评分块已消 ~40dp，是否仍有 >48dp 空白需真机判断；本次 after 图未见明显 >48dp 空白带，资源卡（SDR 小豆腐块）逐项保持（18sp SDR 主标题 / 17.1GB / 25.79Mbps / NAS / 绿勾），ExpandMediaInfoRow 未受影响。
- **AHS / TMDB 封面不可用态**：本次未单独取证（上一轮 before 已判定 PASS：非 NAS 无 logo 走 34sp 大字，scheme 判据保证不受影响）。
