# 371 · 聚合搜索卡片海报灰块：取证定位（关 #161）

日期：2026-11-18

> 结论先行：灰块 **= 源站本身无 Primary 海报图**（`ImageTags.Primary` 为空），
> 不是我方取图失败、不是限流、不是解析 bug、不是客户端缺陷。**#161 关闭，无需改代码。**

---

## 一、现象与判据（承 D370 §三 #161）

- 现象：聚合搜索 180 条结果，部分卡片是灰底占位（截图 `docs/evidence/ui-shot/aggregate-posters-20261118.png`）：
  左列「兰香如故」「折月亮」有真海报，右列「罪恶拼图」「大夏奇闻诡案录」是灰块。
- D370 判据：区分「源无图 / 取图失败 / 限流」三种责任方，靠 NetDiag 失败原因 + 同一 item 在源站原始数据是否有图。

## 二、取证手段（不改业务逻辑，只加诊断日志 + 读既有缓存）

1. **诊断日志**：`AggregatePoster.kt` 的 `LaunchedEffect` 三处早退各补一条 `NetDiag.log`，
   前缀 `[agg-poster]`，三类：`blank`(url 空) / `fail`(取图异常) / `decode-null`(字节非图)。
   MIUI 下 `Log.d` 抓不到，诊断一律走 App 自有 `files/net-diag.log`。
2. **源站原文**：D367 的列表缓存 `cache/emby-json-v1/` 存了源站 `/Items` 的原始 JSON body
   （`.key` 伴生文件记录 URL）。直接按条目名定位缓存文件，解析 `ImageTags` 字段——
   这是"同一 item 在源站是否有图"最直接证据，比在另一个客户端肉眼看更硬。

## 三、证据

### 3.1 NetDiag 三类计数（K40，聚合页停留期间）

| 类别 | 条数 | 原文 |
|---|---|---|
| blank | 2 | `[agg-poster] blank nl2.lovehd.de`（时间戳 1795006077645/646） |
| fail | **0** | 无 |
| decode-null | **0** | 无 |

→ 没有任何网络异常、没有拿到字节解不出图。**全部灰块都因 url 为空**，即 `posterUrl` 空串。

### 3.2 源站原始 JSON（缓存文件 `d00e37…ef`，URL = lovehd `/Items?…&SearchTerm=`）

| 条目 | ImageTags keys | Primary | BackdropTags | 卡片 |
|---|---|---|---|---|
| 罪恶拼图.Zui.E.Pin.Tu | `[]` | `None` | 0 | 灰块 ✅吻合 |
| 大夏奇闻诡案录.Da.Xia.Qi.Wen.Gui.An.Lu | `[]` | `None` | 0 | 灰块 ✅吻合 |
| 折月亮（对照，有图） | `['Primary','Logo']` | 有 tag | 1 | 真海报 ✅吻合 |

整页统计：**60 条中 28 条无 Primary**（约 47%）——lovehd 源本身大量条目没刮削海报。

### 3.3 代码链路核对（排除我方 bug）

- `EmbyClient.parseItem`：`posterUrl` 仅当 `ImageTags.Primary` 非空白才拼 URL，否则空串。
- `items()` 的 `Fields` 虽未显式列 `ImageTags`，但 Emby `/Items` 默认就返回该基础字段
  （对照「折月亮」同请求同字段拿到了 Primary+Logo+Backdrop，证明字段request没问题）。
- `AggregatePoster`：url 空 → 中性占位（设计如此，不放假图）。

三处一致：源站没给 Primary → posterUrl 空 → 灰块。**链路正确，无可修之处。**

## 四、裁决

- **不改代码**：给无图条目造兜底图会"放假图"，违反单一职责与设计意图；TMDB 兜底是另一个需求
  （聚合结果多为中文剧名 + 拼音串如 `.Zui.E.Pin.Tu`，TMDB 也难命中，且会引入新网络抖动），不在本取证范围。
- **保留诊断日志**：`[agg-poster]` 三类留痕只增 NetDiag 文件一行，无视觉/性能影响，后续若出现
  "有 url 却灰块"能立刻区分 fail/decode-null，值得留着。
- **#161 关闭**；D370 §三 的"下一轮建议先做 #161"到此完成。

## 五、诚实标注

- 未改任何业务逻辑；仅新增诊断日志（已随本机装机验证产出上述 2 条 blank）。
- 源站原文取自 D367 既有磁盘缓存（时间戳 20:44–20:47，与截图同一时段），非新发请求。
- lovehd 服务器近期不稳定（D358 记录其自身在崩），本次只读取已缓存响应，未依赖其在线。
