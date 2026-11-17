# B1 真机验证报告 · 导航选中态 / Shell 路由 / 避让统一

> 归属任务：B1-T4 · 执行时间 2026-11-16 22:38–22:47 · 执行机型 Redmi K40（M2012K11AC / alioth）
> 对照基线：`docs/audit/golden/rex/`（竞品实拍帧）· kivo 侧证据：`docs/audit/golden/kivo/*.png`
> 判据：**只比布局与视觉参数，不做逐像素缩放比对**（竞品 693×1500 vs kivo 1080×2400，见 `golden/manifest.md` §2 注）

---

## 0. 验证环境与产物（可复算）

**设备（按 team-leader 22:38 指令改用 `b6e950ee`）**

| 项 | 实测值 | 来源 |
| --- | --- | --- |
| 序列号 | `b6e950ee` | `adb devices -l` |
| 型号 / 系统 | `M2012K11AC` / Android 13（HyperOS） | `getprop ro.product.model`、`ro.build.version.release` |
| 分辨率 | `Physical size: 1080x2400` | `adb -s b6e950ee shell wm size` ✅ |
| 密度 | `Physical density: 440`（2.75×，1dp=2.75px） | `adb -s b6e950ee shell wm density` ✅ |
| 方向 | `rotation 0`（竖屏）；**起始为横屏，已先归零** | `dumpsys display`（见 §0.1 说明） |

> `dd137cf7` 也在线，但**未在其上安装、未在其上截图**，本报告不引用它的任何数据（无"非指定机"数据可供参考）。

**构建与安装**

| 步骤 | 命令 | 结果 |
| --- | --- | --- |
| 编译 | `JAVA_HOME=C:/Android/jdk-17.0.13+11 ./gradlew.bat :app:assembleDebug --offline -q` | **exit 0** ✅ |
| APK | `android/app/build/outputs/apk/debug/app-debug.apk` | 69,772,519 B · 时间戳 `Nov 16 22:41` ✅ |
| 安装 | `adb -s b6e950ee install -r <apk>` | `Performing Streamed Install` → **`Success`**，exit 0 ✅ |
| 启动 | `adb -s b6e950ee shell am start -n com.kivo.cinema/.MainActivity` | `mCurrentFocus=…com.kivo.cinema.MainActivity` ✅ |

> 安装期间未出现 `INSTALL_FAILED_UPDATE_INCOMPATIBLE`，未执行任何 `uninstall`（铁律）。HyperOS 的「USB安装提示」在本轮未阻塞（`install -r` 一次成功）。

**截图产物（`docs/audit/golden/kivo/`，均为 1080×2400 PNG）**

| 文件 | 页面 | 字节数 | 全图平均亮度 |
| --- | --- | --- | --- |
| `discover.png` | 发现（page=1） | 2,569,832 | 79.51 |
| `profile.png` | 我的（page=3） | 87,926 | 240.61 |
| `library.png` | 资源库（page=0） | 197,914 | 229.93 |
| `settings.png` | 设置（page=4） | 215,539 | 241.87 |

> 抓图方式：`adb -s b6e950ee exec-out screencap -p > <file>`（Git Bash 重定向不做 CRLF 转换）；每张都用 Python 校验 `\x89PNG` 头与 `IHDR=1080×2400`，四张全部通过。

### 0.1 采集过程中的两个环境坑（已解决，记录以便复算）

1. **起始横屏**：首次 `uiautomator dump` 得到的导航 bounds 是 `x=1907`（>1080），说明设备当时是横屏（2400×1080 逻辑坐标）。
   处置：`settings put system accelerometer_rotation 0` + `user_rotation 0`，`dumpsys display` 复核 `mCurrentOrientation=0` 后重采。**首轮横屏数据全部作废，未用于本报告任何结论。**
2. **Git Bash 路径改写**：`adb shell uiautomator dump /sdcard/ui.xml` 会把 `/sdcard/ui.xml` 改写成 `C:/Program Files/Git/sdcard/ui.xml`，dump 与 pull 双双失败却仍打印 "dumped to"。处置：全部命令前加 `MSYS_NO_PATHCONV=1`。

---

## 1. 竞品契约（rex 帧读数）

| 契约项 | 读数 | 证据 | 置信 |
| --- | --- | --- | --- |
| 胶囊条显示顺序 | 发现 / 我的 / 资源库 / 设置，右侧独立圆形钮 | `rex/2026_11_16_21_22_14_IMG_0366.jpg`、`rex/2026_11_16_21_17_23_IMG_0333.jpg` | ✅ 实拍 |
| 选中态形态 | **选中项 = 白/近白胶囊底 + 近黑图标与文字**，非蓝色 tint | `IMG_0366`（资源库选中） | ✅ 实拍 |
| 选中胶囊与未选中区亮度差 | 选中底 ≈ 纯白（≈255）vs 胶囊条底 ≈ 240（Δ≈15） | `IMG_0366` 目测取样 | ◐ 推断（JPEG + 玻璃渐变，无硬边界，`thr=6` 边缘检测无输出） |
| 胶囊高度 / 屏高 | 92px / 1500 ≈ **6.1%** | `IMG_0366` | ◐ 目测（同因无硬边界） |
| 胶囊距底 / 屏高 | ≈ 75px / 1500 ≈ **5.0%** | `IMG_0366` | ◐ 目测 |
| 「发现」图标 | **彩色真人头像（圆角方图，非单色描边）** | `IMG_0333`、`IMG_0366` 左下第一项 | ✅ 实拍 |
| 「我的」图标 | 实心圆形人像剪影（黑圆 + 反白人形） | `IMG_0366` | ✅ 实拍 |
| 「资源库」图标 | 深色「箱/库」形（带盖的梯形块） | `IMG_0366` | ✅ 实拍 |
| 「设置」图标 | 实心齿轮 | `IMG_0366` | ✅ 实拍 |
| 内容与导航关系 | **内容全屏铺底，胶囊浮在内容之上**（0333 的综艺大卡被胶囊压住） | `IMG_0333` | ✅ 实拍 |
| 设置页竞品帧 | **不存在**（`golden/rex/` 12 帧中无设置页） | `golden/manifest.md` §2 | ✅ 清单核对 |

> **订正（实拍推翻此前审计推断）**：`NavGlyph.kt:18-19` 的注释写「0366 中『发现』项是头像轮廓（描边），不是指南针」。实拍复核：0366/0333 的「发现」是**彩色头像照片**，不是单色描边。B1-T1 把它做成描边轮廓只达成"不是指南针"，未达成"与竞品同形"。该差异列差距 D-4。

---

## 2. 当前实现（代码锚点）

| 项 | 实现位置 | 关键值 |
| --- | --- | --- |
| 选中胶囊底 | `ui/design/CapsuleNavBar.kt:22,25` | `SelectedCapsuleLight = White α0.60`、`SelectedCapsuleDark = White α0.14` |
| 选中前景 | `ui/design/CapsuleNavBar.kt:57` | 浅色 `textMain`（近黑）/ 暗色 `Color.White`；**无 tint** |
| 胶囊条 | `ui/design/CapsuleNavBar.kt:48,52-53` | `RoundedCornerShape(25.dp)`、`height(50.dp)`、`shadow(6.dp)` |
| 选中胶囊内缩 | `ui/design/CapsuleNavBar.kt:58-63` | `padding(vertical = 3.dp)` → 上下各内缩 3dp |
| 搜索钮 | `ui/design/CapsuleNavBar.kt:74-79` | `size(50.dp)`、`CircleShape`、字形 `search`（放大镜） |
| 图标字形 | `ui/design/home/NavGlyph.kt:43-72` | `"发现"`→`drawAvatarOutline`（描边）；`"我的"`→Difference 实心剪影；`"资源库"`→梯形盖+圆角块；`"设置"`→48 点齿轮 |
| Shell 路由 | `MainActivityShell.kt:69-104`、`ui/shell/ShellTab.kt:10-22` | `ShellTab` 枚举 + `displayOrder`；`WatchingPageCode` 归到「发现」选中 |
| 页面避让 | `MainActivityShell.kt:117-124`、`ui/shell/ShellInsets.kt:13` | 发现/资源库 `bottom=0`；其余页 `ShellInsets.NavReservedHeight = 84.dp` |
| 资源库内部留白 | `sources/ui/workspace/SourceWorkspace.kt:26` | `padding(bottom = ShellInsets.NavReservedHeight)`（原 94dp → 84dp） |
| FLAG_SECURE | `privatezone/PrivateActivity.kt:59,66` | `onCreate` 加标志、`onDestroy` 清除；Shell 侧已无该标志 |

---

## 3. 逐页对照（五维判定）

### 3.1 选中态颜色

| 页面 | 选中项实测 | 判据 | 结论 |
| --- | --- | --- | --- |
| 发现 | 选中列胶囊底亮度 74.2 vs 其余 44.2（Δ30）；前景白 | 白半透明胶囊、无蓝 tint | **PASS** |
| 我的 | 选中列 246.2 vs 其余 233.2（Δ13）；前景近黑（图标亮度 26） | 同上 | **PASS** |
| 资源库 | 选中列 246.2 vs 其余 233.2（Δ13）；前景近黑 | 同上 | **PASS** |
| 设置 | 选中列 246.2 vs 其余 233.2（Δ13）；前景近黑 | 同上 | **PASS** |

- **蓝色 tint 残留复核**：四页所有导航列采样 RGB 的 `max-min` 色偏为 1.5–10.3，且 B 通道仅比 R/G 高 3–10（属 #F2F2F8 类中性灰），**没有任何一列是蓝主导**。→ 去 tint 达成。
- 证据：`docs/audit/golden/kivo/{discover,profile,library,settings}.png` + `CapsuleNavBar.kt:22,25,57`。
- 与竞品差：rex 选中底为近纯白（◐ ≈255），kivo 为 α0.60 叠加后 246（Δ13）。量级同阶，视觉上足够明确。见差距 D-2。

### 3.2 胶囊位置与高度

| 测量项 | 实测（px，1080×2400） | 代码值 | 结论 |
| --- | --- | --- | --- |
| 胶囊条 top（四页） | **2157**（discover/profile/library/settings 全部一致） | — | **PASS**（跨页零跳动） |
| 胶囊条 bottom | 2294 → 高度 **137px = 49.8dp** | `height(50.dp)` = 137.5px | **PASS**（误差 0.5px） |
| 搜索钮 top/bottom | 2157 / 2294 → **138px = 50.2dp** | `size(50.dp)` | **PASS** |
| 选中胶囊 top | 2165（= 2157 + 8px = 3dp 内缩） | `padding(vertical = 3.dp)` = 8.25px | **PASS** |
| 距屏底 | 2400 − 2294 = **106px**（22dp 内容距 + 系统导航栏 inset 45px） | `MainActivityShell.kt:183` | **PASS** |

- **跨页一致性**：B1-T3 前 `SourcesScreen` 用 94dp（=258.5px）、Shell 用 84dp（=231px），导致资源库与其他页胶囊位置差 10dp（27.5px）。本轮实测资源库页胶囊 top 与「我的/设置」**同为 2157**，差值为 0 → **PASS**，回归已消除。
- 证据：`profile.png` / `library.png` 同位置边缘扫描（@x=100：上下沿均 2158 / 2294）；`ShellInsets.kt:13`、`MainActivityShell.kt:119`。

### 3.3 图标形状

| 图标 | kivo 实测 | rex 契约 | 结论 |
| --- | --- | --- | --- |
| 发现 | 单色**描边**头肩轮廓 | **彩色真人头像照片** | **FAIL**（见 D-4） |
| 我的 | 实心圆 + 反白剪影 | 实心圆 + 反白剪影 | **PASS** |
| 资源库 | 梯形盖 + 圆角块（选中态转近黑） | 深色箱/库形 | **PASS**（同语义，细节略简） |
| 设置 | 48 点齿轮，齿形清晰 | 实心齿轮 | **PASS** |
| 搜索钮 | 放大镜 | 0333 帧为放大镜 ✅ / 0366 帧右侧圆钮为「+框」◐ | **PASS**（以 0333 为准） |

- 证据：`library.png`（四图标同屏最佳观测）、`rex/2026_11_16_21_22_14_IMG_0366.jpg`、`NavGlyph.kt:43-72`。

### 3.4 底部避让

| 页面 | 实测 | 判据 | 结论 |
| --- | --- | --- | --- |
| 发现 | 内容满屏铺底，胶囊浮于其上（hero 图透到胶囊下沿） | 与 rex 0333 同做法 | **PASS** |
| 资源库 | 内容不裁底（第三行服务器卡被胶囊遮一半） | `MainActivityShell.kt:118` 主动要求 | **PASS** |
| 我的 | 内容区在胶囊上方结束（`2400 − 231 = 2169` 以上） | 84dp 预留 | **PASS** |
| 设置 | 内容可滚动进入胶囊下方，但预留 84dp 后**滚到底不会再被永久遮挡** | `ShellInsets.NavReservedHeight` | **PASS** |

- 证据：`library.png`（@x=100 内容末像素 2125，胶囊 2157 起）、`settings.png`（末项「数据与资源」位于胶囊上方，可继续滚动）、`MainActivityShell.kt:117-124`、`SourceWorkspace.kt:26`。

### 3.5 与 rex 的剩余差异

| 编号 | 差异 | 证据 | 严重度 |
| --- | --- | --- | --- |
| D-1 | kivo 胶囊条为**灰白实底 + 6dp 阴影**；rex 为**高透玻璃**（背景色透出，几乎无阴影） | `library.png` 胶囊底 233 vs 页面白 242；`IMG_0366` 胶囊底透出粉蓝环境色 | P2 |
| D-2 | 选中胶囊 α0.60 叠加后 246，达不到 rex 的近纯白 | 亮度实测 246.2 / ◐255 | P3 |
| D-3 | 导航字号 10sp、图标 25dp（含 3dp 内缩后有效胶囊高 44dp）；rex 胶囊比例 6.1% 屏高 vs kivo 5.7% | 137/2400 vs ◐92/1500 | P3 |
| D-4 | 「发现」为单色描边轮廓，rex 为彩色头像照片 | `library.png` vs `IMG_0333` | P2 |
| D-5 | 「资源库」选中图标内置一个 `White α0.75` 小圆点（`NavGlyph.kt:58`），在白色选中胶囊上几乎不可见 | 代码 `NavGlyph.kt:58` + `library.png` 目视无白点 | P3 |
| D-6 | 设置页无竞品帧可对照，其内部形态（卡片分组）本轮**无法判定** | `golden/manifest.md` | — 记"未测"而非 PASS |

---

## 4. FLAG_SECURE 回归验证（B1-T2 核心风险点）

| 步骤 | 观测 | 判定 |
| --- | --- | --- |
| 资源库 → 点「私密区」卡（`[557,447][1044,772]`） | 前台变为 `com.kivo.cinema.privatezone.PrivateActivity` | 路由 OK ✅ |
| 私密区页截图 | **0 字节**（`screencap` 被 `FLAG_SECURE` 拦截，输出空） | 私有内容仍受保护 ✅ |
| `KEYCODE_BACK` 返回 | 前台回到 `com.kivo.cinema.MainActivity` | ✅ |
| 返回后立即截图（资源库页） | 平均亮度 **229.92** | 非黑 ✅ |
| 再切「发现」页截图 | 平均亮度 **79.51**（暗色 hero 图，非黑屏） | **PASS**（阈值 >20） |

- 结论：`FLAG_SECURE` 已从 Shell 侧彻底移除、只在 `PrivateActivity` 生效；`onDestroy` 的 `clearFlags` 能正确解除，**未发现"回主界面后整屏被禁截图"的回归**。
- 证据：`.tmp/audit/private_page.png`（0 B，临时文件，不入库）、`after_private_library.png`（229.92）、`after_private_discover.png`（79.51）；代码 `PrivateActivity.kt:59,66`。

---

## 5. 差距清单（可落地）

| 编号 | 差距 | 证据 | 用户可见后果 | 可落地改法（文件与符号） | 优先级 |
| --- | --- | --- | --- | --- | --- |
| D-4 | 「发现」是单色描边头像，竞品是彩色头像照片 | `library.png` vs `IMG_0333` | 四个 tab 里唯一一个"不像人"的入口，第一眼识别度低 | 在 `ui/design/home/NavGlyph.kt` 增加 `AvatarImage` 分支：`NavGlyph` 保持单色兜底，`CapsuleNavBar` 对 `label == "发现"` 改走 `Image(Bitmap)`（占位先用 `ui/components/MediaCard.kt` 同款圆角 6dp 裁切）；无头像时回落描边 | P2 |
| D-1 | 胶囊条是灰白实底 + 阴影，竞品是高透玻璃 | `library.png` 胶囊底 233 / 页面 242；`IMG_0366` 透色 | 导航条在浅色页像"贴上去的灰条"，缺少悬浮玻璃感 | `CapsuleNavBar.kt:46,53,75`：`pill` 改 `Color.White.copy(alpha = 0.55f)` + `Modifier.blur`/`GlassSurface`（复用 `ui/components/GlassSurface.kt`），`shadow(6.dp)` → `2.dp` | P2 |
| D-2 | 选中胶囊不到近纯白（246 vs ◐255） | 亮度实测 | 选中态在浅色页偏弱，"哪一页被选中"要凑近看 | `CapsuleNavBar.kt:22`：`alpha 0.60f` → `0.92f`；同步实测复采 `library.png` 取新亮度 | P3 |
| D-5 | 「资源库」字形里写死 `Color.White.copy(alpha=.75f)` 圆点，白胶囊上不可见 | `NavGlyph.kt:58` | 无（视觉噪声，且违反裸色字面量规范） | `NavGlyph.kt:58`：删掉该 `drawCircle`，或在 `NavGlyph` 增加 `accent: Color?` 参数由调用方传 `DesignTokens.textMain` | P3 |
| D-3 | 胶囊比例 5.7% 屏高 vs 竞品 6.1%；字号 10sp | 137/2400 vs ◐92/1500 | 胶囊比竞品略扁，长标签（「资源库」）显得紧 | `CapsuleNavBar.kt:52` `height(50.dp)` → `54.dp`，`ShellInsets.kt:13` 同步 `84.dp` → `88.dp`（**必须同卡改，否则重演 B1-T3 的跳变**） | P3 |
| D-6 | 设置页无竞品帧，形态未判定 | `manifest.md` §2 | 无（信息缺口） | 需追加一张竞品设置页帧进 `golden/rex/` 并更新 `manifest.md` 数量（12→13），再开卡 | — |

**优先级汇总卡**

| 优先级 | 条目 | 数量 |
| --- | --- | --- |
| P0 | 无 | 0 |
| P1 | 无 | 0 |
| P2 | D-4（发现头像）、D-1（玻璃胶囊） | 2 |
| P3 | D-2（选中白度）、D-3（比例）、D-5（白点） | 3 |
| 待补信息 | D-6（设置页竞品帧缺失） | 1 |

---

## 6. 结论

**B1 达成情况：达成（PASS），无回归。**

| B1 子项 | 判据 | 结果 |
| --- | --- | --- |
| 选中态去蓝 tint、改白胶囊 + 近黑前景 | 四页实测无蓝主导，选中列 Δ13–Δ30 | ✅ PASS |
| 「发现」不再是指南针 | 描边人像轮廓 | ✅ PASS（与竞品"彩色头像"仍差，列 D-4） |
| Shell 路由用 `ShellTab` 枚举（无裸 int） | `MainActivityShell.kt:90-93` | ✅ 代码核对通过，四页切换实测可达 |
| `FLAG_SECURE` 下沉到 `PrivateActivity` | 私密页 0 字节 / 主界面 229.92、79.51 | ✅ PASS，无泄漏、无残留 |
| 避让统一（94dp → 84dp），跨页胶囊零跳动 | 四页胶囊 top 全为 2157 | ✅ PASS（差值 0） |
| 编译/安装 | `assembleDebug` exit 0、`install -r` Success | ✅ PASS |

**回归清单：0 项。** 特别复核的两处高风险均正常：① `FLAG_SECURE` 未泄漏到 Shell；② 资源库与其他页胶囊位置差为 0（B1-T3 的目标）。

**下一步建议**：B1 可以收口；把 D-4 / D-1 并入 B2（我的页/头像体系）与 B3（发现页），D-2/D-3/D-5 作为一个 15 分钟的微调卡（`CapsuleNavBar.kt` + `NavGlyph.kt` + `ShellInsets.kt` 三文件同改，改完必须重采本报告四张图复算）。

---

## 7. 附：复算命令

```bash
export MSYS_NO_PATHCONV=1
ADB=C:/Android/sdk/platform-tools/adb.exe

# 0) 竖屏锁定（否则 uiautomator 给横屏坐标，会出现 x>1080 的 bounds）
$ADB -s b6e950ee shell settings put system accelerometer_rotation 0
$ADB -s b6e950ee shell settings put system user_rotation 0
$ADB -s b6e950ee shell wm size && $ADB -s b6e950ee shell wm density   # 1080x2400 / 440

# 1) 编译 + 安装
cd android && JAVA_HOME=C:/Android/jdk-17.0.13+11 ./gradlew.bat :app:assembleDebug --offline -q
$ADB -s b6e950ee install -r app/build/outputs/apk/debug/app-debug.apk

# 2) 四页采集（tab 中心：发现 158,2259 / 我的 356,2259 / 资源库 554,2259 / 设置 752,2259）
$ADB -s b6e950ee shell am start -n com.kivo.cinema/.MainActivity
$ADB -s b6e950ee exec-out screencap -p > docs/audit/golden/kivo/discover.png
$ADB -s b6e950ee shell input tap 356 2259; sleep 2; $ADB -s b6e950ee exec-out screencap -p > docs/audit/golden/kivo/profile.png
$ADB -s b6e950ee shell input tap 554 2259; sleep 2; $ADB -s b6e950ee exec-out screencap -p > docs/audit/golden/kivo/library.png
$ADB -s b6e950ee shell input tap 752 2259; sleep 2; $ADB -s b6e950ee exec-out screencap -p > docs/audit/golden/kivo/settings.png

# 3) FLAG_SECURE 回归：资源库 →「私密区」卡 [557,447][1044,772]
$ADB -s b6e950ee shell input tap 554 2259; sleep 2
$ADB -s b6e950ee shell input tap 800 610; sleep 3      # 进入 PrivateActivity
$ADB -s b6e950ee exec-out screencap -p > .tmp/audit/private_page.png   # 期望 0 字节
$ADB -s b6e950ee shell input keyevent KEYCODE_BACK; sleep 2
$ADB -s b6e950ee exec-out screencap -p > .tmp/audit/after_private_library.png
$ADB -s b6e950ee shell input tap 158 2259; sleep 2
$ADB -s b6e950ee exec-out screencap -p > .tmp/audit/after_private_discover.png

# 4) 亮度断言（阈值 >20）
python -c "from PIL import Image
for f in ['after_private_library','after_private_discover']:
    im=Image.open(f'.tmp/audit/{f}.png').convert('RGB'); px=list(im.getdata())
    print(f, round(sum(0.299*r+0.587*g+0.114*b for r,g,b in px)/len(px),2))"
```

---

## 8. 门禁（`tools/ui_gate.py`）

本轮为**验证卡，不修改生产代码**（B1 三项改动由 B1-T1/T2/T3 落地），故门禁以既有基线复核。真实执行：

```bash
python tools/ui_gate.py --baseline    # 注意：--baseline 是开关，不接受路径参数
```

关键输出（2026-11-16T22:48:30+08:00，Kotlin 376 个 / Page 5 个）：

```
── 4. 回归结论
   [FAIL] 存量违规 17 项（已在 baseline 中，不算回归；修复目标见 B5）
   [INFO] 行数无新增超限文件（当前超限 22 个，均在 baseline 内）
   [WARN] 裸色与 baseline 持平：326（未下降）
   [WARN] 缺 @Preview 的 Page 数与 baseline 持平：5
RESULT: PASS（无回归；存量违规与 warning 为待治理项，不阻断）
EXIT=0
```

> **结论：B1 三项改动未引入行数超限、未新增裸色、未减少 `@Preview` 覆盖率（全部与 baseline 持平）。**
> `CapsuleNavBar.kt`（81 行）、`NavGlyph.kt`（84 行）、`ShellTab.kt`（23 行）、`ShellInsets.kt`（14 行）均远低于 300 行上限，且均未出现在裸色 top-20 中 —— 与 §5 D-5 的判断一致：唯一残留裸色是 `NavGlyph.kt:58` 的 `Color.White`，量小未进榜单，仍按 P3 记录。