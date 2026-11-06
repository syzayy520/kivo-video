# kivo video 新产品蓝图：「Rex 的 UI + SenPlayer 的核心」

> 依据：两份拆解报告（`01-dv-pipeline-teardown.md` / `03-senplayer-teardown.md`），全部条目标注证据来源。
> 决策（用户拍板，2026-11-06）：**UI 学 Rex（声明式、体验），核心与功能学 SenPlayer（播放能力、生态）**。
> 目标平台：Android 首发 → iOS → TV → 桌面。

## 一、技术架构定案（基于两家实证）

```
┌────────────────────────────────────────────────────────┐
│                 C++ 核心（跨平台一份）                    │
│  解码(MediaCodec/FFmpeg) · 时钟/节拍(P16已闭环) ·         │
│  DV 映射(libplacebo Vulkan) · 字幕(libass)              │
├────────────────────────────────────────────────────────┤
│  Kotlin Multiplatform：队列/媒体库逻辑/设置/同步状态       │
├──────────────────────┬─────────────────────────────────┤
│ Android: Compose     │ iOS: SwiftUI（对齐 Rex 实测模式） │
│ + Compose for TV     │ 桌面: Compose Desktop（后置）     │
├──────────────────────┴─────────────────────────────────┤
│  视频层永远零拷贝：SurfaceView(安卓)/CAMetalLayer(iOS)    │
│  —— 两家竞品共同的架构红线【01§五 / 03§四】               │
└────────────────────────────────────────────────────────┘
```

## 二、从 SenPlayer 抄的核心能力清单（实证来源：03 号报告）

| # | 能力 | SenPlayer 实证 | kivo 落地方案 | 优先级 |
| --- | --- | --- | --- | --- |
| S1 | 进度条缩略图 | `PreviewReader$Still`（独立出帧器，与主链路解耦） | 独立 `ThumbnailReader`：MediaCodec 独立实例低分辨率批量出帧，缓存到 LRU | **P0** |
| S2 | FFmpeg 只做解码、渲染另起一层 | FFmpegKit 裁剪版（滤镜链全裁）【已验证】 | 印证我们"MediaCodec/FFmpeg 解码 + libplacebo 渲染"拆分正确 | 架构原则 |
| S3 | SMB 直连 | `com.mlibkit.smb.SMBClient` | Android 侧 smbj 或 JCIFS-NG；KMP 化后双端共享协议逻辑 | P1 |
| S4 | 网盘/WebDAV | `gcd.WebUploader` + 网盘上传页 | WebDAV 先行（kivo 自有云服务可直连），阿里云盘/百度盘按需 | P1 |
| S5 | DLNA 投屏 | `com.mlibkit.dlna.DLNAClient` | Cling/自研 SSDP+AVTransport | P2 |
| S6 | 后台播放 | `BackgroundPlaybackService`（前台服务） | MediaSession + Foreground Service | **P0** |
| S7 | 下载管理 | `download + Service/JobService` | WorkManager + 分块下载 | P2 |
| S8 | 文件管理（缩略图墙） | `image×7 + libraryFolder/FileVideo...`（Rex Assets 同款） | MediaStore + 自建索引 | P1 |

## 三、从 Rex 抄的 UI/体验清单（实证来源：01/02 号报告）

| # | 能力 | Rex 实证 | kivo 落地方案 | 优先级 |
| --- | --- | --- | --- | --- |
| R1 | 三合一信息架构 | 播放器+媒体库(Drivemby)+网盘【02§3.3】 | 首页=媒体库 / 播放器沉浸式 / 来源管理三分区 | **P0** |
| R2 | 声明式 UI 全覆盖 | SwiftUI（2424 处引用，零 storyboard） | Compose 全量，`AndroidView` 只包 SurfaceView | **P0** |
| R3 | 弹幕 | `rex.playback.danmu.imported`【02§3.3】 | DanmakuView 或 Compose Canvas 自绘，B 站 xml/protobuf 格式 | P1 |
| R4 | 双字幕/图形字幕冲突处理 | `player.subtitle.image_selection_conflict` | 字幕轨管理 + libass 渲染，PGS 冲突提示 | P1 |
| R5 | 新手引导 | TipKit×80 | Compose 提供 `Tip`组件或自研浮层 | P2 |
| R6 | 私密区 | FaceID 权限声明【02§3.1】 | Android: BiometricPrompt + 隐藏空间 | P2 |
| R7 | 订阅制 | StoreKit×166 / `Rex PRO` | Google Play Billing；国内渠道另议 | P2 |
| R8 | webhook/媒体库整理 | Drivemby + 入库通知【02§3.3】 | 对接 Emby/Jellyfin API（扫描/入库/通知） | P2 |
| R9 | 推送 | `remote-notification` + Rex Push | FCM/厂商通道，播放器场景优先级低 | P3 |

## 四、kivo 已有资产 vs 待建（不重复造轮子）

**已闭环（P16 期间 21 张卡）**：MediaCodec 解码、AAudio 时钟、seek 显式栅栏、
**pause-resume（真机 0 丢帧）**、性能门禁×8、`position_us` 位置暴露。
→ 直接对应 S6/R2 的底层。

**待建（按蓝图）**：
1. **P0 主体 = Compose UI 骨架**（三分区 IA + 播放控制层 + S1 缩略图 + S6 后台播放）
   —— 这是"从能播到能用"的一步；
2. P1 = 字幕(libass) + SMB/WebDAV(S3/S4) + 弹幕(R3) + 媒体库(S8)；
3. P2 = DLNA(S5) + DV 映射(libplacebo) + 私密区(R6) + 订阅(R7)。

## 五、验收口径（沿用 P16 门禁风格）

- UI 骨架：核心任务完成率（打开→播→暂停→续播→退出 ≤ 4 步）、冷启动首帧（预算 300ms 不变）；
- 缩略图：拖动进度条 1080p 片源缩略图首现 ≤ 500ms【待验证假设】；
- 后台播放：息屏 30min 音频不中断、回前台画面无缝（对齐 P16 pause-resume 判据）；
- SMB：1GB 文件起播时间与本地差 ≤ 2s【待验证假设】。

## 六、风险

1. Rex/SenPlayer 的 UI 视觉参数尚未完全提取（Ghidra 分析进行中；SwiftUI/混淆层限制）——
   蓝图先锁结构与能力，视觉 tokens 等反编译或截图补充；
2. S3/S4 的协议实现量不小，若要 P1 上线建议直接选型成熟库而非自研；
3. iOS 侧 SwiftUI 复刻是独立的月级工程，与 Android 并行推进需要人力决策。
