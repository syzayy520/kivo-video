# P9 字幕 · 复验记录（UI 层）

- 设备：`b6e950ee`（Redmi K40，只此一台；`dd137cf7` 属其他助手，本轮未触碰）
- 构建：`assembleDebug`，包含 低-2 修复（`MpvSubtitleControl.disableSubtitles` 同时清 `secondary-sid`）
- 测试片源：`files/media/kivo-ui5.mkv`（35 s，1 视频 + 1 英语 subrip），`--es kivo.mpvOpt "loop-file=inf"` 保持文件常驻
- 原始日志：`mpv-kernel-p9fix.log`（内核属性写入）、`p220-status-p9fix.log`（面板事件）、`p9-panel-p9fix.log`（面板几何/行数据）
- 截图 / a11y dump：本目录 `p9ui-*.png`、`ui*.xml`

## PASS

| # | 项 | 证据 |
| --- | --- | --- |
| 1 | 字幕入口面板打开（7 行，含 导入字幕 / 显示设置 / 关闭字幕） | `p9-panel-p9fix.log` `kind=subtitle … n=7` |
| 2 | 内嵌字幕 → 轨列表（关闭字幕 + 英语） | `... n=2 first=关闭字幕 tags=[sub-off, 1]` |
| 3 | 选轨 = `sid=1` | `mpv-kernel-p9fix.log` `Set property: sid="1"` |
| 4 | 关闭字幕 = `sid=no`（无次字幕时不写多余属性） | `[189.753] Set property: sid="no"`，其后无 secondary 写 |
| 5 | 次字幕选择 = `secondary-sid=1` | `[331.212] Set property: secondary-sid="1"` |
| 6 | **低-2 修复**：主字幕已关、次字幕在用时再点「关闭字幕」，两轨一起清 | `[532.319] sid="no"` → `[532.320] secondary-sid="no"` |
| 7 | 关闭后入口面板状态自洽：内嵌字幕=关闭 / 次字幕=未启用 | `p9ui-secoff.png`、`uiJ.xml` |
| 8 | 显示设置面板 18 行（ASS/预设/微调/颜色/效果/同步/次字幕） | `... kind=subtitleDisplay … n=18` |
| 9 | 「字幕延迟」步进：+ 写入并被界面刷新 | `[1524.695] sub-delay="0.1"`；`uiX.xml` 显示 `0.1 s` |
| 10 | 「重置」批量回默认（字号 40、ass-override no、delay 0、spacing 0.0、bold no、secondary-sub-pos 15、secondary-sub-visibility yes） | `[1572.27x]` 一组 `Set property` |
| 11 | 导入字幕入口拉起系统选择器 | `p220-status` `sub-import: launching SAF picker` / `launch() returned ok`，前台为 DocumentsUI PickActivity |

## 未覆盖 / 不成立

| # | 项 | 说明 |
| --- | --- | --- |
| A | 导入字幕的**端到端**（选择 → 拷贝 → `sub-add`） | SAF 选择器里定位目标 srt 不可靠（Download 混杂数百文件），本轮只验证到"入口打开了正确选择器"；拷贝与挂载逻辑此前由 `kivo.subFile` 取证钩子覆盖 |
| B | 「搜索字幕」 | 入口存在但文案明示"搜索服务未接入"，属第三波（网络字幕发现）范围，不算缺陷 |
| C | 多内嵌轨切换 | 测试片源只有 1 条内嵌字幕；多轨切换需另一片源，未做 |

## 新发现的界面缺陷（本轮复验暴露，未修）

| # | 现象 | 证据 |
| --- | --- | --- |
| D-1 | 显示设置面板滚动到底部时，「加粗」行与「字幕延迟」行文字**叠印**（加粗行 176 px，延迟行被压在同一位置） | `p9ui-display4.png`、`uiV.xml`（加粗 rect 高 176，同步头 860，延迟行 905 起被页脚压住） |
| D-2 | 底部「重置」页脚**盖住**最后两行（字幕延迟 / 次字幕位置），需精确上滚才能点到 | `uiW.xml`：重置 `[1374,905][2320,1036]` 与延迟行区域重叠 |

> D-1 / D-2 属面板质感专项（任务 #8），修复需重排显示设置卡的列表底部内边距与行高，不与字幕逻辑耦合。

## 设备状态恢复

- 字号已回 40（`sub-font-size="40"`），ASS 覆盖回 `no`，延迟 0，其余样式回默认（已核对 `[1572.27x]` 批次）
- 已删除临时片源副本 `kivo-ui-embed / kivo-ui2..ui5.mkv`、`files/evidence/prop-diag.log`、`/sdcard/ui*.xml`、`/sdcard/p9ui-*.png`
- 应用已 force-stop，配置未改动