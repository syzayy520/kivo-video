# 在线字幕 · 设计稿（评审用，未决策不实现）

> 依据：`research.md`（平台调研）。本文是功能与结构设计，供用户决策；**决策前不写实现代码**。

## 一、目标与范围

为 Kivo 增加「在线字幕」闭环：**搜索 → 选择 → 下载 → 落本地字幕库 → 自动挂载当前播放**。

第一批（建议）：
- 数据源：**SubHD（中文首选）+ OpenSubtitles（官方 API，主源）**。
- 入口：**播放页字幕面板新增「在线字幕」Tab**（与「本地字幕」并列）。
- 结果：真实列表（源/语言/格式/评分或下载量/字幕名），可下载 SRT/ASS，下载后自动选中当前字幕。
- 设置：语言偏好、OpenSubtitles key/账号、默认源。

不做（第一批）：Zimuku/Podnapisi/Addic7ed、详情页独立字幕入口、自动按文件名批量下载全季、字幕上传。

## 二、用户流程

1. 播放中点「字幕」→ 面板分「本地字幕 / 在线字幕」。
2. 在线 Tab 自动以当前媒体信息搜索：片名（解析后的 title）+ 年份 + 季/集（剧集）+ 版本文件名（release 匹配）。
3. 两源并行查询，结果合并排序：语言偏好（简体→双语→繁体→英文）→ 评分/下载量 → 版本匹配度。
4. 点某条 → 下载到本地字幕库（复用现有存储与编码归一化）→ 自动选中该字幕（播放面板立即可用）。
5. 失败/无结果/配额/登录要求，给非空可见原因与重试；可切换「键盘输入关键词」重搜。

## 三、数据模型（真实字段，不编造）

`OnlineSubtitle`：
- `source`：`opensubtitles` / `subhd`
- `title`：字幕标题（含 release 信息）
- `language`：简体 / 繁体 / 英语 / 双语 / 其他（源原值归一）
- `format`：SRT / ASS / SUP（SUP 只展示，不承诺可直接挂 SRT 管线）
- `ratingUp` / `ratingDown`（SubHD 赞/踩；OpenSubtitles 用评分/下载量映射，若字段存在）
- `downloads`：下载次数（SubHD 有）
- `uploader`：上传人
- `releaseName`：原始发布名（用于版本匹配）
- `pageUrl`：详情页（可追溯）
- `downloadUrl`：真实文件地址（http/https only）

匹配分（可解释，不伪造热度）：语言偏好 + release 与当前文件名包含度 + 下载量/评分；同分按源序（SubHD 优先中文）。

## 四、下载与落库（复用既有铁律）

- 仅 http/https；单文件 ≤ 8 MiB（沿用远端链接导入上限）；HTML 内容拒收（防把详情页当字幕）。
- 内容哈希去重（同文件不重复落库）；文件名 sanitized（保留 release+语言+源）。
- 编码归一化沿用 `project_subtitle_encoding_panel`：宁乱码不丢字幕、BIG5 ⊂ GB18030、只记显式选择。
- 落库位置 = 现有本地字幕库（SubtitleImportStore 体系），供播放面板「本地字幕」直接读取；
  下载完成默认选中当前字幕（可关）。
- OpenSubtitles 下载需登录时：跳「账号设置」而非静默失败；配额用尽显示额度与重置时间（若字段提供）。

## 五、设置项（设置 → 播放体验 → 字幕）

- 在线字幕开关（默认开；关闭时面板隐藏在线 Tab，不显示「待接入」占位）。
- 语言偏好（多选排序：简体 / 双语 / 繁体 / 英语）。
- 默认源（自动=中文 SubHD 优先、OpenSubtitles 补充）。
- OpenSubtitles：Api-Key（内置默认 + 可自填）、账号/密码（下载需要时登录；密码只存私有加密存储）。
- 下载后自动选中（默认开）。

## 六、文件家族树（不新建项目包，单文件单职责）

```
android/app/src/main/java/com/kivo/cinema/
├── subtitle/online/                       # 在线字幕数据族（无 Compose 依赖）
│   ├── OnlineSubtitleModels.kt            # OnlineSubtitle / 语言归一（按系统语言排序）/ 匹配分（纯函数）
│   ├── OpenSubtitlesClient.kt             # 官方 REST API v1：搜索/下载/鉴权/配额错误
│   ├── SubHdClient.kt                     # SubHD 网页解析：搜索/详情/下载链接/限频/域名回退
│   ├── OnlineSubtitleRepository.kt        # 两源并行查询、合并排序、取消与错误分类
│   ├── OnlineSubtitleDownloader.kt        # 下载校验（类型/大小/哈希）并写入本地字幕库
│   └── OnlineSubtitleTranslator.kt        # 外语字幕 → 中文（复用 kivo_core_subtitle_translation）
└── ui/subtitles/online/                   # 在线字幕 UI 族
    ├── OnlineSubtitleTab.kt               # 面板在线 Tab：搜索态/结果态/下载态/错误态
    ├── OnlineSubtitleResultList.kt        # 结果行：源/语言/格式/评分下载量/字幕名/下载与翻译
    └── OnlineSubtitleSettings.kt          # 设置项（key/账号/语言/自动选中）

设置 → 关于：数据与致谢区（OpenSubtitles / SubHD 名称、链接、用途与感谢文案）。
```

接线（最小改动、只读播放）：播放字幕面板加 Tab；下载成功后由面板重新读本地字幕列表并选中。
**不碰 `playback_graph/mpv/libass`。**

## 七、验收口径（真机 b6e950ee）

1. 《奥德赛》《谷雨》在线搜索出真实字幕列表（源/语言/格式/下载量正确）。
2. 下载 SRT 与 ASS 各一条：落库可见、自动挂载、中文无乱码（含 BIG5 样本）。
3. 无结果、断网、SubHD 域名失效回退、OpenSubtitles 未登录/超配额，均有非空可见原因。
4. 播放手势与既有面板交互不回归（字幕面板可正常唤起/关闭）。

## 八、待用户决策（回复后按决策实现）

**决策已落锤（2026-11-24 用户逐条回复）：**

1. **OpenSubtitles 鉴权：接受**。Api-Key「内置默认 + 可自填」；账号密码在设置里填（不填只能搜索不能下载）。
2. **SubHD 网页解析：接受接入**（限频 + 域名回退 + 结构变化容错）。
3. **语言排序改为按系统语言**：不固定中文优先。取 `Locale.getDefault()` 的语言/地区决定偏好：
   中文环境按 简体 → 双语 → 繁体 → 英文；非中文环境按 系统语言（如 English）→ 双语（含该语言）→ 其他语言 → 中文。
   设置里保留手动改排序（可选覆盖系统默认）。
4. **下载后自动选中当前字幕：接受（默认开）**。
5. **Zimuku / Podnapisi 不做**（其他播放器未采用的不琢磨；不追新源）。
6. **设置 → 关于：必须列出在线字幕数据来源与致谢**（OpenSubtitles、SubHD 名称/链接/用途说明与感谢文案；
   不只字幕，后续第三方服务沿用同一「数据与致谢」区）。
7. **外语字幕下载后可再翻译成中文**：复用既有 `kivo_core_subtitle_translation` 工具链（不碰 libass）。
   在线结果对外语（非中文）字幕提供「下载并翻译成中文」：先下载原文，再调现有翻译管线生成中文字幕并选中；
   翻译能力不可用时给可见原因，不得伪造翻译结果。

后续约束：设置里的说明/关于文案为正式用户可见内容，不写开发备注；API 字段以官方文档为准，
OpenSubtitles 端点/配额在实现前需再次核验（当前 stoplight 文档需 JS，未实时核验）。