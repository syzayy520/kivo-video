# 设置页 UI 重设计（268 · 浅色定稿 · 家族树先行）

> 性质：UI 设计卡（主助手自留）。用户 2026-11-13：「当前设置页完全没 UI、难看杂乱」→「先把浅色做好，能改进就改进，觉得好就实现」。
> 参考情绪板：`C:/Users/Administrator/Pictures/设置页/`（26 张 / 九套语言：REX·Reflix·Forward·EplayerX·Vidora·Noctra·DenPlayer·SenPlayer·Infuse）。
> 草图：会话可视化 `settings-redesign-mockup.html` v2（用户已认可浅色方向）。
> 裁决：真机操作 > 参考图 > 拆解推断。本草图只锁**视觉层**；267/266 的真实接线、持久化、导航栈**零改动**。

## 一、构思：为什么现状"没 UI"

对照真机 [s1-root.png](../../../evidence/p261/settings-page/shots/s1-root.png) / [c01.png](../../../evidence/p267/appearance-interaction/c01.png)：
零图标（无视觉锚点）、一节一孤卡（稀疏失衡）、置灰行整片灰段落（灰显墙）、
淡紫底+纯白卡+iOS 绿三套语言打架、字阶单调、无垂直 rhythm。
**骨架（淡紫渐变+白分组卡+胶囊底栏）是对的**（与 REX/Vidora/DenPlayer 同族），缺的是执行层。

## 二、视觉语言（v2 定稿）

- **底**：沿用 `DesignTokens` 淡紫→淡粉→米白渐变 + 白玻璃分组卡（radiusCard 13→16 视觉稿）。
- **图标**：**同色 14% 淡底圆角 chip + 彩色线 glyph**（Vidora 线图标 与 EplayerX 实心徽标 的中间态：
  比实心轻、比纯线有锚点）。root 与子页统一此语言。置灰态 chip `saturate(.3) opacity(.6)`。
- **行解剖**：`chip + 标题(17 semibold) + 副标题(11 gray) + 尾部`；尾部五态：
  开关(iOS 绿/灰) / 取值+⇅(picker) / 取值+›(nav-value) / ›(chevron) / ✓(radio)。
  副标题**保留换行不截断**——长不可用原因必须完整可读（铁律·不静默降级），不做单行 ellipsis 隐藏 reason。
- **不可用态降噪**：标题降 alpha、chip 降饱和、控件灰 + 可选 `footnote` 单行原因（组内行下）；
  reason 仍用户可见。子页图标为**选择性**加在主交互行（root 承载完整 chip 语言），置灰解释行保持安静。
- **根页 IA**：大标题 → 空间占用 hero 卡（真实缓存/磁盘字节→堆叠条+图例+总量，**可点→清理缓存**）
  → 播放体验 2×2 入口卡（播放/音画/操控/AI字幕，真实子页）→ 资源库概览双卡（Emby/SMB 真实配置计数）
  → 使用偏好/关于分组卡 → 版本脚注（左品牌右版本，等宽数字）。
- **字阶/节奏**：标题 32/800；节头 12.5/600/字距 .3；行 17/600；副标题 11；卡隙 12；横 inset 14；
  分隔线从 chip 后内缩；MB/版本用 tabular-nums。
- **动效**：push/pop slide+fade 保留；补开关弹簧、行按压 ripple（候选值真机拟合）。

## 三、自然家族树（只改视觉层；单文件单职责 ≤300）

```text
ui/design/settings/
├── SettingsVisuals.kt      新：settings 语义色板(chip 淡底/彩 glyph)/字阶/间距（单一职责）
├── SettingsIcons.kt        新：curated 线 glyph ImageVector 集（play/sliders/hand/subtitle/drive/brush/lock/info/cloud/globe/image/menu/speaker）
├── SettingsStorageHero.kt  新：空间占用 hero（真实字节→堆叠条+图例+管理跳转）
├── SettingsEntryGrid.kt    新：播放体验 2×2 入口卡
├── SettingsStatusTiles.kt  新：Emby/SMB 资源库概览双卡（真实配置计数，无实时连接探针）
├── SettingsRows.kt         升级：RowIcon 淡底 chip；行加 optional icon/footnote；尾部五态；超 300 拆 SettingsRowParts.kt
├── SettingsScaffold.kt     升级：根/子页 chrome（淡紫玻璃头+tint 返回钮）+ 节头 + 版本脚注
├── SettingsPage.kt         升级：根页 IA 重排（hero+grid+tiles+分组+脚注）
└── 其余 17 子页/Store/Nav   逻辑零改动；仅换行调用传 icon/footnote
```

- **复用**：`DesignTokens`、`GlassSurface`、`HomePreferences`、`SettingsStore`(缓存字节)、accounts(Emby/SMB)。
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