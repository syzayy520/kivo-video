# Rex 1.0.2 UI 拆解（阶段三·第一部分）：rea 能拆到什么、SwiftUI 的边界、已获证据

> 状态：进行中。本文回答"UI 能不能在 rea 里拆出来"——答案是**部分能，且有一个我此前判断错误**。

## 一、rea 的 UI 拆解能力实测（修正之前的结论）

| 命令 | 实测结果 | 说明 |
| --- | --- | --- |
| `decode-interface-builder` | ✅ **Windows 可用** | rea 自带 `rea-interface-builder/1` 纯静态解码器，不依赖 macOS。我此前推断它依赖 macOS 是**错的** |
| `inspect-asset-catalog` | ❌ `unsupported binary format` | 不认 Assets.car |
| `inspect-plist` / `inspect-macho` | ❌ 依赖 macOS 原生工具 | 自己用 Python plistlib / llvm 解析 |
| `analyze` / `trace` / `search` | ❌ 需要深度 provider | Windows Ghidra 只支持 PE；Hopper 无 win 版；IDA/Kuna 未配置 |

`decode-interface-builder` 实际产出（`Launch Screen.storyboardc`，2 个 nib，对象图完整）：

- `UIView` 全屏，背景 = 资源色 `RexLaunchBackground`（NSRGB = "1 1 1"，白）
- `UIImageView`：logo 图 `RexLaunchLogo`（1×1 占位），tint = `RexLaunchLogoTint`（黑）
- 约束：logo 居中，**宽 76 × 高 76**（`NSConstantV2: 76`），`UIContentMode: 1`（scale to fit）
- 证据：`decode-interface-builder` 输出的 nodes/edges，`evidence_id: ev_e5d28f...`

## 二、结构性边界：SwiftUI 应用没有可反编译的界面文件【已验证】

`UILaunchStoryboardName = "Launch Screen"` 是全 app **唯一**的 storyboard（iOS 硬性要求）。
主体 UI 是 SwiftUI（二进制中 SwiftUI 框架引用 2424 次，全场最高）。

SwiftUI 没有 storyboard/nib——界面树是**编译后的 Swift 代码**。任何静态工具（包括 rea）
都拿不到声明式 UI 层级；这不是工具缺陷，是 SwiftUI 的产物形态。Rex 主模块的
Swift 符号也被 strip（`_$s` 只剩 KSPlayer/JXPhotoBrowser 等外部模块名）。

**SwiftUI 应用的 UI 拆解只能走四条侧路**（本文已走通三条）。

## 三、已获得的 UI 证据（三条侧路）

### 3.1 Info.plist——产品形态【已验证】

| 字段 | 值 | 含义 |
| --- | --- | --- |
| Bundle ID | `tv.rexnow.app` | 官网 rexnow.tv |
| 最低系统 | **iOS 17.0** | 放弃旧设备，敢用新 API |
| URL scheme | `rex://` | 外部唤起（WebDAV/分享导入入口） |
| 后台模式 | `audio` + `remote-notification` | **后台播放** + 推送唤醒 |
| 权限声明 | FaceID、**本地网络**、Bonjour、照片写入、**语音识别** | SMB/网盘发现、私密区、（语音搜索？【未知】） |
| ATS | 有自定义 NSAppTransportSecurity | 允许自建服务的明文 HTTP【推断】 |

### 3.2 框架引用计数——技术栈与功能暗示【已验证】

`StoreKit ×166`（内购/订阅）、`CloudKit ×98`（**iCloud 同步**）、`TipKit ×80`
（新手引导气泡）、`Network ×216`（本地发现）、`JXPhotoBrowser ×143`（图片预览）、
`CryptoKit ×132`。

### 3.3 本地化文案——功能面【已验证】

en/zh-Hans 各 22 条（**只本地化了通知类文案**，主 UI 硬编码英文【推断】）。关键条目：

- `Drivemby 整理完成` / `%@ webhook 已连通` / `电影已入库` / `第 %@ 季第 %@ 集已入库`——
  **媒体库自动整理 + webhook 双向通知**（Emby/Jellyfin 风格）；
- `rex.playback.danmu.imported` / `invalid_file`——**弹幕**（导入本地弹幕文件）；
- `player.subtitle.image_selection_conflict`——**图形字幕双轨冲突**提示；
- `rex.playback.queue.season_format` / `specials`——剧集队列按季组织；
- `Rex PRO` / `Rex Push 测试通知`——**订阅制 + 自建推送**。

### 3.4 Assets 资源名——页面与生态【已验证】

`RexDinoIcon`、`RexHomeSharePackage`、`RexSettingsAboutSprite`、`RexSettingsOrderSprite`、
`libraryFolder / libraryFileVideo / libraryFileImage / libraryFileDocument`（文件管理 UI）、
**`libraryLvlian`（绿联 NAS——私有云生态适配的直接证据）**、深浅色双图标（`UIAppearanceDark`）。

## 四、结论与下一步

**rea 能拆**：storyboard/nib 层级（Launch Screen 全参数）、（在有 macOS 或配好 IDA/Kuna 的机器上）二进制反编译。
**rea 拆不了**（本机）：Assets.car、plist、SwiftUI 的界面树。

**已拆出的功能面全景**：本地/SMB/网盘播放 + 媒体库整理（Drivemby）+ webhook 通知 +
弹幕 + 双字幕（含图形字幕）+ FaceID 私密区 + iCloud 同步 + 订阅制 + 绿联等 NAS 生态 +
后台播放。这是一个"播放器 + 私人媒体库 + NAS 客户端"三合一产品。

**剩余缺口与取数方法**【未知→需补】：
1. 页面布局/视觉参数/交互流程——SwiftUI 静态不可得，需要 **App Store 产品截图**（公开）或真机录屏；
2. 服务器协议细节——抓包（用户操作真机时）；
3. libplacebo/onnxruntime 的实际角色——运行时观察。

建议下一步：抓取 App Store `tv.rexnow.app` 的官方截图做视觉拆解（合法、公开、零工具依赖），
配合本文的功能面证据产出信息架构与设计规范。