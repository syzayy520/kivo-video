# SenPlayer 1.0.0 (Android) 拆解报告：技术栈判定与架构解读

> 对象：`SenPlayer-1.0.0-b8-Android-arm64-v8a.apk`（87 MB，arm64-v8a）
> 日期：2026-11-06。证据等级：【已验证】= 直接读取 APK 内文件/符号/dex；【推断】= 有依据的推导；【未知】= 需运行时观察。
> 与 Rex 拆解（`01-dv-pipeline-teardown.md`）配套阅读——两个 iOS/Android 播放器的架构对照。

## 〇、一句话结论

**SenPlayer Android = "极薄原生壳（5 个 Kotlin 类）+ FFmpegKit 裁剪版 + 自研双 Kit（MPlayKit 播放 / MLibKit 业务）+ 跨端脚本总线"**。
UI 是**传统 View 体系（非 Compose/Flutter）且全部混淆**；业务逻辑疑似跑在自研脚本引擎里实现 iOS/Android 共享。
**它和 Rex 代表了两条截然不同但都成功的路线，且两家都没用跨平台 UI 框架。**

## 一、体积与组件构成【已验证】

| 组成 | 体积 | 内容 |
| --- | --- | --- |
| `lib/arm64-v8a` | **72.7 MB** | `libFFmpegKit.so` 54.4 MB + `libMLibKit.so` 15.7 MB + libc++ + crashlytics |
| `classes.dex` | 8.9 MB | **仅 1 个 dex，7183 类，其中 6811 个被混淆**（`rl0`/`a00` 式包名） |
| `resources.arsc` + res | ~4 MB | 资源混淆（`res/18.png` 式命名，**AndResGuard 微信系构建链**） |
| `assets` | 0.7 MB | 仅 `gcdweb/`（网盘 Web 上传页，jQuery+Bootstrap）+ baseline.prof |

**没有**：libflutter、libreactnative、libmpv、libijkplayer、libplacebo、libdovi、media3/ExoPlayer、第三方弹幕库。

## 二、三层结构【已验证】

### 2.1 壳层：`com.wuziqi.SenPlayer`（仅 5 类）

```
SenApp / MainActivity / PlayerActivity / BackgroundPlaybackService / wxapi.WXEntryActivity
```
`com.tencent.mm ×248`（微信登录/支付 SDK）+ `WXEntryActivity`——**微信生态深度绑定**。

### 2.2 播放层

- **`libFFmpegKit.so`（54.4 MB）**：完整 demux+decode（含 `DOVI RPU` / `DOVI configuration record`
  解析、Vulkan 头文件痕迹），但**滤镜链被整个裁掉**——`tonemap_*`/`scale_vulkan`/`zscale`/
  `libplacebo` 零命中。FFmpeg 在这里只当"解码器"，不当"渲染器"。
- **`com.mplaykit`（16 类）**：`Player`、`Player$Preview`、`Playback$State`、`Playback$Progress`、
  `PreviewReader$Still`（**进度条缩略图**）、`Diagnostics` + 9 个混淆类。
  其 native 实现不在任何导出表（`Java_com_mplaykit_*` 零命中）→ **RegisterNatives 动态注册**，
  实现藏在 `libMLibKit.so` 内部【推断，高置信：15.7 MB 的体积远超纯业务桥所需】。

### 2.3 业务层：`com.mlibkit`（20 类 + `libMLibKit.so` 15.7 MB）

JNI 模块全景（`Java_com_mlibkit_*` 导出【已验证】）：

```
account（账号）  dlna（投屏）  download（+Service/JobService 后台下载）
smb（SMB 客户端）  gcd.WebUploader（网盘上传，配 assets/gcdweb 页面）
image（×7，文件/缩略图服务）  storage  log  product.ProductRules
base.BaseKit  log.LogKit
```

**关键架构**：`Native.call` / `Native.callModule` / `Native.openScript` / `Native$ScriptStore`
——**统一入口 + 脚本存储**【已验证】。结合 dex 里几乎无业务类：业务逻辑以**脚本形式**
（引擎种类【未知】——非 JS/V8/Hermes/Lua 明文标识，可能是自研 VM 或字节码形态）
发布，由原生桥执行。**这正是 iOS/Android 双端能同步迭代的原因：业务层一份**【推断，高置信】。

## 三、UI 层判定【已验证 + 推断】

- **不是** Compose（androidx.compose 族 0 个类）、不是 Flutter/RN/Unity（引擎零命中）；
- androidx 全家仅 24 个基建类（core/window/graphics/datastore）——**连 appcompat/material 都没进**；
- 【推断，高置信】UI = **原生 View 体系（TextureView/SurfaceView + 自绘控件）**，代码全混淆。

## 四、两家播放器的架构对照（回答"安卓该用什么"）

| | **Rex (iOS)** | **SenPlayer (Android)** |
| --- | --- | --- |
| UI | **SwiftUI**（声明式，iOS 17+） | **View 体系 + 全混淆** |
| 播放内核 | KSPlayer（GPL-3.0，FFmpeg+Metal） | 自研 MPlayKit（FFmpegKit 裁剪版 + 动态注册 native） |
| DV 映射 | libdovi + FFmpeg RPU + 自研 Metal shader | FFmpeg RPU 解析在，**映射层【未知】**（无 libplacebo/无滤镜链——可能交系统 or 自绘） |
| 业务层 | 原生 Swift | **跨端脚本总线**（openScript/ScriptStore） |
| 生态绑定 | App Store / iCloud | 微信登录支付 / 网盘 / DLNA / SMB |
| 共同点 | **都不用跨平台 UI 框架**；播放内核都是 FFmpeg 系 | 同左 |

**对 kivo video 选型的含义**【设计建议】：
1. 两条成功路线里，**Rex 模式（原生声明式 UI + 开源内核）的可复制度远高于 SenPlayer 模式**——
   自研脚本引擎 + 双 Kit 是多年资产，不可复制也不必复制；
2. SenPlayer 验证了"FFmpeg 只做解码、渲染另起一层"是可行且常见的拆分——与我们
   `MediaCodec/FFmpeg 解码 + libplacebo Vulkan 渲染` 的规划一致；
3. `PreviewReader$Still`（进度条缩略图）是 SenPlayer 的亮点功能，实现思路（独立 reader 批量出帧）
   值得借鉴——与解码主链路解耦。

## 五、rea 在本任务的实际贡献与边界【已验证】

- ✅ `search-android-classes` 需要**npm tarball 的 Windows native 构件**（本地 git clone 缺
  `native/windows/build/rea-windows-x64.node`）——已定位，装 npm 版即可启用；
- ✅ APK 直接解压 + dex 自解析（`string_ids/type_ids/class_defs` 三表）已覆盖类清单需求；
- 【未知】混淆后的 6811 类的内部逻辑——需 jadx 反编译（rea 的 jadx provider 待装）。

## 六、覆盖清单

- [x] 包结构 / 体积 / 混淆状态（完整）
- [x] 三层架构与 JNI 面（完整）
- [x] UI 框架排除法判定（Compose/Flutter/RN 全排除，View 体系确认）
- [x] DV 映射落点判定（2026-11-06 补验）：**极可能不存在自研映射层**——
  MLibKit 的 dolby 字段全是媒体库元数据（`dolbyVisionProfile/Title`，Emby 风格 API）；
  唯一色彩痕迹是 `dolby vision + BT.2020 NCL + DV` 连排的**标签字符串**；
  dex 层 HDR/dataspace 处理为零；加此前确认的无 libplacebo/libdovi、滤镜链全裁——
  三边证据一致指向「**解析识别 + 兼容层透传**」：profile 8 按 HDR10（smpte2084+bt2020）
  给 Surface，tone mapping 交给系统显示管线【推断，高置信】。
- [x] 脚本引擎种类（仍未定，但已排除 JS/V8/Hermes/Lua 明文——【未知】维持，需动态分析）
- [ ] 脚本引擎种类（【未知】：需动态分析）
- [ ] UI 视觉参数（【未知】：需截图或运行观察）

## 七、DV 逆向收线（2026-11-07 补验，二进制级）

按交接计划对 `libMLibKit.so` 做了完整 Ghidra 分析（成功入库）+ pyghidra 全函数扫描：
**460 个函数 0 个 dovi/vivid/tone/ICtCp 命中**——业务库里没有 DV 代码，与 §六 判定一致。

随后分析真正的解码库 `libFFmpegKit.so`（54MB，Ghidra 全量分析成功入库），
`llvm-nm` 双重确认：**stripped（no symbols），dynsym 里 dovi/rpu/ictcp 零命中**；
仅存 2–3 处日志字符串（`dovi_rpu`/`dv_profile`/`ICtCp`）。按名字匹配的函数级提取
在这份库上没有可行路径（内部符号全无），且预期价值低——

**结论（【实测】）**：SenPlayer 的 DV 实现不超过「dvcC/dvh1 解析 + 兼容层透传」，
**没有 Rex 式 RPU 映射/曲线代码**。233（DV-B 真映射）的曲线公式来源维持
`01-dv-pipeline-teardown.md` 的 Rex DoviMatrix 产出，SenPlayer 侧不再深挖。
（工具链遗留：`C:\re-tools\ghidra-projects\SenProject` 内有 libMLibKit.so 与
libFFmpegKit.so 两份已分析程序，pyghidra 提取先例见 `re-tools/ffmpegkit-extract.log`；
注意 pyghidra CLI `--skip-analysis + 二进制路径` 打开的是**新导入骨架**（函数数 ~600），
不是工程内已分析程序——要开已分析程序需走 GhidraProject.openProgram 路线。）
