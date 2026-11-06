# 210 · DV-A：识别 + 透传 —— 状态（收口）

> 2026-11-07 凌晨收口。设计稿与实测更正块：`210-dv-a-detect-passthrough-design.md`
> （§十 的 7 条实测事实是本卡的实现依据，先于代码）。
> 判据：`tools/verify_p210_dv.py`（自检 4 正例 + 10 变异全抓，exit 0）。

## 一、结论（一张表）

| # | 判据 | 结果 | 证据 |
| --- | --- | --- | --- |
| AC1 | 识别 | ✅ dv_profile=8 / dv_level=1 / bl_compatibility_id=1 全部解出，kind=`dolby_vision` | `p210-dv81-r{1,2,3}.json` `color.dv` |
| AC2 | 透传 | ✅ BL 以 `video/hevc` 交给 **OMX.qcom.video.decoder.hevc**；301/300/0 零丢帧；dataspace=163971072（BT2020_PQ）下发且窗口回读一致 | 同上 + `color.hdr.dataspace*` |
| AC3 | 拒绝 | ✅ profile 5 在 `open()` 拒绝：rc=13、stage=open、reason 点名 `dv_profile=5 (IPT-coded BL…)`，**没碰 configure** | `p210-dv5-r{1,2,3}.json` |
| AC4 | 回归 | ✅ HDR10 轮 kind=`hdr10`+dataspace applied、SDR 轮 kind=`sdr`+不设 dataspace，`dv.verdict=not_dolby_vision`，透传动作全空 | `p210-regress-{hdr10,sdr}-r1.json` |
| AC5 | 门禁 | ✅ 10 份证据 / 94 项全 PASS；自检 10 变异全抓 | `verify_p210_dv.py` 输出 |

桌面：`kivo_p210_dv_tests` **73 checks / 0 failed**；全量回归 ctest **283 通过 / 0 失败**
（5 个 Not Run 是前任账户 python 路径的 ctest 缓存遗留，与本卡无关；等效门禁
`python tools/verify_*.py` 直跑全绿）。双清单一致性门禁 PASS。

## 二、真机数字（K40 `b6e950ee`，Android 13）

| 轮 | codec | recv/rend/drop | kind | dv.verdict | dataspace | rebuilt | bl_mime | end |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| dv81 r1-r3 | OMX.qcom…hevc | 301/300/0 ×3 | dolby_vision | passthrough_hdr10 | 163971072 | true | video/hevc | completed |
| dv5 r1-r3 | （未 configure） | — | — | （negative 证据） | — | — | — | rc=13 |
| regress-hdr10 | — | — | hdr10 | not_dolby_vision | applied | false | null | — |
| regress-sdr | — | — | sdr | not_dolby_vision | **不设** | false | null | — |

## 三、诚实清单（五条）

1. **8.x 透传用的是合成片**（`tools/inject_dvcc.py` 往真 HDR10 流注入 dvcC）——
   识别与透传路径已验，**DV RPU 元数据本体未验**（合成流里没有 RPU NAL）。
   真 DV profile 8 样片到手后跑一轮对照，判据不变。
2. **8.4（EL 层）未测**：判定按 profile 号放行 8.x，8.4 的 EL NAL 行为没有真机数据。
3. **单机**：全部数字来自 K40。平台键编码（`1<<profile`、`1<<(level-1)`）与
   "无 dolby-vision 解码器" 都是这颗设备的事实，换机型要重测（`track_format_dump` 一发即可）。
4. 负向证据文件名沿用 F3B 的 `p12f3-<case>-failure.json` 命名规则（设备端），
   归档时改名为 `p210-dv5-rN.json`——命名规则本身是别人的卡定的，没动。
5. **平台键编码是从两支片交叉验证出来的**（n=2），不是规范文档；非 2 的幂一律判
   "不认识"并拒绝，宁可错杀不猜。

## 四、过程事故（四条，都当场修正）

1. **`gradle rc=$?` 取的是管道尾 `tail` 的退出码** → BUILD FAILED 被当成成功，
   第一轮真机全部跑在旧 APK 上。教训：构建命令的重定向与退出码必须直取
   （`> log 2>&1; rc=$?`），不许经管道。
2. **`/data/local/tmp/kivo.apk` 不重推** → 装回去的还是预检时的旧包。
   教训：`push → install → 跑` 三步必须在同一批里；装完用二进制字符串自证
   （本卡最终以 APK 内 `configure_format_rebuilt":` 真引号字节序列确认）。
3. **"文件未读先改" 失败后重读补编辑时漏掉一处** → 渲染器的 dv 块编辑丢了
   （include 在、块不在），且 .so 里的 DWARF 字段名让字符串自检误判为"已构建"。
   教训：批量编辑被拒后，逐条核对每一处的落地，不靠记忆。
4. **字节搜索用了源码转义形式**（`\"` 经编译已是真引号）→ 白惊一场"没进包"。
   教训：搜二进制先想清楚编译后的字节长什么样。

## 五、复算步骤

```sh
python tools/verify_p210_dv.py --self-test          # 4 正例 + 10 变异
python tools/verify_p210_dv.py                      # 10 份证据 / 94 项
ctest -C Debug -R kivo_p210_dv_tests                # 73 checks / 0 failed（桌面树）
python tools/inject_dvcc.py tests/real/dv210/host_hdr10_safe.mp4 out.mp4 --profile 8 --level 1
                                                    # 测试片再生成（ffprobe 可独立验证）
```

## 六、交接

- 下一张卡：**211（软解兜底）**——FFmpeg avcodec 后端进 APK，与 210 无文件交集。
- 233（DV-B）输入：`libFFmpegKit.so` 的 Ghidra 分析在跑（`C:\re-tools\ffmpegkit.log`），
  完成后用 pyghidra CLI 提取（先例：`sen-extract-pyghidra.log`，analyzeHeadless 的
  postScript 在 Ghidra 12.2 上会因 PyGhidra 未启用而失败）。
- `classify_hdr` 与 DV 判定的优先级已定死：DV 事实（mime=dolby-vision）先于色彩键——
  dv5 片源没有颜色键，旧分类会判 sdr 放过去，这是本卡消灭的失败形状。
