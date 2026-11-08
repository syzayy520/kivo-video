# 真实服务器播放验证

目标：把合成 Range 接线验证推进到设备已保存的真实 Emby；先定位准备超时，再验证生产宿主音视频及播放控制。不能用旧同步文件代替本轮结果。

家族树及边界：

- `android/app/src/androidTest/java/com/kivo/cinema/playback/RealEmbyRangeInstrumentation.kt`：在设备内读取非隐藏来源，登录并对每源前两条媒体做有界 Range 请求；只输出来源序号、HTTP 状态、字节数、耗时和异常类型。
- `android/app/src/main/java/com/kivo/cinema/net/HttpRangeFile.kt`：只有确定读取层缺陷后才改；仍负责网络字节，不接管解码。
- `android/app/src/main/java/com/kivo/cinema/playback/NetworkPlaybackSession.kt`：继续拥有原生会话及释放顺序，不改原生时钟/Surface 契约。
- 本目录：记录真实轮次、失败边界及后续回归；不归档地址、凭据、token 或片名。
- `android/app/src/androidTest/java/com/kivo/cinema/playback/RangeProbeRegression.kt`：回环服务器复现单字节探测误报总长、小文件截短范围与越界响应；从现有 instrumentation 调用。

验收：真实服务器轮次应有新生成的音频输出与音频主时钟证据，暂停冻结、恢复推进、前后 seek、退出释放分别通过。准备超时只记录失败，不算通过。

## 2026-11-09 已确认的读取层缺陷

K40 同一真实片源：`0-0` 返回 `bytes 0-0/1`，`0-4095` 返回
`bytes 0-4095/7901050401`，`4096-8191` 与开放范围返回相同完整总长。
旧 prepare 因此把视频当成 1 字节，头部只读到 1 字节，MediaExtractor 抛 IOException。
修复仍在 net/HttpRangeFile：探测改用 0–4095，允许短文件截短，拒绝超出请求上界的响应；
206、起点、总长、原始编码及后续读取总长一致性约束不放宽。不改原生解码/时钟。

第二个可复现缺陷：HTTP InputStream.read 不保证填满请求，旧 Reader 把短读直接
当文件回调结果返回。回环服务器先送 16 字节，60ms 后送余下字节，4096 字节
完整性断言在旧实现失败。Reader 改为填满当前请求/当前合法响应范围后返回；
中途 EOF 仍报错，循环检查取消，不填零也不吞错误。对应回归随同真实源测试执行。
