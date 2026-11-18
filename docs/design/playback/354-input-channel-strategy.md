# 354 · 播放输入通道策略化（本地文件直连 mpv）

> 触发：D12I 修完 `Content-Type` 后发现，「所有源统一塞进 loopback HTTP 代理」这个架构
> 把 mpv 削平到了最低公共分母。用户裁决：按最强实现，不接受老旧做法。
>
> 本文是**规划**。家族树冻结后才会写生产代码。

---

## 一、现状事实（实测，非推断）

### 1.1 当前链路

```
MediaItem.path（MediaStore DATA 列，绝对路径）
  → FileByteSource(RandomAccessFile)
  → LocalRangeServer.register()          // 绑 127.0.0.1
  → http://127.0.0.1:<port>/v/<id>
  → mpv loadfile
```

`MpvStartSequence.kt:105-106` 是**唯一**的 register/urlFor 点；SMB / HTTP / 本地文件
全部汇到这里（`MpvPlaybackSession(context, input: ByteSource)` 签名决定了这一点）。

### 1.2 代理的真实职责边界（逐个消费者查过）

| 消费者 | 位置 | 是否依赖播放代理 URL |
|---|---|---|
| 主播放 | `MpvStartSequence.kt:105` | ✅ 唯一入口 |
| 预览帧内核 | `MpvThumbnailReader.kt:151` | ✅ 但它注册的是**第二份**源；本地预览根本不走它（走 `ThumbnailReader`） |
| DLNA 投屏 | `CastController.castSource` | ❌ **自己 new `FileByteSource`** + 独立 `LocalRangeProxy`（绑 `0.0.0.0`） |
| 缩略图（本地） | `PlayerLocalStart` 注释已写明 | ❌ 走 `ThumbnailReader` |

⇒ **本地主播放不再需要代理时，没有任何其他代码会断。**

### 1.3 走代理对本地文件造成的三项实际损失

| 损失 | 机制 | 证据等级 |
|---|---|---|
| **同名字幕失效** | mpv 的 `sub-auto` 只看 URL 所在目录，代理 URL 是 `/v/<id>` ⇒ 看不到真实兄弟文件 | ✅ 实证（D12-B2 为此写了 `PlayerSiblingSubtitle` 补丁） |
| **格式识别受响应头支配** | 代理必须回 `Content-Type`，回错就 demuxer 直接死 | ✅ 实证（D12I：写死 matroska 导致全部 `.ts` 播不了） |
| **多一次 socket + 一次全量拷贝** | 文件 → JVM byte[] → 写 socket → mpv 读 socket → 再拷进 demuxer | ◐ 机制确定，收益量级未测 |

第三项是**唯一待量化项**。前两项是功能与可靠性损失，不依赖数字即成立。

---

## 二、架构裁决

### 2.1 「统一代理」不是最强，是「最低公共分母」

统一抽象的好处（一处实现、统一 seek 语义、统一指标）是真实的，但代价是把
**每类源的原生最优能力削平**。mpv 对四类源本来各有更优通道：

| 源 | mpv 原生最优 | 我们现在的做法 | 差距性质 |
|---|---|---|---|
| 本地文件 | `loadfile <abs path>`（file 协议、`sub-auto` 全功能、ffmpeg 原生 seek） | loopback HTTP | **纯粹是损失**，无收益 |
| HTTP / Emby 直链 | `loadfile <url>`（mpv 自带多连接、range、header 透传） | 先代理再喂 | 我们代理更弱，但有 `OriginPrefetchPump` / `WarmupPolicy` 预热收益（未量化对比） |
| SMB | SenPlayer 用 Rust SMBKit → 本地缓存文件 → mpv 读文件 | loopback HTTP | 我们已实测 38.4GB REMUX 零丢帧零卡顿，**无证据说它更差** |
| WebDAV | 同 HTTP | loopback HTTP | 同 HTTP |

### 2.2 分层路线（本批只做 L1）

**L1 · 本地文件直连（本批）**
零风险、纯收益、且能删掉一个我们自己写的补丁文件。判据见 §五。

**L2 · HTTP/Emby 交还 mpv 原生（本批只做评估，不实施）**
必须先回答：mpv 原生 http + 我们放弃代理预热，首帧与卡顿是否更好？
这是**实测题**，不是信仰题。没有数字不动。

**L3 · SMB 改本地缓存文件（不做）**
SenPlayer 那套（Rust SMBKit + 稀疏缓存文件）工程量极大，而我们 SMB 当前
表现已经很好，**没有任何证据**说它更优。做了是拿确定性换不确定性。

⇒ 「最强」的正确形态是**每类源走它的最优通道，且决策集中、可取证、可回退**，
不是把所有源重写一遍。

---

## 三、L1 设计

### 3.1 文件家族树

| # | 文件 | 动作 | 职责（单一） | 预估行数 |
|---|---|---|---|---|
| 1 | `playback/PlaybackInputMode.kt` | **新建** | 输入通道唯一决策点：`DIRECT_FILE` / `LOOPBACK`。与 `PlaybackStack`、`PlaybackRenderMode` 同构。三层优先级 + `Decision(mode, reason)` + `describe()` | ~85 |
| 2 | `playback/DirectFileMedia.kt` | **新建** | 绝对路径 → mpv URI（空格/中文/`%`/`#` 安全）；预检（`isFile` + 可读 + 首 64 KiB 实读）。失败原因枚举 | ~70 |
| 3 | `playback/MpvStartSequence.kt` | 改 | `prepare()` 按 mode 分支：DIRECT 不启代理、不 register；`Prepared` 增加 `directUri` | +30 / −6 |
| 4 | `playback/MpvPlaybackSession.kt` | 改 | `start()` 透传 `directUri`；`server`/`registeredId` 已是 nullable，释放路径不改 | +8 / −2 |
| 5 | `ui/compose/PlayerLocalStart.kt` | 改 | 不再 `new FileByteSource`；改为把路径交给会话；`T_SOURCE_READY` 语义改为「预检通过」 | −8 / +4 |
| 6 | `cpp/mpv/mpv_sub_options.cpp` | 改 | `sub-auto` `fuzzy` → `exact` | +1 / −1 |
| 7 | `ui/compose/PlayerSiblingSubtitle.kt` | **删除** | 被内核原生 `sub-auto` 取代（见 §3.3） | −45 |
| 8 | `ui/compose/PlayerSmbStart.kt` | 改 | 删 `localMediaPath` 参数与 `attachSiblingSubtitle` 调用 | −4 |

**为什么第 3 条要单独建两个文件而不是在 `PlayerLocalStart` 里写 if**

项目已有三个同构决策点（`PlaybackEngine` / `PlaybackRenderMode` / `HdrOutputSwitch`），
`PlaybackStack` 负责聚合。输入通道是同一层的第四个维度 ⇒ 必须同构，
否则就是「两个地方各算一遍」——280 已经为这个错误付过一次代价（HDR Surface 8bit/10bit 打架）。

### 3.2 开关与回退

```
debug.kivo.inputMode  (系统属性)   ← 取证/紧急回退，即时生效，不用重装
   > kivo.inputMode   (intent)
   > 默认 auto（本地文件走 DIRECT_FILE）
```

取值：`auto` / `direct` / `proxy`。`proxy` 强制回旧行为，用于对照实验与出问题时的现场回退。

每次起播必落一行：

```
[input] mode=direct uri=/storage/emulated/0/Movies/x.mkv reason=本地文件可直读
[input] mode=loopback reason=预检失败：文件不可读
```

禁止静默切换：`direct` 预检失败时**回退到代理并留痕**，不回退到黑屏。

### 3.3 字幕：删掉自己的补丁，交还内核

`sub-auto` 当前是 `fuzzy`（`mpv_sub_options.cpp:72`，这是 253 为「网络源看不到兄弟文件」时的
无害设置）。直连后 mpv 能看到真实目录，`fuzzy` 会把**同目录别的影片的字幕**也挂上
（如 `Alita.ass` 挂到 `3 Idiots`）。所以必须同时改成 `exact`。

改 `exact` 后，`PlayerSiblingSubtitle` 的三个劣势全部消失：

| 我们的补丁 | 内核原生 `sub-auto=exact` |
|---|---|
| 只认 ass/ssa/srt/vtt 四种 | ffmpeg 能解的文本格式全支持 |
| 不处理字符编码探测 | libass 自己处理 |
| 不处理内嵌字体附件 | 自动挂 |
| 首帧后才挂（晚一步） | 加载期即挂 |

⇒ 补丁删除，不是「新旧共存」。

### 3.4 为什么不做运行时自动回退

同一次播放里「直连失败 → 重建会话走代理」要拆掉已建的内核、Surface、指标打点，
复杂度高且会产生第二段首帧时间（指标污染）。本地文件的失败原因几乎都能在
**打开前**判出来（不存在 / 不可读 / 读不出），所以用**预检**覆盖，剩余极端情况
交给「日志 + 用户重试 + 开关回退」。这是可靠性换复杂度，且换得值。

---

## 四、风险与处置

| # | 风险 | 处置 |
|---|---|---|
| R1 | 路径可读性因设备而异（scoped storage） | 预检实读 64 KiB；失败自动走代理并留痕。K40 已实证 `RandomAccessFile` 可读同进程 mpv 亦可 |
| R2 | `sub-auto` 改 `exact` 影响 SMB/HTTP | 那两条源看不到兄弟文件，`exact` 与 `fuzzy` 都不挂 ⇒ 行为不变 |
| R3 | 直连后 `demuxer-cache-time` 语义变化 | 本地无网络缓存，该值意义不同；345 契约文档加注，不假装可比 |
| R4 | 投屏 | 已验证 `CastController` 自己 new 源 + 独立 `0.0.0.0` 代理，不依赖播放代理 |
| R5 | 文件名含 `%` | `%` 在 mpv 的 URL 解析里是转义符 ⇒ `DirectFileMedia` 必须做 percent-encode 或走 `file://` 形式；真机造含 `%` 文件验证 |

---

## 五、验证矩阵（每条都要数字或截图，不接受「逻辑上应该」）

| # | 项 | 判据 | 手段 |
|---|---|---|---|
| V1 | 首帧时间 | 同文件 direct vs `inputMode=proxy` 各 3 次，`startup_total` 中位数对比 | `files/metrics/*.json` |
| V2 | 三种容器 | `.mp4` / `.mkv` / `.ts` 均 `Detected file format` + 出画 | 内核日志 + 截图 |
| V3 | 中文 + 空格 + `%` 路径 | 造 `测试 100% 视频.mp4`，能起播 | 真机 |
| V4 | 同名字幕 | 只放 `x.srt` → 自动挂；同目录放 `别的.srt` → **不挂** | 截图 + 内核日志 `Loading external subs` |
| V5 | seek 落点 | 拖到 50% / 90%，`position` 与 HUD 偏差 ≤ 200 ms | 截图 |
| V6 | 续播 | `start=` 生效，从上次位置起播 | 内核 `options="start=..."` |
| V7 | 回退 | `debug.kivo.inputMode=proxy` → 日志出现 `mode=loopback`，正常播放 | NetDiag |
| V8 | 不回归 | SMB 38.4 GB REMUX / Emby / 投屏 / 私密区各起播一次 | 真机 |
| V9 | 门禁 | `assembleDebug` BUILD SUCCESSFUL + `ui_gate.py` PASS | 命令输出 |

**V1 是唯一可能推翻本批的项**：若直连首帧不比代理快（甚至更慢），说明真正的开销不在
这一层，我要如实报出来，而不是把「架构更正确」当成「性能更好」。

---

## 六、不做（附理由）

| 不做 | 理由 |
|---|---|
| 改 SMB 走本地缓存文件 | 无证据优于现状；工程量极大 |
| HTTP/Emby 交还 mpv 原生 | 会丢 `OriginPrefetchPump`/`WarmupPolicy`；必须先量化对比，本批只做评估 |
| 删 `LocalRangeServer` | SMB / HTTP / 预览内核仍在用 |
| 保留 `PlayerSiblingSubtitle` 做双保险 | 新旧共存违反铁律；且内核实现严格更强 |
| 同一次播放的运行时自动回退 | 指标会被两段首帧污染，复杂度不值 |
