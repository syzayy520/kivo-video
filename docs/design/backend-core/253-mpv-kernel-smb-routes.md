# 253 SMB 路线：A / B / C 三条路的成本与决策

状态：**设计稿**
职责：**只回答一件事**：让 libmpv 读到 NAS 上的文件有哪几条技术路线，各自的成本、风险与取舍，以及按阶段的决策。
家族：253 换内核到 libmpv（总纲见 [253-mpv-kernel-plan.md](253-mpv-kernel-plan.md)，家族树见 [253-mpv-kernel-family-tree.md](253-mpv-kernel-family-tree.md)）
前置证据：[p252/senplayer-arch](../../evidence/p252/senplayer-arch/status.md)

---

## 五、SMB 路线：3 条方案（**P1 先不碰，P2 再定**）

### 方案 A：ffmpeg + libsmbclient（与 SenPlayer 同路）

- **做法**：交叉编译 Samba 的 `libsmbclient`（LGPLv3），ffmpeg 加
  `--enable-libsmbclient`，mpv 直接开 `smb://`。
- **成本**：Samba 交叉编译（talloc/tevent/ldb 依赖链）**风险最高**，
  但 mpv-android 社区有现成 PR 可抄。
- **代价**：凭据进 URL（安全）、丢掉我们的块窗口/预读/断网恢复。

### 方案 B：我们的 libsmb2 通过 FFmpeg 自定义协议注入（**推荐**）

- **做法**：FFmpeg 允许第三方注册 `AVIOProtocol`。在 mpv 侧注册一个
  `kivo-smb://` 协议，实现 IO 回调 → 走我们的 `SmbByteSource`（现有 Kotlin/Java 实现，
  或下沉一个 C 版）。
- **成本**：改 mpv 源码（加一个 `stream/stream_kivosmb.c`）+ 把 SMB 客户端下沉到 C
  （或用 JNI 反向回调，但 IO 回调在解码线程，回 JNI 有性能与死锁风险 ⇒ **建议下沉 C**）。
- **收益**：保住我们全部 SMB 资产（块窗口/预读/读画像/断网恢复），凭据不进 URL。
- **风险**：改 mpv 源码意味着要维护 patch。

### 方案 C：本地代理（最快出活，先解燃眉）

- **做法**：App 内起一个**本地 HTTP 服务**（或已有 `RangeFd` 抽象），把 SMB 文件
  以 `http://127.0.0.1:port/…` 暴露给 mpv，mpv 用 `http` 协议 + range 请求。
- **成本**：最小（复用现有 `RangeFd`/代理能力），**1~2 天能出可播版本**。
- **代价**：多一跳拷贝；本地 HTTP 服务在移动端后台被杀的风险。
  但**只作过渡**，P4 换成方案 B。

### 决策

| 阶段 | 用哪条 | 理由 |
| --- | --- | --- |
| P1 内核打通 | **不接 SMB**（本地文件 / 已挂载路径） | 把"内核能不能跑"与"SMB 能不能接"解耦 |
| P2 可播版本 | **方案 C** | 最快让用户能看片 |
| P3 正式版 | **方案 B**（备选 A） | 保住我们的 SMB 资产 |

---
