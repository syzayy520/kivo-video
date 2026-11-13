# 266 数据与资源 · 真机证据索引

- 设备：Redmi K40 `b6e950ee`（1080×2400，HyperOS）。取证期间锁竖屏，收口后已恢复自动旋转。
- 日期：2026-11-13。
- 构建：p266（首装）/ p266b（片段标记来源行改可钻入，见 §修正记录）。
- 设计稿：`docs/design/ui/replica/data-resources-settings.md`（闸门 §五）。
- 设备源状态：2 个非隐藏源 —— XiaoMi（SMB）、emby.bangumi.ca（Emby）；TMDB Key 已配置（首页评分徽章 8.0，c00）。

## 一、截图索引

| 文件 | 屏幕 | 要点 |
| --- | --- | --- |
| c00.png | 首页·发现 | TMDB 徽章 8.0（Key 已配置、首页聚合正常） |
| c01.png | 设置根页 | DATA 入口「数据与资源 / 语言·聚合·来源·TMDB·排序」；其余入口 intact |
| c02.png | 数据根·顶 | 语言 3 行灰 / 聚合 3 开关灰 / 数据来源前 3 行 1/2、1/1、1/1 |
| c03.png | 数据根·滚 1 | 来源行含片段标记来源（全色图标+「未接入」+›，可钻入）+ 内容加载卡顶（p266b） |
| c04.png | 数据根·滚 2 | 内容加载卡：API 地址 `https://api.tmdb.org/3` 只读 / Key 已配置 / 测试连接·图片修复灰 |
| c05.png | 数据根·滚 3 | 排序 5 行灰 + 脚注 + 媒体服务器管理 LinkRow |
| c06.png | 资源聚合(详情) | 初态：XiaoMi OFF、emby ON ⇒ 根计数 1/2 |
| c07.png | 资源聚合(详情) | 点 XiaoMi 行后：双 ON（accounts.bin 写回） |
| c08.png | 数据根 | 返回后计数复算 2/2 |
| c09.png | 资源聚合(详情) | 完全退出 DATA 再进再钻：XiaoMi 仍 ON（磁盘持久化） |
| c10.png | 资源聚合(详情) | 再点恢复 XiaoMi OFF |
| c11.png | 数据根 | 返回后计数恢复 1/2（设备状态已还原） |
| c12.png | 搜索来源 | TMDB/网络硬盘灰；XiaoMi(SMB) 灰「SMB 不参与聚合搜索」；emby 真开关 ON；AI 搜索灰 |
| c13.png | 观看记录来源 | 本机只读 ON（淡绿轨）+ Trakt 灰 |
| c14.png | 收藏数据来源 | 本机只读 ON + Trakt 灰 |
| c15.png | 待看来源 | 本机只读 ON + Trakt 灰 |
| c16.png | 数据根·滚 2' | 内容加载卡底（代理 直连）+ 排序卡 + 媒体服务器管理（p266b） |
| c17.png | 影片信息来源 | TMDB 只读 ON「固定简体中文」+ 逐源灰「元数据源优先级未接入」 |
| c18.png | 观影统计 | 本机只读 ON「本机统计，始终启用」+ Trakt 灰 |
| c19.png | 片段标记来源 | 6 源全灰「片头/片尾标记尚未接入」+ 脚注（p266b） |
| c20.png | 播放子页 | 回归：缓冲/Beta/stepper/灰行/播放习惯均正常（p266b） |
| ui-root-top.xml | uiautomator bounds | 数据根顶态 |
| ui-root-scrolled.xml | uiautomator bounds | 数据根滚 1（与 c03 同刻，钻入 tap 坐标依据） |

## 二、真实复算链（闸门 3）

c06(1/2) → tap XiaoMi(540,412) → c07(页内双 ON) → 返回 c08(根 2/2) →
完全退出 DATA 再进入再钻 c09(XiaoMi 仍 ON ⇒ accounts.bin schema2 磁盘写回) →
tap 恢复 c10 → 返回 c11(根 1/2)。

计数公式（SettingsDataRoot.kt）：`visible=非隐藏源`；`aggOn=visible.count{aggregateEnabled}`；
`emby=visible.filter{!isSmb}`；`searchOn=emby.count{aggregateSearchEnabled}`。
与 c02/c08/c11 实测一致（1/2 → 2/2 → 1/2；搜索恒 1/1）。

- SMB 双口径（D7）：c06/c07 XiaoMi 可真开关（home 聚合含 SMB）；c12 XiaoMi 灰（AggregateSession 过滤 `!isSmb`）。
- 钻入/返回栈：8 来源页均真 tap 钻入、内层 BackHandler 返回（c06–c19）；完全退出再进强制重读磁盘（c09）。
- TMDB 只读真值：c04 API 地址 = 真实 `TmdbClient.API_BASE`；Key「已配置」/ 代理「直连」读自文件，无 UI  setter。

## 三、闸门对账（设计稿 §五）

1. 视觉 PASS：c02–c05 根页 5 卡；c06–c19 八来源页节头/卡/行形态/脚注/图标计数逐行对位（灰化适配见披露）。
2. 无假状态 PASS：灰行均带非空原因（c02–c05、c12、c17–c19）；无假开关/假计数/假拖拽柄（D2 ≡ 不放置）；本机恒启用 = 只读 Switch 淡绿轨（c13–c15、c17、c18）；ON 态死 UI 不放置（D1）。
3. 真实复算 PASS：§二链；每源开关真写回 + 重读复算 + 持久化 + 还原。
4. 行数 PASS：SettingsDataSub 49 / SettingsDataRoot 82 / SettingsDataSources 142 / SettingsDataTmdb 43 / SettingsRows 265（均 ≤300）。
5. 真机 PASS：K40 b6e950ee；c01 设置根其余入口 intact + c20 播放子页无回归。
6. 证据 PASS：本目录。

## 四、披露（D1–D7，设计稿 §三）

- D1：参考 ON 态条件行（picker 弹层/自定义 API 输入/测试连接）不放置，灰行不可达。
- D2：≡ 拖拽排序柄无后端 ⇒ 不放置；脚注披露顺序固定。
- D3：「Rex」按 D6 改称「本机」（含「本机手动标记」）。
- D4：计数为真：聚合/搜索按实测源记录；本机类「1/1」；片段标记「未接入」不假计数；无源时「0/0」+空态。
- D5：API 地址显示真实 `API_BASE`（api.tmdb.org/3），真实值优先于参考措辞。
- D6：本机恒启用用只读 Switch（checked=true enabled=false 淡绿轨）+副标题，非假可切换。
- D7：SMB 搜索灰 / 聚合真。

## 五、修正记录（p266b）

首版片段标记来源行 `enabled=false`，致 MarksContent（6 灰源页）成为不可达死代码。
修正：行改可钻入（值仍「未接入」，页内全灰+原因），设计稿 §一 表行同步改「真钻入」。
重编安装（BUILD SUCCESSFUL 8s）后重取 c03/c16/c19，c20 回归亦在 p266b。
其余截图区域不受该修正影响（仅根页该行图标灰化与可点性变化）。

## 六、设备状态还原

- 源开关还原初态（XiaoMi 聚合 OFF，c10/c11）；
- 竖屏锁解除：`settings put system accelerometer_rotation 1`（已复算 get=1）；
- 未 uninstall、未清应用私有证据与测试片源；/sdcard 取证临时文件已删。