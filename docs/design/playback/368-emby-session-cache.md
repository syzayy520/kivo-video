# 368 · Emby 登录会话复用 + hero 取数缓存（承接 367）

> 367 做完列表缓存后继续量，发现进入 Emby 浏览页真正的大头不是列表，而是**每次重新登录**：
> `[emby-connect] login 1221ms`。同一次进入里 hero/最近入库/续播又是 1.4–5.4 s（未缓存）。

## 一、实测（真机 K40 `b6e950ee`）

| 阶段 | 改造前 | 改造后 |
|---|---|---|
| `login`（`AuthenticateByName`） | **1221 ms** | **0 ms**（命中会话） |
| `items`（列表） | 4 ms（367 已缓存） | 4–5 ms |
| hero `recent`（最近入库） | 3496–5371 ms | **1 ms**（命中） |
| hero `resume`（继续观看） | 1390 ms | **2 ms**（命中） |
| 进入 Emby 浏览页合计（connect 段） | **1225 ms** | **4–5 ms** |

## 二、改造

### 2.1 会话缓存：`EmbyResumeSession` → `EmbyAuthCache`（升级而非另起一套）

原来只有一个只服务首页续播的 `EmbyResumeSession`，浏览页/聚合搜索各自 `login`。
本轮把它升级成通用会话缓存并**删除旧文件**（不留两套）：

| 维度 | 做法 |
|---|---|
| 键 | **凭据哈希**（地址+账号+密码）而非 sourceId —— 服务器删了再加（id 变、凭据不变）也能命中；另存 sourceId → 凭据键小索引，供只有 id 的场景（海报取图） |
| TTL | 15 分钟 |
| 存放 | **只在内存**；不落盘、不进 `HomeEntry`、不写任何文件 |
| 唯一入口 | `EmbyAuthCache.connect(address,user,secret)`：先查、未命中才 login、登录后回填。三个调用方（浏览页 / 聚合搜索 / 首页续播）共用，不会漏回填 |

**主流程收益**：首页启动时 `refreshContinue` 已为每个源登录一次，进入 Emby/聚合搜索时直接复用
⇒ 那 1.2 s 只付一次，而不是每次进页面各付一次。

### 2.2 hero 取数缓存

`EmbyHeroSource.recent/resume` 原本自己开连接读正文，绕过 `EmbyResponseCache`。改为走 `c.cachedBody(...)`：

| 端点 | TTL | 理由 |
|---|---|---|
| `recent`（最近入库，hero 候选） | 10 分钟 | 与列表同量级变化 |
| `resume`（继续观看） | **60 秒** | 带 `PlaybackPositionTicks`；刚看完一集再回首页不能显示旧进度，60 s 够覆盖"来回点开再返回" |

## 三、验证

- `ui_gate` PASS。中途曾因 `EmbySessionModel` 涨到 305 行触发门禁回归（新建超限文件），
  改为把"取会话或登录"收进 `EmbyAuthCache.connect()` 后回到 **299 行**。
- 单测 111 例 0 失败；`assembleDebug` SUCCESS。
- 真机：进入 Emby `login 0ms / items 4ms / total 4ms`；聚合搜索二次进入三源全 `[emby-cache] HIT`（4 ms），
  页面 180 条结果正常渲染。

## 四、边界（如实）

1. 会话 TTL 15 分钟。服务器**主动吊销 token**（改密踢下线）后，最多 15 分钟内仍会用旧 token；
   调用方遇到 401 应 `EmbyAuthCache.invalidate(sourceId)`，本缓存不自行探测。
2. `resume` 60 秒 TTL ⇒ 最坏 1 分钟内首页续播位可能滞后（播放停止时会走 `invalidateItem` 让详情缓存失效，
   但 home 的 resume 缓存按时间自然过期）。
3. 会话只在本进程：杀进程后首次进入仍需一次登录（约 0.4–1.2 s，视服务器）。
