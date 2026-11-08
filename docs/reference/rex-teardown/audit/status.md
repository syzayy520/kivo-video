# REA 拆解资料审计索引

生成工具：`tools/reference/rex/audit_package.py`。日期：2026-11-08。

## 实际清点

- 仓库包内文件 89 份；原件哈希一致 89，不一致 0，原件未找到/未提供 0。逐文件结果见 [source-hashes.json](source-hashes.json)。
- 动效 CSV 实际 32 个场景（不计表头），模块：PRO、全局、列表/详情、在看、导航、小恐龙、小恐龙/播放器、播放器、设置、详情、首页、首页/列表、首页/编辑器。见 [interactions.csv](interactions.csv)。未逐条做运行时验证；原等级列原样保留。
- 文案 JSON 78 份，中文模块 39 个；关键词筛出的中英文候选 333 条，见 [string-evidence.csv](string-evidence.csv)。这不是已验证交互总数，也不是所有交互的穷尽证明。
- 外层原始图像 0 份；ZIP 内清单见 [archive-inventory.json](archive-inventory.json)。`assets/*.json` 是资产元数据，不能直接作为图片使用。

## 哪些能用、怎样用

| 资料 | 可直接利用 | 必须再验证 |
| --- | --- | --- |
| 中英文 strings | 文案、无障碍语义、错误类别、待查能力线索 | 当前手机版入口、默认值、动作完整状态机 |
| assetutil 元数据 | 资产名、尺寸/色彩声明、图层或图像族线索 | 原始图层位图能否获取；运行时布局、裁切、混合方式 |
| 动效 CSV/拆解 MD | 场景索引、触发与取消条件候选、资源关联 | 时长、弹簧、延迟、位移、手势阈值与对象适用性 |
| 旧截图/录屏 | 已知版本/状态下的画面及有限时序 | 同素材、同设置对照与帧率误差 |
| 当前 USB 真机 | 可重现操作与最新版本结构 | 单张截图不证明动画曲线；按需连续录像 |

原始包标注 Rex 1.0.2，当前 iPhone Rex 1.1。FairPlay cryptid=1；不能声称已从加密 SwiftUI 代码提取精确参数。启动屏 393×852 也不证明全应用固定设计宽。

## 排期和缺口

首页/详情滚动、片名独立与下拉分层的最新截图见 `docs/evidence/p240/layered-scroll/`。本次没有把这些观察推广为所有 CSV 场景实测。

下一步 R1 优先核对详情操作和首页切页/刷新/预告取消，其余模块逐页登记；播放器单独 R5，PRO 不纳入。当前资料足够开始这些盘点，不需用户重录已有内容。只有 USB 无法重现的特定状态，才说明缺哪个场景、起止状态和录制要求。

新规划与旧结论处置见 `docs/design/ui/replica/roadmap.md` 和 `review.md`。复算命令：

```powershell
python tools/reference/rex/audit_package.py --source-root 'C:/Users/Administrator/Pictures/REX/rex/rex'
```
