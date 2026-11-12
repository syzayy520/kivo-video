# 39 私密区 / 隐藏源：应用内独立数字密码（重构）

> 性质：重构卡。用户拍板（2026-11-13）：「私密区那个最好是做数字密码」「独立的，不是手机 pin 密码，独立密码最好」。
> 现状两套门禁都依赖设备，K40 无锁屏 / 未录指纹 ⇒ 两条都进不去（nyamedia 隐藏源因此搁浅）。
> 本卡用**一个应用内独立数字密码**统一取代两套设备门禁，不新增播放功能、不新建项目包。

## 一、现状审计（代码证据）

| subsystem | 门禁 | 文件 | 设备依赖 | 失效原因 |
| --- | --- | --- | --- | --- |
| 私密区（本地私密视频） | 平台 `BiometricPrompt`（指纹） | `privatezone/PrivateGate.kt`、`PrivateActivity.kt:73-90`、`library/MediaLibraryActivity.kt:159-177` | 需录入生物特征 | K40 未录指纹 ⇒ `canAuthenticate≠0` ⇒ 「设备未录入生物特征」 |
| 隐藏源（Emby/SMB 账号） | `KeyguardManager.createConfirmDeviceCredentialIntent`（锁屏 PIN） | `sources/access/SourceUnlockController.kt:21-36` | 需 `isDeviceSecure` | K40 无锁屏 ⇒ intent=null ⇒ 「请先在系统设置中启用锁屏密码」 |

两者各持一套解锁态：私密区用 `PrivateActivity.unlocked`（Activity 局部），隐藏源用
`SourceAccessSession.unlocked`（进程级、退后台自动重锁，`sources/access/SourceAccessSession.kt`）。

**复用点**：`SourceAccessSession` 已是进程级解锁会话且自带退后台重锁——把它升格为**全应用唯一隐私解锁会话**，
私密区也读它，一次解锁同时放行私密区与隐藏源。`SourceEligibility.visible/home/search` 过滤逻辑不变。

## 二、设计决策

1. **一个密码，两处共用**。4–6 位纯数字，应用内独立设置，与设备锁屏 / 生物特征完全解耦。
2. **存储 = 盐化 PBKDF2 哈希，不落明文**。`PBKDF2WithHmacSHA256`（minSdk=26 可用，零新依赖，离线构建可行），
   16 字节随机盐、120 000 轮、256-bit 派生；`MessageDigest.isEqual` 常数时间比对。
   文件落 `noBackupFilesDir/privacy/lock.bin`（应用私有、不参与备份，与 `SourceStore` 同口径），AtomicFile 原子写。
3. **威胁模型如实披露**：这是**访问门**，不是文件加密。私密视频仍在 MediaStore、隐藏源凭据仍由
   `SourceCipher`（设备 Keystore）加密。数字密码防的是「手机已解锁时的随手翻看」，不防离线取证。UI 与证据都写明，不静默夸大。
4. **忘记密码 = 无后门**。不设找回（无其它可信鉴权通道）。UNLOCK 态提供「忘记密码？」说明弹窗：
   指引「系统设置→应用→Kivo→存储→清除数据」，并明示会清空已保存源与私密标记。不做「一键重置即放行」的伪找回（那等于拆门）。
5. **改密**：需验原密码 → 新密码 → 确认。入口在私密区头部（已解锁时）。
6. **FLAG_SECURE 收口**：私密区已解锁时挂 SECURE（现状漏挂，截图 / 最近任务会泄露私密海报）；隐藏源 SECURE 逻辑保持。
7. **过渡动效**（铁律）：键盘按压回弹、错误抖动、步骤交叉淡入，不做瞬现瞬消。

## 三、家族树（新增 `privacy/` 家族 + 两处退役 + 六处接线）

> 「不新建项目包」= 不另起工程 / Gradle 模块。应用内按语义建 Kotlin 家族包与现有 `sources/access`、`privatezone` 同例。
> 新家族 `com.kivo.cinema.privacy/` 是私密区与隐藏源的**共用密码层**，语义中立，故不塞进任一消费方包内。

```text
android/app/src/main/java/com/kivo/cinema/
├── privacy/                         ← 新家族：应用内独立数字密码（共用层）
│   ├── PrivacyPassword.kt           单一职责：密码派生 / 存储 / 校验（PBKDF2，无 UI、无会话）
│   ├── PrivacyKeypad.kt             单一职责：数字键盘 + 圆点显示（纯输入件，无业务）
│   ├── PrivacyLockFlow.kt           单一职责：设置 / 解锁 / 改密三态流程编排（用 Keypad + Password）
│   └── PrivacyUnlockDialog.kt       单一职责：资源库 / Emby 的「解锁或首设」门弹窗（用 Flow + 会话）
├── privatezone/
│   ├── PrivateActivity.kt           改：删指纹门，读 SourceAccessSession，内嵌 PrivacyLockFlow；挂 SECURE；加改密入口
│   ├── PrivateGate.kt               ✗ 退役删除（BiometricPrompt 已被取代，两调用方均迁走）
│   └── PrivateStore.kt              不动（私密条目 ID 集合）
├── sources/access/
│   ├── SourceAccessSession.kt       不动（升格为全应用唯一隐私会话，逻辑已满足）
│   ├── SourceUnlockController.kt    ✗ 退役删除（KeyguardManager 已被取代，两调用方均迁走）
│   └── SourceEligibility.kt         不动
├── emby/EmbyActivity.kt             改：门按钮 → PrivacyUnlockDialog（删 SourceUnlockController）
├── MainActivity.kt                  改：onUnlock → PrivacyUnlockDialog（删 sourceUnlock 字段；既有 350 行债仍挂 #37）
├── library/MediaLibraryActivity.kt  改：openPrivateZone 删指纹前置探测，直达 PrivateActivity（删 privGate）
└── sources/ui/SourcesScreen.kt      改：私密区卡副标题随会话态（已锁定 / 已解锁）
AndroidManifest.xml                  改：删 USE_BIOMETRIC（仅指纹用，已无引用）
```

**调用方计数（防单调用方空壳）**：`PrivacyLockFlow` ← PrivateActivity + PrivacyUnlockDialog（2）；
`PrivacyUnlockDialog` ← MainActivity + EmbyActivity（2）；`PrivacyKeypad` ← PrivacyLockFlow（1，但是含真实渲染逻辑的输入件，非空壳）；
`PrivacyPassword` ← Flow + Dialog（2）。

## 四、实现闸门

| # | 闸门 | 判据 |
| --- | --- | --- |
| G1 | 首设 | 无密码时点私密区 / 锁图标 → 设置键盘 → 4–6 位 → 确认 → 落 lock.bin → 放行 |
| G2 | 解锁 + 重锁 | 退后台回前台自动锁；错密码抖动 + 非空 reason；对密码放行 |
| G3 | 隐藏源共用 | 隐藏 nyamedia → 锁图标解锁 → 列表可见 → EmbyActivity 门解锁 → 浏览（全程无需设备锁屏） |
| G4 | 改密 | 验原 → 新 → 确认 → 重锁 → 新密码放行、旧密码拒 |
| G5 | 忘记说明 | UNLOCK 态「忘记密码？」弹说明（无后门），不静默吞点击 |
| G6 | 死代码清零 | `android/` 源码与 manifest 中 `BiometricPrompt`/`KeyguardManager`/`PrivateGate`/`SourceUnlockController`/`USE_BIOMETRIC` 零匹配（231/237 历史设计稿保留作存档） |
| G7 | 行数闸 | 新增家族与重写文件 ≤300 行；既有超闸不新增超闸文件（MainActivity 357 挂 #37、MediaLibraryActivity 334 挂 #45，本卡只减不增） |
| G8 | 凭据红线 | 测试数字密码不入证据 / 文档；键盘只显圆点；含密码表单态零截图 |

## 五、证据

`docs/evidence/p258/privacy-lock/`（闸门表 + 帧 + 复算命令 + 披露）。
设备 K40 `b6e950ee`（无锁屏 / 未录指纹——正是本卡要解的环境）。