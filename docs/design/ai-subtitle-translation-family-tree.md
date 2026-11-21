# AI 字幕翻译 · 实现层家族树与设计（v3.2，完整合并版）

> v1→v2→v3→v3.1→**v3.2**（2026-11-21，第四轮评审 ×3 份 + 自查 N6-N10）
> 上游依据：`staging/README.md` + `subtitle-translation-landing.md`。产品层配套：`ai-subtitle-translation-ux-spec.md`（v3）。
> **v3.2 是完整合并版**（评审 F1：v3.1 只写增量导致内容丢失与引用悬空）——本文件含全部现行决策，无需对照旧版。
> 行数预算定位声明（评审 E9）：**预算 = 预估值而非硬指标**；验收看职责单一/依赖方向/状态唯一管理者/
> 契约完整/可单测，超限即拆职责，不为达标缩写——与 300 行铁律（禁堆砌）精神一致。

---

## 0. v3.1→v3.2 判定表（合并三份评审 + 自查）

### 最重大判定：DEC-07 撤销重写

评审 E1/G1（撤销跨 cue 时间轴合并：合并组取首尾时间会提前显示/延迟消失/遮挡插话/混淆说话人）、
F7（确认「折行 cue」含义）共同指向一个真相：**v3.1 的 N3 把 A4a 的「cue 内折行」误解成了跨 cue 合并**。
A4a 原文「普通折行 cue 合并成一句」指**单个 cue 内部的换行**（单 cue 内合并根本不涉及时间轴）。
**裁定（DEC-07 v2）：撤销跨 cue 时间轴合并；v1 只做 cue 内折行合并（时间轴天然不变）+ 上下文组**
（跨 cue 组只作为 prompt 上下文，按 cue 回填、时间轴不变）；跨 cue 合句 = 二期。加**时间轴不变性单测**。
N3 自查随之失效（前提不存在）。

### 其余判定（E=深度审查 / F=技术+UX / G=产品 UX）

| # | 条目 | 判定 | 落点 |
|---|---|---|---|
| E2/G2 | 模型指纹：一次性 SHA 不够（外部目录 adb 可改写、同 name+大小可不同权重） | **采纳**——内容寻址文件名 + 临时文件→全量 SHA256→**与可信发布清单比对**（清单 SHA 来自发布方，不是本地算完自认）→ 原子发布 → 加载期禁替换；缓存键用可信清单摘要 | §2 ModelStore |
| E3/G3 | `:ai` 进程只有方向没有 IPC 契约；且普通独立进程 ≠ isolatedProcess（勿引入 UID 沙箱） | **采纳**——**主进程持任务/journal/SRT 发布/UI，AI 进程只管模型加载+推理+结构化结果**；Binder 断开重连、幂等、超时进契约；v1 进程内先实现 `AiEngineClient` 接口（IPC-ready，见 F16） | §2 / DEC-13 |
| E4/G4 | 字符数不能代替 Token 数；prompt 含 system+fewshot+上下文+输出预留 | **采纳**——双层保护：planner 字符粗估 + **Engine 用真实 tokenizer 编完整 prompt 查 n_ctx**（T_prefix+T_context+T_input+T_output_reserve ≤ n_ctx），超限返回可拆分结果交 planner 重切——不可绕过的最后防线 | §2 引擎 |
| E5/G5 | 混合语言三层规则缺失；CJK ≠ 中文语言检测 | **采纳**——文件级/cue 级/文本级三层；英文 cue 翻译、纯中文 cue 保留、cue 内混合明确提示 v1 不保证；日文假名检测区分（假名=非中文）；语言检测按 source_hash 缓存、归属 cue_assembler、高频词命中率方法 + fixtures | §2 |
| E6/G6 | Cue/Unit 提交单位未统一（对话 cue 拆 Unit 后半中半英） | **采纳**——**Unit=模型执行单位，Cue=用户可见与提交单位**：Cue 汇总全部关联 Unit 结果，Cue 内任一 Unit 失败 → 整 Cue 回退原文（避免半中半英）；上下文组不改时间轴 | §2 |
| E7/G7 | journal 只有行结果，缺事件模型；AtomicFile 不负责锁 | **采纳**——事件型记录（JOB_CREATED/BLOCK_STARTED/BLOCK_COMMITTED/PAUSE_REQUESTED/SYSTEM_PAUSED/RESUME_REQUESTED/JOB_STOPPED/JOB_COMPLETED/JOB_FAILED/SOURCE_INVALIDATED）+ 单调序号 + 校验值 + 幂等 + schema_version + 损坏定位到最后有效检查点；单写入者约束明示 | §2 Job |
| E8/G8 | 首块 8 行与固定网格边界不清 | **采纳**——**Preview Slice**：网格块不变，落点所在网格块拆三段 [播放头,+8) → 余下 → 前方回填；Preview 与完整块完成状态分开记录；+自查 N7 修正：首块 = **包含当前播放头的网格块**（v3.1「网格块前 8 行」在续播场景失效） | §2 planner |
| E9/F9 | mpv 重载串行化+刷新合并；重载副作用大 | **采纳**——发布协调器：同一时刻单发布流程、文件递增版本号、刷新期间新结果标待发布、完成后合并发布、失败保留旧版可重试；**发布版本号与 journal 序号对账**（自查 N7） | §2 Controller |
| E10/G9 | 轨道 ID 双身份；自动选轨区分「用户操作/程序设置/mpv 内部」；自动挂载尊重用户偏好 | **采纳**——会话内数字 ID（每次 reload 后重查 track-list）vs 业务身份（文件 hash+类型，跨会话匹配）；用户偏好由**交互事件**写入（非监听 sid 推断）；自动挂载记住**片源级**最后用户选择，默认只加轨不抢选中（除非该片从未手选） | §2 TrackManager |
| E11/G11 | 缓存自动挂载 vs 手动选择冲突 | **采纳**——优先级：片源级最后用户选择 > 自动挂载偏好 > 默认策略；「关闭 AI 中文字幕」= 片源级记住该偏好直到用户重开 | UX v3 |
| E12/G12 | 两文档状态体系不统一；缺四层状态模型；总开关/单片开关/翻译暂停三个操作混用 | **采纳**——四层状态模型（功能层/任务层/环境层/展示层）+ 统一状态解析器；**完整十态枚举表**（状态×进入条件×入口投影×TalkBack×操作）两文档共同引用 | UX v3 §3 |
| E13/G13 | 结果状态不够分；连续就绪区间口径 | **采纳**——五结果状态（LLM_TRANSLATED/RULE_TRANSLATED/INTENTIONAL_KEEP/FAILED_FALLBACK/PENDING）；**主指标口径修正 = 连续已处理终点**（评审 F3：失败回退算已处理，只有 PENDING 打断——否则一条失败行永久卡死指标）；G13「失败中断」与 F3 冲突裁定：**指标口径连续已处理、统计口径未译计数分离**，INTENTIONAL_KEEP 连续；连续已译的「译」字弃用 | §2 / DEC-11 v2 |
| E14/G14 | 「本地不联网」与模型下载矛盾；10 tok/s 绝对门槛降为参考；30-40 分钟承诺矛盾（10÷8=1.25×→48 分钟） | **采纳**——文案改「AI 翻译在设备本地运行，字幕内容不会上传。模型安装和更新可能需要网络」；go/no-go 改**真实字幕工作负载基准**（指定档位+字幕集+完成率+落后时长频率+倍速），10 tok/s 降为参考；**耗时承诺由领先窗口策略消解**（F15）：首块约 1 分钟、之后跟随播放，不再承诺全片耗时 | UX v3 |
| E15/G15 | 存储估算粗糙；源字幕 hash 不进用户诊断报告 | **采纳**——动态估算公式（下载暂存+输出预留+journal+安全余量）；诊断区分内部缓存 hash/匿名 Job ID/脱敏标识 | §2 / UX |
| E16/G17 | 无障碍只写 AA 不够（动态背景、TalkBack 频率、CaptioningManager） | **采纳**——AA 靠描边/底板达成（叠视频无静态对比度）；TalkBack 只播操作结果/完成/出错（进度合并通知）；CaptioningManager 系统字幕偏好是否尊重需明确 | UX v3 §7 |
| E18/G18 | T-0 单一 decode 测试不代表真实负载 | **采纳**——**两种独立测试**：引擎压力测试（连续推理/内存/温度）vs **真实字幕工作负载测试**（从输入到可播放中文，逐 cue 完整处理时间与成功率）——后者才是发布前性能评估主依据 | §4 |
| E19/G19 | 200 条三法则上界仍 1.5%；评分程序缺失 | **采纳**——50 行日常快检 / 200 条首轮正式 / 按发布目标扩大；**双人独立评分 + 分歧仲裁**；固定发布测试集 + 真实影片自然分布集 | §4 |
| E20/G20 | 播放保护缺倍速/热稳态组合 | **采纳**——8 场景矩阵（1.0/1.5/2.0×、长时间、高码率 HEVC、频繁 seek、低电量、热稳态×倍速） | §4 |
| E21/G21 | 单测齐但缺集成测试 | **采纳**——三组：Cue 全链路（SRT→归一→映射→prompt→解析→回填→SRT）/ 调度与恢复全链路（播放头→重规划→部分翻译→进程终止→journal 恢复→不重复提交）/ 播放器联动（挂载→首批→切轨→增量重载→重识别→恢复选择） | §4 |
| E22/G22 | 一个开关 ≠ 砍掉次级操作 | **采纳**——一个主操作 + 必要次级控制（详情 sheet：暂停/继续/停止/失败行重试/来源查看） | UX v3 |
| E23/G23 | 来源选择交互未落地 | **采纳**——「选择翻译来源」页：多份英文 SRT 列表、同名区分（hash 后缀）、开始后高级信息保留来源名；**来源选择与轨道显示是两个操作** | UX v3 |
| E24/G24 | 已完成/部分完成/缓存命中场景表缺失 | **采纳**——七场景表进 UX v3 | UX v3 |
| E25/G25 | 落后态三方案（A 原文回退/B 不显示/C 提示追赶） | **采纳**——**倾向 A + C 的低干扰组合**：原文回退保信息完整，只在状态发生重要变化时经面板状态行说明，不给每条未译 cue 弹提示；中英跳变的感知强度由真实影片设计稿判定（DEC-09） | UX v3 |
| E26/G26 | CPS 阅读速度缺验收 | **采纳**——CPS = 显示字符数/显示秒数，进 cue_assembler 质量检查（口径：中文字符/标点/空格/专名一致）；超长译文精简重试；阈值 T-C 实测定 | §2 |
| E27/G27 | 设置页放开发质量数据降低高级感 | **采纳**——常用区只放诚实说明；质量指标进高级诊断模式 | UX v3 |
| E28/G28 | KV 池数值 T-0 定；LRU 不逐在用序列；复用重置采样器状态 | **采纳**——v3.2 修正：**静态 3 序列（块/单行/重试前缀形态各一），无需驱逐**（评审 F17：池里只有几种前缀形态，LRU 过度设计）；每前缀占 prefix_len×每 token KV 计入 n_ctx 预算；复用重置采样器/stop/abort/缓冲 | §2 |
| E29 | 重试不变量落 prompt hash（第一次已含上下文则 hash 不变） | **采纳**——每次推理前算完整 prompt hash，重试验证 hash 确变，否则直接换策略/逐行 | §2 |
| E30 | 编号输出协议防污染（正文数字误认编号/Markdown/多行译文） | **采纳**——严格输出协议：Unit 稳定 ID、固定格式、正文禁解释、解析器不把正文数字/时间戳当新编号、协议违反=结构错误；金标加畸形输出用例 | §2 |
| E31 | SRT 更多盲点+输出端统一 | **采纳**——无序号/序号重复乱序/非标时间戳/零时长/开始>结束/控制字符/正文含时间戳样串；输出端统一时间戳精度/UTF-8/序号重排/换行/转义 | §2 fixtures |
| E32/G32 | 温控三档位（正常/降级/暂停）；充电/低电量阈值；暂停原因分别维护 | **采纳**——三档位规则表；阈值 T-0 产出进 decision log（F20：单机常量不写死，**首次使用 5-10 秒自测** decode 20 token+cpuset 选策略）；暂停原因集合分别维护非共享 Boolean | §2 Lifecycle |
| E33/G33 | 模型下载状态机+失败恢复 | **采纳**——未安装→下载中→校验中→安装中→已就绪 + 暂停/失败/取消态；完整校验前不进可用目录；临时文件与正式文件同文件系统 + 安装互斥 | UX v3 §8 |
| F1/G 的丢失清单 | UX v2 重写丢 v1 内容且引用悬空；技术 v3.1 只写增量 | **采纳**——v3.2 两份均为**完整合并版**；修订摘要含删除标注 | 全文 |
| F2 | 入口 8 态缺可用·待开启（happy path）/已就绪/部分完成/已停止/失败/熔断禁用/校验中；优先级反了；存储 1.2GB 放错阶段；总开关灰显 vs 隐藏 | **采纳**——入口态重做（见 UX v3 §3，优先级：功能关闭 > 任务态 > 字幕条件 > 模型条件 > 可用）；存储前置条件移入北极星安装旅程；总开关=灰显（保可发现） | UX v3 §3 |
| F3 | 「连续已译」被失败行卡死 | **采纳**——改「连续已处理终点」（见 E13 行） | DEC-11 v2 |
| F4 | journal 回放重建不了用户意图态 | **采纳**——**分工：进度 ← journal（事件模型）；意图 ← manifest（tmp+rename 原子写）**；DEC-08 措辞限定「进度真源」；恢复规则：manifest 说翻译中但进程已死 → 「已中断·可继续」，**不悄悄自动续译** | §2 |
| F5 | 前块上下文：N1 只修缓存没修模型行为（1.7B 可能翻上文破坏对齐；few-shot 无上文样例） | **采纳**——上文不编号、固定标签包裹（「上文（仅供理解，不要翻译）」）+ few-shot 示例同步带上文块（前缀仍恒定）+ **T-A 消融实验**（有/无上文比 BLEU/chrF/错号率/误翻率，收益不明显就不上） | §2 / DEC-14 |
| F6 | 固定网格+首块 8 行 seek 后落空；跨路径可复现承诺放弃 | **采纳**——Preview Slice（见 E8 行）；「边界不随 seek 漂移」改「仅落点块局部漂移」；journal 是实际产出真源，不做跨路径可复现承诺 | §2 |
| F7 | DEC-07 把合句拉回 v1 与 §6 二期矛盾；「折行 cue」含义需确认 | **裁定（本表头条）**：A4a 原意 = cue 内折行，v1 只做 cue 内合并，跨 cue 合句 = 二期；DEC-07 重写 | §1 |
| F8 | 双语逃生口无技术落点；英文检测方法缺失 | **采纳**——逃生口**降二期**（行级语言分类+输出规则工作量大、可逆性高）；英文检测 = 高频词命中率（拉丁语系字符集分不出）+ fixtures；CJK 占比阈值列未决项带默认（假名≠中文）；检测按 source_hash 缓存 | UX v3 |
| F9/G27 | 入口行过载（信任锚点 vs 进度副行抢槽位；「正在准备」在中文已上屏后没好） | **采纳**——**入口 = 一行开关（中文字幕：开/关）+ 详情 sheet 收纳次级操作**；信任提示放轨道名（「AI 中文（机器翻译）」）+ 首次说明 + 完成卡且可操作；开发期质量数据进诊断页 | UX v3 §3 |
| F10/G9 | 「关闭」三层级 | **采纳**——词汇表：暂停=临时停；关闭=移除当前轨、保留进度；删除=清该片进度与缓存；设置总开关叫**「启用 AI 字幕翻译」** | UX v3 §2 |
| F11/G14 | 首次说明写死 30-40 分钟（冷测口径） | **采纳**——改「耗时与片长相当，视设备温度而定」；领先窗口策略下首块约 1 分钟，之后跟随播放 | UX v3 |
| F12 | 轨道契约缺自动选中前提/持久范围/挂载 vs 选中；自动挂载依赖 Kivo 是否记得外挂 SRT | **采纳 + 已核实**——SubtitleImportStore 按片源登记存在（注释确认）→ 依赖成立；自动挂载 = 只加轨；选中取决于该片上次用户选择（从未手选则选中 AI 轨）；手动选择按片持久 | §2 TrackManager |
| F13 | 任务互斥 vs 换片即暂停打架 | **裁定 + 已核实**——**PiP 存在**（pipActive 引用确认）→ 「页面离开任务仍活跃」场景成立：互斥态保留但场景写明（PiP 播放中/受控退出窗口期）；换片 = 自动暂停旧任务、检查点保留 | UX v3 |
| F14 | 无障碍补定义（看片时语音抢占/AA 靠描边/CaptioningManager） | **采纳**（见 E16 行） | UX v3 §7 |
| F15 | **领先窗口默认策略**（设计建议）：保持领先播放头 8-10 分钟，超过歇着降温；占空比降→发热耗电降→门槛放宽到 ≥1.0×；用户中途不看不浪费算力 | **采纳为默认**——「提前翻译整片」作为可选（充电时或用户主动）；T-0 顺手比较两种策略温度曲线；**自查 N6：完成态口径改「已覆盖全部已播放区域」**——领先窗口下整片永远不「完成」，完成卡按已播放覆盖统计，全片完成 = 播放头到片尾且无 PENDING | §3 / UX |
| F16 | DEC-04 低可逆 → 现在就把接口画成 IPC 形状 | **采纳**——`AiEngineClient` 接口：全异步、请求/响应可序列化、cancel 独立消息、不暴露 native 句柄；v1 进程内实现，发布态换 Messenger——低可逆变中等 | §2 / DEC-13 |
| F17 | 重试需要新 prompt 形态；序列池 LRU 过度设计 | **采纳**——重试 = 上下文注入含后 2 条原文（前 2 已有），全在 user 段不破坏前缀，无需新 prompt 形态；序列池改**静态 3 序列无驱逐**（见 E28 行） | §2 |
| F18 | 状态机纯 reducer 上轮建议无落点；Controller 三职责 | **采纳**——`EntryStateResolver`（纯函数，入口态表驱动单测）+ `SubtitleTrackManager`（mpv 接口 fake 化，N2/重载回归在 fake 上跑）+ Controller 门面；状态机 reducer 可 JVM 单测；**不采纳之处如实记录**：无 | §2 |
| F19 | 金标固化后与真源脱钩 | **采纳**——fixture 头记真源脚本 hash；`verify_generated.py` 字节比对进提交前检查 | §2 |
| F20 | 阈值无数值；单机常量不写死 | **采纳**——热控/低电量具体级别由 T-0 实测产出进 decision log；**首用 5-10 秒自测**（decode 20 token + cpuset）选策略 | §2 |
| F21 | 引用错编号/计数对不上/DEC-04 状态措辞/「未完成」歧义 | **采纳**——全部修正（本版引用以本轮判定表编号为准；测试 target 计数实列；DEC-04 状态「建议冻结，待确认」；「未译（保留原文）」） | 全文 |
| F22 | v1 范围划切线 | **采纳**——必做/可砍二期清单见 §5 | §5 |
| E 尾/F 尾 | 片源形态调查 | **待用户回答**（唯一悬而未决的范围决策） | §6 |

### 自查第四轮（N6-N10，三份评审未覆盖）

| # | 问题 | 处置 |
|---|---|---|
| N6 | **领先窗口（F15）与「已完成」态定义冲突**：跟随播放模式下整片永不完成，完成卡/已完成态口径悬空 | 完成态改「已覆盖全部已播放区域」；全片完成 = 播放头到片尾且无 PENDING；「提前翻译整片」选项触发传统完成口径 |
| N7 | journal 追加与 SRT 发布（rename+reload）的版本对账缺失 | 发布协调器：发布版本号 ↔ journal 序号对账，reload 只读 ≥ 当前展示所需版本（并入 E9） |
| N8 | 查表跳过的执行位置未定（planner/scheduler/alignment 各有嫌疑） | 定死：**alignment 之后、scheduler 之前**——RULE_TRANSLATED 由 scheduler 直写 journal 不进模型；CPS 检查在 alignment（E26） |
| N9 | AI 开关对照测试的公平口径（领先窗口下 CPU 占空比不平稳，瞬时对比失真） | 对照口径 = **整段总掉帧数/总卡顿时长对比**而非瞬时值；同片同参数同热状态起点 |
| N10 | 词汇表（F10）与四层状态模型（E12）要合成一张总表 | UX v3 §3 产出「状态 × 进入条件 × 入口投影 × TalkBack × 可用操作」总表，作为唯一状态引用源 |

---

## 1. 决策 log（唯一权威源；两文档只引用不复述）

| # | 决策 | 状态 |
|---|---|---|
| DEC-01 | 分层：llama 封装 android cpp；调度/规划/校验 backend 纯逻辑 | 定稿 |
| DEC-02 | 模型分发：v1 开发者模式；应用内安装 = 发布门槛（北极星旅程已立） | 定稿 |
| DEC-03 | few-shot 块契约要做（真源+SHA+金标 fixture 真源 hash 头）；重试上下文全进 user 段无需新形态 | 定稿 |
| **DEC-07 v2** | **撤销跨 cue 时间轴合并；v1 只做 cue 内折行合并（时间轴不变）+ 上下文组按 cue 回填；跨 cue 合句 = 二期** | 定稿 |
| DEC-05 | 失败行纯原文零标记（维持） | 定稿 |
| DEC-06 | 上下文只进 user 段（N1） | 定稿 |
| **DEC-08 v2** | journal = **进度**真源（事件模型）；manifest = **意图**真源（独立原子写）；恢复不自动续译 | 定稿 |
| **DEC-11 v2** | 主指标 = **自播放头起连续已处理终点**（LLM_TRANSLATED/RULE_TRANSLATED/INTENTIONAL_KEEP/FAILED_FALLBACK 连续；仅 PENDING 打断）；未译计数分离 | 定稿 |
| DEC-13 | `AiEngineClient` 接口 IPC-ready（v1 进程内、发布态 Messenger）；`:ai` 进程 = 发布门槛 | **建议冻结，待确认** |
| DEC-14 | 前块上下文消融实验（T-A）：收益不明显就不上 | 待 T-A 实测 |
| **DEC-15** | **翻译节奏默认 = 领先窗口**（领先播放头 8-10 分钟，超过降温）；「提前翻译整片」为可选 | **待确认** |
| DEC-16 | 模型清单 SHA 来自可信发布方（非本地自认）；内容寻址发布 | 定稿 |
| DEC-17 | CPS 阅读速度进 cue_assembler 质量检查；阈值 T-C 定 | 定稿（阈值待实测） |
| DEC-18 | 温控三档位 + 阈值由 T-0 实测进 decision log + 首用 5-10 秒自测 | 定稿（数值待 T-0） |

---

## 2. 文件家族树（v3.2 完整版）

```
backend/src/subtitle_translation_core/
├── subtitle_translation_cue_assembler.cpp/.h      SRT 解析归一（含畸形容错全清单 E31）+ cue 内折行
│                                                  合并 + 对话 cue 拆 Unit + cue 级标签回填/词级丢弃 +
│                                                  Unit↔cue 双向映射 + 语言检测三层（E5）+ CPS 检查
│                                                  （DEC-17）。预算 ≤260 行。
├── subtitle_translation_cue_assembler_test.cpp    畸形 SRT 全清单 + 双语/混合语言 fixtures（escape 口
│                                                  降二期，fixtures 仍留拒收样例）。预算 ≤240 行。
├── subtitle_translation_block_planner.cpp         固定网格切块（多维约束）+ Preview Slice（E8/N7）+
│                                                  BlockPlanner 纯函数（块列表+已完成集合+播放头→下一块，
│                                                  领先窗口策略参数化 DEC-15）。预算 ≤240 行。
├── subtitle_translation_block_planner_test.cpp    网格/Preview/重排/续译/领先窗口切换。≤220 行。
├── subtitle_translation_alignment.cpp             按号对齐 + 三层校验（结构/语言/质量预警）+ 输出协议
│                                                  解析（E30：正文数字不当编号）。≤200 行。
├── subtitle_translation_alignment_test.cpp        ≤180 行（含畸形输出协议用例）。
├── subtitle_translation_scheduler.cpp             重试状态机（prompt-hash 不变量 E29）+ 静态 3 序列注记
│                                                  + 查表跳过定序（N8：alignment 后直写 RULE_TRANSLATED）
│                                                  + BlockStats + JSON lines。≤280 行。
├── subtitle_translation_scheduler_test.cpp        ≤240 行。
├── fixtures/prompt_golden.json                    金标固化 + fixture 头记真源脚本 hash（F19）。
├── fixtures/srt_corpus/                           畸形 SRT 全清单语料（E31）。

android/app/src/main/cpp/
├── ai_subtitle_engine.h/.cpp                      llama 封装：mmap、前缀 KV（静态 3 序列、复用重置
│                                                  E28）、**双层 Token 预算**（E4：tokenizer 精确检查
│                                                  n_ctx，超限返回可拆分）、温度 0、流式行级早停
│                                                  （×3 且 ≥12 token 下限、n-gram 白名单、早停结果不
│                                                  直接使用）、abort_callback、n_threads=T-0 定。
│                                                  预算 ≤320 行（超限拆 stats）。
├── AiTranslationNative.cpp                        JNI → libkivo_ai.so。

android/app/src/main/java/com/kivo/cinema/subtitle/
├── AiEngineClient.kt                              **IPC-ready 接口**（F16）：全异步、可序列化请求/
│                                                  响应、cancel 独立消息、不暴露 native 句柄。
│                                                  v1 = 进程内实现（LocalAiEngineClient）；发布态换
│                                                  Messenger 实现。≤120 行。
├── AiTranslationNative.kt                         external fun 七件套 + nativeBlockStats。≤90 行。
├── AiSubtitleModelStore.kt                        内容寻址发布（E2）+ 可信清单 + 校验状态机。≤200 行。
├── AiSubtitleEngineLifecycle.kt                   三档位（E32）+ 受控退出 + onTrimMemory 按 API 级别
│                                                  + 首用自测（F20）。≤180 行。
├── TranslationJob.kt                              驱动循环 + journal 事件模型（E7 十类事件）+
│                                                  manifest 意图真源（F4）+ 输入快照 + Preview Slice
│                                                  驱动。≤240 行。
├── TranslationCache.kt                            缓存键（可信 SHA+契约指纹+解析版本+源 hash+语言）
│                                                  + 五结果状态计数（G13）+ 占用/清空/活动保护。≤200 行。
├── SubtitleTrackManager.kt                        新增（F18）：mpv 轨道接口 fake 化——add 显式选中/
│                                                  reload 后重读 track-list/业务身份匹配/恢复「用户最后
│                                                  手动选择」/发布协调器（单发布流程+版本对账 N7）。
│                                                  回归用例在 fake 上跑。≤200 行。
├── EntryStateResolver.kt                          新增（F18）：纯函数（任务态/字幕条件/模型条件 →
│                                                  入口态），表驱动单测。≤120 行。
├── AiSubtitleController.kt                        门面（决策全部下沉后收窄）：委托 + 状态暴露 +
│                                                  Handler 主线程单入口写状态（线程契约）。≤150 行。
├── TranslationStateMachine.kt                     新增（F18）：纯 reducer（state,event)→(state,effects)，
│                                                  四层状态模型（E12/G12），JVM 可单测。≤160 行。

ui/（入口/设置/详情 sheet——见 UX v3 §3/§6/§7）

backend CMakeLists + android cpp CMakeLists：注册全部源 + libkivo_ai.so + llama IMPORTED。
```

单测 target 计数修正（F21）：新增 **5 个**（cue_assembler/block_planner/alignment/scheduler 四个
+ prompt 金标比对），既有 2 个翻译 target 保留，全量 ctest 目标 = 296 + 5 桌面新 target 及其用例，
最终数以实测为准（不再拍脑袋写 306）。

---

## 3. 数据流（v3.2：领先窗口 + 发布协调）

```
开启（入口可用态）→ Controller → TranslationJob（输入快照锁定）
  → BlockPlanner：固定网格切块 → Preview Slice（[播放头,+8)）→ 领先窗口调度
  → 循环（独立线程，档位由 T-0/首用自测定）：
      Scheduler.translateBlock(selected)        // prompt-hash 不变量
        └─ AiEngineClient → LlamaSubtitleEngine // 前缀 KV 命中 + 双层 Token 预算 + 行级早停
      AlignmentValidator 按号对齐 + 三层校验 → 查表跳过（RULE_TRANSLATED）
      → journal 事件追加（BLOCK_COMMITTED，fsync）
      → 发布协调器（单流程+版本对账）：journal 派生 SRT → 临时文件+rename → SubtitleTrackManager
        （reload → 重读 track-list → 恢复「用户最后手动选择」）
      → 领先窗口达 8-10 分钟 → 引擎歇着降温（DEC-15）
  → 播放头到片尾且无 PENDING → 完成（已覆盖全部已播放区域，N6）
```

四层状态模型（E12）：功能层（启用/禁用）· 任务层（十态）· 环境层（缺模型/存储/热控/低电量/内存）·
展示层（AI 轨挂载/未挂载/就绪/落后）——统一状态解析器生成 UI 态（TranslationStateMachine reducer）。

---

## 4. 验收矩阵（v3.2）

| 层 | 验收 |
|---|---|
| **T-0（前置）** | ①引擎压力：n_threads×优先级×cpuset×tok/s×温度×内存 + **15 分钟下限、温度/频率平台期结束**（评审 E17：15 分钟可能测不到热稳态）；②真实字幕工作负载：60s 片源全链路逐 cue 处理时间+成功率；③sub-reload 行为五问 + **发布协调器串行验证**；④token 金标；⑤SIGILL 排除；⑥首用自测脚本验证 |
| 桌面 | 5 个新单测 target + 金标 fixture 比对 + **时间轴不变性单测**（DEC-07 v2）+ 重试不变量 + 三组集成测试（E21） |
| arm64 | assembleDebug 过（libkivo_ai.so）+ 哨兵注入 + **AiEngineClient 接口形状验证**（异步/可序列化/cancel 独立） |
| 真机·端到端 | 60s 片源：Preview Slice 首批耗时 → sub-reload → 状态行 → 轨道恢复用户选择（N2） |
| 真机·长跑 | 1000 行全程：BlockStats 拆 prefill/decode/retry + 五结果状态分布 + 连续已处理终点曲线 |
| 真机·韧性 | 杀进程 journal 回放 + manifest 意图保持（手动暂停不被悄悄续译，F4）+ 输入中途替换 + 畸形 SRT 全清单 + **磁盘写满 → 系统暂停保 journal（评审 F 类，v3.1 缺失）** |
| 真机·质量 | 两阶段抽检（50 快检 / 200 首轮）+ 双人独立评分 + 仲裁 + 固定集 + 自然分布集 + 评分细则 |
| 真机·播放保护 | 8 场景矩阵 + AI 开关对照（**整段总掉帧/卡顿对比口径，N9**）+ 容差（均值 <0.5%、P95 <1%、seek <10%） |
| 真机·兜底 | 断网全流程；失败行纯原文；secondary-sid 双语（设计稿后验） |

---

## 5. v1 范围切线（F22）

**必做**：planner/alignment/scheduler 基础链路 · cue_assembler（含语言检测）· journal 事件模型 +
manifest · 原子发布 + 发布协调器 · 前缀 KV + 双层 Token 预算 · 早停 · AiEngineClient（IPC-ready 接口）
· 入口态（含可用/就绪/部分完成/失败/落后）· 落后态文案 · 时间轴不变性。
**可砍二期**：跨 cue 合句 · 双语逃生口 · 序列池 LRU（静态 3 序列已够）· 自动挂载（先手点挂载）·
提前翻译整片选项 · 追剧默认开启 · 导出 SRT · 术语表 · 译文编辑。

## 6. 不做清单（内嵌轨优先级**待用户回答片源形态**）

## 7. 发布门槛（不变 + C13/E19/G19 补强：双人评分/仲裁/自然分布集/数值冻结）

## 8. 发布前合规清单（增：崩溃转储脱敏——tombstone 内存含 prompt，上报前脱敏或不上传；隐私文案
「AI 翻译在设备本地运行，字幕内容不会上传。模型安装和更新可能需要网络」G15）

## 9. 待用户拍板

| # | 事项 | 推荐默认 | 可逆性 |
|---|---|---|---|
| 1 | DEC-13 `:ai` 进程 = 发布门槛冻结（v1 不实现但接口 IPC-ready） | 建议冻结 | 低 |
| 2 | DEC-15 领先窗口默认策略 | 建议采纳 | 高 |
| 3 | **片源形态**（外挂 SRT vs 内嵌居多）→ 内嵌轨提取优先级 | 待回答 | — |
| 4 | T-0 + T-A 开工 | 建议开工 | 高 |