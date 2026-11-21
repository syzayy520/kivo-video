# 首页「继续观看」`HomeEntry` 迁移收尾 · 文件家族树

> 2026-11-22 · 接管收尾（用户确认：当前只有本助手在工作）
> 目标：**编译恢复 + 可验证换编码的位置保持**。不扩首页视觉、不改播放核心。

## 0. 现状（实测）

| 调用方 | 期望类型 | 现在传进来的 |
| --- | --- | --- |
| `MainActivityShell` → `DiscoverPage.continueItems` | `List<HomeEntry>`（首页=NAS+Emby） | 已是 `HomeEntry` ✓ |
| `MainActivityShell` → `ProfileStats.build` | `List<Pair<MediaItem, Progress>>?` | `List<HomeEntry>` ❌ 类型不匹配 |
| `MainActivityShell` → `WatchingPage.continueItems` | `List<Pair<MediaItem, Progress>>` | `List<HomeEntry>` ❌ |
| `MainActivityShell` → `DiscoverPage.posterOf` | `(HomeEntry) -> Composable?` | `activity.entryPoster` **不存在** ❌ |
| `MainActivityShell` → `WatchingPage.posterOf` | `(MediaItem) -> Composable?` | `activity.entryPoster` **不存在** ❌ |
| `MainActivity.onResume` → `refreshNasLibrary()` | — | **不存在** ❌ |

`HomeFeedAggregator.merge(local: List<Pair<MediaItem, Progress>>, emby: List<HomeEntry>)`
只在**本地源**上构造 `HomeEntry`；NAS 分节 `NasLibrarySource.Section.entries` **本身已是 `HomeEntry`**，
不应再过 `merge`。

## 1. 文件家族（每个文件一条变化原因）

```
MainActivity.kt
├── entryPoster(entry)   → 已移回 MainActivityHomeFeed.kt（HEAD 原位；本文件只留状态所有权）
├── posterForMedia(media) 负责：MediaItem → 海报 Composable（ThumbnailReader 抽帧，在看页用）
└── HomeEntry.toPair()   负责：HomeEntry → Pair<MediaItem, Progress>?（mediaItem 为空即丢）

MainActivityHomeFeed.kt（首页数据供给唯一职责文件）
├── refreshNasLibrary()  负责：NasIndexStore 读索引 → 过期/空则 NasScanService 全扫 →
│                        NasLibrarySource.sections → shellState.nasSections（HEAD 原语义恢复）
├── refreshContinue()    负责：本地(仅在看页) + SMB(SmbContinueSource.recent) + Emby(EmbyHomeResume
│                        增量回调) → homeEntries / continueItems（HEAD 原语义恢复）
└── entryPoster(entry)   负责：HomeEntry → 海报 Composable（Emby=EmbyResumePoster；
                         本地/SMB=localMedia.posterBlock；无 mediaItem=NasPoster）

MainActivityShell.kt
└── 调用点收口           负责：把 HomeEntry 映射回 Pair 再喂给 ProfileStats / WatchingPage
```

## 1.1 语义裁定（2026-11-22 接管收尾时定）

`homeEntries` = **继续观看行**（SMB + Emby），**不**把 NAS 片库展平灌进去。理由：
`DiscoverPage` 的片库分类行已直接消费 `nasSections`（`NasLibraryRow`），
把 `nasSections` 展平进 `homeEntries` 等于把整库塞进「继续观看」行，且与
`refreshContinue` 形成两个 homeEntries 写入者互相覆盖。中途出现过的
「展平版 refreshNasLibrary」（读 shellState.nasSections 合并 Emby）已删除，
`nasSections` 的唯一写入者回到 `refreshNasLibrary`（读索引/全扫/分节）。

**类型不丢原则**：`HomeEntry.mediaItem`（Local/Smb 非空）+ `positionMs/durationMs/lastPlayedMs`
可以**无损**还原 `Pair<MediaItem, Progress>`，所以**不需要改** `ProfileStats` / `WatchingPage` 的内部。

## 3. 验收（2026-11-21 接手线实测结果）

| # | 判据 | 结果 |
| --- | --- | --- |
| A | `:app:assembleDebug` 绿 | ✅ BUILD SUCCESSFUL（恢复 HEAD 数据供给后复验） |
| B | `onResume` 路径正常、NAS 数据可见 | ✅ `net-diag.log`：`smb=2`、`emby 完成 成功源=3/3 条=13`、`homeEntries=10`；截图 `docs/evidence/ui-shot/home-nas-restored2.png`（继续观看 2 张 NAS 卡 + 电影/剧集分类行真实海报）。⚠️ 原 B 的「NAS 展平进 homeEntries」已按 §1.1 裁定**不采用**——片库行走 `nasSections`，不展平 |
| C | ProfileStats/WatchingPage 收到 `Pair` | ❌→✅ **本判据已推翻**（2026-11-21 晚）：Pair 化把 `WatchingPage`/`ProfileStats`/`ContinueSource` 签名改坏，而 `SmbContinueSource` 构造 `HomeEntry` 不带 `mediaItem` ⇒ `toPair()` 把 SMB/Emby 全滤掉，在看页空态（真机截图证实）。已恢复三文件 HEAD + Shell 调用 HEAD 形态（收尾版本备份 `docs/evidence/p9-subtitle-reaccept/overshadowed/`）；**在看页直接吃 `List<HomeEntry>`**（HEAD 原口径），修复后 3 NAS + 3 本地完整列表（`B1-watching-fixed.png`）、看板正常（`B1-profile-fixed.png`）。`toPair()`/`posterForMedia`/`openMediaItem` 成孤儿 |
| D | entryPoster 分源渲染 | ✅ Emby=`EmbyResumePoster`、本地/SMB=`localMedia.posterBlock`、无 mediaItem=`NasPoster`（首页海报全部真实图，见截图） |
| E | 换编码位置保持 ≤2s | ✅ `pre-pos pos=30.1` → `pos_after=30.166667`（差 0.067s），证据 `docs/evidence/p9-subtitle-reaccept/POS-DEVICE.md` |

## 4. 不做（不越界）

- 不补 `smbPosterPath` 的 NAS 封面取数（要源 id/share 解析，属另一批）。
- 不改 `ProfileStats` / `WatchingPage` 的内部实现（用映射避免大改）。
- 不碰播放内核、面板材质、编码归一化。
- 不提交、不推送（除非用户另行要求）。