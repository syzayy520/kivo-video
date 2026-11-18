# UI 落地清单 · 资源库 + 设置页

> 生成：2026-11-18 · 配套审查报告：`docs/review/2026-11-18-ui-review-library-settings.md`
> 用途：给落地执行的同事按文件逐项勾选。**本文件不改代码**，只把审查结论翻译成可执行 diff。
> 值来源：逐行核对真实源码（非报告复述）。凡与报告不同处，以本清单为准，见 §0。
> 2026-11-18 补：并入落地复审的三类裁决，见 §0.1。

---

## 0.1 裁决状态（2026-11-18 落地复审后）

| 项 | 状态 | 结论 |
| --- | --- | --- |
| chevron 几何二选一 | ✅ 设计侧已定 | 取 `RowChevron` 几何（12dp / 折点 0.28–0.72 / 1.4dp 圆头），能力取 `CalendarChromeControls.Chevron` 的 `pointsLeft` 参数化；合成单个 `KivoChevron`，见 §2 |
| 状态灯三色统一 | ✅ 设计侧已定 | 在线→`accentGreen`(34C759)、离线→`dangerRed`(FF3B30)、待检测→`textSub`；保留色点+中文语义双编码，见 §6 |
| reduced-motion 冲突 | ✅ 设计侧已定 | 降为纯 fade（150ms）**不归零**；取消位移但保留反馈，见 §5 |
| `KivoChevron` 带色值过门禁 | ✅ 设计侧已定 | 组件无自身颜色，`color` 由调用方传入 → 零裸色，天然过 `ui_gate` |
| **铁律 4「UI 已定稿」是否解除** | ❌ **待用户裁决** | 卡住 §1 textSub 全局调深（47 文件）+ §7 空状态新建。**未解除前这两项不动** |
| `SettingsEntryGrid` 是否保留 | ❌ 待用户裁决 | 注释写"按任务要求保留"，未确认前不删，见 §7 |

> 解读：设计侧能定的都已定稿；两个待裁决项都属于「要不要动全局视觉」，不是设计分歧，需用户给边界。

---

## 0. 核对修正（相对审查报告）

这几处是读源码后发现的，报告写得不够准或漏了，落地时按这里：

| # | 报告说法 | 核对后的真实情况 | 影响 |
| --- | --- | --- | --- |
| 1 | `SettingsScaffold.kt:46 fontRootTitle=32` 是死常量 | **整个 `SettingsScaffold` 函数（`:41-52`）都是死代码**，全项目零调用；`SubPageScaffold`（`:72`，被 12+ 子页调用）是活的 | 删除范围更大：删函数本身，`fontRootTitle` 随之为死常量 |
| 2 | 蓝色两值 `0xFF0A84FF` / `0xFF007AFF` | `0A84FF` 是 token 体系（DesignPalette.navActive = DesignTokens.navActive = SettingsVisuals.blue，**三处同值**）；`007AFF` 是硬编码，**共 5 处**（SettingsRows ×2、SettingsPicker ×1、SettingsDataRoot ×2） | 改 `007AFF` 五处 → 指向 navActive，别动 token 本身 |
| 3 | 只有一个现成几何 chevron（`SettingsRowItem` 私有） | **有两处**：`SettingsRowItem.kt:99 RowChevron()`（private/12dp/固定右向）+ `CalendarChromeControls.kt:97 Chevron(pointsLeft,size,color)`（internal/参数化左右/更通用） | 复用候选选后者，提到公共包做唯一源 |
| 4 | 符号当图标清单 | 报告漏了 `SettingsRows.kt:238 "⇕"`（排序）、`SettingsPicker.kt:95 "✓"`；`SettingsDataRoot.kt:54/68 "▦"/"≡"` 是**有意的 chip glyph**，不在此次替换范围 | 替换清单按 §2 的 10 处，别漏别多 |
| 5 | 设置页标题 34 / 资源库 28 冲突 | 设置页 34sp 是**活的正确值**（`SettingsPage.kt:124`）；32sp 在死函数里 | P0-3 实际只需改资源库一处 + 删死函数 |
| 6 | 三个死文件都可删 | `SettingsEntryGrid.kt` 在 `SettingsPage.kt:46` 注释里明确"按任务要求保留" | 只删 StatusTiles / StorageHero；EntryGrid 先裁决 |
| 7 | §7「复用 `ui/components/EmptyState.kt`」 | **该文件不存在**。全项目仅 `CalendarPage.kt`/`CalendarPageChrome.kt` 有 emptyState 字样；audit 规划的 `ui/design/components/state/EmptyState.kt` 也未落地 | 改为「新建统一 EmptyState」；且受铁律 4 约束，未解除前维持现状，见 §7 |
| 8 | 两套 chevron「复用其一」 | 两份几何**参数不同**（0.28–0.72/12dp 固定 vs 0.34–0.66/参数化），不能随便挑 | 已定：几何取前者、能力取后者，见 §0.1 / §2 |

---

## 1. Token 层（全局，先做，否则下游改了也白改）

| 文件:行 | 当前值 | 目标值 | 优先级 | 协同 |
| --- | --- | --- | --- | --- |
| `ui/tokens/DesignPalette.kt:25` | `textSub = 0xFF8A8A8E`（浅底约 3.1:1，不达 4.5:1） | 调深到 ≥4.5:1（参考 `SettingsVisuals.sub = 0xFF6B7280`） | **P0 / 阻塞** | ⛔ **被铁律 4 阻塞**：被 **47 个文件**引用，调深=全 App 视觉变更，不是两页局部改动。用户未解除铁律 4 前**不动**；解除后再做，须真机看整体 |
| `ui/tokens/DesignPalette.kt:34` | `navActive = 0xFF0A84FF` | **保留**，定为全 App 唯一蓝 | P0 | — |
| `settings/SettingsVisuals.kt:20` | `blue = 0xFF0A84FF`（硬编码，与 navActive 同值） | 改为 `DesignTokens.navActive` 别名（或保留同值但注明） | P1 | 组 A |
| `settings/SettingsVisuals.kt:33-34` | `sub=0xFF6B7280` / `sec=0xFF7A8089`（第 2/3 套次级灰） | 收敛到唯一 `DesignTokens.textSub`，删除这两个常量或改别名 | **P0** | 组 A；依赖 textSub 对比度裁决 |
| `settings/SettingsVisuals.kt:39` | `fontRootTitle = 32f` | 删除（唯一引用在死函数 SettingsScaffold，见 §4） | P1 | 组 C |

> ⚠️ **组 A 拆分**：原计划的原子提交（textSub 调深 + SettingsVisuals 收敛 + 5 处蓝）拆成两块——
> - **可立即做**：`SettingsVisuals.sub/sec` 收敛到 `DesignTokens.textSub` + 5 处蓝指向 `navActive`（不改 textSub 本身的值，视觉零变化，符合铁律 4）。
> - **阻塞**：`textSub` 本身调深（47 文件全 App 变更），等铁律 4 裁决。
> 分块后不出现"一半深一半浅"，且阻塞项不拖住可执行项。

---

## 2. 符号字形 → 矢量（P0-2，质感提升最直观）

**先做这一步**：抽 `KivoChevron(size, pointsLeft, color)` 到 `ui/design` 公共包，作为全 App 唯一 chevron 源。

**已定稿的实现口径**（三类裁决之一，见 §0.1）：
- 几何取 `RowChevron`——12dp、折点 0.28/0.72、描边 1.4dp 圆头（比 0.34/0.66 更平缓，接近 SF Symbols 细线气质）。
- 能力取 `CalendarChromeControls.Chevron`——`pointsLeft` 参数化左右，供返回键复用。
- **组件无自身颜色**，`color: Color` 由调用方传入 → 组件文件零裸色，天然过 `ui_gate` 基线 326。
- `SettingsRowItem.kt:99 RowChevron()` 改为调用它并删私有实现；`CalendarChromeControls.kt:97 Chevron()` 同理改为调用（保留 `internal` 签名不动下游）。
- 同步修 `SettingsVisuals.kt:8` 注释「骨架在 SettingsScaffold」——`SettingsScaffold` 已删，改为指向 `SubPageScaffold`。

| 文件:行 | 当前 | 目标 | 优先级 |
| --- | --- | --- | --- |
| `sources/ui/SourcesScreen.kt:113` | `Text("›", 22.sp)` 聚合卡 | `KivoChevron(pointsLeft=false, ...)` | P0 |
| `sources/ui/SourcesScreen.kt:124` | `Text("↻", 22.sp)` 刷新 | `Icons.Filled.Refresh`（保留现有 semantics） | P0 |
| `sources/ui/cards/StorageEntryCard.kt:10` | `Text("›", 22.sp)` | `KivoChevron` | P0 |
| `sources/ui/SourceCard.kt:56` | `Text("⋯", 24.sp)` | `Icons.Filled.MoreVert` | P0 |
| `settings/SettingsRows.kt:88` | `Text("›", 24.sp)` | `KivoChevron` | P0 |
| `settings/SettingsRows.kt:120` | `Text("›", 24.sp)` | `KivoChevron` | P0 |
| `settings/SettingsRows.kt:213` | `Text("✓", 20.sp)` | `Icons.Filled.Check` | P0 |
| `settings/SettingsRows.kt:238` | `Text("⇕", 16.sp)` 排序 | 排序矢量图标 | P1（报告漏列） |
| `settings/SettingsScaffold.kt:85` | `Text("‹", 24.sp)` 返回 | `KivoChevron(pointsLeft=true, ...)` | P0（与 §4 同处改） |
| `settings/SettingsPicker.kt:95` | `Text("✓", 18.sp)` | `Icons.Filled.Check` | P0（报告漏列） |

> `SettingsDataRoot.kt:54/68` 的 `"▦"/"≡"` 是淡底 chip 里的 glyph，属有意设计，本次**不动**。
> ⚠️ 矢量图标必须带 `contentDescription`（`Icons.*` 用 `null` 表示装饰性时确认语义不丢）；chevron 基线是否与文字对齐**需真机大字体档复看**，代码算不出来。

---

## 3. 蓝色硬编码（组 A 的一部分，单列便于勾选）

| 文件:行 | 当前值 | 目标值 |
| --- | --- | --- |
| `settings/SettingsRows.kt:149` | `Color(0xFF007AFF)` | `DesignTokens.navActive` |
| `settings/SettingsRows.kt:213` | `Color(0xFF007AFF)` | `DesignTokens.navActive` |
| `settings/SettingsPicker.kt:95` | `Color(0xFF007AFF)` | `DesignTokens.navActive` |
| `settings/SettingsDataRoot.kt:54` | `Color(0xFF007AFF)` | `DesignTokens.navActive` |
| `settings/SettingsDataRoot.kt:68` | `Color(0xFF007AFF)` | `DesignTokens.navActive` |

---

## 4. 设置页骨架（同文件，一组改完）

| 文件:行 | 当前 | 目标 | 优先级 |
| --- | --- | --- | --- |
| `settings/SettingsScaffold.kt:41-52` | `SettingsScaffold` 整个函数（死代码） | **删除** | P1 |
| `settings/SettingsScaffold.kt:80` | `Modifier.size(36.dp)` 返回钮（在活的 `SubPageScaffold` 里） | 视觉 36dp 保留，透明命中区撑到 48dp | P1 |
| `settings/SettingsScaffold.kt:85` | `Text("‹", 24.sp)` | `KivoChevron(pointsLeft=true)` + 48dp 命中 | P0 |
| `sources/ui/SourcesScreen.kt:79` | `Text("资源库", fontSize=28.sp, Bold)` | `fontSize = DesignTokens.fontPageTitle.sp`（34） | **P0**（改一处即可，设置页 34 是正确活值） |

---

## 5. 转场动效（P1-4，需先裁决一个矛盾）

| 文件:行 | 当前 | 目标 | 优先级 |
| --- | --- | --- | --- |
| `settings/SettingsNav.kt:38` | `tween<IntOffset>(350, easing=EaseOut)` 整页平移 | `tween(MotionTokens.pageTransitionMs /* 300 */, easing=FastOutSlowInEasing)` | P1 |
| `settings/SettingsNav.kt:39` | `tween<Float>(350)` fade | 与位移时长对齐或取 `MotionTokens.fadeMs`(150) | P1 |
| `settings/SettingsNav.kt` transitionSpec | 无 reduced-motion 分支 | 加 `MotionGate` 分支 | P1 |

> ✅ **已裁决**（见 §0.1）：`MotionTokens.kt:5` 注释写"Reduce Motion 时取 0"，但 Apple 标准是"更轻柔而非完全没反馈"——取后者：**降为纯 fade（150ms）不归零**，取消位移但保留反馈。落地时按此改 `MotionGate` 分支，并把 `MotionTokens.kt:5` 的注释同步改掉，别留两套说法。
> 需新增 import：`MotionTokens`、`FastOutSlowInEasing`、`MotionGate`。
> ⚠️ 推入手感**需真机 2–5× 慢放验证**，代码判断不了顺不顺。

---

## 6. 状态 / 无障碍（P1）

| 文件:行 | 当前 | 目标 | 优先级 | 注意 |
| --- | --- | --- | --- | --- |
| `sources/ui/SourceCard.kt:43-53` | 6dp 色点，`0xFF76C442`/`0xFFE65361`/`0xFFB8BBC4`，有中文 semantics（这点保留） | 加形状/文字双编码（在线实心+「在线」/离线空心+「离线」）；色值改为全局语义色 | P1 | ✅ **已定**（见 §0.1）：在线→`DesignTokens.accentGreen`(34C759)、离线→`DesignTokens.dangerRed`(FF3B30)、待检测→`textSub`。三色与旧值不同，是**刻意统一**，不是笔误 |
| `SourcesScreen.kt:92` | 分区标题「本地与存储」13sp 常规 | 与 `:119`「媒体服务器」16sp Medium 统一（建议 13sp Medium 或 16sp Medium） | P2 | 需真机截图看层级是否误判 |

---

## 7. 空状态 / 死文件（P2）

| 文件:行 | 当前 | 目标 | 优先级 | 注意 |
| --- | --- | --- | --- | --- |
| `SourcesScreen.kt:136-144` | 手写 Surface + 三段文字空状态 | 铁律 4 解除后**新建**统一 `EmptyState`（audit DUP-03 规划在 `ui/design/components/state/EmptyState.kt`） | P2 / 阻塞 | ⛔ `ui/components/EmptyState.kt` **不存在**，清单旧版写"复用"是错的（见 §0 #7）。且受铁律 4 阻塞：**未解除前空态维持现状，不动** |
| `settings/SettingsStatusTiles.kt` | 零调用 | 删除 | P2 | 铁律第 3 条 |
| `settings/SettingsStorageHero.kt` | 零调用 | 删除 | P2 | 铁律第 3 条 |
| `settings/SettingsEntryGrid.kt` | 零调用，但 `SettingsPage.kt:46` 注释"按任务要求保留" | **先不删**，确认那个任务要求是否还有效 | P2 | 裁决项，别直接删 |

---

## 8. 成组依赖（哪些必须一起做）

```
组 A1 可立即做（不改视觉，符合铁律 4）
  └─ SettingsVisuals.sub/sec 收敛 + 5 处蓝→navActive（§1 两行 + §3 全表）

组 A2 阻塞（等铁律 4 裁决）
  └─ textSub 全局调深（47 文件）+ 空态新建（§7）

组 B  chevron/图标（§2 全表）
  ├─ 前置：抽 KivoChevron(size, pointsLeft, color) 到公共包，几何/能力口径见 §2
  ├─ 交叉：SettingsScaffold:85 与组 C 同处，一起改
  └─ 顺带：两个旧 chevron 改调公共组件 + SettingsVisuals:8 注释同步

组 C  设置页骨架（§4 表 + SettingsScaffold:85）
  └─ 删死函数 + fontRootTitle + 返回钮热区 + chevron，一个文件一次改完

组 D  转场（§5）
  └─ SettingsNav + MotionTokens；reduced-motion 已定 fade 不归零，注释同步

组 E  资源库（SourcesScreen 一个文件：:79 / :92 / :113 / :124）
  └─ 标题 + 分区 + 两个符号，同文件一起改（空态 :136-144 属组 A2，阻塞）
```

**顺序**：B 的公共 chevron 抽取 → A1 / B / C / D / E 可并行 → 收尾验证。
组 A2 单独等用户裁决，不混入可执行批次。§6 状态灯用已定的语义色，不依赖 A2。

---

## 9. 验收（改完必做）

1. `python tools/ui_gate.py` —— **必过**，单文件 ≤300 行的门禁在这里，assembleDebug 看不出来。
2. `./gradlew :app:assembleDebug 2>&1 | grep -E "^e: |error:|BUILD"` —— 只看 `BUILD SUCCESSFUL`。
3. 真机（MatePad 优先，K40 锁屏时别碰）截图对比：
   - 大字体档：chevron / 图标基线是否对齐（P0-2）
   - 设置页子页转场慢放 2–5×（P1-4）
   - 资源库 vs 设置页大标题是否同尺寸（P0-3）
   - 在线状态双编码是否可辨（P1-5）

> 铁律提醒：UI 已定稿，本清单只做"消除方言/统一 token/矢量替换"，**不改变视觉设计方向**。任何看起来像"重新设计"的改动先停。
> 铁律 4 未解除前，§1 的 textSub 调深与 §7 的空态新建**不动**——这两项是全局视觉变更，其余项视觉零变化可做。
