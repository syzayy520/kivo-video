# M1 首页切换与详情媒体动效

2026-11-08。源码基点 `90561977`（已认可基线正式集成）。本卡静态媒体动效范围已完成 K40 回归；不代表 R1 全场景、R3 生命周期或全页像素验收完成。

## 改动与来源

- 首页：图片与片名前景分别跟手位移；相邻海报预组合；环境色、页点连续变化；松手无过冲吸附。取消拖动反向恢复。视差、渐隐和 spring 数值是 Kivo 设计值，不能称为 REX 源码参数。
- 详情：只有媒体随上滑中心缩放，去掉旧的额外上移；片名、按钮和渐变前景保持独立，向上覆盖媒体。回顶与下拉释放恢复原尺寸。
- 加载：图片查询与 logo 查询分开，尚无 logo URL 时显示文本片名；无 feed 时显示加载或失败信息，避免只留空占位。
- 默认图片；预告仍需用户开启。本轮没有改动态 feed 来源，没有写死片单，也不保证上游每天必然更换影片。

设计与家族树：[首页切换](../../../design/ui/replica/hero-transition.md)、[详情缩放](../../../design/ui/replica/detail-media-motion.md)。[播放器观察](../../../design/ui/replica/player-study.md) 记录六款应用的实际观察与取舍，Forward 正常内容、Noctra 详情仍有资料缺口。

## 构建、安装与复现

Redmi K40 `b6e950ee`，1080×2400px，density 2.75。最终构建 [build.log](build.log) 为 `BUILD SUCCESSFUL in 19s`；[install-final.json](install-final.json) 记录固定签名 `adb install -r -t` 成功及 APK SHA-256。没有卸载或清数据，没有修改后端、JNI、Surface。

```powershell
$env:JAVA_HOME='C:/Android/jdk-17.0.13+11'
$env:JAVA_TOOL_OPTIONS='-Djdk.net.unixdomain.tmpdir=C:/Android/no-unix-sockets'
./android/gradlew.bat -p android :app:assembleDebug --no-daemon
```

[working-tree.patch](working-tree.patch) 仅含本卡四个 Kotlin 文件相对基点的变更，在基点的独立检出目录应用；不要对当前工作区重复应用。[manifest.json](manifest.json) 记录证据原始字节、源码换行归一化哈希、设备与最终 APK 哈希；APK 不入库。

## 真机证据与结果

| 场景 | 证据 | 可支持的结论 |
| --- | --- | --- |
| 首页慢拖、松手、取消、连续快切三页、纵滚、回顶、下拉释放 | [最终录屏](android/home-motion-final.mp4)、[操作时序](home-actions-final.json)、`android/home-*-final.png` | 中间帧媒体和前景分层，没有观察到漏底；松手吸附、反拖取消恢复 |
| 同页取消恢复 | `home-next-final.png` / `home-cancel-final.png` | ROI `[0,120,1080,1800]` RGB MAE=0，变化像素比例=0；仅说明这次静态页该区域恢复一致 |
| 详情分段上滚、回顶、下拉释放 | [详情录屏](android/detail-motion.mp4)、[操作时序](detail-actions.json)、`android/detail-*.png` | 媒体逐步放大，前景大小保持；回顶、释放恢复 |
| 默认预告关闭 | [前态](preview-before.json)、[回归后](preview-off-check.json) | 历史事件188行→188行，新增0；本轮没有观察到预告创建事件 |
| 用户开启、切页、后台恢复、再关闭 | [事件](preview-enabled-check.json)、`android/preview-*.png` | 12个新事件：停稳图片→约2001ms准备→切页/后台释放；最终开关恢复 false |
| 冷启动资源等待 | [时序](startup-timing.json)、`android/startup-final-1.png` 至4 | 约0.86s仍为启动黑屏；1.875s有加载文字/片名；3.469s图片和logo已显示；不是冷启动无黑帧 |

详情截图在首轮安装后取得；后续只修改首页，详情源码与最终安装版一致。无 `final` 后缀的首页截图/录屏及 `install.json` 为早期诊断，不能替代最终回归；`home-top.png` 曾是空白。`home-loading-final.png` 截在 Activity 首帧前，只能说明启动黑屏，不能作为加载提示通过图。

固定纹理配准结果见 [android-registration.json](android-registration.json)、[reference-registration.json](reference-registration.json)，派生摘要见 [verification.json](verification.json)：

| 前景上移 dp | 我方媒体比例 | 前景比例 |
| --- | --- | --- |
| 67.27 | 1.02300 | 1.00001 |
| 133.82 | 1.04574 | 0.99999 |
| 200.73 | 1.06861 | 1.00003 |
| 267.63 | 1.09149 | 1.00000 |
| 回顶 | 1.00001 | 1.00000 |
| 下拉释放 | 1.00004 | 1.00000 |

我方与模型 `1 + clamp(scrollDp,0,350)/2925` 的比例最大残差约 `0.0000376`；这是图像配准与实现模型的一致性，不是全页像素误差。REX 0149→0157 的前景上移约311.67pt、媒体约1.10649倍；0158内点不足，被明确拒绝，未用异常拟合值。

## 复算

需 Python、opencv-python-headless、numpy、Pillow。从仓库根执行；输出写 build，避免覆盖原始档案。

```powershell
$root='docs/evidence/p240/media-motion'
$targets=1..4 | ForEach-Object { "$root/android/detail-scroll-$_.png" }
$targets+=@("$root/android/detail-return.png", "$root/android/detail-pull-released.png")
python tools/reference/rex/measure_scroll.py "$root/android/detail-top.png" @targets --media-roi 140 160 1000 900 --foreground-roi 80 1120 1000 1340 --pixels-per-unit 2.75 --output build/android-motion-recheck.json
$refs=Get-ChildItem "$root/reference" -Filter '*IMG_015*.PNG' | Where-Object {$_.Name -match 'IMG_015[0-8]\.PNG$'} | Sort-Object Name | ForEach-Object FullName
python tools/reference/rex/measure_scroll.py "$root/reference/2026_11_08_19_25_03_IMG_0149.PNG" @refs --media-roi 120 160 1060 900 --foreground-roi 80 1280 1090 1480 --pixels-per-unit 3 --output build/reference-motion-recheck.json
python tools/reference/rex/verify_media_motion.py
```

SIFT与RANSAC阈值、库版本、原始来源路径及每张图SHA保留在配准JSON；低质量结果不按零运动处理。录像720×1600、716帧，容器平均23.868fps仅表示录制文件，不能证明屏幕帧率。固定背景采样仅记录编码视频RGB，不是色彩校准。

## 明确未测与后续

- 预告本轮没有 `playing` 事件，实际 YouTube 成功播放、首帧替换及播放中取消未闭环；不能把准备/释放事件称为播放成功。
- 未采 FrameTimeline/三轮帧时分布，未证明60fps或无掉帧；精确复现原版 spring 参数无证据。
- 启动黑屏仍存在；logo URL 返回后到位图完成之间仍可能短暂透明。
- 还未完成所有亮暗图、无网/部分失败、feed更换、字体放大、全页像素与所有按钮验收。
- 后续按 [roadmap](../../../design/ui/replica/roadmap.md) 进入 R2，先盘点收藏/待看等已有状态服务和对应动作，再设计接线；不因本卡通过而把 R1/R3 整体标 DONE。
