# P243 · SMB 参考取证（status）

> 采集时间：2026-11-09 22:35–22:44（GMT+8）
> 设备：iPhone13,3（iPhone 12 Pro），iOS 27.2（BuildVersion 24B115），USB 直连
> 工具：`pymobiledevice3 11.50.3`（本机临时 venv，未写入项目依赖）
> 目的：为 `docs/design/backend-core/243-smb-rebuild-design.md` 提供"其他播放器怎么做 SMB"的一手证据

## 一、截图清单

| 文件 | App | Bundle ID | 版本 | 页面 |
| --- | --- | --- | --- | --- |
| `rex-add-smb.png` | Rex | `tv.rexnow.app` | 1.1 (7) | 添加资源库 · SMB 表单 |
| `eplayerx-smb-form.png` | EplayerX | `com.snowlee.eplayerx` | 10.0.22 (80) | SMB 表单 + 媒体库背景 |
| `noctra-add-smb.png` | Noctra | `cn.zijiu522.noctra` | 1.0.1 (26006) | 添加 SMB 全屏页 |
| `forward-add-source.png` | Forward | `flux.inchmade.app` | 1.3.18 | 添加资料库（类型列表） |
| `vidora-add-source.png` | Vidora | `date.wwmm.vidora` | 1.0.6 (28) | 添加来源（仅 Plex/Emby/Jellyfin/MP，无 SMB） |
| `denplayer-my.png` | DenPlayer | `com.moonsofa.MoonSofa` | 1.0.1 (29) | 我的（观看记录，未进来源管理） |

## 二、核心发现（与设计 D1/D2 对应）

### 2.1 没有任何一家要求用户填"共享名"

| App | SMB 表单字段 |
| --- | --- |
| Rex | 可用地址（自动发现）/ 服务器 / 端口 / 用户名 / 密码 —— **无共享** |
| EplayerX | 名称 / 备注 / **地址 `smb://nas.local/video`** / 用户名（可选）/ 密码 —— 共享写在 URL 里 |
| Noctra | 名称（选填）/ 备注（选填）/ **完整地址 `smb://主机/共享名`** / 用户名（选填）/ 密码（选填） |

→ Kivo 现行"主机/端口/共享/路径/用户/密码"六字段里，**"共享"是唯一一个用户无法自证的字段**。

### 2.2 三家都把"可选"写在界面上

- EplayerX：用户名占位"可选"；脚注"匿名访问可留空用户名"。
- Noctra：分组标题直接写"来源信息 · 选填""访问账号 · 选填"；用户名占位"留空表示匿名访问"。
- Rex：`library.form.namePlaceholder` = "资源库名称（自动获取）"。

### 2.3 Rex 拆解包（非截图）提供的硬参数

来源 `C:\Users\Administrator\Pictures\REX\rex\rex\rex\Rex_拆解成果_落地包\strings\zh-Hans__Library.json`
（460 键）。与 SMB 相关的键值已摘录进设计稿 §2.1.1，其中三条产品规则：

1. 去重键 = 服务器 + 账号（`library.form.duplicate`）。
2. 粘贴识别要能解析"链接 + 用户名 + 密码"（`library.form.smartRecognitionPlaceholder`）。
3. 共享是独立实体（`library.drivemby.shareMissing`）。

## 三、服务器侧共享枚举实证

| 文件 | 命令 | 结论 |
| --- | --- | --- |
| `net-view-192.168.5.1.txt` | `net view \\192.168.5.1` | ✅ 返回 6 个磁盘共享：TDDOWNLOAD / 下载 / 共享 / 图片 / 文档 / 视频 / 音乐 |
| `net-view-192.168.5.197.txt` | `net view \\192.168.5.197` | ⚠️ 系统错误 1707（该实现不支持此枚举路径，需 SMB2 `IPC$` 复算） |

两台服务器 TCP 445 均 OPEN。

## 四、未取得的部分（不冒充）

- 连接成功后的共享列表页、目录浏览页、播放页布局 —— 需要真机再操作一次。
- Vidora / DenPlayer 的 SMB 入口 —— 本轮未找到（Vidora 添加页只有媒体服务器；用户确认可跳过）。
- iPhone 端 App 的网络抓包 —— 设备无 root，不可行；以"同库（libsmb2）行为对齐"替代。

## 五、复算方法

```powershell
# 本机临时环境（不写入项目）
python -m venv $env:TEMP\iosvenv
$env:TEMP\iosvenv\Scripts\pip install pymobiledevice3
$env:TEMP\iosvenv\Scripts\pymobiledevice3 usbmux list
$env:TEMP\iosvenv\Scripts\pymobiledevice3 developer dvt screenshot out.png
```

首次需在 iPhone 上点"信任此电脑"；iOS 17+ 由工具自动建 userspace tunnel，无需 root。
