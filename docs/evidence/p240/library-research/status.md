# 资源库研究证据 · 2026-11-09

状态：研究阶段，非 Kivo 功能验收。没有构建或安装本轮资源库重构包；既有分页 WIP 未安装，不视为交付。

设备与方法：iPhone13,3 / iOS 27.2，DVT screenshot 1170×2532；CoreDevice universal-hid-service 点击。日期按工作会话记录；手机状态栏时钟与工作机时间不混用。应用版本与范围见 [研究矩阵](../../../reference/player-libraries/README.md)。

## 归档规则和文件

只归档肉眼核查过的空表单、默认分组页面。没有复制已有账号、服务器卡片或私人片单。`manifest.json` 记录每张原尺寸 PNG 的哈希、页面含义和操作路径；本轮不是像素差异验收，也未测动画时长。

- `forms/rex-*.png`：连接、信息、线路、同步、高级、分组六张。
- `forms/reflix-*.png`：添加与智能解析空页两张。
- `forms/eplayerx-emby.png`：空添加表单一张。
- `forms/vidora-*.png`：空表单与展开地址两张。
- `forms/forward-emby.png`：空添加表单一张。
- `forms/noctra-*.png`：空添加表单与个性化展开两张。

共14张。原始空表单未作裁剪、改色、模糊或重绘。Noctra 有来源未确认的悬浮感叹号，保留以反映实际画面；不纳入 Kivo 设计。

## 无效和未归档观察

临时 `vidora-emby.png`、`noctra-emby.png` 实际是资源库内容页，文件名不代表验证成功，未采用。EplayerX 合成文本被第三方中文输入法改写（标点、换行与字符变化），该解析试验作废，既不判成功也不判失败；退出空表单，未保存。

未提交任何测试记录、未删除或编辑现有服务器、未授权网盘登录、未切换已有设置。REX 分组页只观察，未拖动、改开关或新建。

## 复核

从仓库根在 PowerShell 执行：

```powershell
$root = 'docs/evidence/p240/library-research'
$manifest = Get-Content "$root/manifest.json" -Raw | ConvertFrom-Json
foreach ($entry in $manifest.screenshots) {
    if ((Get-FileHash "$root/$($entry.file)" -Algorithm SHA256).Hash.ToLowerInvariant() -ne $entry.sha256) {
        throw "Screenshot hash mismatch: $($entry.file)"
    }
}
```

哈希证明归档文件未变，不证明原版动作成功。后续解析、持久化和管理流程仍需单独证据。
