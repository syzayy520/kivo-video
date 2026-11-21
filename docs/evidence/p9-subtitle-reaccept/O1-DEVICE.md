# O1 · 次字幕面板锚点 · 真机验收（2026-11-21）

- 设备：Redmi K40 `b6e950ee`（未对 `dd137cf7` 执行任何 adb）。横屏 2400×1080 / 440dpi ⇒ `density=2.75`、屏宽 **872.7 dp**。
- 被测 APK：`android/app/build/outputs/apk/debug/app-debug.apk`，编译 2026-11-21 02:43，67,880,299 B。
- 安装原文（`O1-00-install.log`）：`Performing Streamed Install` / `Success`；
  `lastUpdateTime=2026-11-21 02:43:57`、`firstInstallTime=2026-11-10 18:25:08`（未重装、未 `uninstall`）。

## 1. 改动

唯一改动点：`ui/compose/player/PlayerPanelHost.kt` → `anchorFor` 的 `"captions"` 分支补
`"subtitleSecondary"`（文件 63–67 行），与该文件既有口径「钻入子面板自动跟随父面板图标」对齐。

> `O1-01-diff.txt` 是**相对 HEAD** 的全量 diff：该文件在工作区里另有既有在途改动
> （`density()`、`onLongSelect` 形参），**不属于本次 O1**。本次新增仅 captions 分支一项与两行注释。

## 2. 判据与读数

期望值（由 `PlayerTokens` + `PlayerPanelGeometry` 直接算）：

- 顶边 y = `topBarHeight(30+44)` + `anchorGap(8)` = **82 dp = 225.5 px**
- 左缘 x = `clamp(872.7−80−124, 16, 872.7−248−16)` = **608.7 dp = 1674 px**（被右侧 clamp 吃住，既有口径）
- 宽 = `panelWidth` = **248 dp = 682 px**

实测（PIL 灰度：行均值突变找顶边、行内列突变找左右缘）：

| 截图 | 面板 | 顶边 y/px | 顶边 dp | 左缘 x/px | 左缘 dp | 宽/px |
| --- | --- | --- | --- | --- | --- | --- |
| `O1-01-subcard.png` | 字幕卡（`kivo-panel-mid`，无字幕轨） | 226 | 82.18 | 1674 | 608.73 | 682 |
| `O1-02-secondary.png` | 次字幕子面板（空态「无内嵌字幕」） | 226 | 82.18 | 1674 | 608.73 | 682 |
| `O1-03-subcard-multi.png` | 字幕卡（`生化危机…1080p.mp4`） | 229 | 83.27 | — | — | — |
| `O1-04-subcard-withsub.png` | 字幕卡（同名外挂 srt 已挂） | 226 | 82.18 | — | — | — |
| `O1-05-secondary-list.png` | 次字幕子面板（有外挂轨） | 226 | 82.18 | 1674 | 608.73 | 682 |

**判定 PASS**：Δ顶边 0.18 dp（0.5 px）、Δ左缘 0.03 dp（0.07 px）；次字幕面板与「字幕」卡**同锚点**，
且贴顶栏图标行下沿，**不再渲染在屏幕正中**（修复前 `anchorFor` 落 `else → Center`，由 diff 静态证明）。

面板内容核对：`O1-02` 为标题「次字幕」+「无内嵌字幕 / 当前片源没有内嵌字幕轨」；
`O1-05` 为标题「次字幕」+ 一行列表（文本带 y=610–640，面板内仅此一行）。

## 3. 复现命令

```bash
ADB=C:/Android/sdk/platform-tools/adb.exe; export MSYS_NO_PATHCONV=1
# 构造外挂轨（mpv sub-auto=exact 只认同目录同 basename）
$ADB -s b6e950ee push docs/evidence/p9-subtitle-reaccept/O1-fixture.srt /sdcard/Movies/Kivo/kivo-panel-mid.srt
$ADB -s b6e950ee shell 'am start -S -n com.kivo.cinema/.ui.compose.PlayerComposeActivity \
  --es kivo.media /sdcard/Movies/Kivo/kivo-panel-mid.mp4 --ei kivo.seekSec 2 --ez kivo.freeze true'
sleep 7
# 唤出控制条 → 字幕图标(x≈2160,y≈180)
$ADB -s b6e950ee shell "input tap 1200 540; sleep 0.6; input tap 2160 180; sleep 1.0; screencap -p /sdcard/o1-4.png"
# 字幕卡「次字幕」行（y≈880）→ 子面板
$ADB -s b6e950ee shell "input tap 1900 880; sleep 1.2; screencap -p /sdcard/o1-5.png"
```

## 4. 未取证边界（不夸大）

1. **Center 对照未实机回退**：修复前状态由 `anchorFor` 的 `else → Center` 静态证明，没有拍"居中"截图。
2. 两台片源**均无内嵌字幕轨**，完整列表形态用 mpv `sub-auto=exact` 的同名外挂轨构造
   （`O1-fixture.srt`，39 B，1 条 cue）。锚点几何与轨内容无关。
3. 面板底边未逐像素量（非本项判据）。

## 5. 收尾

设备夹具已删：`/sdcard/Movies/Kivo/kivo-panel-mid.srt` 与临时 `o1-*.png`；
`ls /sdcard/Movies/Kivo/` 已回到 6 个片源。未 commit / push / 发布。