# 303 · 字幕闭环：PGS 已通，SRT/ASS 修好了（根因是漏设 `osd-fonts-dir`）

状态：**三条通路均闭环（PGS 位图 / SRT 文本 / ASS 特效）**
设备：K40 `b6e950ee`（SM8250 / A13）
时间：2026-11-16 03:24–03:50
片源：`奥德赛 1080p AV1`（4 条 PGS，60.44 fps）+ `shelter.mkv`（SMB）+ 外挂 `probe.srt`

## 一、PGS 位图字幕：已闭环（275 §5.4 的未闭合项）

| 步骤 | 结果 |
| --- | --- |
| 轨道枚举 | `● Subs --sid=1 '中上英下' (hdmv_pgs_subtitle) [default]`，共 4 条 |
| 字幕真实区间 | ffprobe 扫包：**33.450 s 起**（首包 13447 B），36.787 s 清除（不是 253 那部片的 13.6 s） |
| seek 到 34 s 截图 | **「一支舰队… / a fleet…」清晰显示**（中上英下样式） |

⇒ **PGS 在 gpu-next 路径下能上屏**。253 P6-2 记的"未闭环"是旧口径；
更早的"OSD 画到看不见的位置"假设**不成立**（至少这条路径上不成立）。

**教训**：先确认"这一刻本来有没有字幕"再谈渲染。本轮第一次截图（16 s）拿到的是片头 Logo，
如果不查 PGS 包区间就会误判成"字幕坏了"。

## 二、SRT/ASS 文本字幕：找到根因并修复

### 2.1 现象

```
[osd/libass] Setting up fonts...
[osd/libass] can't find selected font provider
[osd/libass] fontselect: failed to find any fallback with glyph 0x0 for font: (Noto Sans CJK JP, 400, 0)
```

对应画面：**完全没有文字**（不是方块、不是乱码，是空的）。

### 2.2 根因（两层，第二层才是真的）

**第一层（误导项）**：`can't find selected font provider`

我们的 libass 编译时 `CONFIG_FONTCONFIG` 未定义（`config.h` 实读，coretext/directwrite 也没有），
于是 `font_constructors[]` 里只剩 `{ASS_FONTPROVIDER_NONE, NULL, NULL}`，
`ass_fontselect_init` 的循环一次都不执行 ⇒ `default_provider == NULL` ⇒ 打这句警告。

**但这句警告是无害的**：libass 的 `ass_embedded_fonts_add_provider()` 会
**无条件**扫描 `lib->fonts_dir` 并逐条打 `Loading font file`，**与 fontconfig 无关**。
所以「没有 provider」不等于「没有字体」。

**第二层（真根因）**：`osd-fonts-dir` 没设

- `sub-fonts-dir` 设置成功，且读 `options/sub-fonts-dir` **能读回正确路径**（所以"看配置"看不出问题）；
- 但 mpv 的 `mp_ass_init()` 读的是 **`osd_style_opts.fonts_dir`**（字幕由 OSD 层统一渲染），
  不是 `sub_style` ⇒ 我们只设了 `sub-fonts-dir` ⇒ libass 拿到的 `fonts_dir` 是空；
- 结果：**一条 `Loading font file` 都没有**，`fontselect` 找不到任何字体。

`mpv_sub_options.cpp` 的注释里已经写了「`osd-font` 与 `sub-font` 都要设」，
但**漏了同样要设的 `osd-fonts-dir`**。

### 2.3 修复（一行）

`mpv_sub_options.cpp` 的字幕选项组加：

```cpp
options.push_back({"osd-fonts-dir", dir.c_str(), true});
```

### 2.4 修复后验证

```
[osd/libass] Loading font file '.../files/subfonts/NotoSansCJK-Regular.ttc'
[osd/libass] Loading font file '.../files/subfonts/NotoColorEmoji.ttf'
[osd/libass] can't find selected font provider          ← 仍在，但已证无害
[osd/libass] fontselect: (Noto Sans CJK JP, 400, 0) -> NotoSansCJKjp-Regular, 0, NotoSansCJKjp-Regular
```

`fontselect` 从 **failed** 变成 **成功匹配 `NotoSansCJKjp-Regular`**。

画面级验证（SMB 路径 + 外挂 `probe.srt`）：
`probe sub-add probe.srt added=true` → `probe sub-select id=6 ok=true`
⇒ 截图底部显示 **`KIVO-MPV-SUBTITLE-PROBE-AAAA`**，白色清晰、位置正确。

## 三、附带确认

- `SubtitleFontSet` 的符号链接工作正常：
  `[subfonts] 字体目录=/data/user/0/com.kivo.cinema/files/subfonts 族名=Noto Sans CJK JP 链接=[NotoSansCJK-Regular.ttc, NotoColorEmoji.ttf]`
- 新增的诊断日志（`[subfonts] 内核 sub-fonts-dir=… 期望=… 匹配`）会常驻，
  下次字体出问题时**第一眼就能看到**是"选项没生效"还是"字体没加载"。

## 四、排查过程中踩的两个坑（都是我自己造成的）

1. **拿错片源时间点**：先用 16 s 截图，那时本来没字幕 ⇒ 差点误判 PGS 坏了。
   **必须先用 ffprobe 扫出字幕包的真实区间**。
2. **验证路径选错**：`kivo.subFile` / `kivo.subtitleId` 只在 **SMB 路径**
   （`PlayerSmbStart:193 → applyProbeSubtitle`）生效；走 `kivo.playurl` 的 URL 路径
   **根本不会执行**这段 probe。第一次用 URL 路径测，自然"什么都没发生"。
   另外 `kivo.subFile` 是**相对 `filesDir` 的文件名**（`File(filesDir, subFile)`），
   传绝对路径会拼成不存在的路径。

## 四之二、ASS 特效字幕：闭环

用 `artifacts/p240-player-ui/v3-checks/kivo-fx-test.ass`（四类特效一次覆盖）挂到 SMB 链路：

```
probe sub-add kivo-fx-test.ass added=true
probe sub-select id=6 ok=true
```

画面实测（`ass-fx.png`）：

| 特效 | 标签 | 结果 |
| --- | --- | --- |
| `\an8` 顶部对齐 | `ASS 特效测试 · 顶部固定` | ✅ 出现在画面顶部 |
| `\pos(960,760)` + `\c&H00FFFF&` | `黄色 · 定位 pos` | ✅ 位置正确、**黄色** |
| `\frz20\fscx120\fscy120` | `旋转缩放 frz` | ✅ **明显倾斜约 20°、字号放大** |
| `\k60` 卡拉 OK | `卡拉OK karaoke` | ✅ 正常渲染 |

⇒ **libass 的定位、着色、旋转、缩放、卡拉 OK 标签全部生效**。
（`Arial` 在本机不存在，libass 自动 fallback 到 `Noto Sans CJK JP` —— 字形正常，说明 fallback 也工作。）

## 五、边界

1. 已覆盖 **PGS（内嵌位图）/ SRT（外挂文本）/ ASS（定位·着色·旋转·缩放·卡拉OK）** 三类；
   **滚动弹幕与复杂 \t 动画**只有测试片自带轨的旁证，未做逐项断言。
2. 文本字幕只在 **SMB 路径**做了画面级验证；URL/本地路径的 `applyProbeSubtitle` 不执行，
   但字幕渲染本身与路径无关（同一 mpv 会话）。
3. 未测**中文 SRT**（`probe.srt` 是 ASCII）。不过字体已匹配到 CJK，且 PGS 中文、ASS 中文均显示正常。
4. 未测**字幕样式/位置设置**（`sub-pos`、`sub-align-*`）的实际效果。

## 六、复算命令

```bash
export PATH="/c/Android/sdk/platform-tools:$PATH"; export MSYS_NO_PATHCONV=1
A="adb -s b6e950ee"

# 0) 先定位 PGS 真实区间（不然会拍到没字幕的时刻）
F="//NAS/12T/电影/[奥德赛].../....mkv"
ffprobe -v error -select_streams s:0 -show_packets \
  -show_entries packet=pts_time,size -read_intervals 0%150 -of csv=p=0 "$F" | head

# 1) PGS：URL 路径 + seek 到 34s
$A shell am start -n com.kivo.cinema/.ui.compose.PlayerComposeActivity \
  --es kivo.playurl "http://127.0.0.1:8134/v" --ei kivo.seekMs 34000

# 2) 文本字幕：**必须走 SMB 路径**（probe 钩子只在那里）
$A shell am start -n com.kivo.cinema/.ui.compose.PlayerComposeActivity \
  --es kivo.sourceId seed-smb-xiaomi --es kivo.smbShare XiaoMi \
  --es kivo.smbPath "视频/kivo-test/shelter.mkv" \
  --ei kivo.subtitleId 2 --es kivo.subFile "probe.srt" --ei kivo.seekMs 14014

# 3) 判据
$A shell "run-as com.kivo.cinema cat files/mpv-kernel.log" | grep -iE "Loading font file|fontselect"
#   期望：2 条 Loading font file + fontselect -> NotoSansCJKjp-Regular
$A shell "run-as com.kivo.cinema cat files/evidence/p220-status.log" | grep "probe sub"
#   期望：sub-add ... added=true / sub-select id=6 ok=true
```

## 七、证据文件

| 文件 | 内容 |
| --- | --- |
| `pgs-34s.png` | PGS 字幕「一支舰队… / a fleet…」 |
| `pgs-16s.png` | 反例：16 s 片头本来没字幕（差点误判） |
| `srt-failed.png` / `srt-clean.png` | 修复前 SRT 全空 |
| `srt-fixed2.png` | 字体已加载（URL 路径，无 probe） |
| `srt-smb.png` | **修复后 SRT 上屏 `KIVO-MPV-SUBTITLE-PROBE-AAAA`** |
