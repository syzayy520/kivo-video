# D12 · 详情/播放入口收敛（G3 第一批）

时间：2026-11-18 05:35 GMT+8
设备：K40 `b6e950ee`（A13）
范围：**D12-0 桌面入口收敛 + D12-A 本地详情入口收敛**；D12-B（播放宿主合并）按审计结论本批不做。

## 裁决记录

1. **D12-A 做"新增第三来源"，不做"让新页模拟旧页"**：`MovieDetailActivity` 增加 `kivo.media` 入口，
   本地文件走新的 `LocalDetailContent`（路径即片源、TMDB 只做增强），旧页独有区块用一个新文件挂回 `DetailPage`。
2. **本地详情不查 TMDB**。实测断网时 `searchId` 会把首帧拖到 8–15s 超时；本地文件必须秒开。
   旧页同样零网络依赖（评分只是可选装饰），本条与旧页 parity 一致。
3. **D12-B 本批不能删旧宿主**。`ui/PlayerActivity` 是取证/度量宿主：
   Kotlin 产品代码 0 引用，但 `tools/` 下 **31 个脚本**依赖它 20 个 `kivo.*` 探针开关与
   `files/evidence/` 三份证据 schema；新宿主还缺「本地软解路由」「同名外挂字幕自动挂载」两项能力。
   本批只做零风险部分：摘掉 `MAIN/LAUNCHER` + `@Deprecated` + 文档化退役前提。

## 改动文件（9 个，2 新建，1 删除）

| 文件 | 变化 |
|---|---|
| `ui/detail/LocalDetailContent.kt` | **新建** 130 行：path→MediaItem→fileOnlyDetails→DetailContent；永不返回 null；本地缩略图 loader |
| `ui/detail/LocalDetailBlocks.kt` | **新建** 107 行：技术徽章/私密区/文件信息/技术明细/同目录（IO 线程探测） |
| `ui/detail/MovieDetailActivity.kt` | 加 `EXTRA_PATH`/`EXTRA_DURATION`/`localIntent()`；onCreate 三分支；无 tmdbId 时用禁用态 actions |
| `ui/detail/actions/DetailActions.kt` | 加 `localOnlyActions()`（`FilmKey` 要求 id>0，不能伪造标记键） |
| `ui/detail/DetailContent.kt` | 加 `local: LocalFileFacts?` |
| `ui/detail/DetailPage.kt` | 在 LazyListScope 里以 `item(key="local-extra")` 挂载本地区块（+4 行，284） |
| `MainActivity.kt` / `library/MediaLibraryActivity.kt` / `emby/aggregate/ui/LocalResultCard.kt` | 三个入口改 `MovieDetailActivity.localIntent(...)` |
| `ui/detail/DetailActivity.kt` | **删除**（256 行旧详情页） |
| `AndroidManifest.xml` | 删旧详情页注册；删 `PlayerActivity` 的 `MAIN/LAUNCHER` |
| `ui/PlayerActivity.kt` / `ui/MultiPlayerActivity.kt` | `@Deprecated` + KDoc 注明取证宿主身份与退役前提 |

## 编译
`build-d12.log`（Manifest 注释含 `--` 导致 XML 解析失败）→ `build-d12d.log` **BUILD SUCCESSFUL**
→ 零网络改造后 `build-d12f.log` **BUILD SUCCESSFUL**。

## 验证结果

### D12-0 桌面入口 —— ✅ 只剩一个
- `cmd package query-activities -a MAIN -c LAUNCHER | grep com.kivo.cinema` → **1 条**（`.MainActivity`）。
- 此前 `PlayerActivity` 也挂 LAUNCHER，安装后桌面出现第二个图标（点开黑屏 + 状态字）。
- 31 个取证脚本用显式 `am start`，不依赖 launcher，未受影响。

### D12-A 本地详情入口 —— ✅ 两条真实入口 + 播放链路
| 场景 | 结果 | 证据 |
|---|---|---|
| 资源库 → 本地影片 → 卡片 | 进入 `MovieDetailActivity`，标题 `exit_sync`、`1080 × 2400 / 793 KB / 1.30Mbps / 本地`、技术徽章 `1080P/AVC`、`加入私密区` | `d12-final.png`、`d12-detail2.xml` |
| **断网**同一路径 | 3 秒内正常出页，**无「请检查网络」、无转圈** | `d12-offline.png`、`d12-detail.xml` |
| 首页「继续观看」卡片 | 进入同一详情页：`VID_20240729_195421`、`5分钟`、`1080P / 1920×1080 / 402 MB / 10.28Mbps`、`HEVC / AAC 2.0` | `d12-home-detail.xml` |
| 详情页「▶ 播放」 | `PlayerComposeActivity` 起播并出画 | `d12-play-ok.png` |

- UI 文字 dump 证明页面只由文件事实驱动（无 TMDB 标题/简介/评分混入）；
  `d12-local-detail.png` 里看到的「谷雨」是本地视频画面烧字，不是 UI 数据。
- 旧类引用清零：`grep DetailActivity` 仅剩 3 处注释，代码引用 0；Manifest 注册 0。

## 已知边界（如实标注）

1. **聚合搜索本地卡未单独点击验证**：与另两处是同一个 `localIntent`，代码等价；未做真机点击。
2. **本地详情没有 TMDB 海报/简介/演员**：为保证断网秒开主动放弃（旧页也没有）。如需增强，
   必须用「短超时 + 先上文件事实、异步替换」的方案，另立批次。
3. **收藏/待看对无 TMDB id 的本地文件禁用**：`FilmKey` 的 `require(id>0)` + `FilmMarksStore`
   只接受 `movie|tv:` 前缀，伪造 `local:` 键会让整库读不出来。禁用是诚实态，未静默写错数据。
4. **私密区只对 MediaStore 文件可用**：推送进设备的文件 `_ID=0`，该行不渲染。
5. **旧页「文件名搜索 TMDB 评分」未迁移**：本地详情不再显示评分（旧页常因文件名带 `1080p` 搜不到而隐藏）。
6. **删除 `DetailActivity` 会让历史 adb 取证脚本失效**：`docs/evidence/p256/aggregate-search/README.md`、
   `p240/*-stack.json` 等脚本引用 `.ui.detail.DetailActivity`；历史证据已归档，脚本属历史工具，未回改。
7. **D12-B 未合并**：旧宿主退役需先补齐 E1–E8 八项 parity 证据（启动契约/帧账/seek/AV 同步/软解/
   同名字幕/负向用例/中断收尾），否则 31 个脚本会「成功返回但拿不到证据」——最强的静默失败形态。

## 下一批
D12-B 阶段 1：新宿主接 `ProbeExtras` 兼容层 + 本地软解路由 + 同名外挂字幕，逐条跑 E1–E8；
通过后再改 31 个脚本指向、最后删旧宿主。
