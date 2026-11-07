# 224 · Emby/Jellyfin 对接 —— 状态稿（收口）

> 日期：2026-11-08 · 设备：Redmi K40（b6e950ee）· 服务端：用户两台真实 Emby（见下）
> 结论：**AC1–AC4 全部真机通过，M2 收官**。证据 `docs/evidence/p224/`，
> 校验器 `tools/verify_p224_emby.py`（5/5 变异自检）。

---

## 一、一句话结果

**填 `emby.bangumi.ca` + 账号 → 登录 → 60 部真电影海报墙（2:3 卡全渲染）→
点《死神来了 2》→ 4K 直连流硬解起播，精确节拍 0.983×——零新增解码代码，全程复用 220b 管线。**

## 二、实测矩阵

| 判据 | server 1（空密码路径） | server 2（密码路径） |
| --- | --- | --- |
| AC1 登录 | `emby.bangumi.ca:443`（nyamedia / Emby **4.8.0.62**），`syzayy` + `Pw:""` → AccessToken ✓ | `emby.9527.new.tg:443`，账号 + 密码 → AccessToken ✓ |
| AC2 海报墙 | **60 部**真条目全渲染：死神来了2/2003、第一滴血/1982、云图/2012… | **60 部**：Picnic at Hanging Rock/1975、Under Paris、美女与野兽/1991… |
| AC3 流播 | 《死神来了 2》：`url-play harddec ok 3840x2160`，状态栏 0:00:25→0:02:37 走带，**Δ媒体 31s / Δ墙钟 31.527s = 0.983×** | （同管线，未单独起播） |

AC3 走带证据：`p224-play2.png`（0:00:25）与 `p224-play3.png`（0:01:13）、`p224-rate-a/b.png`
（0:02:06→0:02:37 配精确墙钟）四帧状态栏读数 + `p224-status.log` 的 `url-play begin/harddec ok` 行。

## 三、与设计稿的差异（三条）

1. **PlayerComposeActivity 零改动**（设计稿预估"旧(小改)"）：现成 `kivo.playurl/kivo.label/kivo.routing`
   分支（L203-209）完全够用——家族树已在实现前修正并写明"零改"。
2. **家族树加了一条** `library/MediaLibraryActivity.kt` 旧(小改)：媒体库头部加 "Emby" 入口按钮
   （与 SMB 按钮同款）——实现前补入设计稿。
3. `static=true` 直连在 4K 片源上直接成立，PlaybackInfo 协商缺口未触发（G3 不静默降级的
   reason 路由保持待命，未被走到）。

## 四、本卡新知（下次直接用）

1. **K40 熄屏 + MIUI 锁屏**：无密码但 `input keyevent 82` 解不开——要 `input swipe 540 1900 540 500` 上滑；
   长测试先 `svc power stayon true`（USB 供电常亮），比抢 15s 熄屏超时可靠。
2. ** Emby 4.8 直连即 Range**：`/Videos/{Id}/stream?static=true&api_key=…` 直接被 MediaExtractor 吃下
   （HTTPS 有效证书 + HTTP Range），与 WebDAV(221) 同管线同结论——**远端 HTTP 视频源的通用解已经稳定**。
3. `X-Emby-Authorization` 头里 DeviceId 用静态串即可；`Pw:""` 空密码登录是合法路径。
4. 状态栏读数 + `date +%s%3N` 双截配对是 logcat 被抑制设备上量节拍的最便宜手段（±0.5s 读数误差可接受）。

## 五、豁免与未做（明示，不藏）

- Rex **Drivemby**（扫描/整理/webhook）不在本卡：拆解证据（§6.2）表明它是 Rex 自家云端服务
  （email 绑定 + `progress.waiting/recognizing/uploading`），**不是 Emby 协议**——永久排除。
- 剧集树/详情页（Rex §6.4 16:9 backdrop + 2:3 海报 + Watch/收藏）、观看状态回写、字幕多选、
  转码协商、账号持久化：设计稿 §六已排除，留 v2。
- Jellyfin 未实测（协议同源；无 Jellyfin 服务器环境，实测算 bonus）。
- 密码不入库：证据 JSON 只记 `auth_mode`，不记明文。
- server 2 只验登录+海报墙，未起播（AC3 用 server 1 的 4K 片已足够钉死管线）。
