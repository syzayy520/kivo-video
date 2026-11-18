# D12-J 状态：容器兼容性矩阵（8 种格式）

日期：2026-11-18
上游：`D12I-STATUS.md`（Content-Type 修复）未覆盖项 #1「`.avi`/`.flv`/`.m2ts` 未逐一验证」

---

## 一、矩阵（当前版本，K40）

同一段素材（`testsrc` 1280x720@25 + sine 440，10 s）分别封装成 8 种容器，逐个本地起播：

| 扩展名 | 内核识别结果 | 视频轨道 | 结论 |
|---|---|---|---|
| `.mp4` | `mov,mp4,m4a,3gp,3g2,mj2` | 1280x720 | ✅ |
| `.mkv` | `Matroska` | 1280x720 | ✅ |
| `.ts` | `mpegts` | 1280x720 | ✅ （`D12I` 修复前 ❌） |
| `.m2ts` | `mpegts` | 1280x720 | ✅ |
| `.mov` | `mov,mp4,m4a,3gp,3g2,mj2` | 1280x720 | ✅ |
| `.avi` | `avi` | 1280x720 | ✅ |
| `.flv` | `flv` | 1280x720 | ✅ |
| `.webm` | `Matroska` | 1280x720 | ✅ |

**8/8 全部识别成功，全部有正常视频轨道。**
（`.webm` 被识别成 `Matroska` 是正常的——webm 是 matroska 的子集。）

---

## 二、这个矩阵能证明什么、不能证明什么

### 能证明

`D12I` 的 `Content-Type: application/octet-stream` 修复**没有破坏任何一种容器**，
且 `.ts` / `.m2ts` 这两个 mpegts 家族从"必失败"变成"正常"。

### 不能证明（诚实标注）

**修复前只实测过三种**：`.ts` 失败、`.mp4` 成功、`.mkv` 成功。
`.m2ts` / `.mov` / `.avi` / `.flv` / `.webm` **修复前没测过**，所以我不能声称
"修复前它们也坏"或"修复前它们就好"。

要严格给出"修复前各格式状态"，需要 checkout `f9d70ded^` 重新编译再跑一遍矩阵。
本批没做——它不影响"修复是安全的"这个结论，只影响"影响面有多大"的描述。

---

## 三、仍然没解释的机制问题

**为什么修复前 `.mp4` / `.mkv` 能播，只有 `.ts` 不能？**

代理当年对**所有**片源都回 `Content-Type: video/x-matroska`。如果 ffmpeg 真的
"按 mime 排他地只挑 matroska 探测器"，那 `.mp4` 也该失败。但它没失败。

可能是：
- mpv 自己在 `http://127.0.0.1:PORT/1` 这种无扩展名 URL 上另有 demuxer 选择逻辑；
- 或 ffmpeg 对 mime 只是"提示"而不是"锁定"，mpegts 失败另有原因（例如需要更多连续数据、
  或 `av_probe_input_format3` 的 score 阈值）。

**没深究**。结论只到"改成 octet-stream 后 8/8 通过"这一层，不往上推机制。

> 这条留给以后：如果再遇到"某格式播不了"，先跑这个矩阵确认是普遍问题还是单格式问题，
> 再去翻 ffmpeg 的 probe 逻辑，别又从外围猜起（`D12F/G/H` 的教训）。

---

## 四、回归脚本（可复现）

样本不入库（体积 + `.gitignore` 忽略 `docs/evidence/**/*.mp4`），生成命令：

```bash
SRC='-f lavfi -i testsrc=duration=10:size=1280x720:rate=25 -f lavfi -i sine=frequency=440:duration=10'
ffmpeg -y $SRC -c:v libx264 -preset ultrafast -pix_fmt yuv420p -c:a aac -shortest s.mp4
ffmpeg -y $SRC -c:v libx264 -preset ultrafast -pix_fmt yuv420p -c:a aac -shortest s.mkv
ffmpeg -y $SRC -c:v libx264 -preset ultrafast -pix_fmt yuv420p -c:a aac -f mpegts s.ts
ffmpeg -y $SRC -c:v libx264 -preset ultrafast -pix_fmt yuv420p -c:a aac -f mpegts -mpegts_m2ts_mode 1 s.m2ts
ffmpeg -y $SRC -c:v libx264 -preset ultrafast -pix_fmt yuv420p -c:a aac -f mov s.mov
ffmpeg -y $SRC -c:v mpeg4 -q:v 5 -c:a mp3 -f avi s.avi
ffmpeg -y $SRC -c:v libx264 -preset ultrafast -pix_fmt yuv420p -c:a aac -f flv s.flv
ffmpeg -y $SRC -c:v libvpx-vp9 -b:v 1M -c:a libopus -f webm s.webm
```

逐个起播并抓识别结果：

```bash
for f in mp4 mkv ts m2ts mov avi flv webm; do
  adb shell am force-stop com.kivo.cinema
  adb shell run-as com.kivo.cinema rm -f files/mpv-kernel.log
  adb shell am start -n com.kivo.cinema/.ui.compose.PlayerComposeActivity \
    --es kivo.media /storage/emulated/0/Movies/s.$f
  sleep 8
  adb shell run-as com.kivo.cinema cat files/mpv-kernel.log | grep "Detected file format"
done
```

---

## 五、证据清单

| 文件 | 内容 |
|---|---|
| `d12j-matrix.txt` | 8 种容器的原始识别结果（探针脚本直接输出） |

---

## 六、未覆盖

1. **修复前各格式状态未实测**（只测过 mp4/mkv/ts 三种），见 §二。
2. **机制问题未解释**（为何只有 mpegts 被 mime 卡住），见 §三。
3. 只测了"能识别 + 有视频轨道"，**没测播放时长/音画同步/seek**——矩阵只回答"打得开"。
4. 编码覆盖面有限：avi 用 mpeg4、webm 用 vp9，其他用 h264。**编码 × 容器组合未穷举**。
