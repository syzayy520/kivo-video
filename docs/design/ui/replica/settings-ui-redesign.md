# 设置页 UI 重设计（268 · 浅色定稿 · 家族树先行）

> 性质：UI 设计卡（主助手自留）。用户 2026-11-13：「当前设置页完全没 UI、难看杂乱」→「先把浅色做好，能改进就改进，觉得好就实现」。
> 参考情绪板：`C:/Users/Administrator/Pictures/设置页/`（26 张 / 九套语言：REX·Reflix·Forward·EplayerX·Vidora·Noctra·DenPlayer·SenPlayer·Infuse）。
> 草图：会话可视化 `settings-redesign-mockup.html` v2（用户已认可浅色方向）。
> 裁决：真机操作 > 参考图 > 拆解推断。本草图只锁**视觉层**；267/266 的真实接线、持久化、导航栈**零改动**。

## 一、构思：为什么现状"没 UI"

对照真机 [s1-root.png](../../../evidence/p261/settings-page/shots/s1-root.png) / [c01.png](../../../evidence/p267/appearance-interaction/c01.png)：
零图标（无视觉锚点）、一节一孤卡（稀疏失衡）、置灰行整片灰段落（灰显墙）、
淡紫底+纯白卡+iOS 绿三套语言打架、字阶单调、无垂直 rhythm。
**分组卡+胶囊底栏的骨架是对的**（与 REX/Vidora/DenPlayer 同族）；但 v2 沿用的淡紫渐变底被用户否（"脏、粉紫不明"），v3 换中性冷灰底（见§二）。

## 二、视觉语言（v3 定稿 · Surge-clean）

> v2（淡紫渐变+白玻璃）真机后被用户否："底色脏、粉紫不明"；且任何描边/投影被看成"框"亦被否。
> v3 取 Surge 参考的冷灰底+扁平卡，但卡用**纯白**——Surge 的近白卡与底明度差太小、分组发糊，同样被否。
> 单变量并排草图裁决（settings-card-compare / settings-final）：**干净冷灰底 + 纯白扁平卡（无框无阴影）**
> = 清楚 + 安静 + 无框感，为最终语言。用户授权"一次设计到极致"。

- **底**：单一中性浅灰 `#F2F2F7`（= `DesignTokens.pageBackground`，iOS grouped 标准灰），**无渐变、无色相**（脏感根源=粉紫色相+高饱和，移除）。
  用户裁决「首页底色挺好的，统一用那个色」：首页 resting（`HomeAtmosphere` 收敛值）/资源库/我的/设置**四页共用同一画布**。
  落地：`AnimatedGradientBg(flatColor)` 外层全屏 Box 直接铺扁平色（状态栏/导航条同色全 bleed）；发现页/在看页保留原动画渐变。
- **卡**：纯白 `#FFFFFF`、**扁平无描边无投影**、圆角 20；靠纯白 vs 浅灰的明度差分层，不需要阴影。
  仅**浮起元素**（底栏胶囊/搜索圆钮）带柔和投影 `0 6 20 rgba(0,0,0,.08)`——那是深度不是框。
- **图标**：同色 14% 淡底圆角 chip(30/9) + 彩色线 glyph(17/1.9)；彩色**只**出现在 chip/链接/开关/存储条，其余全中性。置灰 chip 降饱和。
- **行解剖**：chip + 标题(16 regular) + 副标题(12 gray 换行不截断) + 尾部五态（开关绿/灰、值+⇅、值+›、›、✓）。
  分隔线 `#E3E5EA` 0.5dp；**有 chip 的行内缩 42dp 对齐文字**，无 chip 行内缩 16dp 对齐文字。
- **不可用态**：标题淡 `#B9BDC4`、chip 降饱和、控件灰；**reason 保持 `#6B7280` 完整可读**（不静默降级）。
- **根页 IA**：大标题 → 空间 hero（真实字节，细 4dp 条+单行图例+总量 tnum，可点→缓存）→ 2×2 入口
  → 资源库双卡（真实配置计数+›）→ 使用偏好/安全与关于分组 → 版本脚注(tnum)。
- **字阶/节奏**：页标题 32/800；hero 数 30/600 tnum + 副题 12 gray；节头 13/500 `#7A8089`；行 16/400；副 12 `#6B7280`；
  卡隙 12、横 inset 16、节间距 22、行 padding 14。副标题/节头灰取 ≥4.5:1 对比度（a11y）。
- **导航**：白胶囊 radius 28 + 选中灰 pill `#E8E9ED` + 蓝字/蓝图标（Surge）；搜索独立白圆钮。
- **子页**：tinted 返回圆钮（蓝 12%）+ 标题 22/700；其余同根页语言。
- **动效**：push/pop slide+fade 保留；开关弹簧、行按压微反馈（候选值真机拟合）；respect reduced-motion。

## 三、自然家族树（只改视觉层；单文件单职责 ≤300）

```text
ui/design/settings/
├── SettingsVisuals.kt      新：v3 tokens（冷灰底/纯白扁平卡/字阶/对比度灰/chip 淡底）单一职责
├── SettingsIcons.kt        新：curated 线 glyph ImageVector 集（play/sliders/hand/subtitle/drive/brush/lock/info/cloud/globe/image/menu/speaker）
├── SettingsStorageHero.kt  新：空间占用 hero（真实字节→堆叠条+图例+管理跳转）
├── SettingsEntryGrid.kt    新：播放体验 2×2 入口卡
├── SettingsStatusTiles.kt  新：Emby/SMB 资源库概览双卡（真实配置计数，无实时连接探针）
├── SettingsRows.kt         升级：RowIcon 淡底 chip；行加 optional icon/footnote；尾部五态；超 300 拆 SettingsRowParts.kt
├── SettingsScaffold.kt     升级：根/子页 chrome（冷灰底+扁平白卡+tint 返回钮）+ 节头 + 版本脚注
├── SettingsPage.kt         升级：根页 IA 重排（hero+grid+tiles+分组+脚注）
└── 其余 17 子页/Store/Nav   逻辑零改动；仅换行调用传 icon/footnote
```

- **复用**：`DesignTokens`（底/卡色改 v3）、`HomePreferences`、`SettingsStore`(缓存字节)、accounts(Emby/SMB)。
  settings 内容卡**不再用 `GlassSurface`**（改扁平纯白）；仅浮起导航保留玻璃/投影。
- **退役**：无（不新建重复组件；IconChevronRow 实心徽标升级为淡底 chip）。
- **零改动清单**：AppearancePrefs/PlaybackPrefs/HomePreferences 读写、266 accounts 写回、SettingsNav 栈、
  播放器消费、privacy 改密、SourcesScreen、缓存清理动作。

## 四、闸门与证据

1. 行数闸：新/改 `.kt` ≤300；超按变化原因拆。
2. 编译闸：`assembleDebug` BUILD SUCCESSFUL（读日志不读 exit code）。
3. 真机闸：K40 `b6e950ee` before/after 截图 + 与草图/参考**逐项对比**；复跑 266/267 写回抽查防回归。
4. 不静默降级：置灰行 reason 单行脚注可见；hero/tiles 只读真实字节/状态，读不到显示"未知"不编默认。
5. 证据：`docs/evidence/p268/settings-ui-redesign/`（截图+复算+README+披露）。
6. 收口：提交推送 + TASKS/DISPATCH 回写。

## 五、披露

- 空间占用分类只放**能真实测量**的类（图片缓存/评分缓存/其他）；不造假分类（REX 的 视频/下载/AI模型 我们无对应测量 ⇒ 不放或标未知）。
- **资源库双卡无实时连接探针**：Emby/SMB 只显示 accounts.bin 的真实配置计数（"N 个已配置"/"未配置"），
  不显示假"在线/已连接"指示灯；点按钻入数据与资源。读不到源记录显示 0/未配置，不编默认。
- 深色影院（Noctra 式）为候选上限，用户本轮拍板**先浅色**；深色留待后续卡。
- 草图数值（128MB/2 源）为示意；真机以实测字节/状态为准。
- **v3 拒框拒脏**：内容卡一律无描边无投影（用户明确否"黑边"）；仅浮起导航带投影。近白扁平卡（Surge 原样）因分组发糊被否，卡必须纯白。
- v3 底色/卡色/字阶为会话单变量草图裁决 + 用户"一次设计到极致"授权打磨；真机复验在实现卡完成。
- **统一画布**（用户 2026-11-13 拍板）：底色取首页 resting 色 `#F2F2F7`，资源库/我的/设置同步；发现页/在看页保留动画渐变（环境层职责不同）。
- **导航灰 pill + 蓝字本轮不动**：底栏是四页共享 chrome，改动会波及发现/在看页，留待独立卡；§二导航条为候选上限非本轮交付。