# 225 · 下载管理 —— 设计稿

> 状态：**设计完成，实现开工**。M2 第五卡。
> 卡片原文：DownloadService 参照 + 后台下载。
> 参照：SenPlayer `DownloadClient + DownloadService + DownloadJobService`（03 拆解）；
> Rex `download` 模块。依赖：222 媒体库 ✓（下载完成 → 出现在媒体库）。

---

## 一、一句话目标

**从 WebDAV/SMB 远端源下载视频到本地 → 出现在媒体库 → 断网/杀进程不丢——
SenPlayer 同款"先下载后离线看"。**

## 二、设计决策

### D1 下载引擎：OkHttp + CoroutineWorker

- OkHttp 下载（Range 续传 + 进度回调）——不走 MediaDownloader（框架太重）；
- CoroutineWorker（WorkManager）后台执行——杀进程恢复（WorkManager 持久化）；
- 下载到 `getExternalFilesDir("downloads")/` → 媒体库自动可见（同目录扫描）。

### D2 前台服务：DownloadService

- Foreground Service + 通知（进度条）——对齐 SenPlayer `DownloadService`；
- 下载完成 → 通知点击跳转媒体库。

### D3 UI：SmbActivity 浏览行加"下载"按钮

- 文件行（非目录）加下载图标 → 点击下载 → 通知栏进度 → 完成出现在媒体库。

## 三、家族树

| 文件 | 新/旧 | 单一职责 |
| --- | --- | --- |
| `download/DownloadEntry.kt` | **新** | 下载任务数据类（url/localPath/title/progress/state） |
| `download/DownloadWorker.kt` | **新** | OkHttp 下载 + Range 续传 + 进度 Flow |
| `download/DownloadService.kt` | **新** | Foreground Service + 通知进度 |
| `smb/SmbActivity.kt` | 旧（小改） | 文件行加下载按钮 |
| `AndroidManifest.xml` | 旧（小改） | DownloadService + POST_NOTIFICATIONS 权限 |

## 四、验收判据

| # | 判据 | 通过条件 |
| --- | --- | --- |
| AC1 | 下载 | WebDAV/Windows 共享 33MB 文件下载到 downloads/ ✓ |
| AC2 | 进度 | 通知栏显示进度百分比 ✓ |
| AC3 | 媒体库 | 下载完成 → 文件出现在本地媒体库 ✓ |
| AC4 | 续传 | 中断后重新下载从断点继续（Range header）✓ |

## 五、明确不做

批量下载/队列管理 · 下载速度限制 · Wi-Fi only 策略 · 自动删除。
