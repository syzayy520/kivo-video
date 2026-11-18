# 360 · SMB 大文件 `.ts` 续播黑屏"未定根因"已关闭：**就是 D351 的 Content-Type bug，已顺带修复**

> 长期挂着一条 P0 疑点：`12T/REMUX/A Beautiful Mind…VC-1….ts`（33.8 GB）
> 点播黑屏、`start=3574.582` + `PROXY conn fail: Connection reset` +
> `Failed to recognize file format`，当时"死在 demuxer 之前"，归因落在
> 「SMB 大文件 + 远 seek + 本地代理」组合，三个候选（代理 range / SMB 远偏移 / .ts 无索引）未区分。
>
> 本批（2026-11-18 11:26，K40 真机）用**同一文件、intent 起播、SMB 远 seek** 复验，
> 结论：**该文件已能正确识别为 mpegts、远 seek 成功、持续出画、零丢帧。**
> 根因即 D351 已修的"代理 `Content-Type` 写死 `video/x-matroska`"——
> ffmpeg 被误导只挑 matroska 探测器，才打出 `EBML header parsing failed` →
> `Failed to recognize file format`。该缺陷**已被 D351 顺带修复，只是一直没回归验证**，本批补齐。

---

## 一、结论

| 判据 | 当时（缺陷发现时） | 本批（11:26 构建含 D351） |
|---|---|---|
| 格式识别 | `Failed to recognize file format` | **`Detected file format: mpegts (libavformat)`** |
| `Failed to recognize file format` 次数 | ≥1 | **0** |
| 远 seek | 疑"36GB 偏移读不动" | **`stream level seek … to 36282456560` 成功，`seek done`** |
| 出画 | 黑屏 40s+ | 22→37 条 `[cache]`，dur 0→16.4s，持续推进 |
| 丢帧 | 未测到 | `vo_drop=0` / `decoder_drop=0`（t=51s 健康度） |
| `PROXY conn fail` | 持续、致命 | 13 次**全集中在起播头 9 秒**（11:26:11–17），之后 60s 无 |

⇒ 缺陷不复现。三个旧候选里，"代理 range 处理"（D351 修的就是它）命中；
"SMB 远偏移限制"与".ts 无索引扫全文件"均被证伪（远 seek 到 36GB 偏移成功）。

---

## 二、方法（严格真机，不碰系统 UI）

吸取 D358 设备锁屏事故教训：**全程 intent 起播，不 reboot、不 swipe、不碰通知栏**。

```
# 同源同文件，复刻远 seek 场景（sourceId 指向 NAS 192.168.5.36，凭据从加密 SourceStore 解析）
adb shell am force-stop com.kivo.cinema
adb shell "run-as com.kivo.cinema sh -c ': > files/net-diag.log; : > files/mpv-kernel.log'"
adb shell "am start -n com.kivo.cinema/.ui.compose.PlayerComposeActivity \
  --es kivo.sourceId 55ccf8d3-ccec-4652-b19f-a42d3580e7b3 \
  --es kivo.smbShare 12T \
  --es kivo.smbPath 'REMUX/A Beautiful Mind 2001 BluRay REMUX 1080p VC-1 DTS-HD MA5.1 2Audio-CHD/A Beautiful Mind 2001 BluRay REMUX 1080p VC-1 DTS-HD MA5.1 2Audio-CHD.ts' \
  --es kivo.label d360-ts-seek --ei kivo.seekSec 3574"
```

设备前置：`deviceLocked=0`、飞行模式 off、LAN(192.168.5.36)/WAN 通、apk 为当日 09:09 构建
（含 D351 的 `LocalRangeProxy.kt:136` → `application/octet-stream`）。

---

## 三、诚实边界（不夸大）

1. **`kivo.seekSec=3574` 实际落点是 664.23 s（11:04），不是 3574 s**：日志
   `queuing seek to 664.230000`、`hr-seek, skipping to 64.230000`。这个 intent 参数未把
   播放位置精确设到 3574s（被续播逻辑/TS 无精确索引影响）。
   **但这不削弱结论**：内核完成了 `stream level seek … to 36282456560`（36 GB 字节偏移，
   远超 3574s 对应位置）并 `seek done` ⇒ "SMB 大文件远偏移读取"这一候选已被证伪；
   而"格式识别"判据与 seek 落点无关（读头部即判定）。
2. **13 次 `conn fail` 不是零**：全部在起播头 9 秒，是 TS 探测阶段多次 range 请求的瞬时重置，
   随后持续出画 60s+。记为"起播瞬时抖动"，不写成"零异常"。
3. **本批未重现 3574s 精确落点**，故严格说证明的是"同一 .ts 经 SMB + 远 seek 可正常识别与出画"，
   而非"3574s 那个字节点逐字节复现"。对"Content-Type 根因"这一判定已充分（判据是格式识别）。

---

## 四、对 `originKeepAlive` / 默认值的影响

无。本批是缺陷关闭，不改任何默认值、不改代码（D351 的修复已在代码中）。

---

## 五、证据

| 文件 | 内容 |
|---|---|
| `v360/ts-seek-netdiag.txt` | 完整 net-diag：22→37 条 `[cache]`、13 次起播瞬时 conn fail |
| `v360/ts-seek-health.ndjson` | 健康度：`vo_drop=0` / `decoder_drop=0`，画面推进 |
| `v360/ts-seek-kernel.log`（本地，`*.log` 忽略） | `Detected file format: mpegts`、`seek done`、远 seek 到 36282456560、`Failed to recognize` 0 次 |

判据复算：
```
grep -c 'Detected file format: mpegts' docs/evidence/p403-design/v360/ts-seek-kernel.log   # 1
grep -c 'Failed to recognize file format' docs/evidence/p403-design/v360/ts-seek-kernel.log # 0
grep -c '36282456560' docs/evidence/p403-design/v360/ts-seek-kernel.log                     # 1
```
