# 403 · 设计治理与缺陷修复规划（专业设计师视角）

> 2026-11-18 02:13 GMT+8
> 性质：**规划文档，本轮不改任何生产代码**。
> 前置裁决：UI 已定稿。允许的改动只有三类——① 真缺陷修复；② 文档回填到实测事实；③ 零视觉变化的可维护性收口。任何"重新统一视觉"的提案都必须单独授权。

---

## 0. 结论：最恰当的做法不是再统一一次视觉

审查报告里 60+ 条"不一致"，大部分**不是设计缺陷，而是文档与代码的版本差**。
如果按 P1-6/P1-7 的字面建议去"统一 token / 统一玻璃 / 统一蓝色选中"，会把已经真机验收过的 REX 几何和颜色推倒重来，制造新的回归。

**本规划把问题分成两堆：**

| 堆 | 判据 | 处置 |
| --- | --- | --- |
| A · 文档滞后 | 代码有更晚真机 golden 支撑，文档是旧稿 | **只改文档，代码零改动** |
| B · 真缺陷 | 用户可感知：读不清、重叠、点不到、卡死、假状态 | 按 P0→P1→P2 修，每项带真机证据 |

三条硬边界：
1. **REX 实测像素与播放器 overlay 几何不动**（轨道 6pt / 按钮 pitch 65 / 面板 248-16-42-60 / HUD 150-1000-300）。
2. **不新增视觉元素**；组件只有在 ≥2 个真实调用点时才允许新建。
3. **冲突裁决一律以"最新真机 golden + 现行代码"为准**，文档回填，不反过来用旧文档改代码。

---

## 1. 权威顺序（裁决树）

| 顺位 | 来源 | 效力 | 例子 |
| --- | --- | --- | --- |
| 1 | REX 真机 golden + 现行代码 | 几何/颜色/字号的唯一事实源 | `CapsuleNavBar.kt:21-34` 的 2026-11-16 实拍 |
| 2 | 已收口卡的现行实现 | 行为事实源 | 270 扁平弹窗、338 原地切服、339 退出修复 |
| 3 | `docs/design/style-unification.md` | 语言目标；只在无实测项时生效 | 页面语言"克制、扁平" |
| 4 | `docs/design/ui/replica/274-design-system.md` | **归档参考，无实施效力** | 文档第 3–14 行已写明"不实施 UI 改造" |
| 5 | `docs/design/apple-tv-principles.md` | 只补 REX 空白，不推翻尺寸 | 焦点五态、退场时序、对比度理由 |

### 1.1 五处冲突的明确裁决

| 冲突 | 裁决 | 依据 |
| --- | --- | --- |
| 导航选中：269 文档=蓝 `#0A84FF` vs 代码=白 60% 胶囊 + 近黑前景 | **保留代码现状，改 269 文档** | 代码注释指向 `2026_11_16_21_22_14_IMG_0366.jpg`，比 269 文档更晚；翻转会造成已验收视觉回归 |
| 圆角：style 16 / 代码 13 / 海报 12 / 浮层 20 / sheet 30 | **按角色保留多值，不强行归一** | 都是不同角色的 REX 实测值；统一后必破坏其中一类 |
| 强调色：style 的绿 vs 274/269 的蓝 | **蓝 = 导航选中与链接；绿 = 播放进度/能力徽章/正向状态；语义不重叠** | 两套基准都有效，分区后冲突消失 |
| 材质：玻璃 vs 扁平 | **页面扁平/渐变；详情浮层扁平纯白(270)；播放器内是唯一允许 75% 黑玻璃的区域** | 274 §1.3 的 M3 硬边界；`detail-popups-flat.md` 已收口 |
| 字阶：全仓 20 种 | **不合并数值**；只把现行代码用到的档位登记为语义令牌（名字统一，数值不动） | 合并字号 = 改视觉，超出本轮授权 |

> 这五条写进 `style-unification.md` 的新章节"裁决记录"，避免出现第六个互相矛盾的设计文档。

---

## 2. 真缺陷清单（只列必须处理的）

### P0 · 不可读 / 不可用 / 明显错误

| 编号 | 问题 | 证据 | 修复方向 |
| --- | --- | --- | --- |
| D1 | 详情页 hero 文字前景取自 `sheetColor`，文字实际压在 `field.value` 渐变上，白字压浅底不可读 | `DetailHeroContent.kt:120-123`、`DetailBackdrop.kt:53-64`、`p240/03_detail_top.png` | 前景色改从**合成后背景**判定；渐变暗段移到文字带下方；不新增描边以外的装饰 |
| D2 | Emby hero 中英文标题叠在同一行、英文溢出屏幕；封面失败留整屏空白；无评分时显示绿色 `Emby 0.0` | `switched_server.png`、`EmbyBrowseTopBar.kt` | 主/副标题分两行，`maxLines=1 + Ellipsis`；失败态收成 hero 内小 chip；无评分隐藏评分位 |
| D3 | 本地影片仍是 3 列 8dp 小格 + emoji 占位 + 10sp 文件名（SU-01 标 P0 至今未做） | `MediaLibraryActivity.kt:334`、`style-unification.md:38` | 2 列白卡（封面 16:9 或 2:3 按现行比例）、16dp 圆角、去扩展名、异步缩略图、标题 13sp |
| D4 | 字幕翻译只有纯虚接口 + 未链接静态库 + 无 JNI，桌面单测全绿造成"已完成"错觉 | `subtitle_translation_engine.hpp:45-83`、`CMakeLists.txt:100-143` | 不改 UI；文档明确标"仅契约层、App 不可用"，入口若有则置灰并给非空原因 |

### P1 · 用户可感知但可用

| 编号 | 问题 | 修复方向 |
| --- | --- | --- |
| D5 | 返回键至少 4 种样式 | 新建单一 `KivoBackPill`，视觉取**现行详情页 42dp 半透明圆 + chevron**，替换 Emby/日历/目录页；播放器 ✕ 属"关闭"语义，保留但注释区分 |
| D6 | 触摸目标 42/44/38/30/26dp 普遍 <48 | `HitBox` 下限 44→48；视觉尺寸不变，用透明 Box 扩命中区；进度条视觉 6dp、命中 48dp |
| D7 | 播放器三处手势 bug：`begin(0)` 哨兵吞片头横滑；`alpha=0` 控制栏仍吃事件；双指缩放跨 pass consume 不传播 | `active: Boolean` 替哨兵；hidden 态不让进度条参与命中；单指回调显式排除 ≥2 指 |
| D8 | 详情 preview 加载失败后永久停在 preview，无重试入口 | 失败态就近显示重试（按钮 ≥48dp），不新增页面 |
| D9 | `MotionGate` 的 App 关会覆盖系统"减弱动画"；播放器还有第二套门 | `reduce = systemReduced || !appEnabled`，删第二套门 |
| D10 | 模块管理是假开关：不驱动显隐、不持久化、离开页面即恢复 | 做成真开关：落盘 + 驱动「我的」页 item 显隐；视觉行样式不变 |
| D11 | 4 处主线程磁盘遍历/图片解码 | 统一改 `produceState + Dispatchers.IO`，沿用 `HeroPage.kt:72` 既有范式 |
| D12 | 两套详情页 + 两套播放 Activity + 两套同名 `GlassSurface` | 合并为单入口；旧入口退役须以行为 parity 为前提，单独批次做 |

### P2 · 稳态质量

- D13 首页冷启约 26 次 TMDB 请求无 in-flight 去重 → 同 URL 合并 + 最小间隔。
- D14 日历 Canvas 与播放器图标缺 TalkBack 语义 → role + onClick + stateDescription。
- D15 设置页开发术语、错误文案两份 → 收敛成一份自然语言文案。
- D16 21 个 Kotlin >300 行 → **只在改动到该文件时顺手拆**，不做全仓重构。

---

## 3. 文件家族树（实施前先冻结，写代码时严格照此）

```
ui/tokens/
  DesignPalette.kt                  [现状] 唯一原始色板，数值不动
ui/design/
  DesignTokens.kt                   [改] 唯一语义令牌；新增 HitTargetMin=48
                                    与 TypeScale 语义别名（数值=现行代码）
  CapsuleNavBar.kt                  [不改] 文档回填为"白胶囊 + 近黑前景"
ui/components/
  HitBox.kt                         [改] 44→48；新增透明命中 Modifier 重载
  KivoBackPill.kt                   [新] 唯一非播放器返回键，≤70 行，≥2 调用点才建
ui/detail/
  DetailHeroContent.kt              [改] 前景色同源判定、标题分行（≤160 行）
  DetailPage.kt                     [改] preview 失败就近重试（≤290 行）
  DetailCalendarGrid.kt             [改] 补 semantics
emby/ui/
  EmbyHeroHeader.kt                 [新] hero 标题/副标题/评分/失败 chip（≤130 行）
  EmbyBrowseTopBar.kt               [改] 只做顶栏编排，降到 ≤180 行
library/
  MediaLibraryGrid.kt               [新] 2 列白卡 + 异步缩略图（≤150 行）
  MediaLibraryActivity.kt           [改] 只做宿主与数据，降到 ≤200 行
ui/compose/
  HandSeekState.kt                  [改] active 布尔替 0 哨兵
  PlayerGestureRouter.kt            [改] 多指排除 + 边界 clamp 不累积
  PlayerBarsHost.kt                 [改] hidden 态不参与命中
ui/motion/
  MotionGate.kt                     [改] reduce 取或；删第二套门
ui/profile/modules/
  ModuleManagerSheet.kt             [改] 真持久化 + 回调驱动显隐
docs/design/
  style-unification.md              [改] 增"裁决记录"章节
  ui/replica/nav-capsule-v3.md      [改] 标注选中态被 11-16 golden 取代
  ui/replica/274-design-system.md   [不改] 保持归档标记
```

**删除/退役清单**：`ui/components/*` 已隔离的 9 个死代码文件不得复活；两套同名 `GlassSurface` 只留播放器侧 `PlayerGlassSurface`；新组件若无 ≥2 调用点，立即删除而不是留着。

---

## 4. 分期与门禁

| 期 | 内容 | 门禁（缺一不可） |
| --- | --- | --- |
| G0 | 本规划文档落盘 | 代码零改动；用户授权后再进 G1 |
| G1 | D1–D4（P0） | `assembleDebug` 出现 `BUILD SUCCESSFUL`；K40 `b6e950ee` + MatePad 前后截图；hero 对比度 ≥4.5:1；Emby 标题不溢出；本地影片 2 列；字幕入口状态如实 |
| G2 | D5–D11（P1） | 横滑在 0/中点/末尾三点真机跟手；双指缩放不跳进度；48dp 命中像素量测；返回键 5 处同款；开关重启后仍生效 |
| G3 | D12–D16（P2 + 结构） | 无新增 >300 行文件；旧入口 grep 0 命中；同 URL 请求数下降有日志 |
| G4 | 入库 | 按"代码 / 文档证据"两次提交；推送前复核 remote URL 与 `git status` |

> 编译判定只看 `BUILD SUCCESSFUL`；Kotlin 错误行是 `e: file://…`，不是 `error:`。
> 证据统一落 `docs/evidence/p403-design/`，每张图标注机号、时间、对应缺陷编号。

---

## 5. 明确不做（防范围蔓延）

- 不把导航选中改成蓝色，不把圆角统一成一个数，不引入全 App 玻璃风。
- 不改播放器任何已验收几何、轨道厚度、面板高度、HUD 位置。
- 不为"看起来统一"把 REX 实测值改成文档里的旧值。
- 不新建无调用点的组件；不复活已隔离的 1563 行死代码。
- 不在本轮做字幕翻译引擎实现（那是独立能力批次，不是设计缺陷修复）。

---

## 6. 一句话给执行者的口径

**"用最新真机事实封版，文档追认代码；只修读不清、点不到、显示假值这三类缺陷；其余一律不动。"**
