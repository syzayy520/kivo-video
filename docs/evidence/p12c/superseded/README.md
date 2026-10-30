# 已废弃的证据（保留原样，不要拿来复算）

这里放的是**测量定义写错**的那一版证据，保留是为了让"错在哪"可复核，
而不是为了当结论引用。

## `p12c8-av-r1*.json`

- 日期：2026-10-29
- schema：`kivo.p12c.sync/1`（字段布局比正式版少：没有 `timeline`，`av_drift` 没有 `measurable`）
- 错在哪：`av_drift` 采在 `FramePacer::arrive()`，而 `arrive()` 里的 `lateness_ns`
  在 `kSurfaceAtTime` 路径下等于**提交提前量**，不是音画偏差。
- 同一轮的三个数互相矛盾，正是这个矛盾的提示：

  | 读数 | 数值 |
  | --- | --- |
  | `av_drift` p50 | −133.18 ms |
  | `schedule_lead` p50 | −99.875 ms（= 3 帧） |
  | `lateness` 样本数 | **0** |
  | `av_drift` 直方图 `below`（比 −128 ms 还低） | **174 / 180** |

- 修正后口径见 `docs/design/backend-core/24-p12c8-design.md` §四，
  完整推导与实测见 `25-p12c8-status.md` §三。
- **`tools/verify_p12c_sync.py` 会在这两份上失败，这是预期的** ——
  它们缺 `measurable` / `timeline`，属于旧布局。别把失败当成脚本坏了。
