# Rex 拆解证据源总目录：三批证据的分工、冲突与合并结论

> 2026-11-08 复核：本篇保留为历史集成记录。“全部文案级确定状态机”“35 行”“启动屏等于全应用设计宽”不得直接沿用。实际为 32 个 CSV 场景；原件核验和资源缺口见 [审计索引](audit/status.md)。后续执行 [新规划](../../design/ui/replica/roadmap.md)。

> 状态：**集成完成**。本文是 rex-teardown 目录的证据源索引，三批证据互补使用。
> 第二批（本目录 `rex-package/`）由用户在 Mac 端用 REA v6.7.0 + `assetutil`/`plutil`/`sips`
> 对 **App Store 版 IPA（SHA-256 `ef2c21cc…4485`，FairPlay 加密 cryptid=1）** 静态拆解产出，
> 2026-11-07 以 `docs/rex.zip` 交付入库。

## 一、三批证据源与各自边界

| # | 证据源 | 位置 | 方法 | 能拿到 | 拿不到 |
| --- | --- | --- | --- | --- | --- |
| 1 | 静态第一部分 | `02-ui-teardown-part1.md` | Windows rea + Ghidra | 二进制侧路结论、KSPlayer 血统 | SwiftUI 声明式 UI |
| 2 | **Mac 静态全量包** | `rex-package/`（2 份 MD + CSV + 78 文案 JSON + 6 资产元数据） | REA v6.7.0 + assetutil/plutil/sips | 资产/文案/框架清单/启动屏约束/交互状态机 | 加密代码内的 padding/字号/圆角 |
| 3 | **真机录屏逐帧** | `05-ui-spec-from-recording.md`（本文姊妹篇） | ffmpeg 抽帧像素测量 | 运行时像素/动画时序/实际页面结构 | 加密常量的语义来源 |

**合并规则**：结构契约用 2，像素规格用 3，两者冲突时 3（真机实证）优先，冲突记录在 §三。

## 二、第二批包给到的关键增量（按 kivo 卡号索引）

### 全局
- **设计基准 393×852pt**（启动屏 nib 实测）——录屏设备为 390pt（iPhone 12/13 系），规格按比例不按绝对值。
- **5 个语义色 token**（`RexDesign` bundle，明暗双外观）：
  `canvasFill #FFFFFF/#222222` · `groupedFill #F2F2F7/#1C1C1E` · `pageBackground #F2F2F7/#000000`
  · `hairline 黑5%/白10%` · `washFill #EDEDFB@50%(P3)/#3D3D41`。
  ⚠️ washFill 浅色态是 **Display P3 + 50% α**，sRGB 直替有色偏；录屏里看到的淡紫渐变背景与它同族。
- **无第三方动效库**（Frameworks 22 项全列）：动效全为 SwiftUI/CoreAnimation 原生 →
  动效文档 §4 的 **P01–P12 参数配方**（spring 族 + Reduce Motion 降级矩阵）可直接移植 Compose。
- **15 语言 + RTL**（ar.lproj）：布局须支持镜像。
- **字体**：UI 走系统字体（SF Pro/PingFang SC + Dynamic Type）；包内唯一 ttf 是字幕渲染用的 PingFangSC。

### 卡片级
| 卡 | 增量事实 | 证据 |
| --- | --- | --- |
| 213 v2 | 色板 5 token、设计基准、P01–P12 动效配方、首页 15 种组件样式枚举、分区操作契约 | `Rex_UI结构复刻说明书.md` §3/§5、动效文档 §4 |
| 220 SMB | **Rex 用 AMSMB2.framework**（Swift SMB 客户端）——选型直接参照 | `Rex_UI结构复刻说明书.md` §6.2/§7 |
| 223 弹幕 | **弹幕是 JS Widget**（`RexScript_RexScript.bundle/OfficialModules/danmu.js` + cheerio）；另有 rex-tmdb.js / bangumi.js / nlsearch.js → 首页数据源可脚本化 | §7 |
| 224 Emby | Drivemby 流程四态文案（waiting/recognizing/uploading/done）+ 库类型全枚举（Emby/Jellyfin/WebDAV/SMB/百度/阿里/本地/Trakt） | §6.2 |
| 225/232 | **免费额度确定**：每日 3 次播放、1 个资源库、1 个观影环境；付费墙触发条件含 4K/120fps/DoVi P5+P8/HDR10/非 MP4 | 动效文档 M08 |
| 233 DV-B | **Rex 自带 libdovi + libplacebo**（Frameworks 内）——233 选型与 Rex 同栈；DoviMatrix Metal shader 是 Rex 自研层 | `Rex_UI结构复刻说明书.md` §7 |
| 潜在新卡 | **Anime4K 超分**（KSPlayer bundle：49 glsl + metallib，模式 A/A+A/B/B+B/C/C+A × HQ/Fast）；**AI 字幕**（onnxruntime）；CloudKit 同步（`Rex.ckdb` schema）；图标按配色+景别程序合成 | §6.10/§7 |

### 交互状态机（全部 ✅ 文案级确定）
- 双击热区三区可配（左/中/右）；锁定态**保留长按与双击**（非全禁）；
- 长按快进 → 左右滑调速 → 上滑锁定的完整链路；退出播放**必须还原系统亮度**（三种亮度策略）；
- 换集平滑过渡依赖预加载就绪（G5），失败态文案独立；
- 5 类网络错误 + 3 类加载态文案各自独立、不可合并；
- 多个点击特效是**轮询队列**非并发；孵蛋 3 蛋/3 候选/重置 1 次的长按入口；
- 抽奖关闭即放弃、再抽作废旧券 → 需作废动画 + 二次确认。

## 三、冲突裁决（静态推断 vs 录屏实证）

| 项 | Mac 包推断（◐） | 录屏实证 | 裁决 |
| --- | --- | --- | --- |
| iPhone 4 Tab 结构 | Libraries/Discover/**Watching**/Me | **发现/我的/资源库/设置** + 独立搜索钮 | **录屏胜**：「在看」不是独立 Tab（Mac 包按 strings 键序推断，已被真机推翻） |
| 页面底色 | `pageBackground #F2F2F7` 纯色 | 浅色**渐变**（淡紫→粉→米白，随页流动） | 两者共存：token 是基底，渐变是 washFill 族叠加/动画层；复刻按录屏观感实现 |
| 评分徽章 | （未涉及） | 六源聚合行（IMDb/TMDB/豆瓣/烂番茄/爆米花/豆） | 以录屏为准补入 05 §三 |
| 拖动进度预览 | `thumbnail.enabled` 开关存在 ✅ | 167×90pt 白描边真缩略图 + 相机圆钮截帧 | 录屏补齐像素规格（05 §6.2） |

## 四、仓库内文件清单

```
rex-package/
├── Rex_动效与交互全量拆解.md      # M01–M08 交互状态机 + P01–P12 配方（44KB）
├── Rex_UI结构复刻说明书.md        # 应用身份/色板/资产规格/逐页结构（23KB）
├── Rex_动效参数表.csv             # 35 行机器可读动效参数矩阵
├── strings/                       # 39 模块 × en + zh-Hans = 78 份文案 JSON（全文证据）
└── assets/                        # 6 份 assetutil 元数据 + main.json（911 项 Assets.car）
```

> `rex.zip` 原件与 `安装到docs-reference.command`（Mac 端一键拷贝脚本，源路径
> `/Users/huyasi/tools/rex-analysis`）已由本目录内容取代，不再保留于 docs/ 根。

## 五、仍未拿到（下一代增强，均不阻塞）

1. 加密二进制内的 padding/圆角/字号字重 —— 录屏量测已覆盖主要页面，精度 ±2pt 够用；
2. 精确动画时长/缓动 —— P01–P12 系统基线 + 录屏抽帧可校准（240fps 录屏最佳）；
3. DoviMatrix Metal shader 源码级反编译 —— metallib AIR 提取仍需 Mac Xcode 工具链（233 开工时再做）；
4. 运行时 Frida 视图树 —— 仅当 213 v2 对某个组件拿不准时按需做。
