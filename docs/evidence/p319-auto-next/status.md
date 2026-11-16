# 319 · 自动连播 + 下一集预加载（对标 SenPlayer）

状态：**已实现并装机，待真机验证**
设备：K40 `b6e950ee`
时间：2026-11-16 17:53–18:17

## 一、SenPlayer 怎么做的（从 `resources.arsc` 提取，作为设计基准）

```
#1879  Auto play next item          自动播放下一项（总开关）
#1088  Next episode %1$s            下一集（带集名）
#1345  Preload Next Episode         预加载下一集
#1346  播放到达指定进度后，提前预加载下一集的 100 MB；
       成功时"下一集"按钮旁显示闪电图标
#1299  Playlist                     播放列表
```

**两个层次**：基础连播 + **预加载（100 MB）+ 闪电图标反馈**。

## 二、我们的实现（两处都比它省）

### 2.1 下一集信息：详情页提前带上，不查 API

`EmbyResource` 本来就有整个剧集列表（`EmbyShows.Episode` 带 `seasonIndex`/`index`），
只是启动播放时只传了当前集的 URL。现在：

```kotlin
private fun episodeIntent(c, a, ep, next: EmbyShows.Episode? = null): Intent =
    Intent(...)
        .putExtra("kivo.playurl", c.streamUrl(a, ep.id))
        .apply {
            if (next != null) {
                putExtra("kivo.nextUrl", c.streamUrl(a, next.id))
                putExtra("kivo.nextLabel", "...S%02dE%02d ...")
            }
        }
```

⇒ **播放页结束后不需要 token、不需要 SeriesId、不需要任何请求**。
SenPlayer 是播放页结束后再去查，我们省掉这一次往返。

`playEpisode` 的回调参数只有被点的那一集，拿不到它在列表里的位置 ⇒
`EmbyResource` 用 `currentEpisodes`（初始加载与换季加载都刷新）按 id 定位取下一个。

### 2.2 连播：无缝换集（同 Activity，不黑屏）

第一版是「新 Activity + finish 旧的」，黑 1~2 秒。改成**同 Activity 内换源**：

```
closeNetworkPlayback(回调)        ← 等旧会话 stop 回调（不能重入同一条代理/内核路径）
  → intent.putExtra("kivo.playurl", 新 URL)   ← Activity.intent 是同一对象，立即生效
  → urlPrep?.source?.close()                  ← 此时旧会话已停，读不到它
  → urlPrep = prepareUrlSource()              ← 新 URL 的新源（会 take 预热）
  → startUrlPlayback(holder, url, label)      ← 自带 generation 取消机制，本就可重入
```

**顺序不可换**：先关源会让仍在跑的旧会话读到已关闭的描述符。
**兜底**：没有 `SurfaceHolder` 时回落「重启 Activity」（冒烟测试会走到这条）。

### 2.3 预加载：复用 `OriginPrewarm`，不自己开连接

`OriginPrewarm.offer(url, token, sizeHint, resumeMs)` 本来就是详情页→播放页的预热通道
（预取片头 4 MiB + 片尾 + 恢复点），`prepareUrlSource()` 会用 `take(url)` **接手**。
复用它的三个好处：预热的连接读完回 keep-alive 池、切集后第一跳复用同一条 TCP、
不会出现第二套预热账本。

**触发条件**：**剩余时长 ≤ 5 分钟**（不是"播放到 90%"）。

> 为什么按剩余时间而不是百分比：90% 对 2 小时片是剩 12 分钟（白占带宽），
> 对 22 分钟番剧是剩 2 分钟（刚好）。按剩余时间对不同时长都合理。

**与 SenPlayer 的差异（我们更克制）**：它固定预取 100 MB；我们只预取足以让首帧表头
降到 ~300 ms 的那一组窗口。真机量过：未预热第一跳表头 **1000–1600 ms**，
预热过 **270–360 ms**（4 倍差），再往上加预取量的收益已经很小。

## 三、待验证（用户复验项）

| 项 | 期望日志 |
| --- | --- |
| 剩余 ≤5 分钟触发预加载 | `[autonext] 剩余 ${n}s ⇒ 预加载下一集片头` |
| 预热真的发起 | `[prep] 预热开始 url=<下一集>` |
| 播完自动切集 | `[autonext] 播放结束，3000ms 后播放下一集：xxx` |
| 无缝切换 | `[autonext] 无缝换集开始：xxx` → `[autonext] 无缝换集已提交起播` |
| 切集后起播快 | 新一集首帧表头应在 300 ms 量级（对照组 1000+ ms） |

## 四、没做的（明确记录）

1. **闪电图标**（SenPlayer 用它在"下一集"按钮旁表示预热成功）—— 需要改 UI，按铁律暂不动；
2. **预加载量的自适应**：现在是固定的窗口组。理论上可以按链路速率算"30 秒能下多少"，
   但先验证基础版是否够，避免过早优化；
3. **跨季连播**：`currentEpisodes` 是"当前展示季"的列表，最后一集之后不会自动进下一季。
   SenPlayer 有 Playlist，我们是剧集列表 ⇒ 需要 UI 层配合，另立卡。

## 五、改动文件

```
emby/EmbyResource.kt                         下一集 URL/Label 入 intent
playback/AutoNextEpisode.kt                  连播策略（3 秒倒计时 + 幂等）
playback/NextEpisodePrewarm.kt               预加载（剩余 ≤5 分钟触发）
ui/compose/PlayerEpisodeSwitch.kt            无缝换集（同 Activity）
ui/compose/PlayerSessionTeardown.kt          closeNetworkPlayback 加完成回调
ui/compose/PlayerSessionBridge.kt            两处 ENDED 接连播 + 每 tick 检查预加载
ui/compose/PlayerComposeActivity.kt          autoNextScheduled / nextPrewarmed
```
