# Kivo 字幕修复轮 · 真机复验（ROUND 2 · 收敛范围）

- 日期：2026-11-21（设备本地时间 02:00–02:22）
- 设备：Redmi K40 `b6e950ee`（只使用该设备；未对 `dd137cf7` 执行任何 adb 命令）
- 被测 APK：`android/app/build/outputs/apk/debug/app-debug.apk`，2026-11-21 01:55:59 (+0800)，67,880,299 字节（安装前核对，无需编译）
- 说明：本文件只记录实测证据（截图像素读数、`p220-status.log` / `mpv-kernel.log` 原文、`run-as` 文件与 SharedPreferences 清单）。无法构造/无法取证的项如实标注。

---

## 0. 安装证据

命令（Git Bash，`MSYS_NO_PATHCONV=1`）：

```
C:/Android/sdk/platform-tools/adb.exe -s b6e950ee install -r android/app/build/outputs/apk/debug/app-debug.apk
```

安装输出原文（`R2-00-install.log`）：

```
Performing Streamed Install
Success
```

`dumpsys package com.kivo.cinema`（`R2-00-package.txt`）关键行：

```
versionName=0.1.0-p12a
lastUpdateTime=2026-11-21 02:00:50
firstInstallTime=2026-11-10 18:25:08
```

判定：`lastUpdateTime` 变为今天、`firstInstallTime` 保持 2026-11-10 18:25:08 ⇒ 覆盖安装成功，非重装。未执行 `adb uninstall`。

---

## 1. 取证方法与口径

- 启动契约：
  `am start -S -n com.kivo.cinema/.ui.compose.PlayerComposeActivity --es kivo.media <path> --ei kivo.seekSec 2 --ez kivo.freeze true`
- 控制条 3 秒自动隐藏：所有 tap/swipe/screencap 均在同一条 `adb shell` 链里完成（`tap; sleep 0.3; tap; sleep 0.6; screencap`）。若控制条已隐藏，第一次 tap 只用于唤出，第二次 tap 才命中图标。
- 像素一律用 Python/PIL 直接采样（Read 图片工具存在缓存错位，不用于读数）。
- 应用私有目录：`run-as com.kivo.cinema ls -l files/subtitles/`、`run-as com.kivo.cinema cat shared_prefs/subtitle_imports.xml`。
- 埋点日志：`files/evidence/p220-status.log`（`sub-import` / `sub-remove` / `shared=` / `panelDrill`），内核 `files/mpv-kernel.log`（`grep -a`）。
- 片源：`/sdcard/Movies/Kivo/kivo-panel-{bright,mid,dark}.mp4`（均无内嵌字幕轨）。

已知设备限制（影响取证方式，非产品缺陷）：

1. **logcat 不可用**：`logcat -d` 输出 0 行；实时 `logcat -s KivoSub:*` 流 60 s 为 0 字节。MIUI 未保留应用日志，因此 `KivoSub` 行（M4）无法从设备读出（见 §9）。
2. `uiautomator dump` 对播放器面板返回陈旧/旋转错位节点，行定位全部用「代码行顺序 + 像素几何」交叉确认。

---

## 2. 逐项结论

| # | 项 | 操作（坐标/链路） | 实测证据 | 期望 | 判定 |
|---|---|---|---|---|---|
| 0 | 前提取证：`external-filename` 身份 | 同内容换名导入（dup-b→已有 dup-a 轨），观察复用分支 | trace：`sub-import reuse: id=1 selected=true closedSecondary=false path=b8a03b8d6af9-dup-a.srt`；内核**没有**第二次 `sub-add`；全量 trace 中 `dedupe unavailable` 计数 = 0 | 非空、canonical 后相等 | **通过（间接）**（无属性读取钩子，见备注） |
| 1 | M1 · 同内容改名复用 | 在 mid 片源导入 dup-b.srt（内容与已加载 dup-a 相同） | `sub-import ok: …/b8a03b8d6af9-dup-a.srt deduped=true size=39` → 两次 `sub-import reuse: id=1 selected=true closedSecondary=false`；`files/subtitles/` 始终 5 个文件，`b8a03…dup-a.srt` mtime 保持 `2026-11-21 00:42`（未重新拷贝）；内核仅 1 条 `● Subs --sid=1 'dup-a.srt' [external]` | 文件数不增、外挂轨数不增、复用 trace、toast「字幕已导入并启用」 | **PASS** |
| 2 | R1 · 复用分支主/次互斥 | 次字幕列表选 dup-a（id=1）→ 再导入同一文件 | 选次字幕：内核 `315.748 Set property: secondary-sid="1" -> 1`；再导入：trace `sub-import reuse: id=1 selected=true closedSecondary=true path=b8a03b8d6af9-dup-a.srt`；内核 `366.133 Set property: secondary-sid="no" -> 1`、`366.159 Set property: sid="1" -> 1`（无 `sid == secondary-sid` 时刻）；面板「次字幕：关闭」+ 画面无叠印（R2-01/R2-02） | `closedSecondary=true`、sid=id 且 secondary-sid=no | **PASS** |
| 3a | M2+M4 · 主轨删除（唯一文件） | mid 片源导入 `uniq-m4.srt`（新内容，仅本片源登记）→ 外挂列表长按 → 「删除」 | trace `sub-remove ok id=1 shared=1 file=140d8ab92be0-uniq-m4.srt disk=true warn=null`；`files/subtitles/` 该文件消失；`subtitle_imports.xml` 中 `kivo-panel-mid.mp4` 键整体消失；内核 `694.830 sub-remove id="1"` → `Track removed` → `694.842 Set property: sid="no"`（该片源无内嵌轨可回落）；toast「已删除外挂字幕与导入文件」（R2-08） | `disk=true`、文件消失、登记清除、sid 回落/关闭 | **PASS** |
| 3a′ | 同上（跨片源共享文件） | mid 片源删除 dup-a（bright 仍登记同一文件） | trace `sub-remove ok id=1 shared=1 file=b8a03b8d6af9-dup-a.srt disk=false warn=null`；文件保留；mid 键清除；toast「已移除外挂字幕」 | — | **预期保护行为**（见 3c） |
| 3b | M2+M4 · 删次轨后的 secondary-sid | 导入 A=`uniq-m4.srt`（主，`573.942 sid="1"`）、B=`demo.srt`（次，`629.968 secondary-sid="2"`）→ 删除 B(id=2) | trace `sub-remove ok id=2 shared=1 file=46c701361183-demo.srt disk=false warn=null`；内核 `650.208 sub-remove id="2"` → `Track removed`；之后无 secondary-sid 写入；重新打开「次字幕」列表：**「关闭次字幕（当前未启用）」为勾选态**（R2-09）⇒ 回读无残留旧 id | 无残留/无异常 | **PASS**（`KivoSub` 行缺失，见 §9-1） |
| 3c | 多引用保护（同文件多轨） | — | 无法构造：同内容再次导入必走复用分支（§1 实测），UI 不产生第二条指向同一文件的轨 | shared>1 不删盘 | **未取证**（原因如上） |
| 4 | L1 · 重置 | 显示设置：字号 62→64、垂直位置 100→99、底部边距 40→41、文字颜色→暖白，随后点「重置」 | 改前内核：`742.313/742.608 sub-font-size="64"`、`742.905 sub-pos="99"`、`743.215 sub-margin-y="41"`、`755.893 sub-color="#FFFFF1D0"`。重置后：`768.262 sub-font-size="40"`、`768.263 sub-pos="100"`、`768.263 sub-margin-y="40"`、`768.264 sub-border-size="3.0"`、`768.264 sub-color="#FFFFFFFF"`、`768.265 secondary-sub-pos="15"`；`subtitle_prefs.xml` 同步为 `font_size=40 / pos=100 / margin_y=40 / color=#FFFFFFFF`；面板行立即刷新为 40 / 100% / 40、「文字颜色 白色」，toast「字幕样式已恢复默认」（暂停冻结态下测量，R2-10/11/12） | 内核回读 40/100/40/默认色，prefs 同步，行刷新 | **PASS** |
| 4b | 重置与步进并发注入 | — | 无并发注入通道 | — | **未取证**（不可构造） |
| 5 | L5 · 文案 | 显示设置 → 微调 → 「底部边距」行 | 副题「仅文本字幕」完整显示；像素测量：两行文本带 y=489–578（90 px ≈32.7 dp），行节距 176 px = 64 dp，无裁剪、无压线（R2-13） | 副题=仅文本字幕，64dp 行内无裁剪 | **PASS** |
| 6 | 面板 #4 · 按压圆 | 暗场 `kivo-panel-dark.mp4`；步进「底部边距 +」按住 0.4 s；控制栏「倍速」按住 0.4 s；顶栏 pip 图标按住 0.4 s | 步进按下圆心 (59,62,67)/(58,62,66) ≈ 12%（对照期望 60,64,67）；松开 (32,36,40) ≈ 7%（对照 30,34,38）。控制栏文字钮在纯黑区按下 (20,20,20) = 8% 白（非 31）。顶栏图标：背景 (17,17,17) → 按下 (28,28,28)，Δ≈11（≈8%×0.62，容器整体 alpha 所致，非 12%）。截图：R2-14/14b/15/15b/16/16b/17 | 步进 ≈12%；控制栏 8%（≈20），若 ≈31 则 FAIL | **PASS** |
| 7 | 失败注入类 | — | 无故障注入钩子/无法构造 | — | **未取证** |

---

## 3. 关键日志原文

### 3.1 复用与 R1（`p220-status.log`，`R2-00-trace.log` 第 190–205 行）

```
1795197889165 sub-import: launching SAF picker
1795197901131 sub-import ok: /data/user/0/com.kivo.cinema/files/subtitles/b8a03b8d6af9-dup-a.srt deduped=true size=39
1795197916243 sub-import reuse: id=1 selected=true closedSecondary=false path=b8a03b8d6af9-dup-a.srt
1795197964809 sub-import reuse: id=1 selected=true closedSecondary=false path=b8a03b8d6af9-dup-a.srt
1795198208930 sub-import reuse: id=1 selected=true closedSecondary=true path=b8a03b8d6af9-dup-a.srt
```

### 3.2 内核字幕属性（`mpv-kernel.log`，`R2-00-mpv-kernel.log`）

```
[   0.012][v][cplayer] Set property: sub-font-size="62" -> 1
[  58.359][i][cplayer]  ● Subs   --sid=1  'dup-a.srt' (subrip [Advanced Sub Station Alpha]) [external]
[  73.472][v][cplayer] Set property: sid="1" -> 1
[ 122.040][v][cplayer] Set property: sid="1" -> 1
[ 149.093][v][cplayer] Set property: sid="no" -> 1
[ 315.748][v][cplayer] Set property: secondary-sid="1" -> 1
[ 366.133][v][cplayer] Set property: secondary-sid="no" -> 1
[ 366.159][v][cplayer] Set property: sid="1" -> 1
[ 470.050][d][cplayer] Run command: sub-remove, flags=73, args=[id="1"]
[ 470.051][i][cplayer] Track removed:
[ 470.051][v][cplayer] Set property: sid="no" -> 1
[ 523.128][d][cplayer] Run command: sub-add, flags=73, args=[url="/data/user/0/com.kivo.cinema/files/subtitles/140d8ab92be0-uniq-m4.srt", flags="select", title="uniq-m4.srt", lang=""]
[ 559.133][d][cplayer] Run command: sub-add, flags=73, args=[url="/data/user/0/com.kivo.cinema/files/subtitles/46c701361183-demo.srt", flags="select", title="demo.srt", lang=""]
[ 573.942][v][cplayer] Set property: sid="1" -> 1
[ 629.968][v][cplayer] Set property: secondary-sid="2" -> 1
[ 650.208][d][cplayer] Run command: sub-remove, flags=73, args=[id="2"]
[ 650.211][i][cplayer] Track removed:
[ 694.830][d][cplayer] Run command: sub-remove, flags=73, args=[id="1"]
[ 694.838][i][cplayer] Track removed:
[ 694.842][v][cplayer] Set property: sid="no" -> 1
[ 768.262][v][cplayer] Set property: sub-font-size="40" -> 1
[ 768.263][v][cplayer] Set property: sub-pos="100" -> 1
[ 768.263][v][cplayer] Set property: sub-margin-y="40" -> 1
[ 768.264][v][cplayer] Set property: sub-border-size="3.0" -> 1
[ 768.264][v][cplayer] Set property: sub-color="#FFFFFFFF" -> 1
[ 768.265][v][cplayer] Set property: secondary-sub-pos="15" -> 1
```

### 3.3 `sub-remove` 应用侧（`p220-status.log`）

```
1795198312823 sub-remove ok id=1 shared=1 file=b8a03b8d6af9-dup-a.srt disk=false warn=null
1795198492983 sub-remove ok id=2 shared=1 file=46c701361183-demo.srt disk=false warn=null
1795198537612 sub-remove ok id=1 shared=1 file=140d8ab92be0-uniq-m4.srt disk=true warn=null
1795198864221 sub-remove ok id=1 shared=1 file=b8a03b8d6af9-dup-a.srt disk=false warn=null
```

（`KivoSub` 行期望为 `sub-remove id=…; sid=… secondary-sid=…`，本机无法读取，见 §9-1；同一代码路径的 trace `sub-remove ok` 与内核 `Run command: sub-remove` 已作为等价证据。）

### 3.4 像素读数（完整列表见 `R2-pixel-samples.txt`）

```
stepper-pressed-dark  2248,520  (59,62,67)      stepper-released-dark 2248,520 (32,36,40)
stepper-pressed-dark  2228,536  (59,62,67)      stepper-released-dark 2228,536 (32,36,40)
bar-press-8pct        2313,960  (20,20,20)      bar-press-8pct bg     2250,960 (0,0,0)
topbar-pip-pressed    1352,150  (28,28,28)      topbar-pip-released   1352,150 (17,17,17)
```

---

## 4. 文件与登记清单（M1 / M2 前后）

`files/subtitles/`（M1 前后一致，5 个文件；dup-a 时间戳未被改写）：

```
-rw------- 40  2026-11-21 01:00 46c701361183-demo.srt
-rw------- 830 2026-11-20 00:43 8f1ffc9e05bf-QQ-望星空-卓依婷-surround.lrc
-rw------- 40  2026-11-21 00:57 931e71778c4f-demo.srt
-rw------- 39  2026-11-21 00:42 b8a03b8d6af9-dup-a.srt
-rw------- 104 2026-11-20 01:28 e1bab4f75375-kivo-import-test.srt
```

删除后：`140d8ab92be0-uniq-m4.srt` 已消失；`subtitle_imports.xml` 中 `file:/sdcard/Movies/Kivo/kivo-panel-mid.mp4` 键随 3a 删除后整体消失；`kivo-panel-dark.mp4` 恢复为原来的两条 demo 登记；`b8a03…dup-a.srt` 因 `kivo-panel-bright.mp4` 仍登记而保留（`disk=false`）。

---

## 5. 截图 / 日志文件清单

| 文件 | 内容 |
|---|---|
| `R2-00-install.log` | adb install 原文（Success） |
| `R2-00-package.txt` | dumpsys package（lastUpdateTime / firstInstallTime） |
| `R2-00-trace.log` | p220-status.log 全量（含 M1/R1/M2 埋点） |
| `R2-00-mpv-kernel.log` | mpv-kernel.log 全量 |
| `R2-00-prefs-subtitle_imports.xml.txt` | 导入登记表（测试结束时） |
| `R2-00-prefs-subtitle_prefs.xml.txt` | 字幕样式 prefs（测试收尾再次重置后的默认值） |
| `R2-00-files-subtitles.txt` | `files/subtitles/` 收尾清单（5 个文件） |
| `R2-01-reuse-panel-secondary-closed.png` | R1 后：外挂=dup-a 主轨，次字幕=关闭 |
| `R2-02-reuse-no-duplicate-overlay.png` | R1 后画面（无叠印） |
| `R2-03-secondary-list-uniq-main.png` | 次字幕列表（主轨置灰「不能与主字幕相同」） |
| `R2-04-toast-imported-enabled.png` | 新构建 toast「字幕已导入并启用」（成功分支截图） |
| `R2-05-external-list-A-B.png` | 外挂列表 A(uniq)/B(demo) |
| `R2-06-delete-dialog-dup-a.png` | 长按删除确认框（dup-a） |
| `R2-07-delete-toast-removed.png` | 删除后 toast「已移除外挂字幕」+ 列表空 |
| `R2-08-delete-A-disk-true.png` | 唯一文件删除：toast「已删除外挂字幕与导入文件」，列表「无外挂字幕」 |
| `R2-09-secondary-after-delete-B.png` | 删次轨 B 后：次字幕列表「关闭次字幕（当前未启用）」勾选 |
| `R2-10-reset-before-changed.png` | 重置前：64 / 99% / 41 |
| `R2-11-reset-toast-default.png` | 重置 toast「字幕样式已恢复默认」+ 文字颜色 白色 |
| `R2-12-reset-after-values.png` | 重置后：40 / 100% / 40 |
| `R2-13-L5-margin-row.png` | 「底部边距 / 仅文本字幕」行放大 |
| `R2-14(-b)-bar-press-8pct*.png` | 控制栏「倍速」按住，圆心 (20,20,20) |
| `R2-15(-b)-stepper-pressed-dark*.png` | 步进按住（无暗场片源）圆心 ≈(59,62,67) |
| `R2-16(-b)-stepper-released*.png` | 步进松开圆心 ≈(32,36,40) |
| `R2-17-topbar-pip-pressed.png` | 顶栏图标按住（Δ≈11，容器 alpha 0.62） |
| `R2-18-delete-dialog-dup-dark.png` | dark 片源删除确认框 |
| `R2-pixel-samples.txt` | 全部像素读数（PIL 采样） |

---

## 6. 未取证项与原因

1. **`KivoSub` M4 日志行**：设备 `logcat -d` 输出 0 行，实时流 0 字节（MIUI 不保留应用日志），无法读出 `sub-remove id=…; sid=… secondary-sid=…`。替代证据：trace `sub-remove ok id=… shared=… file=… disk=…` + 内核 `Run command: sub-remove` / `Track removed`。建议 M4 埋点改为写入 `p220-status.log`（smbTrace），否则 M4 在真机上不可取证。
2. **3c 多引用保护（同一文件多轨）**：同内容导入必走复用分支（M1 已实测），UI 无法构造 shared>1 的两条轨。跨片源引用保护（同一文件被不同片源登记）已实测两次：`disk=false`、文件保留、仅清当前片源登记。
3. **失败注入类（7）**：无故障注入入口，无法构造，未取证。
4. **重置与步进并发注入（4b）**：无并发注入通道，未取证。

## 7. 观察项（不属于本轮验收判定，供负责人排期）

1. **次字幕面板锚点**：`PlayerPanelHost.anchorFor` 未包含 `"subtitleSecondary"`，该子面板回落到 `PlayerPanelAnchor.Center`，实际渲染在屏幕居中，而非字幕图标下方（R1 过程中截图可见：面板出现在画面中央 860–1540 px）。
2. **删除次轨后内核未显式写 `secondary-sid=no`**：`sub-remove id=2` 后内核日志没有该属性写入行；但 UI 回读为「关闭次字幕（当前未启用）」，未见残留异常。若要更硬证据，可增加属性读取钩子或在删除路径显式补写。
3. **顶栏图标按压圆受容器 alpha 影响**：`TopIconButton` 对整个 Box 施加 `barInactiveAlpha=0.62`，8% 按压圆合成后约 5%（黑底约 12–13）。底栏文字钮为精确 8%（20）。若设计口径要求顶栏也贴合 8%，需要把 PressIndicator 移出 alpha 容器。
4. **长按底栏播放键位置会进入「已锁定屏幕」浮层**（重新 `am start -S` 可恢复）。面板 #4 的按压测量因此改到「倍速」文字钮与顶栏图标上完成，仍在纯黑区/暗场片源上取样。

## 8. 合规确认

- 只用 `b6e950ee`；未对 `dd137cf7` 执行任何 adb 命令。
- 未执行 `adb uninstall`；覆盖安装成功，`firstInstallTime` 未变。
- 未修改任何源码；只新增 `docs/evidence/p9-subtitle-reaccept/` 下本文件与证据文件（`git status --porcelain` 仍为 127 项，与进场一致）。
- 未 commit / 未 push / 未发布。
- 设备侧临时资源已清理：`/sdcard` 下的本轮截图与 xml、`/sdcard/Movies/Kivo/uniq-m4.srt`、`/sdcard/Download/{uniq-m4.srt,demo-b.srt}` 已删除；`svc power stayon` 已恢复 `false`；字幕样式在面板 #4 长按步进（自动连发，font 46 / margin 47）后已再次「重置」回默认（40 / 100 / 40 / #FFFFFFFF）。
