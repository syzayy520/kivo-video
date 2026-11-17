# G2-B · P1 真缺陷修复家族树（冻结版）

> 2026-11-18 03:45 GMT+8
> 前置：`403-design-governance-plan.md` §3 家族树 + G2-A 已验证事实。
> 本文件是**实现前的家族树冻结**，写代码严格照此；偏离必须改本文件先。

## 裁决记录（新事实推翻旧规划的两处）

1. **D5 不做视觉统一**。规划原文写"视觉取详情页 42dp 替换 Emby/日历/目录页"，但只读定位实证 13 处返回键视觉参数两两不同（42/44/50/36/38/32dp；底色 5 种）。用户铁律"UI 已定稿、视觉零变化"优先 ⇒ `KivoBackPill` 做成**参数化容器**：统一命中/语义/绘制入口，逐处原样传入尺寸、底色、图标 lambda。替换前后逐页截图对比，像素不变。播放器 ✕ 属关闭语义，不动。
   - 实施收敛：详情页 `BackCircle` 与日历顶栏两处替换（均 Box+background+clip 同构，2 个真实调用点，满足组件门槛）。Emby 首页是 `Surface + 0.5dp 描边`，与 Box 结构不同，换容器有像素风险 ⇒ **不替换**，仅文档记录为已评估项；不为此新增第三参数分支。
2. **D11 的 4 处已失真**。原 4 处中 `ContinueHero.kt` 已随重构消失；另实证 5 处同类漏计（`SettingsAsrModels`、`MainActivityLocalMedia`、`PrivateActivity`(每次重组都解码)、`DetailActivity`、`LocalResultCard`）。本批按现存 **8 处**收口，不按文档字面 4 处。
3. **D8 不新建 RetryPill**。根因是 `DetailPage.kt:85-92` 的 `if (content == null)` 吞掉 error；`DetailLoadFeedback` 已有 Snackbar 重试原语（视觉已冻结）。修状态层即可复用，新增组件违背"不堆砌"。

## 本批文件家族树

```
android/app/src/main/java/com/kivo/cinema/
├─ ui/components/
│   └─ KivoBackPill.kt                  [新] ≤95 参数化返回键容器；统一 Role.Button + "返回" 语义
├─ ui/detail/
│   ├─ DetailHeroContent.kt             [改] BackCircle 委托 KivoBackPill，视觉参数原样传入
│   ├─ DetailPage.kt                    [改] error 置位不再被 preview 吞；复用 Snackbar 重试，≤290
│   └─ DetailActivity.kt                [改] D11 缩略图 produceState + IO
├─ emby/ui/
│   └─ EmbyBrowseTopBar.kt              [改] 返回键委托 KivoBackPill，44dp 淡蓝圆原样
├─ ui/profile/calendar/
│   └─ CalendarPageChrome.kt            [改] 返回键委托 KivoBackPill，canvasFill@0.6 + Chevron 原样（298→≤290）
├─ ui/profile/modules/
│   ├─ ProfileModulePrefs.kt            [新] ≤95 SP 落盘 + observable 真值；模块↔区块映射
│   └─ ModuleManagerSheet.kt            [不改] 行原语与回调签名冻结
├─ ui/profile/
│   ├─ ProfilePage.kt                   [改] 进入读盘 / 切换写盘 / 10 个区块按真值显隐；≤300
│   └─ thumbs/EntryCover.kt             [改] D11 produceState + IO
├─ ui/design/settings/
│   ├─ SettingsPage.kt                  [改] D11 两处缓存字节 produceState + IO
│   └─ SettingsAsrModels.kt             [改] D11 ASR 模型统计 produceState + IO
├─ privatezone/
│   └─ PrivateActivity.kt               [改] D11 移除无 remember 的每次重组解码
├─ emby/aggregate/ui/
│   └─ LocalResultCard.kt               [改] D11 produceState + IO
├─ MainActivityLocalMedia.kt            [改] D11 produceState + IO；thumbCache 改 ConcurrentHashMap
├─ evidence/
│   └─ IoTrace.kt                       [新] ≤45 仅 DEBUG 落盘线程名+耗时；验证主线程无 IO
└─ docs/evidence/p403-design/
    ├─ G2-B-PLAN.md                     [本文件]
    └─ G2-B-STATUS.md                   [验收集，实现后写]
```

**禁止**：不改 `PlayerTopBar`/`PlayerDetailCard`/`DetailActionSheet`/`ModuleManagerSheet` 的 ✕（关闭语义）；不改导航胶囊、不改任何颜色/尺寸/圆角数值；不复活隔离死代码；不新建无 ≥2 调用点的视觉组件（`IoTrace` 是取证工具，8 调用点）。

## 逐项验收判据

| 缺陷 | 判据 | 证据 |
|---|---|---|
| D5 | 三处返回键替换前后同页截图逐像素一致；语义统一 `contentDescription="返回"` | `g2b-back-*.png` |
| D8 | 带 preview 进入 + load 失败 → Snackbar"已保留当前内容 / 重试"可见；点重试后内容加载 | `g2b-preview-retry-*.png` |
| D10 | 关「日历」→ 周历消失；切 Tab 再回仍隐藏；重开恢复；胶囊常显 | `g2b-module-*.png` |
| D11 | `IoTrace` 日志 8 处线程均为 `DefaultDispatcher-worker-*`；无 `main`；页面首帧先出占位 | `g2b-io-threads.txt` |

## 实施顺序

D8（最小、根因清晰）→ D10（状态闭环）→ D11（8 处机械替换）→ D5（视觉最敏感，最后）→ 一次编译 → 装包 → 真机逐项取证。
