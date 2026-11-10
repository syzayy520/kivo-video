# P250 · S6 补丁 · 断网看门狗缺权限（真机 bug）

日期：2026-11-10
发现方式：251 前置实验里，`SmbReadAheadInstrumentation` 在真机上抛
`SecurityException: ConnectivityService: Neither user 10904 nor current process has
android.permission.ACCESS_NETWORK_STATE`。

## 一、问题

`NetworkLinkMonitor.isOnline()` 读 `ConnectivityManager.activeNetwork`，这个接口需要
`ACCESS_NETWORK_STATE`。主 manifest 原来只声明了 `INTERNET`：

```
$ grep uses-permission android/app/src/main/AndroidManifest.xml
  INTERNET / USE_BIOMETRIC / CHANGE_WIFI_MULTICAST_STATE /
  FOREGROUND_SERVICE / FOREGROUND_SERVICE_MEDIA_PLAYBACK /
  READ_MEDIA_VIDEO / POST_NOTIFICATIONS
```

**没有 `ACCESS_NETWORK_STATE`。**

后果：`activeNetwork` 抛 `SecurityException`（不是返回 null）。S3 加的断网看门狗
（`RangeFd` 里那个 1 s 轮询）**在真机上其实一直是失效的** —— 异常被
`runCatching { probe() }.getOrDefault(true)` 吞掉，永远返回"在线"。

## 二、那 S3 的 1000 ms 是怎么来的？（必须纠正）

S3 证据里写的是"看门狗把提示从 15 s 压到 1 s"。补上权限后复跑同一测试：

```
P243 BUFFERING 位置停止后 1000 ms 首次上报重连中
P243 STALL 位置停在 14788 ms（已 5001 ms 未推进）
P243 RESUME 停 16002 ms 后从 14788 ms 接上
5/5 PASS
```

**结果一模一样（1000 ms）**。所以那 1 秒不是看门狗的功劳。

真实机制：`svc wifi disable` 会让内核拆掉 socket，正在阻塞的 `poll` 立刻返回错误
→ `smb2_pread` 快速失败 → `RangeFd.onRead` catch → `stalled = true`。
**是"Wi-Fi 被关掉"这个动作本身很快，不是我们察觉快。**

看门狗的真正价值在另一个场景：**连接没断、但对端不响应**（NAT 表项超时、
AP 信号弱、睡眠唤醒）。那时 socket 不会报错，读会卡满超时，看门狗才有用。
这个场景**本卡没有能力构造**（需要能模拟 half-open 连接）。

## 三、修了什么

1. `AndroidManifest.xml` 加 `ACCESS_NETWORK_STATE`（附注释说明为什么）。
2. `NetworkLinkMonitor.isOnline` 加 `SecurityException` 兜底：返回 `true` 而不是抛。
   理由与文件里原有的取向一致 —— 宁可多试一次网络，也不要因为一次查询失败把
   还能读的播放判死。（manifest 已声明权限，这条是防"有人删了权限"时行为静默变化。）

## 四、复算

```bash
cd android && JAVA_HOME=C:/Android/jdk-17.0.13+11 ./gradlew.bat assembleDebug
adb install -r app/build/outputs/apk/debug/app-debug.apk
# 断链测试同 S3 的编排（-e windowSec 45，第 15 秒 svc wifi disable，12 秒后 enable）
```

## 五、诚实标注

- **S3 的"1000 ms"结论要重新理解**：数字对，归因错。已在 251 前置实验文档与
  `TASKS.md` 里更正。
- 看门狗在"half-open 连接"下是否真的能在 1 s 内报警，**仍然没有证据**，
  因为本机造不出该场景。这一条留给有条件的卡。
