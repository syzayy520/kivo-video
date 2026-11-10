# 253 自然家族树：libmpv 播放内核

状态：**设计稿**（家族树已定，实施按 §四 的阶段推进）
上游：252（5.1 下混 / 杂音 / seek 精确落位）
前置证据：[p252/senplayer-arch](../../evidence/p252/senplayer-arch/status.md)、
[p252/audio-downmix](../../evidence/p252/audio-downmix/status.md)

---

## 零、这份文档解决什么

253 换内核（自研 FFmpeg demux + MediaCodec + AAudio 拼装 → libmpv）涉及
**文档、代码、构建脚本、证据**四类文件。铁律要求"先定自然家族树、模块边界、
目录归属，再落文件"，所以这份文档只回答三件事：

1. **哪些文件属于 253**（归属）
2. **每个文件只做一件事**（单职责）
3. **它们之间怎么依赖**（边界）

**不属于本卡的**：换内核的理由、SenPlayer 拆解、SMB 方案对比 —— 那些在
[253-mpv-kernel-plan.md](253-mpv-kernel-plan.md) 与
[253-mpv-kernel-smb-routes.md](253-mpv-kernel-smb-routes.md)。

---

## 一、文档家族（`docs/design/backend-core/`）

| 文件 | 只负责一件事 |
| --- | --- |
| `253-mpv-kernel-plan.md` | **总纲**：为什么换、目标架构、与旧内核的边界、风险 |
| `253-mpv-kernel-family-tree.md` | **本文件**：自然家族树、目录归属、模块边界 |
| `253-mpv-kernel-senplayer-teardown.md` | **SenPlayer 与 Rex 的拆解结论**（内核是什么、SMB 怎么走、seek 怎么发起） |
| `253-mpv-kernel-smb-routes.md` | **SMB 三条路线**（A libsmbclient / B 我们的 libsmb2 / C 本地代理）的成本与决策 |
| `253-mpv-kernel-phases.md` | **六个实施阶段**的交付物与验收判据（每阶段可独立验证） |
| `253-mpv-kernel-beyond-senplayer.md` | **"比他做得更好"的 9 条**，每条带判据与落地阶段 |

**证据家族**（`docs/evidence/p253/`）：

| 目录 | 只负责一件事 |
| --- | --- |
| `p253/kernel-build/` | 交叉编译链路的复算（ffmpeg/libass/libplacebo/libmpv 各自的构建事实与产物清单） |
| `p253/kernel-smoke/` | 内核冒烟测试（本地文件起播、硬解、声音、seek） |
| `p253/smb-via-mpv/` | mpv 内核下的 SMB 播放证据（三条路线各自的结果） |
| `p253/legacy-baseline/` | 旧内核的基线矩阵（回退与对照的客观数字） |

---

## 二、代码家族（`android/app/src/main/cpp/mpv/`）

**这是新目录**。它与旧的 `backend/src/video/video_plane/backends/` **并列而不混合** ——
旧内核整体保留为备用（tag `p252-selfmade-backup-27eff534`），两套内核不得互相引用。

```
android/app/src/main/cpp/mpv/
├── CMakeLists.txt                 # 只做一件事：登记本家族的源文件与依赖
├── jni_mpv_bridge.cpp             # 只做一件事：JNI 导出表（薄壳，转发到下列各层）
├── mpv_session.hpp                # 家族内唯一的会话接口（其余文件都依赖它）
├── mpv_session.cpp                # 只做一件事：libmpv 句柄的创建/销毁/生命周期
├── mpv_options.cpp                # 只做一件事：把播放参数映射成 mpv option
├── mpv_commands.cpp               # 只做一件事：seek/pause/音量等**命令**下发
├── mpv_event_loop.cpp             # 只做一件事：mpv 事件队列 → 回调给上层
├── mpv_video_output.cpp           # 只做一件事：把视频帧交给 ANativeWindow
├── mpv_audio_output.cpp           # 只做一件事：音频通路的配置与统计
├── mpv_source_resolver.cpp        # 只做一件事：把"我们的源描述"翻译成 mpv 能开的 URL
└── mpv_stats_bridge.cpp           # 只做一件事：把 mpv 状态翻译成我们的证据 JSON
```

**边界规则**（写进每个文件的注释头）：

| 规则 | 含义 |
| --- | --- |
| B1 | 本家族**不得**引用 `backend/src/video/video_plane/backends/` 的任何头文件 |
| B2 | 本家族**不得**直接调用 FFmpeg API（那是 mpv 的事） |
| B3 | 本家族**不得**碰 Android UI（上层用 Kotlin 订阅回调） |
| B4 | 每个 .cpp 只暴露 `mpv_session.hpp` 里声明过的接口；跨文件调用一律走该头文件 |
| B5 | 每个 .cpp 的行数控制在 **300 行以内**（与 `jni_player_bridge.cxx` 的 300 行线同一条规矩） |

---

## 三、构建家族（`android/app/buildscripts/mpv/`）

**不放在项目根目录**，也不混进 `android/app/src/main/cpp/`。

```
android/app/buildscripts/mpv/
├── build-libmpv.sh                # 只做一件事：交叉编译 libmpv（meson + NDK cross file）
├── cross-android-arm64.txt        # 只做一件事：meson 交叉编译配置
├── pkg-config-shim.py             # 只做一件事：Windows 上替代 msys 的 pkg-config
└── toolchain.env                  # 只做一件事：导出 NDK/meson/pkgconfig 环境变量
```

**为什么要有 shim**：本机 Git Bash 的路径转换会让 msys 的 pkg-config 找不到
Windows 路径下的 `.pc` 文件；自写 shim 是唯一能让 meson 稳定找到 ffmpeg/libass 的办法。

---

## 四、实施阶段与"家族树长出来"的顺序

| 阶段 | 新增文件 | 验收 |
| --- | --- | --- |
| **P1 内核打通** | `buildscripts/mpv/*`（4 个）、`cpp/mpv/*`（11 个） | 本地文件能起播、硬解、声音、seek |
| **P2 可播版本** | `cpp/mpv/mpv_source_resolver.cpp` 扩展（本地代理 URL） | SMB 片源能播（经代理） |
| **P3 正式版** | `cpp/mpv/mpv_source_resolver.cpp` 改为走我们的 libsmb2 | 与 P2 同判据 + 读画像可见 |
| **P4 断网恢复** | `cpp/mpv/mpv_event_loop.cpp` 扩展、`mpv_stats_bridge.cpp` 扩展 | 断网 12 s → ≤ 4 s 恢复 |
| **P5 品质超越** | 按 [253-mpv-kernel-beyond-senplayer.md](253-mpv-kernel-beyond-senplayer.md) 逐条 | 每条独立验收 |
| **P6 收尾** | 旧内核标记为备用、写切换开关与回退步骤 | 一条命令切回旧内核 |

**P1 的验收命令**（可复算）：

```bash
# 内核冒烟：本地文件
bash android/app/buildscripts/mpv/build-libmpv.sh
adb -s <dev> shell am instrument -w -e localPath /sdcard/kivo-test/mkv-audio.mkv \
  com.kivo.cinema.test/com.kivo.cinema.mpv.MpvSmokeInstrumentation
```

---

## 五、模块边界图

```
   Kotlin（播放页 UI / 资源库 / 字幕 / 弹幕）
        │  订阅回调 + 下发命令
        ▼
   ┌──────────────────────────────────────────┐
   │ android/app/src/main/cpp/mpv/            │  ← 新家族（B1~B5）
   │   jni_mpv_bridge → mpv_session           │
   │   mpv_commands / mpv_options             │
   │   mpv_video_output / mpv_audio_output    │
   │   mpv_event_loop / mpv_stats_bridge      │
   │   mpv_source_resolver                    │
   └───────────────┬──────────────────────────┘
                   │ libmpv.so（LGPL，动态链接）
                   ▼
   ┌──────────────────────────────────────────┐
   │ libmpv  →  libavformat/avcodec/...        │  ← FFmpeg 协议层负责 smb://
   └───────────────┬──────────────────────────┘
                   │
        ┌──────────┴───────────┐
        ▼                      ▼
  方案 C：本地 range 代理    方案 B：我们的 libsmb2
  （http://127.0.0.1:…）    （注册进 FFmpeg 协议层）
        └──────────┬───────────┘
                   ▼
              NAS / 路由器
```

**与旧内核的关系**：两条完全独立的链路，靠**播放页的一个开关**选择。
旧内核（`backend/.../backends/{ffmpeg,mediacodec,aaudio,avsync}`）一行不改地留着。

---

## 六、目录归属核对（防平铺）

| 东西 | 放哪 | 为什么不是别处 |
| --- | --- | --- |
| 交叉编译脚本 | `android/app/buildscripts/mpv/` | 与 Gradle 构建解耦，不进 `src/main/cpp`（那里只放 C++ 源） |
| JNI 与 mpv 适配 | `android/app/src/main/cpp/mpv/` | 独立家族，边界规则 B1~B5 写死 |
| 交叉编译产物（.so） | `android/app/src/main/cpp/mpv/prebuilt/arm64-v8a/` | 随仓库走还是 CI 产物？——**待定**，默认不入库，只记录构建脚本与版本 |
| 设计文档 | `docs/design/backend-core/253-*.md` | 与其它卡同目录同命名法 |
| 真机证据 | `docs/evidence/p253/<主题>/status.md` | 每个主题一个目录，与 p252 同结构 |
| 宿主机工具（cross file、shim） | `android/app/buildscripts/mpv/` | 不放项目根目录（根目录只留项目级入口） |

---

## 七、复算

```bash
# 家族完整性：253 的代码文件是否都满足单职责与行数上限
find android/app/src/main/cpp/mpv -name "*.cpp" -o -name "*.hpp" | while read f; do
  printf "%-60s %s lines\n" "$f" "$(wc -l < "$f")"
done
# 边界：B1（不引用旧内核）
grep -rn "video_plane/backends" android/app/src/main/cpp/mpv/ || echo "B1 OK"
# 家族树文档齐全
ls docs/design/backend-core/253-*.md
```
