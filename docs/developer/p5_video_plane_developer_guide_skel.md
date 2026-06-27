# P5 Video Media Plane Developer Guide (Skeleton)

**Document Type**: developer-guide-skeleton (GATE-033, P5A A14)
**Status**: SKELETON — scenarios TBD after P5A contracts are defined

---

## Scenario Index (11 total)

### 1. 本地 MP4 SDR 播放
- Pipeline flow: P4 → InputBridge → Demux → Decode(SW) → Render(SDR) → Present → P8
- Key types: (TBD after P5A contracts)
- Thread flow: (TBD)
- Stamp changes: (TBD)
- Capability states: SoftwareDecode=ProductionEnabled, HDR=Unsupported
- Error path: DemuxFailure → DemuxBackendCrashedEvidence
- Test names: (TBD)
- P5 owns: demux, video decode, frame lifecycle, present timing
- P5 does NOT own: source selection, audio, UI

### 2. HEVC HDR10 / P010 播放
- Pipeline flow: P4 → InputBridge → Demux → D3D11VA Decode → P010 SRV → HDR Render → Present
- Key types: (TBD)
- Thread flow: (TBD)
- Stamp changes: (TBD)
- Capability states: D3D11VA=ProductionEnabled, P010IntegerPlane=VerifiedInLab
- Error path: P010 SRV failure → UNORM fallback → HDR10StaticFallback
- Test names: (TBD)

### 3. 在线 live low-latency stream
- Pipeline flow: P4 → InputBridge → Demux(live) → Decode → LiveTimestampOrigin → Render(low-latency) → Present
- Key types: (TBD)
- Thread flow: (TBD)
- Stamp changes: (TBD)
- Capability states: LowLatencyLive=ProductionEnabled
- Error path: Queue starvation → CatchUp or FailToUpperLayer
- Test names: (TBD)

### 4. Protected content / output protection loss
- Pipeline flow: P4 → InputBridge → ProtectedBoundary → Decode(SecureSession) → OutputProtection → Present
- Key types: (TBD)
- Thread flow: (TBD)
- Stamp changes: (TBD)
- Capability states: ProtectedBoundary=BoundaryOnly (or PlatformBackend)
- Error path: OutputProtectionLost → FailClosed → ProtectedBoundaryOnlyFailClosedEvidence
- Test names: (TBD)

### 5. Dolby Vision Profile 5 fail-closed
- Pipeline flow: P4 → InputBridge → Demux → DV Detection → FailClosed (no engine)
- Key types: (TBD)
- Thread flow: (TBD)
- Stamp changes: (TBD)
- Capability states: DolbyVision=BoundaryOnly or FailClosed
- Error path: DV Profile 5 detected → FailClosed evidence
- Test names: (TBD)

### 6. Malformed file bounded failure
- Pipeline flow: P4 → InputBridge → Demux(malformed) → AllocationLimit → BoundedFailure
- Key types: (TBD)
- Thread flow: (TBD)
- Stamp changes: (TBD)
- Capability states: (TBD)
- Error path: Allocation bomb → AllocationBudgetExceeded → BoundedFailure evidence
- Test names: (TBD)

### 7. Device lost during HDR playback
- Pipeline flow: Decoding HDR → DeviceRemoved → HoldLastSafeFrame → Rebuild or FailToUpperLayer
- Key types: (TBD)
- Thread flow: (TBD)
- Stamp changes: (TBD)
- Capability states: DeviceLost → RecoverySeek or Standby
- Error path: DeviceRemoved → FailureArbiter → TerminalOutcome
- Test names: (TBD)

### 8. Monitor move SDR → HDR
- Pipeline flow: SDR present → MonitorMove → DisplayCapabilityRefresh → HDR enable (if capable)
- Key types: (TBD)
- Thread flow: (TBD)
- Stamp changes: (TBD)
- Capability states: HDR10=Enabled after refresh
- Error path: DisplayCapabilitySnapshot expired → stop claiming HDR
- Test names: (TBD)

### 9. D3D11VA P010 fallback
- Pipeline flow: D3D11VA Decode → P010 IntegerPlane → SRV fail → UNORM fallback → Tolerance check
- Key types: (TBD)
- Thread flow: (TBD)
- Stamp changes: (TBD)
- Capability states: P010UnormFallback=Active, P010IntegerPlane=Degraded
- Error path: UNORM tolerance exceeded → HDR degraded evidence
- Test names: (TBD)

### 10. Live low latency catch-up
- Pipeline flow: Queue buildup → CatchUpPolicy → Skip frames or reduce quality → Sync
- Key types: (TBD)
- Thread flow: (TBD)
- Stamp changes: (TBD)
- Capability states: (TBD)
- Error path: CatchUp timeout → SkipToLiveEdge or Fail
- Test names: (TBD)

### 11. Subtitle attachment safety
- Pipeline flow: Demux → Attachment detection → MIME sniff → Quota check → OpaqueAttachmentFlow → P7
- Key types: (TBD)
- Thread flow: (TBD)
- Stamp changes: (TBD)
- Capability states: (TBD)
- Error path: MIME mismatch → reject → AttachmentMimeMismatchEvidence
- Test names: (TBD)

---

**END OF DEVELOPER GUIDE SKELETON**
**Status**: SKELETON — to be expanded after P5A contracts are defined and P5B~P5H implementation begins
