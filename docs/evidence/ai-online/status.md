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