# D377 端到端冒烟续篇：详情页/Emby 浏览/起播 + 修复「返回后横屏 5–6 秒」

## 一、冒烟范围（承接 D376 只覆盖首页的缺口）

D366–D368 改过详情页（四路并行+持久化缓存）、Emby 列表（10min 缓存）、登录会话（复用），
但只在各自轮次验过单点指标。本轮做完整链路真机冒烟：**首页 → 详情页 → Emby 浏览 → 起播 → 返回**。

## 二、结论 A：D366–D368 的优化在真机全部生效（无回归）

### 详情页（Emby 剧集《兰香如故》47 集）
| 场景 | 日志 | 结果 |
|---|---|---|
| 冷进（首次） | `load-start preview=true` → `load-done ok=true` | **1068ms** |
| 热进（二次） | 全 `[emby-cache] HIT` | **34ms** |

链路细节（均为实测日志）：
- `detail-perf 1-emby-details` 108ms、`2-tmdb hit=282326`（TMDB 磁盘缓存命中）、`3-quality`；
- **`[emby] all-episodes series=255945 n=47 seasons=[1]`** —— 整剧 47 集走**单请求**，D366 的核心改动生效；
- 页面渲染完整：`2026年9月11日 · 中国大陆 · 47集`、播放按钮、`第1季·1集`、完整简介、`TMDB 7.4`、播出日历 `集47/47`。

### Emby 浏览页
| 场景 | `emby-items` 耗时 | 结果 |
|---|---|---|
| 首次进入 | 724 / 359 / 357 / 346 ms（n=6/60/1/12） | 正常 |
| 二次进入 | **3 / 1 / 1 / 2 ms**（全 HIT） | D367 缓存生效 |

### 起播（Emby 剧集）
- `[color] 出画模式 = gpu-next`、`MPV 首帧就绪`（点播放→首帧 **762ms**）；
- `[cache] fw=… dur=10.0s underrun=false` 稳态无 underrun；
- 截图 `smoke376-emby-play.png`：真实画面、色彩正常、无黑屏无错误。

### 顺带核对 #160（多季）
`[emby] all-episodes ... seasons=[1]` —— 该剧仍只有 1 季，与 D372 结论一致（服务器缺 ≥2 季片源）。
不重复折腾，保持 pending。

## 三、结论 B：抓到真缺陷——退出播放器后主界面横屏残留 5–6 秒

### 现象
从播放器 `KEYCODE_BACK` 返回首页，用 `screencap` 读 PNG 头测实际帧尺寸：

| 时刻 | 修复前 | 修复后 |
|---|---|---|
| 首页起点 | 1080×2400 | 1080×2400 |
| 播放器内 | 2400×1080 | 2400×1080 |
| 返回后 1s | **2400×1080** | **1080×2400** |
| 返回后 2–5s | **2400×1080** | 1080×2400 |
| 返回后 6s | 1080×2400 | 1080×2400 |

横屏期间首页按横屏布局：hero 占满整屏、`继续观看`/`电影` 等行被挤出视口 ——
**用户看到的就是"内容全不见了"**（我第一次误判为渲染故障，靠 PNG 头尺寸才定性）。
`uiautomator` 只剩底部 4 个导航项、无崩溃无 ANR、切 tab 同样空 —— 都是横屏布局的次生现象。

### 根因
- `AndroidManifest.xml` 第 48 行：`PlayerComposeActivity` = `android:screenOrientation="landscape"`；
- `MainActivity` **未声明** `screenOrientation`（默认 `unspecified`），全项目**无任何 `setRequestedOrientation`**；
- 退出播放器后主界面按传感器重新判定方向，系统需数秒稳定（真机 5–6s），期间保持横屏。

MEMORY 里记的"返回时 1–2s 横屏窄带（记账未做）"就是这个问题的轻量版；本轮测到的是 5–6s，
且会完整呈现横屏布局，严重度比台账记录的高。

### 修复（`MainActivity.onCreate`，6 行）
```kotlin
// 手机端主界面锁竖屏；平板（sw600dp+）保留自由方向（横屏读海报墙更合理）。
if (resources.configuration.smallestScreenWidthDp < 600) {
    requestedOrientation = android.content.pm.ActivityInfo.SCREEN_ORIENTATION_PORTRAIT
}
```
不锁平板是有意的：平板横屏看海报墙是合理用法，不能为手机的问题一刀切。

## 四、结论 C：续播功能正常，但没用上更快的 `start=` 预置（性能项，非缺陷）

冒烟时怀疑"继续观看点进去没从断点续播"，**实测证伪**：

- 点击前：首页显示 `看到 8:32`；
- 播 2 分 18 秒后退出：首页变成 **`看到 10:50`**（8:32 + 2:18）—— 证明起播确实从 8:32 开始；
- 机制：`PlayerSmbStart` 首帧后走 `applyResumePosition()` → `seekTo(pos)` 兜底路径。

**但**：`MpvPlaybackSession.start(surface, onReady, startSeconds)` 与 `MpvStartSequence.prepare(startSeconds)`
→ `native.loadFile(url, startSeconds)` 这条"内核打开即落位"的快路径**没被调用**——
`PlayerSmbStart.kt:173` 漏传第 3 个参数（默认 -1.0）。源码注释自己写着"①快得多（5.15s → 4.39s）"。

- 性质：**性能优化遗漏，不是功能缺陷**（续播结果正确）；
- 处置：本轮不改（改它要另跑一轮起播验证，且与本次方向修复混在一起会污染归因）；
- 记账：列为遗留优化项，收益约 0.5–0.8s 起播时间。

## 五、回归验证

| 项 | 结果 |
|---|---|
| `assembleDebug` | BUILD SUCCESSFUL |
| `testDebugUnitTest --rerun-tasks` | 115 例 / 0 失败 / 0 错误 |
| `tools/ui_gate.py` | PASS（MainActivity 151 行，≤300） |
| 真机方向 | 返回后 **1s 内竖屏**（修复前 5–6s） |
| 真机内容 | 返回后首页 `继续观看 + A Beautiful Mind 10:53 + 电影行` 完整 |

## 六、诚实边界

- 冒烟用 UI 点击导航（`input tap`），因为详情页/Emby 浏览页无对外 intent 入口；每次点击前都用
  `uiautomator dump` 重新取坐标，未依赖固定坐标（装包后重启导致过一次坐标失效，已纠正）。
- 只测 1 台 K40（b6e950ee）；平板（sw600dp+）分支**未验**，代码按 `smallestScreenWidthDp < 600` 保守放行。
- Emby 起播只验了 1 集；未跑长稳（D358/p282 等已有 ≥240s 结论，不重复）。
- 横屏残留"5–6s"是单次受控复测值；不同设备/系统版本可能不同，但"锁竖屏"从机制上消除该竞态，不依赖具体秒数。
