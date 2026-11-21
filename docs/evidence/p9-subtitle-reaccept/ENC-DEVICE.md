# W3-R1 · 字幕编码自动归一化 · 真机取证（2026-11-22）

团队批次 `subtitle-encoding`（T1/T2），编译绿，覆盖安装 `Success`。

## 1. 结论先行：**正确**，且零内核风险

GBK 编码的 `.srt` 通过**普通导入**（URL / SAF 同一条 Store 路径）后，字幕**正确显示**
「中文字幕 GBK 转码测试」，并且**同内容不同编码会去重成一条**。

```
1795219450796 sub-enc converted codec=GB18030 bytes=62
1795219450850 sub-import ok: …/8d074d742276-gbk.srt deduped=false size=62
1795219450850 sub-remote ok: 8d074d742276-gbk.srt
— 再导入同内容的 UTF-8 版 —
1795219485875 sub-import ok: …/8d074d742276-gbk.srt deduped=true size=62
```

截图 `ENC-01-gbk-import.png`：底部字幕逐字确认「中文字幕 GBK 转码测试」。
`files/subtitles/` 里只有 `8d074d742276-gbk.srt` 一条。

## 2. 为什么不是 iconv（W2 的 12+4 轮结论）

| 路线 | 结果 |
| --- | --- |
| bionic iconv（抬 minSdk 28 + 补丁 + 重编 libmpv） | 内核 `features` 里**确实出现 iconv**，但 `iconv_open` 对 `GBK` / `GB18030` / `GB2312` / `EUC-CN` **全部失败**（`lavf: Error opening iconv with codepage …`）⇒ **Android 的 bionic iconv 不含中文字符集** |
| `sub-codepage` 运行期注入 | 属性写 ok=false；per-file option 不作用于外部字幕；`options/` 写入 ok 但无效；mpv 没有 `sub-reload` 命令 |
| **Java `Charset`（本方案）** | **一次成功**，且不需要 iconv / 不抬 minSdk / 不重编内核 |

W2 的内核改动**已全部回滚**（`libmpv.so` 恢复 `libmpv.so.orig-26`、`minSdk=26`、`KIVO_API=26`、cross 26、脚本去 iconv），
只留下 `patch-mpv-iconv-android.py` 作为"为什么不走这条路"的留档。

## 3. 实现

| 文件 | 职责 |
| --- | --- |
| `playback/subtitle/SubtitleEncoding.kt`（新） | 纯字节归一化：`UTF-8`(严格) → `UTF-16`(BOM) → `GB18030` → `BIG5`；全失败返回原字节 + `"unknown"`（**不抛、不返回 null**） |
| `playback/subtitle/SubtitleImportStore.kt` | 在**落临时文件之后、算 hash 之前**调用归一化并覆盖写回 ⇒ 去重口径自然变成"UTF-8 内容的哈希"；两条入口（SAF / URL）共用 |
| `PlayerSubtitleImport.kt` / `SubtitlePanelActions.kt` | 构造 Store 时注入 `{ smbTrace(it) }`（**改掉 `android.util.Log`** —— MIUI logcat 读不到，等于没有证据） |

## 4. 边界（不夸大）

1. 只验了 **URL 导入**这条入口；SAF 走同一个 `SubtitleImportStore.import`，代码路径等价但**没有单独真机点文件选择器**。
2. 编码顺序固定（UTF-8 → UTF-16 BOM → GB18030 → BIG5）。**没有**测"一段真 GBK 但恰好是合法 UTF-8 字节序"的病态文件（概率极低）。
3. `unknown`（四种都不成立）分支只做了代码路径核对，未构造真实"坏字节"文件验证 trace。
4. 未做单元测试（`SubtitleEncoding` 是纯函数，理论上很适合，但本轮没加）。

## 5. 收尾

测试夹具（8900 服务）已停、临时防火墙规则已删、设备临时截图已清；未 commit / push。
