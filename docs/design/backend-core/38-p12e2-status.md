# P12-E2 验收：HDR 色彩标记的可识别、可下发、不误判 —— 静态元数据这一项未达成

- 版本：V1.0
- 日期：2026-10-30
- 设计：`37-p12e2-design.md`（已按实测加两处更正）
- 判据：**HDR 静态元数据下发（`AMediaFormat` → `setHdrMetadata` 路径）；K40 上 HDR10 片源可识别为 HDR**
- 结论：**前半达成，后半部分达成。**
  - ✅ 片源可识别为 HDR：真机证据里 `color.hdr.kind = "hdr10"`，依据是 `standard=6(BT2020) / transfer=6(ST2084)`。
  - ✅ 色彩三键可下发：`configure_format.copied = true`，`delivered_color_* = 6/6/2`，且 `dataspace.applied = true`、`value = 163971072`（`ADATASPACE_BT2020_PQ`）。
  - ❌ **`hdr-static-info` 没有下发**：轨道格式里这个键根本不存在（两份探针产物均为 `null`），
    mastering display / MaxCLL 只存在 HEVC SEI 里。**这是本卡未达成项，见 §五 第 1 条。**

---

## 一、真机证据（K40 / `b6e950ee` / Android 13 / 高通）

| 文件 | 片源 | `hdr.kind` | `dataspace` | 帧闭合 | 丢帧 |
| --- | --- | --- | --- | --- | --- |
| `p12e2-hdr10-r1.json` | 自造 HDR10（BT2020/PQ，profile 4096） | `hdr10` | applied=true / **163971072** | 300 送 / 301 收 / 300 上屏 | 0 |
| `p12e2-hdr10-r2.json` | 同上 | `hdr10` | applied=true / 163971072 | 同上 | 0 |
| `p12e2-sdr10-r1.json` | `probe_1080p_hevc10.mp4`（10bit SDR、**无任何色彩标记**） | `sdr` | applied=false（SDR 不动） | 同上 | 0 |
| `p12e2-sdr10-r2.json` | 同上 | `sdr` | applied=false | 同上 | 0 |
| `p12e2-probe-hdr10.json` | 探针 A/B（不给窗口） | `hdr10` | — | — | — |
| `p12e2-probe-sdr10.json` | 同上 | `sdr` | — | — | — |

四轮播放全部 `completed=true`、`samples_rejected=0`、`frames_dropped_late=0`。

---

## 二、三个实测发现（都推翻了设计稿的一处）

### 2.1 颜色键常量：`BT601_NTSC = 4`，`SDR_VIDEO = 3`，且**没有** `BT2020_CL`

设计 §2.1 写的是 `standard ∈ {5,6}`。用本机 `android.jar` 反查：

```
javap -constants android.media.MediaFormat
  COLOR_STANDARD_BT709 = 1
  COLOR_STANDARD_BT601_PAL = 2
  COLOR_STANDARD_BT601_NTSC = 4        ← 不是 3
  COLOR_STANDARD_BT2020 = 6
  COLOR_TRANSFER_LINEAR = 1
  COLOR_TRANSFER_SDR_VIDEO = 3          ← 3 是 SDR_VIDEO，不是 SMPTE_240M
  COLOR_TRANSFER_ST2084 = 6
  COLOR_TRANSFER_HLG = 7
  COLOR_RANGE_FULL = 1 / LIMITED = 2
```

`MediaFormat` 里**没有** `BT2020_CONSTANT_LUMINANCE`（那是 `DataSpace` 的 `STANDARD_*`）。
所以判据只认 `standard == 6`。常量表落 `decode/color_key_names.hpp`，
由 `kivo_p12e2_hdr_tests` 的 E2-1 钉住 —— 包括"`standard_name(5)` 必须是 `unknown`"这一条，
防止后人把设计稿里的 5 顺手补成 `BT2020_CL`。

**顺带把 E1 的一个悬案结了**：E1 在无标记片源上看到 `1/3/2`，猜是"解码器默认值"。
本轮拿到了直接证据 —— 同一片源的轨道格式里三个键**全是 `null`**，
而输出格式仍然报 `1/3/2`（见 `p12e2-sdr10-r1.json`）。**确认是默认值，不是片源信息。**

### 2.2 `ANativeWindow_setBuffersDataSpace` 不在 `libandroid.so` 里

设计 §2.3 写的是 `dlsym(RTLD_DEFAULT, "ANativeWindow_setBuffersDataSpace")`。第一轮真机证据是：

```
"dataspace": { "applied": false, "reason": "ANativeWindow_setBuffersDataSpace is not available on this platform (API < 28)" }
```

而设备是 **API 33**。查下去发现符号根本不在 `libandroid.so`：

```
$ strings /system/lib64/libandroid.so        | grep -c ANativeWindow_setBuffersDataSpace   → 0
$ strings /system/lib64/libnativewindow.so   | grep -c ANativeWindow_setBuffersDataSpace   → 1
```

`RTLD_DEFAULT` 只搜**已加载**的全局命名空间，而工程没有链接 `nativewindow` —— 所以必然是 null。
改成 `dlopen("libnativewindow.so")` 拿句柄再查，`RTLD_DEFAULT` 只作兜底；
失败时 reason 会带"试过哪个库、`dlerror` 原话"。

**这条错误的代价值得记下来**：第一版证据里那句 reason 是假的（把 API 33 说成 API < 28），
比没有 reason 更糟。**"取不到"必须报"具体哪一步取不到"。**

### 2.3 色彩键来自 SPS VUI，不是容器盒

自造片源字节级核对：`colr` / `mdcv` / `clli` 三个盒**一个都没有**（`0 0 0`），
但设备端轨道格式读到了 `6/6/2`。来源只能是 **HEVC SPS 的 VUI**（ffmpeg 用
`-color_primaries/-color_trc/-colorspace` 写进码流），`MediaExtractor` 解析 SPS 后暴露成三个键。

这条会改变后续判断：想补静态元数据，去解析 MP4 盒子是死路。

---

## 三、A/B 探针：显式下发在这条路径上"没有可观测差异"

`kivo_hdr_probe` 对同一片源做两次 configure（都不给窗口）：

| 用例 | configure | start | start 后 `standard/transfer/range` |
| --- | --- | --- | --- |
| A 原样传轨道格式 | ok | ok | 6 / 6 / 2 |
| B 复制再写颜色键 | ok | ok | 6 / 6 / 2 |

`verdict.same_color_after_start = true`。

**这不能读成"下发没用"**：A 组本来就已经从 SPS 读到同样的值，两组当然一样。
它能证明的是另外两件更要紧的事：

1. **"复制再写"这份副本平台接受**（B 组 configure + start 都成功）——
   这是设计 §六 那条"复制逻辑怎么验"的答案：不靠假格式单测，靠平台真的吃下这份副本；
2. **颜色键在 ByteBuffer 模式下会被采纳**（输出报 6/6/2 而不是默认 1/3/2），
   与 E1 的 `color-format` 完全不同 —— 那颗组件不校验 `color-format`，但**认** `color-*` 三键。

---

## 四、代码与产物

| 改动 | 位置 | 说明 |
| --- | --- | --- |
| 颜色键常量表（**新**） | `decode/color_key_names.hpp` | 数值来自 `javap android.jar`，注释写明反直觉的三处 |
| HDR 类型（**移**） | `decode/hdr_kind.hpp` | 原在 `present/surface_dataspace.hpp`；提到 `decode/` 是为了不让判定层反向依赖上屏层 |
| 交付事实（**新**） | `decode/hdr_delivery_facts.hpp` | `DataSpaceOutcome` + `HdrDeliveryFacts`，`present/` 与 `session/` 共用 |
| 源标记（**新**） | `decode/source_color_tags.hpp` | 纯数据 + `any()`（"有没有东西可发"的闸门） |
| 格式读写（**新**） | `decode/mediaformat_color_io.{hpp,cxx}` | 唯一碰 `AMediaFormat` 的一层；复制 mime/几何/csd/max-input-size + 颜色键 |
| 分类纯函数（**改**） | `decode/video_color_facts.{hpp,cxx}` | 加 `source_*` / `delivered_*` / `configure_fallback_reason`；加 `classify_hdr()` |
| 下发（**改**） | `decode/mediacodec_video_decoder.cxx` | 有标记时"复制再写"，失败回退原格式并**留 reason** |
| dataspace（**接上**） | `present/surface_dataspace.{hpp,cxx}` | 之前**写了但没进任何构建**；本轮修好符号解析并接进会话 |
| 编排（**改**） | `session/decode_session.cxx` | `configure` 之前分类 + 设 dataspace（顺序是硬要求） |
| 证据（**改**） | `session/decode_stats.hpp`、`decode_stats_renderer.cxx` | schema 升 **`/8`**，`color` 加源/下发字段与 `hdr` 子对象 |
| 探针（**新**） | `probe/hdr_probe_main.cxx` | 设备端可执行 `kivo_hdr_probe`，A/B 两次 configure |
| 单测（**新**） | `tests/video/decode/p12e2_hdr_tests/` | **11 例 / 40 断言**，桌面 `ctest` 全绿 |

`ctest -R kivo_p12` 现在 **8/8**（新增 `kivo_p12e2_hdr_tests`）。

**片源**：`tests/real/probe_1080p_hevc10_hdr10.mp4`（10s / 1080p30 / Main10 / BT2020 / PQ，
由仓库内 FFmpeg SDK 生成，命令见 §七）。内容由 `testsrc2` 合成，
**颜色不准但不影响元数据链路**；这一点在诚实清单里再说一次。

---

## 五、诚实清单（七条）

1. **`hdr-static-info` 一项未达成。** 轨道格式里没有这个键（两份探针都是 `null`），
   所以"静态元数据下发"这件事**本轮没有做成**。做到需要自己解析 HEVC SEI 的
   mastering display / CLL 并构造 blob —— 设计 §五 已把它列为"不做"，
   但判据里写着，就必须在这里明说，不能拿"dataspace 设上了"当替代品。
2. **`dataspace.applied = true` 只表示"我们设进去了"。** 合成器有没有按 PQ 处理，
   要看 SurfaceFlinger 侧（HDR 层标记 / dataspace 回读）。那是 E3 的活。
3. **A/B 没有可观测差异**，所以"显式下发改变了什么"在这台设备上是**测不出来**的。
   本轮能证明的只有"平台接受了这份副本"与"三键被采纳（输出 6/6/2 而非默认 1/3/2）"。
4. **只有一台设备、一种编解码器**（K40 / OMX.qcom.video.decoder.hevc / Android 13）。
   `libnativewindow.so` 这一条是高通+Android 13 的事实，别处可能不同。
5. **片源是合成的**（`testsrc2` 上变换到 10bit HDR 容器）。它带正确的 BT2020/PQ 标记与 SEI，
   但画面内容不是真实 HDR 素材；**本轮不涉及"画面对不对"**，那是 E4。
6. **只测了 MP4/HEVC。** MKV 的 `hdr-static-info` 是否被解封装器暴露，没测。
7. **探针不给窗口（ByteBuffer 模式）**，产物 `scope` 字段写死了这句话；
   播放路径（Surface）的三键来自播放器自身证据（`decode/8`），两者不互相冒充。

---

## 六、给 E3 / E4 的交接

- **E3（BT2020 色域标记）的起点已经就绪**：`color` 段能读源三键、下发三键、输出三键；
  `hdr` 段能读 `kind` 与 `dataspace`。E3 只需要补"合成器认不认"这一环 ——
  `dumpsys SurfaceFlinger` 的层 dataspace、或 `CMD_GET_HDR_OUTPUT`，
  以及"片源标 BT2020/PQ 时输出键是否跟着变"（本轮已给出 6/6/2 的基线）。
- **E4（tone mapping）需要先决定"判据是什么"**：本轮已证明 HDR 内容会被交给合成器，
  在 SDR 屏上会过曝。E4 的第一件事不是写算法，而是定义"回退后的画面正确"怎么测。
- **不要复用 `dlsym(RTLD_DEFAULT)` 去找 NDK 新符号**：先确认它在哪个 `.so` 里
  （`strings /system/lib64/<lib>.so | grep <symbol>`），再 `dlopen` 那个库。
  这一条建议写进 `AGENTS.md` 的"会咬人的坑"。

---

## 七、复算步骤

```bash
# 0. 片源（仓库内 FFmpeg SDK；10s / 1080p30 / BT2020 / PQ + SEI）
./third_party/ffmpeg-sdk/bin/ffmpeg.exe -y -f lavfi \
  -i "testsrc2=size=1920x1080:rate=30:duration=10,format=yuv420p10le" \
  -c:v libx265 -preset veryfast -crf 24 -pix_fmt yuv420p10le \
  -color_primaries bt2020 -color_trc smpte2084 -colorspace bt2020nc \
  -x265-params "colorprim=bt2020:transfer=smpte2084:colormatrix=bt2020nc:\
master-display=G(13250,34500)B(7500,3000)R(34000,16000)WP(15635,16450)L(10000000,1):\
max-cll=1000,400:log-level=error" \
  -movflags +faststart tests/real/probe_1080p_hevc10_hdr10.mp4
./third_party/ffmpeg-sdk/bin/ffprobe.exe -select_streams v:0 \
  -show_entries stream=profile,color_space,color_transfer,color_primaries \
  -of default=nw=1 tests/real/probe_1080p_hevc10_hdr10.mp4
#   → Main 10 / bt2020nc / smpte2084 / bt2020

# 1. 桌面单测
cd build/vs-debug && ctest -R kivo_p12e2 -C Debug

# 2. 探针（A/B 两次 configure）
ADB=/c/Android/sdk/platform-tools/adb.exe
MSYS_NO_PATHCONV=1 $ADB push \
  android/app/build/intermediates/cxx/Debug/<hash>/obj/arm64-v8a/kivo_hdr_probe /data/local/tmp/
MSYS_NO_PATHCONV=1 $ADB shell "chmod 755 /data/local/tmp/kivo_hdr_probe"
MSYS_NO_PATHCONV=1 $ADB shell '/data/local/tmp/kivo_hdr_probe /data/local/tmp/hdr10.mp4 \
  > /data/local/tmp/p12e2-probe-hdr10.json'

# 3. 播放取证（走 APK，必须有真 Surface）
MEDIA=/storage/emulated/0/Android/data/com.kivo.cinema/files/media
MSYS_NO_PATHCONV=1 $ADB shell "am start -n com.kivo.cinema/.ui.PlayerActivity \
  --es kivo.media $MEDIA/hdr10_1080p30.mp4 --ei kivo.pace 1 --ei kivo.drop 1 --ei kivo.fps 30 \
  --es kivo.evidence p12e2-hdr10-r1.json"
MSYS_NO_PATHCONV=1 $ADB shell "run-as com.kivo.cinema cat files/evidence/p12e2-hdr10-r1.json" \
  > docs/evidence/p12e/p12e2-hdr10-r1.json
```
