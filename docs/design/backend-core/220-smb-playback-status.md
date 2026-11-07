# 220 · SMB 直连播放 —— 状态稿（v1 收口）

> 状态：**v1 收口 2026-11-07 深夜**。六条 AC 全部真机实证（220-smb-compat-report.md §五点九）；
> UI 全链打通（§五点十）。剩余已知问题见 §四，均为后续卡任务。

## 一、交付面

| 层 | 组件 | 状态 |
| --- | --- | --- |
| 协议 | libsmb2（SenPlayer 同款，NDK 编译，无 TLS 内置加密） | ✅ 连接/浏览/读通 |
| 网关 | LocalRangeProxy（HTTP/1.1 + Range，127.0.0.1 随机端口） | ✅ FFmpeg/MediaExtractor 均可接 |
| 解码 | FFmpeg network+http 版（重编译，九轮排障） | ✅ URL 直开 |
| UI | SmbActivity（Rex 0114 表单规格 + 浏览 + 多级导航 + 点播） | ✅ 全链实测 |
| 播放 | 仙草冻.mp4（33MB）真机播放，双帧画面推进 ✓ | ✅ |

## 二、AC 实证（真机，K40 b6e950ee）

六条全过，证据与数字见 220-smb-compat-report.md §五点九。
AC5：SMB 1GB 起播 62ms vs 本地 10ms（差 52ms ≪ 2s）——Windows 共享（机器缓存）实测。

## 三、真机画面证据

- p220-smb-playing*.png：双帧画面推进 + 控制条（时长正确）
- p220-ui-form/browse*/play*.png：UI 表单→浏览→两级导航→点播→渲染 全链
- p220-status.log：阶段耗时（连接+打开 855ms、代理 3ms、探针 2ms、起播 1ms）

## 四、已知问题（诚实清单）

| # | 问题 | 影响 | 后续 |
| --- | --- | --- | --- |
| 1 | ~~软解受限 0.37×~~ → **已由 220b 硬解解决**：NetworkPlaybackSession（MediaExtractor http URL → MediaCodec Surface）+ pts 节拍 → **实测 1.00× 恒定**（12 采样 min 1.00 max 1.01） | 720p SMB 播放实时 ✓ | 220b 已完成（本表更新） |
| 2 | ~~位置显示 0.5×/2.2×~~ → **已解决**：根因是 URL 软解无节拍自由跑（6.0×）或解码受限（0.37×）；硬解 + 节拍后位置显示准确 | 已修 | 无需开发 |
| 2b | **新发现：MediaExtractor 需 cleartext**——Android 9+ 默认禁明文 http，127.0.0.1 本地代理也被拦 | 硬解打不开网络源 | 已修：manifest usesCleartextTraffic（仅本地代理用） |
| 2c | **新发现：硬解码循环需要显式 pts 节拍**——MediaCodec 6.0× 自由跑（软解有 pts sleep、硬解原生无） | 无节拍则音画/进度全错 | 已修：decodeLoop 内 nano 级 pts sleep（标准播放器做法） |
| 3 | 视频/音乐 共享匿名被拒（小米路由共享级 ACL） | 用户需填凭据（表单已支持） | 无需开发 |
| 4 | SmbActivity 非应用入口（adb 启动） | 用户无法从应用内到达 | 222 媒体库卡做入口集成 |
| 5 | AC5 的 Windows 共享走机器缓存 | 数字偏乐观 | 路由器侧复测（冷却缓存后） |
| 6 | 首次连接 855ms 含 WiFi RTT（表单到起播 0.9s） | 可接受 | 无需开发 |

### 节拍曲线仪表（本卡新增）

session 证据新增 `pace_curve`: [[wall_ms, media_s] × N]（每 100 帧采样）。
本轮实测：720p30 SMB wall 9.4/18.2/27.0/35.9s ↔ media 3.3/6.6/10.0/13.3s——恒定 0.36×，
decode 占 wall 68% → **解码受限**，节拍 sleep 逻辑本身无 bug（due > now 才睡，读慢时天然不睡）。
| 3 | 视频/音乐 共享匿名被拒（小米路由共享级 ACL） | 用户需填凭据（表单已支持） | 无需开发 |
| 4 | SmbActivity 非应用入口（adb 启动） | 用户无法从应用内到达 | 222 媒体库卡做入口集成 |
| 5 | AC5 的 Windows 共享走机器缓存 | 数字偏乐观 | 路由器侧复测（冷却缓存后） |
| 6 | 首次连接 855ms 含 WiFi RTT（表单到起播 0.9s） | 可接受 | 无需开发 |

## 五、220 收口判定

v1 范围（浏览/起播/seek/断点续播通道/错误上报）全部有真机实证；
UI 表单/浏览可用；小米路由实机验证通过（匿名 + XiaoMi 共享）。
**正式收口待办**：verify_p220_smb.py 脚本化 + 节拍失效修复（问题 1/2 专卡）
——均为增量，不阻塞 221 WebDAV 开工（ByteSource+Proxy 地基已就绪）。
