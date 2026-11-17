# B3 发现页验收（b6e950ee 真机）

> 2026-11-17 ｜ 设备：Redmi K40 `b6e950ee` / 1080×2400 / 440dpi / Android 13
> 口径：与 rex IMG_0330–0335 逐屏对照，并用裁图反算 dp；**只认实测**。
> 本轮为「用户第一次否决后的二次重做」验收（B3R / B3R2）。

## 一、执行结果

| 命令 | 结果 |
| --- | --- |
| `gradlew.bat :app:assembleDebug --offline -q -Pkotlin.incremental=false` | EXIT=0（多次重编） |
| `adb -s b6e950ee install -r` | **Success**（全程未 uninstall） |
| `python tools/ui_gate.py` | **RESULT: PASS**；超限文件 21，裸色 325（baseline 326，−1） |

## 二、用户否决项的整改与实测

### 2.1 播出平台
- **否决原因**：kivo 是 161×91 纯 logo 卡（且空白），rex 是 236×94、1.5 列、左 logo + 右剧本拼贴。
- **已改**：卡 236×94、`washFill` 浅底、右侧 2 列错落拼贴（52×74 tile，前 4 张）。
- **实测（ee，h2 截图）**：红字 **NETFLIX** 单行 + 右侧 4 张海报拼贴；黑色方块消失。
- **根因订正**：黑底来自 TMDB 的 Netflix/Apple TV `logo_path` PNG **自带黑底**，不是代码底色。
  故品牌位改为**品牌字标**（红 NETFLIX / 蓝 DISNEY+ / 黑 APPLE TV），色值复用 `dangerRed`/`navActive`，零裸色。
- ⚠️ **未复验**：`APPLE TV STORE` 在 14sp 下仍被省略号截断；已降到 12sp 并安装，但 **ee 设备随后掉线**，
  该一版没有截图证据。当前列为 OPEN（P2），不可写成 PASS。

### 2.2 分类浏览
- **否决原因**：只有文字、无容器、无图。
- **已改**：卡 177×59、圆角 12、`groupedFill` + **1dp hairline 描边**（根因：`groupedFill == pageBackground`，只填色等于没画容器）；左侧 64×40 层叠图垂直居中；文字 `CenterStart`。
- **实测（n3）**：两张白底描边卡（动作 / 冒险），左图 + 文字可见。✅ 结构 PASS。
- 剩余：层叠图比 rex 偏小（rex 左图约占卡高 80%），列 PARTIAL（P2）。

### 2.3 高分剧集
- **否决原因**：2 列大图、每张挂标题+日期，与前面海报不统一。
- **已改**：改为 **5 列 2:3 海报、无标题无日期**，与 `GenrePosterWall` 同尺寸（weight(1f)、间距 8、圆角 10）；节标题「高分剧集」整行居中。
- **实测（n3）**：5 列纯海报 + 居中标题。✅ PASS。

### 2.4 电影公司
- **否决原因**：白底 + 蓝 logo + 被裁切，rex 是深色剧照底 + 白色居中 logo。
- **已改**：`TmdbStudio` 增 `backdropUrl`（`/discover/movie?with_companies=id` 取首条有 backdrop 的 w780，实测 **6/6** 取到）；卡 177×80，剧照铺满 + 45% 洗色 + 白色 logo（70%×62%、fit 不裁切）。
- **实测（n3）**：UNIVERSAL / Paramount 为**深色剧照底 + 白色 logo**。✅ PASS。

### 2.5 分类 5 列海报墙
- **已接**：`genrePosterUrls`（动作类型 10/10 实测）；`GenrePosterWall` 5 列 2:3。
- **实测（n3）**：5 列已出图；部分格为加载中的灰块（占位），非缺失。✅ PASS（加载态见 P3）。

## 三、页面其余项（首轮已验）

| 项 | 实测 | 判定 |
| --- | --- | --- |
| hero 分页点 | 8 点，选中显著变大 | ✅ |
| hero 整卡点击 | 点 (540,800) → `.ui.detail.MovieDetailActivity` | ✅ |
| 热门人物 | 圆形头像 + 姓名 + 「演员」，5 列可见 | ✅ |
| 正在热映/播出/趋势/综艺/动漫 | 与 rex 顺序与形态一致 | ✅ |
| 底部导航 | 4 Tab + 搜索，选中白胶囊 | ✅ |
| 继续观看来源 | http→host / 本地→「本地」 | ✅ 结构 / PARTIAL 文案（P2） |

## 四、剩余差距

| 编号 | 差距 | 证据 | 改法 | 优先级 |
| --- | --- | --- | --- | --- |
| B3-01 | ~~`APPLE TV` 字标曾被省略号截断~~ | **已闭环**：12sp 后 ee 真机 `docs/audit/golden/kivo/discover-5.png` 显示完整 `APPLE TV`、`NETFLIX` 红字单行 + 4 张拼贴 | FIXED | — |
| B3-02 | 分类卡层叠图偏小 | `GenreCards.kt` 64×40 | 图改 76×48、层叠偏移 7dp | P2 |
| B3-03 | 海报墙加载中灰块 | `GenrePosterWall.kt` + `TmdbImage` 占位 | 占位改统一 `TmdbPlaceholder` 色块 + 骨架 | P3 |
| B3-04 | 平台拼贴只前 4 张、右侧仍留白 | `PlatformCards.kt` | tile 52→56 或加第 5 张 | P3 |
| B3-05 | 继续卡平台名仍是 host | `DiscoverPage.kt sourceOf` | B5 接来源元数据 | P2 |

## 五、结论

**B3R2 达成（结构与形态）**：四个被否决模块均已按 rex 的**比例/列数/卡内结构**重做，真机可见：
平台卡（浅底 + 红色 NETFLIX 字标 + 拼贴）、分类卡（描边容器 + 左图右文）、高分剧集（5 列纯海报 + 居中标题）、
电影公司（剧照底 + 白 logo）。

**未闭环 0 项**：`APPLE TV` 字标已在重连后的 `b6e950ee` 上复验（`docs/audit/golden/kivo/discover-5.png`），
平台卡四项元素（浅底 / 红色 NETFLIX 单行 / 4 张拼贴 / APPLE TV 完整）全部可见。

**仍属体验层（P2/P3，不影响结构验收）**：分类卡叠图偏小、海报墙加载占位、平台拼贴右侧留白、
继续卡平台名仍取 host。