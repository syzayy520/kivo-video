# KIVO-VIDEO-P8A PLANNING DRAFT

**Task ID**: `KIVO-VIDEO-P8-SUBTITLE-PLANE-DESIGN-LOCK-001-V1.5.5`
**Status**: `P8A_CORRECTIVE_NATURAL_TREE_LOCK`
**Rule source**: `.codex/memory/kivo-video-iron-gate.md`
**Source design**: `C:\Users\huyasi\Downloads\Kivo Video P8 Subtitle Plane Design V1.5.5.docx`

## 1. Corrective Lock

The P8 implementation must not use a monolithic public ABI header or a monolithic runtime source file.

The following rejected shapes are forbidden:

- one 700+ line `kivo_subtitle_plane.h` that owns all public C ABI types,
- one 1000+ line runtime file that owns bootstrap, state, command, source, privacy, frame, snapshot, observer, and format behavior,
- catch-all `core`, `common`, `utils`, or `helpers` files that mix unrelated responsibilities,
- implementation before the natural family tree is written and checked.

The corrected shape is a natural C ABI header family plus private runtime families. `kivo_subtitle_plane.h` is only an umbrella include and bootstrap declaration.

## 2. Public C ABI Family

All files in this family must compile as C and may expose only pure C ABI. External system includes are limited to `stdint.h` and `stddef.h`, through the foundation header.

| File | Responsibility | Owner gate | Allowed dependencies | Forbidden dependencies |
|---|---|---|---|---|
| `include/kivo/video/subtitle_plane/foundation/p8_export.h` | Export/calling-convention/no-exception macros only | P8B-001 | none | types, function table, STL, Qt, FFmpeg, Win32 renderer |
| `include/kivo/video/subtitle_plane/foundation/p8_version.h` | ABI version constants only | P8B-002 | `p8_export.h` | status enums, runtime functions |
| `include/kivo/video/subtitle_plane/foundation/p8_status.h` | `KivoP8ApiStatus` and `KivoP8CallbackStatus` only | P8B-002 | `p8_version.h` | subtitle plane errors, runtime state |
| `include/kivo/video/subtitle_plane/foundation/p8_abi_header.h` | `KivoP8AbiHeader` only | P8B-002 | `p8_status.h` | API table, source, snapshot |
| `include/kivo/video/subtitle_plane/types/subtitle_ids.h` | Opaque numeric IDs/revisions/epochs only | P8B-003 | `p8_abi_header.h` | commands, source descriptors |
| `include/kivo/video/subtitle_plane/types/subtitle_time.h` | time and delay scalar wrappers only | P8B-003 | `subtitle_ids.h` | timeline state machine |
| `include/kivo/video/subtitle_plane/input/subtitle_input_views.h` | input string/bytes and mutable byte span only | P8B-005 | `p8_abi_header.h` | source reader behavior |
| `include/kivo/video/subtitle_plane/generation/subtitle_generation.h` | content/packet/timeline/frame generation structs only | P8B-003 | `subtitle_ids.h`, `subtitle_time.h` | frame lease behavior |
| `include/kivo/video/subtitle_plane/error/subtitle_error.h` | result and plane error enums only | P8B-004 | `p8_abi_header.h` | command lifecycle state |
| `include/kivo/video/subtitle_plane/command/subtitle_command.h` | command token and lifecycle public structs only | P8B-004 | `subtitle_ids.h`, `subtitle_error.h` | command registry implementation |
| `include/kivo/video/subtitle_plane/privacy/subtitle_privacy.h` | hash algorithm, key provider, policy, redacted identity only | P8B-006 | `subtitle_ids.h`, `subtitle_input_views.h` | source reader implementation |
| `include/kivo/video/subtitle_plane/source/subtitle_source.h` | source handle, seek origin, source kind, vtable, descriptor only | P8B-006 | `subtitle_input_views.h`, `subtitle_privacy.h` | file I/O implementation |
| `include/kivo/video/subtitle_plane/format/subtitle_format.h` | format, track kind, embedded codec kind only | P8B-006 | `p8_abi_header.h` | parser implementation |
| `include/kivo/video/subtitle_plane/state/subtitle_plane_state.h` | plane state and timeline availability enums only | P8B-006 | `p8_abi_header.h` | state machine implementation |
| `include/kivo/video/subtitle_plane/evidence/subtitle_evidence.h` | evidence severity/reason/summary only | P8B-006 | `subtitle_ids.h` | evidence ring implementation |
| `include/kivo/video/subtitle_plane/metrics/subtitle_metrics.h` | metrics snapshot struct only | P8B-006 | `p8_abi_header.h` | counters implementation |
| `include/kivo/video/subtitle_plane/frame/subtitle_frame_lease.h` | frame lease public struct only | P8B-006 | `subtitle_generation.h`, `subtitle_error.h` | frame pool implementation |
| `include/kivo/video/subtitle_plane/snapshot/subtitle_snapshot_blob.h` | snapshot constants, section, header, and lease public structs only | P8B-006 | `subtitle_frame_lease.h`, `subtitle_evidence.h`, `subtitle_metrics.h` | serializer implementation |
| `include/kivo/video/subtitle_plane/resource/subtitle_resource_token.h` | resource access token only | P8B-006 | `subtitle_snapshot_blob.h` | validation implementation |
| `include/kivo/video/subtitle_plane/cancel/subtitle_cancellation.h` | cancellation callback view only | P8B-006 | `p8_status.h` | worker implementation |
| `include/kivo/video/subtitle_plane/observer/subtitle_observer.h` | event, callbacks, subscription token use only | P8B-006 | `subtitle_evidence.h`, `subtitle_snapshot_blob.h` | observer registry implementation |
| `include/kivo/video/subtitle_plane/stream/subtitle_stream_end.h` | embedded stream end request only | P8B-006 | `subtitle_generation.h`, `subtitle_ids.h` | packet queue implementation |
| `include/kivo/video/subtitle_plane/config/subtitle_plane_config.h` | runtime caps and plane config only | P8B-006 | `subtitle_privacy.h` | runtime defaults implementation |
| `include/kivo/video/subtitle_plane/request/subtitle_plane_requests.h` | discovery, frame build, snapshot acquire, style report request/result structs only | P8B-006 | config/source/frame/snapshot/cancel/format headers | implementation functions |
| `include/kivo/video/subtitle_plane/api/subtitle_plane_api_v1.h` | `KivoSubtitlePlaneApiV1`, required prefix, bootstrap declaration only | P8B-007 | public request/resource/observer headers | private runtime |
| `include/kivo/video/subtitle_plane/kivo_subtitle_plane.h` | umbrella include for the public C ABI family only | P8B-001 | public family headers | any type or behavior definition |

## 3. Private Runtime Family

Private runtime files may use C++ but must stay outside the public ABI and keep one responsibility each.

| File | Responsibility | Owner gate | Allowed dependencies | Forbidden dependencies |
|---|---|---|---|---|
| `backend/include_private/video/subtitle_plane/abi/api_table_builder.hpp` | API table construction declarations only | P8C-002 | public API headers | plane state storage |
| `backend/src/video/subtitle_plane/abi/api_bootstrap.cpp` | `kivo_subtitle_plane_get_api_v1` validation and table copy only | P8C-002 | api table builder | command/source/frame behavior |
| `backend/src/video/subtitle_plane/abi/api_table_builder.cpp` | fills function table with non-null V1 pointers only | P8C-003 | per-family C adapter function headers | behavior bodies |
| `backend/include_private/video/subtitle_plane/abi/api_function_declarations.hpp` | declarations for public ABI adapter functions only | P8C-003 | public API headers | behavior bodies |
| `backend/include_private/video/subtitle_plane/core/plane_store.hpp` | fixed-slot handle store declarations only | P8C-001 | public config/state | command/source/frame logic |
| `backend/src/video/subtitle_plane/core/plane_store.cpp` | create/find/destroy plane slots only | P8C-001 | plane state record | source/parser/snapshot behavior |
| `backend/include_private/video/subtitle_plane/core/plane_state_record.hpp` | internal plane state data layout only | P8C-005 | public scalar/state types | function table |
| `backend/src/video/subtitle_plane/core/runtime_caps.cpp` | runtime cap defaults and `UINT64_MAX` validation only | P8C-006 | public config | source/frame behavior |
| `backend/include_private/video/subtitle_plane/core/runtime_caps.hpp` | runtime cap function declarations only | P8C-006 | public config | plane store |
| `backend/src/video/subtitle_plane/core/plane_lifecycle_api.cpp` | public ABI create/close/destroy adapters only | P8C-001 | plane store, runtime caps | command/source/frame behavior |
| `backend/src/video/subtitle_plane/command/command_lifecycle.cpp` | command token allocation/query/lifecycle only | P8C-004 | plane state record | source parser behavior |
| `backend/include_private/video/subtitle_plane/command/command_lifecycle.hpp` | command lifecycle helper declarations only | P8C-004 | public command types | source parser behavior |
| `backend/src/video/subtitle_plane/command/command_api.cpp` | public ABI command query adapter only | P8C-004 | command lifecycle, plane store | discovery/source behavior |
| `backend/src/video/subtitle_plane/timeline/timeline_revision.cpp` | revision equality/stale/newer comparison only | P8C-005 | public revision types | command allocation |
| `backend/include_private/video/subtitle_plane/timeline/timeline_revision.hpp` | timeline revision comparison declarations only | P8C-005 | public revision types | command allocation |
| `backend/src/video/subtitle_plane/frame/frame_build_api.cpp` | public ABI frame build/release adapter shell only | P8C-005/P8C-007 | timeline revision, cancellation poll | frame lease pool behavior |
| `backend/src/video/subtitle_plane/threading/cancellation_poll.cpp` | callback cancellation polling only | P8C-007 | public cancellation view | source close logic |
| `backend/include_private/video/subtitle_plane/threading/cancellation_poll.hpp` | cancellation polling declarations only | P8C-007 | public cancellation view | source close logic |
| `backend/src/video/subtitle_plane/discovery/discovery_api.cpp` | public ABI begin discovery adapter shell only | P8C-003/P8D-004 | command lifecycle | discovery ranking implementation |
| `backend/src/video/subtitle_plane/source/source_api.cpp` | public ABI set source adapter shell only | P8C-003/P8D-001 | command lifecycle | reader implementation |
| `backend/src/video/subtitle_plane/source/embedded_stream_api.cpp` | public ABI stream-end adapter shell only | P8C-003/P8D-006 | command lifecycle | parser implementation |
| `backend/src/video/subtitle_plane/snapshot/snapshot_api.cpp` | public ABI snapshot acquire/copy/release shell only | P8C-003/P8F-001 | plane store | snapshot blob implementation |
| `backend/src/video/subtitle_plane/text/text_access_api.cpp` | public ABI text/font copy shell only | P8C-003/P8F-007 | resource token validation | storage implementation |
| `backend/src/video/subtitle_plane/observer/observer_api.cpp` | public ABI subscribe/unsubscribe shell only | P8C-003/P8F-005 | plane store | observer registry implementation |
| `backend/src/video/subtitle_plane/metrics/metrics_api.cpp` | public ABI metrics copy shell only | P8C-003/P8F-004 | plane state record | evidence aggregation |
| `backend/src/video/subtitle_plane/privacy/privacy_api.cpp` | public ABI redacted identity shell only | P8C-003/P8D-003 | public privacy/source types | hashing implementation |
| `backend/src/video/subtitle_plane/format/style_semantics_api.cpp` | public ABI style semantic shell only | P8C-003/P8E-004..006 | public format/request types | parser runtime |
| `backend/src/video/subtitle_plane/source/source_descriptor_validation.cpp` | descriptor shape and input lifetime validation only | P8D-001 | public source/input/privacy types | reader execution |
| `backend/src/video/subtitle_plane/source/external_reader_lifecycle.cpp` | open/read/cancel/close lifecycle only | P8D-001/P8D-008 | source descriptor validation | discovery ranking |
| `backend/src/video/subtitle_plane/source/embedded_stream_end.cpp` | embedded EOS/discontinuity/drain request handling only | P8D-006 | packet generation | parser implementation |
| `backend/src/video/subtitle_plane/discovery/discovery_priority.cpp` | discovery candidate ordering only | P8D-004 | redacted identity | source I/O |
| `backend/src/video/subtitle_plane/privacy/redacted_identity.cpp` | source identity hash and redaction only | P8D-003 | hash key provider | evidence ring |
| `backend/src/video/subtitle_plane/privacy/privacy_redline_policy.cpp` | privacy mode/evidence export checks only | P8F-008 | redacted identity | source opening |
| `backend/src/video/subtitle_plane/format/ssa_semantics.cpp` | SSA behavior only | P8E-004 | public format/style report | ASS/WebVTT behavior |
| `backend/src/video/subtitle_plane/format/ass_semantics.cpp` | ASS color/alpha/PlayRes behavior only | P8E-005 | public format/style report | SSA/WebVTT behavior |
| `backend/src/video/subtitle_plane/format/webvtt_semantics.cpp` | WebVTT cue setting mapping only | P8E-006 | public format/style report | SSA/ASS behavior |
| `backend/src/video/subtitle_plane/generation/generation_freshness.cpp` | generation comparison and cold-start packet rules only | P8E-003 | generation types | frame lease pool |
| `backend/src/video/subtitle_plane/frame/frame_lease_pool.cpp` | frame lease acquire/release/pool exhaustion only | P8E-007 | generation freshness | snapshot serializer |
| `backend/src/video/subtitle_plane/frame/frame_hot_path_guard.cpp` | hot-path retry and non-blocking policy only | P8E-008/P8E-009 | cancellation poll | source I/O |
| `backend/src/video/subtitle_plane/snapshot/snapshot_lease_pool.cpp` | snapshot lease acquire/release only | P8F-001 | plane state record | blob serialization |
| `backend/src/video/subtitle_plane/snapshot/snapshot_blob_writer.cpp` | little-endian snapshot blob copy only | P8F-002/P8F-003 | snapshot lease, metrics, evidence | text buffer store |
| `backend/src/video/subtitle_plane/evidence/evidence_summary.cpp` | severity/reason summary aggregation only | P8F-004 | public evidence types | privacy hashing |
| `backend/src/video/subtitle_plane/metrics/metrics_snapshot.cpp` | metrics snapshot counters only | P8F-004 | public metrics type | evidence redaction |
| `backend/src/video/subtitle_plane/observer/observer_registry.cpp` | subscribe/unsubscribe/delivery guard only | P8F-005 | public observer types | command lifecycle internals |
| `backend/src/video/subtitle_plane/text/text_buffer_store.cpp` | frame-scoped text buffer copy only | P8F-007 | resource token validation | snapshot blob writer |
| `backend/src/video/subtitle_plane/text/font_blob_store.cpp` | snapshot-scoped font blob copy only | P8F-007 | resource token validation | text buffer copy |
| `backend/src/video/subtitle_plane/resource/resource_token_validation.cpp` | resource token irrelevant-field validation only | P8D-002 | public token type | text/font storage |
| `backend/src/video/subtitle_plane/threading/close_tombstone_policy.cpp` | bounded close and tombstone policy only | P8F-006 | lease pools | source reader |

## 4. Test and Tool Family

| File family | Responsibility | Owner gate |
|---|---|---|
| `backend/tests/video/subtitle_plane/abi/*` | C header, forbidden token, bootstrap, API table, ABI layout tests | P8B/P8G |
| `backend/tests/video/subtitle_plane/command/*` | command token/lifecycle tests | P8C |
| `backend/tests/video/subtitle_plane/source/*` | source reader, resource token, discovery, embedded stream tests | P8D |
| `backend/tests/video/subtitle_plane/format/*` | SSA/ASS/WebVTT semantic tests | P8E |
| `backend/tests/video/subtitle_plane/frame/*` | frame lease and hot-path tests | P8E |
| `backend/tests/video/subtitle_plane/snapshot/*` | snapshot lease/blob/endian tests | P8F |
| `backend/tests/video/subtitle_plane/observer/*` | observer callback/subscription tests | P8F |
| `backend/tests/video/subtitle_plane/test_support/*` | deterministic fake readers, keys, clocks, and observer probes only | P8G |
| `backend/tools/video/subtitle_plane/c_header_purist/*` | public C ABI forbidden-token scanner only | P8G-002 |
| `backend/tools/video/subtitle_plane/abi_layout_guard/*` | ABI layout manifest/scanner only | P8G-003 |
| `backend/tools/video/subtitle_plane/architecture_guard/*` | family boundary/frozen-contract guard only | P8G-007 |
| `backend/tools/video/subtitle_plane/matrix_codegen/*` | S1-S30 and design-ID coverage matrix only | P8G-001 |

## 5. File Size Guard

Line count is not the primary design metric; responsibility is. However, P8 now has an explicit corrective guard:

- Public header family files should stay under 220 lines each unless the file is an enum-only contract and the evidence explains why.
- Product `.cpp` files should stay under 320 lines each unless a gate explicitly records a split review.
- Test files should stay under 420 lines each unless they are a generated matrix test with a tool-owned source of truth.
- Any file crossing its guard must stop the stage for split review before closure.

## 6. P8A Decision

P8B may start only after this corrected tree is used as the implementation allowlist. The previously attempted monolithic public header and monolithic runtime file are rejected and removed.
