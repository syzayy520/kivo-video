# 300 · Surface 重建 100 次 + 连续 seek 20 次（补 275 §6.2 两项验收）

状态：**两项均 PASS**；过程中发现并修复一个**真实的 JNI global ref 泄漏**。
设备：K40 `b6e950ee`（SM8250 / A13）
时间：2026-11-16 02:41–03:05
片源：本地 `/sdcard/Android/data/com.kivo.cinema/files/media/mpv-test-3min.mkv`（26.6 MB / 180 s / HEVC）
—— 刻意**不用公网**：这两项只验 Surface/seek 机制，网络波动会污染结论。

---

## 一、测试 A：100 次前后台重建（Surface 重绑）

### 装置

`tools/p300_surface_rebind_soak.sh 100 1.2`
每轮：`KEYCODE_HOME` → 1.2 s → `am start -f 0x00020000`（REORDER_TO_FRONT，复用同一实例）→ 1.2 s。

**先把偏好改成能测到 mpv 路径**（测完已恢复原值）：
`background_audio=false`（true 会走自研链路，mpv 根本不参与）、
`auto_resume=true`（回前台自动续播）、`auto_pip=false`（true 会进 PiP，不进 onStop 暂停路径）、
`always_from_start=true`（避免测试片播完干扰）。

### 为什么每轮都会真的重绑

`HOME` → `onStop` → SurfaceView 的 surface 销毁 → 回前台 → `surfaceCreated`
→ `PlayerSessionStart:32-34` → `MpvPlaybackSession.rebind(surface)`
→ native `attach_surface`（gpu-next 分支）→ `mpv_set_option("wid", …)`。

diag 里 `attach-surface gpu-next (wid set)` 逐轮递增就是硬证据（不是"看着没崩"）。

### 结果

| 判据 | 修复前（第 1 轮） | 修复后（第 2 轮） |
| --- | --- | --- |
| pid 不变 | ✅ 12783（100 次未变） | ✅ 24086（100 次未变） |
| attach 计数 | 1 → **101**（delta=100） | 1 → **101**（delta=100） |
| Native Heap | 17344 → 16224 KB（**下降**） | 18008 → 15676 KB（**下降**） |
| 崩溃/被杀 | 无 | 无 |

### ★ 发现：每次重绑泄漏一个 JNI global ref

**这不是从内存数字看出来的**（每次约 1 KB，被正常波动淹没），是**代码路径 + 计数**共同证明：

1. `jni_mpv_bridge.cpp:142`：每次 `nativeAttachSurface` 都 `NewGlobalRef(surface)`，成功时不释放；
2. `mpv_session.cpp:48`（原）：`impl_->surface_global_ref = wid;` **直接覆盖**旧值；
3. 全仓 `DeleteGlobalRef(surface_global_ref)` **只有会话销毁那一处**（`mpv_session.cpp:258-264`）
   ⇒ 旧引用永远不释放。

⇒ rebind N 次 = 泄漏 N 个 global ref。压测的 `attach 1→101` 与泄漏次数一一对应。
JNI global ref 表有上限，累计到上限会 abort（`global reference table overflow`）——
100 次没触发只是因为离上限还远。

### 修复（`mpv_session.cpp`，gpu-next 分支）

顺序改为：**先 `mpv_set_option("wid")` 成功 → 再 `DeleteGlobalRef(旧)` → 最后存新值**。

```cpp
int rc = mpv_set_option(impl_->handle, "wid", MPV_FORMAT_INT64, &wid);
if (rc < 0) { /* 新 ref 由 JNI 层释放；旧的必须保留 */ return false; }
if (impl_->surface_global_ref != 0 && impl_->surface_global_ref != wid) {
    if (JNIEnv* env = attach_env(); env != nullptr) {
        env->DeleteGlobalRef(reinterpret_cast<jobject>(
            reinterpret_cast<intptr_t>(impl_->surface_global_ref)));
    }
}
impl_->surface_global_ref = wid;
```

**为什么必须先切成功再删**：切换期间 mpv 可能还在用旧 surface；删早了就是野指针。

### 修复安全性验证（CheckJNI）

`adb shell setprop debug.checkjni 1` → 重启 app → 再跑 20 次重绑：

| 检查 | 结果 |
| --- | --- |
| pid 不变 | ✅ 23208 |
| attach 计数 | 1 → 21（20 次） |
| **logcat JNI 错误**（`JNI ERROR`/`global reference`/`use after`/`Fatal signal`） | **0 条** |

⇒ 删除时机安全，"提前删"没有引入野指针。测完已 `setprop debug.checkjni 0` 复位。

---

## 二、测试 B：20 次乱序连续 seek

### 装置

`MpvSeekBurstInstrumentation`（新增 androidTest，走 `MpvPlaybackSession` 真实路径）。
目标序列刻意让相邻跨度大（10→150→30→170→60→5→120…），模拟快速拖动。

### 结果：20/20 有效落点

| 指标 | 第 1 次 | 第 2 次（原始输出 `seek-burst.txt`） |
| --- | --- | --- |
| 落点误差 | **0–458 ms** | **0–542 ms** |
| 单次耗时 | **100–216 ms** | **100–175 ms** |
| 失败/无落点 | **0 次** | **0 次** |
| 最终 position | 114833 ms | 115083 ms（最后目标 110 s，差 ~5 s = 之后继续播放的推进） |

两次独立运行的最大偏差都 < 550 ms、最大耗时 < 220 ms，量级一致。

全部 20 行明细见复算命令输出。

### ⚠️ 口径必须诚实

**当前 `seekTo` 是同步等落点**（`MpvSeekControl.awaitLanding`，上限 1.5 s），
所谓"连续 20 次"实际是**串行**执行（输出里 `serial=true`）。

⇒ 本测试证明的是「**20 次乱序连发都正确且不卡死**」，
**不**证明 275 §4.2 的「取消或合并过期 seek」——**该机制当前不存在**，快速拖动会排队。
这是已知 gap，未被本次测试覆盖，也不能被这条 PASS 掩盖。

## 三、测试 C：暂停态 seek 不误启动

| 判据 | 结果 |
| --- | --- |
| `setPaused(true)` 生效 | ✅ state=PAUSED |
| seek(60 s) 后仍暂停 | ✅ state=PAUSED（未被误启动） |
| 暂停态落点 | ✅ **60000 ms**（精确命中） |

与 275 §4.2 第 4 条一致。

---

## 四、未闭合 / 边界

1. **"取消过期 seek"仍未实现**（`serial=true`）。快速拖动在 UI 侧排队，本次只证明不卡死。
2. **泄漏在 100 次量级不可测量**：修复依赖代码路径正确，不依赖内存数字；
   要观测后果需跑到 global ref 表上限（数千次），本轮未做。
3. 只测**本地 26.6 MB / 180 s 片源**；SMB/公网源下的 rebind 与 seek 未复测。
4. 只测 K40 一台设备；MatePad 未跑。
5. `nRepeat` 100 次是"前后台重建"；**旋转、分屏、PiP 进出**等其它 Surface 重建路径未覆盖。

## 五、复算命令

```bash
export PATH="/c/Android/sdk/platform-tools:$PATH"
export MSYS_NO_PATHCONV=1     # 否则 /sdcard 会被 Git Bash 转成 Windows 路径
A="adb -s b6e950ee"

# A. Surface 重建 100 次（前置：起播后）
bash tools/p300_surface_rebind_soak.sh 100 1.2

# A2. 修复安全性（CheckJNI）
$A shell setprop debug.checkjni 1
$A shell am force-stop com.kivo.cinema     # 重启才生效
# …起播后…
bash tools/p300_surface_rebind_soak.sh 20 1.2
$A logcat -d | grep -iE "JNI ERROR|global reference|use after|Fatal signal"   # 应为空
$A shell setprop debug.checkjni 0

# B/C. seek 压测
cd android
./gradlew :app:assembleDebugAndroidTest \
  -Pkivo.testRunner=com.kivo.cinema.mpv.MpvSeekBurstInstrumentation --offline
$A install -r app/build/outputs/apk/androidTest/debug/app-debug-androidTest.apk
$A shell am instrument -w \
  -e mediaPath "/sdcard/Android/data/com.kivo.cinema/files/media/mpv-test-3min.mkv" \
  com.kivo.cinema.test/com.kivo.cinema.mpv.MpvSeekBurstInstrumentation
```

## 六、证据文件

| 文件 | 内容 |
| --- | --- |
| `home2.log` | HOME→回来 一次的内核日志（含 `Setting option 'wid'`） |
| `after-home.png` | 回前台后画面 |
| `probe.png` | 一次探针截图 |

> 100 次压测的逐轮 `attach` 计数来自脚本 stdout（已在下文与日记留档）；
> diag 文件在每次会话创建时重置，故不单独保存原始文件。
