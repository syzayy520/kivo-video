# 372 · NAS 片库封面稳定性冷启取证（关 #162）

日期：2026-11-18

> 结论先行：3 次冷启封面命中率 **67%→78%→89% 逐轮收敛**；全部失败均为
> `SocketException: Connection reset`（TMDB 国内图片 CDN 抖动），磁盘缓存+3 次重试机制已生效。
> 偶发灰块是**外部依赖问题，非我方代码缺陷**。按铁律不为外部抖动改代码。**#162 关闭。**

---

## 一、判据与方法（承 D370 §三 #162）

- 判据：连续 3 次冷启首页分类行封面命中率。
- 方法（K40 b6e950ee，设备已解锁）：
  1. 备份 net-diag 日志留底；**第 1 轮清 `cache/nas-poster/*.jpg`** 模拟首次冷启（最坏情况）。
  2. 每轮 `am force-stop` → 清日志窗口 → `am start .MainActivity`（intent 起页，不 reboot/不 swipe）→ 等 30s。
  3. 统计日志各阶段：`start / disk-hit / smb-ok / tmdb / done / fail / net-fail`。
  - 只用 intent + force-stop，遵守"不为 UI 故障 reboot/swipe/keyevent"铁律。

## 二、数据

| 轮次 | 场景 | start | done | disk-hit | fail | net-fail | 命中率 |
|---|---|---|---|---|---|---|---|
| 1 | 清缓存·首次冷启 | 9 | 6 | 0 | **3** | 3 | 6/9 = 67% |
| 2 | 带轮1缓存 | 9 | 1 | 6 | **2** | 2 | 7/9 = 78% |
| 3 | 带轮1+2缓存 | 9 | 1 | 7 | **1** | 1 | 8/9 = 89% |

- 失败原因 100% 是 `nas-poster net-fail SocketException: Connection reset`（无一类是解码失败/解析失败）。
- 失败条目逐轮减少：轮1(犯罪都市4/Dangerous Encounters/69两头勾) → 轮2(犯罪都市4/Dangerous Encounters) → 轮3(犯罪都市4)。
- 对照证据（第 3 轮日志原文）：
  - `nas-poster done Dangerous Encounters of the First Kind bmp=500x758` —— 前两轮失败的片子第 3 轮成功，证明是随机抖动而非 URL 错误。
  - `nas-poster tmdb 犯罪都市4 url=https://image.tmdb.org/t/p/w500/mPaDzheLnyHDE52h48WJuHzotps.jpg` —— URL 有效（D365 曾取到 500x750 真海报），当前 CDN 持续 reset；因第 1 轮清了磁盘缓存，暂未重建，网络恢复后会自动缓存回来。

## 三、裁决

- **不改代码**：现有机制（TMDB 3 次重试 + 8s 超时 + 磁盘缓存成功即永久可用 + Semaphore(3) 限流）已正确覆盖抖动场景；
  命中率随缓存积累单调上升即证明有效。为外部 CDN 抖动加更多重试/备用域名属过度工程，且改了无法在抖动的公网稳定验（违反"改代码不能真机验=不做"）。
- **保留诊断日志**：`nas-poster` 各阶段留痕已在位，无需新增。
- **#162 关闭**；D370 §六 已知边界补充一条。

## 四、诚实标注

- 第 1 轮清除了 `cache/nas-poster/*.jpg`（App 自有可重建缓存，无用户数据丢失）；副作用是「犯罪都市4」因当前 CDN 抖动暂未重建缓存，网络恢复后自动恢复。
- 截图 `nas-162-round{1,2,3}.png` 已归档（hero 区/分类行，视觉以日志命中率为准）。
- 未改任何代码；取证全程只读日志 + intent 起页。

## 五、顺带核对 #160（多季片源真机验证）

- 核心逻辑 `EmbyEpisodeGrouping.group()`（多季分组/空季回退/多余季号）已被 5 个单测锁定；判据日志 `[emby] all-episodes ... seasons=[1,2]` 已在 `EmbyShows.kt` 埋点。
- 真机只差 ≥2 季片源触发：扫描全部 Emby 响应缓存（9 个含 Series 文件，197KB），**无任何 ChildCount≥2 剧集**，与既有"服务器仅单季样本"一致。
- 裁决：属**环境限制**（缺片源），非代码缺陷；保持 #160 pending，待有多季片源再验。不绕开 UI 解码加密源配置抓凭据，不折腾 UI。
