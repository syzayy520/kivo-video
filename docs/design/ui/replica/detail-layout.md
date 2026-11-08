# R2 · 系统栏与字号布局

2026-11-09，基线 `003aa3d6`。本轮先修真机可复现的布局边界，不重调已认可海报、色场或动效。

## 构思与验收

深滚时详情内容进入系统状态栏，和时间/电量重叠。列表视口应从真实状态栏下缘开始，海报仍延伸到屏幕顶。减去首项相同的上边距和最小高度，保持默认字号的标题与后续章节原坐标；不改变媒体滚动量公式。返回按钮沿用已有位置。

将手机字号临时设为1.5复测并在 finally 恢复原值。分类当前用不换行 Row，指南卡固定50dp，可能挤出屏幕或裁文字；只有保存前后证据后修复。分类改为自动换行，指南改为最小高度+按内容增长，默认卡高保留。长片名/缺图先盘点，不以代码推断当通过，不用假线上影片覆盖用户数据。

## 自然家族树

```text
android/app/src/main/java/com/kivo/cinema/ui/detail/
  MovieDetailActivity.kt         列表视口与首项坐标补偿
  DetailSections.kt             分类换行与指南自适应高度
tools/reference/rex/
  detail_layout_device.py        隔离证据目录，复用导航取证
  verify_detail_layout.py        离线节点/系统栏/换行复算
docs/evidence/p240/detail-layout/
  android/                      真机默认/大字前后截图、XML和栈
  operations.jsonl              原字号、临时修改与精确恢复记录
  status.md                     结果、未验与复现
  verification.json             离线复算结果
  source.patch                  本轮生产代码差异
  manifest.json                 来源、源码/APK/证据哈希
```

构建固定签名APK并只覆盖安装，默认与1.5字号均验证；不卸载、清数据、不动媒体或后端。R2保持DOING，不将布局阶段说成全页像素或无障碍验收。
