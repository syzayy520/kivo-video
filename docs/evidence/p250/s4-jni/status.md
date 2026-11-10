# P250 · S4 · JNI 编码 / JSON 转义 / IPv6 / 读失败归因

日期：2026-11-10
设备：K40 `b6e950ee`（arm64-v8a / Android 13 / MIUI）
基线：`f083d2e9`（S3）
状态：代码 + 自检 + 回归完成；两条真机判据因环境缺失**未验**（见 §五，如实记录）。

## 一、改了什么

| 文件 | 改动 |
| --- | --- |
| `jni/jni_smb2_json.hpp` | 新增。JSON 字符串转义：`"` `\` `\b\f\n\r\t`，其余 `< 0x20` → `\u00XX` |
| `jni/jni_string_convert.hpp` | `to_utf8` 改走 `GetStringChars` + UTF-16→UTF-8；`to_jstring` 改走 `NewString` + UTF-8→UTF-16 |
| `jni/jni_smb2_context.hpp` | 新增。`ctx_of` / `fh_of` / `user_or_null`（桥与 list 共用，避免两套真相） |
| `jni/jni_smb2_bridge.cxx` | 通道原语：create/destroy/setReadTimeout/connect/open/size/readInto/closeFh/getError/ntStatus |
| `jni/jni_smb2_list.cxx` | 新增。两个 JSON 生产者：`nativeList` / `nativeListShares` |
| `jni/CMakeLists.txt` | + `jni/jni_smb2_list.cxx`（唯一一处构建改动） |
| `smb/Smb2Native.kt` | `nativeConnect` 改收 `authority`（不再 host+port）；删死接口 `nativeReadAt`；`nativeReadInto` 文档改为"<0 保留原值" |
| `smb/SmbReader.kt` | 传 `address.authority`；`pread` 失败走 `SmbError.fromReadCode` |
| `smb/SmbSession.kt` | 传 `address.authority` |
| `smb/SmbShareLister.kt` | 由 `SmbAddress(host, port).authority` 生成，不再自己拼串 |

### 行数

| 文件 | 行数 | 预算 |
| --- | --- | --- |
| `jni_smb2_bridge.cxx` | **160** | ≤175（原 242） |
| `jni_smb2_list.cxx` | **95** | 新文件 ≤95 |
| `jni_smb2_context.hpp` | **35** | ≤40 |
| `jni_smb2_json.hpp` | **69** | ≤70 |
| `jni_string_convert.hpp` | **73** | ≤70（超 3 行，见 §六） |

拆分理由：`nativeList` / `nativeListShares` 是唯一把 libsmb2 结构体序列化成 JSON 的地方，
会随返回格式独立变化；桥里其余部分是"通道原语"。两者被改的原因完全不同。

## 二、JSON 转义自检：真机 14/14 PASS

```bash
NDK=/c/Android/sdk/ndk/27.2.12479018/toolchains/llvm/prebuilt/windows-x86_64/bin
TMP="C:/Users/Administrator/AppData/Local/Temp"
$NDK/aarch64-linux-android26-clang++.cmd -std=c++23 -O1 -Wall -static-libstdc++ \
  -I "C:/kivo video/android/app/src/main/cpp/jni" \
  -o "$TMP/kivo_json_check" "$TMP/kivo_json_check.cpp"
adb push "$TMP/kivo_json_check" /data/local/tmp/ && adb shell chmod 755 /data/local/tmp/kivo_json_check
adb shell /data/local/tmp/kivo_json_check
```

```
PASS plain ascii
PASS dquote
PASS backslash
PASS newline
PASS tab
PASS carriage return
PASS backspace
PASS formfeed
PASS control 0x01
PASS control 0x1f
PASS cjk passthrough
PASS emoji passthrough
PASS empty
PASS only newline
failures=0
```

改造前的对照：只转义 `"` 和 `\`，`\n` 与 `\x01` 直接进 JSON 字符串 →
`JSONArray()` 抛异常 → 用户看到"无法读取文件夹"。**一个字节废掉整个目录。**

UTF-8 编解码部分在 S1 已单独验过（12/12 PASS，见 `../s1-unit/status.md`），
S4 只是把它接进 `jni_string_convert`。

## 三、回归：seek 4/4 PASS

JNI 的 `nativeConnect` 签名变了（多所有调用点），读写路径也改了返回值语义，必须回归。

```
P243 OPEN 视频/kivo-test/local-seek.mkv size=2270964
P243 START codec=OMX.qcom.video.decoder.avc dur=60000000 audio=false
PASS 起播（准备 + 首帧会话就绪）
P243 POSITION 0 ms → 2924 ms
PASS 位置推进（3 秒窗口）
P243 SEEK target=30000 ms landing=25000 ms actual=27113 ms
P243 SUMMARY ... frames=67 dropped=0 first_frame=4.1ms completed=0
PASS seek 到中段并落位
P243 RESUME 27113 ms → 30105 ms
PASS seek 后继续推进
4/4 PASS
```

落点 `27113 ms` vs S2/S3 基线 `27106 ms`，差 7 ms —— 关键帧对齐的正常波动。

## 四、读失败归因（代码已改，真机未构造）

`nativeReadInto` 现在返回 rc 原值而不是 `-1`；`SmbReader.pread` 用
`SmbError.fromReadCode` 区分 NT status（高位 `0xC0`）与 `-errno`，
并新增 `IO_TIMEOUT (0xC0000121)` 单列文案。

**未构造真机场景**：要命中"先失败、再超时"这条组合，需要在同一 context 上先制造一次
确定失败（例如中途改密码），再断网。本卡没做 —— 现有证据只到"代码路径正确 + 单测覆盖
`fromReadCode` 的 NT status / errno 两支"（`SmbErrorTest` 6 例）。

## 五、两条真机判据未验（环境缺失，不掩饰）

| 判据 | 状态 | 缺什么 |
| --- | --- | --- |
| 含 emoji 的文件名可列举、可播放 | ❌ 未验 | 共享 `XiaoMi` 是匿名**只读**，无法创建 `测试😀.mkv` |
| IPv6 地址能连上 | ❌ 未验 | 家用路由未开 IPv6；且需要 IPv6 目标 |

已做到的替代覆盖：

- emoji：`utf8_codec` 真机自检里 `emoji becomes a surrogate pair` + `emoji roundtrip`
  证明编解码正确；`SmbAddress` 的 `authority` 单测证明地址形态正确。**但"整条链路
  走通"没有证据。**
- IPv6：`SmbAddressTest` 里 4 例（`bracketed ipv6 with port` / `ipv6 authority is always
  bracketed` / `link local zone id survives parsing`）证明**拼串形态**正确；
  "真能连上"没有证据。

**建议**：等共享可写、或路由开 IPv6 时补一条 G6 闸门的真机取证。在此之前
本卡不宣称这两条达成。

## 六、`jni_string_convert.hpp` 超预算 3 行（不拆）

73 / 70。超的是注释（说明为什么从 modified UTF-8 换成 UTF-16 中转）。
它只有三个函数、一个职责（字符串转换），再拆会变成"为了 3 行拆出一个文件"。
按"说清职责就不拆"的判据保留，此处记录偏差。

## 七、复算

```bash
cd android && JAVA_HOME=/c/Android/jdk-17.0.13+11 ./gradlew.bat assembleDebug
# seek 回归见 §三 的命令
```
