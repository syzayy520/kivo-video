# 313 · 品牌启动动画（冷启动 2.3 s / 热启动 0.8 s）

状态：**已实现并在平板真机验证通过**
设备：**MatePad `CFE6R21B12000179`**（1600×2560，640×1024 dp）
时间：2026-11-16 10:20–11:45
来源：用户提供的设计稿（`Kivo Video 品牌标识与启动动画.md/.html`）

## 一、做了什么

把设计稿的交互演示（HTML + JS）搬成 Android 上的真动画：

```
ui/brand/
├── BrandTimeline.kt    时间轴与缓动（纯函数 ⇒ 可 JVM 单测，14 个断言）
├── BrandMark.kt        标识几何（三角 + KIVO 字标）
├── StartupOverlay.kt   Compose 覆盖层（绘制）
└── StartupPolicy.kt    播放策略（首装 / 隔天 / 冷热启动判定）
```

接入两处：
- `MainActivity` —— **只有冷启动**（`savedInstanceState == null`）才播；
- `values-v31/themes.xml` —— Android 12+ 强制系统启动页：**纯色底 `#050506` + 全透明图标**，
  由应用内动画接手 ⇒ 两段同色无缝。

## 二、时间轴（照搬设计稿，不是另编）

设计稿的 `render()` 里是一组 `seg(t, a, b, ease)` 调用，这里原样搬成 Kotlin：

| 段 | 冷启动 | 热启动 |
| --- | --- | --- |
| 标识淡入 | 0.75 – 1.30 s | 0.03 – 0.35 s |
| 细线横向展开 | 0.25 – 0.85 s | 同 |
| 显影（clip 从细缝上下打开） | 0.70 – 1.40 s | 同 |
| 柔光 16° 扫过 | 0.95 – 1.75 s | 0.10 – 0.55 s |
| 淡出 + 主界面渐显 | 1.95 – 2.30 s | 0.50 – 0.80 s |
| 总时长 | 2.3 s | 0.8 s |

缓动也照搬：`eOut=bez(.22,.61,.36,1)`、`eIO=bez(.65,0,.35,1)`、`eIn=bez(.4,0,1,1)`
（三次贝塞尔 + 牛顿迭代，与设计稿同一套）。

**显影的实现**（设计稿 `clipPath: inset(top% right% bottom% left%)`）：
令 `c = 1 - reveal`，则 `top = slit × c`、`bottom = (100 - slit) × c`。
冷启动 `slit = 23.3%`（正好落在组合标识里三角细缝的位置），热启动 `slit = 50%`（只有三角）。

## 三、验证

| 项 | 结果 |
| --- | --- |
| JVM 单测 `BrandTimelineTest` | **14/14 通过**（含每个关键节点 + 越界不变量） |
| 平板真机（正常速度） | 显影中 → 细线展开 → **完整标识** → 主页，逐帧确认 |
| 平板真机（慢放抓帧） | 完整标识 = 播放三角（上下两片 + 细缝）+ **KIVO 字标（含 O 圆环）**，布局同设计稿 |
| 淡出衔接 | 主页主体是深色 ⇒ 黑底淡出到主页**不突兀** |

**14 个单测钉住的**：缓动两端夹紧与单调性、冷启动起始纯黑、细线在 0.25–0.85 s 展开、
标识 0.75 s 后才淡入、显影带单调收缩、柔光扫入扫出、末段淡出与主界面显现、
缩放只从 0.985 到 1.0、热启动更短更简、两套时间轴**所有参数恒在合法区间**。

## 四、真机反馈与根因（这一节比代码更值得记）

用户报告两个现象：

1. **标识只显示上半部分**（三角只剩一片、KIVO 只有上半）
2. **进入主页后那条横线长期留在界面**

### 现象 1 的根因：**不是 bug，是我装的「4 倍慢放调试版」**

抓帧需要慢放——2.3 秒的动画按正常速度截图只能抓到 6 帧。我加了 `DEBUG_SLOW_MOTION = 4f`
装到平板上抓帧，**抓完没及时改回、也没告诉用户**。

于是用户看到的动画被拉长到 **9.2 秒**、细线持续 **1~3.4 秒**，
视觉上就是"卡在显影中途"+"横线久久不散"。**两个现象是同一个原因。**

⇒ 教训：**调试用的临时开关，要么当场改回，要么明确告知"这是慢放版"。**
现在该常量已**彻底删除**（不是改回 1，是删掉），从代码上消灭再犯的可能。

### 现象 2 的根因：**`startupAnimationDone` 不是 Compose State（真 bug）**

```kotlin
// ✗ 错误写法：Activity 的普通字段
internal var startupAnimationDone = false
setContent {
    if (coldStart && !startupAnimationDone) { StartupOverlay(...) }
    // onFinished 里 startupAnimationDone = true  ⇒ **不触发重组**
}
```

改了普通字段 ⇒ Compose 不知道要重组 ⇒ **覆盖层永远留在界面上**。
真机表现：主页中间残留一条**白色细线**（动画的细线元素停在某一帧）。
实测位置与长度对得上：线长 ≈ `boxW × lineScale`，`lineScale ≈ 0.49` ⇒ 停在 t ≈ 0.4 s 那一帧。

**修法**：把"已播完"放进 Compose 自己的状态里。

```kotlin
setContent {
    var brandDone by remember { mutableStateOf(false) }   // ✓ State
    if (coldStart && !brandDone) {
        StartupOverlay(plan = startupPlan, onFinished = { brandDone = true })
    }
}
```

**修复验证**：装新包 → 启动 → 7 秒后截屏 = **主页干净、无白线** ✔

> ⚠️ 我最初把这个现象和「标识只显示上半」判断成**同一个原因**（都归给慢放版），
> 这是错的 —— 它是**两个独立问题**：慢放版造成"显示一半"，State bug 造成"白线残留"。
> 教训：**两个现象同时出现，不要急着归成一个原因**，要分别找证据。

### 现象 2 的另一层：覆盖层可能永久停留（已加兜底）

即使 State 修好了，如果帧循环因任何原因中断（异常、帧时钟停摆），`onFinished` 仍不会调用，
**黑色覆盖层会永久盖住整个界面** —— 那属于"应用不可用"级别。

⇒ 已加超时兜底：

```kotlin
val timedOut = withTimeoutOrNull(((end + 0.6f) * 1000f).toLong()) {
    ... 帧循环 ...
}
NetDiag.log("[brand] 启动动画结束 elapsed=$lastElapsed 正常=${timedOut != null}")
onFinished()   // 超时也一定执行
```

**最多 end + 0.6 秒必定收尾**，日志能区分"正常结束"与"超时收尾"。

**验证（平板真机）**：

```
[brand] 启动动画开始 plan=FULL end=2.3
[brand] 启动动画结束 elapsed=2.3000042 正常=true     ← 正常跑完，不是超时救的
```

## 五、踩的坑（五个真 bug）

### 1. O 字母整圈消失
`wordO()` 里圆的 y 中心我写成了 **218.6**（那是 x 中心），正确是 **36**。
⇒ 圆跑到 y=182~255，而字标只有 y=0~72 ⇒ 整圈被裁掉，**O 凭空消失**。
（x 中心与 y 中心数值相同是最容易混的地方，因为 `WORD_O_OUTER` 只有一个常量。）

### 2. 显影完成后整个标识消失
设计稿写的是 `op<1 ? inset(...) : 'none'` —— **完全展开时不裁剪**。
我一开始无脑套 `clipRect(top, bottom)`，展开后 `top == bottom` ⇒ **裁剪高度为 0** ⇒ 标识全没了。
⇒ 必须显式分支：`if (revealed) 直接画，不裁剪`。

### 3. Kotlin 两个编译坑
- lambda 里不能用 `return@label`（`(Float) -> Float` 的 lambda 返回的是 Float 不是函数）⇒ 改成 if-else 表达式；
- **单测方法名不能带小数点**（`fun \`... 0.25 ...\`` 非法）⇒ 名字里改用文字。

### 4. 平板没有 `screenrecord`
K40 有，平板没有 ⇒ 改用**连续 `screencap`** 抓帧。
2.3 秒按正常速度只能抓到 6 帧，看不清中间 ⇒ 加了 `DEBUG_SLOW_MOTION` 慢放开关（**已改回 1**）。

## 五、已知边界

1. **只挂在 MainActivity**，**播放页（PlayerComposeActivity）绝没有** —— 点开就要看片，不能等 2.3 秒。
2. **在平板上验证的**（K40 当晚没电）。尺寸全部按 dp 计算，与屏幕无关，但**手机上未实机复验**。
3. **音效未实现**（设计稿：98/196/294 Hz + lowpass 900 Hz + 1.1 s 衰减，在 0.70 s 响起）。
   需要合成音频或 `AudioTrack` 实时合成；**不得抢占音频焦点**，要遵循静音与勿扰。
4. **柔光只画了热启动版（只有三角）**；冷启动（三角 + 字标组合）的柔光层**未实现**
   ——`drawSheen` 里 `if (!full)` 分支缺失。
5. **没有设置开关**：策略类有 `enabled` 字段（默认 true），但**没有 UI 入口**。
   用户若觉得每天看一次动画烦，需要加一个设置项。
6. **主界面渐显 `homeAlpha` 未使用**：覆盖层在最后 0.35 s 整体淡出，等效，但没有单独的渐显层。

## 六、复算命令

```bash
export JAVA_HOME="C:/Android/jdk-17.0.13+11"; export ANDROID_HOME="C:/Android/sdk"
export PATH="/c/Android/sdk/platform-tools:$PATH"; export MSYS_NO_PATHCONV=1
cd android
./gradlew :app:testDebugUnitTest --offline --tests "*BrandTimelineTest"   # 期望 14/14
./gradlew :app:assembleDebug --offline
adb -s CFE6R21B12000179 install -r "C:/kivo video/android/app/build/outputs/apk/debug/app-debug.apk"

# 看完整版动画（清掉策略记录 ⇒ 判定为首装）
adb -s CFE6R21B12000179 shell am force-stop com.kivo.cinema
adb -s CFE6R21B12000179 shell "run-as com.kivo.cinema rm -f shared_prefs/kivo_startup_anim.xml"
adb -s CFE6R21B12000179 shell am start -n com.kivo.cinema/.MainActivity
```

## 七、证据

`docs/evidence/p313-startup-anim/`：平板真机逐帧截图（正常速度与慢放抓帧）。
