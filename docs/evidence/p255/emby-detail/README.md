# 255 Emby 详情 · 选季控件 REX 复刻 + 图优先级 真机取证

设备：Redmi K40 `b6e950ee`（1080×2400，density 2.75）。模拟器数字无效，本目录全部真机截屏。
凭据（服务器账号/密码、TMDB key）一律运行时持有，**不入库**；截屏不含凭据。

## 复算路径

资源库 → emby.bangumi.ca 卡 → 服务器分类行 → 剧海报 → 详情。
浏览页 FLAG_SECURE 截屏为 0 字节，坐标用 `uiautomator dump` 取 clickable 节点 bounds。
详情加载含 TMDB+季集+图，首屏后等 ≥15s 再截。

## 逐帧判据

| 文件 | 判据 |
| --- | --- |
| detail-library-backdrop-tmdb-badge.png | 清潭国际高中：库横版剧照做幕布（库优先）、TMDB 8.2 徽章（search/tv 修复）、粗体左对齐「第 1 季 ⌄」无胶囊 |
| detail-library-art-trigger-chevron.png | 摩登家庭：库白色海报+logo、TMDB 7.9、多季触发器带自绘 ⌄、集缩略图为库真图 |
| season-popup-flip-glass.png | 弹层底部空间不足→翻转到触发器上方；玻璃卡贴合内容（2 行≈104dp）；真背景模糊（logo/简介在玻璃后糊成色块）；勾在行左、无分隔线、背景不压暗；圆角 20、宽 250dp、行高 42dp；触发器完整可见且 ⌃ 朝上 |
| season-switch-season2.png | 点「第 2 季」：弹层关闭、触发器改「第 2 季」⌄ 复位、集卡换第二季内容、首集白描边 |
| episode-stills-tmdb-fallback-cast.png | 谷雨：库无集缩略图→TMDB 剧照兜底（真剧照，非占位）；4 行元数据=日期/第N集+分钟/粗体标题/两行简介；演职员 TMDB 头像齐 |

## 实测规格（对照 REX-EMBY 0178/0179，390pt 基准）

- 触发器：节标题粗体+自绘 chevron，左对齐，无胶囊底；单季不显示 chevron（谷雨帧）。
- 弹层：宽 250dp≈688px、行高 42dp≈115px、圆角 20dp；勾心≈+26dp、文本左缘≈+45dp；
  模糊半径 48（API31+ RenderEffect，复用首页 NavigationBackdrop 捕获桥，同窗 overlay 非 Popup）；
  高度=min(行数×42+20, 336)dp，超出滚动；底部放不下翻转到锚点上方，展开原点随翻转改左下。
- 修过的两个自引入缺陷：① Box 内 fillMaxSize 玻璃层把卡撑到全屏高→显式 .height(cardH)；
  ② 只 clamp 不翻转→弹层盖触发器→flip 逻辑。

## 图优先级（用户拍板）

海报/幕布/logo：库（Emby）优先，服务器缺图才 TMDB 兜底；集缩略图同口径（TMDB /tv/{id}/season/{n} stills）。
演职员：TMDB 命中用 TMDB 头像，否则 Emby People 图。token 按域路由：本服务器走 X-Emby-Token 头，TMDB 域不带 token。