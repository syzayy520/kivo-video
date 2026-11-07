# 230 · DLNA 投屏 —— 设计稿

> 状态：**设计完成，实现开工**。M3 第一卡。
> 卡片原文：DLNA 投屏（SSDP+AVTransport，学 SenPlayer DLNAClient）P2。
> 参照：SenPlayer `com.mlibkit.dlna.DLNAClient`（04 蓝图 S5，dex 实证）；
> Rex 无 DLNA（iOS 走 AirPlay）——**UI 锚点空缺，按 Rex 05 号规格的 sheet 行样式补位，此处明示**。
> 测试渲染端：本 PC（192.168.5.142）跑 `tools/dlna_renderer.py`（SSDP 响应 + AVTransport 端点 + ffplay 出画面）。

---

## 一、一句话目标

**播放器里点投屏 → SSDP 扫出局域网渲染器 → 选一台 → 本地/远端视频在它上面播——
SenPlayer 同款"小屏推大屏"，协议自研零第三方。**

## 二、证据与选型

| 项 | 证据/决策 | 理由 |
| --- | --- | --- |
| 功能锚 | SenPlayer dex 有 `com.mlibkit.dlna.DLNAClient`（04 蓝图 S5） | 投屏是一等功能 |
| 实现选型 | **自研 SSDP + AVTransport**（蓝图给的另一选项是 Cling） | 协议面就四个动作（Set/Play/Pause/Stop+Seek），Cling 依赖重且停更；与 EmbyClient 同风格零第三方 |
| UI 锚 | Rex 无 DLNA（iOS 用 AirPlay，Rex 拆解无对应键）——按 Rex 05 规格 sheet + 白圆角行样式 | **空缺明示，不装作有锚** |
| 测试渲染端 | PC 最小 MediaRenderer（`tools/dlna_renderer.py`：ssdp:alive 周期广播 + M-SEARCH 应答 + ffplay） | 真机链路里"接收端"这一侧可控、可取证 |

## 三、设计决策

### D1 发现：SSDP M-SEARCH（MediaRenderer）+ 被动 alive 双路径
UDP 组播 `239.255.255.250:1900`，`ST: urn:schemas-upnp-org:device:MediaRenderer:1`，`MX: 3`；
本机 UDP 口收单播应答，去重（USN），扫描窗口 4 s。**M-SEARCH 应答为主路径，同窗口顺带收
`ssdp:alive` 广播为副路径**（实测 PC 测试机上 Windows SSDPSRV 服务与第三方渲染器抢 1900 组播，
alive 是它唯一可达路径——2026-11-08 真机预验证）。Android 收组播必须先拿
`WifiManager.MulticastLock`（WiFi 默认滤组播），故 manifest 加 `CHANGE_WIFI_MULTICAST_STATE`；
M-SEARCH 应答是单播，不受此限。渲染端身份（USN）去重后进设备列表。

### D2 描述：设备 XML → AVTransport 控制点
GET `LOCATION` → `<friendlyName>` + `<serviceList>` 里找
`serviceType == urn:schemas-upnp-org:service:AVTransport:1` 的 `<controlURL>`；
相对 URL 以 LOCATION 为基 resolve。解析用 XmlPullParser（平台内置，零依赖）。

### D3 控制：SOAP AVTransport 四动作
POST controlURL，头 `SOAPACTION: "urn:schemas-upnp-org:service:AVTransport:1#<Action>"`，体标准
SOAP envelope：
- `SetAVTransportURI(InstanceID=0, CurrentURI=<url>, CurrentURIMetadata=<最小 DIDL-lite>)`
- `Play(InstanceID=0, Speed=1)` —— Set 之后必跟
- `Pause(InstanceID=0)` / `Stop(InstanceID=0)` / `Seek(InstanceID=0, Unit=REL_TIME, Target=hh:mm:ss)`
响应 `<s:Body>` 里带 `Fault` 即失败——失败 reason 用户可见（G9）。

### D4 媒体 URL 三来源（谁去取流：渲染端）
| 片源 | 投给渲染器的 URL | 说明 |
| --- | --- | --- |
| 本地文件 | `http://<手机WiFi IP>:<port>/kivo` —— **LocalRangeProxy 加 `bindAddress` 参数**绑 `0.0.0.0` | 新增 `FileByteSource` 实现既有 `ByteSource` 接口，代理链复用 |
| SMB/远端 ByteSource | 同上（`SmbByteSource` 直接喂代理） | 220b 链路原样复用 |
| Emby/WebDAV http(s) URL | **URL 直传**（渲染端自己向 Emby 取流，手机不中转） | 渲染端与 Emby 都在 LAN/WAN 可达时最优 |

`LocalRangeProxy` 只加一个默认值为 `"127.0.0.1"` 的构造参数——旧行为不变，投屏时绑 `0.0.0.0`。

### D5 UI：投屏按钮 + 设备 sheet
`PlayerControls` 底栏加「投」按钮（与「弹」开关同排同规格）；点开 BottomSheet：
扫描中/设备列表（friendlyName + host）/空态文案。点设备 → CastController 接管；
投屏中按钮变绿。样式 token 全走 05 号文档（0xFFF2F2F7 / 白圆角行 / 0xFF34C759）。

### D6 会话语义
投屏成功后**手机端本地播放暂停**（渲染端成为主屏）；状态文本显示"投屏中：设备名"。
返回/Stop 时停渲染端播放、关 LAN 代理。

## 四、家族树

| 文件 | 新/旧 | 单一职责 |
| --- | --- | --- |
| `dlna/DlnaDevice.kt` | **新** | 渲染器数据类（friendlyName/host/controlUrl/usn） |
| `dlna/SsdpClient.kt` | **新** | M-SEARCH 扫描 + 应答去重（同步阻塞，调用方备线程） |
| `dlna/AvTransportClient.kt` | **新** | 设备描述解析 + SOAP 四动作（HttpURLConnection） |
| `dlna/CastController.kt` | **新** | 会话编排：扫描→选中→取 URL 策略（D4）→Set+Play→Stop |
| `net/FileByteSource.kt` | **新** | 本地文件的 ByteSource 实现 |
| `net/LocalRangeProxy.kt` | 旧（小改） | 构造参数加 `bindAddress`（默认 127.0.0.1，行为不变） |
| `ui/compose/PlayerControls.kt` | 旧（小改） | 「投」按钮 + 设备 sheet（Rex 05 token） |
| `ui/compose/PlayerComposeActivity.kt` | 旧（小改） | 接线：按钮→CastController；投屏中状态文本 |
| `AndroidManifest.xml` | 旧（小改） | `CHANGE_WIFI_MULTICAST_STATE` 权限（MulticastLock 前置） |
| `tools/dlna_renderer.py` | **新（测试件，不入 app）** | PC 最小 MediaRenderer：SSDP 广播/应答 + SOAP 端点 + ffplay |

## 五、验收判据

| # | 判据 | 通过条件 |
| --- | --- | --- |
| AC1 | 发现 | K40 M-SEARCH 扫到 PC 渲染器（friendlyName 正确） |
| AC2 | 投本地 | 本地 1080p 文件（LAN 代理 URL）投到渲染器，ffplay 出画面 |
| AC3 | 投远端 | Emby 直连流 URL 直传渲染器，ffplay 出画面 |
| AC4 | 控制 | Pause/Stop 生效（渲染端日志可证）；校验器 5+ 变异自检 |

## 六、明确不做（v1）

事件订阅（GENA Subscribe/NOTIFY，进度回传）· 多房间分组 · 镜像推流（screen cast）·
DIDL-lite 完整元数据（只给最小 title）· 渲染端音量控制（RenderingControl 服务）·
与播放进度条的双向联动（投屏后手机进度条隐藏）。
