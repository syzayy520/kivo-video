# G2-B · P1 真缺陷修复验证（403 规划第二批）

时间：2026-11-18 04:20 GMT+8
设备：K40 `b6e950ee`（A13，用户指定机型）
范围：D5 返回键统一 / D8 preview 失败重试 / D10 模块真开关 / D11 主线程 IO 收口。
家族树冻结见 `G2-B-PLAN.md`（含两处对旧规划的裁决修正）。

## 改动文件（15 个）

| 文件 | 缺陷 | 变化 |
|---|---|---|
| `ui/components/KivoBackPill.kt` | D5 | **新建** 42 行：统一 Role.Button + "返回" 语义 + 圆形底色槽，视觉全参数化 |
| `ui/detail/DetailHeroContent.kt` | D5 | `BackCircle` 委托 KivoBackPill；42dp/20dp/42dp 与 0x8CFFFFFF、Canvas 11×17dp/2.2dp 原样传入 |
| `ui/profile/calendar/CalendarPageChrome.kt` | D5 | 顶栏返回钮委托 KivoBackPill；44dp + canvasFill@0.6 + Chevron 原样 |
| `ui/detail/DetailPage.kt` | D8 | 根因修复：`error` 不再被 `content == null` 条件吞掉；有 preview 失败文案为「详情加载失败」，复用现有 Snackbar 重试 |
| `ui/profile/modules/ProfileModulePrefs.kt` | D10 | **新建** 72 行：SP 落盘 + observable 真值 + 区块映射 |
| `ui/profile/ProfilePage.kt` | D10 | 进入读真值、切换写盘、10 个区块按值显隐；管理胶囊恒显；234 行 |
| `MainActivity.kt` | D10 | 启动期 `ProfileModulePrefs.bind(this)`（进入页面无闪烁、组合期无磁盘 IO） |
| `ui/design/settings/SettingsPage.kt` | D11 | 两处缓存字节 → `produceState + Dispatchers.IO` |
| `ui/design/settings/SettingsAsrModels.kt` | D11 | ASR 模型统计 → 同上 |
| `ui/profile/thumbs/EntryCover.kt` | D11 | 封面解码 → 同上 |
| `MainActivityLocalMedia.kt` | D11 | 海报解码 → 同上；`thumbCache` 读写加 `@Synchronized` 防并发 |
| `privatezone/PrivateActivity.kt` | D11 | 删除「无 remember、每次重组都解码」，改 produceState |
| `ui/detail/DetailActivity.kt` | D11 | hero 缩略图 → produceState |
| `emby/aggregate/ui/LocalResultCard.kt` | D11 | 本地结果缩略图 → produceState |
| `evidence/IoTrace.kt` | D11 | **新建** 25 行：仅 DEBUG 落盘线程名+耗时，供真机判据 |

> D8 未新建 RetryPill：`DetailLoadFeedback` 已有 Snackbar 重试原语（视觉冻结），修状态层即可，遵守「不堆砌」。
> D5 未替换 Emby：其返回键是 `Surface + 0.5dp 描边`，与 Box 结构不同，换容器有像素风险；已记录为评估项，不新增第三参数分支。

## 编译
- `build-g2b.log`（首轮，6 个 `e:`）→ 修 smart cast 与漏删 import。
- `build-g2b-final.log`：**BUILD SUCCESSFUL in 1m 26s**。
- 清理脚手架/未用 import 后 `build-g2b-final2.log`：**BUILD SUCCESSFUL**。
- 全部改动文件 ≤300 行（最大 CalendarPageChrome 296；ProfilePage 234；DetailPage 282）。

## 验证结果

### D8 preview 失败 → 就近重试 —— ✅ 真机通过
- 证据：`g2b-d8-foreground.png`（K40，04:03）。
- 现象：带列表快照 + 不存在 id 进入详情 → 页面保留 preview（标题/海报/简介），底部出现 Snackbar
  **「详情加载失败，已保留当前内容　[重试]　[×]」**，indefinite 不自动消失。
- 修前行为：`if (content == null) error = ...` ⇒ 有 preview 时 error 恒 null ⇒ 永久静默、无入口。
- 说明：UI 路径被 TMDB 磁盘缓存命中干扰（点已缓存影片 load 成功，属正常），最终用应用进程内 instrumentation 注入无效 id 取证；脚手架已删除（`grep D8PreviewFailure` = 0），test APK 已卸载。

### D10 模块真开关 —— ✅ 真机五步闭环
- 证据序列（K40）：
  1. `g2b-profile-before.png`：周历 15–21 可见；
  2. `g2b-module-sheet.png`：管理模块 12 项全开；
  3. `g2b-module-calendar-off.png`：关「日历」→ 开关变灰；
  4. `g2b-module-hidden.png`：关闭弹层 → **周历条消失**，顶部直接是「今天」概览；
  5. `g2b-module-persist.png`：切「发现」再回「我的」→ 仍隐藏；
  6. `g2b-module-relaunch.png`：`force-stop` 重启 App → 仍隐藏（落盘生效）；
  7. `g2b-module-restored.png`：重开「日历」→ 周历恢复。
- 显隐覆盖：日历/观看概览/历史观看/待看/收藏/在看/本月观看/类型分布/走势1/走势2，共 10 项；
  「环境头」「热门 TOP3」「管理模块」胶囊恒显（胶囊常显是恢复入口）。
- **如实标注**：`Subscription`（订阅）/`Download`（下载）在现行「我的」页没有对应区块，开关真实落盘但不驱动 UI；
  未新增区块（UI 已定稿）、未删行（视觉基线不变）。

### D11 主线程 IO —— ✅ 7/8 处真机落盘实证；1 处受系统拦截至未取证
判据：`files/net-diag.log` 中 `[io]` 行的线程名。证据：`g2b-io-threads.txt`。

| # | 标签 | 线程 | 耗时 |
|---|---|---|---|
| 1 | `home.posterBlock` | DefaultDispatcher-worker-3/1/16/7 | 0–243ms |
| 2 | `profile.entryCover` | DefaultDispatcher-worker-1/8 | 1ms |
| 3 | `settings.imageBytes` | DefaultDispatcher-worker-1 | 53ms |
| 4 | `settings.ratingBytes` | DefaultDispatcher-worker-7 | 0ms |
| 5 | `settings.asrStats` | DefaultDispatcher-worker-1 | 0ms |
| 6 | `detail.heroThumb` | DefaultDispatcher-worker-7 | 95ms |
| 7 | `search.localCard` | DefaultDispatcher-worker-9/4/7/2 | 100–257ms |
| 8 | `private.posterCard` | **未取证** | — |

- 全部命中线程均为 `DefaultDispatcher-worker-*`，无 `main`。改前这些耗时直接落在组合期主线程
  （`PrivateActivity` 更严重：每次重组都解码）。
- 未取证原因：私密区需应用内密码，且页面受 FLAG_SECURE 保护，`screencap` 输出 0 字节；
  代码与其余 7 处同一范式（produceState + IO），**如实标注为「同范式、未单独真机取证」**。

### D5 返回键统一 —— ✅ 2 处替换，视觉零变化
- 详情页：`g2b-d5-detail.png` vs 改前 `p0-1-detail-after.png`；
  ffmpeg PSNR 裁剪对比：箭头实色区 `8×24@(105,160)` = **inf**（逐像素相同），圆底外沿 `6×6@(52,112)` = **inf**。
  整个圆域 PSNR 31.9dB 的差异来自 hero 背景缓推动画，与返回键无关。
- 日历页：`g2b-d5-calendar.png`，44dp 浅底圆 + 细 Chevron 原样（无改前基线，代码为等价替换）。
- 新增 `contentDescription="返回"` + `Role.Button`（视觉零变化，补无障碍语义）。

## 清理
- 临时 instrumentation 已删（0 命中），test APK 已卸载，设备 `/sdcard/g2b-*` 与 ui.xml 已清。
- 主包最终 `assembleDebug` = **BUILD SUCCESSFUL**。
- **未提交、未推送。**

## 下一批（G2-C 候选）
D6 命中区 44→48（进度条/播放器按钮像素量测）、D12 双详情页/双播放 Activity 合并、D13 TMDB in-flight 去重、D14 TalkBack 语义、D16 超 300 行文件顺手拆。
