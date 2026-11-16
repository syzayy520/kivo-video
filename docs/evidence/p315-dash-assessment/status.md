# 315 · DASH 能力评估（275 §1.4「DASH 完全没碰」的定性）

状态：**评估完成 —— 当前不支持，原因已定位**
设备：MatePad `CFE6R21B12000179`（A12）
时间：2026-11-16 14:58–15:10

## 一、实测：`.mpd` 被当 matroska 解

用 Sony 官方 DASH 测试流（`dash.akamaized.net/dash264/TestCases/1a/sony/...mpd`）：

```
[prep] URL 预备完成 ms=360 verdict=无
[prep] 内核创建完成 ms=26
[d][cplayer] loadfile url="http://127.0.0.1:40115/1?..."
[e][ffmpeg/demuxer] matroska,webm: EBML header parsing failed
[e][lavf] avformat_open_input() failed
```

**关键**：`matroska,webm: EBML header parsing failed` —— 内核把 MPD（XML）当成了
**matroska/EBML** 去解。这只有一种解释：**libavformat 里没有 dashdec 注册**，
于是 probe 阶段没有匹配的 demuxer，最后退回默认探测并失败。

（注意：`[prep]` 层一切正常——代理、起播、闸门都放行了，**失败发生在内核 demux 阶段**。
所以这不是我们的取数层问题。）

## 二、原因：FFmpeg 是精简构建，缺 libxml2

| 证据 | 值 |
| --- | --- |
| APK 内 `libavformat.so` 大小 | **2.5 MB**（完整构建通常 10–20 MB） |
| APK 内是否有 `libxml2` | **0 个** |
| 内核日志 | 无 `dash` demuxer 匹配 |

FFmpeg 的 `dashdec` 依赖 **libxml2**（解析 MPD 的 XML）。
没有 libxml2 ⇒ 配置阶段 `dashdec` 被跳过 ⇒ 运行时没有 DASH 能力。

## 三、结论

1. **DASH 不支持**（实测，非推测）。
2. **不是代码问题，是构建配置问题** —— 补 `--enable-libxml2`（并确保 `dashdec` 不被 `--disable-everything` 排掉）重编 FFmpeg 即可。
3. **修复成本**：重编 FFmpeg + 重新验证 HLS/其它 demuxer 无回归（我们的 mpv 构建链已经跑通过，
   属于「已知流程 + 一次回归」量级，不是探索性工作）。

## 四、要不要做（判断，不是结论）

**建议暂不做**，理由：

- 我们**已有一条可用的自适应路径**（HLS：309 直通 + 310 选档，实测 540p→1080p）；
- 实际片源以 **Emby 转码流（HLS）**、**SMB 原盘（MKV/MP4）** 为主，DASH 不是主流入口；
- 重编 FFmpeg 会动到**所有** demuxer（HLS、MKV、MP4 全在这一个 .so 里），
  收益（多一个容器）与风险（可能引入 HLS 回归）不成比例。

**什么时候值得做**：如果出现明确的 DASH 片源需求（比如接入某个只提供 DASH 的源），
再单独立卡，且**必须带 HLS 回归验证**。

## 五、复算命令

```bash
export PATH="/c/Android/sdk/platform-tools:$PATH"; export MSYS_NO_PATHCONV=1
adb -s CFE6R21B12000179 shell am force-stop com.kivo.cinema
adb -s CFE6R21B12000179 shell "run-as com.kivo.cinema sh -c ': > files/mpv-kernel.log'"
adb -s CFE6R21B12000179 shell am start -n com.kivo.cinema/.ui.compose.PlayerComposeActivity \
  --es kivo.playurl "https://dash.akamaized.net/dash264/TestCases/1a/sony/SNE_DASH_SD_CASE1A_REVISED.mpd"
sleep 18
adb -s CFE6R21B12000179 shell "run-as com.kivo.cinema cat files/mpv-kernel.log" | grep -iE "demuxer|EBML|failed"

# 检查构建里有没有 dashdec 依赖
cd android/app/build/outputs/apk/debug
unzip -l app-debug.apk | grep libavformat        # 2.5 MB = 精简构建
unzip -l app-debug.apk | grep -c libxml2         # 0 = 没有 XML 解析器
```

## 六、证据

内核日志片段（上引）；`libavformat.so` 大小与无 libxml2 为静态事实。
