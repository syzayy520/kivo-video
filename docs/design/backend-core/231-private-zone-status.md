# 231 · 私密区 —— 状态稿（收口）

> 日期：2026-11-08 · 设备：Redmi K40（b6e950ee）
> 结论：**AC1–AC4 全部真机通过**。证据 `docs/evidence/p231/`，
> 校验器 `tools/verify_p231_private.py`（6/6 变异自检）。
> 设备状态注：测试前 K40 无任何锁屏凭据（BiometricPrompt 无法运行）；经用户同意，
> 本轮由 adb 设 PIN + 用户录入一枚指纹——**这是 231 能真机验证的前置条件，不是产品改动**。

---

## 一、一句话结果

**媒体库长按标记「私密」→ 条目即时从主网格消失（159→隐藏）→「私密区」按钮 →
系统指纹弹窗（真实 BiometricPrompt）→ 真指纹解锁 → 只见私密条目 → 长按移出 →
主网格恢复——Rex "protected viewing environments" 同语义，平台 API 零第三方。**

## 二、实测矩阵

| # | 判据 | 实测 | 证据 |
| --- | --- | --- | --- |
| AC1 | 门 | 弹窗文本 `私密区 / 请触摸指纹传感器 / 取消`；取消路径 `未进入私密区：已取消`（G9）；**真指纹两轮解锁成功**；未录入负路径 `canAuthenticate=11 NONE_ENROLLED` | `p231-prompt-dump.txt`（弹窗设 FLAG_SECURE，截屏 0 字节，dump 是替代证据） |
| AC2 | 隐藏 | 加入后主网格 1080p30 消失，首位移给 1gb_720p | `p231-hidden.png` |
| AC3 | 私密区 | `已解锁：1 个私密条目`，网格只含 1080p30 | `p231-grid.png` |
| AC4 | 往返 | 私密区内长按移出 → `0 个私密条目` + `private.json=[]` → 主网格 159 个且 1080p30 回首位 | `p231-after-remove.png`、`p231-restored2.png` |

## 三、实现要点（设计稿修正两处）

1. **D1 选型更正**：设计稿写"平台 android.hardware.biometrics.BiometricPrompt (API 28+)"——
   实测它**从未进入公开 SDK 存根**（android-37.0 jar 里其实存在，但 Builder 方法面不同：
   `setNegativeButton(CharSequence, Executor, Listener)`，老教程的 `setNegativeButtonText` 不存在）；
   deprecated 的 `FingerprintManager` 在 SDK 37 存根已删除。终选平台 BiometricPrompt +
   SDK 37 方法面，零第三方成立。
2. **G9 禁用态陷阱**：入口按钮原设计"未录入即禁用"——禁用按钮静默吞点击。改为**常开 +
   点进去给 canAuthenticate 码的 reason**。

## 四、本卡真机踩的坑（下次直接用）

1. **无锁屏凭据 = 生物特征全链路不可用**：MIUI 录指纹前强制先设锁屏密码；dumpsys fingerprint
   里 `"count":0` 的 print 记录不代表可用，`canAuthenticate` 码才是真相。
2. **BiometricPrompt 窗口 FLAG_SECURE**：弹窗期间 screencap 返回 0 字节——uiautomator dump
   拿弹窗文本是替代取证手段。
3. **`remember` 不能放进 LazyGridScope content lambda**（那里非 composable 上下文）——
   过滤列表的 remember 要放在网格外面。
4. **跨页状态同步**：私密区里移出条目后返回媒体库，媒体库的过滤 epoch 不会自动刷新——
   onResume 里强制重算。

## 五、豁免与未做（明示，不藏）

- CryptoObject 加密绑定（密钥绑文件内容，防复制）：v1 未做，标记存储为明文 JSON。
- PIN/图案回退路径：BiometricPrompt 只允许生物特征（设备凭据路径未启用）。
- 远端源（SMB/Emby）条目入私密区：v1 只支持本地媒体库条目。
- 生物特征删除后的行为：private.json 保留，重新录入指纹即可再进（无加密绑定所致，v1 明示取舍）。

## 六、设备状态注（用户须知）

测试期间给 K40 设了锁屏 PIN（`1234`，可自行在设置中更改/清除）并录入了一枚指纹。
清除锁屏密码会连带清除指纹，私密区将回到 `NONE_ENROLLED` 负路径。
