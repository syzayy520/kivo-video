# 首页接管真机记录 · 2026-11-08

本轮已修复顶部视频条、透明logo缓存、异步图片串页、首页加载后错位、箭头挤压，
并实现用户选择的预告片、实时趋势海报、下拉放大和浅淡取色。**全页像素级验收仍未通过，
不能把本记录当作完全一致证明。** 当前安装包已覆盖安装到 Redmi K40，没有卸载或清数据。

## 构建与设备

- 基线提交：2aaeccf；分支 codex/kivo-video-p0-ui-01-playback-overlay-v13。
- `:app:assembleDebug --no-daemon` 成功；原有仓库固定签名；`adb install -r -t` Success。
- 主机 JDK17，以进程参数 `-Djdk.net.unixdomain.tmpdir=C:/Android/no-unix-sockets` 绕开本机AF_UNIX问题。
- Redmi K40 / b6e950ee / 1080×2400 / density 2.75；最终进程30280。
- 前序桌面p12c相关5/5通过；本轮不改后端、JNI或本地播放器Surface。

## 可核对的结果

| 行为 | 真机证据与结论 |
| --- | --- |
| 默认图片 | [默认画面](default-image-final.png)、[最终偏好](preview-default-off.xml)：false；关闭后等待期间没有新增预告片事件。代码在关闭时不组合HeroTrailer，因此不请求/videos或创建YouTube播放器 |
| 用户开启 | 设置→首页→首页预告片自动播放；[设置关闭截图](settings-off.png)。用户开启后持久保存，网络需能访问YouTube |
| 整幅预告片 | [58秒录屏](preview-final.mp4)、[播放截图](preview-playing.png)；逐秒抽帧检查画面已覆盖封面，上下不再拼静态海报。连续录屏之后进程仍30280 |
| 海报延迟 | [事件](preview-events.jsonl)：90014097 poster_visible，90016098 preview_due，90020208 playing；两秒开始加载，实际画面因网络约6.111秒出现，不能宣传固定两秒出画 |
| 视频尺寸 | layout事件：原生1080×1609，文档/视窗392×585 CSS px，iframe覆盖1404.21×789.87并中心裁切；MATCH_PARENT修复零高度vh，fixed容器阻止溢出扩大视窗 |
| 横滑与释放 | 第二部1368337实际playing；90217866离开tab释放。90468627切后台释放；回前台重新播放；90507761上滚使hero超出可播范围后释放 |
| 崩溃回归 | [exit-info](exit-info.txt)保留之前失败记录；最新native crash仍是修复前16:26:45。修复后本次播放、横滑、后台、滚动保持同一进程；不代表长稳或多机型验收 |
| 下拉与回弹 | [静态录屏](static-final.mp4)、[拉住](pull-held.png)、[回弹](pull-released.png)；static-start与pull-released在x=0…1079/y=120…1599的逐通道绝对差均值为0，见[metrics](metrics.json) |
| 浅淡背景 | 海报下15%在线性光取色，混入28%中性页面色，再随滚动过渡页面底色；[中段](scroll-mid.png)、[深处](scroll-deep.png)。文件名不宣称精确滚动距离 |
| 箭头及功能 | [默认画面](default-image-final.png)箭头完整；点击进入[在看](watching-arrow.png)；[资源库入口](library-regression.png)可达；海报卡打开[真实信息](title-sheet.png) |
| 动态数据 | [当次候选](home-feed.json)来自/trending/all/week，hero取前10，回前台刷新；不固定影片ID，也不承诺与REX私有排序完全相同 |

## 视觉比较与边界

[原版/本地并排](comparison.png)：原始Display P3先转sRGB；两侧均统一宽390、按原比例缩放，
原版高844，本地高867，未单独压缩纵向。海报、logo与文字锚点按390pt参照校准，
Hero585dp、logo中心402dp、信息472dp、评分522dp、圆点574.5dp。

用户要求背景更浅，因此本轮底色有意偏离原版暗色。系统状态栏、原生字体渲染、
继续观看内容、实时影片列表、缺来源评分不作全屏逐像素比较；只展示真实TMDB评分。
逆向材料只证明配置项存在，未提供可信的两秒延迟或下拉公式；下拉阻尼、回弹、
视频1.35倍裁切为本轮实现参数。不同影片黑边与构图仍需更多素材校准。
YouTube偶有短暂平台播放提示，片源禁嵌/超时/断网的错误提示已实现，最终轮未逐项注入验证。

复算回弹（仓库根）：

```python
from PIL import Image
import numpy as np
from pathlib import Path
p = Path('docs/evidence/p237/home-takeover')
a = np.array(Image.open(p/'static-start.png').convert('RGB'), dtype=float)
b = np.array(Image.open(p/'pull-released.png').convert('RGB'), dtype=float)
print(np.abs(a[120:1600] - b[120:1600]).mean())  # 0.0
```

抽帧复查：`python tools/home/analyze_recording.py docs/evidence/p237/home-takeover/preview-final.mp4 build/preview-check --duration 58`。
设计、家族树与已证伪结论见 `docs/design/backend-core/home/2026-11-08-takeover.md`。
