# R2 详情布局阶段 · 2026-11-09

基线 `003aa3d6`，Redmi K40 `b6e950ee`，1080×2400。本轮仅改详情列表视口、分类流式排布与指南卡高度；R2仍DOING。

## 真机结果

- 默认首屏片名logo、信息行、按钮、简介和评分等可见内容的语义坐标逐项一致：`android/before-default-top.*` 与 `after-default-top.*`。恢复字号后的 `after-restored-top.*` 再次一致；不将坐标一致称为全页像素一致。
- 系统状态栏实测高80px，来源 `system-insets.txt`。旧版深滚正文进入该区域；新版列表从80px开始裁切，海报背景仍延伸至屏幕顶：`before-default-scroll.*` / `after-default-scroll.*`。两次手势滚动落点不同，不比较两张深滚图的绝对纵坐标。
- 旧版指南卡固定50dp，1.5倍字号下第二行高度仅48px，底部被裁。新版按内容增长，两个指南值均为73px完整行高，默认字号文字尺寸保持：`before-large-genres-full.*` / `after-large-guide-genres.*`。截图已人工检查；横向长内容沿用单行省略，未称全部文字显示完整。
- 分类改为FlowRow。1.5倍下本片三个短分类仍能排在一行；额外3倍字号压力检查时，第三个分类落到第二行，三个标签均在可视区域内：`after-stress-genres-wrap.*`。3倍测试只验分类换行，不代表全页支持3倍字号。
- 两次测试均在finally恢复系统字号至原1.0，`operations.jsonl`有原值及回读。字体设置变化会重建详情并回顶，这是测试中观察到的现有行为，未修复。
- 固定仓库签名构建成功并覆盖安装，`build.log` / `install.log`；未卸载、清数据、改后端/JNI/Surface或测试媒体。误用通用launcher时选中了仓库旧诊断播放器，随后停止应用并显式启动MainActivity，正式截图均从正常首页进入本周趋势详情取得；旧诊断入口未作生产修改。

## 复现与边界

`python tools/reference/rex/verify_detail_layout.py` 独立读取保存的XML与操作记录，结果存于 `verification.json`。源码差异 `source.patch`；来源及文件哈希 `manifest.json`。取证工具 `tools/reference/rex/detail_layout_device.py` 复用已有ADB工具，不改应用数据。

`before-large-genres`命名不准确，截图仍在上半页，不作为分类证据；实际旧分类使用带`-full`的记录。长片名、缺logo、图片加载失败、不同屏宽、系统栏可读性的所有色场、旋转、字体重建后滚动恢复、预告成功播放、帧时和全页像素仍未验。本轮保留已认可海报、片名独立层与媒体动效参数。
