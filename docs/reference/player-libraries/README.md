# 资源库与服务器管理研究

研究日期：2026-11-09。先研究再重构；本目录记录原版观察，Kivo 决策归入 `docs/design/ui/library-rebuild/`，截图归入 `docs/evidence/p240/library-research/`。

## 来源与证据等级

用户原件根：`C:/Users/Administrator/Pictures/REX/rex/rex`。其中有落地目录、同名 ZIP、UI 说明书、动效说明书和 CSV。原件没有遗失或换成网上资料。仓库镜像见 [REX 审计](../rex-teardown/audit/status.md)，89 份逐文件哈希在该目录 `source-hashes.json`；原包 1.0.2，手机 REX 1.1，须分别记录。

真机：iPhone13,3 / iOS 27.2 / 截图 1170×2532。经 USB 的 pymobiledevice3 DVT 截图与 CoreDevice HID 实际操作。截图只证明所见状态；按钮存在不代表功能执行成功，更不证明内部算法。

| 等级 | 本文用法 |
| --- | --- |
| 实见 | 实际进入页面、展开分区或看到既存记录 |
| 实测 | 输入未被改写，并验证动作结果；本轮没有有效的解析成功/持久保存测试 |
| 静态线索 | 来自 REX strings、元数据或拆解说明，待当前版本核实 |
| 未测 | 未执行或证据不足，不能填写为支持/不支持 |

空表单与分组截图、SHA256、操作路径见 [证据目录](../../evidence/p240/library-research/status.md)。私人服务器名、片单和账号所在截图保留在系统临时目录，不归档；以下不复述那些记录内容。

## 六款应用实际观察

| 应用 | 实际路径与页面 | 可借鉴之处 | 限制与代价 |
| --- | --- | --- | --- |
| REX 1.1 | 资源库→添加→Emby；依次查看连接/信息/线路/iCloud/高级。资源库→编辑进入分组页 | 内容源管理与连接分开；资源库名称和线路别名分离；主/备用地址；分组隐藏、排序、新建入口 | 五分区的初次填写成本较高；只看到控件，未测试线路同步、分组保存与排序结果 |
| Reflix 100.0.0 | 侧栏→添加资源库→智能解析 | 表单明确写“资源库名称，线路名称单独管理”；解析独立成页，有返回编辑；密码标可选 | 独立页增加一次往返；未测试解析填表成功、保存或线路管理页 |
| EplayerX 10.0.22 | 资源库→右上＋→Emby | 顶部快速导入；名称/备注、服务器地址、登录信息分组；HTTPS与隐私空间开关可见 | 首屏字段多，密码区需继续滚动；合成文本被输入法改写，解析试验无效；已退出未保存 |
| Vidora 1.0.6 | 资源库→＋→连接Emby服务器→更多信息 | 简洁主表单；端口、路径和线路备注渐进展开；账号信息另组 | 高级入口层级更深；复制图标的动作未执行，不能据此声称解析能力已验证 |
| Forward 1.3.18 | 资源库→添加资料库→Emby | 资源库按来源分组；快速导入带显式粘贴；名称与线路别名单独字段 | iCloud开关占用首屏；未测试导入、保存和聚合结果。本轮能打开资源库，不再沿用此前“仅见插件错误”的旧观察描述此流程 |
| Noctra 1.0.1 | 资源库媒体页左上返回→服务器管理→＋→Emby→个性化设置展开 | 管理已有源；主机/端口/路径清楚分组；名称/备注折叠；连接与保存草稿是两个动作 | 全屏海报与透明表单的对比度随图变化；截图存在悬浮感叹号，来源未确认，不作为设计元素；草稿保存、测速、导入未执行 |

数量观察：REX、Forward 看到多个既存服务器卡，其他应用也看到既存记录。**没有实测任何应用的数量上限**，也未验证跨重启保存；不能由“可添加”推导“无限”。Kivo 不设产品数量硬上限来自用户明确要求。

## REX 拆解包中能用的内容

`rex-package/strings/zh-Hans__Library.json` 共 460 键。本轮按 form、validation、groups、aggregation 域读键值，关键索引如下；中英文件均在原件审计范围内。

| 线索 | 精确键 | 当前手机验证范围 |
| --- | --- | --- |
| 分享文本识别 | `library.form.smartRecognition` / `smartRecognitionPlaceholder` / `pasteAndRecognize` | 空文本框与粘贴识别按钮实见；识别算法未测 |
| 名称、副标题 | `library.form.name` / `namePlaceholder` / `subtitle` | 信息分区实见；自动命名结果未测 |
| 地址结构 | `library.form.host` / `port` / `path` / `enableHTTPS` | 连接分区实见；路径拼接算法未解出 |
| 主备线路 | `library.form.primaryLine` / `primaryURL` / `backupServers` / `addBackupServer` / `syncLines` | 线路分区实见；同步、连通性结果未测 |
| 重复限制 | `library.form.duplicate` | 文案称相同服务器和账号只能添加一次；未触发验证 |
| 分组 | `library.groups.editor.footer` / `new.action` / `keepOne` | 编辑页的三默认组、开关、拖柄、新分组实见；未改用户布局，保存与至少一组限制未触发 |
| 聚合范围 | `library.aggregation.search` / `detail` / `favorites` / `watchlist` / `watching` / `history` / `metadata` / `reporting` | 静态线索；不能当作聚合流程实测 |
| 耗时解释 | `library.aggregation.duration.explanation` | 文案称过滤缓存、超时、异常耗时；没有测量内部实现 |
| 多设备与高级项 | `library.form.deviceTypes` / `iCloudSync` / `traktReporting` / `preferOriginalStrm`；`library.advanced.ignoreHTTPSCertificate` | 分区控件及默认外观实见，功能效果未测 |

注意密码必填相关 strings 属于共用资源库域，不能推导所有协议都必须有密码。Kivo 必须支持用户提供的空密码 Emby 与匿名 SMB。

原包为 FairPlay 加密（cryptid=1）；没有获得其 SwiftUI 业务实现。CSV、动效 MD 是研究线索，不能把其中数字宣布成已逆向确认的精确弹簧/曲线；assets JSON 是元数据，不是可直接使用的白色/透明图片。PRO 文案中另有免费库数量限制，不能泛称 REX 所有版本无限。

## 未完成的研究

1. 六款解析器：有效合成文本的填表结果、特殊字符、多个地址歧义、空密码。之前 EplayerX 输入法改写试验作废；不据此判应用失败。下一次先确认英语直输或可控粘贴，不读取用户私人剪贴板内容作测试。
2. 既存记录的管理菜单、主备线路切换、编辑失败是否保留原记录、跨进程重启登录。不能只凭本轮空表单声称全部功能了解完毕。
3. 分组拖动/隐藏/新增/删除的提交结果、聚合和离线行为。当前没有修改用户已有分组或账号。
4. 弹出、返回、键盘避让、取消恢复的连续录屏。分段截图没有帧时数据，不给动画精确值或流畅度排名。

上述缺口不需要用户重复录制已有素材；优先继续用已连接真机复现。设计中明确区分已知需求、观察启发与待验证实现。
