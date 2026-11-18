# D12-G 状态：SMB 续播键串台（P0，已修）+ 33.8 GB .ts 读不出来（未解）

日期：2026-11-18
上游：`D12F-STATUS.md` §三「SMB 大文件(.ts) 续播起播失败」
结果：**根因不是 .ts、不是大文件，是续播键算错**。已修；剩余一个独立问题未解。

---

## 一、P0 缺陷：所有 SMB 影片共用一个续播键（已修）

### 现象

播 `A Beautiful Mind`（33.8 GB `.ts`）黑屏，内核日志：

```
Run command: loadfile, ... options="start=3574.582"     ← 从 59:34 起播
PROXY REQ range=bytes=0- cap=0
PROXY conn fail: ConnectionResetException: Connection reset
[cplayer] Failed to recognize file format.
```

`start=3574.582` = 3574 秒 = **59 分 34 秒**，而 A Beautiful Mind 我从没播过。

### 根因

`ui/compose/PlayerSessionBridge.kt` 的续播键：

```kotlin
internal fun watchKey(): String = playUrlOverride.ifBlank { currentMediaPath() }
```

SMB 播放既没有 `kivo.playurl`（不是 URL 片源）也没有 `kivo.media`（不是本地文件），
于是 `currentMediaPath()` 落到 `locateTestMedia()`：

```kotlin
// files/media 下按名字排序的第一个文件
return dir.listFiles()?.filter { it.isFile }?.minByOrNull { it.name }
```

**该目录里躺着 `kivo-local-test.mp4`（历史测试残留）**
⇒ 所有 SMB 影片的续播键都是它。

设备上 `files/watch.json` 的实证：

```json
"/storage/emulated/0/Android/data/com.kivo.cinema/files/media/kivo-local-test.mp4":
  { "pos": 3574582, "dur": 10267291 }
```

`pos=3574582` 就是日志里的 `start=3574.582`；
`dur=10267291`（2h51m）是 **3 Idiots 的时长**，不是那个 5 分钟测试片的。

⇒ 3 Idiots 播到 59:34 的进度，被记到了 `kivo-local-test.mp4` 键上，
然后 A Beautiful Mind 读同一个键，直接 seek 到 59:34。

### 影响面

**所有 SMB 片源共用一条续播记录**：
- 播 A 记到 B；下次播 A 从 B 的位置开始
- 任意两部 SMB 影片互相覆盖进度
- `.ts` 这次只是"恰好远 seek 到 33.8 GB 的 59 分钟处"才暴露成起播失败

本地文件（走 `kivo.media`）与 Emby（走 URL）**不受影响**，所以这个 bug 藏了很久。

### 修复

```kotlin
internal fun PlayerComposeActivity.watchKey(): String =
    playUrlOverride.ifBlank {
        val sourceId = intent.getStringExtra("kivo.sourceId")
        val smbPath = intent.getStringExtra("kivo.smbPath")
        if (!sourceId.isNullOrBlank() && !smbPath.isNullOrBlank()) {
            "smb://$sourceId/" + intent.getStringExtra("kivo.smbShare").orEmpty() + "/$smbPath"
        } else {
            currentMediaPath()
        }
    }
```

同时给 `WatchStore.report` 加空键保护（空键会把所有"拿不到路径"的播放写到 `""` 上，同一类错误）。

### 真机验证（K40）

| 检查 | 结果 |
|---|---|
| 播 3 Idiots（38.4 GB mkv） | `watch.json` 出现 `smb://55ccf8d3-.../12T/REMUX/3 Idiots...` → `pos=22458` |
| 同一次 loadfile 参数 | `options=""`（从 0 起播，不再继承错误进度） |
| 换播 A Beautiful Mind | `options=""` —— **没有**继承 3 Idiots 的 22458 ms |
| 旧键 `kivo-local-test.mp4`（pos=3574582） | 已从设备清除（备份见证据） |

`assembleDebug` = **BUILD SUCCESSFUL**；`WatchStore` 146 行、`PlayerSessionBridge` 288 行。

---

## 二、独立问题：33.8 GB `.ts` 仍读不出来（未解）

修完续播键后**仍然失败**，所以这是另一个问题：

```
PROXY REQ range=bytes=0- cap=0
PROXY conn fail: ConnectionResetException: Connection reset     ← 296 ms 后
[cplayer] Failed to recognize file format.                      ← 所有 demuxer 探测失败
```

### 已排除

| 候选 | 为什么排除 |
|---|---|
| **续播远 seek** | 修好后 `options=""`，仍失败 |
| **文件太大** | 同目录 3 Idiots（**38.4 GB**，比它还大）能正常播 |
| **VC-1 编码** | 失败在 demuxer 阶段，没进解码器 |
| **`.ts` 容器** | 未排除，但 `ConnectionResetException` 来自 netty（SMB 底层），像是**读不到数据**而不是"识别不了格式" |

### 剩余嫌疑

1. **该文件本身在 NAS 上不可读**（损坏 / 被占用 / 权限）；
2. `.ts` 无索引，mpv 需要连续读一大段才能确认格式，而 SMB 侧先断了；
3. 该路径的 SMB 读取有其它限制（该目录下还有另一个 `.ts`：Vertigo，未测）。

### 下一步需要（本批没做）

- 用 Kivo 播 `Vertigo`（同目录另一个 `.ts`）—— 若也失败 ⇒ `.ts` 容器问题；若成功 ⇒ 该文件问题；
- 或在设备上直接读该文件前 1 MB（Kivo 的 SMB 库）看是否成功。

> Windows 侧读不了：`net view \\192.168.5.36` 报**错误 5**（需凭据），所以没法从 PC 侧分诊。

---

## 三、证据清单

| 文件 | 内容 |
|---|---|
| `d12g-watch-backup.json` | 修复前 `watch.json` 原样备份（含错误键 `kivo-local-test.mp4` pos=3574582） |
| `d12g-watch-fixed.json` | 清除该键后的版本（用于回写设备） |
| `d12f-vc1-kernel.log` / `d12f-vc1-netdiag.log` | `.ts` 失败的内核与代理日志（同 D12-F） |
| `build-d12g.log` | 修复后编译日志 |

---

## 四、未覆盖

1. §二 的 `.ts` 问题**未解**，三个候选都没验完。
2. 修复只做了"键正确"，没做**历史数据迁移**：设备上已有的 `kivo-local-test.mp4` 那条记录
   （3 Idiots 的真实进度）只能丢弃或人工处理，不会自动归位到新键。
3. 「继续观看」列表只处理本地 `MediaItem`（`ContinueSource.continueEntries`），
   **SMB 影片本来就不进这个列表** —— 所以本次修复不改变列表行为，也没解决"SMB 没有继续观看入口"这个产品缺口。
