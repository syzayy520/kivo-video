# 播放页 UI v2 · 实施现状与 REX 逐项对照（2026-11-12）

> 本节是**实施后的自查记录**，不是设计稿。
> 基准：REX 1.0.2 真机横屏播放页
> （`C:\Users\Administrator\Pictures\REX\frames2\rot_76.png` 2532×1170 = 844×390pt @3x；
> 交互/动效目录 `docs/reference/rex-teardown/audit/interactions.csv`）。
> 设备：K40 `b6e950ee`，横屏 viewport 2400×1080。

---

## 一、整体判断

| 维度 | 结论 |
| --- | --- |
| **结构** | 与参考**同构**：顶栏一行（✕ + 片名 + 网速 + 8 个动作图标）、底部两行（时间与进度条**同一行** / 按钮行）。 |
| **交互** | 参考的 5 套手势已落地 2 套（双击三区、长按系待做），面板"点空白处关闭"已实现。 |
| **材质** | 参考的毛玻璃弹层我们用「投影 + 垂直渐变底 + 顶部高光边」三层还原（Android 没有 backdrop blur，见 §五）。 |
| **仍缺的** | 见 §四 —— 明确列出，不粉饰。 |

---

## 二、已达成（含真机证据）

### 2.1 结构与视觉

| 项 | 参考 | 我们 | 证据 |
| --- | --- | --- | --- |
| 沉浸式全屏 | 参考播放时也是全屏（录屏露状态栏是为了展示功能） | 状态栏+导航条都隐藏，下滑临时唤出 | `device/06-controls-rex-icons.png` |
| 底栏两行 | 时间与进度条**同一行** + 按钮行 | 同（第一版是上下两行，已改） | 同上 |
| 时间格式 | `00:08` / `45:28`（**两位补零**） | 同 | 同上 |
| 进度条 | 3.5pt 细轨、无圆形滑块 | 同 + **拖动时出现 8dp 圆点**（比参考多一步） | — |
| 顶栏动作图标 | **8 个**，中心距 50pt | **8 个**，中心距 50pt（实测 135/139/136px） | `device/04-topbar-icons-ours-3x.png` |
| 网速 | 右上角，12pt 等宽 | 同（取 mpv `cache-speed`，真实 bytes/s） | — |
| 加载指示 | 中央三点波纹 | 同 | — |

### 2.2 图标（**用户明确指出过不合格，已按参考重画**）

| 我们的 key | 参考形状 | 语义 |
| --- | --- | --- |
| `airplay` | 屏幕 + 实心三角 | 投屏 |
| `danmaku` | 开口圆 + 汉字「弹」 | 弹幕开关 |
| `pip` | 屏幕框 + 斜箭头 + 实心小方块 | 画中画 |
| `clapper` | 斜顶板 + 斜条纹 + 机身 | 选集（剧集） |
| `expand` | 圈内双向箭头 | 画质比例 |
| `equalizer` | 圈内高低波形条 | 均衡器 |
| `captions` | 对话气泡（左下尾巴）+ 两行横线 | 字幕 |
| `headphones` | 圆弧头梁 + 两只耳罩 | 音轨 |

**做法**：参考用 SF Symbols，拆解包里**没有位图资源**（只有 AssetCatalog 的 JSON），
所以按 `rot_76.png` 的像素逐个重画为 Compose Path（24×24 基准画布）。
代码：`PlayerGlyphFeature.kt`（功能入口族）/ `PlayerGlyphCore.kt`（基础族）/ `PlayerGlyph.kt`（分发）。

### 2.3 交互与过渡

| 交互 | 参考 | 我们 | 状态 |
| --- | --- | --- | --- |
| 点画面 | 控制条显隐 | 同 | ✅ |
| **点空白处关面板** | 有 | **有**（用户明确要求） | ✅ |
| 双击左 1/3 | 快退 | 同（±10s） | ✅ |
| 双击右 1/3 | 快进 | 同 | ✅ |
| 双击中 1/3 | 播放/暂停 | 同 | ✅ |
| 双击闪示图标 | 图标闪现 0.5s | 同（居中图标 + 圆底，150ms 淡入淡出） | ✅ |
| 控制条显隐过渡 | 有 | 淡入淡出 + 轻微纵向位移 180ms | ✅ |
| 面板进出过渡 | 有 | 淡入 + 0.96→1 缩放 200ms | ✅ |
| 轻提示 | 淡入→停留→淡出 | 同（1.8s 停留） | ✅ |

### 2.4 功能（**都写进内核，不是 UI 自己记一份**）

| 功能 | 写入 | 内核证据 |
| --- | --- | --- |
| 倍速 | `speed` | `Set property: speed="1.5" -> 1` + `adding scaletempo2` |
| **画质比例** | `video-aspect-override` | `Set property: video-aspect-override="4:3" -> 1` |
| **均衡器** | `af`（`lavfi=[equalizer=…]`） | 面板可开、当前值带勾 |
| 音轨 | `aid` | 面板显示内核真实轨道（en✓/truehd/en/truehd/MANDARIN…/dts） |
| 字幕轨 | `sid` | 同上 |

**新增到与参考持平的功能**：画中画（PiP）、均衡器。

---

## 三、本轮修掉的真 bug（**都是"看起来像 Compose 不听话"的那种**）

### 1. `pointerInput(Unit)` 捕获了**首次组合时的闭包**

**症状**：面板打开后点空白处 → 面板**不关**，反而把控制条收起了。
**根因**：`pointerInput` 的 key 是 `Unit` ⇒ 协程永不重启 ⇒ 闭包里的 `panelOpen` 永远是首次组合时的值。
**修法**：`rememberUpdatedState` 把最新回调带进长生命周期的手势协程。
**教训**：任何长生命周期的手势协程，捕获的回调都要用 `rememberUpdatedState` 刷新。

### 2. `Modifier.align()` 作为参数传给子组件会**静默失效**

`align` 是 BoxScope 专属的 ParentData 修饰符；传进子组件后不在同一个 BoxScope ⇒ 无效（不报错）。
**症状**：底栏没贴屏幕底 → 被裁 → 文字只剩上半 → **连命中区一起被裁** ⇒ 按钮永远点不到。
**修法**：每一层在 Overlay 里自己包一个 Box，align 写在本层的 BoxScope 里。

### 3. `Row` 固定高度小于文字行盒 ⇒ 文字连命中区一起被裁

17sp 文字的行盒是 24sp(=66px)；`Row(Modifier.height(24.dp))` ⇒ 文字被裁到 21px。
**修法**：行高用 `defaultMinSize`（只保底不封顶），并给 `Text` 显式 `lineHeight`。

### 4. 拖动预览浮窗只 `offset(x)` ⇒ 向下溢出 90dp 盖住按钮行

**修法**：`offset(x, y = -(popupHeight + 6.dp))` 向上挂，且放在独立一层不占布局高度。

### 5. 轮询里调用会抛异常的 `native()` ⇒ **Activity 起来又退回桌面**

顶栏网速的轮询在会话建立之前就会跑，`requireSession()` 抛异常 ⇒ 轮询任务崩 ⇒ 整个 Activity 挂掉。
**修法**：`nativeOrNull()` / `tracksOrNull()` / `currentSpeedOrNull()` 三个安全访问器。
**原则**：**"读不到"返回 null，不返回默认值** —— 返回 "1x" 等于对用户撒谎。

---

## 四、**还欠着的**（对照参考，明确列出）

| 参考能力 | 我们的状态 | 打算 |
| --- | --- | --- |
| **长按快进 + 拖动调速 + 上滑锁定** | ❌ 未做 | 参考 M03-S02：longPress≥0.5s → 标签淡入 → 左右滑调速 → 上滑(Δy≈−60pt)锁定 → 松手恢复，2 次触觉 |
| **上下滑音量/亮度 HUD** | ❌ 未做 | 参考 M03-S03：左半屏亮度、右半屏音量，HUD 淡入 0.15/淡出 0.3，停止 1s 后淡出 |
| **双指缩放** | ❌ 未做 | 参考 M03-S04：1.0–2.0 倍 |
| **换集交叉溶解** | ❌ 未做 | 参考 M03-S05：0.3–0.5s easeInOut，与预加载耦合（剧集列表未接入，暂无入口） |
| **拖动预览三件套** | ⚠️ 做了浮窗，缺**左侧相机圆钮**与**左端拖动目标时间** | 参考 §6.2：浮窗左侧 48pt 毛玻璃圆钮（截当前帧为剧照）+ 左端实时显示目标时间 + 拖动中轨道整体变亮 |
| **字幕菜单** | ⚠️ 只有轨道列表 | 参考：内封字幕/外挂字幕/搜索字幕/导入字幕/显示设置/字体设置 |
| **锁定态** | ❌ 未做 | 参考 §6.4：全屏只剩左下角挂锁图标，弹幕/字幕继续渲染 |
| **进度条"已播粗白 + 未播细灰"** | ⚠️ 统一细轨 | 参考是**两段式**（已播粗白条 + 未播细灰线），待按像素复核后改 |

---

## 五、材质：为什么不是真的毛玻璃

Android 12+ 的 `Modifier.graphicsLayer { renderEffect = BlurEffect(...) }` **只能模糊自己这一层的内容**，
做不出"把背后的画面采样后模糊"的 backdrop blur（那需要 `RenderNode` 截屏或 OpenGL FBO）。

⇒ 我们用**三层叠加**把质感做出来（`PlayerGlassSurface.kt`）：
1. **投影**（18dp，黑）—— 面板从画面上"浮"起来，不是贴着；
2. **垂直渐变底**（上 `#F72A2A30` → 下 `#F2101013`）—— 平色块立刻有厚度；
3. **顶部高光边**（上白 20% → 淡出的渐变描边）—— iOS 玻璃的标志性细节。

并且把不透明度提到 **97%**：参考是 75% + 模糊，我们没有模糊就**不能再透**，
背景漏出来正是"廉价感"的主因。

---

## 六、复算命令

```powershell
cd C:\kivo video\android
$env:JAVA_HOME="C:/Android/jdk-17.0.13+11"; $env:ANDROID_HOME="C:/Android/sdk"
& ./gradlew --% :app:assembleDebug --offline --no-configuration-cache
adb -s b6e950ee install -r -t app/build/outputs/apk/debug/app-debug.apk
adb -s b6e950ee shell am force-stop com.kivo.cinema
adb -s b6e950ee shell am start -n com.kivo.cinema/.ui.compose.PlayerComposeActivity `
  --es kivo.sourceId seed-smb-xiaomi --es kivo.smbShare XiaoMi `
  --es kivo.smbPath "视频/kivo-test/shelter.mkv"

# 唤出控制条
adb -s b6e950ee shell input tap 1200 540
# 开画质比例面板（圈内箭头图标，真实坐标 x≈1914,y≈58）
adb -s b6e950ee shell input tap 1914 58
# 选 4:3（真实坐标 x≈1626,y≈574）
adb -s b6e950ee shell input tap 1626 574
# 核对内核写入
adb -s b6e950ee shell "run-as com.kivo.cinema cat files/mpv-kernel.log" | Select-String "Set property"
```

> ⚠️ 顶栏 8 个图标后，**坐标每次都要重新量**（图标数变了位置就变了）。
> 量坐标用像素脚本，别从缩放后的截图上目测。
