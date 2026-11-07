# 230 · DLNA 投屏 —— 状态稿（收口）

> 日期：2026-11-08 · 设备：Redmi K40（b6e950ee，192.168.5.152）·
> 渲染端：PC 测试件（`tools/dlna_renderer.py` + ffplay）+ **真实设备小爱音箱-8245**（Mi GUPnP，192.168.5.145）
> 结论：**AC1–AC4 全部真机通过**。证据 `docs/evidence/p230/`，校验器 `tools/verify_p230_dlna.py`（5/5 变异自检）。

---

## 一、一句话结果

**播放器点「投」→ SSDP 发现（单播扫段）找到 PC 测试渲染器 + 真小爱音箱 →
投本地文件（手机 LAN 代理）与投 Emby URL（渲染端直取流）都能在 ffplay 出画面 →
BACK 触发 Stop，渲染端日志可证——协议自研零第三方。**

## 二、实测矩阵

| # | 判据 | 实测 | 证据 |
| --- | --- | --- | --- |
| AC1 | 发现 | `cast scan done: 2 devices 小爱音箱-8245;kivo-test-renderer`——真设备与测试件同轮发现 | `p230-status-cast.log` |
| AC2 | 投本地 | `FileByteSource → LocalRangeProxy(0.0.0.0 :43173)` → 渲染器 ffplay 渲染 1920×1080 彩条片 | `p230-pc-ffplay.png`（窗口标题即代理 URL） |
| AC3 | URL 直传 | Emby 直连流 URL 经 SetAVTransportURI 交渲染器，**渲染端自行向 Emby 取流**（手机不在数据路径），ffplay 出《FINAL DESTINATION 2》 | `p230-pc-emby2.png`、手机状态栏「投屏中：kivo-test-renderer」 |
| AC4 | 控制 | `SOAP Stop` 两次生效（本地片 03:00:38 / Emby 片 03:05:47），渲染端 `Stop received + ffplay killed` | `dlna_renderer.log`（PC 侧，未入库） |

## 三、与设计稿的差异（三条）

1. **发现路径以单播扫段为主**（设计稿 D1 原以组播 M-SEARCH 为主）：真机实测 **MIUI 把手机的组播发送
   在驱动层丢弃**（nc 单播到小爱正常、组播三连发无应答——两个独立接收端都没收到）。
   扫描现为「组播三连发 + /24 单播扫段（每主机 1900 + 1901 双端口）+ 同窗收 alive」三路径并行。
2. **PC 测试渲染器用 1901 端口收单播 M-SEARCH**（1900 被 Windows SSDPSRV 的 WFP 重定向吃掉，
   服务停了也拦——实测）。1901 是测试件约定端口；真设备（小爱）照常走 1900。
3. Pause 未接线（设计稿 D3 有 Pause 动作）：UI 无入口，`AvTransportClient.pause` 已实现——v1 豁免明示。

## 四、本卡踩的坑（下次直接用）

1. **MIUI 组播发送被驱动丢弃**：UDP 单播收发全通、组播发送无影——任何 DLNA/mDNS 类功能在这台
   K40 上不能依赖组播发送，扫段/单播发现是正解。
2. **Windows 对 1900 的 WFP 重定向**：SSDPSRV 即使 stop，1900 单播仍进不了用户进程；测试件换 1901。
3. **Windows 防火墙对 LAN UDP 入站按程序+端口过滤不足**：`remoteip=192.168.5.0/24` 子网级放行规则
   才让手机包进来（单独的 1900 端口规则不够）。测试后已恢复原状。
4. **XmlPullParser 的标签间空白也是 TEXT 事件**：`</serviceType>` 后的换行会把刚置的服务匹配状态
   重置——解析设备描述必须跳过空白 TEXT（此 bug 让两台设备的 controlURL 全部解析失败）。
5. **uiautomator dump 会静默返回陈旧内容**：连点投屏按钮的自动化里，dump 坐标可能来自上一次。
   盲按固定坐标 + 立即截图比对更可靠。
6. 多次 `am start` 会叠加多个 PlayerComposeActivity 实例——投屏链路测试前先 `am force-stop`，
   否则老实例（intent 无 playurl）会抢走投屏动作。

## 五、豁免与未做（明示，不藏）

- GENA 事件订阅（进度回传/状态推送）：设计稿 §六已排除。
- 小爱音箱只验证到「发现+解析」（设备列表可见、friendlyName 正确），**未向它 Set+Play**
  （音箱无屏，视频直连流会只出声音——验证价值低，PC 渲染端已出完整画面）。
- SMB 片源投屏（D4 表里的第三行）：CastController.castSource 接口已支持任意 ByteSource，
  但 UI 未接线（SMB 会话需第二 SmbByteSource）——v2。
- 手机侧投屏后的进度条/双屏联动：设计稿 §六已排除。

## 六、环境收尾记录

测试期间改动的本机状态：Windows 防火墙 privateprofile 关闭 + SSDPSRV 停止——
收口时已恢复（`net start SSDPSRV` / 防火墙 profile 回开）；`kivo-lan-udp` 等放行规则保留。
