# 310 · HLS master 选档（真实流从 960x540 提升到 1920x1080）

状态：**已落地并验证（15 单测 + 2 真机场景）**
设备：K40 `b6e950ee`
时间：2026-11-16 05:16–07:10

## 一、解决的问题（用户可见的画质问题）

309 发现：真实 HLS master（Apple / Netflix / Emby fMP4）的每个码率档是**独立 program**，
而 **mpv 没有 `--program` 选项** ⇒ 内核只能播**第一个** program。
Apple 官方流的第一个是 **960x540** —— 也就是**一直在播低档，而且没人知道**。

这和 ABR 无关，纯粹是"**没选档**"。SenPlayer 也没做（它请求里写死 `VideoBitrate`），
但它至少能拿到自己指定的那一档；我们连档都没选。

## 二、做法：代理层把 master 裁成单档

HLS 请求本来就经过我们的代理（Java 层做 TLS，内核无 TLS 后端），所以**代理能看到 master 内容**：

```
内核 ──GET master.m3u8──> 代理 ──GET──> 源站
                           │ 1. 解析 variant 列表（24 档）
                           │ 2. 按策略选一档（设备能解 + 不超屏 + 带宽装得下）
                           │ 3. 只留那一档，返回裁剪后的 master
                           ▼
                     内核对着一份只有 1 个 variant 的 master ⇒ 不存在 program 选择问题
```

### 家族树（新增独立 `hls` 包，不依赖任何业务包）

```
hls/
├── HlsVariant.kt        — 一条 variant（uri / 带宽 / 宽高 / codecs）+ videoCodec 提取
├── HlsMaster.kt         — isMaster / parse / keepOnly（纯文本，无 IO）
├── HlsVariantPolicy.kt  — choose（纯函数，可 JVM 单测）
└── HlsForwardPolicy.kt  — 策略参数打包（上层构造，代理执行）
```

接线：`PassthroughSource` 带 `hls` 字段 → `LocalRangeServer` 转发时传入 →
`LocalRangeResponder.respondForward` 对 **playlist** 读全量、判断、改写。

分片（`.ts`/`.m4s`）**继续流式转发**，只有 `.m3u8` 才读全量（几 KB，可接受）。

## 三、验证

### 3.1 JVM 单测 15/15 通过

| 用例 | 钉住什么 |
| --- | --- |
| `parse reads BANDWIDTH not AVERAGE-BANDWIDTH` | `BANDWIDTH=` 是 `AVERAGE-BANDWIDTH=` 的子串，错了会偏一个数量级 |
| `parse reads resolution and codecs with comma` | `CODECS="avc1.640020,mp4a.40.2"` 里的逗号不能当属性分隔符 |
| `keepOnly keeps one variant and preserves EXT-X-MEDIA` | 删了 `EXT-X-MEDIA` 会把音频组弄坏 |
| **`real apple master parses all 24 variants`** | 用真实 6291 字节 master 断言 **24 档**（防"解析漏档"） |
| `real apple master keepOnly yields single variant` | 裁剪后必须只剩 1 档且 URI 正确 |
| `policy picks highest affordable within budget` | 预算内取最高 |
| `policy falls back to smallest when all exceed budget` | 全超预算退最小 |
| `policy refuses when decoder gate rejects everything` | 全不可解 ⇒ 不裁（交给内核） |

### 3.2 真机：Apple 多档流

```
[hls] 选档（maxHeight=1243 maxBps=12000000）：带宽内最高档：1920x1080 avc1 8.2 Mbps
PROXY 直通 ok len=1735（原 6291） master.m3u8     ← 24 档裁成 1 档
[13.481] Decoder format: 1920x1080                 ← 改前是 960x540
```

- 子清单请求从 **10+ 个降到 2 个**（不再把 8 个 variant 全拉一遍）；
- 画面截图 `sp-apple-1080p.png` 正常。

**像素量 4 倍**（960×540 → 1920×1080）。

### 3.3 真机：单档流回归（不受影响）

```
PROXY 直通 ok len=200（原 200） v2.m3u8    ← 长度不变 = 没裁剪
[3.137] Decoder format: 1920x1080
```

单档 playlist（Emby 转码流就是这种）`Choice(-1)` ⇒ 原样返回，**行为与改前完全一致**。

## 四、踩到的坑（值得记）

**`maxHeight` 用屏幕短边会挡掉 1080p。**

K40 实测 `displayMetrics` 短边是 **1036**（扣掉状态栏/导航栏），而 1080p 档需要 1080
—— 差 **44 像素**就被闸门拒了，结果选了 720p。

第一次真机验证日志：

```
[hls] 选档（maxHeight=1036 …）：带宽内最高档：1280x720 avc1 3.4 Mbps（已排除 9 档）
```

**修法**：`maxHeight = 屏幕短边 × 1.2`（容差）。理由不是"凑数"，是**超采样后降采样更清晰**
—— 1080p 内容在 1036 高的屏上确实更好看；而 4K（2160）远超 1.2×1036≈1243，仍被挡掉。

## 五、边界（不许夸大）

1. **这不是 ABR**。整个播放**只选一次档，之后不换**。网速中途变差不会自动降档
   —— 与 SenPlayer 的做法一致（305/311 逆向：它也没有运行时 ABR）。
2. **只对 HLS 生效**。MP4/MKV/SMB 路径完全不经过这段代码（`PassthroughSource` 只用于 `.m3u8`）。
3. **只覆盖"master 有多档"的流**。单档流行为不变。
4. **带宽预算 12 Mbps 是固定值**，没有起播前探测。慢源上仍可能选到 8 Mbps 档然后卡
   —— 这是"一次性决策"的固有代价，真要解决得上 ABR（310 设计稿的另一半，未实施）。
5. **首次起播多一次远端请求成本**：master 现在要代理读完再返回（原本也是代理转发，只是改成读全量），
   6291 字节的差可忽略。
6. **未测**：Emby 真实转码流（多档 master 形态？Emby 大多是单档）、鉴权 HLS、
   `#EXT-X-KEY` 加密流的裁剪安全性。

## 六、复算命令

```bash
export JAVA_HOME="C:/Android/jdk-17.0.13+11"; export ANDROID_HOME="C:/Android/sdk"
export PATH="/c/Android/sdk/platform-tools:$PATH"; export MSYS_NO_PATHCONV=1
cd android

# 1) 纯函数单测（含真实 Apple master）
./gradlew :app:testDebugUnitTest --offline --tests "*HlsMasterTest"
# 期望：tests=15 failures=0

# 2) 真机：多档流应被裁成单档并选 1080p
./gradlew :app:assembleDebug --offline
adb -s b6e950ee install -r app/build/outputs/apk/debug/app-debug.apk
adb -s b6e950ee shell am force-stop com.kivo.cinema
adb -s b6e950ee shell "run-as com.kivo.cinema sh -c ': > files/net-diag.log'"
adb -s b6e950ee shell am start -n com.kivo.cinema/.ui.compose.PlayerComposeActivity \
  --es kivo.playurl "https://devstreaming-cdn.apple.com/videos/streaming/examples/img_bipbop_adv_example_fmp4/master.m3u8"
sleep 28
adb -s b6e950ee shell "run-as com.kivo.cinema cat files/net-diag.log" | grep "\[hls\] 选档"
# 期望：带宽内最高档：1920x1080 … ；master len=1735（原 6291）
```

## 七、改动文件

| 文件 | 改动 |
| --- | --- |
| `hls/HlsVariant.kt` | **新增**（数据 + videoCodec 提取） |
| `hls/HlsMaster.kt` | **新增**（解析 / 裁剪，纯文本） |
| `hls/HlsVariantPolicy.kt` | **新增**（选档决策，纯函数） |
| `hls/HlsForwardPolicy.kt` | **新增**（策略参数） |
| `net/PassthroughSource.kt` | 加 `hls` 字段（默认 null = 旧行为） |
| `mpv/LocalRangeResponder.kt` | `respondForward` 对 playlist 读全量 + 裁剪；新增 `isPlaylistUrl` / `rewriteIfMaster` |
| `mpv/LocalRangeServer.kt` | 两处转发传 `source.hls` |
| `ui/compose/PlayerUrlStart.kt` | 新增 `hlsPolicy()`（短边 ×1.2 / 12 Mbps / AV1 闸门） |
| `test/.../HlsMasterTest.kt` | **新增**，15 个断言（含真实 master 资源） |
| `test/resources/apple-master.m3u8` | **新增**，Apple 官方 master 留档（6291 B） |
