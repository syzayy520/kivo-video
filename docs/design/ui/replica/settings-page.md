# 设置页 · REX 1:1 复刻设计（家族树先行）

> 性质：UI 设计卡（主助手自留，不派发）。用户：「我想把更重要的 UI 设计交给你，咱们的设置页还没真正开始」。
> 现状：`ui/design/Pages.kt:SettingsPage` 仅为骨架（首页/播放/关于三组 + 占位脚注），未真正开始。
> 参考：`docs/reference/rex-teardown/rex-package/strings/zh-Hans__Settings*.json`（入口/文案事实）、
> `Rex_UI结构复刻说明书.md` §6.7、`Rex_动效与交互全量拆解.md` / `Rex_动效参数表.csv`（动效候选值）、
> 既有基线 `docs/evidence/p236/p236-tab-settings.png`（set_a 帧：inset 分组卡 59pt 行 + 节头 + 多行脚注）。
> 裁决顺序遵 `roadmap.md` §二：真机操作 > 包内文案事实 > 旧截图 > 拆解推断。**文案证明有入口，不证明手机版位置/默认/动效**；
> 无真机对照处标「运行时未测」。PRO 付费 / iCloud / Trakt / Telegram / 模块系统 **不纳入**（roadmap 明确）。

## 一、构思：REX 设置页是什么

REX 设置 = **浅色 inset 分组列表**（`groupedFill #F2F2F7` 底 + 白卡 `radiusCard 13`），行高 59pt、
行标题 17pt、节头小字灰、组下**多行脚注"敢解释"**；根页为若干「分组卡」，每行要么**开关/取值**、要么**chevron 进子页**。
iPad/Mac 为双栏 Split（`settings.split.placeholder`），**手机为单栏 push/pop**（本卡只做手机）。
根页顶部大标题「设置」28pt Bold。交互：行按压反馈、开关弹簧、子页 push 滑入 + 返回 pop（参数取 CSV 候选值，真机拟合）。

## 二、Kivo 根页分组（只放真实能力，禁假行/假开关）

映射 REX taxonomy → Kivo 现状。**每行必须绑定真实持久化状态或真实导航**；无对应能力者不放（不伪造 REX 有而我们没有的行）。

| 分组 | 行 | 绑定（真实来源） | REX 对应键 |
| --- | --- | --- | --- |
| 播放 | 硬解优先·失败报 reason | 只读说明行（解码策略既有，不假开关） | `settings.root.video.preference` |
| 播放 | 网络明文传输 | `clearTextOk` 既有开关 | — |
| 播放 | 播放器按钮/手势/倍速 | 导航→既有播放面板设置（`PlayerPanel*`） | `settings.playerButtons.*` |
| 字幕 | 字幕渲染/轨道偏好 | 导航→既有字幕设置（`PlayerPanel*`） | `SettingsSubtitle` |
| 首页 | 预告片默认开关 | `HomePreferences.previewEnabled` | `settings.interface.page.trailer.autoplay` |
| 外观与交互 | 外观模式/减弱动画 | **若未实现则不放**（v1 仅浅色，不假深色开关） | `settings.interface.page.appearance` |
| 数据与资源 | 海报语言/图质量/元数据语言 | 导航→既有 TMDB 选图偏好（若已持久化） | `settings.aggregated.posterLanguage` 等 |
| 媒体服务器 | Emby / SMB 管理 | 导航→`SourcesScreen`（真实） | `settings.library.mediaServer` |
| 私密与安全 | 应用内数字密码 | 导航→`privacy/` 设置/改密（#39 真实） | —（Kivo 自有） |
| 存储与缓存 | 清理缓存（图片/接口） | 真实清理动作 + 确认弹窗 | `settings.cache.*` |
| 关于 | 构建 / UI 对齐 / 版本 | 只读真实值 | `settings.about.page.version` |

> 取舍：REX 的「模块/服务(Trakt/Telegram/iCloud)/PRO/追剧日历设置/AI 字幕」Kivo 无对应后端 ⇒ **不放**。
> 追剧日历入口已在首页浮层（#31），其"提醒/推送"无后端 ⇒ 不放设置行。

## 三、自然家族树（新包 `ui/design/settings/`，单文件单职责 ≤300）

```text
android/app/src/main/java/com/kivo/cinema/ui/design/settings/
├── SettingsStore.kt        唯一持久化源（SharedPreferences，仿 HomePreferences）；读写各真实开关
├── SettingsRows.kt         行原语：SwitchRow/ValueRow/ChevronRow/SectionCard/脚注（复用 GlassSurface/DesignTokens）
├── SettingsPage.kt         根页编排：分组卡列表 + 大标题；只组合 Rows，不堆业务
├── SettingsNav.kt          单栏 push/pop 路由 + 返回栈（手机），子页枚举
├── SettingsPlaybackSub.kt  播放/字幕子页（导航到既有播放面板设置的桥 + 真实开关）
├── SettingsDataSub.kt      数据与资源/媒体服务器子页（导航 SourcesScreen/TMDB 偏好）
├── SettingsCacheSub.kt     存储与缓存子页（真实清理 + 确认）
└── SettingsAboutSub.kt     关于/私密入口子页（只读 + 导航 privacy）
```

- **退役**：`Pages.kt` 的骨架 `SettingsPage`/`GroupSection`/`GroupRow` 迁出到 `settings/`（Pages.kt 只留 Profile/时间格式化），不新建重复组件。
- **复用**：`GlassSurface`、`DesignTokens`（rowHeight 59 / radiusCard 13 / fontRowTitle 17 / fontFootnote 10.5）、
  `HomePreferences` 持久化模式、`privacy/` 改密流、`SourcesScreen`、播放/字幕面板。
- 调用方：MainActivity 底栏「设置」tab 改指向 `settings.SettingsPage`（一处接线）。

## 四、动效（REX 候选值，真机拟合后定稿）

- 子页 push：右滑入 + 轻微 fade；pop 反向；时长/弹簧取 `Rex_动效参数表.csv` 候选，真机录屏拟合（不称常量）。
- 行按压：alpha/scale 微反馈（对齐 #15 已验收按压口径）；开关：弹簧 toggle（accentGreen）。
- 根页滚动：inset 卡随滚动，节头吸顶可选（REX 未测⇒v1 不吸顶）。

## 五、实现闸门

| # | 闸门 | 判据 |
| --- | --- | --- |
| S1 | 视觉 1:1 | 对照 set_a/REX：inset 分组卡 59pt 行、节头、多行脚注、浅色 tokens；真机截图对照 |
| S2 | 无假状态 | 每行绑定真实持久化/导航；无后端者不放；开关重启后保持 |
| S3 | 导航正确 | 单栏 push/pop 返回栈正确；深链子页返回不丢根页 |
| S4 | 家族/行数 | 新包单文件单职责 ≤300；Pages.kt 骨架迁出不留重复；不新增超闸文件 |
| S5 | 动效 | push/pop/按压/开关对照 REX 候选 + 真机丝滑（不破坏已验收基线） |
| S6 | 证据/红线 | 真机 K40 取证 `docs/evidence/p261/settings-page/`；凭据不入证据；含密码态不截图 |

## 六、证据与复算

`docs/evidence/p261/settings-page/`：根页/各子页真机帧 + 开关重启保持断言 + 返回栈断言 + 行数 + 复算命令。
设置页非 FLAG_SECURE（不含密码态时）可 screencap；进 privacy 改密态改 uiautomator。