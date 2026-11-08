# TMDB 手机直连排查

2026-11-09，Redmi K40 / b6e950ee。用户确认手机浏览器关闭代理可打开TMDB；尚未确认浏览器访问的是网页、API还是图片同一路径，不将网页成功等同API成功。

发现应用私有配置`files/tmdb_proxy.txt`指向`127.0.0.1:10808`，但 `adb -s b6e950ee reverse --list` 无转发记录。已原位改名为`files/tmdb_proxy.txt.disabled-20261109`备份，未删除key/缓存/账号；停用后旧API域仍超时，说明残留代理不是唯一因素。

显式诊断命令（不走配置代理）：

```powershell
adb -s b6e950ee shell am instrument -w com.kivo.cinema.test/com.kivo.cinema.tmdb.TmdbConnectivityInstrumentation
```

第一版诊断的一轮输出（后续扩充结果见文末）：

```text
api.themoviedb.org DNS=111.243.214.169
api.themoviedb.org direct SocketTimeoutException elapsedMs=6007
api.tmdb.org DNS=143.204.160.79, 143.204.160.25, 143.204.160.10, 143.204.160.124
api.tmdb.org direct HTTP=401 elapsedMs=802
image.tmdb.org DNS=169.150.249.164
image.tmdb.org direct SocketException elapsedMs=324
API authenticated HTTP=200
API resultCount=20
Actual poster SocketException
```

之前一轮长API域解析为114.43.24.59、超时；短API域401，图片域同样SocketException。DNS结果在本次网络下变化，尚未对解析器/浏览器安全DNS作因果验证，不把ping或解析结果单独当成HTTPS结论。

首页、详情、评分改用共享`TmdbClient.API_BASE=https://api.tmdb.org/3`，不硬编码IP、不关闭TLS证书校验、不降到HTTP。手机同包覆盖安装后首页从超时错误变为真实影片元数据；独立认证请求得到20条结果。诊断未输出key或用户凭据。

**边界：API直连在当轮通过，图片直连未修复。** 首页已有磁盘/内存图片缓存，不得凭首屏出现海报推定新图下载成功。本轮没有使用代理转发，也未宣称全地区/全网络可达；图片链路继续DOING。构建与手机/平板覆盖安装均成功，本轮未修改播放后端。

## 后续图片探测与最终包

增加图片短/网页媒体域对照后，一轮`image.tmdb.org`真实海报HTTP200且解码true，`media.themoviedb.org`同样200且解码true；无认证的长API域仍超时。由此只能判图片链路间歇失败，不能说它持续不可达。生产图片加载器增加一次IO失败重试（上限2次），404和解码失败不重试。

最终包构建通过、手机和平板覆盖安装Success。增加w154未用于首页的尺寸验证生产加载器，最终结果见[原始脱敏报告](phone-direct-report.txt)：API200/20条，图片两域均SocketException，`Production poster loader decoded=false`。因此重试并未在本轮解决当前网络的图片失败，不将该分支标绿，也不重复探测直到拿到绿。

ADB打开手机浏览器访问图片做同路径对照，被自动审批以`blocked by policy`拦截，未执行；不据此推测浏览器的DNS/TLS行为。尚未取得系统DNS与浏览器差异的因果证据。

最终APK SHA256另见[构建摘要](build-artifact.txt)。修复边界：失效调试代理已停用、API使用短域当轮稳定；图片根因仍待查。
