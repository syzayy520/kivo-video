# 270 · 详情页玻璃弹窗扁平化 · 真机证据

- 设备：Redmi K40 `b6e950ee`（1080×2400），2026-11-13
- 构建：`build-p270.log`（BUILD SUCCESSFUL in 17s）
- 规格：`docs/design/ui/replica/detail-popups-flat.md`（家族树先行；5 改 1 删，无新文件）
- 像素复算命令：`ffmpeg -v error -i <shot> -vf "crop=1:1:X:Y" -frames:v 1 -f rawvideo -pix_fmt rgb24 - | od -An -tu1`

## before（玻璃基线）

| 文件 | 场景 |
| --- | --- |
| `before/b1-more.png` | 「更多」玻璃弹出菜单（深色·生化危机：爆发夜） |
| `before/b2-calendar.png` | 播出日历玻璃底 sheet（深色） |
| `before/b3-season.png` | 选季玻璃菜单（宝可梦·8 季单条目） |

## after

| 文件 | 场景 | 要点 |
| --- | --- | --- |
| `after/a1-more.png` | 深色「更多」 | 不透明纯白、无 gloss 无描边、12dp 浮起投影、radius 20 |
| `after/a2-calendar.png` | 深色日历 | 纯白 sheet、顶 radius 30、scrim Black .32×fade、trash 在位 |
| `after/a3-season.png` | 宝可梦选季菜单 | 扁平纯白卡 8 季、勾选在第1季、无玻璃无模糊 |
| `after/a4-more-light.png` | 浅色「更多」（辛普森） | 真浅色环境纯白卡 |
| `after/a5-calendar-light.png` | 浅色日历 | 纯白 sheet + scrim |
| `after/a6-toast.png` | 下载行 | 诚实 Toast「下载功能尚未开放，敬请期待」 |
| `after/a7-remove.png` | 移除待看 | Toast「已移出待看」+ 日历 trash 消失 |
| `after/a8-share.png` | 分享行 | 系统 ACTION_SEND 选择器（微信/支付宝/Edge…） |
| `after/a9-season2.png` | 切季回归 | 触发器「第2季 橘子联盟 (橘子群岛)」+ 播放「第2季·1集」+ 日历「10-07 集36/36」 |

## 像素复算（卡内必须 255,255,255）

| 证据 | 坐标 | 实测 |
| --- | --- | --- |
| a1-more | (980,1660) / (980,1850) / (980,2040) | 255,255,255 ×3 |
| a2-calendar | (100,1800) / (900,2100) | 255,255,255 ×2 |
| a3-season | (400,1140) / (700,1600) / (100,2030) | 255,255,255 ×3 |
| a4-more-light | (980,1660) | 255,255,255 |
| a4-more-light | (100,1000) 画布 | 242,242,247（= #F2F2F7） |
| a5-calendar-light | (100,1800) / (900,2100) | 255,255,255 ×2 |

scrim 衰减复算（同坐标 (540,600)，a1 无 scrim → a2 有 scrim）：
(19,24,28) → (13,16,19)，逐通道比 0.684 / 0.667 / 0.679 ≈ 1 − 0.32，与 `Black.copy(alpha = .32f * fade)` 一致。

非白点披露：a2 (540,1300) = (97,7,7)、a5 (540,1300) = (172,155,0)，
均为日历**日期标记色**（深色红 / 橄榄黄），不是卡底漏色；卡底白由上表边缘坐标证明。

## 行为回归（全 PASS）

1. 添加到日历 = 待看 on + 日历打开（a2 trash 出现）；追剧进度行重开日历。
2. 分享 = 真系统选择器（a8），非假面板。
3. 移除 = 「已移出待看」Toast + trash 消失（a7），与 1 互逆。
4. 下载 = 诚实 Toast（a6），无假下载态。
5. scrim 点按 / BACK 均关闭弹窗；菜单外点按关闭选季菜单。
6. 切季（a9）：触发器名、播放按钮副标题、日历集数三处联动改写（82/82 → 36/36）。
7. 浅 / 深双环境对照：a1/a2（深）vs a4/a5（浅），卡内均纯白。

## 退役自证

- `ui/design/home/NavigationBackdrop.kt` 已删除（GraphicsLayer record/replay + RenderEffect 捕获桥）。
- 全仓 `grep -rn "NavigationBackdrop\|navigationSource"` = 0 命中。
- 行数：DetailMorePopup 116 / DetailSeasonMenu 232 / DetailCalendarSheet 178 / DetailOverlayHost 66 / DetailPage 288（全 ≤300）。
- blurOk / API31 分支整删：无模糊 ⇒ 不需要降级分支，符合「不静默降级」。

## 披露

- **深色选季菜单不可达**：Emby 服务器把多数季存为独立条目（`expandable = seasons.size > 1`），
  唯一多季单条目是宝可梦（1997，8 季），其详情页为浅色环境 ⇒ 选季扁平化以浅色 a3 + 同源 before b3 对照验证。
- 首次「添加到日历」tap 无响应（截图与 a1 相同），第二次 tap 生效；按调度偶发丢失处理，以成功态归档并验证终态。
- a3 首拍因资源库滚动位置漂移误落电影「活着」，已重新导航复拍覆盖（本目录 a3 为宝可梦正确帧）。