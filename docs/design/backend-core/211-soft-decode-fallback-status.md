# 211 · 软解兜底 —— 状态（收口）

> 2026-11-07 收口。设计稿与实测更正块：`211-soft-decode-fallback-design.md`
> （§八 复用面定稿、§九 fd 桥定稿先于实现）。
> 判据：`tools/verify_p211_softdec.py`（自检 4 正例 + 7 变异全抓，exit 0）。

## 一、验收总表

| # | 判据 | 结果 | 证据 |
| --- | --- | --- | --- |
| AC1 | RM 容器可播 | ✅ rv10 走软解：150/150 渲染零失败，可见屏实播 | `p211-ffmpeg-rm-visible-k40.json` |
| AC2 | WMV 可播 | ✅ wmv2：150/150/0，completed | `p211-ffmpeg-wmv-visible-k40.json` |
| AC3 | 硬解零回归 | ✅ HDR10 轮 backend=`mediacodec`、301/300/0、completed（行为与 210 基线一致） | `p211-mediacodec-hdr10-k40.json` |
| AC4 | 4K 不走软解 | ✅ 4K60 HEVC10 轮 backend=`mediacodec`、301/300/0 | `p211-mediacodec-4k-k40.json` |
| AC5 | 路由理由 | ✅ asf 轮 `no video track`、rm 轮 `setDataSource failed`、理由原样出版 | 各轮 `routing` 块 |
| AC6 | 门禁 | ✅ 7 份证据全 PASS；自检 7 变异全抓 | `verify_p211_softdec.py` |

桌面：`kivo_p211_fd_callbacks_tests` **29/0**；`kivo_p211_local_session_tests` **12/0**
（端到端：5s 30fps 片 150 帧实时播完）。全量回归 ctest 283 通过 / 0 失败。

## 二、路由真机实证（AC5 的两条理由都各有各的形状）

| 片 | 平台行为 | 路由结果 |
| --- | --- | --- |
| a211wmv.wmv (asf) | `MediaExtractor` **能开但无视频轨** | ffmpeg，reason=`no video track` |
| a211rm.rm | `setDataSource` **直接抛异常** | ffmpeg，reason=`setDataSource failed` |
| a211hdr10.mp4 / a211_4k.mp4 | 视频轨 + HEVC 解码器在 | mediacodec（历史路径零回归） |

**首版路由的真机翻车**：rm 片被误路由进硬解路径然后 open 失败——首版把整个探测体
包进一个大 catch、一律回 mediacodec。判据的两层失败各有归属（容器不可解 → 软解；
匹配异常 → 平台说不行 → 软解），合并成一个 catch 就是把设计稿 D1 的判据改写了。
修复后 rm 正确落软解（教训：判据的"例外"不许比判据本身先写）。

## 三、诚实清单（五条）

1. **RMVB(RV40) 未验**：合成片是 rv10（FFmpeg 无 RV40 编码器）。rv10 与 rv40 同走
   `rm` demuxer + RealVideo 解码族，链路等价；真 RMVB 样片到手补一轮。
2. **节拍是 simple pts sleep**（`pacing` 字段如实出版），与硬解的 FramePacer 不同级——
   软解路径的 av sync 深度对齐不在 M1 范围。
3. **无 seek/pause**：M1 会话不支持，调用即走到但不承诺语义（后续卡）。
4. **单机**（K40）；平板（MRR-W29）装的是路由修复前的 APK，跨设备软解对照待补。
5. **桌面测试退出期 abort 已根治但只在 Windows Debug+DLL 组合复现**——四段二分
   实验记录在 `ffmpeg_local_playback_session_tests.cpp`（--trio-only/--decode-inline/
   --thread-only/--thread-ffmpeg-use）；修复 = FFmpeg 全生命周期在工作线程上
   （单线程持有一切，架构更对，不是绕）。Android 构建是静态链接无 DLL 边界，
   探针/会话在真机从未复现。

## 四、实现落位（家族树）

| 文件 | 职责 |
| --- | --- |
| `third_party/ffmpeg-android/build.sh` + README | FFmpeg 7.1 → arm64 静态库（七个环境坑全记录） |
| `runtime/local_media/ffmpeg_fd_callbacks.{hpp,cxx}` | fd 版 FFmpeg 输入回调（29/0 单测） |
| `runtime/local_media/ffmpeg_frame_sink.hpp` | 呈现接缝接口（无 Android 类型，P12 红线） |
| `runtime/local_media/ffmpeg_render_sink.{hpp,cxx}` | RGBA 直写 ANativeWindow（`anative_window_cpu`） |
| `runtime/local_media/ffmpeg_local_playback_session.{hpp,cxx}` | 旁路会话（FFmpeg 全生命周期在工作线程） |
| `jni/jni_ffmpeg_bridge.cxx` | 软解句柄族 JNI（create/open/start/running/stop/destroy） |
| `bridge/FfmpegPlaybackSession.kt` + `FfmpegRouting` | Kotlin 门面 + 路由判定 |
| `session/decode_stats.hpp` + renderer | 硬解证据 `decoder.backend="mediacodec"`（加字段不升版本） |
| `tools/verify_p211_softdec.py` | 证据门禁 + 变异自检 |

## 五、复算步骤

```sh
python tools/verify_p211_softdec.py --self-test   # 4 正例 + 7 变异
python tools/verify_p211_softdec.py               # docs/evidence/p211/ 全量
ctest -C Debug -R kivo_p211 -C Debug              # fd 回调 29/0（桌面主树）
cmake --preset vs-debug-ffmpeg && cmake --build build/vs-debug-ffmpeg --target kivo_p211_local_session_tests --config Debug
./build/vs-debug-ffmpeg/backend/Debug/kivo_p211_local_session_tests.exe   # 端到端 12/0
```

## 六、交接

- **下一张卡：212（字幕进渲染）**——与 211 无文件交集。
- 跨设备软解对照（平板装新 APK 后各跑一轮）→ 并入 241 机型矩阵的持续项。
- 软解路径的 av sync 深度对齐（FramePacer 级节拍 + av_drift 出版）→ M2 评估。
- 真机可见轮的经验：软解 sink 的 lock/post 在锁屏下**会计数失败**（与 MediaCodec 的
  `frames_discarded_no_surface` 机制不同）——取证前查 `mDreamingLockscreen` 的纪律对
  两条路径都必须执行。
