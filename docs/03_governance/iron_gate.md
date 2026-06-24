# Kivo Video 铁律门闸

本仓库所有 Kivo Video / KivoCinemaEngine 任务必须遵守本门闸。

## 最高流程

```text
构思 → 规划设计 → 设计自然家族树 → 实现 → 检查 → 最小验证 → 总结
```

## 不可违反的工程铁律

1. **构思先行**：没有定位、目标、非目标、边界，不允许写代码。
2. **规划设计先行**：没有任务分层、允许文件、禁止文件、验收标准，不允许写代码。
3. **自然家族树先行**：文件夹必须代表功能族；子文件夹必须代表子功能族；单文件必须是最小职责单元。
4. **禁止平铺文件**：不得把大量无层级文件直接堆在同一目录。
5. **单文件单职责**：一个文件只能承载一个清晰职责，不得变成 manager / helper / utils 大杂烩。
6. **单轮单家族**：每轮最多推进一个自然家族，禁止 source / plan / media / decision 并行乱写。
7. **合同优先**：P0 只冻结合同、状态边界、事件与治理规则；不得抢跑真实播放实现。
8. **Adapter 边界**：FFmpeg / D3D / DXGI / WASAPI / Win32 / libass / MediaInfo / 云盘 SDK 只能进入 adapter 家族。
9. **状态唯一 owner**：authoritative state 必须有唯一写入者。
10. **无静默降级**：任何 fallback 必须有 reason、trace、quality impact 和 user hint。

## STOP 条件

出现以下任一情况必须停止：

- 未规划先写代码。
- 出现 unknown 文件或 unknown 生成目标。
- 一次跨多个核心家族生成实现。
- C++ business core include 原生第三方头。
- UI 直接拼接 source URL、HDR/DV/音频 badge 或 fallback 文案。
- 无法解释文件职责、owner、依赖方向。
