# 网络原生音视频接线 · 当前证据

日期：2026-11-09

平板 `CFE6R21B12000179` 与 Redmi K40 `b6e950ee` 都通过了同一条合成 Range 测试：

- [tablet-r1.txt](tablet-r1.txt)
- [k40-r1.txt](k40-r1.txt)
- [tablet-sync.json](tablet-sync.json)
- [k40-sync.json](k40-sync.json)

每轮均通过独立描述符随机读、生产 Compose 宿主启动、硬件视频轨、音轨接入、暂停冻结/恢复推进、前后跳转和退出释放；核心证据包含 `audio.opened=true`、`audio.started=true`、正的 `frames_rendered` 与 `clock_source=audio_timestamp`。

这证明的是网络适配器与现有原生会话在两台设备上的合成数据闭环。它还不等于真实 Emby/SMB 长时间播放、弱网取消、Surface 重建、人耳音画同步或完整流畅性验收；这些仍按后续卡片分别取证。暂停状态下 seek 当前提示先恢复播放，避免原生核心拒绝 paused seek。
