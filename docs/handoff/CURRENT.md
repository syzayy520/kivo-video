# 交接快照 · Kivo Video Android 后端核心

> 2026-11-09 生命周期补测：真实宿主内部跳转/重建、三轮桌面返回及系统凭据取消通过；合成缺失源ID直达拒绝正常。[测试与边界](../evidence/p240/source-management/lifecycle/status.md)。生产代码未改，实际隐藏源成功解锁/锁屏/隐藏默认首页及手机矩阵仍未验，L2继续DOING。

> 2026-11-09 L2服务器控制追加：隐藏/系统解锁、唯一默认首页、首页聚合和聚合搜索独立持久化已实现；最终包平板覆盖安装，隔离七组PASS，真实搜索偏好/默认启动与撤销已测。[阶段证据与未测边界](../evidence/p240/source-management/l2-controls-status.md)。实际聚合尚未接入；搜索请求、历史/建议/缓存隔离是L3硬闸门。系统凭据取消自动断言失败，不算通过；后台重锁与隐藏直达待验。L2仍DOING。用户新要求：平板管理/编辑弹层采用左右双栏，先设计再适配。

> 2026-11-09 L2紧凑卡使用官方Emby图标、服名及真实电影/剧集数量，添加先选来源类型；新增6dp绿/红/灰状态点。平板已验在线→断网红→恢复，前台每60秒更新；[证据与未验项](../evidence/p240/source-management/l2-compact-status.md)。L2仍DOING。K40再次USB断开，最后大字检查后可能仍1.5倍，下次连接第一件事恢复font_scale=1.0并确认，不能沿用此前“字号已恢复”的结论。

> 2026-11-09用户要求视觉不能最后才做：L2已提前统一资源库主题/总览基础，手机和平板覆盖安装；[截图与边界](../evidence/p240/source-management/l2-visual-status.md)。表单/媒体浏览最终UI及原有未测矩阵仍待做。另查到手机残留TMDB回环代理但无USB转发，已原位备份停用；共享API短域直连真实片单200，图片间歇成功/SocketException，限重试一次后最终生产加载器仍false，[诊断证据](../evidence/p240/tmdb-connectivity/status.md)。不要把API修复当图片修复；浏览器同路径对照被自动审批拦截。手机原微信输入法已恢复；最终instrument运行会停止目标应用，随后重开。L2仍DOING，未进入L3。

> 2026-11-09 当前资源库任务：先收 L2 多源管理，再做内容分类。L1 加密存储/解析及平板接手自检已通过；L2 已接入总览、命名、真实登录保存、源ID恢复与编辑事务，两个真实服务器已在平板验证跨进程保留、切换、编辑取消及失败不覆盖原连接。[L2证据与未测项](../evidence/p240/source-management/l2-status.md)。K40随后已连接，同包覆盖安装，两源新增/跨进程恢复/切换、成功编辑、真实删除后恢复已实测，[手机证据](../evidence/p240/source-management/l2-phone-status.md)。L2保持 DOING，编辑后单独重启、粘贴完整UI、键盘/大字号与取消竞态尚待验。分页WIP已随包安装，首批浏览通过，分页搜索未验。用户指出混排，已记入 [L3内容范围卡](../design/ui/library-rebuild/l3-content-scope.md)，未开始分类实现。REX原件、六款播放器研究和自然家族树见 [统一设计](../design/ui/library-rebuild/plan.md)。后端P15指针独立。

> 2026-11-09 用户要求重设计 Emby：旧表单方案作废，按[新设计与家族树](../design/ui/replica/emby-redesign.md)拆为宿主、会话模型、连接页与媒体页。平板 USB 已恢复并覆盖安装，13项平板流程断言已通过，最终文案修正包另作复测；[新版证据](../evidence/p240/emby-redesign/status.md)。前序网络入口移除本地测试片依赖和失败软解回退已保留，但错误路径、SMB、音频尚未通过完整真机回归；[历史事实](../evidence/p240/network-sources/status.md)。R2/R4 不提前标完成。

> 2026-11-09 R2片名/缺图阶段：长片名自然增长、logo失败文字回退、缺封面浅色场提示已覆盖安装K40；7组组件对照与真实详情首屏25项复算通过。[证据与限制](../evidence/p240/detail-artwork/status.md)。R2仍DOING；预告成功播放和完整多影片矩阵待验。用户新增免登录SMB测试源192.168.5.1，连接/播放尚未验证。

> 2026-11-09 R2 布局阶段：默认首屏坐标保持，深滚正文避开80px状态栏；1.5倍字号指南卡垂直裁字已修，3倍压力场景分类换行已验，系统字号已恢复1.0。[证据与限制](../evidence/p240/detail-layout/status.md)。长片名、缺logo与缺图已在独立组件阶段修复；预告成功播放仍未验，R2仍DOING。

> UI 用户任务（2026-11-08）：用户已认可当前首页/详情效果，停止无依据调参。
> 已认可基线正式集成并推送于 `90561977`：[认可基线](../evidence/p240/layered-scroll/status.md)。
> M1 静态媒体动效真机回归已收口：[首页分层切换/连续色场/吸附、详情上滑缩放](../evidence/p240/media-motion/status.md)。最终版已覆盖安装K40；默认预告关闭，未卸载清数据。
> 后续唯一规划入口：[roadmap.md](../design/ui/replica/roadmap.md)，旧稿裁决：[review.md](../design/ui/replica/review.md)。
> R2 正在实现：标记持久化与底部操作卡片已测，[操作阶段证据](../evidence/p240/detail-actions/status.md)；本轮修复刷新错误遮片名，最终版已覆盖安装K40。简介、电影/剧集连续点击与返回、首载失败/恢复、刷新失败/重试/关闭已测，[导航与异常证据](../evidence/p240/detail-regression/status.md)。播放入口与预告链接已分阶段回归，系统栏/指南大字/分类换行已分阶段实测；片名/缺图已完成组件阶段及真实首屏回归；预告成功播放、完整多影片矩阵待验。[多播放器观察与取舍](../design/ui/replica/player-study.md) 已补；目标是Kivo独立设计的细节精度，不整页照搬REX。
> R1全场景、R3、预告成功播放、帧时性能及全页像素仍未验完；短暂启动黑屏仍存在。不重置其他助手改动。下文是独立后端台账。

> 最后更新：2026-11-01 19:30（P15-A2 收口：**`media_rate` 一直是个没有窗口的比值** ——
> 拖一次进度条，它就从 1.000 变成 1.332 / 0.789，因为分子取整场、分母取真实时长。
> 修完回到 0.999991 / 0.999999，并出版 `span_segments` / `media_rate_scope` 让每个比值
> 都能说出自己描述的是哪一段）
>
> **P14-A…AG 已收口**（`sync/41`）；**P15-A / P15-A2 已 DONE，P15-C 有数字待定阈值，P15-E 已 DONE**
> （`sync/43`）。`ctest` **287/287 全绿**，全仓 **286 份 sync 证据 exit 0**。
> 读这一份就能接手。**下一步看 `TASKS.md`，本文只讲"现在在哪、怎么跑、哪里会踩坑"。**
> 如果只给你 30 秒：读本文第一节，然后打开 `TASKS.md` 找 `P15-A2`（已 DONE）；
> 下一张卡是 **P15-B（G5 零拷贝计数）**，理由与开法在 `180-p15a2-status.md` §七。

## 一、30 秒上手

> **先确认项目是怎么到你手上的。** 截至 2026-10-31 22:20，分支**已推送**
> （`7b8c291` = P14-L…X，upstream 已挂），`git clone` 拿到的就是完整工程。
> 自证只有三条：`git rev-parse HEAD`、`rev-list --count origin/<分支>..HEAD`（应为 0）、
> `git status --porcelain | wc -l`（应为 0）。详见 `AGENTS.md` §0.1。

| 问题 | 答案 |
| --- | --- |
| 项目是什么 | 自研播放器后端核心，**平台顺序 Android → iOS → 桌面**，当前只做后端，不做 UI |
| 现在到哪了 | P12-A … P12-F 全部收口（P12 结题）；**P13-A … P13-AF、P14-A…AG、P15-A/A2/E 已收口（schema `decode/19` + `sync/43`）**。`sync` 侧 **286 份**证据（另 3 份 `superseded/` 明确跳过）：账目全绿，媒体速率 178 判得动 / 108 不可判 / 5 已登记越界。**G8 结题（1080p30：58.9 / 52.9 ms，预算 150 ms）；G3 有数字（304 / 202 / 284 次稳态分配，不是 0）；G4 标"不适用"；G5 仍只有结构没有计数** |
| 下一步做什么 | ① **P15-B · G5 零拷贝计数** —— G5 目前只有"结构上成立"（代码里没有 `sws_scale`/`Map`/`memcpy` 在判定路径），**没有任何计数**；而它是"硬解有没有白干"的唯一证据。② **P15-C 的 302 次分配归因** —— `make_alloc_facts()` 已按线程分 8 桶，出版那 8 格就能回答"这 300 次是谁"；阈值不定就没法结 G3。③ **P15-D · G1′/G2′ 重新定义** —— Android 的 `decode_fps` 被节拍器封顶，测不出硬解上限，要单开 decode-only 路径。④ **字幕接线** —— 74 个文件的语义层（ASS/SSA/WebVTT/帧构建/租约池）**没有任何像素产出**，也没在 Android 上验过；这是"最强播放器"最大的缺口。⑤ **HDR/tone mapping** —— `core/hdr` 不存在，只有 `surface_dataspace` 设了不验。⑥ **网络流媒体** —— `providers/http` 144 行、无 socket，"从网络播"还没实现。⑦ 8 s 片源量程不足（P14-V 产物）：换 ≥ 60 s 片源看门是否在第 12 秒动作。⑧ **多机型矩阵** —— 至今只有一台 K40/一个 SoC。 |
   改为给 `av_drift` 配使用条件 —— 块内出版 `sync_verdict`（`av_synced` / `clock_off` / `unknown`）、
   `lead_ns`、`lead_bound_ns`（一帧）。真机两轮：control `lead 0.01 ms → av_synced`，
   5000 ppm `lead 40.18 ms → clock_off`，而两轮的 `av_drift.p99` **都是 0.125 ms** ——
   那 40 ms 以前没有任何一格说得出来。判据 AC1/AC2 双向守（越界判同步、未越界判偏移都红）。

③ **红线扫描域** —— 现在有分项数字（2026-10-31 复核，`redline_gate_result.json`）：
   **30 条违规 / 259 个被扫文件**，按位置是 `docs/` **26**（历史 roadmap）、
   `.codebuddy/memory` **3**、`evidence/` **1**（旧 schema）；按规则是
   R4 凭证路径 **23**、R8 提前宣称 **6**、SCHEMA **1**。
   但**"交付物干净"这句是错的**（我先前按相对路径比对，漏了绝对路径）：用试写的
   `--scan=delivery`（试验版，未保留在仓库）跑了一遍，交付目录里还有 **4 条** —— `docs/design/backend-core/158-p14v-status.md`
   （R4 + R8）、`docs/design/p7/…V1.9.md`（R4）、`docs/handoff/p10-real-playback-current-state.md`（R4）。
   外加 schema 闸门是**独立**跑的，它扫的是 `artifacts/p2/evidence` 两份旧证据，与模式无关。
   **已定并落地（2026-10-31，P14-AB）**：默认扫"交付物 + 本次改动"，`--scan=all` 留作历史审计。
   4 条交付物违规已修（两份文档脱敏、`158` 去掉规则字面量），**`ctest` 286/286 全绿**，
   `#170` 从长期红转绿。历史 29 条 + 2 份旧 schema 仍在台账里，每轮扫描都会印出来 —— 不判红 ≠ 没有。④ **`jumps_clamped = 3` 的线索** —— 当时 3 次、今天 0 次。⑤ **让 `surface_at_time` 也提供钟读数** —— 但**不是 108 份**：拆开是 90 份旧格式（/1–/15，补不了）+ **18 份 at_time**（真的缺）。设计见 `167-p14aa-design.md`，里面还查出一个总闸覆盖洞：`p13p` 只扫 `p13*`，`p14k` 的 at_time 证据从没被扫过，手动跑当场红（P1 `due_step_supplied=0`）。⑥ **打断之后"回来从哪继续"** —— 需要 native 暴露 `position_us`。⑦ **32 位 Android**：CM311-1A 只有 `armeabi-v7a`，要先有 32 位 FFmpeg。⑧ **≥ 1 s 的台阶** —— **已由 P14-AE 收口（2026-11-01）**：悬崖在 1000 ms（999 走台阶、1000 走跳变，由 `wall_delta` ±0.1 ms 的抖动决定）；跳变分支把台阶丢掉、`lead` 归 0、`sync_verdict` 说 `av_synced`。**下一张卡 P14-AF**：顶层出版 `jump_max_ns`，并让 `jump_rebases > 0` 的轮次 `sync_verdict = unknown`。⑨ **逐帧时间轴**：`present_jitter` 只有分位，问不了"那 24 帧丢在哪几秒" —— 要回答它，报告得出版逐帧 `(pts, due, present)` 抽样（出版层改动，单独一张卡）。 |
| 代码在哪 | C++ 后端 `backend/`；Android 壳 `android/`。`android/app/src/main/cpp/CMakeLists.txt` 用 `${KIVO_ROOT}/backend/src/...` **逐条列源文件**（不是 `add_subdirectory`，也别去找那个），所以两个目录必须同一个仓库、相对位置不能变 |
| 工作目录 | **`C:\kivo video` 整个仓库**。`android/` 单独拿出来会在 CMake 配置期 `FATAL_ERROR`。哪些目录必须一起给，见 `AGENTS.md` §零 |
| 怎么验证 | 必须真机（Redmi K40，序列号 `b6e950ee`），模拟器数字一律无效 |
| 唯一铁律 | 单文件单职责 / 家族树归位 / 不许堆砌 / 没有实测数字不许说"完成" |

必读文档（按顺序，别跳）：

1. `.codex/memory/kivo-video-iron-gate.md` —— 铁律门闸，动架构前必读
2. `docs/design/backend-core/03-goals.md` —— G1–G10 验收口径，所有数字都从这里来
3. `docs/design/backend-core/07-p12-plan.md` —— P12 阶段划分与红线
4. `docs/design/backend-core/08-p12-family-tree.md` —— 文件该放哪
5. `docs/handoff/TASKS.md` —— 任务台账

---

## 二、已经闭环的事实（带证据，可直接引用）

### P12-A · 真机能力矩阵

- 证据：`docs/evidence/p12a/p12a-capabilities-redmi-k40.json`（117 条记录 / 0 解析失败）
- 分析：`docs/design/backend-core/10-p12a-evidence.md`
- 结论：77 个解码器 / 41 个硬件 / 11 个十位 / 8 种像素格式

### P12-B · 真实硬解 + 零拷贝上屏

| 片源 | 解码器 | 帧数 | fps | 相对实时 | 首帧 |
| --- | --- | --- | --- | --- | --- |
| H.264 1080p 8bit | `OMX.qcom.video.decoder.avc` | 180/180 | 169.8 | 5.66× | 103.7 ms |
| HEVC 1080p 10bit | `OMX.qcom.video.decoder.hevc` | 180/180 | 175.3 | 5.84× | 90.8 ms |
| HEVC 4K 10bit | `OMX.qcom.video.decoder.hevc` | 120/120 | 127.4 | 4.25× | 113.0 ms |

- 证据：`docs/evidence/p12b/{avc-1080p8,hevc-1080p10,hevc-4k10}.json`
- 状态文档：`docs/design/backend-core/11-p12b-status.md`

**必须守住的不变量**（以后任何改动都要保住）：

```
samples_submitted == frames_rendered == 轨道总帧数
samples_rejected == 0
completed == true && error == ""
```

这条比任何 fps 数字都更能说明正确性。输入侧用"挂起包 + 重试"，不是"失败就丢"。

### 瓶颈判定（决定了 P12-C 做什么）

4K 像素是 1080p 的 **4 倍**，每帧只慢 **1.37 倍** → 限制来自**每帧固定开销**
（送包/收帧往返、`releaseOutputBuffer` 与 BufferQueue 交互），**不是解码吞吐**。

已做的一轮优化（批量灌 8 包 + 去热路径锁与字符串复制）：1080p +21%，4K +12%。
剩下的三条线索全部指向呈现节拍 → 归入 P12-C，不要零散地再优化解码器。

### P12-C1 · AAudio 输出骨架（已闭环 2026-10-29）

| 项 | 实测 |
| --- | --- |
| 协商结果 | 48000 Hz / 2ch / pcm_float / low_latency / burst 192（4ms）/ capacity 1536（32ms） |
| 共享模式 | 请求 exclusive，**实际 shared**（已记入 `downgrade_reason`） |
| 3s 440Hz | 146,304 / 144,000 帧，ratio 1.016，`xrun=0`，补静音 0 |
| 10s 1kHz | 482,496 / 480,000 帧，ratio 1.005，`xrun=0`，补静音 0 |
| **人耳验收** | ✅ 2026-10-29 06:03 用户真机当场确认「有声音」 |

证据：`docs/evidence/p12c/aaudio-tone-redmi-k40{,-10s}.json` +
`aaudio-tone-listening-redmi-k40.json`（人耳那轮），分析见 `12-p12c-status.md`。

**两条必须记住的结论**：

1. **启动偏移不是常数（24–146 ms），禁止固定补偿**：`wall - requested` 五次测量为
   146/25/24/146 ms，帧侧偏移 36–52 ms，冷启动最慢。
   **C4 的音频时钟零点必须取"首帧实际出声"、用 `AAudioStream_getTimestamp` 读真实帧位置**，
   不能取 `requestStart`，也不能减任何常数 —— 偏移抖动本身就有 20–120 ms，
   与 G7 的 40 ms 阈值同阶。（我第一版只测两个点就写死"恒定 146ms"，第三次测量当场推翻，
   过程与全部数据见 `12-p12c-status.md` §三·发现 2。）
2. **独占是提示不是契约**：`openStream` 成功、`getSharingMode` 却给 shared。
   第一次跑时我们的检测漏了（只校验格式/采样率/声道），已修。
   任何"协商结果"都必须逐项对照请求值，不符即记录。

### P12-C2 · SPSC 无锁环形缓冲（已闭环 2026-10-29）

| 项 | 结果 |
| --- | --- |
| 单测 R1–R9 | 9/9 PASS，433 项断言 0 失败 |
| 平台 | Windows x86_64（Debug + Release）+ **Redmi K40 arm64 真机**，各连跑 20 次零失败 |
| 热路径分配 | 1 万轮读写期间 `operator new` 调用 = **0** |
| 真机联调（实时供给） | `silenced=0`、`xrun=0` |
| 真机联调（**半速供给，故意欠载**） | `silenced=104,192`，且 `242,496 − 138,400 = 104,096` **闭合** |

- 证据：`docs/evidence/p12c/ring-buffer-tests-arm64-k40.txt`、
  `p12c2-ring-realtime-redmi-k40.json`、`p12c2-ring-underrun-halfspeed-redmi-k40.json`
- 分析：`docs/design/backend-core/13-p12c2-status.md`

**三条必须记住的结论**：

1. **缓冲本体在 `core/audio/`，不在 `backends/aaudio/buffer/`。**
   后端目录只放 `ring_buffer_sample_source.hpp` 这个薄适配器。
   放进后端就等于绑定 Android，iOS 就得复制或反向依赖。
2. **`frames_silenced` 是有效计数器，但前提是两侧计数闭合**：
   `silenced = rendered − pushed`。只有"实时供给 silenced=0"这一组数据**不能**证明它有效，
   必须配一组"半速供给 silenced>0"才算证完。以后加任何统计字段都照这个规矩办。
3. **供数要按绝对时间轴**（`sleep_until`），不能"处理完睡 N ms"；
   **预灌要放在 `session.start()` 之前**。这两条都是我在探针里先踩了才发现，
   错一个就会出现几十毫秒的"假欠载"，而且看起来极像缓冲容量不足。

> **补充（C3 修正）**：绝对时间轴那条**只适用于无缓冲直连**。
> 一旦中间有环形缓冲，节奏由"环满即停、环空即续"决定，解码线程不该再自己睡固定周期。

### P12-C3 · 音频解码与 PCM 供给（已闭环 2026-10-29）

| 项 | 结果 |
| --- | --- |
| 48k / 立体声 / AAC-LC（10s 整曲） | 解出 480,256 帧，写入 480,256，丢弃 0，`xrun=0` |
| 44.1k / 单声道（5s） | 协商 **44100Hz/1ch**（未被顶成 48k），`xrun=0` |
| 转换单测 P1–P9 | 9/9 PASS，44 项断言，x86_64 + **arm64 真机**全过 |
| 负向用例 | 无音频轨 → 退出码 4 且说明"no audio track among 1 tracks"；坏路径 → 退出码 3 |
| 人耳验收 | ✅ 2026-10-29 06:35 用户确认「有声音」（左 440Hz / 右 880Hz 片源） |

- 证据：`docs/evidence/p12c/p12c3-*.json`、`pcm-tests-arm64-k40.txt`
- 分析：`docs/design/backend-core/14-p12c3-status.md`

> **C3 出声已收工**（2026-10-29 用户二次确认"声音没问题"）。唯一没做的一格：
> 左 440Hz / 右 880Hz 片源的**左右顺序未逐耳分辨**——它不影响任何客观指标
> （声像接反时所有计数器依然全绿，所以数据判不出来）。日后若怀疑接反，
> 用单边片源交叉验证即可，**不要为此回炉重测 C3**。

### P12-C4 · 音频主时钟（已闭环 2026-10-29）

| 项 | 实测（Redmi K40，7 次真机） |
| --- | --- |
| 主源可用性 | `timestamp_fail=0`、`fallback=0`、`stale=0`、`clamped=0`（全部 7 次） |
| 启动开销 | **双峰**：25 ms（4 次）/ 127–147 ms（3 次），与 C1 独立测的 24–146 ms 吻合 |
| 首帧帧位置 | `first_ok_frame` = 0 或 768（0–16 ms），**不是** 127/146 ms |
| 管线延迟 | 48k = 2736 帧（**57 ms**）、44.1k = 6747 帧（**153 ms**），七次完全恒定 |
| 单调回退 | 48k 每次 2–3 次、44.1k 每次 8 次，全部被守卫拦下 |

- 证据：`docs/evidence/p12c/p12c4-clock-*.json`（7 份）
- 设计 / 分析：`15-p12c4-design.md`、`16-p12c4-status.md`
- 单测：`kivo_p12c4_clock_tests` 13 例 / 66 断言，桌面 Debug 全绿

**三条必须记住的结论**：

1. **零点问题不存在，是"选对驱动量"的必然结果。**
   `audio_clock = framePosition / sampleRate`，而 `framePosition` 在首帧出声前恒为 0，
   所以启动开销那 25–147 ms 自动被排除。
   **反过来，任何以 `requestStart` 为原点的写法都必错**，且错多少每次都不同。
2. **官方说"单调递增"，实测不成立。** NDK 原文承诺 `monotonically increasing`，
   实测 1963–1969 次查询里有 2–3 次（44.1k 是 8 次）回退。
   `MonotonicClockGuard` 不是防御性代码，是必需品 —— 一次回退就够 C5 误判一帧。
3. **管线延迟按采样率分档，且比 G7 阈值还大。** 48k 是 57 ms、44.1k 是 153 ms，
   而 G7 要求 `|video_pts - audio_clock| p99 < 40 ms`。
   C5 算 `due_at` 必须扣掉这个延迟，否则会系统性提前排帧。

> **C3 遗留静音问题已结案**：新增 `first_audible_frame` 后，七次全部 `silence_head_frames=0`，
> 静音全在尾部（128/198/320 帧 = 2.7–6.7 ms）。
> 即：**预灌足够，尾部是正常排空**，稳态播放期间零静音。G7 现在有干净基线了。

**四条必须记住的结论**：

1. **音频路径允许 CPU 拷贝**：G5 的零拷贝只约束视频呈现路径。别拿 G5 卡音频。
2. **转换格式只能取 `decoder.output_format()`**，不许用容器声明的格式。
   容器给的是期望值；声道数一旦不符，交错步长就错，听感是噪声而非"格式不对"，且不会报错。
3. **不做重采样**：AAudio 协商结果与片源不一致就显式失败（退出码 7）。
   实测 K40 会把 44.1k 原样返回 44100，所以这条路目前不需要重采样。
4. **`submit` 返回 `TRY_AGAIN` 时绝不能读下一包**：读一次就推进了读指针，那一包永久丢失，
   AAC 一包约 21ms，丢几包就是可听的断续。

### P12-C5 · 呈现节拍器（已闭环 2026-10-29）

| 档位 | 呈现 / 丢帧 | 丢帧率 | `present−due` p99 | 间隔偏差 p99 | 墙钟 / 片长 |
| --- | --- | --- | --- | --- | --- |
| 24 fps | 193 / **0** | 0% | **0.125 ms** | 0.074 ms | 8.047 / 8.000 s |
| 30 fps | 181 / **0** | 0% | **0.125 ms** | 0.072 ms | 6.072 / 6.000 s |
| 60 fps | 481 / **0** | 0% | **0.125 ms** | 0.125 ms | 8.048 / 8.000 s |
| 对照（关节拍，24 fps） | 193 / — | — | 未测 | 未测 | **1.087 s = 7.36×** |

- 证据：`docs/evidence/p12c/p12c5-{pace-24fps,pace-30fps,pace-60fps,nopace-24fps}.json`
- 设计 / 分析：`17-p12c5-design.md`、`18-p12c5-status.md`
- 单测：`kivo_p12c5_pace_tests` 17 例 / 69 断言，桌面 Debug 全绿

**四条必须记住的结论**：

1. **0.125 ms 是测量分辨率，不是精度。** 桶宽 0.25 ms，所以真值在 0–0.25 ms 之间。
   写成"抖动 0.125ms"会被读成"精确到这个数"，正确读法是"**小于测量分辨率**"。
2. **`clock_source` 这一轮是 `system`，不是 `audio_timestamp`。**
   C5 是纯视频取证（无音轨），只证明 **G6（节奏）**；**G7（音画同步）是 C8 的事**。
   把这两条混起来就是红线第 5 条。
3. **`due_at = now + (pts − anchor_pts − elapsed)`，首帧在到达时建立锚点。**
   这样启动与首帧解码的几十~上百毫秒被自动排除；用 `start()` 时刻作原点，
   一半的启动会把头几帧判成"迟到 130ms"直接丢掉。
4. **节拍器会在解码线程里睡到 `due_at`**，等待期间解码器空闲。这是 C6 要解的债；
   在此之前**不要**用"加大批大小"之类的办法掩盖它。

> **C5 修掉的两个真机才暴露的问题**：
> ① 时钟停更降级时若沿用降级前的 `last_due`，时间轴落后 500 ms+，随后几十帧全判迟到 ——
>   降级自己制造一场丢帧风暴。改为**降级当帧从现在起算**。
> ② 分位数取桶中心、`max` 取真实值，会出现 `p99 = 0.125ms` 而 `max = 0.079ms`。
>   已把分位数钳进 `[min, max]`，并加用例 C5-17。

### P12-C6 · 送收分线程（已收口 2026-10-29，**判据未成立**）

| 片源 | merged | split | 增益 |
| --- | --- | --- | --- |
| H.264 1080p8 | 180.4 fps | 178.7 fps | **−0.9%** |
| HEVC 1080p10 | 176.4 fps | 179.8 fps | +2.0% |
| HEVC 4K10 | 133.5 fps | 134.1 fps | **+0.5%** |

判据是「4K 增益 ≥ 1080p，增益随像素走」。实测三者都在噪声内且顺序反了 →
**按 `19-p12c6-design.md` §七 回退，默认模式已改回 `kMerged`**，`kSplit` 保留为实验路径
（同一二进制传 `--ei kivo.threads 1` 即可复跑，无需重编）。

- 证据：`docs/evidence/p12c/p12c6-*.json`（36 份 + 回退后复核 12 份，共 48 份）
- 设计 / 验收：`19-p12c6-design.md`、`20-p12c6-status.md`
- 单测：`kivo_p12c6_pipeline_tests` 9 例 / 42 断言；`ctest -R kivo_p12c` **5/5**

**回退已真机复核（2026-10-29 09:2x，11 轮默认路径）**：不传任何 extra 时证据里
`pipeline.mode` 全部为 `merged`，三档 fps 为 180.4 / 178.2 / 134.3，相对 C6 基线
±1% 内、无退化；11 次运行帧数全闭合、零拒绝、零丢帧。传 `--ei kivo.threads 1`
仍能跑出 `mode=split`（132.0 fps、121/121 闭合），实验通道未被阉掉。

**四条必须记住的结论**：

1. **瓶颈不在送收往返。** `present=false` 对照：1080p 180→283 fps（+57%）、
   1080p10 176→379 fps（+115%）、4K 133→131 fps（−1.7%）。
   → **1080p 卡在呈现侧（2.0–3.0 ms/帧），4K 卡在解码器本身（7.6 ms/帧）。**
   ⚠️ 「BufferQueue 回压」这个说法已被 **C6B 修正**：那 2 ms 不在 `release` 上，在 `dequeue` 的空等，
   且真实节拍下不存在。见下面 C6B 一节。
2. **P12-B 那个「5.2 ms 与像素无关的固定开销」是错的**：它拿「1080p 带呈现」比「4K 带呈现」，
   把呈现回压算进了固定开销。方向（别零散优化解码器）仍对，机制解释不对。
3. **C7 不会提高吞吐。** `releaseOutputBufferAtTime` 只改"这一帧何时显示"，不改槽位何时归还。
   ⚠️ **C7 的验收口径已被 C6B §六 改写**：不能写 fps，也不能直接拿 C5 的 `p99` 当基线 ——
   那个数测的是"交出时刻"，不是上屏时刻。见下面 C6B 一节。
4. **拆线程本身没有害处，也没有收益**：三档节拍零丢帧、p99 ≤ 0.375 ms，正确性不变量全绿。
   它在别的芯片或 C7 改了呈现路径之后仍值得复测，所以没删。

### P12-C6B · 呈现回压的边界（已结题，**结论是不做优化**）

C6 §六 留下的问题：「1080p 的呈现回压是否可解」。答案是 **不该解，因为真实播放里它不存在**。

| 读数 | 无节拍（解码狂奔 180 fps） | 真实节拍（24/30/60 fps） |
| --- | --- | --- |
| `TRY_AGAIN` / 帧 | 2.03 | **0.02–0.31** |
| `receive` 占 wall | 85.5% | **0.5–3%** |
| 丢帧 | 0 | 0 |

**落点修正**：present vs norender 的 2.01 ms 差异里，`receive` 占 **+1.99 ms（99%）**，
`release` 反而是 **−0.01 ms**。回压是 `dequeueOutputBuffer` 空等满 2000 µs 的 `TRY_AGAIN`，
不是 `releaseOutputBuffer` 交不出去。

**触发条件**：解码速率 > 显示消费速率。实验室 180 fps 解码 vs 60 Hz 屏幕是 3:1，
槽位必然堆满；真实播放两者接近 1:1，回压自动消失。

**对 C7 的三条修正**：① 验收不能写 fps；② C5 的 `p99=0.125 ms` 测的是"交出时刻"
（采样点在 `releaseOutputBuffer` 之前，中间隔 1–3 个 vsync），C7 必须先建立上屏时刻测量；
③ C7 的价值在高负载下的调度免疫，取证必须构造负载。

- 分析：`21-p12c6b-present-backpressure.md`
- 证据：`p12c6b-pace-4k30-{merged,split}.json`（新增 2 份）
- 欠账：没真去试加 buffer 数量（结论是"不该解"不是"解不了"）；无 4K60 片源

### P12-C7 · 精确呈现时刻（已结题，**已设为默认**）

把"CPU 睡到 `due_at` 再交"换成"不睡、把 `due_at` 交给合成器"
（`AMediaCodec_releaseOutputBufferAtTime`）。**结论：有效，已设为默认。**

| 读数（5 忙循环负载，≈27% idle） | `cpu_paced`（旧） | `surface_at_time`（新） |
| --- | --- | --- |
| 交出错峰 `lateness` max | 1.26–2.12 ms（空载仅 0.13–0.30） | —（该路径无 lateness 样本） |
| 提交提前量 p50 | — | **−99.875 ms**（= 3 帧，卡在 cap） |
| 跨轮跨度 | p99 极差 0.50 ms、max 0.86 ms | **恒为 0** |
| 丢帧 | 0 | 0 |

**spread = 0 是算出来的，不是测出来的**：`at_time` 的提交时刻 = `due_at − 3 × frame_duration`，
由算术决定，CPU 忙不忙不进这个算式。所以它是结构性结论，不是这一轮运气好。

**它买到了什么，别写错**：买到的是**"不再受线程调度摆布"**，不是"画面更流畅"。
12 次运行两种结构**都是 0 丢帧**。上屏时刻仍然测不到（C6B §二 已证），留给 P12-D。

**默认值改了四处**（少改一处就会"文档说 A、APK 跑 B"）：`present_schedule.hpp` 枚举注释、
`PipelineSetup::present_schedule`、`jni_session_handle.hpp` + `NativePlayer.kt` 的 `at_time` 说明、
`PlayerActivity.kt` 的 `atTimePresent = true`。

**回退是一条命令，不用重编**（换机后若帧数不闭合 / `completed=false` / 画面停顿，第一件事就是它）：

```bash
... --es kivo.present cpu_paced --es kivo.evidence p12c7-cpu.json
```

- 设计 / 验收：`22-p12c7-design.md`、`23-p12c7-status.md`
- 证据：`p12c7-{idle,load}-1080p30-{cpu_paced,at_time}-r{1..3}.json`（新增 12 份）+ `p12c7-probe-at.json`
- schema：`kivo.p12c.decode/4`（新增 `pipeline.present_schedule` 与 `pace.schedule_lead_ns`）
- 欠账：负载是合成的；`kMaxScheduleLeadFrames` 没做 1/2/4 对照；只测 1080p；只验过 K40

### P12-C8 · 证据产出（已结题，**C 组收口**）

C 组最后一张卡。它从一开始就不是"把两边数字拼起来" ——
**在 C8 之前，本项目从来没有一条运行同时有音频和视频**：音频探针只播音频，
`PlayerActivity` 只播视频。所以 `av_drift` 这个字段一直没有数据源。

为此新增 `backends/avsync/`：音频播放侧（解封装 → 解码 → 环 → AAudio → 时钟）
+ 合并报表（`kivo.p12c.sync/1`），并把 AAudio 主时钟第一次接到视频节拍器上。

**结论：G7 首次可测 —— `av_drift` p99 = 0.125–0.375 ms，阈值 40 ms。**
但**必须连下面这个条件一起引用**，否则就是造假：

| 路径 | `av_drift` | 为什么 |
| --- | --- | --- |
| `cpu_paced`（帧在 `due_at` 当刻交出） | **可测**，p99 = 0.125–0.375 ms | `lateness = 音频已走 − 视频 pts 已走` |
| `surface_at_time`（默认，提前 3 帧交） | **不可测**，证据写 `measurable: false` + 理由 | 那个差值等于提交提前量（实测 p50 = −99.875 ms = 3 帧） |

⚠️ **这条把 C8 的第一版设计推翻过一次。** 第一版在 `arrive()` 里采样，
真机跑出 `p50 = −133.18 ms` 而同轮 `lateness` 样本数为 0，两个数对不上，一查到底才发现
测的是提前量。详见 `25-p12c8-status.md` §三 —— **这是本次最重要的发现，别重复踩。**

**可复算不是形容词**：`tools/verify_p12c_sync.py` 只吃 JSON，从分布（非空桶 + 溢出计数 +
精确极值 + 三个桶映射常量）重放分位数，与证据里的 p50/p95/p99 **逐位比对**，
不做"容差内就算过"。五份证据全过。脚本还守四类自洽：桶合计 = 样本数、
帧数闭合、`clock_source` 与 `clock_mix` 一致、`playback_rate` 在 [0.98, 1.02]。

**`playback_rate` 是唯一不与节拍器结论循环的检查**：它只看墙钟
（媒体跨度 / 墙钟跨度）。`av_drift`、`schedule_lead` 都拿节拍器自己的目标时刻当基准，
节拍器整体跑偏时它们可能依然"完美"。三轮实测都是 0.99998。

- 设计 / 验收：`24-p12c8-design.md`、`25-p12c8-status.md`
- 复算：`python tools/verify_p12c_sync.py docs/evidence/p12c/p12c8-*-sync.json`
- 证据：`p12c8-av-cpu-r{1,2,3}-sync.json`（主）、`p12c8-av-at-r1-sync.json`、
  `p12c8-noaudio-cpu-r1-sync.json`；`superseded/` 里是已废弃的第一版
- 复跑要显式加 `--es kivo.present cpu_paced`，否则 `av_drift` 一定是"不可测量"
- 欠账：**默认路径的上屏同步测不了 —— 这是 C 组留下的最大一条，交 P12-D 的 present fence**；
  只有 6s/1080p30/AAC48k/K40 一种组合；没做人耳确认

### P12-D1 · Surface 重建（已结题）

**改之前不是"恢复得慢"，是没有恢复。** `onSurfaceLost()` 直接 `teardownSession()`，
按一次 Home 会话全丢，回来从第一帧重开。

改法：**摘面 → 挂新面，会话不动**。平台侧 `AMediaCodec_setOutputSurface` window→window 直换
（ExoPlayer 换面同一条路）。

| 轮 | 空窗 `gap_ms` | 恢复 `resume_ms` | `frames_lead` | 上屏帧 | 空窗丢弃 | 节拍丢帧 | 播放速率 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 对照（不切） | — | — | 0 | 180 | 0 | 0 | 0.999977 |
| r1 | 470 | **29** | 0 | 165 | 15 | 0 | 0.999979 |
| r2 | 1019 | **15** | 0 | 149 | 31 | 0 | 0.999979 |
| r3 | 2020 | **2** | 0 | 120 | 60 | 0 | 0.999978 |

**两条自洽性最值得看**：空窗丢弃帧数 = 空窗时长 × 30fps（误差 ≤ 1 帧），
说明丢的确实是空窗期的帧；`frames_dropped_late` 全程 0，说明**时间轴没有被重建打断**。

⚠️ **真机坑（已写进设计稿 §3.2）**：`setOutputSurface(nullptr)` 切到 ByteBuffer 模式之后
**切不回来**（`AMEDIA_ERROR_UNKNOWN` / −10000）。所以"摘面"改成本地标志 + 强制丢帧，
一次都不碰 Surface；而且摘面时**不能**放掉 `ANativeWindow*` —— codec 还指着它。

⚠️ **口径**：`gap_ms` 是用户的时间，`resume_ms` 才是我们的账。
空窗丢帧单列 `frames_discarded_no_surface`，**不许**并进 `frames_dropped_late`
（第一版并了，500 ms 切后台凭空多出 15 个丢帧，看起来像节拍崩了）。

- 设计 / 验收：`26-p12d1-design.md`、`27-p12d1-status.md`
- 证据：`p12d1-{control,rebuild}-r{1,2,3}{,-sync}.json`，schema 升到 `kivo.p12c.decode/6`
- 帧数闭合关系变成四项：`rendered + dropped + discarded + 1 == received`
- 没做：旋转（本 App 锁横屏 + `configChanges` 全包，转屏不重建 Surface，没有可测的东西）；
  分屏/画中画；空窗期不停解码（切走 5 分钟会白解 9000 帧）

### P12-D2 · 多实例并发解码（已结题）

**native 侧一行没改** —— 3 路用的是三个独立 `SessionHandle`，各有自己的线程与 codec。
这是 C6 之后"会话不共享状态"的红利。Kotlin 侧新增 `bridge/PlaybackSession.kt` +
`ui/MultiPlayerActivity.kt`（N 个 SurfaceView 横排，**先全部 open 再全部 start**，
否则测出来的是接力不是并发）。

| 并发数 | 各路 fps | 合计 | 相对单路 | 轮内极差 |
| --- | --- | --- | --- | --- |
| 1（基线） | 160.84（147.27 / 179.58 / 160.84） | 160.84 | 100% | — |
| **3** | **132.87 / 132.61 / 130.81** | 396.29 | **246%** | 1.6% |
| 3（另两轮） | 131.8–132.9 / 131.4–131.5 / 131.0–131.1 | 395.1–396.2 | 246% | 1.1% |
| 4 | 99.48 / 98.86 / 98.35 / 98.44 | **395.13** | 246% | 1.1% |

3 路 @30fps：各 720/720 帧、**0 丢帧**、30.06–30.11 fps、墙钟 23.9 s 播完 24 s 素材。

**这张卡最值钱的不是"过了门槛"，是 4 路那一行**：合计 395.13 fps，与 3 路的 396.29 差 0.3%。
**加一路不涨 ⇒ 天花板 ≈396 fps；而单路 160 只吃到 41%。**
也就是说单路吞吐被"每实例的固定往返开销"卡着，不是硬件算力 ——
这给后续提单路吞吐指了方向（动往返，不是换解码器、也不是拆线程，C6 已证拆线程无增益）。
⚠️ 但**不许**据此宣称"单路能优化 2 倍"，41% 里有多少是平台强制的往返，没有实测。

⚠️ **混杂因素必须跟着数字一起说**：单路占满全屏（1920 宽），3 路各占 1/3（640 宽），
总缩放像素更少。这个因素**偏向"多路更好看"**。若三路都占满全屏，比例会比 81% 低。

- 设计 / 验收：`28-p12d2-status.md`（§三 天花板推导、§六 七条诚实清单）
- 校验：`tools/verify_p12d_multi.py` —— `ratio`（60% 门槛）与 `paced`（零丢帧）两种检查，
  外加帧数闭合与"轮内极差 ≤10%"（防某一路被饿着但每路都过了 60%）
- 证据：`p12d2-{baseline,multi,multi4,paced}-*`（15 + 4 份）
- 未做：HEVC/4K；多路带音轨；2 路与 5/6 路；并发下的长稳（D3）

### P12-D3 · 长稳测试 30 min（已结题）

30 分钟 1:1 实时播完 **54000/54000 帧**，`playback_rate 0.999990`。

| 项 | 判据 | 实测 |
| --- | --- | --- |
| 内存增长（PSS 总） | < 5 MB | **−0.62 MB**（103.48 → 102.86） |
| Native Heap | — | **+2.19 MB**，头 10 分钟涨完就平 |
| Dalvik Heap | — | +0.11 MB |
| `frames_dropped_late` | 无累积趋势 | **31 个采样点全 0** |
| 帧数闭合 | 停稳后严格 | 54000 + 0 + 0 + 1 == 54001 |

**只报 PSS 会像"什么都没发生"，只报 Native 会像"有泄漏"** —— 合起来才是事实：
涨在 native 堆（分配器 arena 早期扩张），被别处的页回收抵平，总量没动。

⚠️ **两个测量坑，都是窗口 bug 不是代码 bug**：

1. **停机瞬间 PSS 从 105 掉到 60 MB。** 不裁窗口会算出"内存下降 43 MB"。
   修法是从数据里识别停机（相对中位数掉 20% 以上），不写死秒数。
2. **这台 K40 的时钟比真实 UTC 快 8 小时**（`adb shell date -u` 就比主机快 8h）。
   设备证据的 `generated_at_unix_ms` 不能和主机时间直接相减 ——
   第一次直接相减的结果是"所有样本都在播放窗口外"，一条都没剩。
   脚本现在会估偏移并把 `+7.99 小时` 打出来。

> **对全项目成立的一条**：`generated_at_unix_ms` 反映的是**设备时钟**，不是真实时间。
> 拿它排序没问题，拿它跨机器算先后就会错。

⚠️ **中途快照不是静止态**：`released == received` 只在会话停下后成立。
中途只要求 `0 ≤ 在途 ≤ 16`，结束证据才要严格闭合。

- 设计 / 验收：`29-p12d3-design.md`、`30-p12d3-status.md`
- 新增：播放器 `kivo.soak` / `kivo.soak.ms`（每 N 秒写一份完整 `decode/6`）；
  `tools/sample_p12d3_mem.py`、`tools/verify_p12d3_soak.py`
- 证据：`p12d3-soak-r1-mem.csv` + 30 份 `-t####.json` + 结束证据 + sync
- 判据的边界：5 MB **绝对值**只能挡住每分钟 >0.17 MB 的持续增长，更慢的挡不住
- 未做：高码率片源；3 路并发长稳；带音轨长稳；几小时级；内存归因；温控/功耗

### P12-D4 · 生命周期收敛（已结题）

**判据不是"补个测试"，是把顺序提成数据。** 原来顺序藏在三处
（`session_stop` 的语句、`session_destroy` 的语句、`SessionHandle` 的**成员声明逆序**），
且全在 Android-only 文件里 —— 桌面测试树编不进来，"有单测守护"在旧结构上不可能。

现在顺序是 `session_teardown_plan.hpp` 里一份 `constexpr` 表：

```
kSessionDestroyPlan = 停视频 → 取音频位置 → 停音频 → 放会话 → 放音频 → 放窗口 → 放时钟
kSessionStopPlanSize = 3      // 停止 = 前 3 步，严格前缀，不是另一份列表
run_session_teardown(plan, hooks)   // 唯一的执行器
```

JNI 侧只提供七个钩子函数体（"每步做什么"），**顺序一行都不写**。

| 项 | 结果 |
| --- | --- |
| 单测 | `kivo_p12d4_lifecycle_tests` 6 例 / **36 断言**，0 失败 |
| 变异 ①：时钟提前 | **11 项红** |
| 变异 ②：采样挪到停音频之后 | **6 项红** |
| 还原 | 36 项全绿 |

变异 ② 最值得看：所有资源顺序都对，只有那个位置读到的是停流后的回退档 ——
"看起来完全正常、实际静默出错"，正是这套测试存在的理由。

**Kotlin 侧同时收敛**（D2 留的账）：`PlaybackSession` 成为句柄与两个 fd 的唯一入口，
`PlayerActivity` 不再存任何副本。判据是"数字不变"，所以重跑了两条取证路径：

| 轮 | rate | drift p99 | 帧 recv/rend/drop/disc | rebuild |
| --- | --- | --- | --- | --- |
| C8 归档 | 0.999984 / 0.999985 | 125 000 ns | 181/180/0/— | — |
| **C8 回归 ×3** | 0.999983–0.999986 | 121 500–125 000 ns | 181/180/0/0 | 0 |
| D1 归档 | 0.999979 | —（system） | 181/149/0/31 | 1 |
| **D1 回归** | 0.999973 | —（system） | 181/163/0/17 | 1 |

D1 空窗丢帧自洽：`574 ms × 30fps = 17.2`，实际 17。四份证据全部通过
`tools/verify_p12c_sync.py` 独立复算。

**顺带修两个真 bug**：① `session_destroy()` 从来不取音频位置（现在走全计划 + 幂等保护，
避免"先 stop 再 destroy"时用停流后的回退档覆盖好数据）；
② 结束证据被 400 ms 轮询**反复重写**（现在"成功才封口、失败重试"）。

- 设计 / 验收：`31-p12d4-design.md`、`32-p12d4-status.md`（§六 七条诚实清单）
- 证据：`p12d4-{c8reg-r1..r3,d1reg-r1}{,-sync}.json`
- 未做：钩子函数体本身（单测测不到，靠真机回归）；成员声明顺序仍靠注释维持

### P12-D5 · 上屏时刻（已结题，P12-D 随之结题）

**这张卡最有价值的部分是推翻了一条旧结论。** C7 当年说"上屏时刻在这个架构下拿不到"，
理由是 `dumpsys SurfaceFlinger --latency` 只返回刷新率一行。真相是：

```
SurfaceView[com.kivo.cinema/com.kivo.cinema.ui.PlayerActivity](BLAST)#115401
                                                                     ↑ 少了 #id 就没有数据
```

`--latency` 按名字匹配，匹配不到**不报错、不提示**，只安静地返回刷新率。
一条格式正确但没内容的输出，被读成了"平台不支持"。

| 列 | 含义 |
| --- | --- |
| `desiredPresentTime` | 缓冲时间戳。`surface_at_time` 下 = 我们传的 `due_at` |
| `actualPresentTime` | **合成器锁存这一帧的时刻（CLOCK_MONOTONIC）** |
| `frameReadyTime` | 厂商相关，不作判据 |

于是 `Δ = actual − desired` 就是"要它在这时刻上屏，实际晚了多少"，
而 `due_at` 按定义是"为音画同步该上屏的时刻" ⇒ **Δ 就是上屏端的音画偏差**。

| 轮 | 实测空闲率 | Δ 稳态 p50 | p99 | max | 稳态 vsync miss |
| --- | --- | --- | --- | --- | --- |
| idle ×3 | 91.6–94.1% | 8.11–8.85 | 16.33–16.45 | **≤16.47** | **0** |
| load5 ×2 | 27.6–41.6% | 7.40–9.17 | 16.21–16.28 | **≤16.41** | **0** |
| load16 ×2 | **1.1–15.5%** | 7.71–8.23 | 16.01–16.27 | **≤16.42** | **0** |
| **stall150 ×3** | 89.2–93.4% | 7.49–8.92 | 16.53–31.99 | **54.47–92.62** | **1–2** |

单位 ms，刷新周期 16.67 ms。**Δ 的上界正好卡在刷新周期** —— 这是"提交落在两个 vsync 之间、
合成器在下一个 vsync 锁存"的必然形状。整机只剩 1.1% 空闲时 0 miss，而冻进程 150 ms
就能把它推到 54–93 ms：两组不重叠，**说明量灵敏、结论成立**。

⚠️ 这张卡中途把证据全部推倒重测了一次：以为停掉的负载其实还在跑
（`TaskStop` 只杀本地 adb 客户端，远端 44 个忙循环仍在），于是"空载"轮其实是满载。
现在每轮记录**实测空闲率**，标签与实测不符会告警。见 `34-p12d5-status.md` §六。

- 设计 / 验收：`33-p12d5-design.md`、`34-p12d5-status.md`（§七 九条诚实清单）
- 工具：`tools/probe_present_timing.py`、`tools/verify_p12d5_present.py`（独立复算，9 份全过）
- 证据：`docs/evidence/p12d5/p12d5-*.json`（11 份，含实测空闲率与原始行）
- ⚠️ **两处旧结论已加更正块**：`23-p12c7-status.md` §六、`25-p12c8-status.md` §九。原文没删
- 口径：窗口 128 行 ≈ 4.3 s；Δ 是"锁存"不是"液晶翻转"（差一个扫描周期）；负载是纯 CPU 饥饿；稳态窗口 15 帧（改过，已在文档声明）

### P12-E1 · `COLOR_FormatYUVP010` 协商（已结题）

**结论是否定的，而且是两个方向都查过的否定。**

| 路径 | 观测 | 结论 |
| --- | --- | --- |
| 播放（Surface） | `color-format` = **0x7FA30C06**（2141391878，高通私有区） | 8bit AVC 与 10bit HEVC **给出同一个值** ⇒ 位深不可判定 |
| 探针（ByteBuffer） | 三次 configure 全部成功，start 后都报 **21 (NV12)** | 组件不采纳 `color-format`，"配置成功"不能当支持证据 |

探针里最关键的一行是**反证**：`color-format = Surface` 且**不给窗口**时，
configure 依然成功、start 依然成功。Surface 输出不可能没有窗口，
所以这颗 OMX 组件在 configure 阶段**根本不校验** `color-format`。

证据里因此写 `verdict = "indeterminate"` + 非空 reason（`decode/7` 新字段 `color`），
永不出现"看起来成功"的空字段。

**顺带量到一件对 E2/E3 很关键的事**：8bit 与 10bit 片源的
`color-standard/transfer/range` 都是 `1/3/2`（BT709/SDR/limited）。
ffprobe 核对片源 —— `color_space=unknown, color_transfer=unknown` ——
**那是解码器默认值，不是片源信息**。想验 HDR，必须先有带 BT2020+PQ 的真 HDR10 片源。

⚠️ **设计被实测推翻两处**（`35-p12e1-design.md` 已加更正块）：
① 以为 Surface 会报 `0x7F000789`；② 以为"P010 configure 成功 = 支持"且"Surface 无窗口会失败"。
另加一条：`getOutputFormat` 必须 **start 之后**读，configure 后读到的是默认值。

- 设计 / 验收：`35-p12e1-design.md`、`36-p12e1-status.md`（§五 七条诚实清单、§六 E2/E3 交接）
- 新增：`decode/color_format_names.*`、`decode/video_color_facts.*`、`probe/color_probe_main.cxx`
- 单测：`kivo_p12e1_color_tests` 10 例 / 52 断言
- 证据：`docs/evidence/p12e/` 5 份

### P12-E2 · HDR 色彩标记下发（2026-10-30 收口，核心项之一未达成）

**打通了"识别 → 下发 → 设 dataspace"，但静态元数据没下发。**

| 事实 | 证据字段 | 值 |
| --- | --- | --- |
| HDR10 片源被识别 | `color.hdr.kind` | `hdr10`（`standard=6/transfer=6`） |
| 显式下发 | `color.configure_format.copied` / `delivered_color_*` | `true` / `6,6,2` |
| dataspace 设上 | `color.hdr.dataspace` | `applied=true`，`163971072`（`ADATASPACE_BT2020_PQ`） |
| 无标记 SDR 不误判 | `color.hdr.kind` + `dataspace.applied` | `sdr` + `false`（两轮） |
| **静态元数据** | `color.hdr.static_info_bytes` | **`null`** —— 轨道格式里没有 `hdr-static-info` 键 |

**三处设计被实测打回**（`37-p12e2-design.md` 已加更正块）：

1. `color-standard` 没有 5/7：`BT601_NTSC=4`、`BT2020=6`；`BT2020_CL` 是 `DataSpace` 的常量。
   `color-transfer` 的 3 是 `SDR_VIDEO`，不是 `SMPTE_240M` —— E1 看到的 `1/3/2` 就是默认值。
2. `ANativeWindow_setBuffersDataSpace` **不在 `libandroid.so`**，在 `libnativewindow.so`。
   第一版 `dlsym(RTLD_DEFAULT)` 返回 null，证据里写成"API < 28"，而设备是 API 33。
3. 色彩键来自 **SPS VUI**，不是容器 `colr` 盒（自造片源里 `colr/mdcv/clli` 计数全 0）。

**顺手结掉 E1 的悬案**：无标记片源轨道格式三键全 `null`，输出仍报 `1/3/2` ⇒ 确认是解码器默认值。

- 设计 / 验收：`37-p12e2-design.md`、`38-p12e2-status.md`（§五 七条诚实清单、§六 E3/E4 交接）
- 新增：`decode/{color_key_names,hdr_kind,hdr_delivery_facts,source_color_tags}.hpp`、
  `decode/mediaformat_color_io.*`、`probe/hdr_probe_main.cxx`；
  `present/surface_dataspace.*` **此前写了没进构建，本轮接上**
- 单测：`kivo_p12e2_hdr_tests` 11 例 / 40 断言；`ctest -R kivo_p12` **8/8**
- 证据：`docs/evidence/p12e/p12e2-*.json`（6 份，schema `decode/8`）
- 片源：`tests/real/probe_1080p_hevc10_hdr10.mp4`（合成内容、真实元数据，生成命令见状态文档 §七）

---

### P12-E3 · BT2020 色域标记（2026-10-30 收口）

**判据"可查"落到了合成器侧**，不是"我们调用了、返回 0"。

| 层 | 观测 | HDR10 轮 | 无标记 SDR 轮 |
| --- | --- | --- | --- |
| 我们调用成功 | `hdr.dataspace.applied` | true | false（故意不动） |
| 窗口回读 | `hdr.dataspace_readback` | 163971072 | 0 |
| 合成器认账 | SF 图层 `dataspace` | **`BT2020_ITU_PQ` (298188800)** | `V0_BT709` (281083904) |
| 屏幕状态 | SF `dynamic_range` / `Current Color Mode` | **HDR** / `standard_hdr` | SDR / `smart_MC` |
| 静态元数据类型位 | SF `hdr metadata types` | 3 | 0 |

3 轮 HDR + 2 轮 SDR 全部一致；五轮 300/301/300 帧、0 丢帧、`completed=true`。

**三处坑（都会让人写出"永远通过"的判据）**：

1. **`HDR Panel:true` 是面板能力，不是状态** —— SDR 播放时同样 true。
2. **SF 的数值不等于我们设的数值**：298188800 vs 163971072（差 range 位）。
   判据比**名字**（BT2020 + PQ），比整数会得到"下发失败"的假结论。
3. **层名匹配少一个片段就静默拿到空** —— 探针改成显式失败并打印候选层名（D5 的教训）。

**`hdr metadata types=3` 的正确解读**：静态元数据的**类型位**到了合成器，
但那是解码器从 SEI 带上去的，不是我们下发的 —— E2 那条"`hdr-static-info` 未达成"的结论不变。

- 设计 / 验收：`39-p12e3-design.md`、`40-p12e3-status.md`（§四 六条诚实清单、§五 E4 交接）
- 新增：`tools/probe_layer_dataspace.py`、`tools/verify_p12e3_layer.py`（独立复算 5/5，变异能打红）、
  `read_surface_dataspace()`（C++ 只加了这一个回读函数）
- 证据：`docs/evidence/p12e3/`（10 份：5 图层 + 5 应用侧，schema `decode/9`）
- **Windows 坑**：给 Python 传 adb 必须用 `C:/...` 路径；传 `/c/...` 且带 `MSYS_NO_PATHCONV=1`
  会 `FileNotFoundError: [WinError 2]`

---

### P12-E4 · tone mapping（2026-10-30 收口，按字面达成、映射本体未实现）

**三条路径全部实测过，所以"不做"是有依据的话。**

| 路径 | 做法 | 结果 |
| --- | --- | --- |
| A 让屏幕变 SDR | `cmd display set-user-disabled-hdr-types 1 2 3 4` | **造不出来**：设完即被 MIUI 清回 `[]`，`dynamic_range` 仍 HDR |
| B 让解码器转 SDR | `color-transfer-request=3` | **被忽略**：`start()` 后 `output_transfer` 仍 6、`honoured=false` |
| C 自研 GL shader | SurfaceTexture→shader→Surface | **不做**：换架构，B/C/D 全部数字要重取证 |

**对比截图（同像素，只改 VUI 标记）**：

| 组 | 图层 dataspace / dynamic_range | mean | p50 |
| --- | --- | --- | --- |
| A PQ 标记 | `BT2020_ITU_PQ` / HDR | 105.99 | **71** |
| B SDR 标记 | `V0_BT709` / SDR | 91.15 | **61** |

标记错了不会花屏，只会**整幅发闷**（中位亮度 −14%）。这就是 E2/E3 那些 dataspace 工作的意义。

**算法（给全规格，不在本架构实现）**：线性域 + BT.2390 EETF + BT2020→BT709 + BT1886 输出，
落点 GPU 片元着色器；代价是零拷贝直出失效，P12-B/C/D 的 fps/丢帧/上屏时刻全部重取证。

- 设计 / 验收：`41-p12e4-design.md`、`42-p12e4-status.md`（§六 六条诚实清单、§七 E 组结题）
- 证据：`docs/evidence/p12e4/`（探针 1 + 图层 2 + 应用侧 2 + 亮度 1）；截图 `artifacts/p12e4/`
- 新工具：`tools/analyze_frame_luma.py`（用仓库 FFmpeg 解裸像素，**不依赖 numpy/PIL**）
- 证据字段：`color.hdr.tonemap.implemented=false` + 非空 reason，schema 升 **`decode/10`**

---

### P12-F1 · `seek_to` 接入会话（2026-10-30 收口）

| 轮次 | target | landing | prepare | first_frame | **total** | 段内帧数 |
| --- | --- | --- | --- | --- | --- | --- |
| r1 | 5 000 000 | 5 000 000 | 29 454 | 10 337 | **39 791** | 150/151/150，0 丢帧 |
| r2 | 5 000 000 | 5 000 000 | 17 546 | 10 539 | **28 085** | 同上 |
| r3 | 5 000 000 | 5 000 000 | 22 520 | 11 012 | **33 532** | 同上 |

（µs；预算 150 000。口径：终点是"交出"不是"上屏"。）

**做法**：`stop()`（只停线程、不动 codec）→ `extractor.seek_to`（PREVIOUS_SYNC）→
`decoder.flush()` → `begin_run()`（与 `start()` 共用的起跑段）。

**两件必记的事**：

1. **片源关键帧密度决定落点**。第一版用 `hdr10_1080p30.mp4`（整段只有一个关键帧），
   seek 到 5 s 落到 **0**、段内 300 帧 = 整段重播 —— 那是 `PREVIOUS_SYNC` 的正确行为，不是 bug。
   为此造了 `tests/real/probe_seek_1080p30.mp4`（每秒一个关键帧）。
2. **`seek_to` 原来恒返回请求值**（`return time_us;`），于是"落点"字段永远等于目标。
   现在返回 `AMediaExtractor_getSampleTime()`。恒真的字段比没有字段更糟。

- 设计 / 验收：`43-p12f1-design.md`、`44-p12f1-status.md`（§五 六条诚实清单、§六 F2 交接）
- 落位：`core/seek/seek_budget.{hpp,cxx}`、`session/decode_session.*`、renderer（`seek` 段，schema `decode/11`）、
  JNI `nativeSessionSeek`、`PlaybackSession.seekTo()`、`kivo.seek.ms` / `kivo.seek.at.ms`
- 单测：`kivo_p12f1_seek_tests` 8 例 / 20 断言；`ctest -R kivo_p12` **9/9**
- 证据：`docs/evidence/p12f1/p12f1-seek-r{1,2,3}.json`

---

### P12-F2 · 音视频双侧重同步（2026-10-30，**BLOCKED**）

**没做成。** 落点对（`gap = −8 ms`，5/5），但 5 轮里 **2 轮** seek 后约 1.7 s 连丢 101 帧：

| 轮次 | 渲染帧 | 丢帧 | `fallback_frames` | `catch_up_frames` |
| --- | --- | --- | --- | --- |
| r1 / r3 / r5 | 150 | 0 | 0 | 0 |
| **r2 / r4** | **49** | **101** | **50 / 51** | **91** |

**最危险的细节**：失败轮 `lateness.p50` 也是 **0**。只看时延会把"冻住 3 秒"读成"完美同步"，
必须同时看 `pace.clock_mix` 与 `pace.catch_up_frames`。

**三次修复都失败**：单次 rebase（1/3 失败）→ 二次 rebase（4/5 失败）→ 等时钟前进再 rebase（2/5 失败）。
第三次把"再多等一会儿"这条路否掉了：时钟重启后是活的，**约 1.7 s 后才掉进 `kFreeRun`**。

**解卡推荐**：不重启 AAudio 流，用"静音栅栏"代替 `ring.reset()`（会触碰 C2 的线程契约，需单独立卡）。

- 设计 / 验收：`45-p12f2-design.md`、`46-p12f2-status.md`（§六 六条诚实清单）
- 已落地：`AudioPlayback::seek_to`、时钟 `rebase` 三层、`assess_av_seek`、首帧 pts、`seek.av` 证据、JNI 编排
- 单测：`kivo_p12f1_seek_tests` 14 例 / 38 断言（桌面全过）
- 证据：`docs/evidence/p12f2/p12f2-avseek-r{1..5}.json`

---

### P12-F2A · 音频 seek 修复（2026-10-30 收口）

F2 的 BLOCKED 已解开。**两处改动各解一个不同的问题**：

| 改动 | 解决的问题 | 数据 |
| --- | --- | --- |
| 不重启 AAudio 流，改**静音栅栏**（`render()` 返回 0，一次不碰环） | 停流让 `getTimestamp` 成片失效 → `kFreeRun` → 3 s 追赶 | `fallback=0`、`catch_up=0`，6/6 轮 |
| **音频追视频落点**（`defer_start` 定位 → 音频按该落点 → 一起起跑） | 后向 seek 两边差一个 GOP（视频 0 / 音频 490 ms） | gap **490 ms → 0**，3/3 轮 |

**14 轮全绿**：gap 0 或 −8 ms、0 丢帧、`total_us` 19–60 ms。

**顺序细节**：`set_muted(true)` → 等 10 ms → 停喂 → 定位 → flush → `ring.reset()` →
重灌 → `set_muted(false)` → `clock.rebase(landing)`。**先放音后对表** —— 反了会让时钟在
静音期前进，视频提前。

- 设计 / 验收：`47-p12f2a-design.md`、`48-p12f2a-status.md`（§五 六条诚实清单）
- 落位：`ring_buffer_sample_source.hpp`、`audio_playback.cxx`、`decode_session.{hpp,cxx}`、`jni_session_handle.cxx`
- 证据：`docs/evidence/p12f2a/`（14 份；`superseded/` 是「音频按用户目标跳」的反例）

---

### P12-F3 · 错误路径全覆盖（2026-10-30，**3/4，加密轨 BLOCKED**）

| 用例 | 结果 | reason 要点 |
| --- | --- | --- |
| `bad-file` | ✅ | `container reports 0 tracks — likely truncated or corrupt` |
| `no-video-track` | ✅ | `no video track among 1 tracks` |
| `no-audio-track` | ✅ | `source has no audio track; there is no audio timeline…` |
| `encrypted` | ❌ `NOT_VERIFIED` | 设备把 CENC 读成明文（`is_encrypted=false`） |

**加密那条的硬事实**：`encv/sinf/senc/tenc` 齐全但 `pssh=0`，K40 上首包
`is_encrypted=false` ⇒ `submit()` 的拒绝分支不可达；该片源实际只出 25/90 帧、23 帧是垃圾像素，
却 `completed=true`、`error=""` ⇒ **G9 盲点**（平台没告诉我们加密）。

**改了一处代码**：`0 tracks` 与"有轨但没视频轨"原本同一句话，会把人引向"换片源"；现在单独成句。

- 校验：`python tools/verify_p12f3_errors.py docs/evidence/p12f3` → 3 PASS + 1 NOT_VERIFIED，**退出码 1**
- 设计 / 验收：`49-p12f3-design.md`、`50-p12f3-status.md`（§五 五条诚实清单）
- 证据：`docs/evidence/p12f3/`（6 份）；负向片源 `tests/real/negative_*.mp4`
- **解卡**：要带 `pssh` 的 CENC（mp4box / shaka-packager / CMAF），拿到后**代码不用改**

---

### P12-F3B · 加密轨识别（2026-10-30 收口，F3 4/4）

**原来那句"平台没告诉我们它加密了"是错的。** 平台报得清清楚楚，报在**轨道格式**里：

```
crypto-mode: int32(1), crypto-default-iv-size: int32(8), crypto-key: data
```

我们的 bug 是只读 `AMEDIAEXTRACTOR_SAMPLE_FLAG_ENCRYPTED`（K40 对真 CENC 恒不设这位）。

| 补测步骤 | 结果 | 排除了什么 |
| --- | --- | --- |
| 手工注入 `pssh`（`tools/inject_pssh.py`） | `is_encrypted` 仍 false | "缺 pssh" |
| 分片 fMP4（4 × `moof`） | 仍 false | "非分片不支持" |
| 探针 `track_format_dump`（`hdr-probe/3`） | **`crypto-mode:1` 在** | 定位到真正的信号 |

**改判与拒绝点**：

- 主判据 `crypto-mode`（int32，API 21）；`crypto-key` 只作旁证（读它要 API 28，minSdk 26）。
- 在 `open()` 里、**`configure()` 之前**拒绝，返回码 **12**。
  原来那条路会"配好解码器 → 解出 25/90 帧垃圾像素 → `completed=true`"，比拒绝更糟。

**三态判定（最关键的设计）**：

| `crypto-mode` | verdict | 行为 |
| --- | --- | --- |
| 1 / 2 / 其它非 0 | `encrypted` | 拒绝，reason 点名模式 |
| 0 | `clear` | 放行 |
| 键缺席 | **`unknown`** | **放行但留 reason** |

第三态不能省：K40 上明文片源同样没有这个键（`crypto_mode_present=false`）。

**实测**：加密 → `stage=open` / `rc=12` / `track declares crypto-mode=1 (aes-ctr)`；
明文对照 → `verdict=unknown` / 300·301·300 帧 / 0 丢帧 / `completed=true`。

- 校验：`verify_p12f3_errors.py` **4 PASS / 退出码 0**；变异（rc→5）打红
- 单测：`kivo_p12f3b_encryption_tests` 8 例 / 33 断言；`ctest -R kivo_p12` **10/10**
- 证据：`docs/evidence/p12f3b/`（4 份）；旧行为留 `docs/evidence/p12f3/superseded/`
- 文档：`51-p12f3b-design.md`、`52-p12f3b-status.md`（六条诚实清单 + 复算步骤）

---

### P12-F4 · 证据总收口（2026-10-30，P12 结题）

**一条命令回答"P12 整体还算不算数"：**

```bash
python tools/verify_p12_all.py              # 全量收口
python tools/verify_p12_all.py --self-test  # 证明它能打红
```

实测：**230 份证据 / 四层全通过 / 7 个子校验器 exit 0**，另含 **46 项已声明豁免**
（16 份早期音频证据无时间戳 + 30 份长稳中途快照）。

| 层 | 管什么 |
| --- | --- |
| 1 结构 | 可解析、schema 在白名单（24 种）、有实测时间戳、无占位符 |
| 2 卡片 | 12 组不变量（判据抄自各卡验收文档） |
| 3 红线 | 零静默失败 / 零无解释 / 零含糊 / 恒真字段告警 |
| 4 复用 | 调 6 个既有校验器（7 次），**不重写判据** |

**自检 8/8**：8 个变异（删 schema、未知 schema、塞 TODO、静默失败、不可测无理由、
HDR 未设 dataspace、seek 超预算、谎称实现 tonemap）全部被抓到，抓不全就退出码 2。

**收口时修正了 3 处我自己写错的判据**：长稳中途快照的 `completed=false` 不算静默失败、
负向样本不该要求 `reached_eos`、`paced` 传错了证据集（该传 `p12d2-paced-*.json`）。

**一次操作事故**：给探针加时间戳重跑时用 10bit 片源覆盖了 `p12e1-probe-8bit.json`；
靠同组 `p12e1-8bit-r1.json` 的 `source.mime`（`video/avc`）对照发现，已用 H.264 源重跑修复。
教训：**重跑证据前核对源与同组播放证据一致**。

- 设计 / 验收：`53-p12f4-design.md`、`54-p12f4-status.md`（§五 七条诚实清单、§六 能力边界、§七 交接）
- 机读总表：`artifacts/p12f4/report.json`
- 新增：`tools/verify_p12_all.py`；探针与 `analyze_frame_luma.py` 补 `generated_at_unix_ms`

---

### P13-A · 4K / 60fps / 高码率（2026-10-30 收口）

**四组全部零丢帧，但顺带证伪了一条既有结论。**

| 片源 | 分辨率 | 位深 | 码率 | fps | 实时比 | 首帧 |
| --- | --- | --- | --- | --- | --- | --- |
| `p13a_1080p60_hevc50m` | 1080p | 8 | **46.7 Mbps** | 60.2 | 1.00 | **47.0 ms** |
| `p13a_4k60_hevc8` | 4K | 8 | 23.0 Mbps | 60.0 | 1.00 | 66.3 ms |
| `p13a_4k60_avc8` | 4K | 8 | 16.8 Mbps | 59.4 | 0.99 | 115.9 ms |
| `p13a_4k60_hevc10` | 4K | **10** | 25.4 Mbps | 59.2 | 0.99 | **132.5 ms** |

301 收 / 300 上屏 / **0 丢帧** / 0 追赶 / `completed=true`（四组一致）。
10bit 组 `color.source_is_10bit=true`，位深确实进了解码器。

**三条结论**：

1. **能力表的"5% 余量"是保守的**：4K60 AVC 声明 63fps（余量 1.05×），实测 0.99× 实时零丢帧。
   `achievableFrameRatesFor` 是**下界**，别拿它当上限判死。
2. **10bit 4K60 没有可见代价**（59.2fps）—— 但这只对这台设备成立，测法要保留。
3. **首帧时延随分辨率上升**：47 → 66 / 116 / **133 ms**。**150 ms 的 G8 预算在 4K 上不够。**

- 验收：`56-p13a-design.md`、`57-p13a-probe-status.md`、`58-p13a-status.md`
- 证据：`docs/evidence/p13a/`（5 份）；片源 `tests/real/p13a_*.mp4`
- 新探针：`adb shell am start -n com.kivo.cinema/.ui.FramerateDumpActivity --es kivo.out x.json`
- 单测：`kivo_p13a_framerate_tests` 9 例 / 21 断言；`ctest -R "kivo_p12|kivo_p13"` **11/11**

**顺带修掉一个 bug（比 4K 结论更重要）**：`decode_stats_renderer` 里有两个 `"source"` 键
（原有 `{mime,width,height,duration}`，F3B 又加了一个 `{encryption}`），JSON 重复键让前者
静默消失，活了 6 张卡。收口层 1 现在用 `object_pairs_hook` 直接查重复键。

---

### P13-B · 冷启动 / seek 首帧分离（2026-10-30 收口）

**这张卡的产出是三条自我更正。**

| 判据 | 预算 | 1080p | 4K8 | 4K10 |
| --- | --- | --- | --- | --- |
| **G8a 冷启动** | 300 ms | 33–42 ms | 54–92 ms | **122–146 ms** |
| **G8b seek** | 150 ms | 28–43 ms | 36–48 ms | 40–49 ms |

21 轮冷启动 + 9 轮 seek 全 pass。

**更正一**：P13-A 说"4K 上 150ms 不够"，错在把**冷启动**（132ms）当成了 **seek 首帧**
（实测 36–49ms）。二者共用计数器 `counters.first_frame_latency_us`，而 `begin_run()`
会重置它 —— 一份证据里分不出来。

**更正二**：即使按 150ms 判冷启动，132ms 也没超。"不够"不成立，真实是余量 3.2× → 1.13×。

**更正三**（过程中）：首版 G8a 阈值定 200ms（据 P13-A 单轮数据），真机首轮 206ms 就红。
18 轮测出 4K10 分布 122–146ms，改 300ms。**阈值必须按已观测最大值定，不能按中位数或单轮。**

**代码**：`assess_cold_start()`；`DecodeSession::cold_start_first_frame_us_`（第一次 `seek_to()`
前抓一次，seek 不覆盖）；证据新增 `cold_start` 段，schema → **`decode/13`**。

- 验收：`59-p13b-design.md`、`60-p13b-status.md`（§六 五条诚实清单）
- 证据：`docs/evidence/p13b/`（21 份）
- 单测：`kivo_p12f1_seek_tests` 19 例 / 58 断言；`ctest` **11/11**

---

### P13-C · 4K + 音频（2026-10-30 收口）

**12 轮全部零丢帧、零 xrun、音频真的打开。结论和预期相反：分辨率不是变量，帧率才是。**

| 组 | n | p50 | p95 | p99 | max |
| --- | --- | --- | --- | --- | --- |
| P12-C 1080p**30**（基线） | 180 | 0.125 | 0.125 | **0.125–0.375** | 0.14–0.50 |
| P13-C 1080p**60** | 480 | 0.125 | 0.125–0.375 | **0.625–1.875** | 7.26–12.55 |
| P13-C **4K**60 | 480 | 0.125 | 0.125–0.375 | **1.125–1.375** | 3.99–20.62 |

（ms；`cpu_paced` 下可测，`surface_at_time` 下按 C8 结论不可测）

- **4K60 与 1080p60 几乎相同** → 4K 的解码负载没有传导到音频时钟。
- **60fps 让 p99 涨约 4 倍**（0.375 → 0.6–1.9ms）。p50 恒 0.125ms（量化步长），
  说明是**采样密度翻倍、尾巴看得更清**，不是漂移真的变大。
- 绝对值仍安全：最大单次尖峰 20.6ms，480 次采样中出现一次。

**修掉两个「证据字段恒错」的 bug**：

1. `audio.opened` **从未被赋值** —— 15 份历史 sync 证据全写 `false`，
   而同一份里 `clock_source: audio_timestamp`、`av_drift.samples: 180`。
   `false` 看起来完全合理，所以一直没人发现 —— **恒错的字段比缺失的字段更危险**。
2. `audio.started` 被 `stop()` 清掉（证据在 stop 后采集）。证据记的是"这一轮发生过什么"，
   不是"此刻的内部状态"。

**收口新增跨字段检查**：`clock_source == audio_timestamp` 或 `av_drift.samples > 0` 时，
`audio.opened`/`started` 必须都为 true。单看一个字段永远发现不了这类矛盾。

- 验收：`61-p13c-status.md`（§五 五条诚实清单，含"surface_at_time 下 drift 不可测"）
- 证据：`docs/evidence/p13c/`（24 份）；片源 `tests/real/p13c_{4k60,1080p60}_av.mp4`

---

### P13-D … P13-K · 漂移这件事的六层（2026-10-30 陆续收口）

这一段没有单列小节，因为每张卡的结论都写在 `62`–`77` 号文档里。只留一张索引：

| 卡 | 一句话 | 文档 | schema |
|---|---|---|---|
| P13-D | 按媒体时间 50 ms 绝对格点抽稀 + 环形序列 + 逐点回归 | `62`/`63` | `/3` |
| P13-E | 分位数必须与采样口径绑在一起（三个线格式名字） | `64`/`65` | `/3` |
| P13-F | `max` 只说多大、说不出在片头还是片尾 | `66`/`67` | `/4` |
| P13-G | 结构门（≥2 样本）与统计门（≥30 样本）必须分开 | `68`/`69` | `/5` |
| P13-H | `observed_hz` 的分母取错了口径，超过 102.4 s 后无上界 | `70`/`71` | `/6` |
| P13-I | `truncated` 拿整场累计比容量，seek 之后谎报覆盖 | `72`/`73` | `/7` |
| P13-J | 斜率没有误差棒；八窗中位数全被 0.25 ms 桶中心吃掉 | `74`/`75` | `/8` |
| P13-K | SE 换成 HAC（Newey–West）；**前提没被证实**（ρ1 只有 ±0.02） | `76`/`77` | `/9` |

### P13-L · 把"看得见"翻译成"要紧不要紧"（2026-10-30 收口）

**`detectable = true` 只说"与 0 分得开"，不说"要紧"。三轮真机第一次把这句话量化了。**

| 轮次 | 片源 | 样本 | `slope_ppm` | 95% CI | `detectable` | 累积量 | 保守上界 | 用掉 45 ms 额度 |
|---|---|---|---|---|---|---|---|---|
| r1 | 130 s 360p30 | 2048 | −0.237854 | [−0.624, +0.149] | false | −24.3 µs | 63.9 µs | **0.142%** |
| r2 | 130 s，60→30 s seek | 2000 | −0.343318 | **[−0.683, −0.003]** | **true** | −34.3 µs | 68.3 µs | **0.152%** |
| r3 | 35 s 360p30 | 700 | −0.997770 | [−2.289, +0.293] | false | −34.9 µs | 80.0 µs | **0.178%** |

三轮零丢帧、零 xrun。**r2 是靶心**：`detectable = true` 而 `within_budget`
—— 两张卡的分歧第一次同时出现在真机证据里，34.3 µs 等于额度的 0.076%。

- 额度 **45 ms**，取 ITU-R BT.1359 的音频超前可检测阈值（对称使用，比标准偏严）。
- 判定用**置信带远端**积出的上界，不用点估计 —— 一份"可能听得出来"的证据不该以"大概率没事"结案。
- 三态 `unknown` / `within_budget` / `over_budget`：`false` 同时意味"低于预算"和"没算出来"，
  这两种对读者的指示完全相反。
- **顺带撞出缺陷**：向前 seek 不切段（见 §一"下一步做什么"）；物证隔离在
  `docs/evidence/p13l/defect/`，不进总收口。
- 验收：`78-p13l-design.md`、`79-p13l-status.md`（§五 十条诚实清单）
- 证据：`docs/evidence/p13l/`（6 份 `/10`）；单测 `p13l_budget_tests` 11 例 / 207 断言

### P13-M · 段边界的权威是锚点重建（2026-10-30 收口）

**P13-L 撞出的那份缺陷，同一个场景重跑之后自己消失了。**

| 字段 | P13-L 缺陷轮 | P13-M 同场景 | 差 |
|---|---|---|---|
| `segments` | `1` | **`2`** | 切段真的发生了 |
| `truncated` | `true` | `false` | 第二段 1800 < 2048 |
| `span_ns` | 112.367 s | **89.967 s** | 十秒空白不再算进跨度 |
| `media_span_ns` | 129.967 s | **119.933 s** | 30 + 90，不是"末 − 首" |
| `observed_hz` | 18.466 | **20.0111** | 那个"采样不足"告警是假的 |
| `unobserved_cells` | 200 | **0** | 跨锚点不是"空格点" |

- **根因不是阈值，是信号**：上一版拿 `media_ns < last_media_ns_` 推断"换了时间轴"。
  倒退确实是换了（充分条件），但**前进推不出"没换"** —— 向前 seek 正是"前进且换了"。
  用存在性推断不存在性，必然漏一次。
- **换信号**：段边界由 `FramePacer::reset()` 决定（那里 `tracker_.reset()` 正是
  `lateness` 零点被重新定义的一刻）。`segments` 收窄成"一次锚点生命周期算一段"。
- **不加阈值切段**：卡顿与向前 seek 在 pts 上不可区分，而正确处理相反 ——
  卡顿之前的点是真数据，切了就是拿真数据赔告警。段内空洞改为**出版**
  `segment_largest_gap_cells` / `segment_contiguous`（阈值 20 格 = 1 s）。
- **四轮真机**：向前 seek / 向后 seek / 平跑 / **冻结 2 s**。前三轮零丢帧零 xrun；
  冻结轮用 `kill -STOP` 造出 `gap = 45`（2.25 s）、`contiguous = false`、
  `observed_hz = 19.6589`、xrun 1 —— 段内空洞第一次在真机上被看见，
  且**没有被误判成 seek**（`segments` 仍是 1）。
- 验收：`80-p13m-design.md`、`81-p13m-status.md`（§六 十条诚实清单）
- 证据：`docs/evidence/p13m/`（8 份 `/11`）；单测 `p13m_segment_tests` 14 例 / 75 断言
- 变异：单元层 12 处（11 杀死 + 1 等价重构仍绿）、校验器层 7 处（6 杀死 + 1 仍绿）

### P13-N · 分位数必须自己说出精度（2026-10-30 收口）

**同一份证据里的两个 p50 差 66%，现在它们不打架了 —— 因为第一个自己说出了范围。**

| 字段 | 值 | 读法 |
|---|---|---|
| `p50` | 125 000 | 桶中心，**不是量出来的值** |
| `p50_range_low_ns` / `_high_ns` | 13 490 / 250 000 | 真值在这个区间里 |
| `p50_bucket_samples` | 2553 / 2600 = **98.19%** | 98% 的样本挤在同一个桶里 |
| `bucket_width_ns` | 250 000 | 尺子的刻度 |
| `median_of_window_medians_ns` | **73 999** | 同一批点的精确参照，落在上面那个区间里 |

- **缺陷来自 P13-J 只改了一半**：窗口那份换成了精确选值，整场 `p50` 还是桶中心。
  不改时两个都是 125 000（读者会去查桶宽）；改一半时一个 125 000 一个 73 646，
  读者只会认定数据有问题 —— **改一半比不改更危险**。
- **`p50` 的值一个字没改。** 改的是它旁边多了三格。要真做整场精确分位数，
  得存全样本（内存随时长涨）或引入分位数草图（新算法 + 新误差分析）——
  为一格差 ±125 µs 的数，代价与收益不成比例。
- **结构性修复**：`percentile_ns` 原来在函数体内联定位循环，`percentile_bucket` /
  `percentile_range` 若各写一遍，必然在秩边界上分家。抽成私有 `locate(q)` 三者共用。
- **范围算在 core 不算在报告层**：`av_sync_report.cxx` 只在 Android 侧编译，
  写在那里的钳制逻辑没有桌面单测；而"下界取大、上界取小"写反会得到一个
  **不含 p50 的空区间**，那在 JSON 里长得像"范围很窄、很准"。
- **三轮真机零丢帧零 xrun**：`p50` 全是 125 000（九个块无一例外），
  精确参照 73 999 / 77 334 / 77 395 **全部落在自己声明的范围内**。
  r2 的 `segments = 2` / `observed_hz = 20.0111` 与 P13-M 同一场景逐位一致。
- 验收：`82-p13n-design.md`、`83-p13n-status.md`（§六 十条诚实清单）
- 证据：`docs/evidence/p13n/`（6 份 `/12`）；单测 `p13n_quantile_tests` 11 例 / 76 断言
- 变异：单元层 10 处（9 杀死 + 1 等价重构仍绿）、校验器层 8 处（7 杀死 + 1 仍绿）。
  C8（删 `sort`）第一次**存活** —— 输入是递增的，排序与否给同一个数，
  改成递减才分开。**凡"排序/取极值"的量，输入必须让两种算法给不同结果。**

---

## 三、怎么跑起来（照抄，别改）

> **首次接管先补 `android/local.properties`。** 这个文件在 `android/.gitignore` 里，
> **仓库里根本没有它**，而本机 `ANDROID_HOME` / `ANDROID_SDK_ROOT` 都是空的，
> 所以不补就一定失败（实测 18 秒后报错）：
>
> ```
> SDK location not found. Define a valid SDK location with an ANDROID_HOME
> environment variable or by setting the sdk.dir path in your project's
> local properties file at '...\android\local.properties'.
> ```
>
> ```bash
> echo 'sdk.dir=C:/Android/sdk' > android/local.properties
> ```
>
> **另外本机 `JAVA_HOME` 是空的、PATH 里也没有 `java`**，不带就报
> `ERROR: JAVA_HOME is not set and no 'java' command could be found in your PATH.`。
> 所以下面构建命令里那个 `JAVA_HOME=...` 前缀**不是可选项，别删**。

```bash
# 0. 确认设备在线（必须是 K40，别用错机器）
adb devices -l          # 期望看到 b6e950ee

# 1. 构建
cd "/c/kivo video/android"
JAVA_HOME=/c/Android/jdk-17.0.13+11 ./gradlew.bat assembleDebug --no-daemon

# 2. 安装（HyperOS 会拒绝普通 install，必须走 push + pm install）
adb push app/build/outputs/apk/debug/app-debug.apk /data/local/tmp/kivo.apk
adb shell "pm install -r -t /data/local/tmp/kivo.apk"

# 3. 放片源（应用按文件名排序取第一个）
adb push <clip>.mp4 /sdcard/Android/data/com.kivo.cinema/files/media/a_clip.mp4

# 4. 启动（自动探测 → 播放 → 播完落盘统计）
adb shell "am start -n com.kivo.cinema/.ui.PlayerActivity"

# 5. 取证据
adb shell "run-as com.kivo.cinema cat files/evidence/p12b-decode.json"

# 6. 音频链路探针（P12-C 起；独立进程，不依赖 APK）
#    build/p12c/ 下的可执行文件是从 Gradle 产物拷过来的，源头在
#    android/app/build/intermediates/cxx/Debug/<hash>/obj/arm64-v8a/
#    改了 clock / probe 源码后必须重新 assembleDebug 再拷一次，
#    否则推上机的还是旧二进制 —— 这个坑不报错，只会安静地给出旧数据。
ADB=/c/Android/sdk/platform-tools/adb.exe
MSYS_NO_PATHCONV=1 $ADB push build/p12c/kivo_audio_probe /data/local/tmp/kivo_audio_probe
MSYS_NO_PATHCONV=1 $ADB shell "chmod 755 /data/local/tmp/kivo_audio_probe"   # push 会重置权限，每次都要做
MSYS_NO_PATHCONV=1 $ADB shell \
  "/data/local/tmp/kivo_audio_probe 3 440 > /data/local/tmp/p12c.json 2>/data/local/tmp/p12c.err"
MSYS_NO_PATHCONV=1 $ADB pull /data/local/tmp/p12c.json docs/evidence/p12c/<name>.json

# 7. 真实音轨探针（P12-C3 起；两个入口，参数语义不同，别合并）
MSYS_NO_PATHCONV=1 $ADB push build/p12c/kivo_audio_file_probe /data/local/tmp/kivo_audio_file_probe
MSYS_NO_PATHCONV=1 $ADB shell "chmod 755 /data/local/tmp/kivo_audio_file_probe"
MSYS_NO_PATHCONV=1 $ADB shell \
  '/data/local/tmp/kivo_audio_file_probe /data/local/tmp/aac_48k_stereo_lr.mp4 > /data/local/tmp/p12c3.json 2>/data/local/tmp/p12c3.err'
# 片源在 C:/Android/probe/media/：aac_48k_stereo_lr.mp4（左 440Hz / 右 880Hz）、aac_44k1_mono.mp4

# 8. 节拍取证（P12-C5 起；走 APK，因为必须有真 Surface 才能硬解上屏）
#    五个 extra：kivo.media / kivo.pace / kivo.drop / kivo.fps / kivo.evidence
#    一个都不传时行为与 P12-B 完全一致（播外部目录第一个文件、不开节拍）。
MEDIA=/storage/emulated/0/Android/data/com.kivo.cinema/files/media   # App 私有可写目录
MSYS_NO_PATHCONV=1 $ADB push "C:/Android/probe/media/h264_1080p24.mp4" "$MEDIA/"
MSYS_NO_PATHCONV=1 $ADB shell "am force-stop com.kivo.cinema"
MSYS_NO_PATHCONV=1 $ADB shell "am start -n com.kivo.cinema/.ui.PlayerActivity \
  --es kivo.media $MEDIA/h264_1080p24.mp4 \
  --ei kivo.pace 1 --ei kivo.drop 1 --ei kivo.fps 24 \
  --es kivo.evidence p12c5-pace-24fps.json"
sleep 15   # 8s 片源 + 起流开销；带了 kivo.evidence 时 App 写完证据会自动退出
MSYS_NO_PATHCONV=1 $ADB shell "run-as com.kivo.cinema cat files/evidence/p12c5-pace-24fps.json" \
  > docs/evidence/p12c/p12c5-pace-24fps.json
# 三档片源：h264_1080p24.mp4（8s/24fps）、h264_1080p8.mp4（6s/30fps）、h264_1080p60.mp4（8s/60fps）
# 关掉节拍作对照：把 --ei kivo.pace 改成 0（同一片源应当跑到 7× 实时以上）
```

> **向 adb shell 传重定向时要整段用单引号。**
> 写成 `$ADB shell "... > /data/local/tmp/x.json"` 时，本地 Git Bash 会把 `>` 当自己的重定向，
> 结果是本地报 `/data/local/tmp/x.json: No such file or directory`，而远端什么都没收到。

探针的完整编译命令见 `docs/design/backend-core/12-p12c-status.md` §六
（直接 clang++ 交叉编译，约 10 秒；CMake 里也已正式注册为 `kivo_audio_probe` 目标）。

探针第 3 参是供给方式、第 4 参是供给倍速（P12-C2 起）：

```bash
# 正弦直连回调（C1 基线）
$ADB shell "/data/local/tmp/kivo_audio_probe 3 440"
# 经环形缓冲、实时供给 → 期望 silenced=0
$ADB shell "/data/local/tmp/kivo_audio_probe 5 440 ring 1.0"
# 经环形缓冲、半速供给 → 期望 silenced>0（这是"计数器有效"的证据，不是故障）
$ADB shell "/data/local/tmp/kivo_audio_probe 5 440 ring 0.5"
```

环形缓冲单测（主机侧，无需设备）：

```bash
cmake --build build/vs-debug --target kivo_p12c_ring_buffer_tests --config Release
./build/vs-debug/backend/Release/kivo_p12c_ring_buffer_tests.exe
# 或 cd build/vs-debug && ctest -R kivo_p12c_ring_buffer_tests -C Debug
```

> **构建前先 `unset http_proxy https_proxy`**：本机 Git Bash 里大小写两套代理变量并存，
> .NET 的 MSBuild 会因"大小写重名"报 `MSB6001 CL.exe 命令行开关无效`，
> 报错信息完全看不出是这个原因（见 §五）。

桌面端 C++ 构建（做 core 层单测时用）：见根目录 `CMakePresets.json`，
FFmpeg 走 `third_party/ffmpeg-sdk/`（**不要**再去动原 SDK 的权限，会卡管理员）。

---

## 四、环境与设备

| 项 | 值 |
| --- | --- |
| 工具链根 | `C:\Android`（零管理员，不写系统目录） |
| JDK | Temurin 17.0.13+11 |
| Gradle / AGP | 9.6.1 / 9.4.1（AGP 9 内置 Kotlin） |
| NDK | 30.0.16248370（r30） |
| compileSdk / minSdk / targetSdk | 37 / **26** / 37 |
| ABI | 仅 `arm64-v8a` |
| 主测试机 | Redmi K40 `b6e950ee` / alioth / SM8250 / Android 13 API 33 / Adreno 650 |
| 下限验证机 | Coolpad C106 / API 23（仅用于降级路径，不作主基线） |

细节（含 AGP 9 三处硬变更、环境变量、镜像源）见 `docs/design/backend-core/06-android-toolchain.md`。

---

## 五、会咬人的坑（全是真踩过的）

| 坑 | 现象 | 正确做法 |
| --- | --- | --- |
| `AMediaExtractor_setDataSource(路径)` | 返回 `-10002 MALFORMED`，但文件完好的 | 一律用 `setDataSourceFd(fd, 0, len)`；路径只作显示名 |
| logcat 不可用 | `adb logcat -d` 返回 **0 行**（HyperOS 关了 logd） | 别靠日志诊断。失败走句柄回传 + 落盘 JSON |
| `adb install` 被拒 | `INSTALL_FAILED_USER_RESTRICTED` | push 到 `/data/local/tmp` 再 `pm install -r -t` |
| adb unauthorized | 首次连接没弹窗 | 重建 `~/.android/adbkey`（旧 key 备份在 `keybackup-20261028`） |
| bash 路径转换 | `adb push` 目标路径被 MSYS 改写 | 用 PowerShell，或 `MSYS_NO_PATHCONV=1` |
| NDK codec 枚举 | `AMediaCodecList` 不存在；r30 的 `AMediaCodecStore` 要 API 36 | 枚举只能走 Java `MediaCodecList` |
| API 28 符号撞 minSdk 26 | `AMediaCodec_getName`、`AMEDIAFORMAT_KEY_CSD_0` | getName 用 dlsym；键名用字面量 `"csd-0"` / `"rotation"` |
| `AMediaFormat_copy` | 是 API 29，minSdk 26 上没有 | **不要试图克隆格式**，configure 不接管所有权 |
| extractor 标志位透传 | `ENCRYPTED=2` 撞上 codec 的 `CODEC_CONFIG=2`，真实数据被当配置包丢弃 | 必须经中立语义翻译（见 `sample_view.hpp`） |
| 按 codec 名筛解码器 | `c2.qti.hevc.decoder` 只在 XML 声明，运行时不存在 | 只按能力筛，不按框架/命名筛 |
| 能力上限取大值 | 打开成功但首帧解码失败 | 取同硬件各组件**最小值** |
| `adb push` 覆盖文件 | 之前 `chmod 755` 白做，运行报 `rc=126 Permission denied` | 每次 push 后重新 `chmod 755` |
| AAudio 枚举名凭印象写 | `AAUDIO_FORMAT_PCM_32_BIT`、`AAudioDataCallback` 等根本不存在 | 一律照 `aaudio/AAudio.h` 抄，实际名是 `AAUDIO_FORMAT_PCM_I32` / `AAudioStream_dataCallback` |
| MSBuild 报 `MSB6001 命令行开关无效` | 实际原因是 `HTTP_PROXY` 与 `http_proxy` 同时存在，.NET 字典把它们当重名 | 构建前 `unset http_proxy https_proxy` |
| 预灌放在 `start()` 之后 | 前 13 个 burst（2496 帧 / 52ms）被记成欠载，数字每次一模一样 | 预灌必须在 `session.start()` **之前** |
| 供给线程用 `sleep_for` | 相对睡眠每轮略长，5 秒累积约 40ms 缺口，看起来像缓冲不够 | 用 `sleep_until(绝对截止时刻)` |
| **取证时屏幕会睡** | 报告写出 `decided=1800 / presented=84 / dropped=0`、`drop_ratio = 0.0`，而**没有判据看这一条**；`lateness_ns.p99` 甚至到 43.6 ms。⚠️ P14-Q 之后要补一句：那些帧**很可能记在 `frames_discarded_no_surface` 里**（锁屏轮实测 1799 帧一帧不差），"凭空消失"是当时的推断而非实测 | 取证脚本开头先 `svc power stayon usb` + `input keyevent KEYCODE_WAKEUP` + `wm dismiss-keyguard`（见 `tools/run_p14p_lateness_range.sh`）。**别改系统的 `screen_off_timeout`**，那是用户的设置 |
| **设备锁屏 ≠ 屏幕熄灭** | App 照样启动、`decided=1800`，但 `presented=1`、`dns=1799` —— 报告看起来像"节拍崩了"，实际是窗口不可见。更糟的是 `input swipe` / `wm dismiss-keyguard` / `input keyevent 82` / `locksettings set-disabled` / **重启**全部无效（MIUI 拦注入事件），只能人工解锁 | 取证前**先查 `dumpsys window \| grep mDreamingLockscreen`**，为 `true` 就 `exit 2`。`run_p14q_frame_accounting.sh` 与 `run_p14r_end_reason.sh` 都已加 |
| **两份报告的 `frames_discarded_no_surface` 不在同一块** | 节拍报告的 `video` 块里有，解码报告**只有** `counters` 块里有。判据若只读前者，读不到就按 0 处理 —— 而"播完了"的时候差额本来就是 0，这个 0 永远不显形（P14-R 的第一份打断证据当场把它照出来） | 判据两处都要找（`video` → `counters`），且**两处都没有**时单独报"缺格"而不是"帧丢了" |
| **`verify_p12c_sync.py` 只认 `sync/1`** | 四项闭合式（`rendered+dropped+dns+EOS == received`）在 `/2` 之后**从来没有判据在判**。扩到 `/29` 时它先报"缺 histogram"与 `KeyError('p50')` —— 那两个不是证据坏了，是脚本还在读 `/1` 的字段位置 | 升 sync 版本号时，除了 `KNOWN_SCHEMAS` 与 `SYNC_QUANTILE_BLOCK_SCHEMAS` **两张表**，还要看这个脚本认不认；P14-R 已扩到 `/29`，`/2`–`/28` 仍未判 |
| **EOS 那一帧只在"自然结束"时存在** | `frames_received` 比 `rendered+dropped+dns` 多 1，那 1 是末尾的 EOS 缓冲。被打断的会话没有它 —— 判据若写死 `+1`，一份**本该通过**的打断证据会红，且方向指向"漏交缓冲" | 四项闭合的常数由 `end_reason` 决定：`completed` 才 `+1`（`verify_p12c_sync.py`） |
| **`playback_rate` 的分子不是 pts** | `timeline.video_pts_span_ns` 这个名字说的是 pts 跨度，实际是 `(rendered+dropped+dns−1) × 名义帧时长` —— 216/216 份逐份核对，一条不差。于是那 15 份"速率越界"里 11 份是假的（0.645 那种数是帧数换成的秒除以墙钟） | `/30` 起改名 `nominal_span_ns` / `nominal_rate`，区间改给 `media_rate`（分子取 `pace.pts_span_ns`，实测、`/16` 起就在）。判据另外禁止 `/30` 上出现旧名 |
| **"同一份报告里两个 pts 跨度"** | `timeline.video_pts_span_ns`（乘法）与 `pace.pts_span_ns`（实测）都自称跨度，读者看得见的是乘法那个。P14-R 那条"打断轮上界不成立"就是被它逼出来的：换实测跨度是 1.000026，正中间 | 一个量只有一个名字。改名而不是加字段 —— 两个名字指同一个数，读者没法知道该信哪个 |
| **用文件名给证据做键会丢样本** | `p14g/p14g-long1800-r1-sync.json` 与 `p14g/pre/` 下那份同名，`seen[path.name]` 让 216 份只记了 215 条，而汇总行照样打印"合计 215 份" —— 判据在数自己的样本时丢了一份还报得很自信 | 键用 `docs/evidence/` 相对的**路径**，不用基名 |
| **判据的版本范围放开了，喂它的文件清单没跟上** | `verify_p12c_sync.py` 扩到 `/1`–`/30`，层 4 仍只喂 `p12c/` 的 27 份 —— 189 份照样没人判，而输出上完全看不出来（通过的行长得一样）。这是 P13-F "两张表只更新一张"的老病换了个维度：从文件内的两张表变成两个文件之间 | 清单改成递归 + 子校验器用 `覆盖：N 份` 自报份数 |
| **`av_drift` 是"相对钟"，不是"相对耳朵"** | 它量的是 `present − due`，而 `due` 由那只音频钟算出来。钟自称走了 7.967 s、视频就按 7.967 s 排，两者之差永远是 125 µs —— **而真实听起来差 427 ms**（`p13r-4k60-r3`：`clock_lead = 426.7 ms`）。所以"`lateness` 等于音画偏差"这句写在多处注释里的话，只在"钟说的是真的"时成立 | 读 `av_drift` 前先看 `pace.clock_lead_ns`；非 0 时分位数**不能**当音画偏差读 |
| **变异写在了容差里** | P14-T 的 V3 想把"墙钟被截短"造出来，截了 10 ms —— 而一帧是 16.7 ms，判据的容差就是一帧，于是那条变异永远绿。**变异"通过"了，判据却没被测到** | 造变异先问"这一改动的是不是判据线的另一侧"，量要**跨过**容差，不是挨着它 |
| **测试测错了对象** | P14-T 的 t1 想测"钟快 5% 时领先量是多少"，但没关漂移门 —— 门把领先量夹在 45 ms，量的其实是门限。默认策略里 `drift_gate_enabled = true`，写测试时很容易忘 | 单测里凡涉及"钟快/钟慢"，先明确门开不开；本卡的 `run()` 默认门关，t5 专门测门 |
| **第二台设备装不上** | `CM311-1A` 的 `ro.product.cpu.abilist` 只有 `armeabi-v7a,armeabi`，而 `abiFilters` 只编 `arm64-v8a` → `INSTALL_FAILED_NO_MATCHING_ABIS` | 要覆盖它得加 32 位 ABI **和** 32 位 FFmpeg，不是改一行 gradle |

---

## 六、架构上不许破的线

1. **`core/` 下不许出现任何 `<android/*>` / `<aaudio/*>` / `<GLES*>` 头文件。**
   时钟、节拍、队列都必须走接口注入，与桌面端共用同一份实现。
   "Android 版另写一套队列/时钟/同步" = 架构失败，回退重做。
   现成证据：`core/audio/spsc_audio_ring_buffer` 在 Windows MSVC 与 arm64 上分别编译、
   各跑通 433 项断言 —— 同一份源，零平台分支。
   同样的证据：`core/audio/clock/`（外推 + 单调守卫）在 MSVC 与 NDK 上分别编译，
   同一份源跑通 66 项断言，Android 侧只留"读时间戳"那一层（`backends/aaudio/clock/`）。
2. **私有头文件放 `backend/include_private/video/...`，与 `backend/src/video/...` 镜像**，
   不许和 .cpp 并排堆在 src 下。
3. **单文件单职责，> 400 行即需拆分评审。** 本轮已经因违反此条，把原来的
   `mediacodec_inventory_record.cxx`（4 职责）与 `capability_probe.cxx`（5 职责）拆成 13 个单元
   （这两个旧文件名现已不存在，别去找）。
4. **视频呈现路径零 CPU 读回。** 音频 PCM 的拷贝是物理必需，不受此条约束。
5. **失败必须有非空 reason，且用户可见。** 不许静默降级（G9）。

---

## 七、接手自检单（5 分钟确认环境没坏）

做完这几项再动代码，否则会把环境问题误判成代码问题：

**先过环境三关**（都是"git 里没有、本机才有"的东西，实测过的失败顺序就是这个顺序）：

- [ ] `android/local.properties` 存在，且 `sdk.dir` 指向本机 SDK（仓库不带此文件）
- [ ] `JAVA_HOME` 显式指定 —— **本机 PATH 里没有 `java`，`JAVA_HOME` 也是空的**，
      不带就报 `ERROR: JAVA_HOME is not set and no 'java' command could be found in your PATH.`
- [ ] `adb devices -l` 能看到 `b6e950ee`

**再跑功能自检**：

- [ ] `cd android && JAVA_HOME=/c/Android/jdk-17.0.13+11 ./gradlew.bat assembleDebug` 能出 APK（首次约 3–6 分钟）
- [ ] 装到机器上，`am start` 后画面能出（说明 P12-B 基线没退化）
- [ ] `run-as com.kivo.cinema ls files/evidence/` 能看到上次的 JSON
- [ ] `kivo_p12c_ring_buffer_tests` 能编能跑（已存在，可当作桌面端 CMake 是否正常的探针）
- [ ] `kivo_p12c4_clock_tests` 13 例全绿（`ctest -R kivo_p12c` 应为 **5/5**，
      含 `kivo_p12c6_pipeline_tests` 9 例 / 42 断言）
- [ ] 若本轮要动 `core/`：桌面端 CMake + CTest 能跑通

任何一项失败 → 先修环境，别往下写代码。

**另外：`ctest` 的绿必须来自"刚重建过"的树。** 2026-10-31 复核时发现，
`build/ninja-debug` 里 16 个测试有 13 个还是前一天（10-30 23:17）的产物，
其中一个（`kivo_p13p_episode_tests`）从 P14-U 起就已经红了 —— 旧二进制把它盖住了，
"16/16" 因此连着两轮被写成绿。判是否陈旧的一条命令：

```bash
find build/ninja-debug -name "kivo_p1*.exe" -newer backend/src/video/video_plane/core/pace/pace_stats.cxx
```

列不全就先把树重建，再谈绿不绿。

---

## 八、交接时容易丢的东西

| 容易丢 | 现在记在哪 |
| --- | --- |
| 为什么不用 Vulkan | `07-p12-plan.md` §2.4 |
| 为什么 minSdk 是 26 不是 24/28 | `06-android-toolchain.md` §五 |
| 为什么 HEVC 走 OMX 而不是 c2.qti | `11-p12b-status.md` + 本文 §五 |
| 选型四判据（官方支持/生态规模/头部采用/可替换性） | `09-tech-radar.md` |
| 家族树每一格该放什么 | `08-p12-family-tree.md` |
| 任务台账 | `docs/handoff/TASKS.md` |

---

## UI 复刻卡点交接（2026-11-08，237/238）

- 首页复刻被用户判定不合格：根因是连续补丁污染 `ui/design/Pages.kt`。
- **下一步必须**：删掉现 DiscoverPage/HeroCarousel，按 `docs/design/backend-core/238-home-spec.md`
  （566 键全量）+ `237-ui-migration-plan.md` §8 勘误表**一次性重写**，禁止在旧结构上 patch。
- 已验证机制（勿推翻）：poster 2:3 铺满+钉死 585dp、标题在素材不渲染、环境色取图底边缘、
  washFill 渐变、只有首屏跟随海报底色、TMDB key 在设备 filesDir/tmdb_key.txt、
  代理 tmdb_proxy.txt（adb reverse 10808）、错误态五类文案。
- 闭环纪律：拆解参数 → 实现 → 边界测量 + Rex/kivo 并排图入库 → 不一致修到一致。
