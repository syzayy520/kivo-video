# 搜索页落地 · 真机验收证据（2026-11-24）

> 设计：`docs/design/ui/search-2026-11-24/search-final.html`；家族树与偏差：同目录 `family-tree.md`。
> 设备：Redmi K40 `b6e950ee`（1080×2400）；包：`app-debug.apk`，`install -r` 覆盖安装（未卸载、未清数据）。

## 一、构建与门禁

- `assembleDebug`：**BUILD SUCCESSFUL**（三轮：初版 → 修 3 处编译错 → 修 Preview 注解形式）。
- `python tools/ui_gate.py`：搜索页**零新增回归**——无行数超限（全部新文件 ≤300 行）、
  裸色仅 `ui/search/SearchTokens.kt` 且在明示豁免清单、3 个新 `*Page.kt` 均已补 `@Preview`
  （`page_preview_missing` 回落到 baseline 存量的 2）。
- 仍存在的 regressions（行数 4 项、裸色 +22）来自他人在途/存量文件（`NasMediaProbe.kt`、
  `GenreWallData.kt` 等），与本次改动无关；未刷 baseline、未动他人文件。

## 二、四态真机验收

| 状态 | 证据 | 结果 |
| --- | --- | --- |
| ① 搜索首页 | `01-search-home.png` / `01b-search-home-loaded.png` / `home-ui.xml` | 大标题/玻璃搜索框/六分类剧照卡（无文字剧照）/热搜榜真实片单（trending/day 前 10，NEW 按上映 ≤30 天）/底栏实心胶囊。最近搜索无记录时整段不渲染 |
| ② 输入中 | `02-query-hot.png` / `02b-query-list.png` / `query.xml` | 点热搜片名自动填入中文并出结果；输入 `spider` → **共 20 条**，最佳匹配大卡 + 结果列表 + 「TMDB · 未入库」签条逐条真实；最佳匹配命中 NAS 时出现「播放」钮 |
| ③ 分类筛选 | `03-browse-movie.png` / `browse2.xml` | 「电影」标题+返回、tabs、三行筛选、共 20001 部、人气最高排序、只看可播、三列海报网格（★评分·年份） |
| ③ 筛选生效 | `03b-crime.png` / `crime.xml` | 点「犯罪」后片单整体替换为犯罪片（绝境反击/消失的夜晚/杀人者的记忆法…），结果与条件一致 |
| ④ 吸顶摘要 | `04-sticky.png` | 滚动后摘要出现：「人气最高 · 共 20001 部」，tabs/三行筛选滚出，网格延续；摘要含全部非默认条件的逻辑见 `filterSummary` |

## 三、验收中发现并已修的问题

- 热搜第 10 名序号「10」被挤成两行 → 排名列宽 18dp → 26dp + 居中 + maxLines=1，重编译后装机。
- 编译期修 3 处：缺 `SearchGlyph` import、`padding(horizontal=, top=)` 无此重载、`@Preview` 全限定不被门禁正则识别。

## 四、测试手法记录（下一棒复用）

- 中文 IME 下 `adb input text` 只进组词不上屏；英文词输入后补 `keyevent 66`（Enter）可上屏原文。
  中文搜索验证走「点热搜片名自动填入」这条设计内置路径，不依赖 IME。
- `uiautomator dump` / `pull` 在 git-bash 必须 `MSYS2_ARG_CONV_EXCL="*"`，否则 /sdcard 被转成 Git 目录。
- **布局会随最近搜索出现而下移**：点击坐标必须以当次 dump 的 bounds 为准，不能复用旧截图坐标
  （本次③首次未拍到就是旧坐标点空，不是功能 bug）。

## 五、未覆盖范围（不宣称完成）

- 语音输入：已接系统 `RecognizerIntent`（无服务时 Toast 明示），真机未实测识别流程。
- 吸顶摘要点击展开、排序下拉切换：代码已接，未逐点真机点击。
- 热搜名次升降无数据源，不实现。
- 减弱动画开关下的过渡降级已接 `MotionGate`，未在开关开启态截图复核。
- 改动未 commit / 未 push（等用户指令）。

## 六、第二批：渠道集成与集中修复（2026-11-24 下午）

用户追问「只显示 NAS 里面的吗？其他渠道没集成吗？」，检索后集中修复 4 项：

| # | 问题 | 修法与验证 |
| --- | --- | --- |
| P1 | Emby 渠道未集成 | 新增 `search/EmbyPlayableIndex.kt`：已授权源（非隐藏/开聚合搜索/非 SMB）复用 `EmbyAuthCache` 登录，全库标题池（Movie+Series，100/页、≤5000 条/源、10min 缓存），②③④ 共用；池就绪自动重搜/重筛。真机 `query2.xml`：**Spiderhead → `emby.hohai.eu.org 可播`、蜘蛛侠：崭新之日 → `nl2.lovehd.de 可播`、蜘蛛侠/暗影蜘蛛侠 → `emby.bangumi.ca 可播`**，结果共 21 条（较前 +1 Emby 命中） |
| P2 | 「共 20001 部」是 TMDB 分页上限 | `displayCountText()`：>1 万显示「N 万+」；真机 `emby-browse.xml` 显示「共 2 万+ 部」 |
| P3 | 点搜索框进②键盘未唤起 | `SearchFieldEdit` 补 `keyboardController.show()` |
| P4 | Emby 部分源失败不能静默 | `notice` 进②状态区；③「只看可播」页脚显示；索引中提示「结果将陆续补全」 |

- ③「只看可播」真机（`03c-only-playable.png`）：开启后 **共 19 部**（本地可播命中，含 Emby 渠道），开关绿、网格正常，无崩溃。
- 匹配优先级：NAS → Emby → 本机 MediaStore；Emby 结果点击走 `AggregateSource.detailIntent` 进该源详情。
- 构建 BUILD SUCCESSFUL；ui_gate 搜索页零新增回归（存量 4 项行数 +22 裸色仍为他人/存量）。
- 证据新增：`query2.xml`、`emby-browse.xml`、`playable.xml`（dump 时机异常为空，截图正常）、
  `02c-emby.png`、`03c-only-playable.png`。
## 七、键盘不弹出：诊断与结论（2026-11-24 14:10）

现象：点搜索框进②，输入框有蓝色焦点描边与光标，但系统键盘不出现。

诊断链（真机 b6e950ee）：

1. `kbd2.xml`：`android.widget.EditText focused="true"`（焦点正常）；`dumpsys input_method`：
   `mShowRequested=true mInputShown=true`，`mServedView=AndroidComposeView` —— App 已正确请求 IME。
2. 截图 `05c-keyboard-check.png`：`mInputShown=true` 但屏上无键盘 → 输入法窗口级异常，非焦点问题。
3. **对照实验**：临时切默认输入法为 LatinIME → 同一②页面键盘正常弹出（`05d-latinime.png`），
   搜索框焦点描边正常。测完已恢复 `com.tencent.wetype/.plugin.hld.WxHldService`。

结论：**App 侧焦点/键盘请求链路正常；不弹键盘是微信输入法（com.tencent.wetype）在该场景的兼容问题**
（`mInputShown=true` 但其 InputMethod 窗口不可见）。

App 侧已做的稳健修复（`ui/search/home/SearchField.kt`）：
- `AnimatedContent` 过渡期间 `requestFocus()` 会被吞 → 延迟 180ms 再请求；
- `onFocusChanged` 确认真正聚焦后才 `keyboardController.show()`；
- 该修复在 LatinIME 下已验证「进②自动聚焦 + 自动弹键盘」。

给用户的建议：微信输入法设置里检查「悬浮键盘/分屏小键盘」开关，或将默认键盘换成
设备已有的搜狗小米版 / 讯飞 / LatinIME；App 内无法强制第三方输入法显示其窗口。
## 八、点搜索框误入饥饿游戏详情：根因与修复（2026-11-24 14:30）

现象（用户报告）：一点搜索框就进入「饥饿游戏」详情页。

复现与根因：
- ②保留着上次搜索（饥饿游戏）的最佳匹配大卡；①搜索框区域（y252–384）与②大卡
  （y≈302 起）**位置重叠**。用户因键盘不出**连点搜索框**时，第二次 down 落在②大卡上
  → `openEntry` → 详情。单击复现正常（`repro5.xml`），连点即中。

修复（`ui/search/SearchOverlay.kt`）：从①点搜索框进②一律 `search.clear()` 开始新搜索，
②不再一进来就把旧大卡摆在搜索框下方。

验证（`fix1.xml`）：第一次点①搜索框 → ②空白（仅搜索框+取消，无大卡）；
第二次点同位置 → 仍 `MainActivity`，未进详情。BUILD SUCCESSFUL，已装机。
## 九、微信输入法根因坐实与第三批六项修复（2026-11-24 下午）

### 键盘根因（用户已确认修好）

- 换 LatinIME 能弹、wetype 不弹 → 补 `imeAction=Search` 后 **wetype 九宫格正常弹出、蓝色「搜索」键**
  （`06-wetype-imeaction.png`）。根因：输入框未声明 `imeAction`，wetype 对缺省字段键盘形态异常；
  App 侧可解，不需要用户换键盘。

### 第三批六项修复（用户「按你的建议推进」授权）

| # | 修复 | 验证 |
| --- | --- | --- |
| 1 | 键盘「搜索」键提交记入最近搜索（`onSubmitQuery`） | 代码已接线；**待用户手动按一次蓝色「搜索」键确认**（adb 硬件 Enter 不触发 IME action，估算坐标会误触结果条目） |
| 2 | ③顶栏背景常驻不透明 | `08-browse-topbar.png`：滚动时海报不再从「电影」标题下穿过；吸顶摘要「人气最高 · 只看可播」 |
| 3 | 网格到底计数改 `items.size` | 未滚到底：可播命中随分页真实增长 19 → 35 → 68 → 125 部，footer 未出现；逻辑已改，不再用 TMDB 上限 total |
| 4 | 部分源失败提示与结果并存 | `QueryStatus` 改为状态文案 + notice 双行；未制造故障，代码路径 |
| 5 | 进搜索 `local.invalidate()` 重读 NAS 索引 | `MainActivitySearch` 每次进覆盖层执行；代码路径 |
| 6 | 热搜「今日」可点刷新（`hotVersion++` 重跑 produceState） | `today.xml` 已点击；缓存读取快，未抓到「正在加载」文案 |

- 构建 BUILD SUCCESSFUL；ui_gate 搜索页零新增回归（存量 4 项行数 +22 裸色为他人/存量）。
- 遗留：语音输入未实测；搜索键记录待手动；footer「已显示全部」需结果集有限时复核。
## 十、遗留项收口（2026-11-24 傍晚）

- **语音输入实测通过**（`voice.xml`）：①搜索框麦克风调起小米语音识别
  `com.xiaomi.mibrain.speech/.asr.AsrActivity`（「正在倾听 / 说完了」），非 Toast 降级。
- **footer 计数实测通过**（`step9.xml`）：③ 大陆 + 更早 + 只看可播，滚到底显示
  **「已显示全部 47 部」**，与吸顶摘要「共 47 部」一致 —— 真实加载条数，不再是 TMDB 上限假数。
- **吸顶摘要全条件验证**：「大陆 · 更早 · 人气最高 · 只看可播」—— 地区/年份/排序/可播全部入列（D4）；
  点摘要 `animateScrollToItem(0)` 展开回顶部有效。
- 可播命中随分页增长实测：大陆+只看可播 4 → 42 → 89 → 171 部（分页加载，非一次性假数）。
- 测试手法教训（记入下一棒）：**每步 dump 确认页面再点**，盲连点会误入详情；
  缩小结果集（地区+年份+只看可播）是把网格滚到底的正确方法，不要盲翻大结果集。
- 唯一剩余：搜索键记入最近搜索，待用户手动按一次蓝色「搜索」键确认（代码已接线）。
## 十一、最近搜索记录口径补强与最终收口（2026-11-24 晚）

- 问题：adb 无法可靠触发 IME 搜索动作，「搜索键记录」无法真机验证；且部分 IME 不发 action，
  只靠搜索键记录对真实用户也不可靠。
- 修法（`SearchOverlay.kt`）：取消/返回②时，`query` 非空且有结果也记入最近搜索
  （`cancelQuery()`，取消按钮与系统返回键同路径）。搜索键 `onSubmitQuery` 保留。
- 真机验证（`recent-final.xml`）：输入 spider 出结果 → 点取消 → ①最近搜索第一条 = **spider**。
- 临时验证目录 `build/verify-tree` 已清理；本卡全部待办闭环。
## 十二、语音卡与三项交互修复（2026-11-24 深夜）

- **误改回滚**：最佳匹配大卡曾被误改为 v2，用户指出后回滚原稿；后按用户决策**去掉大卡播放钮**
  （`no-play.xml`：`HAS_PLAY=False`）。
- **点搜索框边框进首页海报详情**：根因是覆盖层未兜底消费点击，边框/空白点击穿透到首页第一个海报。
  `SearchOverlay` 根容器加 `pointerInput { detectTapGestures(onTap = {}) }` 拦截；
  真机 `step-e.xml`：②空白区点击后仍停搜索页，不再进详情。
- **语音卡**：系统旧弹窗 → App 内 `SpeechRecognizer` + 自绘底部轻面板。
  `VoiceSearchController.kt`（识别）+ `VoiceSearchDialog.kt`（UI）+ `RECORD_AUDIO` 权限。
  迭代：居中巨卡 → 底部面板 → 近乎不透明面板 + 文案去重 + 主按钮收窄 + 次按钮浅底 + 失败「键盘输入」退路。
  终版 `13-voice-final2.png` 评审：不透字、层级协调、可定稿。