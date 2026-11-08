# R2 · 片名与缺图边界

2026-11-09，基线513239ab。先用真机隔离组件探针复现，再修复；不更换线上影片数据或删除缓存。

## 构思与验收

TitleLayer目前在logo非null时只渲染图片，失败后无片名；无logo文本受88dp和两行限制。成功logo继续保持88dp及原坐标；无logo或加载失败使用可增长的最小88dp文字带，片名完整换行，后续信息自然下移。失败状态按URL和片名重置，切片不能带入旧失败状态。

DetailBackdrop目前无图/失败时保留黑底。缺图改为当前色场承接并显示简短封面提示，正常图的缩放、渐变与独立片名层不动。缺图不是播放能力失败，不引入网络重试框或后端改动。

## 自然家族树

```text
android/app/src/main/java/com/kivo/cinema/ui/detail/
  DetailHeroContent.kt            logo失败文字回退、文字带自然增长
  DetailBackdrop.kt               缺封面色场及可见提示
android/app/build.gradle.kts      可选kivo.testRunner属性；默认标记runner保持
android/app/src/androidTest/
  java/com/kivo/cinema/ui/detail/DetailArtworkInstrumentation.kt
                                 复用生产组件，构造明确标识的测试内容
tools/reference/rex/
  detail_artwork_device.py        构建前后探针证据导出与哈希，不输出用户资料
  verify_detail_artwork.py        离线布局/语义证据复算
docs/evidence/p240/detail-artwork/
  before/ after/                  真机组件截图、几何和可访问节点
  status.md manifest.json         范围、限制、来源
```

探针使用MainActivity临时替换composition，不改生产入口或加影片ID特例；测试模型只存在androidTest。用非法测试URL触发真实加载器失败，不写磁盘缓存；成功logo使用进程内合成位图。测试只证明组件边界，正常首页进入真实详情另抓一组首屏回归。字体测试通过局部Density注入1.5，不改系统字号。覆盖安装固定签名主包和测试包，不卸载或清数据。原默认runner保留。
