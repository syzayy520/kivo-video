# 231 · 私密区 —— 设计稿

> 状态：**设计完成，实现开工**。M3 第二卡。
> 卡片原文：私密区（BiometricPrompt，对齐 Rex FaceID）P2。
> 参照：Rex `NSFaceIDUsageDescription`（InfoPlist 拆解）：「Rex uses Face ID to verify your
> identity and **unlock protected viewing environments**」；蓝图 R6「BiometricPrompt + 隐藏空间」。
> Rex 侧只有权限声明与蓝图级证据（主 UI 未本地化）——交互细节以 Android 平台惯例补位，此处明示。
> 测试设备：K40 指纹已录 2 枚（Fingerprint21 侧边电源键 FPS，dumpsys 实证）。

---

## 一、一句话目标

**媒体库长按任意视频标记「私密」→ 主网格立即隐藏 → 点「私密区」按钮 → 指纹验证 →
解锁后只看私密条目——Rex 同款"受保护观看空间"，平台 API 零第三方。**

## 二、设计决策

### D1 门：平台 BiometricPrompt（API 28+），零第三方
`android.hardware.biometrics.BiometricPrompt`（K40 = Android 13）。
`canAuthenticate` 探测：无生物特征录入 → 入口按钮置灰 + reason 用户可见（G9），不静默降级。
**每次进入都验证**（对齐 Rex 的"unlock environments"语义，非解锁一次长期有效）。

### D2 标记存储：app 私有 JSON
`filesDir/private.json` 存 MediaStore `_ID` 字符串列表。`add/remove/contains/ids` 四操作。
不搬文件、不改 MediaStore——标记与内容分离，卸载即失效（v1 取舍，明示）。

### D3 标记交互：媒体库长按卡片
长按 → AlertDialog：「加入私密区 / 移出私密区」二选一。主网格查询后过滤私密 id。
**隐藏是即时的**（重组），无需重启。

### D4 私密区网格：解锁后同规格海报墙
`PrivateActivity`：先验证门 → 成功后 LazyVerticalGrid(3列) 2:3 卡（222 同款 Rex 规格），
只渲染 private.json 里的条目；点卡片照常 `kivo.media` 起播（门在入口，播放层无感知）。
失败/取消 → 状态栏 reason（G9）。

### D5 权限
`USE_BIOMETRIC`（平台 API 不强制，按官方推荐声明）。

## 三、家族树

| 文件 | 新/旧 | 单一职责 |
| --- | --- | --- |
| `private/PrivateStore.kt` | **新** | 私密 id 集合存取（JSON 持久化 + 内存镜像） |
| `private/PrivateGate.kt` | **新** | BiometricPrompt 门（能力探测 + 验证回调封装） |
| `private/PrivateActivity.kt` | **新** | 解锁后的私密海报墙 |
| `library/MediaLibraryActivity.kt` | 旧（小改） | 主网格过滤 + 长按标记菜单 + 「私密区」入口按钮 |
| `AndroidManifest.xml` | 旧（小改） | USE_BIOMETRIC 权限 + PrivateActivity 注册 |

## 四、验收判据

| # | 判据 | 通过条件 |
| --- | --- | --- |
| AC1 | 门 | BiometricPrompt 系统弹窗出现；取消路径 reason 用户可见；**指纹成功路径真机验证** |
| AC2 | 隐藏 | 标记后主网格 count 递减且该卡消失（截图对照） |
| AC3 | 私密区 | 解锁后网格只含私密条目 |
| AC4 | 往返 | 加入→移出→主网格恢复；verify 脚本 5+ 变异自检 |

## 五、明确不做（v1）

仿生加密绑定（CryptoObject/密钥绑定文件内容）· PIN/图案回退（无锁屏测试机也不具备）·
私密区条目独立缩略缓存 · 远端源条目入私密区（v1 只支持本地媒体库条目）· 多用户/多套私密区。
