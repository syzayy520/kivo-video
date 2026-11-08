# 真实源失败与反例（2026-11-09）

只记录脱敏诊断，来源序号为设备保存的非隐藏来源顺序，不含地址/凭据/片名。

## 首个已确认缺陷：单字节探测错误总长

K40，修复前：

```text
range=0-0 http=206 content_range=bytes 0-0/1 length=1
range=0-4095 http=206 content_range=bytes 0-4095/7901050401 length=4096
range=4096-8191 http=206 content_range=bytes 4096-8191/7901050401 length=4096
range=0- http=206 content_range=bytes 0-7901050400/7901050401 length=7901050401
PASS real Range size=1
PASS real header read=1 container=other
FAIL stage=extractor-probe type=IOException state=null
```

这些 PASS 是旧诊断打印标签，只证明函数返回，**1 字节不是正确的媒体长度**。

## 单字节修复后的真实轮次

- K40 来源1两轮、平板来源1一轮：`SocketTimeoutException`，尚未到媒体播放。
- K40 来源2：完整总长 `18981737873`，16字节头部 `ebml`，MediaExtractor 为0轨道；生产宿主明确失败，没有降级播放。
- K40 来源2短读修复中间版：诊断未及时返回，助手主动 `am force-stop` 结束测试，instrumentation 报 `Process crashed`。这是**测试被主动终止**，不能归为自然闪退，也不能计为通过。
- 后续有45秒诊断 I/O截止时间及逐项输出；最终轮次保存在同目录 `*-real-source*-*.txt`。

## 第二个已确认缺陷：把网络短读当文件短读返回

回环服务先送16字节、延迟60ms送余下数据，完整4096字节断言在旧Reader上失败：

```text
FAIL IllegalStateException: Check failed.
```

填满回调请求后，同一分包回归通过；另加入服务器每响应最多1024字节，验证跨响应继续读取，以及17字节小文件尾部读取与越界响应拒绝。

## 操作流程通过不等于流畅

平板中间版本完整合成操作测试通过，但新拉取的
`tablet-probe-fix-sync.json` 中：`video.frames_rendered=12`、
`video.frames_dropped_late=56`、seek评估 `total_us=487119`，超过150ms预算；
`audio.output.xrun_count=0`，但 `frames_silenced=18176`。
不能把该轮称为流畅或零丢帧。此文件是中间版反例，不能当最终版本的通过证据。

## 最终读取修复版的真实边界

平板来源2第1片识别5轨；第3片识别3轨，MIME为
`video/hevc,audio/eac3,audio/eac3`。两轮生产宿主均明确报
`audio open: audio decoder configure: no audio decoder for mime audio/eac3`。
这证明读取修复后至少在平板上能继续到音频解码环节，不证明所有设备都已支持容器。

第2片平板生产状态PLAYING，但测试在音轨检查失败；没有理由把失败写成闪退或音视频通过。
K40第2/3片完整解析仍超45秒诊断截止时间，取消后0轨；前64KiB正确不能证明整个远程文件读取都正确。

两机最终合成短流程的视频迟到丢帧已为0；K40仍1次xrun，补静音4800帧，
平板0次xrun、补静音7040帧。与中间反例不同轮，不能覆盖或删除原反例。
完整结果与文件映射见 [status.md](status.md)。
