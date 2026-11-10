# 代理 fd 语义诊断 · K40（2026-11-09 16:33）

## 结论

**A 类根因排除。** `StorageManager.openProxyFileDescriptor` 生成的代理 fd 在 K40 上
fstat 长度、`SEEK_END`、`pread` 头/中/尾全部正常，尾部随机读能在 2 秒内返回真实字节。

因此 K40 容器解析失败**不是**"fd 不可 seek 导致平台 demux 退化为顺序扫描"。
剩余解释只有 B 类：**K40 的系统 MediaExtractor 不解析、或不使用 MKV 的
SeekHead → Cues 索引**，于是沿 cluster 线性扫描。修法只能是替换 demux，
无法通过改 fd 或网络层解决。

## 判据与读数（原始输出）

```
t=1453 source=2 item=3 login=ok
t=2365 http_ready reader=0 offset=0 end=4095 total=7369631541
t=2366 declared_size=7369631541
t=2372 fstat_size=7369631541 matches=true
t=2372 lseek_end=7369631541
t=2373 lseek_set_zero=0
t=2373 read_begin reader=1 offset=0 size=65536
t=4175 read_end reader=1 offset=0 bytes=65536 network_bytes=65536 elapsed_ms=1801
t=4176 pread_head bytes=16384 magic=1a45dfa3
t=4178 read_begin reader=1 offset=7369621504 size=12288
t=6225 http_ready reader=1 offset=7369621504 end=7369631540 total=7369631541
t=6227 read_end reader=1 offset=7369621504 bytes=10037 network_bytes=10037 elapsed_ms=2048
t=6228 pread_tail offset=7369623349 bytes=8192 nonzero=8186
t=6229 lseek_tail=7369623349 read_bytes=8192
t=6229 lseek_end_after_read=7369631541
t=6230 read_begin reader=1 offset=3684814848 size=8192
t=7355 read_end reader=1 offset=3684814848 bytes=8192 network_bytes=8192 elapsed_ms=1124
t=7356 pread_mid offset=3684815770 bytes=4096 nonzero=4080
INSTRUMENTATION_RESULT: result=pass
INSTRUMENTATION_CODE: 0
```

## 与平台 demux 行为的对照

同一片源、同一 fd 提供方式：

| 判据 | K40（Android 13 / API 33） | 平板 |
| --- | --- | --- |
| fd 尾部随机读 | 可用（本次实测 2048ms / 8192B） | 可用 |
| extractor 是否请求尾部偏移 | **从未请求** | 读完头部 344KiB 直接请求 `7369596928` |
| 识别轨道 | 0（45s 诊断取消） | 3（HEVC + 2×E-AC3，7.7s） |

fd 能力相同而行为相反，说明差异在**平台 MKV 解析策略**，不在 I/O 层。

## 复现

```bash
C:/Android/sdk/platform-tools/adb.exe -s b6e950ee shell am instrument -w \
  -e source 1 -e item 2 \
  com.kivo.cinema.test/com.kivo.cinema.playback.RangeFdSemanticsInstrumentation
```

诊断类：`android/app/src/androidTest/java/com/kivo/cinema/playback/RangeFdSemanticsInstrumentation.kt`
（只读，不打开 MediaExtractor、不解码、不改变生产路径）。

## 边界

- 本诊断只证明 fd 可用，**不证明** FFmpeg demux 能解决；后者需实现后实测。
- 平板当前不在线，表中平板侧数据取自 `tablet-extractor-trace-r1.txt`（2026-11-09）。
- 未验证：SMB/本地文件在同一设备上是否也走此路径（本地文件不经代理 fd，预期不受影响）。
