# 在线 AI 翻译接口调研 · 全球版（2026-11-25）

> 背景：本地 Qwen3-1.7B 在 K40 上实测约 12 条/分钟（《龙困》2003 条 ≈ 2.5–3 小时）。
> 目标：接在线 AI 接口把字幕翻译提速到分钟级。
> **产品口径（用户 2026-11-25 定）：面向全球用户，OpenAI / Gemini 必须做，全球接口全部了解一遍。**
> 竞品参照：VidHub / SenPlayer 均提供 AI 字幕翻译且支持自定义 OpenAI 兼容 API（⚠️ 具体型号未逐台核实）。

## 一、核心事实：OpenAI 兼容格式是行业统一标准

一手核实（本轮 webfetch 抓官方文档）：

- **DeepSeek**：BASE URL 标注「OpenAI Format」（api.deepseek.com）
- **OpenRouter**：官方文档原文「normalizes the schema across models and providers to
  comply with the OpenAI Chat API」——一个客户端通吃 400+ 模型
  （OpenAI/Anthropic/Google/Meta/DeepSeek/Qwen/GLM 全有），5xx/限流自动 fallback，
  `usage.cost` 回传实际花费
- **Gemini**：官方提供 OpenAI 兼容层 `generativelanguage.googleapis.com/v1beta/openai/`
  （⚠️ 端点路径待装机复测）
- Anthropic 原生协议（/v1/messages）不兼容，**经 OpenRouter 用 Claude** 即可，v1 不单独适配

**结论：写一个 OpenAI 兼容客户端 = 通吃全球，无需逐家写协议适配。**

## 二、各家数据（官网核实 + 公开知识）

### 一手核实（2026-11-25 抓取）

| 服务商 | 免费路径 | 付费参考 | 备注 |
| --- | --- | --- | --- |
| **Google Gemini** | **Flash/Flash-Lite 免费层可用**；3.5 Flash-Lite 官方定位「optimized for … **translation**」 | Flash-Lite $0.30 输入 / $2.50 输出 每百万；3.6/3.8 Flash $0.75/$3.75 | 全球用户零成本首选 |
| **OpenRouter**（聚合） | `:free` 后缀模型免费（DeepSeek/GLM/Llama free 等） | 一个 key 通吃 400+ 模型；自动 fallback | 海外用户一站式 |
| **DeepSeek** | 无免费额度（充值制） | flash $0.15–0.3 输入 / $0.6–1.2 输出；2003 条 ≈ $0.05 | 国内直连 ✓ |
| **SiliconFlow** | **tencent/Hunyuan-MT-7B 限免**（2026-11-25 用户注册页实测为限时免费，**非长期免费**，以价目页为准） | DeepSeek-V4.1-Flash ¥2/¥8；长期低价比照 Qwen3.5-9B ¥0.5/¥4.0 | 国内直连 ✓ |

### ⚠️ 公开知识（未逐家抓取，待装机核实）

| 服务商 | 免费路径 | 付费参考 | 接入方式 |
| --- | --- | --- | --- |
| OpenAI | 无（gpt-4o-mini 级最便宜） | mini 档 ≈ $0.15–0.6 每百万 | 原生 OpenAI 格式（标准制定者） |
| Anthropic | 无 | haiku 档 ≈ $1/$5 | 经 OpenRouter（OpenAI 兼容） |
| Groq | 免费层（限速） | 开源模型托管，速度极快 | OpenAI 兼容 |
| Mistral | La Plateforme 实验层免费 | tiny 档极低 | OpenAI 兼容 |
| xAI | — | grok-mini 档 | OpenAI 兼容 |
| DeepL API | 50 万字符/月 | — | 独立协议；机翻质量，LLM 免费层已覆盖 → **不做** |

### 真机直连（K40，国内网络，仅说明国内可达性；不代表全球用户）

SiliconFlow/DeepSeek/智谱/Kimi/百炼/百度 ✅ 直连；**OpenAI/Gemini 000 被墙**（需代理）；
DeepL 403 存疑。→ **国内用户连海外接口必须走 App 的 SOCKS 代理机制**（TmdbClient 同款，
tmdb_proxy.txt 配置），这是国内用户用 OpenAI/Gemini 的唯一路径，必须做。

## 三、可行性判定表（全球版）

| 方案 | 判定 | 理由 |
| --- | --- | --- |
| **OpenAI 兼容远程客户端**（一个实现通吃） | **✅ 做** | 全行业标准格式，一手核实 |
| 预置：Gemini（免费层，Flash-Lite 翻译特化） | **✅ 做** | 全球用户零成本；走 v1beta/openai 兼容端点 |
| 预置：OpenAI | **✅ 做** | 全球主流；mini 档便宜；国内用户经代理 |
| 预置：OpenRouter（含 :free 模型） | **✅ 做** | 一个 key 通吃 400+ 模型含 Claude；自动 fallback |
| 预置：SiliconFlow（Hunyuan-MT 免费） | **✅ 做** | 国内用户零成本 |
| 预置：DeepSeek | **✅ 做** | 国内低价 |
| 自定义 base_url + key + model | **✅ 做** | 覆盖 Groq/Mistral/xAI/中转站/本地 Ollama 等 |
| **SOCKS 代理支持**（海外端点经代理） | **✅ 做** | 国内用户连 OpenAI/Gemini 的唯一路径；复用 TmdbClient 机制 |
| Anthropic 原生协议单独适配 | ❌ 不做 | 经 OpenRouter 已覆盖 |
| DeepL 单独适配 | ❌ 不做 | 机翻质量 + 独立协议；LLM 免费层已覆盖 |
| Google Translate 非官方接口 | ❌ 不做 | 违反 ToS、不稳定 |

## 四、架构接入点（不变，见 v1 调研）

1. `AiEngineClient` 按 IPC 形状设计（全异步、可序列化）—— 加 `RemoteAiEngineClient`
   实现同一接口，`TranslationJob` 零改动
2. `AiSubtitleController.sharedClient` 进程级单例收敛，远程实现同走单例
3. prompt 复用本地链的 fewshot + context 结构 → chat/completions 消息体
4. 代理：`RemoteAiEngineClient` HTTP 层接入现有 SOCKS 机制（对海外端点生效）

## 五、建议方案（全球版，待用户拍板）

**一个 OpenAI 兼容客户端 + 五个预置档位 + 自定义**：

| 档位 | 端点 | 默认模型 | 用户 |
| --- | --- | --- | --- |
| SiliconFlow | api.siliconflow.cn/v1 | tencent/Hunyuan-MT-7B | 国内免费 |
| DeepSeek | api.deepseek.com/v1 | deepseek-flash | 国内低价 |
| **Gemini** | generativelanguage.googleapis.com/v1beta/openai | gemini-3.5-flash-lite | **全球免费** |
| **OpenAI** | api.openai.com/v1 | mini 档 | 全球主流 |
| **OpenRouter** | openrouter.ai/api/v1 | 用户自选（含 :free） | 全球一站式 |
| 自定义 | 用户填 | 用户填 | Groq/Mistral/xAI/中转/本地 |

- 设置页「AI 翻译引擎」：本地模型 / 在线接口；在线配置 = 服务商选择 + API Key + 模型名
- 「测试连接」（调 /models 验 key，同 OpenSubtitles 模式）
- 失败给非空中文/英文 reason，**不静默回退**（花钱的事必须用户可见）
- 隐私明示：字幕文本将发送至所选服务商
- 提速预期：2003 条 2.5–3 小时 → 数分钟

**实现范围（家族树增量，待批）**：
- `subtitle/remote/`：`RemoteAiEngineClient.kt`（OpenAI 兼容 HTTP + SOCKS）+
  `RemoteEnginePrefs.kt`（预置档位/端点/key 配置）≈ 2 文件
- 修改：`AiSubtitleController`（引擎选择）、`OnlineSubtitleTranslator`（透传）、
  `OnlineSubtitleSettings.kt`（设置 UI）；`TranslationJob` 不动