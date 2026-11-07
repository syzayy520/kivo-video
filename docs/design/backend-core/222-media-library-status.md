# 222 · 本地媒体库 —— 状态稿（v1 收口）

> 状态：**v1 收口 2026-11-08**。K40 真机一次通过。

## 一、交付面

| 组件 | 内容 |
| --- | --- |
| MediaLibraryActivity | LAUNCHER 入口 + MediaStore 扫描 + LazyVerticalGrid(3列) 海报墙 |
| MediaItem | 数据类（id/title/path/durationMs/width/height） |
| 权限 | READ_MEDIA_VIDEO（API 33+）/ READ_EXTERNAL_STORAGE（旧版） |

## 二、真机实证

- K40 扫描 **157 个本地视频** → 海报墙正确渲染（缩略图/时长角标/文件名）
- 滚动流畅（LazyGrid 懒加载）✓
- 点击海报 → PlayerComposeActivity 起播 ✓
- Rex 规格：3 列 2:3 卡片、时长角标黑底白字、标题 12sp 单行截断
- SMB 绿按钮入口 → SmbActivity（远端源合一入口）

## 三、已知问题

| # | 问题 | 后续 |
| --- | --- | --- |
| 1 | 仅扁平海报墙，无文件夹分组视图 | 后续卡 |
| 2 | 缩略图用 MediaStore 缓存（MINI_KIND ~512px） | 215 ThumbnailReader 高清版可选 |
| 3 | SmbActivity 非 LAUNCHER（从媒体库 SMB 按钮进入） | 符合设计 |
