# 267 · 外观与交互 真机证据与复算链

> 设备：Redmi K40 `b6e950ee`（HyperOS；竖屏 1080×2400，播放器横屏 2400×1080）；签名 `kivoDebug`；
> 日期：2026-11-13；分支 `codex/kivo-video-p0-ui-01-playback-overlay-v13`。
> 规格：[`appearance-interaction-settings.md`](../../../design/ui/replica/appearance-interaction-settings.md)（家族树先行；闸门 §五.1–6）。
> 帧均为真机 screencap；XML 均为真机 uiautomator dump 或 `run-as` shared_prefs 读回；可按本文命令行复算。

## 〇、复算命令

- 开关态：`adb -s b6e950ee shell "run-as com.kivo.cinema cat shared_prefs/appearance_interaction.xml"`
  与 `... shared_prefs/home_display.xml`，对照本目录 `prefs-*.xml` / `home-*.xml`。
- 节显隐：数 `ui-d1/d2/d3.xml` 中 `剧照` / `家长指南` 出现次数（详情语义树）。
- 触感三态：`haptic-log.txt` 三行（测量构建产出，见 D-F）+ 三张同手势锁屏帧。
- 自动旋转恢复：`adb -s b6e950ee shell settings get system accelerometer_rotation` = 1。

## 一、证据清单

### 根页 / 子页 / 钻入 / 弹层
| 文件 | 证明 |
| --- | --- |
| c00.png | 设置根页：外观与交互入口、autoplay OFF 基线 |
| c01.png | 子页首屏：外观模式/减弱动画/减弱视觉/豆瓣 4 灰行带原因 + 导航 › |
| c02.png | 子页滚动 1：封面与剧照/首页/日历卡（灰行带原因） |
| c03.png | 子页滚动 2：预告片/家长指南/操作反馈/图标卡 |
| c04.png | 触感弹层三选：始终开启 / 自动✓ / 禁用 |
| c05.png | 导航钻入：1 真 3 灰 + 主 Tab 只读四行（无假拖拽柄） |
| c06.png | Badge 钻入：全灰带原因，「未配置」为真值 |
| ui-root.xml / ui-root2.xml | 根页语义树：取证前 / 收口回归（播放/操控/AI 字幕/数据与资源/外观 5 条目 intact） |
| ui-top.xml / ui-scroll2.xml | 子页首屏 / 滚动 2 语义树（行 bounds 复算源） |
| ui-haptic-picker.xml | 触感弹层语义树 |
| ui-haptic-always-set.xml | 设「始终开启」后子页态 |
| ui-nav.xml | 导航钻入语义树（隐藏标题 + 主 Tab 四行） |

### 消费两态（闸 4）
| 文件 | 证明 |
| --- | --- |
| c07.png + ui-d1.xml | 详情默认：剧照节=1、家长指南=1 |
| c08.png + ui-d2.xml | 剧照 OFF：剧照=0、家长指南=1 |
| ui-stills-off.xml | 剧照 OFF 时子页开关态 |
| c09.png + ui-d3.xml | 内容提示 OFF：家长指南=0、剧照=1 |
| ui-notes-off.xml | 内容提示 OFF 时子页开关态 |
| c10.png + nav-hide-on.png | 首页隐藏标题 ON：胶囊导航仅图标无文字 |
| ui-home.xml | 首页默认语义树（四标签在） |
| ui-detail0.xml | 详情首 shot 未加载空 dump，作废（以 ui-d1 为准） |
| ui-player.xml | 播放器横屏语义树（控制栏/锁钮坐标源） |

### 同步 / 持久化 / 声音写回（闸 3）
| 文件 | 证明 |
| --- | --- |
| c12.png | 子页 autoplay ON |
| c13.png + ui-root-autoplay-on.xml | 根页同键 ON（对照 ui-root-autoplay-off.xml checked false→true） |
| c14.png | force-stop 重进首页：隐藏标题仍生效 + 继续观看卡（持久化） |
| prefs-after.xml / home-prefs-after.xml | 测试期非默认态读回 |
| c17.png + home-sound-on.xml / home-sound-off.xml | 声音键写回 true→false（帧开关 ON 与 XML 一致） |

### 触感三态（闸 4）
| 文件 | 证明 |
| --- | --- |
| haptic-always-lock.png | always：同长按锁屏手势，锁提示+左下挂锁 |
| haptic-disabled-lock.png | disabled：同手势同帧态（门控关，不震） |
| haptic-auto-lock.png | auto：同手势同帧态（电量 91–95% 非低电，允许） |
| haptic-log.txt | 测量构建三行：always allowed=true perform=true / disabled allowed=false perform=false / auto allowed=true perform=true |
| clean-build-lock.png | 最终干净包：锁提示出现（手势确触发 haptic 路径）而日志停 3 行 ⇒ 无测量代码（D-F） |
| haptic-always.txt / haptic-disabled.txt | vibrator_manager dump：0 条 kivo 记录（MIUI 限制 D-B；disabled 份为无效早期探针） |

### 回归 / 恢复（闸 5/6）
| 文件 | 证明 |
| --- | --- |
| c15-home-default.png | 首页默认回归：发现/我的/资源库/设置四标签恢复 |
| c16-playback-sub.png | 播放子页回归：262 基线 intact，灰行带非空原因 |
| prefs-restored.xml | show_stills=true / show_content_notes=true / hide_nav_labels=false / haptic_mode=auto（全默认） |
| home-prefs-restored.xml | preview_autoplay=false / preview_sound=false（全默认） |

## 二、闸门复算（设计稿 §五）

1. **行数 PASS**：新增 5 文件 73/82/71/39/23 ≤300；修改净增 manifest+2 / PlayerGestureRouter+3 /
   CapsuleNavBar+5 / HomePreferences+3 / HeroTrailer+2 / SettingsNav 0 / SettingsPage+5 / SettingsRows+6 /
   DetailPage+3，全 ≤10；MainActivity 净 0。偏差披露 D-H。
2. **灰行非空原因 PASS**：c01/c02/c03 根页 11 灰 + c05 导航 3 灰 + c06 Badge 全灰，subtitle/Footnote 均非空且用户可见。
3. **真开关写回+持久化 PASS**：4 键 prefs-after.xml 读回；autoplay 子↔根同键同步 c12→c13（checked false→true）；
   c14 force-stop 后仍生效；声音 home-sound-on/off.xml true→false。
4. **消费复算 PASS**：剧照 c07→c08（1→0，指南 1→1）；提示 c07→c09（指南 1→0，剧照 1→1）；
   nav c00→c10（标签→仅图标）；触感三态 haptic-log.txt + 三锁屏帧（vibrator dump 拿不到 → 按闸允许披露 D-B）；
   声音键写回 + 挂载门代码路径 HeroTrailer.kt:58/74/111（运行时音频未单独取证，闸口径=代码路径，D-G）。
5. **不回归 PASS**：c15 首页默认 + c16 播放子页 + ui-root2.xml 根页 5 条目 intact。
6. **设备恢复 PASS**：prefs-restored.xml + home-prefs-restored.xml 全默认；accelerometer_rotation=1 读回；全程未 uninstall。

## 三、披露

- **D-A 真 bug 修复**：manifest 原缺 `android.permission.VIBRATE`，`performHapticFeedback` 抛
  SecurityException 被 runCatching 吞 = 静默降级。本卡加权限并移除 runCatching
  （`PlayerGestureRouter.kt` haptic()），dumpsys 确认 granted=true。
- **D-B MIUI 不记 performHapticFeedback 历史**：vibrator_manager「Previous vibrations」有微信/支付宝
  旧路径记录但无本应用记录（perform=true 仍 0 条，haptic-always.txt）。闸 4 触感复算改走测量构建
  文件日志 + 同手势锁屏帧；haptic-disabled.txt 为手势/坐标修正前的无效早期探针，仅存过程档案、不作判据。
- **D-C auto 低电量分支未真机实测**：全程电量 91–95%，系统 `EXTRA_BATTERY_LOW`=false，auto 只测到
  allowed=true；低电量→关为代码 when 分支（AppearancePrefs.kt:66,70-73），低电量态不可伪造，披露。
- **D-D 前台劫持 ×3**：微信小程序 ×2（c09 首 shot、auto 证明首 shot）+ 闲鱼启动页 ×1（声音测试首 shot，
  c17 首版作废重取）。均 force-stop 劫持者 + 干净重启 + mCurrentFocus 校验处理；本进程 pid 存活、
  crash buffer 空，非崩溃。
- **D-E 播放器横屏坐标 + 控制栏轮盘**：播放器 2400×1080，竖屏坐标无效；控制栏起播自动_show 后自动隐，
  「show」tap 可能反而隐藏；用设备端重试循环（每轮 toggle+长按、按日志行数 break）解决。
- **D-F 测量构建 vs 最终干净包**：haptic-log.txt 由临时测量构建（Log.w + 文件 append）产出；最终包
  haptic() 回退为门控版（无日志）。clean-build-lock.png 锁提示出现而日志停 3 行 ⇒ 最终包无测量代码。
- **D-G 声音运行时音频未取证**：闸 4 口径=键写回+挂载门代码路径（HeroTrailer.kt:58 muted 初值 / :74 重置 /
  :111 setMuted）；声音生效时机=下次预告挂载（设计 D3），本卡未单独录音频。
- **D-H SettingsRows 为第 8 个修改文件**：设计稿估 7 修改、行原语零新增；实际 +6 行 PlainCard（无头卡原语，
  5 消费点：SettingsAppearanceSub×2 / Nav×1 / Badge×2），净增仍 ≤10/文件，闸 1 门槛不破。