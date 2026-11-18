# D12-I 状态：代理把 Content-Type 写死成 matroska，导致所有 `.ts` 播不了（已修）

日期：2026-11-18
上游：`D12H-STATUS.md` §四「`.ts` 问题新边界」
结果：**根因不是 NAS、不是文件、不是 SMB，是本地代理的一行响应头**。

---

## 一、根因

`mpv/LocalRangeResponder.kt` 构造 HTTP 响应头时写死：

```kotlin
header.append("Content-Type: video/x-matroska\r\n")
```

ffmpeg 会采信这个类型并**只挑对应的 demuxer**，于是非 matroska 的片源在探测阶段就撞：

```
[ffmpeg/demuxer] matroska,webm: EBML header parsing failed
[cplayer] Failed to recognize file format.
```

### 怎么定到这一行的

三段排除，每段都有实测：

| 步骤 | 做法 | 结果 |
|---|---|---|
| 1 | 同 NAS 同共享的两个 VC-1 片源对比 | `.mkv` 能播、`.ts` 不能 ⇒ 与 VC-1 无关（D12H） |
| 2 | **自己造**一个 40 s / 2.4 MB 的标准 mpegts，传到同一共享播 | **仍然失败** ⇒ 与"那个 33.8 GB 文件"无关 |
| 3 | 同一个 `.ts` 推到设备本地播（走 `FileByteSource`，**不经 SMB**） | **仍然失败** ⇒ 与 SMB 无关 |

第 3 步把范围压到"本地代理 + 容器"之后，读 `respondGet` 的响应头就看到了那行硬编码。

---

## 二、修复

```kotlin
// ByteSource 不提供 mime ⇒ 用 octet-stream 让 ffmpeg 自己做完整嗅探
header.append("Content-Type: application/octet-stream\r\n")
```

一行改动。`ByteSource` 接口只有 `size()` / `readAt()`，没有 mime，所以"按扩展名给准确类型"
需要额外加接口——没必要，`application/octet-stream` 是通用二进制类型，不会误导 demuxer。

---

## 三、验证（K40，三种容器）

| 容器 | 修复前 | 修复后内核日志 |
|---|---|---|
| `.ts`（本地 40 s 测试片） | ❌ `Failed to recognize file format` | ✅ `Found 'mpegts' at score=50 size=2048` → `Detected file format: mpegts` |
| `.ts`（NAS 33.8 GB VC-1） | ❌ 同上 | ✅ `Detected file format: mpegts` + `Opening decoder vc1` + `Opening decoder dca` |
| `.mp4`（手机拍摄） | ✅（**未受影响**） | ✅ `Detected file format: mov,mp4,m4a,3gp,3g2,mj2` |
| `.mkv`（3 分钟测试片） | ✅（**未受影响**） | ✅ `Detected file format: Matroska` |

**出画确认**：`.ts` 测试片显示标准彩条；NAS 那个 33.8 GB 的播到梦工厂片头（月亮钓鱼）。

### 一个诚实的边界

**`.mp4` 和 `.mkv` 在修复前就能播**，只有 `.ts` 受影响。所以这不是"所有格式都坏了"，
而是"只有 mpegts 探测被那行 content-type 卡住"。ffmpeg 内部差异我没深究
（不是本轮目标），但修复是**朝更宽松的方向**走（octet-stream ⊃ 任何具体 mime），
三种容器的回归都验证过。

---

## 四、对前几轮结论的修正

| 文档 | 原结论 | 修正 |
|---|---|---|
| `D12F-STATUS.md` §三 | "根因落在 SMB 大文件 + 远 seek + 代理 range 组合" | **错**。是代理 content-type |
| `D12G-STATUS.md` §二 | 排除续播/大小/VC-1，剩"该文件不可读 / .ts 走 SMB 链路" | **都错**。与文件和 SMB 都无关 |
| `D12H-STATUS.md` §四 | 同上两种可能 | **都错**。是本地代理 |

三次归因都指向"外部环境"，因为**验证从未离开过"经代理"这条路**——
直到第 3 步把文件推到本地、仍然失败，才意识到代理本身在链路里。

> 教训：当一个格式"在哪都播不了"时，先怀疑**共同的那一段**（这里就是代理），
> 而不是逐个排除外围（NAS、文件、SMB）。

---

## 五、影响面

`.ts` 是蓝光转录、IPTV 录制、部分摄像机输出的常见容器。
修复前这些文件**在任何入口都播不了**（本地/SMB 都经代理），且错误信息只有
"Failed to recognize file format"，看不出是代理的问题。

---

## 六、证据清单

| 文件 | 内容 |
|---|---|
| `d12i-ts-fixed.png` | 修复后 `.ts` 测试片出画（彩条） |
| `d12i-nas-ts.png` | 修复后 NAS 33.8 GB `.ts` 出画（梦工厂片头） |
| `d12i-probe.ts` | 自造 mpegts 测试片（40 s，未入库，生成命令见下） |
| `build-d12i.log` | 编译日志 |

```bash
ffmpeg -y -f lavfi -i "testsrc=duration=40:size=1280x720:rate=25" \
       -f lavfi -i "sine=frequency=440:duration=40" \
       -c:v libx264 -preset ultrafast -g 50 -pix_fmt yuv420p -c:a aac -f mpegts d12i-probe.ts
```

---

## 七、未覆盖

1. `.avi` / `.flv` / `.m2ts` 等其它容器**未逐一验证**（理论上同一修复覆盖，但没有实测）。
2. `LocalRangeResponder.kt` 现 **433 行**（原 427，本次 +6 行注释），仍超 300 存量上限，
   属 B5 拆分范围。
3. ffmpeg 内部"为什么只有 mpegts 被 mime 卡住、mp4/mkv 不受影响"**未深究**。
