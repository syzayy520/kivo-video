# 222 · 本地媒体库 —— 设计稿

> 状态：**设计完成，实现开工**。M2 第三卡。
> 卡片原文：MediaStore 扫描 + 文件墙（缩略图/文件夹）。
> 参照：SenPlayer `image×7`（文件/缩略图服务）；Rex `library*`（460 键）。
> 依赖：215 缩略图管线 ✓；213 Compose 骨架 ✓。

---

## 一、一句话目标

**应用内"媒体库"页：扫描设备视频 → 海报墙（缩略图+时长）→ 点击起播——
用户首次打开 app 即可浏览并播放本地视频，不再依赖 adb 推路径。**

## 二、设计决策

### D1 数据源：MediaStore（content://media/external/video）

- 系统维护的视频索引：标题/路径/时长/宽高/缩略图——零扫描成本；
- 查询投影：`_ID, DISPLAY_NAME, DURATION, WIDTH, HEIGHT, DATA`；
- 按文件夹分组的文件墙 + 扁平海报墙两种视图（M1 先做扁平）。

### D2 海报墙：LazyVerticalGrid + MediaStore thumbnail

- LazyVerticalGrid(3 列)，卡片 2:3（Rex §4 实测比例）；
- 缩略图：`MediaStore.Video.Thumbnails.getThumbnail()`（系统生成，零成本）；
- 缓存：无（MediaStore 自带缓存）；
- 点击：起播 `PlaybackSession(0, File(path), name)` 走 213 管线。

### D3 入口：app 首页换为媒体库

- SmbActivity 的 LAUNCHER intent-filter 保持（SMB 直连入口）；
- 新增 `MediaLibraryActivity` 为 LAUNCHER——**用户打开 app 首先看到本地视频墙**；
- 媒体库内加"SMB"入口按钮跳转 SmbActivity（远端源合一入口）。

### D4 增量加载

- MediaStore 查询一次全量 → LazyColumn/LazyGrid 懒加载（数千项不卡）；
- 缩略图 `getThumbnail` 按需调（滑到才取）。

## 三、家族树

| 文件 | 新/旧 | 单一职责 |
| --- | --- | --- |
| `library/MediaLibraryActivity.kt` | **新** | MediaStore 查询 + 海报墙 Compose 页 |
| `library/MediaItem.kt` | **新** | 数据类（id/title/path/duration/width/height） |
| `AndroidManifest.xml` | 旧（小改） | LAUNCHER 换 MediaLibraryActivity + READ_MEDIA_VIDEO 权限 |

复用：PlayerComposeActivity 起播（213）、ThumbnailReader（215，备用）。

## 四、验收判据

| # | 判据 | 通过条件 |
| --- | --- | --- |
| AC1 | 扫描 | K40 设备视频 ≥10 条，海报墙正确渲染 |
| AC2 | 播放 | 点任意视频 → 起播 + 画面 + seek ✓ |
| AC3 | 权限 | 首次启动请求 READ_MEDIA_VIDEO → 授权后扫描 |
| AC4 | 性能 | 滚动流畅（LazyColumn 懒加载，不 OOM） |

## 五、明确不做

文件夹分组视图（后续卡）· SMB/WebDAV 源合一浏览（SmbActivity 独立入口）·
搜索/排序（后续卡）· 删除/重命名（225 下载卡）。
