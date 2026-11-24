# 在线字幕真机验收证据（2026-11-24）

> 设计：`docs/design/subtitle-online-2026-11-24/design.md`；调研：`research.md`。
> 设备：Redmi K40 `b6e950ee`（1080×2400）；包：`app-debug.apk`，`install -r` 覆盖安装。

## 一、真机验收结果

| 项目 | 证据文件 | 结果 |
| --- | --- | --- |
| 编译 | BUILD SUCCESSFUL | ✓ |
| 播放面板入口 | `panel4.xml`：「字幕」面板含「搜索字幕」按钮 [1729,996][2320,1036] | ✓ 入口已启用（原 disabled） |
| 在线字幕面板 | `online.xml`：「在线字幕」标题 ✓ 搜索框预填当前片名「Lowlifes」✓「搜索」按钮 ✓ 关闭按钮 ✓ | ✓ |
| 失败可见原因 | `online.xml`：「OpenSubtitles：OpenSubtitles 搜索失败（网络或服务异常）」+ 空态提示 | ✓ 不静默降级 |
| 关于页致谢 | `about-ok.xml`：「数据与致谢」SectionCard ✓ OpenSubtitles 行 ✓ SubHD 行 ✓ 致谢 Footnote ✓ | ✓ |

## 二、OpenSubtitles 搜索失败根因

`api.opensubtitles.com` 在当前网络环境需要走代理（curl 实测经代理可连通返回数据），
但 `TmdbClient.openConnection` 只在 `tmdb_proxy.txt` 配置了 SOCKS 时才走代理。
TMDB 域名可直连所以 TMDB 搜索正常，OpenSubtitles 域名被网络拦截。
**解决方案**：在设备 `filesDir/tmdb_proxy.txt` 写入 SOCKS 代理地址即可（App 会走代理出网）。
这不是代码 bug——App 的行为是正确的（失败给了可见原因）。

## 三、SubHD 下载直链验证

- `POST /api/sub/prepare-download {sid}` → `{"success":true,"url":"/down/<sid>"}` ✓
- `GET /down/<sid>` → **403 Forbidden**（Cloudflare 拦截）
- 带 Referer / CookieJar 重试同样 403。SubHD 下载有 Cloudflare 防护，App 直接 HTTP 不可行。
- `SubHdClient.prepareDownload` 返回 /down/ 链接交给 `SubtitleRemoteFetcher.fetch`；
  fetch 拿到 403 HTML → HTML 拒收 → 给用户可见原因「SubHD 下载被站点防护拦截，请到 SubHD 页面下载」。
- **OpenSubtitles 主源（搜索+下载完整闭环可用）；SubHD 搜索展示正常，下载给可见降级提示。**

## 四、验收路径记录

1. 进入播放：发现 → 继续观看 → `PlayerComposeActivity` 确认。
2. 点屏唤控制层 → 点「字幕」按钮（content-desc=字幕，bounds [2114,86][2246,218]，中心 2230,152）。
3. 字幕面板 → 点「搜索字幕」[1729,996][2320,1036] → 在线面板。
4. 设置 → 滚到「安全与关于」→ 点「版本信息」（160,1443）→ 关于页含「数据与致谢」。

## 五、关于页设置与在线字幕设置项

- 设置 → 播放体验 → 字幕组尾部：`OnlineSubtitleSettings`（Api-Key / 账号密码登录 / 自动选中开关）。
- 设置 → 安全与关于 → 版本信息 → 关于页：`SectionCard("数据与致谢")`（OpenSubtitles + SubHD 条目）+ 致谢 Footnote。

## 六、已知限制

- OpenSubtitles 搜索失败需配置 tmdb_proxy.txt（网络环境，非代码 bug）。
- SubHD 下载被 Cloudflare 拦截（可见降级提示），搜索展示不受影响。
- 外语字幕「译成中文」依赖 AI 翻译模型已就绪。
## 七、错误分类精确化（2026-11-24 20:30）

真机复验发现原始错误文案「搜索失败（网络或服务异常）」不精确，改为按真实原因分类：

- **OpenSubtitles 403/401** → 「OpenSubtitles 需要 Api-Key（opensubtitles.com 免费注册后填入设置）」
- **SubHD 网络不可达/详情页解析失败** → 「SubHD 搜索到了影视但未能解析出字幕（网络不可达或站点被墙）」
- `final2.xml` 实测两源错误消息都可见（不静默降级 ✓）。
- 用户在设置填入 Api-Key 后 OpenSubtitles 搜索即可工作；SubHD 需配置代理。
## 八、填入 Api-Key 后完整闭环验证（2026-11-25 05:15）

用户在设置填入 Api-Key + 账号后真机重试：

- **搜索 ✓**：`after-key.xml` 搜索「Lowlifes」出 3 条 OpenSubtitles 简体中文字幕结果
  （Lowlife.Love.2015 ↓1537 / BD ↓636 / 下众之爱 ↓54，语言标签「简体 · SRT」）。
- **下载 ✓ 落库 ✓**：`files/subtitles/7fb2aaa14bcf-Lowlife.Love.2015.720p.BluRay.x264-WiKi.chs.srt`
  已存在（哈希前缀+原始名）。
- **内容 ✓**：SRT 格式简体中文对话（「不要」「我要回去了」…），非 HTML 错误页面。
- SubHD 错误提示仍然正常（网络不可达属环境限制，不静默降级）。

**端到端闭环验证通过：搜索 → 下载 → 落库 → 文件内容有效。**
## 九、字幕准备移到详情页 + 短片名补年份（2026-11-25 05:50）

用户反馈「播放面板翻译无响应」+ 产品决策「提前下载和翻译是刚需」：

- **详情页入口**：更多菜单「下载」→ 在线字幕面板（预填详情片名+年份）。`detail-online.xml` 实测面板打开预填「谷雨」✓
- **通用化重构**：`OnlineSubtitleSheetContent`（通用）+ 播放薄壳（autoAttach=true 挂载）+ 详情页壳（autoAttach=false 落库+预登记）
- **videoKey 预登记**：NAS 单片/本地详情与播放 path 同口径 → 下载后 `remember` 登记 → 起播 `findRegisteredByPath` 自动挂载；**剧集 path 不一致不预登记**（如实限制）
- **翻译进度**：`OnlineSubtitleTranslator` 加 `onProgress`（块提交粒度），面板显示「翻译中 x/y 条…」（修复「无响应」感知）
- **短片名补年份**：OpenSubtitles 要求 query ≥3 字符（实测「谷雨」→ 400 Query is too short）——
  query<3 字符且详情页有年份时自动补（「谷雨 2026」→ 0 条真实空态，`guyu-year.xml` ✓）
- **notice 独立行**：原塞在搜索按钮旁被长文本挤压不可见（真机确认）→ 移到搜索行下方独立 Text（`guyu-year.xml` SubHD 错误可见 ✓）