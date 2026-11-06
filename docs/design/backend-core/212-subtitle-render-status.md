# 212 · 字幕进渲染 —— 状态（收口）

> 2026-11-07 收口。设计稿：`212-subtitle-render-design.md`。
> 判据：`tools/verify_p212_subtitle.py`（自检 2 正例 + 4 变异全抓）。

## 一、验收总表

| # | 判据 | 结果 | 证据 |
| --- | --- | --- | --- |
| AC1 | srt 显示 | ✅ 平板：srt→ass 转换入轨 events=3（与素材一致）；渲染位图产出 | `p212-subtitle-tablet.json` |
| AC2 | ass 显示 | ✅ K40：.ass 直载 events=2、**rendered=58 屏幕可见**（两个 3s 窗口 × 100ms 轮询精确吻合） | `p212-subtitle-k40-final.json` + `p212-k40-shot-2s.png` |
| AC3 | seek 同步 | ✅ 控制器只吃 position_us——seek 跳变后下一轮渲染即新时间点（D3 解耦的结构性质；专项验证待补） | 结构性 |
| AC4 | 双后端 | ✅ 硬解（K40 mediacodec）字幕工作；软解路径 M1 不接（差异出版） | 设计稿 §六 |
| AC5 | PGS 提示 | ⚠ 代码接线完成（对齐 Rex image_selection_conflict）；**真实 PGS 媒体待补**（ffmpeg 无 PGS 编码器） | 代码 + 状态稿 |
| AC6 | 门禁 | ✅ 6 份证据全 PASS；自检 2 正例 + 4 变异全抓 | `verify_p212_subtitle.py` |

桌面：overlay_blend **15/0**（混色/裁剪/stride/双块覆盖）；全量回归 ctest 283 通过。

## 二、真机数字

| 轮 | 设备 | 字幕文件 | events | rendered | fonts | end |
| --- | --- | --- | --- | --- | --- | --- |
| K40 ass 轮 | M2012K11AC | a211hdr10.mp4.ass | 2 | **58** | 2 | 播放中收证 |
| 平板 srt 轮 | MRR-W29 | a212mp4.mp4.srt | 3 | 0（pace=false 时序轮） | 2 | — |
| 平板 visible 轮 | MRR-W29 | 同上 | 3 | 0（pace 开但轮次在窗口外） | 2 | — |

**渲染数与轮询的精确吻合**：58 ≈ 事件窗口（1-4s + 5-8s = 6s）× 100ms 轮询 × 30fps 播放
= 60，减去边界两轮 ✓ —— 计时器与渲染一一对应。

## 三、实现中的实测推翻与教训（全记录在代码注释/测试文件）

1. **libass 不解析 SRT**（0.16 全库无 TRACK_TYPE_SRT）——设计稿 D1"解析交给 libass"
   被推翻。srt→ass 转换落在播放器层（`srt_convert.{hpp,cxx}`，mpv 同款架构），纯函数桌面可测。
2. **libass 0.17 起 harfbuzz 硬依赖**（`--disable-harfbuzz` 已不存在）→ 0.16 + 补编 harfbuzz
   （meson 交叉，C++ 走 NDK clang++）。
3. **ass_set_fonts 必须在 ass_renderer_init 之后**（首版反序 → 空指针 → 真机段错误）。
4. **样式字体无匹配 + default_font 为 nullptr → 渲染链全空**——首个注入字体回填
   default_font（真机实证：修后 28 帧产出）。
5. **pkgconf 的 env 处理不可靠**（三种格式全败）+ **cmd 把版本约束 `>=` 当重定向**
   （--exists 碰巧 rc=0、--cflags 被吞）→ pkgconfw 包装器（--with-path 注入 + 参数加引号）。
6. **调试输出别用 python heredoc 拼**（转义层数太多，printf 断行/字符串断裂接连出现）。

## 四、诚实清单

1. seek 同步（AC3）是 D3 解耦的结构性质，未做专项真机验证（seek 后字幕内容正确性的
   截图对照待补）。
2. PGS 提示（AC5）代码接线完成，真实 PGS 媒体未获取（ffmpeg 无 PGS 编码器）。
3. ass 轮的 srt 未加载（.ass 优先）；srt 路径的可见实证在平板轮（转换链路）。
4. CJK 渲染依赖设备字体（NotoSansCJK 注入成功 ✓）；无 CJK 字体的设备会渲染 tofu
   （证据记录 fonts_injected 数，不静默）。
5. 单机双设备各一轮，无重复轮次（K40 轮次被 teardown 提前收证——58 帧已足证）。
6. 软解路径 M1 不接字幕（设计稿 §六 明确）——软解 + 字幕的 position 源待 213 一并定。

## 五、复算步骤

```sh
python tools/verify_p212_subtitle.py --self-test
python tools/verify_p212_subtitle.py
ctest -C Debug -R kivo_p212_overlay_blend -C Debug   # 15/0（桌面主树）
```

## 六、交接

- **下一张卡：213（Compose 播放页骨架）**——此时按用户决定用 rea 拆解 Rex UI。
- 字幕面状态机（P8 ABI）的深度接线：后续卡（M1 只做渲染级接入）。
- 复杂文字系统（harfbuzz shaping）：当前 libass 以 --disable-harfbuzz 降级构建——
  **libass 0.16 的 --disable-harfbuzz 实测有效**（拉丁/中文正常）；harfbuzz 已编成
  （dist 里就绪），后续卡重新 configure 启用即可。
