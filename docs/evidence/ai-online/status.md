# 在线 AI 翻译接口 · 真机验收证据（2026-11-25）

> 设计：`docs/design/ai-online-2026-11-25/design.md`；调研：`research.md`（含 SenPlayer 逆向）。
> 设备：Redmi K40 `b6e950ee`；包：`app-debug.apk`，`install -r` 覆盖安装。

## 一、交付物

**新文件族 `subtitle/remote/`（3 文件）**：
- `RemoteEnginePrefs.kt`：7 档服务商（SiliconFlow/DeepSeek/OpenAI/Gemini/OpenRouter/Anthropic/自定义）
  + base_url/模型预置表 + 配置读写 + configError（G9 中文校验）
- `RemoteEngineProtocols.kt`：请求/响应编解码纯函数——OpenAI 兼容 + Anthropic 原生双协议；
  prompt 为 SenPlayer 逆向全文还原 + 本地链 context 语义（逐条 id 回传 JSON、防注入、禁合并省略）；
  解析三级兜底（整体 JSON / markdown 包裹平衡块 / 失败交缺号重试）
- `RemoteAiEngineClient.kt`：AiEngineClient 实现——单线程 executor（线程契约同本地）、
  testConnection（GET /models）、错误映射中文、SOCKS 复用 tmdb_proxy.txt

**UI**：`ui/subtitles/online/AiEngineSettings.kt`（设置卡片：引擎开关/服务商/模型/密钥/选择/配置/测试连接/隐私明示）
**修改**：`AiSubtitleController`（engineClient 按配置选择本地/远程单例 + entryState/isEngineReady
远程语义）、`OnlineSubtitleTranslator`（engineClient 透传）、`SettingsPlaybackSub`（挂载）、
`build.gradle.kts`（testImplementation org.json 真实现——桩类在 JVM 全抛 not mocked）
**不动**：`TranslationJob`（切块/缺号重试/缓存/落盘零改动）、`AiEngineClient`（接口形状兑现 IPC 设计）、
播放核心（红线）

## 二、单测（13 项全绿，BUILD SUCCESSFUL）

`RemoteEngineProtocolsTest`：请求体组装（model/system 防注入句/带 id 行/上下文段）、
Anthropic system 字段与单 user 消息、OpenAI/Anthropic 响应解析、按 id 乱序映射、
缺号 → null 槽位（走重试路径）、markdown 代码块提取、纯文本报错、空回复报错、
无 translations 报错、空 translations 报错、prompt 注入样本保持在数据位。

## 三、真机 UI 验证（K40，2026-11-25 07:50）

| 项 | 证据 | 结果 |
| --- | --- | --- |
| 设置入口 | 设置 → 播放 → 滚动至「AI 翻译引擎」卡片 | ✓ |
| 卡片内容 | 引擎开关（副标题「关闭 = 本地模型…」）/ 服务商 / 模型 / 接口密钥 / 选择服务商 / 填写接口配置 / 测试连接 / 隐私 Footnote | ✓ `engine-settings.png` |
| 服务商选择 | 7 档单选列表全呈现 | ✓ `provider-picker.png` |
| 自定义档 | 标题「接口配置（自定义）」+「接口地址」输入框（仅自定义显示）+ Key + 模型名 | ✓ `custom-config-dialog.png` |
| 错误路径（未填 key 点测试连接） | **「✗ 请填写 API Key」**——本地校验即拦截，不发起网络请求，G9 | ✓ |
| 服务商切换 | 选「自定义」后配置标题与服务商 ValueRow 联动 | ✓ |

## 三b、本地引擎回归（3d44e89f 收口后真机复验，2026-11-25 08:10）

引擎选择改动（engineClient / entryState / isEngineReady）触碰 AI 门面，按
「UI 改动不得破坏既有交互」铁律真机回归既有路径：

- 全新进程 → 主页 → 饥饿游戏详情页 → 更多操作 → 下载 → 搜「Hunger Games」→
  英语 SRT → 译成中文（useRemote=false 走本地引擎，覆盖安装后首译）
- 「翻译准备中…」→「翻译中 9/2003 条…」→ 40 秒后「12/2003」**持续推进，零失败**
- entryState 远程分支未误伤本地入口态；本地模型装载/缓存/进度回调全部原行为
- 证据：`local-engine-regression.png`（进度行 12/2003）
- 环境干扰如实登记：期间设备弹出 SiliconFlow 实名验证系统弹窗（非本 App UI），
  点「暂不验证」关闭后翻译未被打断

## 四、端到端（待用户 key 后补验）

- 免费/低价路径：SiliconFlow 注册 → 填 key（默认模型 tencent/Hunyuan-MT-7B，**注册页显示限免**
  ——**用户 2026-11-25 实测更正：是限时免费不是长期免费**，长期使用比照 Qwen3.5-9B ¥0.5/4.0）→
  测试连接 → 任意外语字幕「译成中文」→ 全程分钟级、0 丢条
- 性能锚点：2003 条 ≤ 10 分钟（串行 81 块 × 上限 7s/块余量）
- 已知风险（如实登记）：Hunyuan-MT-7B 为翻译特化模型，可能不遵 JSON 输出指令——
  解析层已兜底（缺号重试 + 行级 fallback 原文），若实测不遵 JSON 则把 SiliconFlow
  预置模型换为通用模型（Qwen3.5-9B ¥0.5/4.0），实测数据说话
- 错误路径三条（错 key / 错 model / 断网）的中文文案已实现，端到端时逐条复验

## 五、决策记录（实现落定）

1. 请求**不带 response_format**（部分服务商/特化模型不支持会 400；prompt 约束 + 解析兜底）
2. SiliconFlow 默认模型按调研定 Hunyuan-MT-7B（特化风险见 §四，实测后可调）
3. Anthropic 走原生 /v1/messages（SenPlayer 同款）；Gemini 走 v1beta OpenAI 兼容层
4. OpenRouter 预置 + X-Title 溯源头；目标语言 v1 固定 Simplified Chinese（字段已参数化）
5. 引擎切换即时生效（engineClient 按配置返回单例）；远程失败不静默回退本地

## 六、端到端实测与性能优化（2026-11-25 08:30–09:00）

**测试连接真机验证**：用户已填 SiliconFlow key → 「✓ 连通（92 个模型）」（`test-connection.png`）。

**模型实测四轮（PC 裸 API 计时，10 条块）**：

| 模型 | 耗时 | reasoning | JSON | 结论 |
| --- | --- | --- | --- | --- |
| tencent/Hunyuan-MT-7B | — | — | ✗ | 纯译文直出不遵 JSON（真机实测解析失败） |
| Qwen/Qwen3.5-9B | 75.8s | 7763 字符 | ✓ | 思考反噬；chat_template_kwargs 关思考无效 |
| zai-org/GLM-5.3-Flash | 6.0–6.8s | 487–670 | ✓ | 思考关不掉；真机 8 条/分钟 |
| **deepseek-ai/DeepSeek-V4.1-Flash** | **3.4s** | 527 | ✓ | **最快 → 改为预置默认**（¥2/¥8，2003 条 ≈ ¥0.5） |

**架构提速：块流水线（本轮正式破「TranslationJob 不动」承诺，性能目标驱动）**：
- `AiEngineClient` 加 `supportsParallelBlocks()`（默认 false = 本地行为不变）
- `RemoteAiEngineClient` override true；`TranslationJob` 块循环改预取窗口（远程 4 路，
  本地 1 路 = 原串行）；context 只带原文（DEC-14）→ 预取不依赖前块译文，窗口安全
- 裸 API 3.4s/块 × 串行 = 234 条仍要 ~5 分钟；4 路并发预期 ~1 分钟

**在线翻译报错链路真机验证**（Hunyuan-MT 轮）：「AI 翻译引擎未就绪：服务商返回内容
无法解析（no json object in reply），请重试或更换模型」——中文原因清晰（G9 ✓）。

**《笼困》资源端到端**（用户指定真实资源，用户已在面板选好 Caged/英语 SRT/↓10659）：
- 发起翻译：234 条（用户真实观看场景）；GLM 轮实测 8 条/分钟
- DeepSeek+流水线版复测因设备升级 PIN 锁屏（非 App 问题）暂停，待用户解锁

## 七、字幕库（用户 2026-11-25 指出缺失：已下载/已翻译必须可见可管）

- 新文件 `ui/subtitles/library/SubtitleLibraryDialog.kt`：两组列表（AI 翻译成品 =
  filesDir/ai_subs 下 .srt；已下载字幕 = filesDir/subtitles），条目显示
  可读名/大小/日期/删除；入口在 AI 翻译引擎卡片「打开字幕库（已翻译 / 已下载）」
- 坑：Kotlin 块注释**可嵌套**——头注释里 `ai_subs/*.srt` 与 `subtitles/*` 的 `/*`
  会吞掉闭合符致全文件 Unclosed comment，已改写为文字描述
- 保存方案现状（如实告知）：翻译成品 = filesDir/ai_subs/\<cacheKey\>.srt（filesDir
  不被系统清理）；**v1 无块级续翻——进程被杀 = 进度全丢**（用户《笼困》第一次
  长翻译即因此丢失）；块级续翻缓存是下一个卡

## 八、待办

1. 用户解锁设备 → 《笼困》234 条 DeepSeek+流水线复测（预期 ~1 分钟）→ 翻译成品在
   字幕库可见 → 播放挂载验证
2. 块级续翻（杀进程不丢进度）——独立卡