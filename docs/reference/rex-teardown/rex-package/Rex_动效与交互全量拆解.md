# Rex 1.0.2 · 动效与交互全量拆解

> 配套文档：`Rex_UI结构复刻说明书.md`（页面结构 / 组件 / 布局 / 色值）
> 目标包：`Rex_1.0.2.ipa` · 分析工具：REA v6.7.0 + `assetutil` / `plutil`
> 机器可读参数表：`Rex_动效参数表.csv`

---

## 0. 口径：确定 / 推断 / 待确认

| 等级 | 定义 | 本文标注方式 |
| --- | --- | --- |
| ✅ **确定** | 随包资源中可直接复算的事实（文案状态名、次数阈值、网格尺寸、色值、开关存在性、Frameworks 清单） | 值 + 证据键名/文件 |
| ◐ **推断** | 由确定事实 + 平台默认值推出的参数，**逐条注明推断依据** | 值 + 「依据：…」 |
| ❌ **待确认** | 静态包无法观测，必须真机/运行时取证 | 明确写「不可静态获取」+ 验证方法 |

**参数符号**：`d`=时长(s)、`delay`=延迟(s)、`ease`=缓动、`Δ`=位移(pt)、`s`=缩放、`r`=旋转(°)、`α`=不透明度、`n`=循环次数、`seq`=过渡顺序、`fb`=反馈节奏。

### 0.1 决定全篇可信度的三条硬约束 ✅

| 约束 | 证据 | 后果 |
| --- | --- | --- |
| 主二进制 **FairPlay 加密** | `LC_ENCRYPTION_INFO_64`：`cryptid=1`, `cryptsize=80,199,680` | SwiftUI 的 `withAnimation` / `.spring()` / `duration` 常量**全部不可静态读取** → 所有精确 `d/ease` 天然是 ◐ 或 ❌ |
| **无第三方动效库** | `Frameworks/` 22 项全表：FFmpeg 系列、libass、libplacebo、libdav1d、libdovi、onnxruntime、AMSMB2、lcms2…**无 Lottie / Rive / Pop / Pow / Spine / SDWebImage 动效库** | 动效必为 **SwiftUI + CoreAnimation 原生**，可直接套系统默认参数基线（§4） |
| **无运行时捕获** | 随包无 mp4/gif/apng/lottie/rive 动画资源 | 无法从素材反推帧率；但发现 `rex-layers.json` 逐帧网格定义（§2），是唯一可量化动效 |

> ⚠️ 另：`KSPlayer` 的 49 个 `*.glsl` + `default.metallib` 是**视频超分着色器**（Anime4K），属渲染管线，**不是 UI 动效**，复刻 UI 时不要误纳入。

---

## 1. 全局动效治理（最容易漏、但直接决定复刻一致性的隐藏参数）✅

| # | 开关 / 状态 | 证据键 | 确定内容 | 对复刻的要求 |
| --- | --- | --- | --- | --- |
| G1 | **减弱动画效果** | `settings.interface.page.reduceMotion` = Reduce Animation Effects / 减弱动画效果 | App 内有显式开关 | **每个动效都必须有降级分支**：位移/缩放/旋转归零，仅保留 α 淡入或瞬时切换；另需响应系统 `UIAccessibility.isReduceMotionEnabled` |
| G2 | **Haptics 三态 + 低电量降级** | `settings.interface.page.haptics.disabled`=Disable；`settings.interface.page.haptics.footer`=「自动模式将在低电量时自动关闭」 | 存在 自动 / 禁用（及启用）三态；**低电量自动关触觉** | 反馈节奏（`fb`）必须与触觉解耦：触觉关闭时视觉反馈不能消失；需监听电量/低功耗模式 |
| G3 | **起播加载动画开关 + 明确回退** | `settings.playback.preference.startupAnimation` = Startup Animation；`.detail` = 「视频加载时播放小恐龙动画，**关闭后显示加载圈**」 | 开→角色动画；关→系统 spinner | 这是**唯一有确定回退态**的动效：必须实现两套加载表现 |
| G4 | **平滑过渡** | `settings.playback.cache.smoothTransition` = Enable smooth transitions；`pro.copy.smooth.transition` = Smooth transition（列为 PRO 卖点） | 存在集间/换集平滑过渡，且与「下一集预加载」耦合 | 关闭时应为硬切；开启时为预加载后的交叉溶解。⚠️ PRO 项是否等同于播放设置项 → ❌ 待确认 |
| G5 | 预加载前置条件 | `settings.playback.cache.smoothTransition.footer` = 「当前播放内容缓冲至结尾时，提前预加载下一集」 | 过渡动画**依赖数据就绪**，不是纯视觉 | 网络慢时过渡会被推迟/取消 → 需有超时回退（❌ 阈值未知） |
| G6 | 动效可被性能预算压制 | `settings.playback.dynamicHDR.footer` = HDR 动态映射与增强层「逐帧处理，播放时更耗电」；`settings.playback.page.upscale.*.detail` 多次标注「需要高端 GPU」 | 高画质/超分开启时 GPU 负载高 | 角色动画/粒子特效需有降级策略，避免帧率塌陷（◐ 依据：GPU 逐帧处理与 UI 动效争抢同一帧预算） |

---

## 2. 唯一可量化的动效素材：`rex-layers.json` ✅

路径：`RexDesign_RexDinoCore.bundle/rex-layers.json`（86,032 B）。
形态：**20 个 64×64 字符网格图层**，字符即调色索引；靠图层组合 + 帧切换实现角色动画，**不是位图序列**。

### 2.1 全量图层规格（确定值）

| 图层 | 网格 | 有效像素(ink) | 字符集 | 归类 |
| --- | --- | --- | --- | --- |
| `restBody` | 64×64 | 2546 | `.bckps` | 静止·主体 |
| `restFins` | 64×64 | 69 | `.p` | 静止·背鳍 |
| `restArm` | 64×64 | 73 | `.ck` | 静止·手臂 |
| `restEye` | 64×64 | 35 | `.k` | 静止·眼 |
| `breathBody` | 64×64 | 2595 | `.bckps` | 呼吸·主体 |
| `breathFins` | 64×64 | 69 | `.p` | 呼吸·背鳍 |
| `breathArm` | 64×64 | 73 | `.ck` | 呼吸·手臂 |
| `breathEye` | 64×64 | 35 | `.k` | 呼吸·眼 |
| `armHappy` | 64×64 | 73 | `.ck` | 姿势·开心手臂 |
| `poseCurious` | 64×64 | 2335 | `.bckps` | 姿势·好奇 |
| `poseConfident` | 64×64 | 2627 | `.bckps` | 姿势·自信 |
| `poseQuiet` | 64×64 | 2117 | `.bckps` | 姿势·安静 |
| `eyeBig` | 64×64 | 60 | `.k` | 表情·大眼 |
| `eyeHappy` | 64×64 | 21 | `.k` | 表情·笑眼 |
| `eyeAngry` | 64×64 | 15 | `.k` | 表情·怒眼 |
| `browUp` | 64×64 | 10 | `.k` | 表情·挑眉 |
| `browAngry` | 64×64 | 22 | `.k` | 表情·怒眉 |
| `mouthO` | 64×64 | 48 | `.bk` | 表情·张口 |
| `mouthSmile` | 64×64 | 37 | `.kp` | 表情·微笑 |
| `mouthGrit` | 64×64 | 24 | `.k` | 表情·咬牙 |

**调色字符**（确定存在 6 个）：`.`=透明，`b / c / k / p / s` = 5 个颜色槽。◐ 推断依据：`k` 为主体轮廓黑、`b` 主体色、`c` 高光/腹部、`p` 粉（背鳍/嘴）、`s` 阴影/斑点；**色值不由 JSON 给出**，应由 `RexDesign` 语义色 + 13 种恐龙配色（`settings.dino.colorName.*` / `RexDinoIcon-{color}-*`）在运行时注入，这也解释了「图标只按小龙配色生成」。→ 具体映射 ❌ 待确认。

### 2.2 呼吸循环（唯一可量化的帧间位移）✅

| 图层 | `rest → breath` 变化像素数 | ink 变化 | 说明 |
| --- | --- | --- | --- |
| Body | **172** | 2546 → 2595 | 主体轮廓整体起伏（约 6.8% 像素） |
| Fins | **30** | 69 → 69 | 背鳍位移（43% 像素换位置，ink 不变 → 纯平移） |
| Arm | **28** | 73 → 73 | 手臂纯平移（38% 像素） |
| Eye | **10** | 35 → 35 | 眼部纯平移（29% 像素） |

> ✅ 确定：`rest*` 与 `breath*` 是**两帧交替**（ink 基本不变、Fins/Arm/Eye 完全不变 → 不是形状渐变，是整层位移）。
> ◐ 推断依据：Fins/Arm/Eye 的 ink 完全守恒，说明这些层以整体 `offset` 平移实现；位移幅度约 **1–2 网格单元**（依据：变化像素数 ≈ 该层周长×位移量）。换算到 76pt 显示尺寸（1 单元 = 76/64 = 1.1875 pt）→ **Δy ≈ 1.2–2.4 pt**。
> ◐ 呼吸周期 `d ≈ 2.0–3.0 s`、`ease = easeInOut`、无限循环 `n = ∞`（依据：iOS 吉祥物呼吸动画惯例 + 两帧交替需慢速往返才不闪）。
> ❌ 精确周期、是否叠加 `α` 呼吸、是否随 Reduce Motion 停止 → 待确认。

**方案 P01（可直接复用，◐ 参数基线）**：两帧 `rest*` / `breath*` 交替，`d=1.2s` 单向 / `2.4s` 往返，`ease=easeInOut`，`Δy=2pt`，`n=∞`；Reduce Motion 时固定 `rest*`。SwiftUI 参考：`TimelineView(.periodic(from: .now, by: 1.2))` 切层，或 `.animation(.easeInOut(duration:1.2).repeatForever(autoreverses:true), value: breathing)`。

---

## 3. 按模块 / 场景 / 动效逐项拆解

> 每项格式：`交互链路`（初始态 → 触发 → 中间态 → 结束态）、`参数表`（含等级与依据）、`边界/异常`。
> **所有 `d / delay / ease / s / r / α` 若无 ✅ 标记，一律为 ◐ 推断或 ❌ 待确认** —— 因 FairPlay 加密，见 §0.1。

### M01 · 小恐龙角色系统（`SettingsDino` 221 键 / `RexDinoCore`）

#### M01-S01 待机呼吸循环
- **链路**：`rest* 四层` →(空闲计时)→ `breath* 四层` →(往返)→ `rest*` 无限循环
- **参数**：见 §2.2 / 方案 P01。触发条件=角色可见且无交互；作用对象=Body/Fins/Arm/Eye 四层；`seq`=四层同帧切换（依据：四层均为 rest/breath 成对）；`n=∞` ◐；`fb`=无触觉、纯视觉。

#### M01-S02 点击部位反馈（头 / 肚子 / 尾巴）
- **链路**：`rest*` →(点按指定热区)→ 对应 `pose*` + 表情图层组合 →(短暂停留)→ `rest*`
- ✅ 确定：`settings.dino.interaction_hint` = 「点头、肚子或尾巴，按住撒娇，**连续点五次会生气**」→ **3 个独立热区**、点击可触发播放（`settings.dino.wardrobe.tapEffectDescription`）。
- **参数表**

| 参数 | 值 | 等级 | 依据 |
| --- | --- | --- | --- |
| 触发 | `tap` × 3 热区 | ✅ | 文案 |
| 作用对象 | 命中区域对应的 pose / eye / mouth / brow 图层 | ◐ | 20 个图层中 `poseCurious/Confident/Quiet` + 9 个表情层可组合 |
| `s` | 1.0 → 0.94 → 1.02 → 1.0（按压回弹） | ◐ | iOS 标准按钮 press spring |
| `d` | 0.15 s 压 / 0.25 s 回 | ◐ | SwiftUI `.spring(response:0.25, dampingFraction:0.7)` 默认族 |
| `ease` | spring(0.25, 0.7) | ◐ | 同上 |
| `r` | ±3° 摇摆（头部热区） | ◐ | 角色反馈惯例，无素材证据 |
| `α` | 1.0 恒定 | ◐ | 字符网格层无 α 通道 |
| `n` | 1 次（每次点击） | ✅ | 交互语义 |
| `fb` | 点击即时（0 延迟），结束后回 rest | ◐ | 依据：文案强调「点一下就会播放」 |

#### M01-S03 长按撒娇（Hold to pet）
- **链路**：`rest*` →(长按 ≥ 阈值)→ `eyeHappy` + `mouthSmile` + `armHappy` →(持续)→ 松开恢复 `rest*`（Mac：`settings.dino.mac.hold` = 「点按互动，**按住打开 Rex**」→ 触发页面打开而非表情）
- ✅ 确定：长按存在；Mac 端长按语义不同（打开 Rex 设置页）。
- **参数**：`delay=0.5s` 长按阈值 ◐（依据：`UILongPressGestureRecognizer` 系统默认 0.5 s；项目另有「长按可替换此操作」「长按节目卡片加收藏」，应统一阈值）；表情层 α 0→1 `d=0.2s easeOut` ◐；维持时长 = 按压时长（无上限）✅ 语义；`n=1`；`fb`=触压开始即反馈（G2 触觉可能关闭）。

#### M01-S04 连点 5 次生气 ⭐（**确定数值**）
- **链路**：`rest*` →(连续 5 次快速点击)→ `browAngry` + `eyeAngry` + `mouthGrit` + `poseQuiet` →(冷却)→ `rest*`
- ✅ **确定：`n = 5`**（`settings.dino.interaction_hint` = 「连续点五次会生气」）；✅ 三个表情层名确定。
- **参数**

| 参数 | 值 | 等级 | 依据 |
| --- | --- | --- | --- |
| 触发窗口 | 连续点击间隔 < ~0.4 s | ◐ | 「连续/quick taps」语义 + 常见连击判定窗 |
| `r` | 生气抖动 ±2°，2–3 次 | ◐ | 愤怒反馈惯例；`browAngry` 层存在可佐证 |
| `d` | 抖动 0.1 s × 3；表情停留 1.2 s | ◐ | 短抖动 + 表情停留两段式 |
| `ease` | linear（抖动）/ easeOut（消退） | ◐ | 抖动需匀速往返 |
| `α` | 表情层 0→1→0 | ◐ | 图层叠加 |
| `seq` | 抖动 → 换表情 → 停留 → 淡出 | ◐ | 反馈节奏 |
| 边界 | 第 6 次点击是否重置冷却 / 长按是否与连点冲突 | ❌ | 需真机 |

#### M01-S05 装扮换装（`wardrobe`）
- **链路**：选中道具 → 对应槽位图层替换 → 角色重绘 → 保存
- ✅ 槽位 6 类：`hat` / `outfit` / `neck` / `held` / `body`(颜色) / `fx`；✅ `wardrobe.wardrobeDescription` = 「同一部位只能穿一件」→ **互斥单选**；✅ `itemNotOwned` / `updateRequired` / `owned` 状态。
- **参数**：换装为**瞬时重绘**（◐ 依据：字符网格是状态表，无中间帧；若要动效应加 `α` 交叉淡化 0.2 s）；`d=0.2s` `ease=easeOut` ◐；`n=1`。

#### M01-S06 点击特效（Tap Effects）⭐
- ✅ `settings.dino.wardrobe.slot.fx` = 点击特效；✅ `tapEffectDescription` = 「兑换后点一下小恐龙就会播放；**可同时启用多个，它们轮流播放**」→ **轮询队列，不是并发动画**。
- ✅ 确定特效项：`fx_rocket`（火箭升空）、`fx_mech`（机甲变身）、`fx_snow`（接雪花）、以及 `fx_*` 系列（221 键中另有若干）。
- **链路**：`rest*` →(点击)→ 从启用队列取下一个特效 → 播放该特效的图层序列 → `rest*`
- **参数表**

| 参数 | 值 | 等级 | 依据 |
| --- | --- | --- | --- |
| `seq` | 队列轮询（每次点击取下一个），非并发 | ✅ | 文案「轮流播放」 |
| `Δ` | 火箭升空：Δy = -64 网格单元（整屏出场） | ◐ | 「升空」语义 + 64 网格高度 |
| `s` | 机甲变身：分段缩放 1.0→1.08→1.0 | ◐ | 变身类过渡惯例 |
| `r` | 接雪花：±5° 轻摆 | ◐ | 接物动作 |
| `d` | 单特效 1.0–2.0 s | ◐ | 短角色特效时长族 |
| `α` | 雪/粒子 0→1→0 | ◐ | 粒子生命周期 |
| `n` | 1 次/点击；`fx_snow` 粒子循环 ❌ | ◐ | — |
| 边界 | 全部特效关闭 → 退回 S02 基础反馈；Reduce Motion → 特效跳过 | ◐ | G1 |

#### M01-S07 孵蛋流程（3 蛋状态机）⭐
- **链路**：`Hatch your dinosaur` 入口 →(点击打开)→ 3 颗蛋 →(逐颗点击)→ `Tap again to help it hatch` 破壳 → 生成候选恐龙 `Dinosaur %lld` → **三只全部孵出后** → `Choose your favorite` → `Hi! Rex!` 确认 → `Your Rex`
- ✅ **确定数值/约束**（`settings.dino.hatch.*`）：

| 项 | 值 | 证据键 |
| --- | --- | --- |
| 蛋数 / 机会数 | **3** | `hatch.rules`、`hatch.remaining`（`%lld of 3 hatches remaining`）、`hatch.egg`（`Egg %lld`）、`hatch.candidate`（`Dinosaur %lld`） |
| 候选数 | 3（全部孵出后才能选） | `hatch.fixed`、`hatch.incomplete`、`hatch.no_chances` |
| 重置次数 | **1 次**（且需先选定恐龙） | `hatch.reset`、`hatch.reset_used`、`hatch.reset_unavailable` |
| 重置入口 | **长按打开菜单** | `hatch.reset_hint` |
| 破壳点击次数 | ≥2（首次「点一颗蛋试试」→「再点一下帮它破壳」） | `hatch.tap` → `hatch.crack` |
| 前置条件 | 需有效 Rex 授权 | `hatch.unauthorized` |
| 道具 | 300 金币「重新孵蛋」，清空当前恐龙与候选 | `points.rehatchProduct`、`points.confirmRehatch` |

- **动效参数**：蛋裂纹 → 破壳为**离散状态切换**（◐ 依据：无蛋的位图帧序列，`rex-layers.json` 只含孵出后的恐龙层）；`s=1.0→1.06→1.0` 蛋摇晃、`r=±6°`、`d=0.12s ×3` ◐（依据：常见「快破壳」抖动）；破壳 `d=0.4s` + 恐龙 `α:0→1` + `s:0.6→1.0` spring ◐；`seq`=抖 → 裂 → 上下半壳分离 → 恐龙弹入 → 候选卡就位。
- **边界/异常**（✅ 全部有确定文案）：`hatch.save_failed`（记录保存失败重试）、`hatch.already_chosen`（已选定，不可再孵）、`hatch.reset_used`（重置已用）、`hatch.unavailable`（「暂时无法显示小恐龙」→ 角色资源加载失败的空态）、`points.chooseFirst`（未选恐龙不能兑换）、`points.shortfall`（金币不足）。

#### M01-S08 起播加载动画（含确定回退态）⭐
- **链路**：进入播放 → 缓冲中 → **开**：恐龙角色动画（`rest/breath` + 可能 `pose*`）→ 缓冲完成 → 淡出 → 画面；**关**：系统加载圈 → 完成 → 淡出
- ✅ 确定：开关与回退（G3，证据 `settings.playback.preference.startupAnimation.detail`）；✅ `settings.dino.points.startup` = 「播放前使用 Rex 动画等待」是另一独立开关 → **两个开关可能叠加**，❌ 优先级待确认。
- **参数**：角色呼吸用 P01；淡出 `α:1→0`, `d=0.25s`, `ease=easeOut` ◐；加载圈为系统 `UIActivityIndicatorView`（60 fps 旋转，`n=∞`）◐；`delay` 由缓冲状态驱动，无视觉延迟 ✅ 语义。

#### M01-S09 图标生成（关联细节）
- ✅ `settings.dino.points.icon.wide/medium` = 远景 / 中景；`icon.footer` = 「**图标只按小龙配色生成，帽子等装扮不会出现在图标里**」；`icon.halloween` 万圣节限定。→ 图标是**按配色 + 景别合成**，不是静态图；复刻若含图标换肤，需实现同样的合成规则（且排除装扮层）。

---

### M02 · PRO 孵蛋抽奖与庆祝（`Pro.strings` 123 键）

#### M02-S01 → S05 抽奖状态机 ⭐
**完整链路（全部文案确定）**：
```
入口提示「上滑或点一下蛋，开始孵蛋」(entry.hint / Mac: entry.hint.mac=点击)
  → 「挑一颗恐龙蛋」(egg.title) + 剩余次数 (chances.left / egg.subtitle.left)
  → 选蛋 (egg.accessibility = Egg %lld)
  → 「它在动了……点一点帮它出来」(egg.hatching)
  → 「快破壳了！」(egg.almost)
  → 「恭喜你！获得优惠」(congrats) + 折扣 (offer.claimed.percent = 已领取 %d%% 优惠)
  → 「立即领取」(claim) / 「再挑一颗」(egg.title.again)
```
| 参数 | 值 | 等级 | 依据 |
| --- | --- | --- | --- |
| 触发方式 | **上滑 `swipe-up` 或 `tap`**；Mac 仅 `click` | ✅ | `entry.hint` / `entry.hint.mac` |
| 作用对象 | 蛋（可多颗，`Egg %lld`）、券卡、庆祝层 | ✅ | 文案 |
| 状态数 | ≥5 段（提示→挑→动→快破壳→恭喜） | ✅ | 文案序列 |
| 最大折扣 | **5 折**（最高半价） | ✅ | `egg.subtitle`「最高5折优惠」 |
| 机会数 | `%lld` 动态，含「最后一颗」态 | ✅ | `chances.left` / `egg.title.last` / `egg.subtitle.last` |
| `r` | 蛋左右摆 ±8°，3–4 次 | ◐ | 「它在动了」需摇摆表现 |
| `s` | 1.0 → 1.05（呼吸）→ 1.12（破壳前）→ 1.0 | ◐ | 分段蓄力 → 释放节奏 |
| `Δ` | 上滑手势 Δy = -80~120 pt 触发 | ◐ | iOS 上滑展示 sheet 的阈值族 |
| `d` | 摆动 0.15 s ×3 → 蓄力 0.4 s → 破壳 0.5 s → 券卡弹入 0.45 s | ◐ | 递增节奏（先慢后快），见方案 P05 |
| `ease` | 摆动 easeInOut / 破壳 spring(0.5,0.6) / 券卡 spring(0.45,0.75) | ◐ | 系统 spring 默认族 |
| `α` | 背景遮罩 0→0.4；券卡 0→1 | ◐ | 弹层惯例 |
| `seq` | 提示淡出 → 蛋集合高亮 → 单蛋放大 → 摆动 → 裂 → 粒子/彩带 → 券卡弹入 → 按钮渐显 | ◐ | 文案顺序 |
| `fb` | 每次点击即时触觉 + 破壳时强反馈；G2 可能关闭 | ◐ | — |
| `n` | 每颗蛋 1 次动画；彩带粒子 ❌ | ◐ | — |

**边界 / 异常（✅ 全有确定文案，复刻必须实现）**：
| 边界 | 文案键 | 视觉/状态要求 |
| --- | --- | --- |
| 关闭页面 = 放弃优惠 | `lottery.close.gives.up` | 需二次确认或明确警示，不能静默关闭 |
| 再抽一次 → 当前券作废 | `lottery.another.gives.up` / `lottery.saved.hint` | 券卡需作废/替换动画（◐ 建议：旧券 α→0 并缩小，新券弹入） |
| 优惠仅存本机 | `lottery.saved.hint` | 换设备丢失 → 需状态提示 |
| 限时 / 终身购买 | `lottery.limited` / `lottery.lifetime` | 倒计时（❌ 具体时长来自服务端） |
| 抽券历史 | `lottery.history.item` = `Egg %1$lld: %2$@` | 列表记录 |
| 重置抽奖（调试） | `mac.settings.copy.reset.egg.draw` / `restored.3.attempts...` | 明确 **3 次机会** 可恢复（✅ 确定数值） |

#### M02-S06 购买成功庆祝页
- **链路**：支付成功 → 全屏庆祝层 → 「欢迎成为 / Welcome to」+「全部功能已解锁」+ 有效期/永久授权 → 「好的」关闭
- ✅ 文案确定：`pro.celebration.title/subtitle/until/lifetime/done`。
- **参数**：`s=0.8→1.0` spring、`α=0→1` `d=0.35s` ◐；彩带/粒子 `n=1` 批、`d=1.5–2.5s` ◐（依据：同类购买庆祝页惯例；包内无粒子素材 → 应为 SwiftUI `Canvas`/`TimelineView` 程序化绘制，不是图片序列 ✅ 推断依据：无 gif/lottie 资源）；`seq`=背景淡入 → 标题弹入 → 副标题延迟 0.15 s → 功能清单逐条 0.06 s stagger → 按钮最后出现（◐）。
- **边界**：`pro.alert.pending.*`（等待批准）、`pro.alert.failed.*`（未能完成）、`pro.purchase.email.failed/conflict/unverified`、`pro.purchase.activation.unavailable`（购买完成但授权服务不可达，稍后自动重试）→ **庆祝页只在授权确认后触发**，不能支付成功即弹（易漏的关联细节）。

---

### M03 · 播放器（`SettingsPlayback` 197 键 / `MacPlayer`）

#### M03-S01 起播加载动画 → 见 M01-S08（含确定回退 spinner）

#### M03-S02 双击手势（分区热区）⭐
- ✅ `settings.playback.gestures.sideDoubleTap`=双击两侧触发、`middleDoubleTap`=双击中间触发、`doubleTap.seek`=快进/快退、`doubleTap.toggle`=播放/暂停 → **画面至少 3 个热区（左 / 中 / 右）**，各可配置动作。
- **链路**：`播放中` →(双击左/右侧)→ 快退/快进 `forwardDuration` →(进度条跳变 + 预览帧)→ 继续播放；`(双击中间)` → 播放/暂停图标闪现 → 状态切换
- **参数**：双击识别窗 `0.3 s` ◐（依据：`UITapGestureRecognizer numberOfTapsRequired=2` 系统容差）；图标闪现 `α:0→1→0`, `d=0.5s`, `delay=0` ◐；进度跳变 `d=0.2s easeOut` ◐；`fb`=即时 + 触觉（G2）。
- **边界**：`settings.playback.buttons.precisionWarning` = 未开启精准 seek 时「可能无法达到快进快退精度」→ 动画目标位置可能与实际落点不一致；`settings.playback.thumbnail.enabled` = 拖动时预览画面（开/关影响 S06）。

#### M03-S03 长按快进 + 调速 + 上滑锁定 ⭐
**链路（全部确定）**：
```
播放中
 →(长按)→ 进入倍速 (longPressDefault 可配置)
 →(左右滑动)→ 调整速度 (longPressPan)
 →(上滑)→ 锁定快进 (longPressLock / player.interaction.swipe-up-to-lock)
 →(松手)→ 恢复至「正常速度」或「长按前速度」(longPressEnd.normal / .current)
```
| 参数 | 值 | 等级 | 依据 |
| --- | --- | --- | --- |
| 触发 | `longPress` ≥ 0.5 s | ◐ | 系统长按阈值，与 M01-S03 统一 |
| 作用对象 | 倍速指示标签、画面、（可选）速度条 | ✅ | 文案 |
| `α` | 倍速标签 0→1，松手 1→0 | ◐ | 状态提示 |
| `s` | 标签 0.9→1.0 | ◐ | spring 弹入 |
| `d` | 进入 0.2 s / 退出 0.15 s | ◐ | 短状态提示 |
| `Δ` | 上滑锁定阈值 Δy ≈ -60 pt | ◐ | 手势阈值族 |
| `seq` | 长按→标签淡入→(可选左右滑调速)→(可选上滑锁定)→松手恢复 | ✅ | 文案顺序 |
| `fb` | 长按开始触觉 1 次；锁定成功再 1 次 | ◐ | G2 |
- **边界**：`settings.playback.gestures.locked` = 「**锁定时允许长按和双击操作**」→ 锁定态不是全禁，必须保留两个手势通道（易漏）；`player.interaction.swipe-up-to-lock-compact` = 紧凑标签文案（窄屏布局变体）。

#### M03-S04 上下滑动调音量 / 亮度 ⭐
- ✅ `settings.playback.gestures.verticalPan` = 「全屏上下滑动调整音量和亮度」→ 左右半屏分属两个通道（◐ 依据：播放器通行设计；文案未指定分区 → ❌ 需真机确认哪侧是音量）。
- **参数**：`Δy` 映射为 0–100% 线性（◐）；HUD `α:0→1` `d=0.15s`，停止滑动后 `delay=1.0s` 再淡出 `d=0.3s` ◐；`n=1`。
- **边界 / 关联（✅ 确定）**：`picture.independentBrightness` = 播放时使用独立亮度，**返回桌面或结束播放后恢复原来亮度**；`picture.rememberBrightness` = 沿用上次亮度；`picture.dolbyBrightness` = 杜比亮度增强 → 亮度通道有 3 种策略且必须在退出时还原系统亮度（动画之外的重要状态恢复）。

#### M03-S05 双指缩放 / 屏幕旋转 ⭐
- ✅ `settings.playback.gestures.pinch` = 双指缩放画面；`picture.followRotation` / `autoLandscapeRotation` = 跟随设备旋转 / 横屏方向自动切换；`picture.rotationButton` = 显示转屏按钮。
- **参数**：`s` 由 pinch scale 驱动，范围 ◐ 推断 `1.0–2.0`（依据：画面填充裁切惯例）；松开是否回弹 ❌；旋转 `r: 0→90/180/270°`, `d=0.3s`, `ease=easeInOut` ◐（依据：iOS 设备旋转动画系统默认 ~0.3 s）。
- **边界**：`autoLandscapeRotation.detail` = 即使关闭「跟随设备旋转」，仍可在两个横屏方向间自动切换 → **两个开关不是父子关系**；外接显示器有独立 Scene（`Player External Display`）→ 旋转/缩放可能只作用于主屏（❌ 待确认）。

#### M03-S06 拖动进度预览 / 换集过渡
- ✅ `settings.playback.thumbnail.enabled` = 拖动时预览画面；`seek.accurate`；`section.transition` = 连播预加载；`pro.copy.smooth.transition` / `settings.playback.cache.smoothTransition`（G4）。
- ✅ `player.episode.transition_failed` = 「暂时无法切换剧集」→ 换集过渡有**明确失败态**。
- **参数**：缩略图 `α` 随拖动 0→1 / 松手 1→0，`d=0.15s` ◐；换集交叉溶解 `d=0.3–0.5s`, `ease=easeInOut` ◐；`seq`=旧画面淡出与新画面淡入重叠（依据：「平滑过渡」需交叉而非串行）；`delay` 由预加载完成度驱动（G5）。
- **边界**：预加载未就绪 → 应硬切或保持加载态；`player.error.resources_exhausted` / `playback_failed_message` 为过渡失败后的错误反馈。

#### M03-S07 锁定态手势矩阵（汇总，✅ 结构确定）
| 手势 | 正常态 | 锁定态 | 证据 |
| --- | --- | --- | --- |
| 长按快进 | ✅ | ✅ | `gestures.locked` |
| 双击（侧/中） | ✅ | ✅ | `gestures.locked` |
| 上下滑动 | ✅ | ❌ | 「锁定」语义 ◐ |
| 双指缩放 | ✅ | ❌ | 同上 ◐ |
| 左右滑调速 / 上滑锁定 | ✅ | ❌ | 同上 ◐ |

---

### M04 · 首页 / 编辑器（`Home` / `HomeEditor` / `AppHome`）

#### M04-S01 精选轮播（Featured Carousel）
- ✅ `home.accessibility.featured.carousel` = 精选轮播；`homeEditor.copy.enable.carousel` = 开启轮播图；`carousel.data` / `carousel.content` 可配置数据源；`AppHome.copy.keep.only.the.home.carousel...` = 「仅保留首页轮播」→ **轮播可独立存在**。
- **链路**：自动轮播 →(用户横向 swipe)→ 打断自动计时 → 吸附到页 → 恢复自动计时
- **参数**：`Δx` = 容器宽（整页平移），`ease=spring(0.5,0.85)` 或 `easeInOut` ◐（依据：`TabView(.page)` / `ScrollView + .scrollTargetBehavior(.paging)` 系统族）；自动轮播间隔 `d≈4–6 s` ◐；`n=∞` ◐；`seq`=当前页淡出 + 下一页淡入（或纯平移）❌；分页点 α 随选中态变化，`d=0.2s` ◐。
- **边界**：`home.error.network.*` 四类（timeout / lost / offline / secure / server）→ 轮播加载失败应显示占位而非无限转圈；Reduce Motion 时停自动轮播（G1，◐）。

#### M04-S02 拖拽排序（跨 5 个模块复用）⭐
- ✅ 出现位置：`home.section.drag.to.sort`、`appHome.copy.tap.to.switch.configurations.enter.edit.mode.to.reorder.them`、`mac.homeConfigurations.footer`（「点按切换配置；**拖动可调整顺序**，右键可删除」）、`library.groups.editor.footer`、`settings.aggregated.sources.skip.footer`、`settings.playback.skip.sources.footer`、`settings.navigation.footer`。
- **链路**：`正常态` →(进入编辑模式 / 长按激活)→ `被拖项抬起` →(拖动)→ `其余项让位` →(松手)→ `落位并保存`
- **参数**

| 参数 | 值 | 等级 | 依据 |
| --- | --- | --- | --- |
| 激活 | 长按 ≥0.5 s 或显式「编辑模式」 | ◐/✅ | 系统阈值；AppHome 明确有编辑模式 |
| `s` | 抬起项 1.0→1.03 | ◐ | iOS 拖拽默认抬升 |
| `α` | 抬起项 0.95；占位区 0.3 | ◐ | 拖拽反馈惯例 |
| 阴影 | `hairline` 色 + 抬升（◐ 8 pt） | ◐ | 系统拖拽阴影；色值见 UI 文档 §3.1 |
| `d` | 让位 0.25 s / 落位 0.3 s | ◐ | `.spring(response:0.3, dampingFraction:0.8)` |
| `ease` | spring | ◐ | 同上 |
| `seq` | 抬起 → 让位（相邻项依次）→ 落位 → 触觉 1 次 | ◐ | — |
| `fb` | 越过边界时触觉 1 次；松手 1 次 | ◐ | G2 可能关闭 |
| `n` | 1 次/拖动 | ✅ | — |

- **边界 / 异常（✅ 确定）**：`appHome.copy.unable.to.reorder` = 无法调整顺序（拖拽失败态）；`home.copy.move.up/down/to.top/to.bottom` = **提供无拖拽的替代操作**（无障碍/Reduce Motion 友好）；`home.section.hidden` = 隐藏分区不参与排序；删除分组需二次确认。
- **关联细节**：`home.copy.home.settings.are.temporarily.unavailable`、`home.copy.the.home.layout.has.changed.review.it.again.before.restoring` = 首页配置恢复时若布局已变，需先复核 → 恢复动画前应插入确认态（易漏）。

#### M04-S03 分区展开 / 收起 / 隐藏
- ✅ `home.section.show` / `.hide` / `.hidden`、`watching.copy.tap.to.expand`、`watching.format.collapse.records`（折叠 %lld 条）、`library.section.expand/collapse`、`mac.sidebar.expand/collapse`。
- **参数**：高度 0↔内容高，`d=0.3s`, `ease=easeInOut` ◐；子项 `α` stagger `delay=0.02s × index` ◐；`seq`=容器高度先动、子项后淡入（避免挤压）；`n=1`。
- **边界**：折叠数量需在收起态显示（`%lld Records Collapsed`）；隐藏分区不渲染；Reduce Motion 直接切换。

#### M04-S04 加载 / 刷新 / 骨架状态
- ✅ 文案族：`home.copy.loading.more`、`home.copy.widget.building.in.progress`、`home.copy.refresh`、`appWatching.copy.refreshing`、`homeEditor.copy.loading...`、`library.drivemby.progress.waiting/recognizing/uploading/done`、`settings.icloud.progress.title`（Syncing）、`offline.copy.downloading`、`settings.feedback.page.video.compressing` 等。
- **参数**：spinner `r` 连续、`n=∞`（系统）；进度条宽度按百分比 `d=0.2s linear` ◐；骨架若存在应 `α` 呼吸 0.3↔0.6 `d=1.2s` `n=∞` ◐（依据：通用骨架屏；**包内无骨架素材**，是否存在 ❌）。
- **异常反馈节奏（✅ 差异明确）**：`home.error.network.timeout / lost / offline / secure / server`、`home.error.loading.code`（带错误码）、`home.error.loading.retry`、`home.copy.loading.was.interrupted.please.retry`、`home.copy.home.loading.returned.no.result.please.retry` → **5 类网络错误 + 3 类加载状态各有独立文案**，不能合并成一个「加载失败」；每类都应附「重试」按钮与 0 延迟的错误态淡入 `d=0.2s` ◐。

---

### M05 · 导航与转场

| # | 场景 | 链路 | 参数（等级 / 依据） | 边界 |
| --- | --- | --- | --- | --- |
| M05-S01 | Tab 切换（4 Tab） | 当前页 → 目标页 | `α`/`Δx` ❌；◐ 系统 `TabView` 默认无交叉动效（仅状态切换） | Reduce Motion 不影响（G1） |
| M05-S02 | Sheet / 表单呈现 | 列表 → 底部 sheet 升起 | `Δy = 0 → -sheetHeight`, `d=0.35s`, `ease=spring(0.35,0.85)` ◐（依据：iOS sheet 系统默认）；遮罩 `α:0→0.4` 同步 | `library.drivemby.sheet.*`、密码输入（6 位 PIN ✅ `environment.pin.placeholder`） |
| M05-S03 | 侧栏 / 副 Tab 折叠 | 展开 → 图标态 | 宽度变化 `d=0.25s easeInOut` ◐；副 Tab 折叠后显示 `%lld` 序号图标 ✅ `shell.format.secondary.tab.collapse.icon` | iPad/Mac 才有；iPhone 无此态 |
| M05-S04 | 横向 rail 翻页 | 点击 next/previous → 横向滚动 | `Δx` = 一屏宽，`ease=spring` ◐；`mac.rail.scrollHint` 滚动提示 | 到头/到尾需禁用按钮（◐） |
| M05-S05 | 双栏 Split | 左选择 → 右详情 | `settings.split.placeholder` = 「选择左侧的一项设置」→ 空态右栏 ✅ | iPad/Mac |

### M06 · 列表 / 卡片 / 详情长按体系 ⭐

| # | 场景 | 交互链路 | 确定值 | 参数（◐/❌） | 边界 |
| --- | --- | --- | --- | --- | --- |
| M06-S01 | 长按节目卡片 → 收藏 / 待看 | 卡片 →(长按 0.5s)→ 上下文菜单 → 选择 → 状态徽标变化 | `mediaMarks.copy.long.press.the.program.card.to.add.to.favorites` / `...to.the` ✅ 两个动作 | 菜单 `s:0.94→1.0`, `α:0→1`, `d=0.2s`, `ease=spring(0.25,0.8)` ◐；卡片 `s=1.02` 抬起 ◐ | `mediaMarks.copy.removed.from.watchlist` 撤销态；独立首页收藏为空时的空态 ✅ |
| M06-S02 | 长按替换详情页主操作 | 主按钮 →(长按)→ 动作替换菜单 → 选新动作 → 按钮图标/文案切换 | `details.copy.touch.and.hold.to.replace.this.action` ✅ | 图标交叉淡化 `d=0.2s` ◐；触觉 1 次 ◐ | 与 M06-S01 用同一长按阈值（统一 0.5 s ◐） |
| M06-S03 | 到底部长按查看完整日历 | 列表滚动到底 →(长按)→ 完整日历展开 | `appWatching.copy.you.ve.reached.the.bottom.long.press.to.view.the.full.calendar` ✅ **触发前必须"已到底部"** | 日历 `Δy` 上推 / `α:0→1`, `d=0.4s` ◐ | 未到底部长按不应触发（边界条件明确） |
| M06-S04 | 折叠 / 展开 N 条记录 | 点击 → 记录收起并显示计数 | `watching.format.collapse.records` / `records.collapsed` / `tap.to.expand` / `expand|collapse.unidentified.records` ✅ | 高度 `d=0.3s easeInOut` + 子项 stagger 0.02 s ◐ | 计数必须实时更新 |
| M06-S05 | 左滑管理模块源 | 行左滑 → 露出操作 | `settings.modules.source.unreachable` = 「模块源无法访问，**左滑进行管理**」✅ | `Δx = -80pt`, `d=0.25s` ◐ | 仅不可达行出现提示（条件态） |

### M07 · Onboarding（132 键）

| # | 场景 | 链路 | 参数 | 依据 / 边界 |
| --- | --- | --- | --- | --- |
| M07-S01 | 4 页分页转场 | `Get Started` → 页 1→2→3→4（`Previous`/`Continue`）→ `Preparing Home` → 首页 | 整页 `Δx`, `d=0.4s`, `ease=easeInOut` ◐；内容元素 stagger `delay=0.08s` ◐ | 4 页文案确定（UI 文档 §6.8）；`onboarding.copy.onboarding.progress` = 引导进度指示 ✅（进度条/圆点，形态 ❌） |
| M07-S02 | 滑动浏览 | `onboarding.copy.arrange.all.home.modules.freely.or.simply.swipe.to.browse` | `ease=spring(0.5,0.85)` 分页吸附 ◐ | 与按钮 `Continue` 双通道触发同一转场（关联细节） |
| M07-S03 | 加载/准备态 | `Preparing Home` / `New Content Is Available` / `Updated` | spinner `n=∞` ◐ | 进入首页前的过渡态 ✅ 文案存在 |

### M08 · 状态反馈 / 异常 / 边界（全模块横切）⭐

| 类别 | 确定文案（节选） | 视觉/节奏要求 | 等级 |
| --- | --- | --- | --- |
| 进度类 | `Syncing` / `Uploading the file list…` / `%1$d videos reported · %2$d / %3$d recognized` / `Downloaded %@ / %@` / `Saving %lld/%lld` / `Compressing video` | 数值实时更新，进度条 `d=0.2s linear`；**不能用无限 spinner 代替确定进度** | ✅ 文案 / ◐ 参数 |
| 后台完成通知 | `details.copy.watch.progress.will.continue.saving.in.the.background...完成后通知` / `shell.copy.watch.progress.finished.saving.in.the.background` | 页内 toast/横幅（`α:0→1→0`, `delay=2s`, `d=0.25s` ◐）+ 系统通知 | ✅ |
| 失败但保留 | `history.delete.failed` = 部分记录未删除已保留；`appWatching.copy.watch.saved.*.sync.failed` = 本地已保存、远端未同步；`settings.trakt.stats.cached` = 刷新失败显示上次统计 | **不要回滚整个 UI**，保留旧数据 + 局部错误标记 | ✅ |
| 自动重试停止 | `appPlayback.error.http.access.denied` = 「已停止自动重试」 | 重试状态有明确终态，动画需停止 | ✅ |
| 额度 / 门槛 | `audit.playback.daily.limit` = 每日 **3 次**免费播放；`paid.playback.reason.*`（4K / 120fps / DoVi P5/P8 / HDR10 / 非 MP4 / 未知格式 / 模块视频 / 信息不全）；`paid.library.limit` 免费 **1 个**资源库；`paid.environment.limit` 免费 **1 个**观影环境 | 播放前拦截 → PRO 引导页 | ✅ 数值确定 |
| 授权 / 密码 | `environment.pin.placeholder` = **6 位数字**、二次输入；`home.sharing.password.*`；`settings.dino.hatch.unauthorized` | 错误输入抖动 `Δx=±6pt ×3, d=0.1s` ◐；路径长度 3–20 字符 ✅ | ✅/◐ |
| 容量边界 | `settings.feedback.page.video.tooLarge` = 压缩后仍 > **200 MB**；`home.sharing.path.minimumLength=3` / `maximumLength=20` | 输入校验即时反馈 | ✅ |
| 平台差异 | `pro.lottery.entry.hint` (iOS 上滑/点击) vs `.mac` (仅点击)；`settings.dino.mac.hold` (Mac 长按打开 Rex) | **同一动效在 Mac 上触发通道不同** | ✅ |
| 数据隔离 | 切换观影环境 → Discover 页 / 播放进度 / 追剧日历全变；`environment.copy.this.cannot.be.undone...` | 切换需整页内容刷新（避免残留旧环境卡片） | ✅ |
| RTL | `ar.lproj` 存在 | 位移/旋转方向需镜像（`Δx` 取反、`r` 取反） | ◐ 依据：阿拉伯语本地化 |
| iPad 方向 | 四方向全支持 | 旋转动画需覆盖 180°/270° | ✅ |

---

## 4. 可直接复用的现成动效方案 / 参数组合（◐ 推断基线）

> 全部为 **◐ 推断值**，依据统一为：① 原生 SwiftUI/CoreAnimation 系统默认族（§0.1 已确认无第三方库）；② §2 可量化素材；③ 文案规定的状态顺序。**上线前按 §6 校准。**

| 编号 | 方案名 | 参数组合 | 适用动效 |
| --- | --- | --- | --- |
| **P01** | 双帧呼吸 | 两图层交替，`d=1.2s` 单向 / `2.4s` 往返，`ease=easeInOut`，`Δy=2pt`，`n=∞` | M01-S01、起播角色动画 |
| **P02** | 标准按压 | `s:1→0.94→1`, `d=0.15s`, `ease=spring(0.25,0.7)`, `α=1`, `n=1`, `fb`=即时 | 所有 tap 反馈 |
| **P03** | 弹入呈现 | `s:0.8→1.0`, `α:0→1`, `d=0.35s`, `ease=spring(0.35,0.8)`, `delay=0` | 券卡、庆祝页、上下文菜单 |
| **P04** | Sheet 升起 | `Δy: +H→0`, `d=0.35s`, `ease=spring(0.35,0.85)`, 遮罩 `α:0→0.4` 同步 | 表单、PRO、设置 sheet |
| **P05** | 蓄力—破壳（**递增节奏**） | `seq`: 摆(±6°,0.12s×3) → 蓄力(s→1.12,0.4s) → 破壳(spring 0.5/0.6,0.5s) → 结果弹入(P03,0.45s)；`fb` 每阶段 1 次触觉 | M01-S07、M02 抽奖 |
| **P06** | 抖动 / 错误 | `Δx:±6pt ×3`, `d=0.1s`, `ease=linear`, `n=1` | 密码错误、连点生气、拖拽失败 |
| **P07** | 拖拽抬升 | `s=1.03`, `α=0.95`, 阴影抬升 8pt，让位 `0.25s spring(0.3,0.8)`，落位 0.3s | M04-S02 及全部排序场景 |
| **P08** | 展开 / 收起 | 高度 `0↔H`, `d=0.3s`, `ease=easeInOut`, 子项 stagger `delay=0.02s × i` | 分区、折叠记录、侧栏 |
| **P09** | 整页分页 | `Δx=容器宽`, `d=0.4s`, `ease=spring(0.5,0.85)`, 自动轮播间隔 5s, `n=∞` | Onboarding、精选轮播、rail |
| **P10** | 交叉溶解 | 旧 `α:1→0` / 新 `α:0→1` 重叠, `d=0.35s`, `ease=easeInOut` | 换集平滑过渡、图标换装 |
| **P11** | Toast / 状态提示 | `α:0→1` 0.2s → `delay=1.0~2.0s` → `α→0` 0.25s | 倍速标签、亮度 HUD、后台完成 |
| **P12** | Stagger 清单揭示 | 元素依次 `delay=0.06s × i`, 单项 `α:0→1` + `Δy:8→0`, `d=0.3s` | 庆祝页功能清单、Onboarding 内容 |

**平台无关映射**（原栈 SwiftUI；Kivo 若用 QML/CSS 可直接换算）：
- `spring(response, damping)` ≈ `cubic-bezier(0.2, 0.8, 0.3, 1.0)`，`d ≈ response × 1.2`
- `easeInOut` ≈ `cubic-bezier(0.4, 0, 0.2, 1)`
- 位移单位为 **pt**（393 pt 设计基准，见 UI 文档 §3.2）；Web 按 `1pt = 1px @1x` 换算

**Reduce Motion 降级矩阵（G1，必须实现）**：P01 固定静帧、P05 跳过摆动直接结果、P06 改为 α 闪烁、P07 取消抬升、P09 直接切换、P10 改 `d=0.1s` 纯淡入、P12 取消 stagger；触觉按 G2 独立降级。

---

## 5. 主动发现的隐藏参数 / 关联细节（用户未点名但影响复刻）

1. **两个「角色动画」开关叠加**：`settings.playback.preference.startupAnimation` 与 `settings.dino.points.startup` 都控制起播动画 → 优先级/互斥 ❌ 待确认。
2. **「平滑过渡」出现两次**：播放设置项 + PRO 功能卖点 → 是否同一能力 ❌ 待确认。
3. **锁定态不是全禁**（M03-S07）：锁定时仍允许长按与双击 → 手势矩阵必须按表实现。
4. **亮度必须还原系统值**（M03-S04）：退出播放/回桌面时恢复，独立亮度与记忆亮度是两条策略。
5. **多特效是轮询队列，不是并发**（M01-S06）。
6. **图标合成排除装扮层**（M01-S09），且按景别 wide/medium 生成。
7. **过渡依赖数据就绪**（G5）：预加载未完成时动画会被推迟，不能假设固定延迟。
8. **庆祝页只在授权确认后触发**（M02-S06），购买成功≠庆祝。
9. **关闭抽奖页即放弃优惠**（M02 边界），再抽会作废旧券 → 需要作废动画与二次确认。
10. **首页布局恢复需复核**（M04-S02 边界），不能直接播放恢复动画。
11. **拖拽有非拖拽替代路径**（上移/下移/置顶/置底）→ Reduce Motion / 无障碍下走替代路径。
12. **5 类网络错误 + 3 类加载状态文案各自独立**（M04-S04），错误态不可合并。
13. **「已到底部」是长按的前置条件**（M06-S03），不是任意位置长按。
14. **Mac 与 iOS 触发通道不同**（M08 平台差异），上滑在 Mac 不存在。
15. **RTL 镜像**（ar.lproj）会反转所有 Δx 与旋转方向。
16. **Dynamic Type / 性能预算**：系统字体走 Dynamic Type；HDR/超分逐帧处理会压缩 UI 动效帧预算（G6）。
17. **外接显示器独立 Scene**：播放器动效是否同步到外屏 ❌ 待确认。
18. **15 语言文案长度差异**：stagger/弹性布局不能按英文文案长度写死（ar/zh 长度差异大）。

---

## 6. 待确认清单与「转为确定」的验证方法

| # | 待确认项 | 影响 | 验证方法 |
| --- | --- | --- | --- |
| 1 | 全部 `d / delay / ease / spring` 精确值 | 节奏一致性 | 真机 120/240 fps 录屏逐帧标注（QuickTime + ffmpeg 抽帧），量测起止帧差 |
| 2 | 角色图层字符 → 色值映射 | 视觉还原 | 真机取色 + 与 `RexDesign` 语义色 / 13 配色比对 |
| 3 | 呼吸周期、是否叠加 α | 待机观感 | 录屏计时 10 个循环取均值 |
| 4 | 长按阈值、双击窗口、连点 5 次的判定窗 | 手势手感 | 脚本化 `idb`/XCTest 按不同间隔注入事件，记录响应时间 |
| 5 | 轮播自动间隔、吸附曲线 | 首页节奏 | 录屏计时 |
| 6 | 特效 `fx_*` 完整清单与各自序列 | 角色表现 | `SettingsDino` 221 键已导出；运行时遍历装扮页录屏 |
| 7 | Reduce Motion / Haptics 各档实际降级行为 | 无障碍一致性 | 开/关两组对照录屏 |
| 8 | 加密代码内的 padding / 圆角 / 字号 | UI 还原 | 越狱 dump 解密二进制 + `class-dump` / Hopper；或真机截图像素量测 |
| 9 | 两个起播开关 / 两处平滑过渡的关系 | 设置逻辑一致性 | 真机 2×2 开关组合测试 |

> 若取得解密后的 Mach-O 或真机录屏，REA 的 `open_binary` / `search_strings` / `trace_feature` 可继续把 ◐/❌ 升级为 ✅。

---

## 7. 证据索引

| 文件 / Evidence | 内容 |
| --- | --- |
| `~/tools/rex-extract/Payload/Rex.app/Rex` | FairPlay 加密主二进制（`cryptid=1`） |
| `~/tools/rex-extract/Payload/Rex.app/RexDesign_RexDinoCore.bundle/rex-layers.json` | 20 图层 × 64×64 网格动画定义（86 KB） |
| `~/tools/rex-extract/Payload/Rex.app/Frameworks/` | 22 个 framework，无第三方动效库 |
| `~/tools/rex-analysis/strings/en__*.json` / `zh-Hans__*.json` | 39 模块 × 2 语言，共 78 份文案 JSON（全文证据） |
| `~/tools/rex-analysis/assets/*.json` | 6 份 `assetutil -I` 资源目录元数据 |
| `ev_1f79a61a…` / `ev_6be889ca…` / `ev_defa93b5…` | REA 清单、Interface Builder、节点视图 Evidence（见 UI 文档 §0） |
| `Rex_动效参数表.csv` | 本文所有动效项的机器可读参数矩阵 |

---

*本文严格区分 ✅ 确定 / ◐ 推断 / ❌ 待确认。凡标 ◐ 的数值均给出推断依据，凡标 ❌ 的均给出验证路径；不存在「默认当成真」的静默降级。*
