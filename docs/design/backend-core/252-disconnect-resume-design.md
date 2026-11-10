# 252 断网后恢复时延：把"在飞的读"变成可放弃的等待

状态：设计稿（未实施）
上游：250（读取链路健壮性）、251（吞吐与预读结论）
前置证据：[p250/s3](../evidence/p250/s3-link/status.md)、[p251/hard-decode-prefetch](../evidence/p251/hard-decode-prefetch/status.md)

---

## 零、结论先行

**问题**：断网 12 秒再恢复，播放要 17 秒才接上（真机两次实测 17002 / 18004 ms）。
250 只解决了"用户多久知道断了"（1 s），没解决"多久接上"。

**根因假设（待 S0 证实）**：一次 `smb2_pread` 的等待**由库内部的 `while` 循环决定**，
我们只能等它返回。断网时那个循环不会因为"我们知道没网了"而提前退出。
所以要改的不是超时数字，是**谁来跑这个循环**。

**方案**：把一次读从 `smb2_pread`（库内循环）改成 `smb2_pread_async` + **我们自己持有的
poll 循环**。等待一旦落在我们手里，就能被已有的链路看门狗随时放弃。
libsmb2 没有 cancel API，但"放弃等待 + 销毁 context"是安全的 —— 因为销毁发生在
**同一个线程**、且那一刻没有别的线程在用这个 context（Kotlin 侧已串行化）。

**判据**：网络恢复 → 播放位置继续推进，**≤ 2000 ms**（现状约 5000–6000 ms）。

---

## 一、17 秒花在哪：三个必须分开量的段

```
t0  断网
    ├─ A 察觉期：多久知道"读不回来了"          ← 250 已压到 1 s（看门狗置 stalled）
    ├─ B 等待期：在飞的那一次读还要等多久       ← 本卡的主要目标，量级未知（0~15 s）
    ├─ C 重连期：网络回来之后重建会话要多久     ← smb2 连接 + 认证 + 树连接 + open + fstat
    └─ D 退避期：gate 把下一次尝试推后多久       ← 400→4000，离线时不翻倍（250 已做）
```

已知（250/251 实测）：

- A ≈ 1000 ms（`BUFFERING 位置停止后 1000 ms`）；
- 恢复总时长 17002 / 18004 ms，扣掉 12 s 断网 → **B + C + D ≈ 5–6 s**；
- D 在"网络刚刚回来"的那一次会被 `SmbRetryGate` 主动清零（`wasOffline` 分支），
  所以 D ≈ 0；**5–6 s 全在 B + C 里，而这两段没有分开量过。**

**S0 就是把这个数拆开**：device 侧每 200 ms 采样一次链路可达性，记录
`onlineAt`（恢复时刻）与 `resumeAt`（位置再次推进），两者之差就是 B+C 的实测值；
再在 `SmbPreadChannel` 与 `SmbReader.open` 上打点，把 B 与 C 分开。

> 不先做 S0 就动手改 JNI，会陷入"改了一个数、但不知道它是不是主要项"的老路 ——
> 251 已经在这条路上栽过一次（把测量口径错当成结论）。

---

## 二、为什么"把超时调小"不是解（已被否掉一次）

250 S3 试过：`smb2_set_timeout(ctx, 3)` 返回 0，断网后 `smb2_pread` 仍然 15 s 才回。
代码层面为什么：`wait_for_reply`（`lib/sync.c:96`）是一个 `poll(1s) + smb2_timeout_pdus`
的循环，**是否退出只看"回调有没有被调过"**：

- PDU 超时靠 `pdu->timeout = time(NULL) + smb2->timeout`，秒级，最坏多等 1 s；
- 超时后 `smb2_timeout_pdus` 会用 `SMB2_STATUS_IO_TIMEOUT` 调回调，循环才会退出；
- 但"连接半死"时 `poll` 可能既不返回可读、也不返回错误（内核还没拆 socket），
  循环就只是空转 —— 直到内核层面某个计时器把它踢出来。**这个时间不由我们控制。**

结论：**只要循环在库里，我们就没有中断点。** 加参数、调数字都只是猜。

---

## 三、目标设计

### 3.1 一次可中断的读（native）

```
nativeReadInto(ctx, fh, offset, buf, off, len, abortFlag)
  └─ smb2_pread_async(ctx, fh, scratch, len, offset, cb, &state)
     while (!state.done):
        if (abortFlag->load())  return -KIVO_ABORTED        // 我们自己决定放弃
        poll(fd, events, SLICE=200ms)
        if (revents) smb2_service(ctx, revents)             // 回调在这里被调用
        if (now > deadline)     return -ETIMEDOUT           // 双保险，不是主要手段
     return state.status
```

三条纪律：

1. **放弃等待 ≠ 取消请求**。返回后 PDU 可能仍在队列里，所以调用方**必须立刻销毁 context**
   （现有 `SmbReader.close()` 已经做这件事）。不销毁就复用会把上一个请求的回复
   写进新请求的缓冲 —— 数据错乱，比卡顿严重得多。
2. **销毁只能在读线程上发生**。跨线程 `smb2_destroy_context` 会 free 掉正在被读的内存（UB）。
   本设计里 abortFlag 只是 `std::atomic<bool>`，**不碰 context**，所以看门狗线程可以安全置位。
3. **scratch 缓冲不能跨请求复用中被放弃的 PDU 写**。返回前若 PDU 未完成，
   scratch 里的内容作废；实现上"返回即销毁 context"保证没有第二次写。

### 3.2 中断信号从哪来

已经有现成的地方知道"链路没了"：`RangeFd.linkWatchdog`（1 s 一次，250 D3）。
它现在只置 `stalled`；本卡让它**同时**通知数据源放弃在飞的读：

```
RangeFd(..., online = {...}, abort = { source.abortInFlightRead() })
```

`abort` 做成可选参数（默认 null），和 `online` 同形 —— 这样 WebDAV / 本地文件不需要实现任何东西，
`RangeFd` 也不必知道"什么叫 SMB"。

### 3.3 中断之后走重连，但不吃退避

中断不是"服务器有问题"，不该被指数退避惩罚。`SmbPreadChannel` 对
`KIVO_ABORTED` 的处理：立即销毁旧 reader、立即尝试重连；若此刻仍离线，
`SmbRetryGate.canAttempt` 会直接答"不许"并给出"网络已断开"，**不阻塞**。

### 3.4 与既有纪律的接缝

| 纪律 | 本卡如何保持 |
| --- | --- |
| 只有真 EOF 才返回 -1 | 中断返回的是**负错误码**，不是 -1；`readFully` 照旧只在 `read < 0 && 非中断` 时停 |
| 不静默降级 | 中断必须带原因进日志，且 `stalled` 仍然由看门狗点亮 |
| 单线程串行访问 context | 中断只在读线程退出循环；看门狗只写 atomic |
| 不新增目录/模块 | 全部落在现有 `jni/` 与 `smb/`、`net/` |

---

## 四、自然家族树

```
android/app/src/main/cpp/jni/
    jni_smb2_poll.hpp      新  ≤110   一次 PDU 等待：poll 切片 / service / 超时 / 中断检查
    jni_smb2_read.cxx      新  ≤150   nativeReadInto 的 async 版本（从 bridge 拆出）
    jni_smb2_abort.cxx     新   ≤60   中断标志的 create / raise / destroy（不碰 context）
    jni_smb2_bridge.cxx    改 175→≤150  删掉 sync 读，只留连接/打开/关闭/错误

android/app/src/main/java/com/kivo/cinema/
    smb/SmbAbortFlag.kt    新   ≤70   native 标志句柄的生命周期（close 必须释放）
    smb/SmbReader.kt       改 97→≤115  pread 传标志；requestAbort()；中断后禁止复用
    smb/SmbPreadChannel.kt 改 96→≤110  中断 → 立即重连，不进退避
    smb/SmbByteSource.kt   改 177→≤180 暴露 abortInFlightRead()（只转发）
    net/RangeFd.kt         改 198→≤205 看门狗发现离线时调用 abort
    ui/compose/PlayerComposeActivity.kt 改 +2 行 ×2 处（视频 / 软解音频接线）
```

**家族树判据（可验证）**：换传输层时只改 `jni_smb2_poll.hpp` 一个文件
（它把"等待"抽出来了，读语义、Kotlin 编排、UI 全都不动）。

不做的事：不新建目录、不新建 Gradle module、不给 `ByteSource` 加抽象方法
（用可选参数接线，避免每个实现都被迫改）。

---

## 五、分阶段与闸门

| 阶段 | 内容 | 判据 |
| --- | --- | --- |
| **S0（先做）** | 只加度量，不改行为：`SmbReconnectInstrumentation` 每 200 ms 采样链路状态，记录 `offlineAt` / `onlineAt` / `resumeAt`；`SmbPreadChannel` 与 `SmbReader.open` 打点（进 instrument 输出，不进 logcat） | 得到 **B / C 的实测值**；若 C 已占 4 s 以上，则先优化重连（见 §六），中断读不是主要项 |
| **S1** | native：`jni_smb2_poll.hpp` + `jni_smb2_read.cxx` + abort 标志；Kotlin：`SmbAbortFlag` | `abort` 置位后读在 ≤250 ms 返回；不断网时吞吐与 251 基线一致（WINDOW 7.2±0.5 MB/s） |
| **S2** | `RangeFd` 看门狗接线 + `SmbPreadChannel` 中断即重连 | 断 12 s：恢复后 ≤2 s 位置推进；`BUFFERING` 仍 ≤1 s |
| **S3** | 证据归档 + 参数表 + 交接 | G7：参数表逐条 grep；设计稿与实现零漂移 |

闸门：

- **G1**：只改上述 9 个文件；`git diff --stat` 可核对。
- **G2**：单文件不超预算；超了按"改动理由"拆，不凑行数。
- **G3**：存量文件变长逐条写理由（预计只有 `SmbReader` / `SmbPreadChannel` / `RangeFd`）。
- **G4**：不动退避数字、不动块窗口参数、不动预读策略（251 刚定案）。
- **G5**：真机判据必须来自 instrument 输出，不用 logcat（MIUI 只给 64 KiB）。
- **G6**：`assembleDebug` + JVM 单测 47 例 + seek 4/4 + 断链 5/5 全绿。
- **G7**：设计稿 §八 参数表与代码逐条对齐。

---

## 六、明确不做

| 不做 | 理由 |
| --- | --- |
| 跨线程 `smb2_destroy_context` | UB，会 free 掉正在被读的内存 |
| `pthread_kill` / `Thread.stop` | Android 上线程无法安全终止；打断点必须在自己的循环里 |
| 把 `smb2->timeout` 继续调小 | 250 已证不生效；本卡不再用"调数字"当手段 |
| 加第二条连接做重连 | 251 已证"加连接为负"（同一 NAS 串行化） |
| 改退避序列 400→4000 | 那是保护服务器的，不是时延的主要项（D≈0） |

---

## 七、风险

| 风险 | 处置 |
| --- | --- |
| async 与 sync 语义差异（credits / 回调时机） | S1 先只替换读、不动连接；对比 251 基线的吞吐与 seek 落点 |
| 中断后 PDU 悬空被复用 | 返回即销毁 context；`SmbReader` 置 `aborted` 标志，任何后续调用直接抛错 |
| 看门狗误判（网络正常但探针失败）导致误中断 | 探针只报"离线"才中断；中断后立刻重连，最坏代价是一次重连（≈300 ms） |
| 软解音频与视频各有一条源 | abort 按源接线，互不影响；`PlayerComposeActivity` 两处都要接 |
| 新增 native 文件漏进 CMake | S1 第一件事就是把 `jni_smb2_read.cxx` / `jni_smb2_abort.cxx` 加进 `CMakeLists.txt` |

---

## 八、参数表（G7 闸门）

| 参数 | 值 | 位置 | 依据 |
| --- | --- | --- | --- |
| `ABORT_POLL_SLICE_MS` | 200 | `jni_smb2_poll.hpp` | 中断反应上限；20 次/秒空转，功耗可忽略 |
| `READ_DEADLINE_MS` | 3 000 | 同上 | 沿用 250 的读超时；中断是主手段，它只是双保险 |
| `LINK_POLL_MS` | 1 000 | `RangeFd` | 沿用 250，不改 |
| `RESUME_TARGET_MS` | 2 000 | 验收判据 | 用户可感知的"卡一下"上限 |

---

## 九、证据归档

- S0：`docs/evidence/p252/s0-attribution/status.md`（B/C 拆解）
- S1：`docs/evidence/p252/s1-abortable-read/status.md`（吞吐与 seek 回归）
- S2：`docs/evidence/p252/s2-resume/status.md`（恢复时延）
