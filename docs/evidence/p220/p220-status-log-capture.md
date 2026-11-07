# 220 SMB 直连播放 状态日志捕获（2026-11-07 21:55）

来源：K40 (b6e950ee) `adb shell cat .../files/evidence/p220-status.log` 终端输出
（设备 USB 闪断，整文件拉取暂缓；以下为 shell 直读原文）。

```
1794059288072 startSmbPlayback begin
1794059289028 EXCEPTION [open] 111/.../仙草冻.mp4: (0xc000003a) OBJECT_PATH_NOT_FOUND
  ← 首轮：path 漏了 "视频" 层级（XiaoMi 共享根 = "视频" 共享的根目录）
1794059538122 startSmbPlayback begin
1794059538935 smb size=33056861            ← libsmb2 打开 33MB mp4 成功
1794059538938 proxy port=37019 url=http://127.0.0.1:37019/kivo
1794059538939 openUrlVideo rc=3 err=ffmpegOpenUrl: fd is negative
  ← 二轮：session open() 的 fd 前置校验挡住 URL 路径（已修：url 非空即合法）
1794059650458 startSmbPlayback begin
1794059651266 smb size=33056861
1794059651268 proxy port=42705 url=http://127.0.0.1:42705/kivo
1794059651270 openUrlVideo rc=0            ← FFmpeg 经本地 Range 代理打开成功
1794059651270 start rc=0                   ← 软解线程起跑
```

播放配置：小米路由 SMB（192.168.5.1/XiaoMi）→ 仙草冻.mp4 (33MB, h264)
链路：libsmb2 匿名 → LocalRangeProxy(127.0.0.1 随机端口) → FFmpeg http 协议 → avformat → 软解
