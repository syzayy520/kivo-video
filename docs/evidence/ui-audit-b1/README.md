# 272 批次一（UI/交互缺陷修复）真机证据

- 设备：Redmi K40（`b6e950ee`，alioth / M2012K11AC），1080×2400，density 440
- 应用：`com.kivo.cinema` debug（`android/keystore/kivo-debug.jks` 固定签名，覆盖安装）
- 构建：`cd android && JAVA_HOME=/c/Android/jdk-17.0.13+11 ./gradlew.bat assembleDebug --no-daemon`
  → 日志 `build-p272-b1.log`（末行 `BUILD SUCCESSFUL in 28s`）
- 复验日期：2026-11-13

## 场景清单

| # | 场景 | 结果 | 证据 |
| --- | --- | --- | --- |
| S1 | 详情页「更多」浮层 + 系统返回键 | PASS | 见下（uiautomator 树比对） |
| S2 | 追剧日历浮层 + 系统返回键 | PASS | 同上 |
| S3 | 「移出待看」二次确认 | PASS | `b1-detail-remove-confirm.png` |
| S4 | 播放钮与副标题指向同一集 | PASS | `b1-detail-cta-subtitle-ep2.png` + `b1-cta-plays-named-episode.png` |
| S5 | 播放失败面板 + 重试 + 30 秒起播看门狗 | PASS | 见 §二 |
| S6 | 「在看」页：选中态 / 扁平底 / 返回 / 手势条避让 | PASS | 见 §二 |

## 一、详情页浮层返回顺序（S1/S2）

修复点：`DetailPage.kt` 新增 `BackHandler`，优先级 **日历 → 更多菜单 → 选季菜单 → 退出详情页**。

复验（TMDB 电影《生化危机：爆发夜》，`MovieDetailActivity`）：

1. 点「⋯ 更多操作」→ 浮层出现，uiautomator 可见 `添加到日历` / `下载` / `追剧进度`。
2. 按 `KEYCODE_BACK` → 浮层三项消失，**页面仍在**
   （`ResumedActivity: com.kivo.cinema/.ui.detail.MovieDetailActivity`）。
3. 再开追剧日历 → 按 `KEYCODE_BACK` → 日历收起，**页面仍在**。
4. 无任何浮层时按 `KEYCODE_BACK` → 才回到 `com.kivo.cinema/.MainActivity`。

修复前：浮层没有 BackHandler，返回键直接退出详情页（浮层被连带销毁，用户要重新进页面）。

## 二、其余场景的复现方式与观察

- **S3 移出待看二次确认**：追剧日历底栏「🗑 移出待看」→ 弹出扁平确认框「移出待看？」＋
  「这部片会从「待看」列表移除，观看记录不受影响。」＋「取消 / 移出」。
  - 点「取消」→ `files/media/marks/titles.json` 里 `watchlist` 仍为 `true`，按钮仍在。
  - 点「移出」→ `watchlist` 变为 `false`，日历底栏的移出按钮随之消失。
  - 截图：`b1-detail-remove-confirm.png`。
- **S5 播放器状态**：本地文件播放失败 → 居中面板「播放失败」＋具体原因＋「重试」；
  点「重试」先关闭旧 libmpv 内核（`awaitClosed`）再起新会话，不会卡在无限缓冲；
  起播 30 秒仍未就绪 → 自动转「起播超时（30 秒）：内核没有就绪，请检查片源或网络后重试」。
- **S6 在看页**：`在看` 属「发现」子页 ⇒ 胶囊选中态落在「发现」；页面底色为冷灰 `#F2F2F7`
  （不再漏出发现页的粉紫渐变）；顶部有「‹ 发现」返回行，系统返回键回到「发现」；
  空态有「去发现」按钮；导航胶囊加了 `navigationBarsPadding()`，不再被手势条压住。

## 三、S4 播放钮与副标题一致（决定性证据）

复现前提：Emby 账号在**服务端**已看过第 1 集（`UserData.Played=true`），
详情页 `nextEp` 因此指向第 2 集。

- 详情页截图 `b1-detail-cta-subtitle-ep2.png`：**《瑞克和莫蒂》**，播放钮副标题 **「第1季·2集」**。
- 点该播放钮 → 播放器标题栏 **「瑞克和莫蒂 · S01E02 割草狗」**（截图 `b1-cta-plays-named-episode.png`）。

修复前：副标题报「下一集」，播放钮却永远开 `playIntent`（系列**第一集**）⇒
按钮说的和做的不是一回事（说第 2 集、播第 1 集）。修复后两者同源于 `nextEp`。

## 四、未能真机复现的项（如实记录）

- **设置写回失败 toast**（`SettingsDataSub.kt` 的 `onFailure` 分支）：需要一次真实写盘失败
  （空间不足 / 权限被拒）才会触发，本机无法稳定制造 ⇒ **只做了代码级实现，未取得真机截图**。
  不谎称已验证。
- **Emby 进度回写**：应用只读 `UserData.Played/PlaybackPositionTicks`，**从不写回**服务端，
  所以"在本应用里看完一集"不会改变 `nextEp`。这是既有功能缺口（进度不同步），
  不属批次一范围，已记入审计待办。
