# O2 · M4 剩余场景「主 A + 次 B 时删 A」· 真机取证（2026-11-21）

- 设备：Redmi K40 `b6e950ee`（未对 `dd137cf7` 执行任何 adb；未 `uninstall`）。
- 被测 APK：编译 2026-11-21 02:53，安装 `Success`，`lastUpdateTime=2026-11-21 02:53:40`
  （`O2-00-install.log`）。
- 目的：回答 p9 家族树 §6.6 登记的观察项 O2 —— **主轨 A 与次轨 B 同时挂着时删掉 A，
  内核会不会自选到 B**；若会，现有兜底（`firstOrNull { it.id != trackId && it.id != secondaryId }`）
  会跳过，是否需要扩展判定。

## 1. 同时落地的埋点迁移（前置）

| 项 | 改动 |
| --- | --- |
| 删 | `playback/MpvSubtitleControl.kt` 的 `Log.i("KivoSub", …)` 与 `import android.util.Log` |
| 增 | `ui/compose/player/subtitle/SubtitlePanelActions.kt` 的 `sub-remove ok` trace 补 `sid=` / `sec=` |

理由两条：① 本机 MIUI `logcat -d` 恒 0 行（§6.6 已登记），`android.util.Log` 在真机读不到；
② `playback` 层不该反向依赖 `ui.compose` 的 `smbTrace`。迁移后全仓 `KivoSub` 只剩无关的
`KivoSubtitle` TAG，`grep -rn "KivoSub\"" android/app/src/main/java` 零命中。

## 2. 场景构造（不依赖 SAF）

| 轨 | 来源 | 说明 |
| --- | --- | --- |
| A（主） | 同名外挂 `O2-fixture-embed.srt` | mpv `sub-auto=exact` 自动挂载，`(subrip) [external]`，id=2 |
| B（次） | 片源内嵌 `mov_text` 轨 | ffmpeg 生成 `O2-fixture-embed.mp4`（640×360 / 20 s / 25 fps / 纯灰 + 内嵌字幕），id=1 |

片源与夹具均留在本目录：`O2-fixture-embed.mp4`（10 871 B）、`O2-fixture-A.srt`（42 B）。

> **为什么不用「同名 .ass + .srt 两条外挂」**：本机实测 `sub-auto=exact` 只挂上了 `.srt`
> （`O2-01-external-list.png` 外挂列表仅一行，内核 `● Subs --sid=1 'kivo-panel-mid.srt'`），
> 同目录同名 `.ass` 未被加入轨道表 ⇒ 改用「内嵌轨 + 同名外挂」构造主/次两条，不依赖 SAF。

## 3. 操作与读数

1. 起播：`--es kivo.media /sdcard/Movies/Kivo/O2-fixture-embed.mp4 --ei kivo.seekSec 2 --ez kivo.freeze true`
2. 字幕卡 →「次字幕」→ 选「字幕 #1 / mov_text」：内核 `Set property: secondary-sid="1" -> 1`
3. 字幕卡 →「外挂字幕」→ 长按 `O2-fixture-embed.srt` → 对话框「删除外挂字幕」→ 点「删除」

**删除前内核状态**（`O2-02-kernel.log`）：

```
[ 0.098] ○ Subs --sid=1 (mov_text) [default]
[ 0.098] ● Subs --sid=2 'O2-fixture-embed.srt' (subrip) [external]
```

**删除后内核原文**：

```
[ 95.245] Run command: sub-remove, flags=73, args=[id="2"]
[ 95.246] Track removed:
[ 95.246]  ○ Subs --sid=1 (mov_text) [default]
[ 95.276]  ● Subs --sid=1 (mov_text [Advanced Sub Station Alpha]) [default]
[ 95.276] Set property: sid="1" -> 1
```

**应用侧 trace（smbTrace → `files/evidence/p220-status.log`）**：

```
1795201158716 sub-remove ok id=2 shared=1 file=null disk=false sid=1 sec=no warn=null
```

## 4. 结论

**内核确实自选到了原次轨（B，id=1），并在同一动作里把 `secondary-sid` 清成 `no`。**
因此不存在「主次同轨叠印」；现有兜底（`current == trackId` 才动作，此时 current=1 ≠ 2）跳过它无碍。

**O2 裁定：维持 §6.6 的「不扩展兜底」，并补上第三组实测** ——
此前 §6.6 只有「删次轨 B」「删唯一主轨」两组；本组是「主 A + 次 B 时删 A」。
`sid=1 sec=no` 是判定依据，不是推断。

## 5. 未取证边界（不夸大）

1. **画面未作为判据**：删除后截图 `O2-10-removed.png` 的播放区没有字幕文字
   （内嵌 cue 为 0–3 s、freeze 点 2 s；`mov_text → ASS` 转换后的实际渲染未细查）。
   判定全部来自内核 track 表 + trace 的 `sec=no`，不依赖画面。
2. 只测「删主轨」这一种顺序（与 O2 定义一致）；「删次轨」§6.6 已测过。
3. 单一片源/单一封装（mp4 / mov_text + srt）；不同封装的内嵌轨自选行为未测。

## 6. 收尾

设备夹具已删：`/sdcard/Movies/Kivo/{kivo-panel-mid.ass,kivo-panel-mid.srt,O2-fixture-embed.mp4,O2-fixture-embed.srt}`、
`/sdcard/Download/o2-A.ass`、临时截图。未 commit / push。