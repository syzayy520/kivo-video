# Kivo Video 交互逻辑与体验缺陷审计（QA）

> 审计人：严过关（Yan，QA 工程师）
> 日期：2026-11-19
> 验证强度口径：**PASS**＝真机确认；**CODE_REVIEW_ONLY**＝仅代码推断，未真机复核。
> 本轮真机未验（详见 §1.2），除特别说明外所有结论均为 **CODE_REVIEW_ONLY**。

---

## 1. 审查范围与方法

### 1.1 代码审查覆盖（只读，未改任何代码）

| 维度 | 覆盖文件（均在 `android/app/src/main/java/com/kivo/cinema/`） |
|---|---|
| 宿主与导航 | `MainActivity.kt`、`MainActivityShell.kt`、`AndroidManifest.xml` |
| 详情壳与编排 | `ui/detail/MovieDetailActivity.kt`、`DetailPage.kt`、`DetailLoadFeedback.kt`、`DetailOverlayHost.kt`、`DetailConfirmDialog.kt`、`DetailHeroContent.kt`、`EmbyResource.kt`、`SmbDetail.kt`、`actions/DetailActions.kt` |
| 在看页（多选/滑动删除） | `ui/design/WatchingPage.kt`、`WatchingSwipeRow.kt` |
| 播放器（Activity + 手势 + 返回） | `ui/compose/PlayerComposeActivity.kt`、`PlayerActionRouter.kt`、`PlayerGestureRouter.kt`、`player/PlayerGestureLayer.kt` |
| 设置页导航 | `ui/design/settings/SettingsNav.kt`、`SettingsAppearanceSub.kt` |
| 我的页 | `ui/profile/ProfilePage.kt`、`profile/chrome/ProfileChrome.kt` |
| 通用组件 | `ui/components/HitBox.kt`、`KivoBackPill.kt`、`ui/design/home/HeroPullZoom.kt` |

参考文档：`docs/evidence/p403-design/D370-STATUS.md`（历史坑：无超时 get、返回横屏残留、串台）、`AGENTS.md`（项目铁律）、`docs/design/ui/replica/` 交互规格。

### 1.2 真机验证状态：未验（设备敏感期铁律）

```
adb devices → b6e950ee(K40) device, dd137cf7 device
K40: mWakefulness=Awake, topResumedActivity=com.kivo.cinema/.MainActivity
关键判据：deviceLocked 无输出但 keyguard 服务不可查；exec-out screencap 得到 15KB 纯黑帧
```

- 截图为**纯黑**（屏幕熄灭/锁屏覆盖），`mWakefulness=Awake` 与黑屏矛盾，说明设备处于敏感期。
- 依据任务书铁律「锁屏/敏感期不要碰真机、不要为 UI 故障 reboot/swipe/keyevent 乱试」——**立即停止真机交互**，删除无效黑屏取证图，**全部结论降级为 CODE_REVIEW_ONLY**。
- 注：MatePad 指定 ID `CFE6R21B12000179` 未连接；实际在线的另一台为 `dd137cf7`，未擅自使用。

### 1.3 方法

静态代码走查 + 关键交互链路追踪（入口→状态→反馈→返回），对照 PRD/设计稿与项目历史坑清单，标注风险等级 P0（阻断/数据错）/P1（明显体验缺陷）/P2（边角/一致性）。

---

## 2. 问题清单

> 共 **18 项**。验证方式一栏若无真机，均写"真机待验（当前 CODE_REVIEW_ONLY）"。

| # | 类别 | 严重度 | 复现步骤或代码位置 | 期望 vs 实际 | 修复建议 | 验证方式 |
|---|---|---|---|---|---|---|
| Q-01 | 导航/反馈 | P1 | `ProfilePage.kt:88,109` + `MainActivityShell.kt:247-250`：「我的」页 `onEditEnvironment = {}`、`onOpenItem = {}`，但 ProfilePage 里"编辑"按钮（`ProfileChrome.kt:72`）、收藏/待看/在看条目 `openEntry` 都调到这些空实现 | 期望：点"编辑"进入环境编辑、点条目进详情；实际：**点了完全没反应，无任何提示** | 补接真实目标（详情/编辑页）；若功能未实现，按铁律给非空 reason 的 Toast/禁用态，不要静默 no-op | 真机待验：我的页点"编辑"、点待看卡片 |
| Q-02 | 反馈 | P1 | `DetailActions.kt:31-56`：`toggle` 在 `!(snapshot.ready && !saving)` 时仅 `NetDiag.log` 后静默 return | 期望：收藏/标记按钮在不可用时置灰或提示"处理中"；实际：用户点击**无任何视觉反馈**，取证只能看日志 | UI 层给禁用态/转态/Toast，`saving` 时按钮不可重复点 | 真机待验：详情页刚进入瞬间连点收藏 |
| Q-03 | 边界/导航 | P1 | `EmbyResource.kt:94,110,123,328`：`embyTask.get()`、`tmdbTask.get()`、`seasonsTask!!.get()`、`futures.map{it.get()}` **无超时**（D376 同类坑） | 期望：任一源站挂起有总预算 + 留痕 + 局部降级；实际：底层 HTTP 虽有 3–20s 超时兜底（有界），但剧集逐季 get 串行叠加，可能数十秒才回，且失败时无 reason 直接少集 | 按 D376 模式加共享总预算 + `catch(Throwable)` 留痕 + `onPartial`；尤其 L328 逐季 get | 真机待验：弱网/单季源站超时进剧集详情 |
| Q-04 | 导航/返回栈 | P1 | `MainShell.kt` 中「在看」为覆盖层 `showWatching`，`WatchingPage.kt:75 BackHandler` 与 `SettingsNav.kt:33`、`SettingsAppearanceSub.kt:33` 等**多处 BackHandler 并存**；详情页又叠 `DetailPage.kt:277` 的浮层 BackHandler | 期望：返回键只消费最顶层；实际：Compose BackHandler 默认 `true` 且同页多个 enabled 时，**同屏多 handler 竞争**（如设置钻入页 route≠null 与 drill≠null 同时为真时，两层 BackHandler 都在栈内），存在一次返回连关两层的风险 | 统一用单一 `OnBackPressedDispatcher` 分层消费，子层 enabled 时确保父层 disabled；补嵌套返回单测 | 真机待验：设置根→外观→导航钻入后按返回；在看多选态按返回 |
| Q-05 | 手势/一致性 | P2 | `WatchingPage.kt:49` 注释写"**右滑**单条"，`:94` 提示语写"**向左滑动**移除单条"，`WatchingSwipeRow.kt:58,95` 实现为**左滑**（iOS 习惯，实现正确） | 期望：文案与实现一致；实际：注释与用户提示矛盾（用户看到的是"向左滑动"，与实际左滑一致，故仅注释误导维护者） | 修正 `WatchingPage.kt:49` 注释为"左滑" | 纯代码核对（本项即代码结论） |
| Q-06 | 手势/热区 | P2 | `WatchingPage.kt:74-77`（多选"取消"/"全选"）、`:84,88`（"全部移除"/"移除"）：均为 `Text + clickable + padding(10dp/7dp)`，**未用 HitBox** | 期望：触摸目标 ≥48dp（项目 `HitBox.kt:26` 自定下限）；实际：文字 13-14sp + 纵向 padding 7-10dp，高度约 27-34dp，**低于 48dp** | 用 `HitBox`/`minimumInteractiveComponentSize` 包裹 | 真机待验：TalkBack/手指点边缘命中率 |
| Q-07 | 可访问性 | P2 | `DetailConfirmDialog.kt:33-40`：确认/取消按钮 `clickable(...).padding(8.dp)`，顺序先 clickable 后 padding → 命中区不含 padding 扩展 | 期望：命中区含 padding 且 ≥48dp；实际：`.clickable{}.padding{}` 使 8dp padding 落在命中区**外侧**，实际可点区域仅文字本身 | 调整为 `padding` 在前、`clickable` 在后，或包 HitBox | 真机待验：点文字周边 8dp 是否可点 |
| Q-08 | 反馈/边界 | P2 | `MovieDetailActivity.kt:154-158`：intent 参数无效时 `Toast("影片 ID 无效") + finish()` | 期望：无效深链给明确原因且不留白屏；实际：`smbReady` 只校验 source+target 非空，`smbShare` 可为空仍放行 → 进详情后 SMB 打开失败，反馈滞后且不在入口 | 入口补齐 share/path 校验，并统一错误文案 | 真机待验：伪造缺失 extra 的 smb intent |
| Q-09 | 反馈 | P2 | `DetailOverlayHost.kt:49-52`：下载按钮 `Toast("下载功能尚未开放")` | 期望：未实现功能应禁用态并说明原因（项目铁律"能力缺失带非空 reason 且用户可见"）；实际：按钮可点、点了才 Toast，属"先可点后告知不可用" | 直接置灰 + 副标题说明，减少无效点击 | 真机待验：详情页更多→下载 |
| Q-10 | 反馈/边界 | P2 | `DetailActions.kt:52`：`writeError = "保存失败：${it.message ?: "请检查设备存储空间后重试"}"`，`it.message` 可能是英文异常原文 | 期望：面向用户的中文、可操作文案；实际：可能直接暴露底层异常 message（如 `IOException: ...`） | 统一映射为用户可读文案，原始异常只进 NetDiag | 真机待验：-storage 满/只读时收藏 |
| Q-11 | 反馈 | P2 | `DetailLoadFeedback.kt:29-39`：仅当 `error!=null && hasContent` 用 Snackbar（Indefinite + 重试）；`DetailPage.kt:90` 有 preview 时无内容失败文案为"详情加载失败" | 期望：所有失败给原因分类 + 重试；实际：文案笼统，未区分网络失败/源站 404/解析失败，用户无法判断该重试还是放弃 | 复用首页五类错误原文口径 | 真机待验：详情页断网/错 ID |
| Q-12 | 导航/横竖屏 | P2 | `AndroidManifest.xml:39` `MovieDetailActivity` **无 `configChanges`、无 `screenOrientation`**；`MainActivity` 手机端锁竖屏、`PlayerComposeActivity` 强制 landscape | 期望：跨 Activity 旋转/返回方向一致；实际：详情页随传感器旋转重建，`LaunchedEffect(reload)` 重新加载（有预览则无感，无预览会重新转圈），横屏转竖屏时详情布局可能重排。D377 已修"播放器返回横屏残留 5-6s"，但详情页自身旋转路径未纳入回归 | 明确详情页方向策略（跟随/锁竖屏）并补旋转回归 | 真机待验（MatePad/手机）：详情页旋屏 |
| Q-13 | 手势/冲突 | P2 | `player/PlayerGestureLayer.kt:114-179`：横向拖动、纵向拖动、tap/double-tap、长按拖动 **4 个独立 pointerInput 共享事件**，靠 `change.consume()` 与 `singleFingerBlocked()` 互斥 | 期望：斜向滑动主方向明确、无误触；实际：横滑 seek 与长按调速在"斜向 + 长按"组合下都可能消费，`onLongPressDrag` 与横滑竞态；设计文档自称"互斥"但无自动化测试锁定 | 补手势组合的真机/单测（斜滑、长按+横滑、双指中插一指） | 真机待验：播放器斜滑、长按后横滑 |
| Q-14 | 手势/边界 | P2 | `PlayerGestureRouter.kt:16-26`：`onVerticalDragStart` 在 `verticalSwipeEnabled=false` 或 亮度关闭时 `return`，但 `onDragEnd` 仍会 `scheduleHudHide()`；`onHorizontalDragStart` 在 `durationMs<=0` return | 期望：开关关闭时手势完全降级且无残留；实际：初始片段未就绪时横滑 seek 静默无效（无 HUD/无提示），用户误以为卡顿 | 对"能力关闭/未就绪"给一次性 toast 或状态提示 | 真机待验：起播瞬间横滑/竖滑 |
| Q-15 | 可访问性 | P2 | 全量走查：`WatchingPage.kt:83-84` 的"全选"、"取消"等 `Text+clickable` 无 `contentDescription`；`WatchingSwipeRow.kt:103` 删除按钮仅 TrashIcon 有 role 无语义文案 | 期望：TalkBack 可朗读每个可点元素；实际：多处纯文本按钮依赖文字本身可朗读尚可，但图标类删除按钮缺 label | 为所有图标按钮补 `contentDescription`，文本按钮确认合并语义 | 真机待验：开启 TalkBack 遍历在看页 |
| Q-16 | 可访问性 | P2 | `WatchingSwipeRow.kt:186-202`、`WatchingPage.kt` 大量字号用 `sp`（正确），但 `DesignTokens` 中部分固定高度容器 + `maxLines=1`（如 `:189` 标题） | 期望：系统字号放大到 200% 不截断；实际：固定行高 + 单行省略，大字模式下元信息/进度可能被裁 | 用 `wrapContentHeight` 或支持多行，补字号缩放回归 | 真机待验：系统字体放大 200% |
| Q-17 | 边界/重复点击 | P2 | `MovieDetailActivity.kt:106,124-135`：`openingPage` 防重入于 `onResume` 复位；`PlayerComposeActivity.kt:234-235`：`livePlayer` 新页 finish 旧页 | 期望：多来源快速连点不叠页/不双音频；实际：`openingPage` 仅覆盖 `openPage`（分享等），**`startActivity` 详情/播放器入口本身无去重**，快速双击卡片可能叠两个详情/播放 Activity | 统一在路由层加 300ms 防抖或 singleTop | 真机待验：连点海报/继续观看 |
| Q-18 | 反馈/导航 | P2 | `MainActivity.kt:170-201` `openHomeEntry` Emby 分支：会话未命中时 `NetDiag.log` 后 `startActivity(EmbyActivity)` | 期望：点"继续观看"应直接续播；实际：冷启/TTL 过期时**静默退回浏览页**，用户需再找一次片，与 D377 已修的同类问题（只是没走 start 快路径）体验相近 | 退回时给 Toast 说明"会话已过期，请在服务器页重进"，并补按 sourceId 取会话直达详情 | 真机待验：冷启点"继续观看"Emby 条目 |

---

## 3. 高风险交互链路

### 链路 A：首页 → 详情 → 播放 → 返回（最高频，跨 3 个 Activity）
- 风险点：Q-12（详情旋屏重建 + 播放返回方向一致性，D377 曾出 5-6s 横屏残留）、Q-13（播放器四手势竞态）、Q-17（连点叠 Activity）、Q-18（继续观看静默退化）、Q-04（多级返回栈竞争）。
- 建议重点回归：手机端"首页卡片→详情→播放→返回"方向必须在 1s 内回竖屏；平板端旋转不丢播放。

### 链路 B：在看页多选删除（删除=不可逆数据操作）
- 风险点：Q-05（滑动方向文案/注释矛盾，易误导后续维护改成反向）、Q-06（多选按钮热区 <48dp 误触/漏触）、Q-04（多选态 BackHandler 与页面 BackHandler 竞争，可能一次返回直接退出不选）、Q-15（删除按钮 TalkBack 无标签）。
- 建议重点回归：左滑单条→确认；长按 2s→多选→全选→移除；多选态按返回应只退多选态。

### 链路 C：收藏 / 待看 / 已看标记（反馈完整性）
- 风险点：Q-02（未就绪/保存中静默无反馈）、Q-10（错误文案暴露英文异常）、Q-09（下载先可点后提示）、Q-01（我的页汇总卡片点击空实现，用户"收藏了却点不开"）。
- 建议重点回归：详情页首帧立刻点收藏；断网/存储满时标记；标记后到"我的"页点该条目。

### 链路 D：详情页加载与浮层（错误态/返回层级）
- 风险点：Q-03（无超时 get 串行叠加）、Q-11（错误文案笼统）、Q-04（日历/更多/选季三层浮层 + 确认框的 BackHandler 层级）、Q-07（确认按钮命中区缺陷）。
- 建议重点回归：弱网进剧集详情；日历→移除→确认框→连续返回逐层关闭。

### 链路 E：设置页多层钻入 + 我的页分块（导航深度/空回调）
- 风险点：Q-04（route/drill 双层 BackHandler）、Q-01（编辑环境空回调）、Q-16（大字模式分块裁切）。
- 建议重点回归：设置根→外观→导航钻入→返回逐级；我的页"编辑"与各卡片。

---

## 4. 回归测试建议（修复后应补用例）

**导航/返回栈**
1. 设置页：根→外观→导航钻入，连续返回，断言每次只退一层、最终回主 Shell。
2. 在看页：多选态按系统返回 → 仅退多选态；再按 → 才关闭在看覆盖层。
3. 详情页：日历开→更多开→确认框开，逐层返回，断言层级顺序与状态一致。
4. 播放→返回：手机端 1s 内回竖屏（回归 D377）；详情页旋屏不重复加载。

**状态反馈**
5. 收藏/标记：未就绪、保存中、成功、失败四态都有可见反馈；失败文案中文且不含原始异常字符串。
6. 加载失败：详情页无预览/有预览两种，网络失败/404/解析失败三类文案可区分且可重试。
7. 能力缺失（下载）：按钮为禁用态 + 非空 reason。

**边界/并发**
8. 剧集详情：模拟单季 get 挂起，断言总预算内返回 + 留痕 + 局部降级（不静默少集）。
9. 连点：快速双击海报/继续观看，断言只起一个目标 Activity。
10. 冷启点"继续观看" Emby 条目，会话未命中时有用户提示。

**手势/一致性/可访问性**
11. 在看页：左滑单条露出删除（不是右滑）；短距离滑动回弹；长按 2s 进多选。
12. 播放器手势组合：斜滑、长按+横滑、双指缩放中插一指，断言主方向唯一、无双响应。
13. 触摸目标：多选按钮、确认框按钮命中区 ≥48dp（含 padding 内侧）。
14. TalkBack：在看页删除、返回、全选等图标按钮可朗读 label。
15. 字号 200%：在看行、详情确认框、设置分块无截断/重叠。

---

## 5. 结论与路由

- **路由决策：工程师（需修复）** — Q-01/Q-02/Q-03/Q-04 为 P1，均为源码侧实现缺陷（测试/断言预期符合设计），应回工程师修复；其余 P2 为一致性/边角改进。
- **已知限制**：本轮未真机验证（设备熄屏敏感期），所有问题为 `CODE_REVIEW_ONLY`，需在有解锁设备时按 §4 用例复核，尤其手势竞态与返回栈（纯静态审查无法定论的部分）。
- **正面结论**：项目已系统性修复历史坑——SMB 详情图缓存 key 已带唯一 token（`SmbDetail.kt:143-148`）、TMDB 增强 2.5s 限时（`SmbDetail.kt:151-184`）、播放器返回横屏残留已修（D377/`MainActivity.kt:66-73`）、返回圆圈统一 48dp 命中（`KivoBackPill.kt`）、手势层用 `rememberUpdatedState` 修陈旧闭包（`PlayerGestureLayer.kt:68-86`）。这些不在本清单重复报。
