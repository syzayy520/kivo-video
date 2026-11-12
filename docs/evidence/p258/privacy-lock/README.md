# 39 私密区 / 隐藏源：应用内独立数字密码 · 真机证据

> 设计稿：`docs/design/ui/replica/privacy-numeric-lock.md`（家族树 + 闸门 G1–G8）。
> 设备：Redmi K40 `b6e950ee`（1080×2400，density 440）。**无锁屏、未录指纹**——正是本卡要解的环境。
> APK md5：`527dec872b58db9487044738a0f1aded`（全部帧 / dump 均出于此包）。
> 威胁模型：访问门，**不是文件加密**；忘记密码**无后门**（见 G5）。

## 一、闸门判定

| # | 闸门 | 判定 | 证据 |
| --- | --- | --- | --- |
| G1 | 首设 | PASS | `g1-01-setup.png`（设置私密密码／独立于手机锁屏）→ `g1-02-dots.png`（只显圆点）→ `g1-03-confirm.png`（确认）→ `g1-04-unlocked-ui.xml`（放行到墙：私密区／修改密码／锁定／已解锁：0 个私密条目）；`lock.bin` 落盘 120B |
| G2 | 解锁 + 重锁 | PASS | `g2-01-relocked.png`（退后台回前台自动锁→UNLOCK 键盘）；`g2-02-wrong.png`（错密码→红点抖动＋非空 reason「密码不正确」＋✓ 置灰）；`g2-03-unlocked.xml`（对密码放行到墙） |
| G3 | 隐藏源共用 | PASS | `g3-01-locked.{png,xml}`（nyamedia 不在列表、「1 个」、锁图标 desc=解锁隐藏资源库、私密卡=数字密码保护）→ `g3-02-dialog.png`（锁图标→**应用内**数字门弹窗，非设备锁屏）→ `g3-03-unlocked.xml`（nyamedia 恢复、「2 个」、desc=锁定隐藏资源库、私密卡=已解锁）→ `g3-04-browse.xml`（Emby 浏览：服务器分类·10 个媒体库）→ 退后台自动锁 → `g3-05-gate.xml`（EmbyActivity 门「资源库已锁定／验证身份并解锁」）→ `g3-06-gate-unlocked.xml`（门内解锁后浏览恢复）。全程无设备锁屏 |
| G4 | 改密 | PASS | `g4-01-old.xml`（先输入原密码）→ `g4-02-new.xml`（新密码／输入 4–6 位数字）→ `g4-03-confirm.xml`（确认新密码／再次输入以确认）→ `g4-04-relocked.xml`（改毕自动重锁→UNLOCK）→ `g4-05-old-rejected.png`（旧密码「密码不正确」）→ `g4-06-new-works.xml`（新密码放行到墙） |
| G5 | 忘记说明 | PASS | `g5-01-forgot.{png,xml}`（UNLOCK 态「忘记密码？」→「忘记数字密码」弹窗：不设找回／不一键重置放行；指引 系统设置→应用→Kivo Cinema→存储→清除数据，并明示会清空已保存源与私密标记；「我知道了」可关闭，不静默吞点击） |
| G6 | 死代码清零 | PASS | `git grep -n -E 'BiometricPrompt\|KeyguardManager\|PrivateGate\|SourceUnlockController\|USE_BIOMETRIC' -- android/` 退出码 1（零匹配）。历史设计稿（231/237）保留作存档，不在闸内 |
| G7 | 行数闸 | PASS（带披露） | 新增家族 4 文件 99/104/124/40 行；重写 PrivateActivity 218、EmbyActivity 113、SourcesScreen 191，均 ≤300。**既有超闸**：MainActivity 357（挂 #37）、MediaLibraryActivity 334（挂 #45）；本卡未新增超闸文件（MediaLibraryActivity 由 ~350 降到 334） |
| G8 | 凭据红线 | PASS | 测试数字密码不入证据/文档（`git grep -E '<测试密码>' -- docs/evidence/p258/ docs/design/...` 零匹配）；键盘只显圆点（G1/G2 帧可见）；服务器凭据 / TMDB key 零匹配；含明文密码的表单态零截图 |

## 二、复算命令

```bash
export PATH="$PATH:/c/Android/sdk/platform-tools"; export MSYS_NO_PATHCONV=1
D=b6e950ee
# 首设（G1）：清掉测试锁后冷启动即 SETUP（测试密码自行输入，勿归档）
adb -s $D shell am force-stop com.kivo.cinema
adb -s $D shell "run-as com.kivo.cinema rm -f no_backup/privacy/lock.bin no_backup/privacy/lock.bin.bak"
adb -s $D shell am start -n com.kivo.cinema/.privatezone.PrivateActivity
# 暗色键盘坐标（PrivateActivity 内嵌 Flow）：
#   1(264,930) 2(540,930) 3(816,930) / 4(264,1170) 5(540,1170) 6(816,1170)
#   7(264,1410) 8(540,1410) 9(816,1410) / ⌫(264,1650) 0(540,1650) ✓(816,1650)
# 亮色门弹窗坐标（MainActivity / EmbyActivity 的 PrivacyUnlockDialog）：
#   1(265,860) 2(540,860) 3(815,860) / 4(265,1102) 5(540,1102) 6(815,1102)
#   7(265,1344) 8(540,1344) 9(815,1344) / ⌫(265,1586) 0(540,1586) ✓(815,1586)
# 资源库锁图标 (762,192)；私密区头部 修改密码 (842,164)、锁定 (993,164)
# 已解锁私密区 / 隐藏源 Emby 页挂 FLAG_SECURE ⇒ screencap 为空，断言一律用：
adb -s $D shell uiautomator dump /sdcard/ui.xml && adb -s $D pull /sdcard/ui.xml ./ui.xml
# G6 / G8 静态闸：
git -c safe.directory="C:/kivo video" grep -n -E 'BiometricPrompt|KeyguardManager|PrivateGate|SourceUnlockController|USE_BIOMETRIC' -- android/   # 期望退出码 1
```

## 三、披露（不静默）

1. **真机抓到并修掉两个自引入缺陷**：
   - 首设确认后 UI 停在确认页：PrivateActivity 内嵌 Flow 的 `onDone` 漏调 `SourceAccessSession.unlock()`（只 scan）。已补，G1 复跑通过。
   - 错密码时圆点「假满」：`error` 分支把 6 个点全染实色，而 `fail()` 已清空输入 ⇒ 画面像输了 6 位但 ✓ 置灰，自相矛盾。改为「实点=基色、空点=基色 22% 透明」，错态显示空红点＋红字，G2 复跑通过。
2. **FLAG_SECURE 使 screencap 为空**（已解锁私密区、隐藏源 Emby 页含门态）。这不是取证失败而是防泄露生效；相应断言改用 `uiautomator dump`（不受 SECURE 限制），XML 即证据。曾产生的 0 字节 png 已删除，不留空证据。
3. **既有超闸债不在本卡扩scope**：MainActivity 357（#37）、MediaLibraryActivity 334（#45）。本卡对二者只减不增超闸文件。
4. **测试数字密码仅运行时使用**，不入任何证据/文档/提交；键盘只显圆点。收口时已 `force-stop` 并删除测试 `lock.bin`，设备回到首设态，交由用户自设密码。
5. **nyamedia 隐藏态保留**（用户数据不动）。G3 证明隐藏源可经应用内密码恢复访问，搁浅问题解除；是否取消隐藏由用户决定。
6. 威胁模型如实：数字密码防「手机已解锁时的随手翻看」，不防离线取证；私密视频仍在 MediaStore、隐藏源凭据仍由 SourceCipher（设备 Keystore）加密。