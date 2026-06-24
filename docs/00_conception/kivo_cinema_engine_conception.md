# KivoCinemaEngine 构思

KivoCinemaEngine 是 Windows 桌面端私人影院旗舰播放引擎。

它不是普通播放器后端，不是 FFmpeg demo，不是 mpv / libmpv 套壳，也不是 Emby / AList / WebDAV 文件浏览器。

## 产品目标

KivoCinemaEngine 的目标不是“能播放最多格式”，而是在当前 source、网络、设备、显示器、音频端点、字幕负载、资源预算和合规边界下，选择最高可达体验，并且能解释、能诊断、能复现、能证明。

## 差异化

Kivo Video 不做 Windows 版 Infuse 复制品。

Kivo Video 的差异化是：

- Windows 桌面私人影院旗舰播放器。
- 复杂源更稳。
- WebDAV / AList / 网盘 / Emby / 本地多源统一决策。
- Direct Play / Cache / Proxy / Transcode 选择可解释。
- Range / TTL / Cache 行为透明。
- HDR / Dolby Vision-aware fallback 可见。
- 播放失败可复现。
- Playback Inspector 面向用户和开发者双层诊断。

## V1.1 冻结范围

V1.1 冻结的是少数一级 core、五大 authoritative state、核心版本化合同、RemoteObjectIdentity、PlaybackPlanSchema、CapabilityJoinResult、DecisionLedger、ColorOutputMatrix、WindowsHdrOutputContract、VideoPresenterContract、SubtitleRenderPlan、AudioOutputDecision、PlaybackInspectorSnapshot、Graph / Playback 边界、Fallback / Recovery 矩阵和最小治理门闸。

V1.1 不冻结完整真实播放、不冻结完整视频增强、不冻结完整影院校准、不冻结完整兼容性农场。
