# P243 · SMB 重构证据（S0/S2）

> 设备：Redmi K40（`b6e950ee`，M2012K11AC，Android 13）
> 服务器：小米路由 `192.168.5.1:445`（匿名）
> 时间：2026-11-09 23:00–23:10

## 一、S0 · 共享枚举复算（服务器侧 + 真机）

### 1.1 服务器侧（Windows 11，`net view`）

| 文件 | 结果 |
| --- | --- |
| `docs/evidence/p243/smb-reference/net-view-192.168.5.1.txt` | 6 个磁盘共享：TDDOWNLOAD / 下载 / 共享 / 图片 / 文档 / 视频 / 音乐 |
| `docs/evidence/p243/smb-reference/net-view-192.168.5.197.txt` | 系统错误 1707（该实现不走 SMB1 ShareEnum） |

### 1.2 真机（libsmb2 `smb2_share_enum_sync`，匿名）

`s0-enum-xiaomi-r4.txt`：

```
P243 SHARES XiaoMi||0
P243 ROOT XiaoMi → 17 项: .filetransfer_tmp/,.sysapihttpd/,.systemconfig/,.torrents/,
  .tunneluploads/,.upload_tmp/,lost+found/,TDDOWNLOAD/,ThunderDB/,timecapsule/,
  下载/,共享/,图片/,摄像机监控视频/,文档/,视频/,音乐/
P243 LEVEL2 .sysapihttpd → 14 项
P243 LEVEL2 .systemconfig → 4 项
P243 LEVEL2 .tunneluploads → 3 项
P243 READ .tunneluploads/ldotTb1YR9qZc1y1QhL0MA.tmp size=23853720 head=1048576 tail=1048576
3/3 PASS
```

**结论（与设计 D1 有一处修正，必须记住）**

1. 枚举链路在真机可用：`IPC$` 连接 + `NetrShareEnum` 返回磁盘共享，`SmbShareLister` 工作正常。
2. **匿名账号只看到 1 个共享（XiaoMi）**，而服务器实际有 6 个 —— 其余共享对匿名不可见
   （Windows/Samba 的共享级 ACL）。`net view` 用的是 PC 已认证的会话，所以能看到全部。
3. 因此 **"枚举" 不能完全取代 "手填共享名"**：匿名场景下列表可能只剩一个名字。
   → 设计 D1 修订为：**枚举优先，手填保留**（表单上有"直接输入共享名"入口），
   两者并存而不是替代。
4. XiaoMi 共享根下已经有 `视频/`、`音乐/`、`图片/`、`文档/` 等目录，
   所以"必须知道共享叫视频"这个前提本身就不成立 —— 枚举出 XiaoMi 进去就是同一批目录。

### 1.3 一条被证伪的旧假设

`s0-enum-video-r3.txt`：指定 `share=视频` 直接连接，返回
`SmbFailure: 服务器不允许匿名访问，请填写账号和密码`（`STATUS_ACCESS_DENIED`）。

220 时期该共享匿名可连；现在被拒，说明**路由器的共享权限已被改过**（不是代码问题）。
记录在此，避免后来者把"连不上视频共享"误判成 libsmb2 或新协议层的 bug。

## 二、S2 · 协议层验收

`SmbAddressInstrumentation`（12 例，纯函数）→ `12/12 PASS`：

```
PASS 完整 URI：主机/共享/路径
PASS 显式端口 + 默认端口不写进存储
PASS 只有主机 → 需要先选共享
PASS Windows UNC 路径
PASS 裸 主机/共享
PASS scheme 大小写不敏感 + 末尾斜杠
PASS 共享名里的空格与中文原样保留
PASS IPv6 主机
PASS 非法输入给出可读原因
PASS filePath 拼接共享与路径
PASS 凭据：匿名判定与密码不进 toString
PASS 共享类型：只展示可浏览的磁盘共享
12/12 PASS
```

其中一处是真 bug：`smb:///share` 原先被解析成 `host="share"`（`trimStart('/')` 吞掉了空 authority
的证据），已改为"去 scheme 后仍以 `/` 开头 → 地址缺少服务器"。

`SmbShareEnumInstrumentation`（真机三条）→ `3/3 PASS`：

| 用例 | 结果 |
| --- | --- |
| 共享枚举（IPC$ + NetrShareEnum） | PASS，返回 1 个（匿名可见范围） |
| 连到共享并列出根目录 | PASS，17 项 + 两级子目录 |
| 随机读：文件头 1 MiB + 尾部 1 MiB | PASS，`head=1048576 tail=1048576` |

## 三、复算命令

```bash
# 1) 打包测试入口（一次只能注册一个 instrumentation 名）
./gradlew :app:assembleDebugAndroidTest \
  -Pkivo.testRunner=com.kivo.cinema.smb.SmbAddressInstrumentation
./gradlew :app:assembleDebugAndroidTest \
  -Pkivo.testRunner=com.kivo.cinema.smb.SmbShareEnumInstrumentation

# 2) HyperOS 上 adb install 会弹「USB安装」并 9 秒后自动拒绝；
#    pm install 偶发被拒，重试 2-3 次即可（本次实测第二次成功）。
adb push app-debug.apk /data/local/tmp/kivo.apk
adb shell pm install -r -t /data/local/tmp/kivo.apk
adb push app-debug-androidTest.apk /data/local/tmp/kivo-test.apk
adb shell pm install -r -t /data/local/tmp/kivo-test.apk

# 3) 跑（服务器地址从参数传，不写死）
adb shell am instrument -w \
  com.kivo.cinema.test/com.kivo.cinema.smb.SmbAddressInstrumentation
adb shell am instrument -w -e smbHost 192.168.5.1 \
  com.kivo.cinema.test/com.kivo.cinema.smb.SmbShareEnumInstrumentation
# 指定共享复算：-e smbShare 视频
```

> 注意：`am instrument -w` 只回传 `finish()` 的 `stream`；`println` 不进结果。
> 测试里的日志必须同时写进 report（见 `SmbShareEnumInstrumentation.log`）。

---

# S3 · 播放通道（SMB 直接产 fd）

> 时间 2026-11-09 23:05–23:35

## 一、实现

`SmbReader → SmbByteSource(1 MiB 预读) → RangeFd → ParcelFileDescriptor → 原生解码器`。
删掉了 SMB 播放路径上的 `LocalRangeProxy`（它只保留给 DLNA 投屏）。

## 二、真机结果

片源：`XiaoMi/视频/电影/The.Furious.2026.2160p.iTunes.WEB-DL.DDP.5.1.Atmos.HDR10+.H.265-CHDWEB.mkv`
（20,905,428,916 B ≈ 19.5 GB，HEVC 10-bit HDR10+）

| 项 | 结果 | 证据 |
| --- | --- | --- |
| 起播 | ✅ `OMX.qcom.video.decoder.hevc`，`dur=6791201000`（1:53:11），`audio=true` | `s3-playback.txt` |
| 位置推进（3 s） | ✅ `0 ms → 2867 ms` | 同上 |
| 连续播放（UI 时间码） | ✅ `0:01:52 → 0:02:01`（间隔 8 s，推进 9 s，1:1 实时） | `/tmp/s3-c1.png` / `s3-c2.png` |
| 随机读 | ✅ 20 GB 文件 `head=1048576 tail=1048576` | `s2-browse-video.txt` |
| seek | ⚠️ `landing=0`，实际位置不动 | `s3-seek-smb-r1.txt` |

## 三、seek 归因（关键：不是 S3 引入的）

三组对照，同一套解码会话、同一套测试：

| 组合 | 起播 | 位置推进 | seek |
| --- | --- | --- | --- |
| 本地 MP4（`local-hevc.mp4`，MediaExtractor） | ✅ | ✅ 0→2971 ms | ✅ target 2500 → landing 2000（关键帧对齐） |
| 本地 MKV（`local-seek.mkv`，FFmpeg demux） | ✅ | ❌ **0 → 0** | 未到 seek 步骤 |
| SMB MKV（19.5 GB，FFmpeg demux） | ✅ | ✅ 0→2867 ms | ❌ landing=0 |

→ **带病的是 MKV 的 FFmpeg demux 路径**（本地 MKV 连"位置"都恒 0），
不是 SMB 传输层。这条与 240 卡记录的"MKV 只出 2 帧 / pts 读数矛盾"属同一族缺陷。

SMB 传输层本身的三项能力（起播 / 连续推进 / 大文件随机读）全部通过，
所以 S3 的通道改造可以收口；**MKV seek 单开一张卡**（编号建议 244），
不在 S3 里顺手改——那会同时动 demux 与 avsync，破坏"一卡一责"。

## 四、未验证（诚实记录）

- **MP4-over-SMB**：共享是只读（匿名匿名写入被拒 `拒绝访问`），无法上传小 MP4 做对照。
  等有可写共享（或 Windows 共享）时补测，它才是"MP4 + SMB"的干净组合。
- **Emby 回归**：`HttpRangeFile` 改为实现 `RangeSource`（行为未变），`NetworkPlaybackSession`
  构造签名扩展。需要一次 Emby 点播回归，未做。

## 五、踩到的坑（后来者注意）

1. **两个 fd 回调线程会并发读同一个 libsmb2 context**。探针和解码器各开一个 fd，
   各自一个回调线程；不加锁会在 5 秒内报"读取中断"。`SmbByteSource` 已用 `lock` 串行化。
2. 播放页**强制横屏**（`rotation=1`，2400×1080），adb 坐标要按横屏算，否则点在屏幕外。
3. Compose 的 Slider 在该机型 `uiautomator dump` 里**没有语义节点**，adb 点/滑都命中不了；
   seek 只能走代码路径验（本卡的 `SmbPlaybackSeekInstrumentation`）。

---

# S4 · 源管理接入（SMB 成为"源"，不再是调试页）

> 时间 2026-11-09 23:24–00:35

## 一、代码

| 文件 | 改动 |
| --- | --- |
| `sources/SourceRecord.kt` | +`protocol`（emby/smb）；username 仅 Emby 必填（SMB 匿名合法） |
| `sources/SourceAddress.kt` | +`smb://` 分支，SMB 地址解析委托给 `SmbAddress`（同一个标准） |
| `sources/storage/SourceStore.kt` | schema **2**，兼容读 1（v1 一律按 emby 解释）；去重键改 `(protocol,address,username)` |
| `sources/SourceManagerModel.kt` | +`protocol`/`startNew(kind)`；`save()` 按协议选验证器；SMB 名称兜底用共享名 |
| `sources/ui/workspace/SourceConnectionContent.kt` | 一份表单两种协议（文案、占位、匿名提示随协议变） |
| `sources/ui/SourcesScreen.kt` | SMB 入口卡 = "添加 SMB 源"；已保存的 SMB 源卡归入「本地与存储」 |
| `sources/ui/SourceCard.kt` / `SourceManagementContent.kt` | 按协议换图标/副标题（SMB 显示地址，不编造片量） |
| `smb/ui/SmbBrowserActivity.kt` | **新增**：读 sourceId → 解密凭据 → 浏览 → 起播 |
| `ui/compose/PlayerComposeActivity.kt` | 收 `sourceId + smbPath`，**删除 `kivo.smb` 明文 JSON 通道** |
| `MainActivity.kt` | 源卡点击按协议分派（EmbyActivity / SmbBrowserActivity） |
| 删除 | `smb/SmbActivity.kt`、`smb/SmbConfig.kt`（旧调试面板与明文配置） |

## 二、真机结果

| 项 | 结果 | 证据 |
| --- | --- | --- |
| 写入 SMB 源（schema 2） | ✅ `小米路由 / smb / smb://192.168.5.1/XiaoMi / 匿名` | `s4-source-save.txt` |
| 读回 + 密文检查 | ✅ protocol/address 正确；磁盘无明文源名、无 `smb://`、无明文密码 | 同上 |
| 用保存的源连接 | ✅ 根目录 17 项 | 同上 |
| 源卡出现在资源库 | ✅（当时归在"媒体服务器"下，已改为"本地与存储"，见下） | `s4-source-card.png` |
| 点击源卡进入浏览页 | ✅ 标题"小米路由"、17 个目录、返回/上一级 | `s4-browser-root.png` |
| 目录导航 | ✅ `/视频` → 111 / tv / 学习 / 电影 | 运行期 dump |
| 播放（sourceId 通道） | ✅ 4K HDR10+ HEVC，"正在播放" | `s4-play-by-sourceid.png` |
| 添加表单 | ✅ 字段：名称/共享地址/用户名/密码 + 粘贴识别 | `s4-add-form.png` |

## 三、没验到的

1. **分组修正未截图**：源卡从"媒体服务器"移到"本地与存储"是收口前最后两行改动，
   装包后设备息屏（`mCurrentFocus=null`），未能复拍。代码逻辑是 `visible.filter { it.isSmb }`，
   `compileDebugKotlin` 通过，但**没有真机画面**。
2. **浏览页点文件起播未点验**：列表里 127 项、目标在末尾，adb 滚动不稳；
   播放走的是同一条 `sourceId + smbPath` 通道（已用 am start 直接验证），
   缺的是"点击 → 起播"这一跳的截图。
3. **表单特殊字符输入**：`adb shell input text` 会吞掉 `:` `/` `.`（`smb://a/b` 变成 `smba`），
   所以 UI 自动化不能用它填地址；真实用户输入不受影响（截图里字段本身工作正常）。

## 四、验证方法备忘

- `SmbBrowserActivity` 是 `exported=false`（正确）；`am start` 会被拒（uid 2000）。
  **验证浏览页必须从 UI 点进去**，或用同 uid 的 instrumented test。
- `PlayerComposeActivity` 仍 `exported=true`，可以用
  `am start ... --es kivo.sourceId <uuid> --es kivo.smbPath <共享内路径>` 直接验证播放。
- K40 长时间静置后会进入"屏幕黑但 Wakefulness=Awake"的状态，
  `wm dismiss-keyguard` / `keyevent 82` 都无效，需要用户手动点亮。

---

# S5 · 局域网发现（mDNS）

> 时间 2026-11-09 23:40–00:05

## 一、网络事实（PC 侧实测，零依赖）

本机装 `zeroconf` 后浏览 4 种服务类型，12 秒结果：

```
FOUND N1._device-info._tcp.local. -> N1.local. ['192.168.5.197'] 0
FOUND N1._smb._tcp.local.        -> N1.local. ['192.168.5.197'] 445
```

**结论**：
1. 局域网里确实有 `_smb._tcp` 广播，来自 NAS `N1`（192.168.5.197:445）。
2. **小米路由（192.168.5.1）不广播** —— 所以"发现"永远只能覆盖一部分服务器，手工地址必须保留。
3. 这正好解释了 Rex 的"可用地址"为什么只有一个 `192.168.5.197 N1`：
   它发现的就是这台 NAS，路由不在里面。我们的实现会得到同一份结果。

## 二、实现

- `smb/discovery/SmbDiscovery.kt`（≤140 行）：系统 `NsdManager` 浏览 `_smb._tcp`，
  resolve 队列串行化（NSD 同一时刻只能 resolve 一个），8 秒超时，回调切主线程。
  **`start()` 内部 post 到主线程** —— 部分 ROM 的 NsdManager 要求调用方带 Looper。
- `SourceConnectionContent`：SMB 表单顶部出现「可用地址」胶囊，点一下填 `smb://host`；
  发现不到任何设备时**不占空间**（手工输入是主路径）。

## 三、真机验证：未完成（设备状态阻塞）

`SmbDiscoveryInstrumentation` 三次尝试均返回 `Process crashed`，`logcat -b crash` 为空
（不是 Java 崩溃），此时 K40 处于"黑屏但 `mWakefulness=Awake`、`mCurrentFocus=null`"的状态：

| 尝试 | 改动 | 结果 |
| --- | --- | --- |
| 1 | 直接调 `SmbDiscovery.start` | Process crashed |
| 2 | `start()` 内部 post 到主线程 | Process crashed |
| 3 | 测试里先 `startActivity(MainActivity)` 前台化，再发现 | Process crashed |

判断：**息屏 + 后台的进程在 MIUI 上拿不到组播**（同类限制在之前的后台下载上也出现过）。
这不是代码证据，不冒充通过；等设备点亮后在**真实表单里**看「可用地址」是否出现 N1，
是最直接的验收方式（比 instrumented test 更贴近用户路径）。

证据文件 `s5-discovery.txt` 只有 `Process crashed`，保留原文不修饰。

---

# 参考：Kivo 音乐播放器（`F:\kivo`）的 SMB 实现对照

> 2026-11-09 23:43 用户提示"音乐播放器刚做的 SMB 可以播放了，参照看看有没有破解之法"。
> 以下是对照结果与移植清单。

## 一、它做对了什么（我们缺的）

| # | 它的做法 | 我们的现状 | 处置 |
| --- | --- | --- | --- |
| 1 | **一个会话 = 一个专属线程**（`SmbWorker` 单线程 executor，所有 native 调用 `worker.call{}`）。注释记录了真机事故："点歌后应用直接消失" = 跨线程用 `smb2_context` | 我们用 `synchronized` 串行化，能工作但没有线程归属 | **未移植**（见下"为什么"） |
| 2 | **块对齐 LRU 缓存**（256 KiB × 16 = 4 MiB），seek 后已缓存的块仍可用 | 我们是 1 MiB 单窗口，seek 后整窗作废 | **未移植**（视频码率高，1 MiB 窗口顺序读更省往返；列为可选优化） |
| 3 | **失败给一次重连**（`recoverThenRead`）：关旧会话→重建→重读同一块，只给一次 | 没有 | ✅ **已移植** |
| 4 | **未到尾部读到 0 不伪装成 EOF**，抛 IOException | 我们 `if (read <= 0) break` 会把传输异常当 EOF | ✅ **已移植** |
| 5 | `nativeReadAt` 在 native 层循环填满（`smb_file_reader.cpp`），并区分"已有进展的短读"与"真失败" | 我们在 Kotlin 层循环 | 等价，不重复实现 |
| 6 | `SmbBlockCache` 单独成文件，可脱离网络做 JVM 单测 | 无 | 记录为后续 |

**为什么第 1 条没移植**：我们已经在 `SmbByteSource` 用一把锁把"窗口 + pread + close"全部串行，
真机跑 19.5 GB 4K 连续播放 3 分钟、两次断网重连测试都没崩溃。再套一层专属线程，
每次读多一次线程切换（fd 回调本身就是串行的），收益不明确、风险是引入新的死锁面。
**等出现随机崩溃再上**，并以此为准：一旦崩，第一嫌疑就是跨线程用 context。

## 二、移植后实测

改动：`SmbByteSource` 现在自己负责"连接 + 预读 + 一次重连"，构造改用 `SmbByteSource.open(...)`；
新增 `closed` / `reconnectFailure` 两个状态，避免在已关闭的句柄上读。

**回归**（改完仍能正常播）：19.5 GB 4K HDR 连续播放 55 秒 `正在播放`，无错误。

**断网测试**（`svc wifi disable` 10 秒 → `enable` → 等 18 秒）：

| 时刻 | 现象 |
| --- | --- |
| 断网 10 秒时 | 仍在播（`正在播放`，画面继续）—— 解码器缓冲比 1 MiB 预读窗口深得多 |
| 恢复 18 秒后 | `网络播放失败：SMB 读取器尚未打开`（第一次）→ 修复后变成明确的"重连失败"原因 |

**结论（不粉饰）**：数据层的"一次重连"实现了，但在当前架构下**救不回正在播的会话**：

```
RangeFd.onRead 抛错 → failureReason 非空
  → NetworkPlaybackSession.poll 看到 → fail() + releaseNative()
  → 会话终止；此后再重连成功也没人读了
```

要真正做到"断网恢复继续播"，需要 `NetworkPlaybackSession` 在失败时**不立即终止**，
而是进入"等待重连"态并冻结位置 —— 那是会话层的改动，**建议单开一张卡**，
不在 S3/S4 里顺手改（会同时动 fd 层、会话层与错误语义）。

## 三、顺带修掉的一个误导性文案

第一次断网测试报的是 `SMB 读取器尚未打开` —— 原因是重连失败后 `reader` 仍指向已 `close()` 的旧对象，
下一次读落在死句柄上。现在重连失败会固化原因（`reconnectFailure`），后续读报
`SMB 连接已断开，重连失败：<真实原因>`，并且 `closed` 后的读一律返回 EOF 而不是抛异常。

---

# S3 补测 · 断网恢复继续播（真的通了）

> 时间 2026-11-09 23:52–23:58
> 触发：用户提示音乐播放器（`F:\kivo`）SMB 已能播，对照后移植"一次重连"，但当时只做到
> "失败原因更准确"，播放会话仍然会死。这一轮把会话救回来了。

## 一、两轮对照

| 轮次 | 改动 | 结果 | 证据 |
| --- | --- | --- | --- |
| r1 | 只有"重连一次 + 记住失败原因" | ❌ `state=FAILED`，`err=SMB 读取器尚未打开` | `s3-reconnect-r1-fail.txt` |
| r2 | 见下三处修正 | ✅ 停 16.0 s 后接上，播到 119.7 s，`state=PLAYING` | `s3-reconnect-r2-pass.txt` |

r2 原始输出（节选）：

```
P243 START codec=OMX.qcom.video.decoder.hevc dur=6791201000 audio=true
P243 STALL 位置停在 25233 ms（已 5000 ms 未推进）
P243 RESUME 停 16002 ms 后从 25233 ms 接上，现在 41996 ms
P243 TAIL state=PLAYING pos=119695 ms err=
4/4 PASS
```

测试方式：`SmbReconnectInstrumentation` 起播后每秒采样位置；宿主在测试跑到第 26 秒时
`svc wifi disable`，10 秒后 `enable`。测试判定三件事：会话没被判死、观察到了停顿、停顿后继续推进。

## 二、真正的三处病根（都不是"重连代码写错"）

**1. 死句柄还在被读（最隐蔽）**
重连失败时旧 `SmbReader` 已被 `close()`，但字段还指着它；下一次读先撞上
`SmbReader.pread` 的"尚未打开"，把真实的"网络还没回来"整句盖掉。
→ 新增 `readerDead` 标志：死句柄不再读，后续尝试直接走重连。

**2. 重连只给一次，退避后没有第二次**
上一版 `reconnectUsed` 一旦置真就不再重连，于是"断网 10 秒、第 3 秒恢复"也救不回来。
→ 改成**有退避的持续重试**（400 → 800 → 1600 → 3200 → 4000 ms 封顶），
   `readerDead` 在每次失败后允许再次重连。

**3. fd 层一遇错就判死**
`RangeFd.onRead` 原来 catch 到异常立刻写 `failureReason` 抛 EIO，
`NetworkPlaybackSession.poll`（每 100 ms）看到就 `fail() + releaseNative()`。
→ `RangeFd` 增加**耐心窗口**（默认 20 s）：窗口内按 250 → 500 → 1000 ms 退避重试，
   期间 `failureReason` 保持空，会话继续 `PLAYING`；只有超过窗口才判死。
   取消（`cancel()`）优先，退出播放时不会拖住 teardown。

## 三、结论与代价（不粉饰）

- 用户可见行为：**播放中 Wi-Fi 断 10 秒 → 画面停住 → 约 16 秒后自己接上继续播**，不再报错。
- 代价：停顿期间位置冻结，恢复后**跳过了约 16 秒内容**（原生播放时钟没停，恢复时对齐到当前时钟）。
  要做到"停多久、回来补多久"需要会话层暂停时钟，属另一张卡。
- 停顿期间 UI 仍显示"正在播放"，没有"重新连接中"提示 —— 也是另一张卡（缓冲态提示）。

## 四、可复用的判定

```
stall 出现      → 位置连续 ≥5 s 不推进
recovered       → 位置超过 stall 点 1 s 以上
session alive   → state 始终 PLAYING，failureReason 为空
```

---

# 待执行验收清单（设备解锁后一次跑完）

> 2026-11-10 00:10 记。设备处于 PIN 锁屏：`screencap` 返回 0 字节、
> `pm install` 报 `INSTALL_FAILED_USER_RESTRICTED`、`uiautomator dump` 读到旧文件。
> 以下命令按顺序执行即可，**不需要改代码**。

## 1. 装包（约 40 s）

```bash
ADB=~/.workbuddy/binaries/android-sdk/platform-tools/adb.exe   # 实际用绝对路径
"$ADB" -s b6e950ee push "C:/kivo video/android/app/build/outputs/apk/debug/app-debug.apk" /data/local/tmp/kivo.apk
"$ADB" -s b6e950ee shell pm install -r -t /data/local/tmp/kivo.apk
"$ADB" -s b6e950ee push "C:/kivo video/android/app/build/outputs/apk/androidTest/debug/app-debug-androidTest.apk" /data/local/tmp/kivo-test.apk
"$ADB" -s b6e950ee shell pm install -r -t /data/local/tmp/kivo-test.apk
```

## 2. 断网恢复（预期 5/5 PASS，新增"断网时上报重连中状态"）

```bash
"$ADB" -s b6e950ee shell am force-stop com.kivo.cinema
("$ADB" -s b6e950ee shell am instrument -w com.kivo.cinema.test/com.kivo.cinema.smb.SmbReconnectInstrumentation \
   > docs/evidence/p243/smb-rebuild/s3-reconnect-r3.txt 2>&1 &)
sleep 26
"$ADB" -s b6e950ee shell svc wifi disable
sleep 10
"$ADB" -s b6e950ee shell svc wifi enable
sleep 90
cat docs/evidence/p243/smb-rebuild/s3-reconnect-r3.txt
```

判据：`PASS 断网时上报重连中状态` 出现，且 `TAIL ... buffering=true`（停顿期间确实上报过）。

## 3. 三张界面截图

```bash
"$ADB" -s b6e950ee shell am start -n com.kivo.cinema/.MainActivity
# 底部标签「资料库」≈ (656, 2270)；进去后 uiautomator dump 找卡片
"$ADB" -s b6e950ee shell input tap 656 2270
"$ADB" -s b6e950ee exec-out screencap -p > docs/evidence/p243/smb-rebuild/s4-group-fixed.png
```

| 要证明的事 | 判据 |
| --- | --- |
| SMB 源卡在「本地与存储」组 | 截图里"小米路由"出现在该分组标题下，且「媒体服务器」组内没有它 |
| 表单有「可用地址」胶囊 | 添加 SMB 源 → 表单顶部出现 `192.168.5.197 N1` 胶囊（PC 侧 zeroconf 已证实只有这台 NAS 广播） |
| 浏览页点文件起播 | `/视频/电影` → 点 `The.Furious…mkv` → 播放页出现画面与时间码 |

## 4. 收尾

- `settings put secure lockscreen.disabled` 本轮查时已是 `1`（原本就是），无需恢复。
- `settings put secure adb_install_need_confirm 0` 是本轮写入的，若不再需要可 `delete secure adb_install_need_confirm`。

---

# 静态审查（无需真机，2026-11-10 00:15）

设备锁屏期间对 243 新增代码做了一轮审查。**改了两处，确认三处无问题**。

## 改：浏览页三处缺陷（`smb/ui/SmbBrowserActivity.kt`）

| # | 缺陷 | 后果 | 修法 |
| --- | --- | --- | --- |
| 1 | `load()` 每次新开线程，结果回来无条件写 UI | 快速连点目录/上一级时，**慢请求后到会把当前目录覆盖成上一个目录的内容**（标题写 `/视频/电影`，列表是 `/视频`） | 加 `loadSeq` 请求序号，只有最新一次的结果能被采用 |
| 2 | `record`/`session` 由后台线程写、UI 线程读，无 `@Volatile` | UI 可能读到 null → 误报"会话未就绪" | 两个字段加 `@Volatile` |
| 3 | `onDestroy` 在主线程 `session.close()` | `SmbSession` 的锁可能在一次目录列举（网络往返）里被持有，主线程 join 它就是 **ANR 风险** | 先摘引用 + 作废在途请求，再丢后台线程 close |

## 改：SMB 投屏判断回归（`ui/compose/PlayerComposeActivity.kt`）

`onCastSelect` 用 `smbSource != null` 判断"这是 SMB 片源"，但 243 之后 SMB 走
`SmbByteSource → RangeFd`，`smbSource` 已恒为 null —— 于是投 SMB 片源会去
`FileByteSource("视频/电影/xxx.mkv")` 这条不存在的本地路径，报一个看不懂的投屏失败，
而不是"SMB 片源投屏暂未开放"。

→ 新增 `smbPlayback` 标记，在 `startNetworkPlayback` 里按 `routing == "smb"` 置位，
投屏分支改用它。

## 确认无问题（审查结论，未改）

| 位置 | 检查项 | 结论 |
| --- | --- | --- |
| `SmbSession` | 并发 `list` / `close` 与 native 句柄生命周期 | `connect/list/close` 全在同一把锁内，`ctx` 置 0 也在锁内，无 UAF |
| `SourceStore` | schema 1→2 兼容、写入原子性、失败不覆盖 | v1 按 emby 解释；`AtomicFile` 写；解析失败一律不落盘 |
| `SourceCipher` | 凭据加密 | AES-GCM + AAD 头校验，无明文回退 |

## 待清理（未做，避免一次改太多）

`PlayerComposeActivity` 里 `smbFfSession` / `smbStarted` / `smbSource` / `smbProxy`
四个字段已无写入方（`smbFfSession` 从未被赋值），对应 `positionPoll` 里约 30 行分支是死代码。
按"旧代码彻底清除"的规矩应删，但涉及 seek 与 onDestroy 三处交叉，建议单独一次提交并配截图验证。

---

# 真机验收（平板 MatePad + 手机 K40，2026-11-10 00:18–00:24）

## 一、断网恢复 5/5 PASS（K40）

`s3-reconnect-r3.txt`：

```
P243 STALL  位置停在 25278 ms（已 5000 ms 未推进）
P243 RESUME 停 16002 ms 后从 25278 ms 接上，现在 41959 ms
P243 TAIL   state=PLAYING pos=119646 ms buffering=true err=
PASS 断网时上报重连中状态
PASS 断网期间会话没有判死
PASS 观察到断网造成的停顿
PASS 网络恢复后继续播放
5/5 PASS
```

`buffering=true` 证明停顿期间确实上报了"正在重连中"，界面文案有数据来源。

## 二、界面两张（平板 MRR-W29 / 2560×1600）

| 文件 | 证明的事 |
| --- | --- |
| `s4-group-tablet.png` | 「本地与存储」组内依次是 本地影片 / SMB / 私密区 / **小米路由（副标题 `192.168.5.1/XiaoMi`）**；「媒体服务器」组只有 Library-A / Library-B。**SMB 源没有混进媒体服务器组** |
| `s5-address-chip-tablet.png` | 添 SMB 表单顶部「可用地址」胶囊显示 `192.168.5.197  N1` —— 与 Rex 的发现结果完全一致。**S5 的 mDNS 发现第一次在真机上验证通过**（此前 instrumented test 被 MIUI 后台限制挡住） |

## 三、截图抓到的一个真 bug（已修）

第一版截图里胶囊文字是**字面量** `${candidate.host}  ${candidate.label}` ——
Kotlin 字符串模板被写成了 `"${'$'}{...}"`（转义过多，`$` 没被解释），
点它还会把地址填成 `smb://${candidate.host}`。
改为字符串拼接后重拍，`s5-address-chip-tablet.png` 是修复后的画面。

**这个 bug 只有截图能发现**：编译通过、测试通过、日志正常。

## 四、第三张（浏览页点文件起播）未完成：服务器侧变了

| 时刻 | 现象 |
| --- | --- |
| 00:15 K40 断网测试 | 匿名读 19.5 GB 4K 正常，播到 119.6 s |
| 00:20 平板连接 | `服务器不允许匿名访问，请填写账号和密码` |
| 00:21 PC 复核 | `Test-Path \192.168.5.1\XiaoMi` → **拒绝访问**；`net view` → 系统错误 1702 |

→ **不是我们的 bug**：同一时间 PC 也被拒，说明小米路由的 Samba 关掉了匿名访问（或共享被移除）。
我们的错误文案恰好是准确的（`ACCESS_DENIED` + 匿名凭据 → 提示填账号密码），这一条反过来被验证了。

要补第三张，需要满足其一：①路由重新开匿名访问；②给我一个可用的 SMB 账号密码。

---

# 死代码清理 + 本地播放回归（2026-11-10 00:26，平板 MatePad）

## 一、删掉的死代码（`PlayerComposeActivity`）

243 把 SMB 播放切到 `SmbByteSource → RangeFd` 后，下面四个字段已经没有写入方，
`positionPoll` 里约 40 行分支永远进不去：

| 字段 | 原用途 | 现状 |
| --- | --- | --- |
| `smbFfSession` | 220 的 FfmpegPlaybackSession 分支 | **从未被赋值**，整段分支是死代码 |
| `smbStarted` | 上面分支的"是否已起播"标记 | 只在那段死代码里被读 |
| `smbSource` | S3 前指向 `SmbReader` | 恒为 null（曾导致投屏判断失效） |
| `smbProxy` | 220 的 `LocalRangeProxy` | 恒为 null |

一并删掉：`onSeek` 里的 `?: smbFfSession?.seekTo(...)`、`closeNetworkPlayback` 里的
proxy/source 提取与 cleanup、`onDestroy` 里的 `smbFfSession` 收尾。
`grep smbFfSession|smbSource|smbProxy|smbStarted` 现在只剩局部变量 `smbSourceId` 与一条注释。

## 二、本地播放回归（截图）

`local-playback-after-cleanup.png`：平板播放 `local-hevc.mp4`，
画面正常、状态"正在播放 · 片源没有音轨"、时间码 **0:00:04 / 0:00:05**。
→ 删掉那段分支后，位置轮询（`playbackSession` 分支）仍然工作。

## 三、顺带修正 244 的假设

244 原来怀疑"没有音轨时钟就不推进"。这张截图推翻了一半：
**这个 MP4 就是无音轨的，位置照样从 0 走到 4 秒**。
所以"无音轨"本身不是原因，剩下的嫌疑集中在 **MKV 的 FFmpeg 解封装路径**
（与 240 卡"MKV 只出 2 帧 / pts 读数矛盾"同族）。
定案仍需"带音轨 MKV / 无音轨 MKV"对照，但方向已经从"时钟"转向"demux"。

---

# SMB 不可用的准确原因（2026-11-10 00:35）

前面记的"服务器不允许匿名访问"只是表象。继续查到达命令层：

```bash
net use \192.168.5.1\XiaoMi     → 发生系统错误 67（找不到网络名）
net view \192.168.5.1           → 发生系统错误 1702（绑定句柄无效）
Test-NetConnection 192.168.5.1:445 → True（端口是通的）
```

**结论：路由器上名为 `XiaoMi` 的共享已经不存在**（USB 存储被拔 / 共享改名 / 路由重启后配置丢失），
不是权限问题。libsmb2 在匿名账号下连接一个不存在的共享时，服务器回的是 `STATUS_ACCESS_DENIED`，
所以 App 侧显示"服务器不允许匿名访问"——文案没错，但**根因在环境**。

要恢复验证（浏览→起播、SMB 上的 H.264 MKV），需要：
1. 在路由后台确认共享是否还在、叫什么名字；
2. 若改名了，把新名字告诉我；若没了，重新挂载 USB 存储。

---

# 后续：244 已关闭（2026-11-10 06:56）

上文 §「MKV seek 单开一张卡」与 §「顺带修正 244 的假设」是当时的判断，**原样保留不改写**。
它们指向的那张卡现已收口：

- **244 状态：CLOSED**。两张症状两套根因，都已在 `docs/evidence/p244/` 取证：
  1. **起播黑屏 / 位置恒 0** → `FfmpegDemuxSource::select_stream()` 只给 HEVC 装了 `hevc_mp4toannexb`，
     H.264 没有转换，MediaCodec 吞掉全部 AVCC 样本（`samples=600 frames=0 speed=37.83x` 指纹）。
     补 `h264_mp4toannexb` 后 `frames=313`、`first_frame=15.8 ms`。
  2. **seek 后进度不跳** → 视频无音轨时进度取自 `SystemClockSource` 墙钟绝对量，
     seek 只 `pacer_->reset()` 不挪 `origin_ns_`。补 `rebase(landing)` 后 `actual 5129 → 27080 ms`。
- **对上文判断的更正**：原文说"方向已经从时钟转向 demux"——这个转向是对的（起播那个症状），
  但**没有覆盖 seek 那个症状**；seek 不跳确实是时钟基准问题（缺 `rebase`），两条根因同时存在。
- **SMB 链路 seek 已验（2026-11-10 07:02）**：本文开头记的「`XiaoMi` 共享不存在」需要更正——
  共享**只是改名了**，现在叫 **`视频`**（`net use` 有 `N:` → `\\192.168.5.1\视频`，匿名可读写）。
  把本地复验的同一份文件传上去做同源对照，经 SMB 走 `RangeFd`：无音轨 `actual=27012`、
  带音轨 `actual=10498`，均 4/4 PASS。证据 `docs/evidence/p244/seek-smb-link.txt`。
- **教训**：「共享不存在（错误 67）」要区分**「名字变了」**和**「服务没了」**。本次是前者，
  却让复验卡了很久。下次先 `net use` 看现有映射，再去怀疑服务器。

---

# 新发现：K40 的 SMB 匿名会话被服务器拒绝（2026-11-10 07:06，**与 244 无关**）

## 一、现象

同一份 APK、同一路由共享 `\\192.168.5.1\视频`、同样空账号：

- **平板 MRR-W29（Android 12 / EMUI）**：匿名连接成功，seek 复验 4/4 PASS（连跑两次一致）。
- **K40 M2012K11AC（Android 13 / MIUI）**：`SmbFailure: 服务器不允许匿名访问`，
  栈顶 `SmbReader.open:36` = `nativeConnect` 返回非 0，**卡在会话建立/树连接，进不到播放**。

**不是 244 的回归**：K40 根本没走到 `session_seek`，与 `rebase` 改动无关。

## 二、排除过程（每条都有实测）

| 假设 | 实验 | 结论 |
| --- | --- | --- |
| `am` 参数中文被转码 | 两台 `echo 视频` 比对字节，均 `350 247 206 351 242 221`（同一 UTF-8） | ❌ 排除 |
| 路由器 SMB 会话数上限（PC 已占 2 条映射） | K40 失败后**立刻**在平板复跑，仍 4/4 PASS | ❌ 排除 |
| 路由器按 IP 缓存旧会话 | K40 关/开 WiFi（`svc wifi disable/enable`），IP 仍 .152，重连仍失败 | ❌ 排除 |
| 共享名 `视频` 不存在/写错 | 平板用**不存在的共享名** → 报「找不到这个共享」（`BAD_NETWORK_NAME`） | ✅ 文案能区分，故 K40 的 `ACCESS_DENIED` 是**真的被拒**，不是名字问题 |
| 空用户名 null session 不被接受 | K40 加 `-e smbUser guest` → 文案变成「这个共享不允许当前账号访问」 | ❌ 换用户名无效，仍 `ACCESS_DENIED` |
| K40 的 SMB 栈坏了 | K40 连本机 Windows 共享 `kivo`（ASCII 名） | 也是 `ACCESS_DENIED`，但 **Windows 默认禁 guest**，属预期，**不能作为反例**（session setup 先失败，到不了树连接） |

## 三、目前能确定的

1. K40 的网络与 SMB 栈可达服务器（有 NT 状态返回，不是超时/连不上）。
2. 服务器**对平板放行匿名、对 K40 回 `ACCESS_DENIED`** —— 是**设备相关**，不是共享名、不是会话数、不是参数编码。
3. 两台差异只剩 SMB 协议层细节：客户端 NetBIOS 名、dialect 顺序、NTLMSSP 标志位。

## 四、下一步（需要一次带调试的构建，未做）

`libsmb2` 有 `LIBSMB2_DEBUG`，需要重编带调试日志的 so，对比两台在
`negotiate → session setup` 阶段的报文差异。**这是新卡的工作量，不塞进 244**。

## 五、顺带发现的文案问题（产品侧）

`SmbError.translate` 在 `anonymous == true` 时，把 `ACCESS_DENIED` **一律**译成
「服务器不允许匿名访问，请填写账号和密码」。实际同一个 `ACCESS_DENIED` 可能来自：
会话建立被拒、树连接被拒、共享 ACL 拒绝。用户按提示去填账号，可能填了也没用。
（`BAD_NETWORK_NAME` 走的是另一分支，文案是准的——本次已实测验证。）

## 六、更正（2026-11-10 07:24，原文保留不改）

上面整节的**现象描述没错，但归因全错**，以本条为准：

1. 先更正一处事实：补打 `SmbFailure.detail` 后，`smb2_get_error()` 原文是
   `Tree Connect failed with (0xc0000022)`——**匿名会话已建立成功**，被拒的是树连接。
   所以"匿名会话被拒"这个标题不准确。
2. **真因是路由器按「设备 + 共享」分别授权**，两台设备看到的共享列表不同
   （`SmbShareEnumInstrumentation` 实测）：

   | 设备 | 枚举到的共享 | 能连 | 被拒 |
   | --- | --- | --- | --- |
   | 平板 | `TDDOWNLOAD,下载,共享,图片,文档,视频,音乐`（7 个） | `视频` | `XiaoMi` |
   | K40 | `XiaoMi`（1 个，整盘共享） | `XiaoMi` | `视频`、`音乐` |

   两台都能用小米路由，只是入口不同。我先前只在 K40 上试了平板的入口名 `视频`，
   就把"入口不对"误判成"设备被区别对待"。
3. 按各自被授权的共享复测，两台都通过：
   - K40：`OPEN 视频/kivo-test/local-seek.mkv` → `POSITION 0 → 2922 ms`
     → `SEEK 30000/25000/27006` → `RESUME 27006 → 30006`，**4/4 PASS**
   - 平板：同一文件走共享 `视频`，`actual=27047`，**4/4 PASS**
   - 更下层（枚举 → 列目录 → 随机读头尾各 1 MiB）：K40 17 项 / 平板 5 项，**两台均 3/3 PASS**
4. 上面第 3 条"两台差异只剩协议层细节"与第 4 条"需要带 `LIBSMB2_DEBUG` 的构建"
   **一并作废**。基于误判写出的 245 卡与证据目录已删除。
5. 教训（已进项目长期记忆）：**共享名不能跨设备套用**。这台路由对不同设备下发不同的
   共享列表，排障第一步永远是**在该设备上枚举**，拿它自己能看到的那个共享名去连。

## 七、S5（自动发现）真机验收：已结清

`SmbDiscoveryInstrumentation`（`_smb._tcp` 浏览）在两台设备上均 **2/2 PASS**：

| 设备 | 结果 |
| --- | --- |
| K40 `b6e950ee`（Android 13 / MIUI） | `FOUND 192.168.5.197:445 N1`，2/2 PASS，**不再崩溃** |
| MatePad `CFE6R21B12000179` | `FOUND 192.168.5.197:445 N1`，2/2 PASS |

之前记录的"K40 息屏状态下三次 `Process crashed`"是**息屏阻塞**造成的，设备唤醒后复跑即通过。
两台都只发现 NAS `N1`、未发现小米路由——与 2026-11-09 用 PC 侧 zeroconf 得到的结论一致：
**小米路由不广播 `_smb._tcp`**，所以它必须走"手填/枚举"路径，自动发现只对 NAS 有效。

上面第 5 节提的文案问题仍作为观察项保留，但**不再是阻塞项**——它不是本次失败的原因。

## 八、平板真实 UI 播不了：已定位并修复

**症状**：平板走真实 UI（资源库 → 小米源 → 视频 → kivo-test → 点片）时，
不是播放而是**黑屏播放器 + 左上角「这个资源库还没有选择共享」+ 时间恒 `0:00:00`**。
同一文件走 instrumentation 是 4/4 PASS，所以问题不在协议/解码/网络。

**根因**：平板这个源是「枚举共享」形态（记录里 `share` 为空）。用户在页内选完共享后，
选择只存在 `Activity` 内存里——既没写回源记录，也没随 Intent 传给播放器；
播放器回源记录解析地址必然 `needsShare == true`，直接拒绝。K40 的源手填了 `XiaoMi`，故一直能播。

**修复**：`SmbBrowserActivity` 记录 `activeShare` 并随 `kivo.smbShare` 传给播放器；
`PlayerComposeActivity.startSmbPlayback` 增 `shareOverride`，非空时覆盖地址里的 `share`。
凭据仍只走加密存储、不进 Intent（D5 未变）。

**真机验证（平板）**：`local-seek.mkv` 出画面并推进到 `0:00:13 / 0:01:00`；
`mkv-audio.mkv`（AAC）播到结尾；播放中拖进度条，画面自带秒表 `4 → 30`，随后续播到 `0:00:59`。
详见 `s3-tablet-playback-fix.txt`。

**顺带修掉**：`SmbError` 的 CONNECT 阶段按 `smb2_get_error()` 原文细分
（`libsmb2.c:703` / `:894`），未授权共享现在归 `共享` 并明说"没有对本机开放"，
不再一律提示"请填写账号和密码"。已授权共享 4/4 PASS 零回归。

**未做**：页内选的共享没有写回源记录，下次进同一源仍需再选一次（改 `SourceStore` 风险高于收益）。
