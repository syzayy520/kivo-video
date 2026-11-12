# P256 聚合搜索 + 类型墙 · 真机证据（第一轮 + 第二轮）

- 设备：Redmi K40 `b6e950ee`（M2012K11AC，1080×2400，density 440）。**无平板在场**，宽屏形态用
  `wm density 200`（≈864dp ≥ 700dp 阈值）在同一真机上验证，验毕恢复 440 —— 不是平板性能/渲染证据。
- 构建：`./gradlew :app:assembleDebug --offline --no-configuration-cache`（build256g/h），固定 kivoDebug 签名覆盖安装。
- 日期：2026-11-12（第一轮 + 第二轮实现与验证）、2026-11-13（归档）。
- 设计稿：`docs/design/ui/replica/emby-aggregate-search.md`（§六 第一轮、§七 第二轮）。
- 脱敏：账号/密码/TMDB key **零入库**；服务器主机名按 p255 惯例可见（非凭据）。
  本地分区截图含用户家庭视频帧 → 缩略图区域高斯模糊（radius 28）后入库，标题/分区头/状态行保留可读。

## 一、闸门表

| # | 闸门 | 结果 | 证据 |
| --- | --- | --- | --- |
| R1-1 | 跨源合并搜索：两源并行、去重、状态行计数 | PASS | r1-01（120 个结果 = 两源合并） |
| R1-2 | 空态/清除：空查询词有可见文案，清除回浏览态 | PASS | r1-02 / r1-03 |
| R1-3 | 类别筛选生效（电影 chip → 全电影网格） | PASS | r1-04 |
| R1-4 | 结果→所属源详情（nyamedia 条目进 nyamedia 详情） | PASS | r1-05 |
| R1-5 | 类型墙 24 渐变卡 + 扇形真实海报 + 到底脚注 | PASS | r1-06 |
| R1-6 | 墙卡→搜索预选 genre | PASS | r1-07 |
| R1-7 | genre 生效（西部 61 vs 不限 120） | PASS | r1-08 |
| R1-8 | 宽屏 4 列网格 / 3 列墙（density 200） | PASS | r1-09 / r1-10 |
| R2-1 | 入口归位：首页底栏右下搜索钮 → 聚合搜索页 | PASS | r2-01 → r2-02 |
| R2-2 | 固定搜索框：玻璃栏+状态+摘要在网格外，深滚不动 | PASS | r2-03 / r2-07 |
| R2-3 | 手机默认折叠：居中动态摘要「媒体库 · 最新更新 · 全部 ▼」 | PASS | r2-03 |
| R2-4 | 点摘要展开六排单行左滑；▲ 可收回 | PASS | r2-04 / r2-06 |
| R2-5 | 媒体库排尾部 label pill；类型/地区/年份起头 label pill | PASS | r2-04 |
| R2-6 | 每排独立横滑，label pill 可滑出屏外 | PASS | r2-05 |
| R2-7 | 摘要动态拼筛选（选电影后「…· 电影 ▼」）+ 网格过滤 | PASS | r2-06 |
| R2-8 | 本地分区：query 非空并入标题匹配，本地徽标+时长角标 | PASS | r2-08 |
| R2-9 | 本地卡→DetailActivity 既有契约（kivo.media/durationMs） | PASS | r2-09 + dumpsys `topResumedActivity=…ui.detail.DetailActivity` |
| R2-10 | 私密红线：被标记条目不出现在搜索结果 | PASS | r2-10（标记 1000051203 后首卡由 1793796391003 变为 1791205455842） |
| R2-11 | 未授权不静默：分区显示非空 reason | PASS | r2-11（红字「本地未授权视频访问…」） |
| R2-12 | 宽屏默认展开 FlowRow 六排 + 不限 chip + 4 列、无摘要行 | PASS | r2-12 |

## 二、复算命令

```bash
adb -s b6e950ee shell content query --uri content://media/external/video/media \
  --projection _display_name | grep -c wx_camera        # 99 → 搜索上限 50，故显示 50
adb -s b6e950ee shell wm density                        # 验毕 440
adb -s b6e950ee shell settings get secure default_input_method   # 验毕 com.iflytek.inputmethod.miui/.FlyIME
adb -s b6e950ee shell run-as com.kivo.cinema ls files | grep -c private.json   # 0（测试标记已删，原状即不存在）
adb -s b6e950ee shell dumpsys package com.kivo.cinema | grep -m1 READ_MEDIA_VIDEO   # granted=true（revoke 后已 grant 恢复）
```

私密排除复算：`run-as … sh -c 'echo ["<mediaId>"] > files/private.json'` → force-stop → 搜索该标题 →
条目缺席；删文件 + force-stop 恢复。本轮用 id `1000051203`（wx_camera_1793796391003.mp4）。

## 三、披露（测量坑与自引入缺陷）

1. **滚动锚定缺陷（自引入，已修）**：展开时六排作为网格首项插入，LazyVerticalGrid 按 key 锚定
   原首可见项 → 新排落在视口外（r2-04b：摘要已 ▲ 但六排不可见）。修复：展开回调里
   `scope.launch { grid.scrollToItem(0) }`（AggregateSearchScreen.kt）。修后 r2-04 正常。
2. **第一次私密排除取证无效**：force-stop 后仅等 5s，会话仍在「正在接入资源库…」，
   搜索页未组合、输入落空（该废帧不入库）。加长等待重测得 r2-10。教训：连接态闸门要先过再取证。
3. **计数被上限掩蔽**：本地匹配上限 50；MediaStore 有 99 个 wx_camera，标记 1 个后仍显示 50。
   排除判定用「被标记条目缺席 + 首卡移位」，不用计数差。
4. 宽屏 = density 200 _override_，非平板；r1-09/r1-10/r2-12 同此披露。
5. 测试期临时改动均已恢复：private.json 删除（原不存在）、READ_MEDIA_VIDEO revoke→grant、
   IME LatinIME→FlyIME、density 200→440。
6. strm 片源诚实显示（第一轮探针）：MediaSources Container=strm、Size=0、无 Video MediaStreams →
   详情页「媒体信息未提供」/「未知」，不造假 0×0/0KB（EmbyResource.kt / DetailResource.kt）。
7. **既有超标披露**：`MainActivity.kt` 基线（253 收口时）已 350 行 > 300 闸；本卡仅 +3 行入口接线，
   不新增超标文件（新文件最大 172 行）。主页 Shell 拆分另立_follow-up_ 卡，不在本卡扩_scope_。