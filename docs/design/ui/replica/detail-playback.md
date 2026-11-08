# R2 · 播放与外部预告入口

2026-11-09，起点 `2400ac26`。延续详情交互回归，不重构播放器或已认可媒体分层。

## 构思与流程

详情主按钮写“播放”，现却先进入旧 `DetailActivity`，用户要第二次点击“直接播放”。先正常首页进入有匹配资源的影片，保存旧路径证据；再改为直达已有 `PlayerComposeActivity`，同一资源路径传递不变。连续点击只进一个播放器，退出恢复详情节点/位置。片源使用手机已有的、前轮建立的测试资源，必须明确它是测试视频而非对应商业影片。

无资源按钮仍显示原因，不创建播放器；外部预告明确 YouTube，只有点击才打开。系统找不到外部应用时保留可理解错误；浏览器打开不等于视频成功播放，外部网络失败不冒称由 Kivo 检测。默认首页图片和用户预告设置不改。

## 自然家族树

```text
android/app/src/main/java/com/kivo/cinema/ui/detail/
  MovieDetailActivity.kt          仅修改播放导航目标，不改会话/解码/Surface
tools/reference/rex/
  detail_playback_device.py       独立证据目录，复用导航取证，不改旧证据
  verify_detail_playback.py       只读复算保存的节点、栈与操作断言
docs/evidence/p240/detail-playback/
  android/                       本轮截图/XML/栈
  operations.jsonl               操作与断言，失败记录保留
  playback.log                   过滤后的本次启动/路由日志
  status.md                      结果、未验范围及复现步骤
  manifest.json                  证据/生产源码/APK 哈希
  source.patch                   基线上的生产代码改动
```

## 验收边界

生产变化后构建、固定签名覆盖安装。真机证明主播放入口不再经过旧详情、只有一个播放器、实际使用匹配的测试资源；截图与 Activity 栈说明画面和导航事实，本轮运行日志抓取为空，不作证据。后台服务会话生命周期尚未验证，不擅自改后端。不卸载、不清数据、不覆盖已有片源、不输出私人影片清单或网络密钥。

R2 保持 DOING；外部预告成功播放、字号/辅助功能、全页像素和帧时性能分别登记，不把入口回归当作全卡完成。
