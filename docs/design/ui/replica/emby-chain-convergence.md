# 257 Emby 链收敛：核销 #18–#25 + 退役死代码 + 真机回归

> 性质：收敛卡，不是功能卡。台账上 #18–#25 八张 in_progress/pending 卡，代码实际已随
> 254/255/256 演进完毕或取代，台账与代码不同步。本卡逐条对代码现状核销：
> 已完成的补真机回归后关闭；被取代的正式退役；真缺的才补。**不新增功能、不新建包。**

## 一、逐条审计（2026-11-13，代码现状）

| 卡 | 原判 | 现状证据 | 处置 |
| --- | --- | --- | --- |
| #18 defaultHome 误路由 + intent 契约 | 修 | `SourceOpen.kt:10` 协议→浏览页单一入口（defaultHome 与 onOpenSource 共用）；`MainActivity.kt:56,81,93,222` 一次性 handled 标志 + savedInstanceState 持久化（旋转不重放）；`EmbyActivity.kt:40-47` 无 sourceId 回主页（254 D15） | 核销 + 回归 R-1 |
| #19 Emby 进 mpv + 预览帧 + token 离 URL | 做 | `PlayerSessionStart.kt:25-28` playurl 分发 → `PlayerUrlStart.kt:16-23` `HttpByteSource(url, token)` 本地代理 + thumbSource 预览帧；`EmbyClient.kt:135-143` 流 URL static=true、token 只走 X-Emby-Token 头；`net/HttpByteSource.kt:9` mpv 日志只见 localhost | 核销 + 回归 R-2/R-3 + 日志断言 |
| #20 死代码清理 | 清 | fake_emby 全仓零匹配（前期已清）；`EmbySourceForm` 裸词有引用（`SourcesScreen.kt:184`）是 workspace 内容活外壳 → **留**；**Pages.kt 存活**（见下更正） | 核销：已清完，本卡零删除 |
| #21 真机闭环 + 分页 WIP | 验 | 钻取网格分页已接线：`EmbyLibraryScreen.kt:66-70` nearEnd→loadMore + 错误重试；聚合分页 256 已取证（R1 闸门）；旧「全库浏览 WIP」被 255 分类行+钻取取代 | 核销 + 回归 R-4 |
| #22 收口提交推送 | 收 | 本卡收口动作 | 本卡执行 |
| #23 海报→复用详情页 | 做 | `EmbyActivity.kt:86-88` 与 `AggregateSource.kt:22-23` 均 → `MovieDetailActivity.embyIntent`（TMDB 智能填充 + Emby 资源槽 `EmbyResource.kt`）；本地仍走 `DetailActivity`（237 契约），两详情各司其职 | 核销（255/256 已真机验） |
| #24 连接表单补全 | 补 | `SourceConnectionContent.kt:66,116` 剪贴板智能识别→预览→applyPreview；`:71` 名称（空则主机名）；`SourceOptionalFields.kt:46-73` 协议 https/http + 端口 + 备注，全部派生自唯一地址态 | 核销 + 回归 R-5 |
| #25 退役旧连接表单 | 退 | 254 D15：`EmbyActivity.kt:42-46` 无 sourceId 一律回主页，添加唯一入口 = 资源库 workspace 流；manifest 无旧连接 Activity | 核销（代码已退役） |

## 二、家族树（两处真改动，均不新建包/文件）

```text
emby/EmbyActivity.kt            修：FLAG_SECURE 改由读取回调驱动（onCreate 默认挂 SECURE，
                                读取后非隐藏源清除）——修隐藏源首帧防泄露与非隐藏源截图被挡两件事
sources/ui/SourcesScreen.kt     删：SMB 入口卡（用户指令「SMB那个卡片给我删了」）；
                                已存 SMB 源卡与类型_sheet 添加入口保留
```

其余零改动：#18–#25 的代码现状已达成，只补真机证据后关闭台账。

### 审计自更正（2026-11-13，实现前复核抓到）

首版审计写「`Pages.kt` 全仓裸词零引用 → 删」，**这是错的**。当时只 grep 了文件名
`Pages`（import 按符号名，自然零匹配），没查文件里定义的符号。删除前逐符号复核：

| 符号 | 引用 | 判定 |
| --- | --- | --- |
| `SettingsPage` | `MainActivity.kt:41,316`（设置 tab） | 活 |
| `ProfilePage` | `MainActivity.kt:40,315`（我的 tab） | 活 |
| `relDays` | `WatchingPage.kt:89`、`DiscoverPage.kt:148` | 活 |
| `fmtSeen` | `WatchingPage.kt:107`、`DiscoverPage.kt:149` | 活 |
| `formatDate` | `Pages.kt:121`（relDays 内部）；`MediaLibraryActivity` 那个是同名私有函数，无关 | 活 |

**结论：Pages.kt 是设置页 + 我的页 + 共用时间格式化，全部存活，不删。**
死代码清理（#20）实际早已完成：fake_emby 全仓零匹配。教训：判死代码必须查
**符号引用**，不是文件名；删除前最后一次复核不能省。

## 三、回归闸门（K40 b6e950ee 真机）

| # | 闸门 | 判据 |
| --- | --- | --- |
| R-1 | defaultHome 往返 | 管理页设 nyamedia 默认首页 → force-stop 冷启直接进 EmbyActivity；取消后冷启回发现页 |
| R-2 | Emby 起播走 mpv 代理 | 详情→播放出画面；`files/mpv-kernel.log` 无 token/无服务器主机名于 URL（只 localhost） |
| R-3 | 预览帧走代理 | 拖动进度条出预览缩略图（thumbSource 同代理链） |
| R-4 | 钻取分页 | 电影钻取网格滚到底自动追加（前后计数增长），错误态有重试 |
| R-5 | 添加流智能识别 | 粘贴 `https://emby.bangumi.ca:443` → 预览解析出协议/主机/端口；名称空则主机名；选填展开有协议/端口/备注 |

凭据红线：回归只用无密码源粘贴串；含密码的表单态**不截图入库**。

## 四、证据

`docs/evidence/p257/emby-convergence/`（闸门表 + 帧 + 日志断言复算命令）。

## 五、回归结果与披露（2026-11-13 真机 b6e950ee）

| # | 结果 | 证据 |
| --- | --- | --- |
| R-1 | PASS | r1-01…r1-10：设默认首页冷启进 Emby、取消回发现页；隐藏开关往返 |
| R-2 | PASS（带披露） | r2-01-play.png 真画面；r2-token-assertion.txt：URL 仅 `http://127.0.0.1:<port>/1`，host/token 扫描 0 命中 |
| R-3 | 部分 PASS | r3-01：拖动目标时间+进度条跟随成立；**Emby 预览帧未出**——预览内核冷启动加载超时，原因可见（不静默）；SMB 预览此前卡已验。缺口转 #40 |
| R-4 | PASS | r4-02→r4-03：钻取头「电影 - 欧美」，60/207 → 207/207 + 尾标「已显示全部电影」 |
| R-5 | PASS | r5-01…r5-04：类型 sheet→表单→智能识别解析→选填协议/端口/备注 |
| R-6 | PASS | r6-01：SMB 入口卡已删，本地/私密/已存 SMB 卡仍在 |

披露（全部已落证据或转卡，不静默）：

1. **SECURE 缺陷已修**：旧 `SideEffect` 清 FLAG_SECURE 不生效，非隐藏源截图 0 字节；
   改读取回调驱动后非隐藏源可截图、隐藏源首帧仍防泄露（r1-05 起截图正常）。
2. **Emby 冷起播首开瞬败**：主内核首开 mp4 header EOF（1MB/0 seeks，见
   r2-cold-failure-excerpt.txt），重试恢复并出画面；服务器侧 Range 契约经 curl 实测
   完好（206 + Content-Range 总长正确），设备侧代理头/1MB 偏移/文件尾实测均正确。
   根因未定论（疑双实例冷并发读被截断），转 #40。
3. **预览内核冷超时后重试不重建**：转 #40（含 mpv 两日志交叉写入的诊断坑）。
4. **nyamedia 隐藏态搁浅**：无锁屏密码设备无法解锁隐藏源，恢复并入 #39（应用内独立数字密码重构）。
5. 环境坑：MIUI logcat 无应用日志；`locksettings set-pin` 被拒；ModalBottomSheet .92f
   裁底部按钮触摸区；重复账号守卫挡 nyamedia 重加。
6. 带宽披露：bangumi 实测 ~400KB/s，4K 28.8GB 片源播放伴随周期性 buffering（网络所致，非代码）。