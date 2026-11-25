# 交接：在线 AI 翻译引擎（2026-11-25 收口态）

> 接手人请先读 `AGENTS.md` 与 `docs/handoff/CURRENT.md` 头部（铁律与构建口径）。
> 本文档覆盖 2026-11-25 全天工作：在线字幕 AI 翻译从调研 → 实现 → 实测 → 提速 → 字幕库。
> 所有提交已推送至 `codex/kivo-video-p0-ui-01-playback-overlay-v13`，工作区有**其他卡的在途改动**
> （detail 重构、watch-history 等），**不属于本卡，不要动也不要提交它们**。

## 一、本轮提交链（全部已推送）

| 提交 | 内容 |
| --- | --- |
| `3b0da311` | fix: 译成中文 IndexOutOfBounds——空槽位整批失败+进程级共享引擎 client+详情页路径装载模型 |
| `3d44e89f` | feat: 在线 AI 翻译引擎——OpenAI 兼容+Anthropic 双协议 7 档服务商（SenPlayer 逆向对齐） |
| `ee63879b` | docs: 引擎收口后本地引擎真机回归——翻译 12/2003 持续推进零失败 |
| `bf546fde` | fix: 接口配置弹窗填不进去（输入框绑 prefs 不触发重组）；限免口径更正 |
| `ddf6fbc8` | feat: DeepSeek-V4.1-Flash 预置 + 远程引擎 4 路块流水线 + 字幕库面板 |
| `54bbdaf9` | docs: Caged 真实片源 PC 同款链路实测——234 条 151s 0 缺号，成品推回设备 |

## 二、代码落位（家族树）

```
android/app/src/main/java/com/kivo/cinema/subtitle/remote/   ← 本卡新族（3 文件）
├── RemoteEnginePrefs.kt        # 7 档服务商 enum + SharedPreferences(remote_ai_engine)
├── RemoteEngineProtocols.kt    # 请求/响应编解码纯函数（单测主战场）
└── RemoteAiEngineClient.kt     # AiEngineClient 实现：HTTP+SOCKS+testConnection
android/.../ui/subtitles/library/SubtitleLibraryDialog.kt    ← 字幕库（已翻译/已下载可见可删）
android/.../ui/subtitles/online/AiEngineSettings.kt          ← 设置卡片（挂 SettingsPlaybackSub）
```

修改过的既有文件：`AiSubtitleController.kt`（engineClient 按配置选本地/远程单例 +
entryState/isEngineReady 远程语义）、`OnlineSubtitleTranslator.kt`、`TranslationJob.kt`
（块循环改预取窗口：远程 4 路 / 本地 1 路原行为）、`AiEngineClient.kt`
（加 `supportsParallelBlocks()`）、`SettingsPlaybackSub.kt`（挂载）、
`build.gradle.kts`（**testImplementation org.json:json:20240303 —— 勿删**，JVM 单测必需）。

单测：`app/src/test/.../remote/RemoteEngineProtocolsTest.kt`（15 项全绿）。
设计：`docs/design/ai-online-2026-11-25/{research,design}.md`；
证据：`docs/evidence/ai-online/status.md`（九节）+ 5 张截图。

## 三、当前设备状态（K40 b6e950ee）

- **已装机**：最新版含全部功能（`install -r` 覆盖，未卸载过）
- **用户已配好 SiliconFlow key**：`shared_prefs/remote_ai_engine.xml`（provider=SILICONFLOW、
  use_remote=true、key 已填）——**不要外泄、不要提交该文件**
- **测试连接已验证**：「✓ 连通（92 个模型）」
- **《笼困》(Caged 2011) 中文字幕成品已在设备**：
  `files/subtitles/75f157a51d23-Caged.chs.srt`（234 条全中文，PC 端 App 同款 prompt 翻译，
  质量已人工抽验）——用户播放 Caged 时经字幕面板「外挂/导入字幕」可选中
- **锁屏注意**：设备可能又自动锁（MIUI 重置超时）；曾设 `screen_off_timeout 600000` +
  `svc power stayon true`，若失效需重设。PIN 密码锁**不能也不应绕过**，让用户解锁。

## 四、实测结论（数据说话，勿凭印象）

1. **模型选型四轮淘汰**（PC 裸 API 计时，10 条块）：
   - Hunyuan-MT-7B：不遵 JSON（纯译文直出）→ 已加行级兜底兼容（parseSlots 纯文本行映射）
   - Qwen3.5-9B：思考反噬 75.8s/块，`chat_template_kwargs` 关思考无效
   - GLM-5.3-Flash：6s/块但思考关不掉
   - **DeepSeek-V4.1-Flash：3.4s/块 JSON ✓ → 当前预置默认**（真实 25 条块 ≈15s）
2. **Caged 真实片源端到端（PC 同款链路）**：234 条 / 151s 串行 / **0 缺号**；
   4 路流水线预期 40-60s
3. **本地引擎回归通过**：门面改动后本地翻译 9→12/2003 持续推进零失败
4. 在线引擎 App 内翻译已真实跑起来（GLM 轮 8 条/分钟）；DeepSeek+流水线的 **App 内复测未完成**
   （卡在设备导航，见 §五-1）

## 五、未完成事项（按优先级）

1. **App 内 DeepSeek+流水线真机复测**：让用户播放《笼困》→ 字幕 → 搜索字幕 → 点
   「译成中文」，观察 234 条是否 ~1-2 分钟完成。**注意三个导航坑**（均为既有问题非本卡引入）：
   - 本地文件版详情页曾渲染为空页（仅返回按钮）
   - 继续观看行 Caged 卡片在几次播放后被移出
   - 播放控制层「字幕」按钮命中不稳定（3s 自动隐藏与点击竞争）；可靠点法：
     `tap(1200,540) 唤层 → 0.5s 后 tap(2180,152)「字幕」→ 3s 后 dump`（横屏坐标）
2. **块级续翻（已立项未开工）**：v1 只在任务完成/停止时写成品，**进程被杀=进度全丢**
   （用户《笼困》第一次 2003 条翻译即因此丢失，用户已表达不满）。方案：每块 commit 时
   落盘中间态（journal 已有块粒度数据，差「重启时从 journal 恢复 translations 数组」）
3. 翻译完成后的自动挂载与播放验证（用户翻完看片是完整闭环，目前成品靠手动导入路径）
4. SubHD「网络不可达」（App 内失败但 curl 可达）：诊断入口 `files/subhd-debug.txt` 已就位未抓到数据

## 六、关键坑（踩过的，勿重踩）

- **Compose 输入框不能直接绑 SharedPreferences**：读值不触发重组 = 「填不进去」；
  必须本地 `remember` state + 保存时落盘（`AiEngineSettings.kt` 已修，照抄该模式）
- **Kotlin 块注释可嵌套**：KDoc 里写 `ai_subs/*.srt` 的 `/*` 会吞掉闭合符 → 全文件
  Unclosed comment 编译错（报错行还是无辜行）——注释里路径用文字描述
- **JVM 单测 org.json 是桩类**：方法全抛 not mocked → 单测全挂；需
  `testImplementation("org.json:json:20240303")`（已加）
- **MSYS 路径转换**：bash 里 adb 命令必须 `export MSYS2_ARG_CONV_EXCL="*"` 前缀，
  丢失后 `/sdcard/...` 会被改写成 `C:/Program Files/Git/sdcard/...`
- **HyperOS 安装弹窗**：`adb install -r` 与 `input tap 347 2111` 并行（9 秒后自动拒绝）
- **设备私有目录写入**：sdcard 中转 + run-as cp 会 Permission denied；用
  `adb shell "run-as com.kivo.cinema sh -c 'cat > files/...'" < 本地文件`（stdin 直写）
- **翻译失败必须中文 reason 上屏**（G9）：`TranslationJobResult.reason` 字段已建，
  UI 已接；勿回退成英文异常原文
- **花钱的事不静默回退**：远程失败不自动切本地，用户可见后手动切

## 七、验证口径（接手后自查）

```bash
# 构建（增量 daemon 崩溃用 -Pkotlin.incremental=false）
cd "C:/kivo video/android" && JAVA_HOME="C:/Android/jdk-17.0.13+11" ./gradlew testDebugUnitTest -Pkotlin.incremental=false
# 装机（K40 b6e950ee，禁卸载）
MSYS2_ARG_CONV_EXCL="*" C:/Android/sdk/platform-tools/adb.exe -s b6e950ee install -r app/build/outputs/apk/debug/app-debug.apk
# git（safe.directory 必须）
git -c safe.directory="C:/kivo video" status --porcelain
```

UI 验证路径：设置 → 播放 → 滚到底 → 「AI 翻译引擎」卡片（开关/服务商/模型/密钥/
测试连接/字幕库）。字幕库入口 = 卡片内「打开字幕库（已翻译 / 已下载）」。

## 八、产品口径（用户定案，勿翻案）

- **全球覆盖**：OpenAI/Gemini 必须做（用户原话「甭管国内用户能不能用…覆盖得广一点
  总是没错的」）；国内 SiliconFlow/DeepSeek、海外 OpenAI/Gemini/OpenRouter/Anthropic+自定义
- **Hunyuan-MT-7B 是限免不是长期免费**（用户实测更正；三份文档已改口径）
- SenPlayer 逆向实证（`build/rea-senplayer/raw/lib/arm64-v8a/libMLibKit.so`，Rust）：
  预置 5 端点、无内置 key、双协议、JSON prompt 全文——我们的实现对齐它并加 OpenRouter
- 目标语言 v1 固定简体中文；默认引擎=本地（不花钱），在线是显式 opt-in