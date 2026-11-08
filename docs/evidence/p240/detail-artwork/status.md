# R2 · 片名与缺图阶段

2026-11-09，基线513239ab，Redmi K40 b6e950ee。阶段通过，R2仍DOING。

## 改动与证据

- TitleLayer保持成功logo的固定88dp带；没有logo、空logo地址或加载失败时渲染完整文字，最小88dp、自然增长，不再两行截断。状态按URL/片名重建。
- DetailBackdrop缺图/失败时使用现有色场，显示“暂无封面”/“封面暂不可用”。成功图片的缩放和渐变参数没有改变，片名仍在内容层。
- before/、after/各7张真机组件截图及report.json，覆盖短/长片名、失败logo、成功logo、局部1.5字体环境、缺封面与失败封面。旧版长标题实际截断、失败logo空白；新版长标题显示完整。片名带242→496px，元信息自然下移；短片名与成功logo几何完全一致。
- 正常首页进入《生化危机：爆发夜》详情：android/real-detail-top.png及XML。片名、日期、播放、标记、收藏、更多和展开简介坐标与上一阶段完全一致。
- 固定签名覆盖安装主包和测试包成功；未卸载、未清数据。build.log为生产包与探针构建，build-probe.log为最终探针构建。

复算：`python -X utf8 tools/reference/rex/verify_detail_artwork.py`，25/25。验证结果见verification.json；manifest.json登记归档与源码SHA-256。

## 方法与限制

探针只在androidTest中构造隔离内容，临时替换MainActivity的composition；不向生产片单加ID特例、不改用户配置/线上数据。成功logo使用进程内合成位图；非法fixture URL经真实加载器返回失败，不访问远程服务器。验证的是加载失败分支，不代表HTTP错误、慢网络、真实多影片完整矩阵已测。

HyperOS阻止runner从后台启动界面，导出脚本显式通过am start将MainActivity带到前台；测试结束恢复正常界面。最初额外manifest入口未注册，改为可选Gradle属性`kivo.testRunner`，默认FilmMarksInstrumentation保持不变。构建探针使用`-Pkivo.testRunner=com.kivo.cinema.ui.detail.DetailArtworkInstrumentation`。

无障碍节点存在缓存延迟：before节点仅留作原始记录，不用于语义断言；最终探针逐节点refresh，after长片名/失败logo语义断言通过。首例短标题节点为空，因此只用截图和布局几何验证。缺图提示位于媒体背景层，被上方透明内容层从无障碍树遮蔽，使用截图区域明度/文字墨迹及人工查看证明视觉可见，不宣称无障碍通过。clearCache在该机framework不存在，已改用兼容的refresh。

1.5使用局部Density环境，未修改系统字号；本轮未证明系统大字号下全页布局或实际字号比例。成功logo与失败logo分别重建组件；同一composition连续切换URL的运行时回归仍未测。正常影片只验默认首屏，没有重新验证全套滚动动效、预告成功播放或性能。没有宣称整页像素级完成。

## 下一步

R2剩余：预告成功播放、真实多影片/系统大字及完整异常矩阵。R3随后检查首页feed更新、缓存与预告生命周期。

用户新增测试源：小米路由器SMB地址192.168.5.1，免用户名和密码，内有可用于测试的视频；已授权读取测试。尚未枚举共享、未验证连接或播放，不把地址当成已播放证据。后续走已有资源库/SMB路径，不改后端架构。
