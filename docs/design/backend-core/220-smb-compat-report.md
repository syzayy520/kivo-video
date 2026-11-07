# 220 · 小米路由器 SMB 兼容性调查报告（按用户六节提示词结构）

> 状态：调查完成度 ~85%，待 libsmb2 自证实验收口。
> 证据目录：`docs/evidence/p220/`（spike JSON × N、jcifs-trace.log、jcifs-trace-pc.log）
> 分析样本：SenPlayer 1.x（`build/rea-senplayer/raw/`）、nPlayer pro（`build/nplayer/`，截断待补）、
> 本播放器（源码 + 自采 trace）。
> 结论先行：**失败阶段 = SMB2 TREE_CONNECT（会话与协商都成功）；字节级同款令牌 PC 通/手机拒；
> SenPlayer 同款库 libsmb2 需在 K40 自证后定案换库。**

## 一、样本与摘要

| 样本 | 摘要/标识 | 来源 |
| --- | --- | --- |
| SenPlayer | `com.wuziqi.SenPlayer`；libMLibKit.so 15.7MB（内嵌 **libsmb2 6.2**：`libsmb2 6.2` 字符串 + SMB2_* 常量 443 处）；dex 单类壳 `com.mlibkit.smb.SMBClient`（native: create/invoke/fileRead/fileWrite/readBlock/writeBlock/progressCore/cancelCore） | 手机→PC 已解包 |
| nPlayer pro | `com.newin.nplayer.pro`；dex 无 SMB 实现（仅 UI `SMBAddPopView`）→ SMB 在 native（arm64 split 未拉全，待补） | 手机 adb pull（截断 63%，dex 已挖出） |
| 本播放器 | `com.kivo.cinema` 0.1.0-p12a+；SMB=kcifs-ng 2.1.38（spike harness：SmbSource + 参数矩阵） | 自研 |

## 二、参考播放器逆向（SenPlayer）

- **实现库**：libsmb2（C，sahlberg）——由二进制证据锁定（SMB2_ 符号族 + 版本串）。非 SMBJ/JCIFS。
- **API 面**（导出符号级）：fileRead/fileWrite/readBlock/writeBlock/progressCore/cancelCore
  → 全功能：块读（随机访问）+ 写 + 进度 + 可取消（jadx SMBClient.java 确认 native 声明）。
- **播放通道**：libMLibKit.so 内嵌 HTTP/1.1 + Content-Range + Accept-Ranges + FFmpegKit 引用
  127.0.0.1 → **本地 HTTP Range 代理**喂播放内核（FFmpegKit 无 libsmbclient，无 smb:// 协议）。
- **凭据处理**：jadx 层拿不到（脚本总线 + 加密串 s1:）；用户实测**只填 IP、账号密码全空**可连。
- 【待验证】libsmb2 匿名 NTLMSSP 的具体 flags/字段（需 Ghidra 反编译 SMB 函数；受串行纪律约束暂缓）。

## 三、当前播放器分析

- 入口：`SmbSource`（jcifs-ng 2.1.38）+ spike harness（参数矩阵 7 变体一次跑完）。
- 已修 bug：URL 双拼路径、NetBIOS 解析（resolveOrder=DNS）、缺 INTERNET 权限（EPERM）。
- 异常处理：因果链 + 全栈进证据 JSON（无吞错）。

## 四、差异证据（字节级）

### 4.1 失败阶段（K40 → 小米路由，全部 7 变体一致）

```
NEGOTIATE  → OK（dialectRevision=768 = SMB 3.0，securityMode=0x1 签名启用非强制，
                 capabilities=0x5 = DFS|LargeMTU，无加密能力）
SESSION_SETUP → OK（NTLMSSP 交换完成，"Context is established"）
TREE_CONNECT  → STATUS_ACCESS_DENIED (0xC0000022)   ← 恒定失败点
```

### 4.2 排除矩阵（K40 + jcifs）

| 变体 | 结果 |
| --- | --- |
| default / signingPreferred / signingEnforced / encryptionEnabled / dialect300 / dialect210 / smb2Negotiation | 全部 TREE_CONNECT 拒 |

### 4.3 决定性对照（同手机、同 Wi-Fi、同字节令牌）

| 实验 | 结果 |
| --- | --- |
| K40 + jcifs → **Windows 共享**（192.168.5.142/kivo，真实账号） | **全通**：浏览两级 + 1MB offset 读 3.9 MB/s |
| K40 + jcifs(GUEST) → 小米路由 | 拒 |
| PC + jcifs(GUEST) → 小米路由 | 通（127 条目） |
| **type-3 令牌逐字节**：PC-NULL(ws_len=0, flags 0x22080205, 全零) vs K40(NULL 同款) | **同字节，PC 过 / 手机拒** |

→ 排除：方言、签名/加密、Android 网络权限（Windows 共享全通）、明文流量问题、用户名密码错误。
→ 剩余变量：**服务器对"匿名会话+该来源"的授权判定**（Mi 路由按设备/客户端形态的访问控制）
或 **NTLMSSP 令牌内未对比的深字段**（NTLMv2 响应、workstation）。PC 的 GUEST 令牌带
NTLMv2（lm24/ntlm252），K40 的 GUEST 令牌是全空——**同 jar 不同形态**【矛盾点，待 libsmb2 自证拆解】。

### 4.4 参照物行为

SenPlayer（libsmb2）：同手机、同路由、**空凭据** → 通（用户实测）。
nPlayer：同路由可用（用户陈述；native 拆解待补）。

## 五、运行时验证

- 本端：jcifs 内部 SMB2 报文 trace 已落盘（jcifs-trace.log 242KB，含完整 PDU hex）。
- 对端（SenPlayer）：手机无 root 无 tcpdump，**SMB 流量抓取不可行**——以"同库自证"替代：
  在 K40 上驱动我们自编译的 **libsmb2**（Nginx… 不对，NDK cmake 已编出 liblibsmb2.a）+
  JNI 桥跑同一 spike 流程。**通过 → 与 SenPlayer 同库同行为，根因链闭合；不通过 → 排除库因素，
  转向路由器设备授权方向（需用户在路由器管理页核对 SMB 匿名/设备白名单设置）。**

## 五点五、播放链路打通（2026-11-07 深夜，K40 实机状态日志）

libsmb2 选型 + 匿名修复后，全链路真机打通（仙草冻.mp4 33MB，小米路由/XiaoMi 共享）：

```
libsmb2 connect ✓ → smb2_open ✓ (size=33056861)
→ LocalRangeProxy 127.0.0.1:42705 ✓（HTTP/1.1 + Range）
→ FFmpeg http://127.0.0.1:42705/kivo 打开 ✓（openUrlVideo rc=0，network+http 重编译版）
→ 软解线程起跑 ✓（start rc=0）
```

途中修掉两处：
1. session open() 的 fd 前置校验挡住 URL 路径 → url 非空即合法（fd 校验仅限 fd 路径）；
2. XiaoMi 共享根 = "视频"共享的根目录——path 需带 "视频/" 前缀
   （首轮 OBJECT_PATH_NOT_FOUND 的根因）。

证据：`docs/evidence/p220/p220-status-log-capture.md`（shell 直读原文；
设备 USB 闪断，整文件拉取与截图待设备回连后补）。

## 五点七、AC4 断网初测 + 剩余清单（2026-11-07 深夜）

- **AC4 初测**：播放中 `svc wifi disable` → 画面冻结（SMB 读阻塞）不崩溃 ✓；
  5 秒后截图存证（p220-smb-disconnect.png）。完整断连错误上报（FFmpeg read
  超时 → end_reason → 用户可见 reason）需下轮深测（当前 end_reason 仅 stop 时出版）。
- **剩余清单**（220 收口前）：① URL 模式 seek（avformat_seek_file，断点续播口径）
  ② positionUs 导出（SMB 路径时间显示恒 0 的缺口）③ AC4 完整断连错误上报
  ④ Rex 表单/浏览 UI ⑤ AC5 1GB 起播差测量。

## 五点八、位置/时长/seek 全套导出（2026-11-07 收官）

双帧对照实证（p1-time/p2-time.png，6s 间隔）：**`0:00:02 / 0:03:46` → `0:00:06 / 0:03:46`**
—— positionUs 随解码帧推进、容器时长 3:46 正确（durationUs 来自 probe_container，
不是文件大小）。

- session：`seekTo`（异步请求 → worker 线程 avformat_seek_file 最近关键帧 +
  解码器 flush×16 + 节拍重锚）/ `positionUs`（渲染帧 media_us 原子出版）/
  `durationUs`（probe 结果）/ `end_reason`（断连上报：连续读失败 >200 次 →
  "error: read failed repeatedly (...)"）+ `completed()` 区分 EOS
- JNI：nativeFfmpegSeekTo/PositionUs/DurationUs；Kotlin 对位
- Activity：SMB 分支接 positionPoll（位置/时长/断连全绿路径）+
  PlayerControls seek 路由（smbFfSession 优先，本地会话回退）
- 连续读失败阈值 200 次的依据：单次 read 失败可能是瞬时抖动（proxy 重试），
  200 次 ≈ 数秒持续断网；end_reason 在 poll 的 isRunning=false 时经
  stopAndCollectEvidence() 出版到证据 JSON 并展示在 statusText（G9 ✓）

## 六、结论与最小修复（待定案）

| 假设 | 证据强度 | 下一步 |
| --- | --- | --- |
| H1 路由器拒 jcifs 的 SPNEGO/匿名形态，接受 libsmb2 形态 | 中（排除矩阵 + SenPlayer 行为） | **libsmb2 自证实验（进行中）** |
| H2 路由器按设备/来源授权，PC 在白名单、手机仅 SenPlayer 的形态被豁免 | 弱（与 SenPlayer 成功矛盾） | 若 H1 不通则核对路由器 SMB 设置 |
| H3 jcifs-on-Android 的 NTLMSSP 令牌有缺陷（workstation/v2 字段缺失） | 中（PC GUEST 带 v2、K40 GUEST 全空——同 jar 异行为，待解释） | libsmb2 实验后复查 |

**最小修复候选**（按证据强度排序，均限 SmbSource 内部，不动网络模块其余部分）：
1. SmbSource 后端换 libsmb2（SenPlayer 同款；H1 成立时这是"复现参照物行为"而非换库赌博）；
2. jcifs 令牌形态补丁（补 workstation/NTLMv2，H3 成立时）；
3. 路由器设置修正（H2 成立时，用户操作）。

**禁止项**（用户提示词）：未定案不重写网络模块、不动无关功能；全程真机复算留证。
