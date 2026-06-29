# KIVO-VIDEO-P8 SOURCE RECONCILIATION

Task ID: `KIVO-VIDEO-P8-SUBTITLE-PLANE-DESIGN-LOCK-001-V1.5.5`
Source SHA-256: `9AC07D575468533312CB403CED5CD368372F2DC248CC5232F3B492B8F72B4197`
Non-empty paragraphs checked: `276`

| Paragraph | Text SHA-256 | Mapping | Status | Excerpt |
|---:|---|---|---|---|
| 1 | `bae48b521bc98891` | context paragraph checked | CHECKED | Kivo Video P8 Subtitle Plane 终极设计书 V1.5.5 |
| 2 | `c69c6c7b0ed48fde` | context paragraph checked | CHECKED | Task ID: KIVO-VIDEO-P8-SUBTITLE-PLANE-DESIGN-LOCK-001-V1.5.5 |
| 3 | `baca1d868348f155` | P8C bootstrap/function table, P8D/P8F privacy, P8E ASS semantics, P8F observer | CHECKED | Document Type: architecture-design / implementation-baseline / c-header-closure / c-abi-purist-closure / no-noexcept-in-c-header / bootstrap-status-lock / api-p |
| 4 | `32459c4b7e3f0dd2` | context paragraph checked | CHECKED | Status: FINAL_DESIGN_BASELINE_CANDIDATE_FOR_P8A_IMPLEMENTATION_PLANNING |
| 5 | `01e589b21cc152b7` | section heading/context | CHECKED | 0. 最终执行口令 |
| 6 | `1dab2cfa2814e9c7` | context paragraph checked | CHECKED | P8 = Subtitle Plane Core。 |
| 7 | `400c8bedf0157f05` | P8B public C ABI, normative rule checked | CHECKED | P8 public ABI 必须能作为 纯 C header 被 C 编译器包含。 P8 不允许把 C++ ABI、Qt、FFmpeg、Win32 renderer、STL 容器、C++ exception/noexcept 直接暴露到 C ABI。 |
| 8 | `ba67efefec37cc85` | context paragraph checked | CHECKED | V1.5.5 的目标不是扩功能，而是完成最后一层： |
| 9 | `1f90ccec48983ce3` | P8B public C ABI, P8C bootstrap/function table, P8D source reader, P8D/P8F privacy, P8E ASS semantics, P8F snapshot blob, P8F observer, P8G ABI layout guard | CHECKED | text id="p8v155-final-goal" True C ABI header closure all referenced public C types closure bootstrap/version negotiation closure source reader vtable version c |
| 10 | `06ac569debe03beb` | context paragraph checked | CHECKED | V1.5.5 继承 V1.5.4 的所有非冲突内容。 凡本版与 V1.5.4 冲突，以 V1.5.5 为准。 |
| 11 | `042f8cea2ec9ddf2` | section heading/context | CHECKED | 1. V1.5.5 修正总表 |
| 12 | `7e374955b7d898cc` | context paragraph checked | CHECKED | 本版修复以下遗漏： |
| 13 | `da35d9fd75e071cf` | C-series public C ABI closure, G-series contract gap closure, A-series ambiguity closure, I-series implementation guard closure, P8B public C ABI, P8D source reader, P8D source vtable, P8D/P8F resource token, P8D/P8F privacy, P8D discovery, P8E SSA semantics, P8E ASS semantics, P8E WebVTT semantics, P8E frame lease, P8F snapshot blob, P8F observer, P8G ABI layout guard | CHECKED | text id="p8v155-fixes" C-001: 定义 KIVO_P8_API_V1_REQUIRED_PREFIX_SIZE。 C-002: C ABI header 中不再直接写 noexcept。 C-003: source reader / observer / executor callback 中 |
| 14 | `54891df0dfaa585d` | section heading/context | CHECKED | 2. Public C Header 基线 |
| 15 | `7ab4a1717d8a8742` | section heading/context | CHECKED | 2.1 Header include |
| 16 | `93ccc57da8f2e6e7` | P8B public C ABI | CHECKED | P8 C ABI header 只允许： |
| 17 | `7728b4aa07f60749` | context paragraph checked | CHECKED | c id="p8v155-include" #include <stdint.h> #include <stddef.h> |
| 18 | `2c2c9fd2dc2f6d5e` | context paragraph checked | CHECKED | 不使用 <stdbool.h> 作为 ABI 返回值基础。 跨语言 ABI 统一使用 uint32_t status / bool-like value。 |
| 19 | `afd3699a9eb954b4` | section heading/context | CHECKED | 2.2 Noexcept 宏 |
| 20 | `90e34f7d673d67ad` | P8B public C ABI, normative rule checked | CHECKED | C ABI header 不得直接写 C++ noexcept。 允许统一宏： |
| 21 | `4b5547c429b773da` | context paragraph checked | CHECKED | c id="p8v155-noexcept" #ifdef __cplusplus #define KIVO_P8_NOEXCEPT noexcept #else #define KIVO_P8_NOEXCEPT #endif |
| 22 | `ec91fb2dfd35e2dc` | normative rule checked | CHECKED | 所有 public function / callback signature 使用 KIVO_P8_NOEXCEPT，不得直接写裸 noexcept。 |
| 23 | `6b7fa52f96c06d51` | section heading/context | CHECKED | 2.3 Calling convention / export |
| 24 | `cfd96b069c01330a` | context paragraph checked | CHECKED | ```c id=“p8v155-call-export” #if defined(_WIN32) #define KIVO_P8_CALL __cdecl #if defined(KIVO_P8_BUILD_DLL) #define KIVO_P8_EXPORT __declspec(dllexport) #else  |
| 25 | `357fe01497ae094a` | section heading/context | CHECKED | #ifdef __cplusplus extern “C” { #endif |
| 26 | `75d5301099ddb62b` | context paragraph checked | CHECKED | typedef struct KivoSubtitlePlaneHandle KivoSubtitlePlaneHandle; |
| 27 | `a0147ced04b8b321` | section heading/context | CHECKED | #ifdef __cplusplus } #endif |
| 28 | `616e6e47f4aeec9a` | P8B public C ABI, P8E ASS semantics, section heading/context, normative rule checked | CHECKED | ## 2.4 Forbidden C ABI tokens  ```text id="p8v155-forbidden-c-header" std:: std::uint32_t std::uint64_t std::size_t bool as ABI return type enum class noexcept  |
| 29 | `35faf7c2f3a88e64` | section heading/context | CHECKED | 3. ABI Version / Status / Struct Header |
| 30 | `fec686847632f666` | section heading/context | CHECKED | 3.1 Version constants |
| 31 | `962e83671d12c94f` | context paragraph checked | CHECKED | c id="p8v155-version" #define KIVO_P8_ABI_MAJOR 1u #define KIVO_P8_ABI_MINOR 5u #define KIVO_P8_ABI_PATCH 5u #define KIVO_P8_ABI_REVISION 155u |
| 32 | `c13d9eded2388a20` | section heading/context | CHECKED | 3.2 API status |
| 33 | `a57fed5d4904469f` | P8C bootstrap/function table | CHECKED | Bootstrap 不再返回 bool，返回明确 status。 |
| 34 | `077694f58a275273` | context paragraph checked | CHECKED | c id="p8v155-api-status" typedef enum KivoP8ApiStatus {     KivoP8ApiStatus_Ok = 0,     KivoP8ApiStatus_NullPointer = 1,     KivoP8ApiStatus_ApiTableTooSmall =  |
| 35 | `14dec291104e67fe` | section heading/context | CHECKED | 3.3 Callback status |
| 36 | `e8d68c0338cfe5ce` | context paragraph checked | CHECKED | 所有外部 callback/vtable 返回统一 status。 |
| 37 | `78c9af383ae1d8a3` | context paragraph checked | CHECKED | c id="p8v155-callback-status" typedef enum KivoP8CallbackStatus {     KivoP8CallbackStatus_Ok = 0,     KivoP8CallbackStatus_Failed = 1,     KivoP8CallbackStatus |
| 38 | `d839512819f5300f` | section heading/context | CHECKED | 3.4 ABI struct header |
| 39 | `1eb62363aa6bea15` | context paragraph checked | CHECKED | c id="p8v155-abi-header" typedef struct KivoP8AbiHeader {     uint32_t struct_size;     uint16_t struct_version;     uint16_t flags; } KivoP8AbiHeader; |
| 40 | `3808c4af0e57b9b8` | context paragraph checked | CHECKED | Rules： |
| 41 | `6f16920d49618e46` | P8B public C ABI, normative rule checked | CHECKED | text id="p8v155-abi-header-rules" 1. 所有跨 DLL 顶层 request/config/result/descriptor/vtable/function-table struct 必须以 KivoP8AbiHeader 开头。 2. 调用方必须填写 struct_size = s |
| 42 | `dd6f1d363ccd557c` | P8C bootstrap/function table, section heading/context | CHECKED | 4. API Bootstrap |
| 43 | `aeb626081d5c43fc` | section heading/context | CHECKED | 4.1 Required prefix size |
| 44 | `ae0298605a9bea3e` | context paragraph checked | CHECKED | KivoSubtitlePlaneApiV1 的 required prefix 定义为： |
| 45 | `b0767b2544767654` | context paragraph checked | CHECKED | text id="p8v155-prefix-semantic" required prefix = KivoP8AbiHeader + abi_major + abi_minor + abi_patch + abi_revision |
| 46 | `ee6dee6476fd3838` | context paragraph checked | CHECKED | 宏定义： |
| 47 | `15bbed33ad1b8db9` | context paragraph checked | CHECKED | c id="p8v155-prefix-macro" #define KIVO_P8_API_V1_REQUIRED_PREFIX_SIZE \     ((uint32_t)(sizeof(KivoP8AbiHeader) + (4u * sizeof(uint32_t)))) |
| 48 | `b1ba844dfe2a1709` | context paragraph checked | CHECKED | 实现可额外使用 offsetof(KivoSubtitlePlaneApiV1, create) 做静态一致性检查。 |
| 49 | `fe6c67a9edbce92a` | P8C bootstrap/function table, section heading/context | CHECKED | 4.2 Bootstrap function |
| 50 | `4ce2b468d2c7f74d` | P8C bootstrap/function table | CHECKED | ```c id=“p8v155-bootstrap” typedef struct KivoSubtitlePlaneApiV1 KivoSubtitlePlaneApiV1; |
| 51 | `6317996615911132` | context paragraph checked | CHECKED | KIVO_P8_EXPORT KivoP8ApiStatus KIVO_P8_CALL kivo_subtitle_plane_get_api_v1( uint32_t requested_abi_revision, KivoSubtitlePlaneApiV1* out_api, uint32_t out_api_s |
| 52 | `9ac5bfdb98291217` | P8C bootstrap/function table | CHECKED | Rules：  ```text id="p8v155-bootstrap-rules" 1. out_api == NULL -> KivoP8ApiStatus_NullPointer。 2. out_api_size < KIVO_P8_API_V1_REQUIRED_PREFIX_SIZE -> ApiTable |
| 53 | `9eb893569c1c00ef` | section heading/context | CHECKED | 5. Core Scalar Types |
| 54 | `937ebc08f73c860f` | section heading/context | CHECKED | 5.1 Time / Revision / Epoch |
| 55 | `f780f8f8a5dadd6f` | context paragraph checked | CHECKED | ```c id=“p8v155-core-time” typedef struct KivoSubtitleTime { int64_t microseconds; } KivoSubtitleTime; |
| 56 | `50889fe57bafb7bb` | context paragraph checked | CHECKED | typedef struct KivoSubtitleDelayOffset { int64_t microseconds; } KivoSubtitleDelayOffset; |
| 57 | `ea3f660d90a4adb6` | context paragraph checked | CHECKED | typedef struct KivoSubtitleSeekEpoch { uint64_t value; } KivoSubtitleSeekEpoch; |
| 58 | `77a282e3d25a14a6` | context paragraph checked | CHECKED | typedef struct KivoP8PlaybackRevision { uint64_t value; } KivoP8PlaybackRevision; |
| 59 | `cb01c00fdbc946a0` | context paragraph checked | CHECKED | typedef struct KivoP8GraphSnapshotRevision { uint64_t value; } KivoP8GraphSnapshotRevision; |
| 60 | `0d63679d1cc55b4e` | context paragraph checked | CHECKED | Rules：  ```text id="p8v155-time-rules" KivoSubtitleTime.microseconds == INT64_MIN means invalid public time. KivoSubtitleDelayOffset range = [-24h, +24h]. KivoP |
| 61 | `2ae11f4294a34df5` | section heading/context | CHECKED | 5.2 ID types |
| 62 | `a59fb2b9b66263af` | P8D discovery, P8F snapshot lease, P8F evidence | CHECKED | c id="p8v155-ids" typedef struct KivoSubtitleCommandId { uint64_t value; } KivoSubtitleCommandId; typedef struct KivoSubtitleCorrelationId { uint64_t value; } K |
| 63 | `3808c4af0e57b9b8` | context paragraph checked | CHECKED | Rules： |
| 64 | `4eb503698adcc5cb` | context paragraph checked | CHECKED | text id="p8v155-id-rules" 1. value == 0 means invalid. 2. 0 is never a valid allocated ID. 3. Caller must not invent internal IDs except by receiving them from  |
| 65 | `9d21c3a922635591` | section heading/context | CHECKED | 6. Error / Result / Command |
| 66 | `9d5ac83200fcf55d` | section heading/context | CHECKED | 6.1 Result status |
| 67 | `b5a713e1bf970900` | context paragraph checked | CHECKED | c id="p8v155-result-status" typedef enum KivoSubtitleResultStatus {     KivoSubtitleResultStatus_Ok = 0,     KivoSubtitleResultStatus_Error = 1,     KivoSubtitl |
| 68 | `1581131d7d439869` | section heading/context | CHECKED | 6.2 Plane error |
| 69 | `10be7b1b6a9a34c0` | context paragraph checked | CHECKED | ```c id=“p8v155-plane-error” typedef enum KivoSubtitlePlaneError { KivoSubtitlePlaneError_None = 0, |
| 70 | `2876c22e5b6b8da3` | P8D discovery, P8E frame lease, P8F snapshot lease | CHECKED | KivoSubtitlePlaneError_NullPointer, KivoSubtitlePlaneError_InvalidArgument, KivoSubtitlePlaneError_MalformedInput, KivoSubtitlePlaneError_IllegalEnumValue, Kivo |
| 71 | `6134a5c3e377215a` | context paragraph checked | CHECKED | } KivoSubtitlePlaneError; |
| 72 | `5cf3179e1ef54ae3` | section heading/context | CHECKED | ## 6.3 Command token  ```c id="p8v155-command-token" typedef enum KivoSubtitleCommandImmediateStatus {     KivoSubtitleCommandImmediateStatus_Accepted = 0,      |
| 73 | `3808c4af0e57b9b8` | context paragraph checked | CHECKED | Rules： |
| 74 | `d930cf24a6618fd3` | context paragraph checked | CHECKED | text id="p8v155-command-token-rules" 1. command_id is extracted from KivoSubtitleCommandToken.command_id. 2. command_id.value == 0 means rejected before command |
| 75 | `a6eb07a6068d7ddc` | section heading/context | CHECKED | 6.4 Command lifecycle |
| 76 | `ba3e908d743cc534` | context paragraph checked | CHECKED | ```c id=“p8v155-command-lifecycle” typedef enum KivoSubtitleCommandLifecycleState { KivoSubtitleCommandLifecycleState_Unknown = 0, KivoSubtitleCommandLifecycleS |
| 77 | `de2a1e2b0ba89d40` | context paragraph checked | CHECKED | typedef struct KivoSubtitleCommandLifecycleSnapshot { KivoSubtitleCommandId command_id; KivoSubtitleCorrelationId correlation_id; KivoSubtitleCommandLifecycleSt |
| 78 | `c06e4e276aad7d37` | context paragraph checked | CHECKED | ---  # 7. Byte / String Input Types  ## 7.1 Input string  ```c id="p8v155-input-string" typedef struct KivoSubtitleInputStringView {     const uint8_t* utf8;    |
| 79 | `3808c4af0e57b9b8` | context paragraph checked | CHECKED | Rules： |
| 80 | `e7d7d1745d61defd` | context paragraph checked | CHECKED | text id="p8v155-input-string-rules" 1. Valid only during current call. 2. P8 copies if retained. 3. Not NUL-terminated unless caller includes NUL in utf8_bytes. |
| 81 | `2237a83c86581e5b` | section heading/context | CHECKED | 7.2 Input bytes |
| 82 | `8013b1bdd3c3f985` | context paragraph checked | CHECKED | 统一名称只允许： |
| 83 | `74dfeb91b753ef54` | context paragraph checked | CHECKED | c id="p8v155-input-bytes" typedef struct KivoSubtitleInputBytesView {     const uint8_t* data;     uint64_t bytes; } KivoSubtitleInputBytesView; |
| 84 | `92f98a3ff600120c` | context paragraph checked | CHECKED | 禁止再出现： |
| 85 | `1604fe46c0e877ac` | context paragraph checked | CHECKED | text id="p8v155-forbidden-input-name" KivoInputBytesView |
| 86 | `d7ffa00102eca9cb` | section heading/context | CHECKED | 7.3 Mutable byte span |
| 87 | `41162ed766a5127d` | context paragraph checked | CHECKED | c id="p8v155-mutable-span" typedef struct KivoMutableByteSpan {     uint8_t* data;     uint64_t bytes; } KivoMutableByteSpan; |
| 88 | `3808c4af0e57b9b8` | context paragraph checked | CHECKED | Rules： |
| 89 | `b99b71c7430efa9f` | context paragraph checked | CHECKED | text id="p8v155-mutable-span-rules" 1. data == NULL && bytes == 0 allowed only for explicit size-query capable APIs. 2. data == NULL && bytes > 0 -> MalformedIn |
| 90 | `286e31a5f7578b0a` | section heading/context | CHECKED | 8. Generation Types |
| 91 | `229930ed3ea86c91` | section heading/context | CHECKED | 8.1 Content generation |
| 92 | `767cadccf22fbe89` | context paragraph checked | CHECKED | c id="p8v155-content-generation" typedef struct KivoSubtitleContentGeneration {     uint64_t source_generation;     uint64_t track_generation;     uint64_t pars |
| 93 | `67e52021c2036858` | section heading/context | CHECKED | 8.2 Packet generation |
| 94 | `6e3557f4fd032fdc` | context paragraph checked | CHECKED | c id="p8v155-packet-generation" typedef struct KivoSubtitlePacketGeneration {     uint64_t source_generation;     uint64_t track_generation;     KivoSubtitleSee |
| 95 | `e9b97df84e6bc006` | section heading/context | CHECKED | 8.3 Timeline generation |
| 96 | `8699d6189695b6f5` | context paragraph checked | CHECKED | c id="p8v155-timeline-generation" typedef struct KivoSubtitleTimelineGeneration {     KivoSubtitleSeekEpoch seek_epoch;     uint64_t delay_epoch;     KivoP8Play |
| 97 | `701dca2fb538c65b` | section heading/context | CHECKED | 8.4 Frame generation |
| 98 | `e9213583c4b120c5` | context paragraph checked | CHECKED | c id="p8v155-frame-generation" typedef struct KivoSubtitleFrameGeneration {     KivoSubtitleContentGeneration content;     KivoSubtitleTimelineGeneration timeli |
| 99 | `3808c4af0e57b9b8` | context paragraph checked | CHECKED | Rules： |
| 100 | `cc015eaeb35e5371` | context paragraph checked | CHECKED | text id="p8v155-generation-rules" 1. delay_epoch never invalidates parsed cues. 2. seek_epoch never invalidates static external cue index. 3. embedded packets a |
| 101 | `a8e95104d2b77657` | P8D/P8F privacy, section heading/context | CHECKED | 9. Source / Redacted Identity / Hash |
| 102 | `9dba369ea788d613` | section heading/context | CHECKED | 9.1 Hash algorithm |
| 103 | `9af2ab928321bfa7` | context paragraph checked | CHECKED | c id="p8v155-hash-algo" typedef enum KivoSubtitleHashAlgorithm {     KivoSubtitleHashAlgorithm_None = 0,     KivoSubtitleHashAlgorithm_SipHash24_64 = 1,     Kiv |
| 104 | `3808c4af0e57b9b8` | context paragraph checked | CHECKED | Rules： |
| 105 | `191049407478e3e1` | P8D/P8F privacy | CHECKED | text id="p8v155-hash-rules" 1. Diagnostic identity hash must use keyed algorithm. 2. Unkeyed hash allowed only deterministic fake/test or local non-exported fon |
| 106 | `a1c951fe82ff0073` | section heading/context | CHECKED | 9.2 Hash key provider |
| 107 | `475330a0223b565f` | context paragraph checked | CHECKED | ```c id=“p8v155-hash-key” typedef struct KivoSubtitleHashKeyProvider { KivoP8AbiHeader header; void* user_data; |
| 108 | `87879b7733599a64` | context paragraph checked | CHECKED | KivoP8CallbackStatus (KIVO_P8_CALL *get_key)(     void* user_data,     uint64_t key_id,     uint8_t* out_key,     uint64_t out_key_capacity,     uint64_t* out_k |
| 109 | `26fecd7d9df1b2eb` | context paragraph checked | CHECKED | } KivoSubtitleHashKeyProvider; |
| 110 | `e0e86958e2bb6abf` | P8D/P8F privacy, section heading/context | CHECKED | ## 9.3 Privacy policy  ```c id="p8v155-privacy" typedef enum KivoSubtitlePrivacyMode {     KivoSubtitlePrivacyMode_Strict = 0,     KivoSubtitlePrivacyMode_Diagn |
| 111 | `5c722444acdf3f7a` | context paragraph checked | CHECKED | Flags： |
| 112 | `d54fd5f49ddee5a6` | P8D/P8F privacy, P8F evidence | CHECKED | text id="p8v155-privacy-flags" bit 0 = redact_raw_paths bit 1 = redact_raw_subtitle_text bit 2 = redact_font_names_in_evidence bit 3 = redact_url_query bit 4 =  |
| 113 | `65b804313f12d9b8` | P8D/P8F privacy, section heading/context | CHECKED | 9.4 Redacted source identity |
| 114 | `21eee5e4919a18db` | P8D/P8F privacy | CHECKED | ```c id=“p8v155-redacted” typedef enum KivoRedactedSourceKind { KivoRedactedSourceKind_Unknown = 0, KivoRedactedSourceKind_LocalFile = 1, KivoRedactedSourceKind |
| 115 | `bc1f250cb4b1b61e` | P8D/P8F privacy | CHECKED | typedef struct KivoRedactedSourceIdentity { KivoRedactedSourceKind kind; uint64_t source_identity_hash; uint64_t basename_hash; uint64_t content_fingerprint_has |
| 116 | `975222d95e884b4b` | P8D/P8F privacy | CHECKED | Flags：  ```text id="p8v155-redacted-flags" bit 0 = has_content_fingerprint bit 1 = hash_key_id_present |
| 117 | `60a5e4ccd8ad955a` | P8D source reader, section heading/context | CHECKED | 10. Source Reader Contract |
| 118 | `fb3f2fe245c6794a` | section heading/context | CHECKED | 10.1 Source handle / seek origin |
| 119 | `c378423901d9d48f` | context paragraph checked | CHECKED | ```c id=“p8v155-source-handle” typedef struct KivoSubtitleSourceHandle { uint64_t value; } KivoSubtitleSourceHandle; |
| 120 | `a478d2dbc31ce9c1` | context paragraph checked | CHECKED | typedef enum KivoSubtitleSeekOrigin { KivoSubtitleSeekOrigin_Begin = 0, KivoSubtitleSeekOrigin_Current = 1, KivoSubtitleSeekOrigin_End = 2 } KivoSubtitleSeekOri |
| 121 | `83411f9e6b74f523` | section heading/context | CHECKED | ## 10.2 Source kind  ```c id="p8v155-source-kind" typedef enum KivoSubtitleSourceKind {     KivoSubtitleSourceKind_Unknown = 0,     KivoSubtitleSourceKind_FileP |
| 122 | `cfed9f2f8783e75e` | section heading/context | CHECKED | 10.3 Source vtable |
| 123 | `6e43f0768c56ccde` | P8D source vtable | CHECKED | KivoSubtitleExternalSourceVTable is V1 frozen and versioned through KivoP8AbiHeader. |
| 124 | `314fc718709c24c5` | P8D source vtable | CHECKED | ```c id=“p8v155-source-vtable” typedef struct KivoSubtitleExternalSourceVTable { KivoP8AbiHeader header; void* user_data; |
| 125 | `6f889cab4cf01883` | context paragraph checked | CHECKED | KivoP8CallbackStatus (KIVO_P8_CALL *open)(     void* user_data,     const struct KivoSubtitleSourceDescriptor* source,     KivoSubtitleSourceHandle* out_handle  |
| 126 | `480f1ea03cf0fffb` | P8D source vtable | CHECKED | } KivoSubtitleExternalSourceVTable; |
| 127 | `4817ca9a188e0df7` | context paragraph checked | CHECKED | Rules：  ```text id="p8v155-source-vtable-rules" 1. V1 required callbacks for Reader/ProviderByteStream: open/read/close. 2. seek/tell may be NULL only if source |
| 128 | `56ce11014f41ffc8` | section heading/context | CHECKED | 10.4 Source descriptor |
| 129 | `e526ea23f1e6fa5e` | context paragraph checked | CHECKED | ```c id=“p8v155-source-desc” typedef struct KivoSubtitleSourceDescriptor { KivoP8AbiHeader header; KivoSubtitleSourceKind kind; KivoSubtitleSourceId source_id; |
| 130 | `c71dd46f51a2760c` | P8D source vtable, P8D/P8F privacy | CHECKED | KivoSubtitleInputStringView file_path_utf8; KivoSubtitleInputBytesView memory_bytes;  KivoSubtitleExternalSourceVTable reader;  KivoRedactedSourceIdentity redac |
| 131 | `6432bff364ea5a5c` | context paragraph checked | CHECKED | } KivoSubtitleSourceDescriptor; |
| 132 | `d4d7cd475b5afe3c` | context paragraph checked | CHECKED | Flags：  ```text id="p8v155-source-flags" bit 0 = seekable bit 1 = reader_callbacks_thread_safe bit 2 = file_access_allowed bit 3 = memory_must_copy_immediately |
| 133 | `3808c4af0e57b9b8` | context paragraph checked | CHECKED | Rules： |
| 134 | `f5130f616921246f` | P8F evidence | CHECKED | ```text id=“p8v155-source-rules” FilePathUtf8: file_path_utf8 valid only during call. raw path never enters evidence. P8 may open only when file access feature  |
| 135 | `4ce57ebe597e137d` | context paragraph checked | CHECKED | MemoryBytes: memory_bytes valid only during call. P8 copies before async parse. |
| 136 | `00cf6be4084e7c6c` | context paragraph checked | CHECKED | Reader/ProviderByteStream: P8 owns handle after open success. P8 closes handle exactly once. cancel may race with read; source vtable must tolerate cancel durin |
| 137 | `ff6cec420cb85c16` | P8E SSA semantics, P8E ASS semantics, P8E WebVTT semantics | CHECKED | ---  # 11. Format / Track / Embedded Types  ## 11.1 Format  ```c id="p8v155-format" typedef enum KivoSubtitleFormat {     KivoSubtitleFormat_Unknown = 0,     Ki |
| 138 | `65b5269ff673ba5e` | section heading/context | CHECKED | 11.2 Track kind |
| 139 | `2ce7f68c18a0a836` | context paragraph checked | CHECKED | c id="p8v155-track-kind" typedef enum KivoSubtitleTrackKind {     KivoSubtitleTrackKind_ExternalFile = 0,     KivoSubtitleTrackKind_ExternalMemory = 1,     Kivo |
| 140 | `3f2103312b623f47` | section heading/context | CHECKED | 11.3 Embedded codec kind |
| 141 | `253c49464ab5b9fd` | P8E ASS semantics, P8E WebVTT semantics | CHECKED | c id="p8v155-embedded-codec" typedef enum KivoSubtitleEmbeddedCodecKind {     KivoSubtitleEmbeddedCodec_Unknown = 0,     KivoSubtitleEmbeddedCodec_MatroskaSrt = |
| 142 | `c3411d8f9066860b` | section heading/context | CHECKED | 12. Plane State / Timeline |
| 143 | `9e790f854dc394f2` | section heading/context | CHECKED | 12.1 Plane state |
| 144 | `5ce7fb82b8717944` | context paragraph checked | CHECKED | c id="p8v155-plane-state" typedef enum KivoSubtitlePlaneState {     KivoSubtitlePlaneState_NotCreated = 0,     KivoSubtitlePlaneState_Created = 1,     KivoSubti |
| 145 | `58269880f950aeb6` | section heading/context | CHECKED | 12.2 Timeline availability |
| 146 | `5f8930e80d4ada33` | context paragraph checked | CHECKED | c id="p8v155-timeline-availability" typedef enum KivoSubtitleTimelineAvailability {     KivoSubtitleTimelineAvailability_Available = 0,     KivoSubtitleTimeline |
| 147 | `ea6adfa9b5cc8549` | section heading/context | CHECKED | 12.3 Revision equality |
| 148 | `90aad64c603a17e1` | context paragraph checked | CHECKED | ```text id=“p8v155-revision-equal” request revision == latest state revision: not a conflict. build_frame proceeds using request playback_time and matching stat |
| 149 | `b917cf1f62d40941` | P8F evidence | CHECKED | request revision < latest: stale request; emit TimelineInconsistent evidence. |
| 150 | `efbcd6e1c241bb35` | context paragraph checked | CHECKED | request revision > latest: allowed if request carries newer graph/playback revision from P7 adapter; P8 updates diagnostics but does not mutate state from build |
| 151 | `33a4d84be4c51dab` | P8E frame lease | CHECKED | ---  # 13. Frame Lease  ## 13.1 Status  ```c id="p8v155-frame-lease-status" typedef enum KivoSubtitleFrameLeaseStatus {     KivoSubtitleFrameLeaseStatus_Ok = 0, |
| 152 | `a397d77f9bff8e5e` | section heading/context | CHECKED | 13.2 Frame lease struct |
| 153 | `016067d8a14775da` | P8E frame lease | CHECKED | c id="p8v155-frame-lease" typedef struct KivoSubtitleFrameLease {     KivoSubtitleFrameId frame_id;     KivoSubtitleSnapshotRevision snapshot_revision;     Kivo |
| 154 | `3808c4af0e57b9b8` | context paragraph checked | CHECKED | Rules： |
| 155 | `a36dc674312c5c06` | context paragraph checked | CHECKED | text id="p8v155-frame-lease-rules" 1. frame is an opaque pointer to KivoSubtitleFrame layout defined in frame header. 2. lease_id == 0 means no release required |
| 156 | `7714819357d15d8a` | P8F snapshot blob, section heading/context | CHECKED | 14. Snapshot Lease / Snapshot Blob |
| 157 | `530b78c7f5a0e2ed` | section heading/context | CHECKED | 14.1 Snapshot lease status |
| 158 | `b9c7ba15219f922b` | P8F snapshot lease | CHECKED | c id="p8v155-snapshot-lease-status" typedef enum KivoSubtitleSnapshotLeaseStatus {     KivoSubtitleSnapshotLeaseStatus_Ok = 0,     KivoSubtitleSnapshotLeaseStat |
| 159 | `156a2e2f1c77788d` | section heading/context | CHECKED | 14.2 Snapshot lease |
| 160 | `d0f138caa7c0ae10` | P8F snapshot lease | CHECKED | c id="p8v155-snapshot-lease" typedef struct KivoSubtitleSnapshotLease {     KivoSubtitleSnapshotLeaseId snapshot_lease_id;     uint64_t snapshot_lease_generatio |
| 161 | `c577034ed21db2dd` | P8F snapshot blob, section heading/context | CHECKED | 14.3 Snapshot blob constants |
| 162 | `9a828ac53c2f77ed` | context paragraph checked | CHECKED | c id="p8v155-snapshot-constants" #define KIVO_P8_SNAPSHOT_BLOB_MAGIC 0x50385342u /* 'P8SB' */ #define KIVO_P8_SNAPSHOT_ENDIAN_LITTLE 1u #define KIVO_P8_SNAPSHOT |
| 163 | `3fd356a91a83e46b` | section heading/context | CHECKED | 14.4 Snapshot section kind |
| 164 | `bc941766ba155d61` | P8F evidence | CHECKED | c id="p8v155-snapshot-section-kind" typedef enum KivoSubtitleSnapshotSectionKind {     KivoSubtitleSnapshotSection_None = 0,     KivoSubtitleSnapshotSection_Tra |
| 165 | `3808c4af0e57b9b8` | context paragraph checked | CHECKED | Rules： |
| 166 | `571f57b0f6655b04` | P8E frame lease, P8F snapshot lease, P8F snapshot blob, P8F metrics | CHECKED | text id="p8v155-snapshot-section-rules" 1. Metrics are inline in KivoSubtitleSnapshotBlobHeader, not a section. 2. Selected track is inline in header. 3. String |
| 167 | `b41eaf6ef9eda18f` | P8F snapshot blob, section heading/context | CHECKED | 14.5 Snapshot blob header |
| 168 | `c254d6df6ae83840` | context paragraph checked | CHECKED | ```c id=“p8v155-snapshot-header” typedef struct KivoSubtitleSnapshotBlobHeader { uint32_t magic; uint16_t endian; uint16_t header_size; |
| 169 | `30a95fe5b0662440` | P8F metrics, P8F evidence | CHECKED | uint32_t abi_major; uint32_t blob_revision;  uint64_t total_bytes; uint64_t section_count; uint64_t section_table_offset;  KivoSubtitlePlaneState state; KivoSub |
| 170 | `a5fc040321ef6c65` | context paragraph checked | CHECKED | } KivoSubtitleSnapshotBlobHeader; |
| 171 | `11897474b0b6360b` | section heading/context | CHECKED | ## 14.6 Snapshot section  ```c id="p8v155-snapshot-section" typedef struct KivoSubtitleSnapshotBlobSection {     uint32_t kind;     uint32_t element_size;     u |
| 172 | `9080688854399205` | context paragraph checked | CHECKED | Endian rule： |
| 173 | `d155a05e6683bf09` | context paragraph checked | CHECKED | text id="p8v155-endian" P8 P0 baseline is Windows x64 little-endian. P8 copy_snapshot always emits little-endian blob. Big-endian host is unsupported in P8 P0 a |
| 174 | `40a3133cba177bd7` | context paragraph checked | CHECKED | Close/lease rule： |
| 175 | `e4b2c4540e7ff223` | P8F snapshot lease | CHECKED | text id="p8v155-snapshot-close" 1. snapshot_active_lease_capacity default = 32. 2. close waits snapshot_lease_close_timeout_ms default = 2000. 3. after timeout, |
| 176 | `ec92ae958f1eb591` | P8F metrics, P8F evidence, section heading/context | CHECKED | 15. Evidence / Metrics |
| 177 | `70772624fcf17639` | P8F evidence, section heading/context | CHECKED | 15.1 Evidence severity |
| 178 | `357b199548d61b7b` | P8F evidence | CHECKED | c id="p8v155-evidence-severity" typedef enum KivoSubtitleEvidenceSeverity {     KivoSubtitleEvidenceSeverity_Info = 0,     KivoSubtitleEvidenceSeverity_Warning  |
| 179 | `4f7ebedf291b09f5` | P8F evidence, section heading/context | CHECKED | 15.2 Evidence reason additions |
| 180 | `161a3b3ec15b4a9c` | P8F evidence | CHECKED | ```c id=“p8v155-evidence-reason” typedef enum KivoSubtitleEvidenceReason { KivoSubtitleEvidenceReason_None = 0, |
| 181 | `1ec1da8987fe6594` | P8D/P8F privacy, P8D discovery, P8E ASS semantics, P8E WebVTT semantics, P8F evidence | CHECKED | KivoSubtitleEvidenceReason_NotReady, KivoSubtitleEvidenceReason_TimelineInconsistent, KivoSubtitleEvidenceReason_InvalidLanguageTag, KivoSubtitleEvidenceReason_ |
| 182 | `f3343e40e343d8da` | P8F evidence | CHECKED | } KivoSubtitleEvidenceReason; |
| 183 | `a2e1be4ab1dbf79a` | P8F evidence, section heading/context | CHECKED | ## 15.3 Evidence summary  ```c id="p8v155-evidence-summary" typedef struct KivoSubtitleEvidenceSummary {     KivoSubtitleEvidenceId primary_evidence_id;     Kiv |
| 184 | `d26ba10ffc2cd0bc` | P8F metrics, section heading/context | CHECKED | 15.4 Metrics snapshot |
| 185 | `980ef6566b89d1f6` | P8F metrics | CHECKED | ```c id=“p8v155-metrics” typedef struct KivoSubtitleMetricsSnapshot { uint64_t cue_count; uint64_t skipped_cue_count; uint64_t repaired_cue_count; uint64_t unsu |
| 186 | `c898dc500ad13baa` | P8F observer, P8F evidence | CHECKED | uint64_t packet_queue_depth; uint64_t packet_queue_bytes; uint64_t peak_packet_queue_bytes; uint64_t packet_drop_count;  uint64_t visible_cue_count; uint64_t vi |
| 187 | `a51977950483f8a3` | P8F metrics | CHECKED | } KivoSubtitleMetricsSnapshot; |
| 188 | `e58eb8aa69c74421` | P8D/P8F resource token, P8E frame lease, P8F snapshot lease | CHECKED | ---  # 16. ResourceAccessToken  ```c id="p8v155-resource-token" typedef enum KivoSubtitleResourceAccessKind {     KivoSubtitleResourceAccessKind_None = 0,     K |
| 189 | `3808c4af0e57b9b8` | context paragraph checked | CHECKED | Rules： |
| 190 | `8a56661bc5b43767` | P8E frame lease | CHECKED | ```text id=“p8v155-resource-rules” FrameLease token: kind = FrameLease lease_id = frame_lease.lease_id lease_generation = frame_lease.lease_generation snapshot_ |
| 191 | `846dfa7f1fd5deaa` | P8F snapshot lease | CHECKED | SnapshotLease token: kind = SnapshotLease snapshot_lease_id = snapshot_lease.snapshot_lease_id snapshot_lease_generation = snapshot_lease.snapshot_lease_generat |
| 192 | `69ed48baf9b8a79c` | context paragraph checked | CHECKED | snapshot_revision is required for both token kinds. Only the other-kind ID/generation fields must be zero. Non-zero irrelevant ID/generation fields -> Malformed |
| 193 | `b03d6347a588b64b` | P8D discovery | CHECKED | ---  # 17. Discovery API Additions  ## 17.1 Begin request flags  ```text id="p8v155-discovery-flags" KivoSubtitleDiscoveryBeginRequest.flags: bit 0 = allow_cand |
| 194 | `0973cfd32b065246` | section heading/context | CHECKED | 17.2 Priority rule fix |
| 195 | `7a7e944ece903b6b` | P8D discovery | CHECKED | text id="p8v155-discovery-priority" 1. user_added first. 2. forced match when prefer_forced. 3. exact language match. 4. default_suffix when prefer_default. 5.  |
| 196 | `ddd40e10d6abe1a1` | section heading/context | CHECKED | 18. Cancellation View |
| 197 | `4841bc737b7f2910` | context paragraph checked | CHECKED | volatile uint32_t* is forbidden as cross-thread synchronization contract. |
| 198 | `c24c9cbeaedc0f9c` | context paragraph checked | CHECKED | ```c id=“p8v155-cancel-view” typedef struct KivoSubtitleCancellationView { uint64_t operation_id; void* user_data; |
| 199 | `01e90076b9e58340` | context paragraph checked | CHECKED | KivoP8CallbackStatus (KIVO_P8_CALL *is_cancelled)(     void* user_data,     uint64_t operation_id,     uint32_t* out_cancelled ) KIVO_P8_NOEXCEPT; |
| 200 | `a29f27ba9767ad45` | context paragraph checked | CHECKED | } KivoSubtitleCancellationView; |
| 201 | `a8ea342e571f1e01` | context paragraph checked | CHECKED | Rules：  ```text id="p8v155-cancel-rules" 1. out_cancelled == NULL -> InvalidArgument. 2. is_cancelled may be called from worker thread. 3. callback must be thre |
| 202 | `4ee7cca4c1824d83` | P8F observer, section heading/context | CHECKED | 19. Observer Callbacks |
| 203 | `99e8f3c3d6df7bb0` | P8F observer | CHECKED | ```c id=“p8v155-observer” typedef struct KivoSubtitleEvent KivoSubtitleEvent; |
| 204 | `5312ceac71416d98` | P8F observer | CHECKED | typedef struct KivoSubtitleObserverCallbacks { KivoP8AbiHeader header; |
| 205 | `6368b32c94b82f86` | context paragraph checked | CHECKED | void (KIVO_P8_CALL *on_event)(     void* user_data,     const KivoSubtitleEvent* event ) KIVO_P8_NOEXCEPT; |
| 206 | `6675e0ee58cbcf7c` | P8F observer | CHECKED | } KivoSubtitleObserverCallbacks; |
| 207 | `ddec266c19f069ec` | P8F observer | CHECKED | Rules：  ```text id="p8v155-observer-rules" 1. callbacks == NULL or callbacks->on_event == NULL -> subscribe returns invalid token. 2. Observer callback must not |
| 208 | `a48f01eab0e9b13c` | section heading/context | CHECKED | 20. Embedded Stream End Request |
| 209 | `2a9ed592dd493dfd` | context paragraph checked | CHECKED | c id="p8v155-stream-end" typedef struct KivoEmbeddedSubtitleStreamEndRequest {     KivoP8AbiHeader header;     KivoSubtitleTrackId track_id;     KivoSubtitlePac |
| 210 | `5c722444acdf3f7a` | context paragraph checked | CHECKED | Flags： |
| 211 | `e7abc2404afbc451` | context paragraph checked | CHECKED | text id="p8v155-stream-end-flags" bit 0 = eos bit 1 = source_discontinuity bit 2 = parser_drain_requested |
| 212 | `3808c4af0e57b9b8` | context paragraph checked | CHECKED | Rules： |
| 213 | `6b079de2d6fd3f6e` | P8F evidence | CHECKED | text id="p8v155-stream-end-rules" 1. stream end with eos marks embedded stream EOS for matching packet_generation. 2. stale generation -> ignored with evidence. |
| 214 | `54923e4e0fbb07d1` | P8E SSA semantics, P8E ASS semantics, P8E WebVTT semantics, section heading/context | CHECKED | 21. ASS / SSA / WebVTT Closure |
| 215 | `66ed78f51ed2239b` | P8E SSA semantics, section heading/context | CHECKED | 21.1 SSA |
| 216 | `15a8eee2ba010bd9` | P8E SSA semantics, P8E ASS semantics, P8F evidence | CHECKED | text id="p8v155-ssa" SSA v4.00 uses SSA style/dialogue grammar. ASS-specific tags absent in SSA are ignored/unsupported with evidence. SSA line breaks follow AS |
| 217 | `91f80fb624ebb495` | P8E ASS semantics, section heading/context | CHECKED | 21.2 ASS color / alpha |
| 218 | `40d05fd43518f408` | P8E ASS semantics | CHECKED | ```text id=“p8v155-ass-color” ASS color is BGR. quals \1c. \1c primary. \2c secondary. \3c outline. \4c shadow. |
| 219 | `e9a6a2052a88f346` | P8E ASS semantics | CHECKED | ASS alpha is inverse: 00 = opaque. FF = transparent. |
| 220 | `e8192545a62656c0` | P8E ASS semantics, P8F evidence | CHECKED | applies to all channels unless channel-specific alpha overrides. \1a/\2a/\3a/\4a override matching channel. Inline override wins over style default. Malformed c |
| 221 | `44b0f7e10586e752` | P8E ASS semantics, section heading/context | CHECKED | ## 21.3 ASS PlayRes missing  ```text id="p8v155-playres" If PlayResX/Y missing: 1. P8 uses NormalizedFallback. 2. P8 does not invent video pixel size. 3. emits  |
| 222 | `240a11e22e52951b` | P8E WebVTT semantics, section heading/context | CHECKED | 21.4 WebVTT cue settings |
| 223 | `3556c439e013c4eb` | P8E WebVTT semantics | CHECKED | ```text id=“p8v155-webvtt-settings” line: maps to line position metadata. percent uses 0.0..100.0 WebVttPercent. |
| 224 | `acf3f467d60f9792` | context paragraph checked | CHECKED | position: maps to x/anchor metadata. |
| 225 | `97320de0f3de13da` | context paragraph checked | CHECKED | size: maps to width percent. |
| 226 | `0d726206f3e51bf7` | context paragraph checked | CHECKED | align: maps to horizontal alignment. |
| 227 | `f3d946234b4e398f` | P8F evidence | CHECKED | vertical: stored as metadata/unsupported evidence in P8 P0. |
| 228 | `c7b9907402abf562` | context paragraph checked | CHECKED | region: maps to SubtitleRegion if valid. |
| 229 | `deb74d46bbdd5a18` | context paragraph checked | CHECKED | snap-to-lines: stored as metadata flag; P9 decides final layout. |
| 230 | `8f2ae935a4d01d2c` | context paragraph checked | CHECKED | ---  # 22. Ambiguity Closures  ## 22.1 UINT64_MAX cap  ```text id="p8v155-uint64max-cap" cap == UINT64_MAX:   return MalformedInput / InvalidCapValue. P8 P0 doe |
| 231 | `c6d000462b093527` | section heading/context | CHECKED | 22.2 Late packet before timeline |
| 232 | `577f5b255551abe1` | context paragraph checked | CHECKED | text id="p8v155-late-cold-start" If no valid timeline state has been received:   P8 must not late-drop embedded packets by playback time.   P8 may queue within  |
| 233 | `6e32732fdc97d986` | section heading/context | CHECKED | 22.3 EOS epsilon |
| 234 | `984ab763abc7dd7c` | context paragraph checked | CHECKED | text id="p8v155-eos-epsilon" EOS hold epsilon = subtitle_time_compare_epsilon_us from runtime caps. If frame_duration_hint exists, effective epsilon may be clam |
| 235 | `ac84e2d9048630fe` | section heading/context | CHECKED | 22.4 Hazard retry bound |
| 236 | `a5de332a41660cca` | context paragraph checked | CHECKED | text id="p8v155-hazard-bound" rcu_hazard_acquire_max_retries = 3 default. If retries exhausted:   build_frame returns TemporarilyUnavailable/Error by policy.    |
| 237 | `287bc48f5c051e89` | P8G ABI layout guard, section heading/context | CHECKED | 23. ABI Layout Gate |
| 238 | `a52058fc2dd0109e` | P8B public C ABI | CHECKED | All public C ABI structs must have layout tests. |
| 239 | `25b1c969b1912e56` | context paragraph checked | CHECKED | Required checks： |
| 240 | `fa3ec51000359faa` | P8G ABI layout guard | CHECKED | text id="p8v155-layout-gate" 1. sizeof(struct) % 8 == 0 for all cross-DLL top-level structs. 2. _Alignof(struct) >= 8 where supported or explicitly documented n |
| 241 | `dda12ff60749d9a3` | context paragraph checked | CHECKED | If a small wrapper struct naturally has 8-byte size but lower formal alignment: |
| 242 | `cf6589ea3145270f` | P8E ASS semantics, P8G ABI layout guard | CHECKED | text id="p8v155-small-align" Allowed only if: 1. it contains one uint64_t field, or 2. ABI layout test records size/offset, 3. it is passed by value only where  |
| 243 | `999f73125d004ca2` | P8F snapshot blob, section heading/context | CHECKED | 24. Snapshot Blob Endian Responsibility |
| 244 | `e290b1ac2801ebcc` | context paragraph checked | CHECKED | text id="p8v155-endian-responsibility" 1. P8 P0 target is Windows x64 little-endian. 2. copy_snapshot emits little-endian blob. 3. Big-endian host unsupported.  |
| 245 | `51341962096532e1` | section heading/context | CHECKED | 25. Snapshot / Frame Lease Close Policy |
| 246 | `f286e94d7eb02511` | context paragraph checked | CHECKED | text id="p8v155-close-lease-policy" 1. close waits frame leases up to subtitle_frame_lease_close_timeout_ms. 2. close waits snapshot leases up to snapshot_lease |
| 247 | `e66d020ef541723c` | section heading/context | CHECKED | 26. Function Table V1 |
| 248 | `9081e9e0aefb1299` | context paragraph checked | CHECKED | The V1 function table from V1.5.4 remains, with these mandatory changes: |
| 249 | `750b648a5a7fa0f6` | context paragraph checked | CHECKED | text id="p8v155-function-table-delta" 1. All bare noexcept replaced by KIVO_P8_NOEXCEPT. 2. All bool returns replaced by KivoP8ApiStatus or KivoP8CallbackStatus |
| 250 | `d02a0e88890c653f` | P8G/P8H required tests, section heading/context | CHECKED | 27. Required Tests V1.5.5 |
| 251 | `f5ea06fa9a11dc98` | P8C bootstrap/function table | CHECKED | ```text id=“p8v155-tests” c_header_compiles_as_c c_header_no_bare_noexcept c_header_no_std_namespace bootstrap_returns_status_not_bool api_required_prefix_size_ |
| 252 | `80be7258ce2404b9` | P8F observer | CHECKED | mutable_byte_span_defined input_bytes_view_name_unified no_kivo_input_bytes_view_token all_referenced_public_types_defined frame_lease_defined snapshot_lease_de |
| 253 | `81aa73ad5f4272d6` | P8D discovery, P8F metrics | CHECKED | external_source_vtable_has_abi_header external_source_vtable_no_bare_noexcept source_reader_cancel_close_race resource_access_token_irrelevant_fields discovery_ |
| 254 | `fc83c7bd868a5fce` | P8D/P8F privacy | CHECKED | privacy_hash_algorithm_config redacted_source_identity_defined source_identity_hash_deterministic hash_key_provider_defined |
| 255 | `7c341f189981bf68` | context paragraph checked | CHECKED | cancellation_callback_no_volatile cancellation_callback_thread_safe uint64max_cap_rejected late_packet_no_timeline_not_dropped eos_epsilon_from_runtime_caps haz |
| 256 | `c445003a8ff3806c` | P8E ASS semantics | CHECKED | abi_layout_static_asserts_all_public_structs sizeof_offset_public_structs small_wrapper_layout_exceptions_documented close_tombstones_frame_and_snapshot_leases  |
| 257 | `edcc3a0db01600e6` | P8E SSA semantics, P8E ASS semantics, P8E WebVTT semantics | CHECKED | ssa_behavior_defined ass_color_alpha_conversion ass_playres_missing_fallback webvtt_cue_settings_mapping |
| 258 | `5505851aa3960929` | S1-S30 exit criteria, P8B public C ABI, P8C bootstrap/function table, P8D/P8F resource token, P8D/P8F privacy, P8D discovery, P8E SSA semantics, P8E ASS semantics, P8E WebVTT semantics, P8E frame lease, P8F snapshot lease, P8F observer, P8F metrics, P8G ABI layout guard | CHECKED | All V1.5.4 / V1.5.3 / V1.5.2 tests remain required.  ---  # 28. Exit Criteria V1.5.5  ```text id="p8v155-exit" S1   C header compiles as pure C. S2   No bare no |
| 259 | `ffe27b6691330a3b` | P8H final report, section heading/context | CHECKED | 29. Final Report Requirements |
| 260 | `22e194db58c38562` | P8H final report | CHECKED | P8A / implementation final report must include: |
| 261 | `2b533d424f29748c` | context paragraph checked | CHECKED | ```text id=“p8v155-report” branch baseline_commit final_head remote_head changed_files |
| 262 | `aa1ff255aff46c57` | context paragraph checked | CHECKED | public_c_header_list cpp_wrapper_header_list_if_any private_runtime_files private_test_support_files |
| 263 | `3f4ac30a33068283` | S1-S30 exit criteria, P8C bootstrap/function table, P8D/P8F privacy, P8E SSA semantics, P8E ASS semantics, P8E WebVTT semantics, P8F snapshot blob, P8F observer, P8F evidence, P8G ABI layout guard | CHECKED | C header compile result no-bare-noexcept scan result no-std-namespace scan result forbidden token scan result bootstrap symbol result api required prefix size t |
| 264 | `9688d0e47f3b61af` | P8B public C ABI, P8D/P8F privacy, P8F snapshot blob, P8F observer | CHECKED | ---  # 30. Closure Definition  V1.5.5 can be considered the P8 design baseline only when:  ```text id="p8v155-closure" 1. All V1.5.5 exit criteria are represent |
| 265 | `113d3076256760e7` | context paragraph checked | CHECKED | Implementation completion state: |
| 266 | `60ab0aff74db8d41` | context paragraph checked | CHECKED | text id="p8v155-final-state" Subtitle Plane CLOSED. Ready for P9 Player Runtime Shell / Qt Adapter. |
| 267 | `efbcdc3545d8a858` | section heading/context | CHECKED | 31. Final Delivery Statement |
| 268 | `f1b294b4a1fa84a7` | context paragraph checked | CHECKED | P8 implementation completed from this design must be able to state: |
| 269 | `38b85f4121704073` | P8B public C ABI | CHECKED | ```text id=“p8v155-final-statement” Kivo Video P8 Subtitle Plane exposes a pure C ABI. |
| 270 | `acf1ab472fef9012` | P8B public C ABI, P8C bootstrap/function table | CHECKED | The public C header compiles in a C compiler. No bare noexcept appears in the C ABI header. No std:: namespace appears in the C ABI header. The bootstrap API re |
| 271 | `1eab977283e63edc` | P8D/P8F privacy, P8E frame lease, P8F snapshot lease, P8F observer, P8F metrics, P8F evidence | CHECKED | All referenced public C types are fully defined. KivoMutableByteSpan is defined. KivoSubtitleInputBytesView is the only input bytes view name. FrameLease, Snaps |
| 272 | `44fd775383412893` | P8D source reader, P8D/P8F resource token, P8D discovery, P8F snapshot blob, P8F observer, P8F metrics | CHECKED | Source reader vtable is versioned. Observer callback table is versioned. Cancellation uses callback-based polling, not volatile pointer. ResourceAccessToken rul |
| 273 | `cc19e9ef49291895` | P8D/P8F privacy | CHECKED | Privacy hash algorithm and key provider are defined. source_identity_hash, basename_hash, and content_fingerprint_hash have deterministic rules. UINT64_MAX caps |
| 274 | `89d8b6f19a008452` | P8E SSA semantics, P8E ASS semantics, P8E WebVTT semantics | CHECKED | SSA behavior, ASS color/alpha, ASS PlayRes missing fallback, and WebVTT cue settings are defined. |
| 275 | `6a2f9afa87dce7c7` | context paragraph checked | CHECKED | Public headers contain no Qt/FFmpeg/Win32 renderer types and no forbidden C++ ABI types. Frame path contains no mutex/shared_mutex/blocking wait. No P4/P5/P6/P7 |
| 276 | `c11bf4232b3c2b26` | context paragraph checked | CHECKED | END OF KIVO VIDEO P8 SUBTITLE PLANE DESIGN V1.5.5 |
