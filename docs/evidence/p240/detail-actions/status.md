# R2 详情动作与底部卡片 · 阶段证据

2026-11-08；起点 `9ec7c3af`；设备 Redmi K40 / `b6e950ee`，1080×2400、density 2.75。R2 保持 **DOING**，本记录只确认下列范围，不代表整页像素验收。

## 结果

- 心形、待看、已观看接入独立的 AtomicFile 本地标记；电影/剧集分型。不改变真实播放统计。
- 用户否定的右侧 DropdownMenu 已废弃。标记与更多统一为浅暖灰底部圆角卡片，线性图标、清晰主次文字，淡紫灰底与勾选表达选中；选择标记立即提交保存，同时收起。更多动作在退场后执行。
- 简介支持展开/收起；空资源的待看入口接同一状态；相似作品导航、加载重试和外部打开的错误反馈已接线，未测范围见后文。
- 最终构建 `build-final.log` 成功（19s）；`sheet-final-install.log` 覆盖安装成功。固定签名，未卸载、未清数据，后端/JNI/Surface 未修改。

## 真机验证

正常入口：首页上滑 → 正在热映 → 荒野丛林 → 标记 / 更多。

| 场景 | 证据 | 结果 |
| --- | --- | --- |
| 收藏、待看、已观看独立添加/取消 | `operations.jsonl` 的逐次 tap、persisted_marks、assert_persisted | PASS；三位互不覆盖，最终恢复本轮测试前的未标记状态 |
| 未选、混合、全选视觉 | `android/final-marks-empty.png`、`marks-sheet-mixed.png`、`final-marks-after-restart.png` | 实机查看无截断，选中有底色与勾选 |
| 强停进程后重新进入 | `force_stop_after_confirmed_write`、`restart_checkboxes`、`android/final-marks-after-restart.{png,xml}` | 两个 checkbox 均 checked=true；存储内容与页面一致 |
| 关闭、系统返回、点遮罩、下拉取消 | `operations.jsonl`、`android/sheet-dismissed.png`、`final-scrim-before-dismiss.png`、`final-scrim-dismissed.png` | 仅收起，不改变已确认标记 |
| 更多卡片与重新加载 | `android/final-more.png`、`final-after-reload.png`、`sheet-device-check.log` | 卡片退场后刷新，详情入口重新可用 |
| 长简介展开 | `android/overview-expanded.png`、`operations.jsonl` | 已实测展开；收起与滚动恢复仍待 R2 完整回归 |
| Android 真实文件系统 | `storage-tests.log` | 5/5 PASS：三态/命名空间/重建读盘；16 个影片交错写入不丢失；真实写失败保留旧值并可重试；损坏与未知 schema 不覆盖 |

`storage-tests.log` 是本轮前段已执行结果，后续仅修改弹层呈现/动作调度和移除无用 import，未重复执行存储测试。测试在独立 cache 临时目录，未破坏生产文件。运行：

```text
adb -s b6e950ee shell am instrument -w com.kivo.cinema.test/com.kivo.cinema.media.marks.FilmMarksInstrumentation
```

结果以输出 `5/5 PASS` 为准。UI 抓图与动作可用 `tools/reference/rex/detail_actions_device.py` 复现；先等首页/弹层真正出现，再进行手势或杀进程；持久化先读盘确认后再强停。

## 保留的失败和方法修正

- `storage-tests-attempt1.log`：初版测试用 `.new` 目录注入写失败，但 AtomicFile 读盘会清理该目录；不是有效失败注入。改为独立测试目录 chmod 0500 后，真实写失败测试通过。
- 初版卡片把保存安排在退场后；一次快速读盘与强停发生在保存之前。最终改成点击立即提交，与退场并行，重启验证在读盘确认后执行。旧 `marks-sheet-after-restart.png` / `restart-current.png` 实为首页，不作状态恢复通过证据。
- `sheet-device-attempt1.log`：测试在打开卡片后立刻点击遮罩，点击早于弹层可交互阶段，断言失败。补充 `final-scrim-before-dismiss` 确认弹层已出现后再点，最终通过。旧同名 `final-scrim-dismissed` 已由最终有效抓图覆盖；操作时间以 JSONL 为准。
- 未用 USB 分段截图推断动画精确时长或性能。采用 Material3 ModalBottomSheet 实际进出/手势行为，参数属于 Kivo 设计。

## 未验范围

相似作品快点去重/返回原滚动位置、剧集 UI、真实加载失败/重试、外部 TMDB/YouTube 成功打开与播放、本地资源播放返回、1.3× 字号/TalkBack、多机型、连续帧动效性能、完整像素差异仍待后续验证。当前 LocalVideoSource.query 自身会将读取异常转为空列表，不能把无匹配资源等同于所有查询失败均有准确原因。R2 不标 DONE。

最终主/测试 APK 路径与 SHA-256、证据文件哈希见 `manifest.json`；源码差异见 `source.patch`（基于上述起点，不含无关脏文件）。截图裁切 `card-preview.png` 仅用于展示，原始 1080×2400 截图保留。
