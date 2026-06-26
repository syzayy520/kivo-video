# P2 Source Core Skeleton 001 Execution Card

Updated: 2026-06-26

Canonical task id: `KIVO-VIDEO-P2-SOURCE-CORE-SKELETON-001`.
Short task id: `SRC-CORE-SKELETON-001`.
Task type: architecture skeleton, contracts-first, source-session-centered, compile-only, minimal contract tests, no provider runtime.

This card converts the pasted task book into a directly executable assistant task. The pasted task book `<REDACTED>/pasted-text.txt` is the design authority. Historical source-core planning is reference only and must not override this task. Historical source-core planning is reference only and must not override this task.

## Position

- This is not the older planning-only `SRC-CORE-PLANNING-001`.
- This is not the older docs-only `SRC-001`.
- This task implements a contract skeleton only.
- This task must not implement provider runtime, player runtime, real I/O, final stream decisions, media pipeline integration, UI, rendering, decoding, cache runtime, credential exchange, or device/audio subsystems.
- Final classification is only `PASS_P2_SOURCE_CORE_SKELETON_001` or `FAIL_P2_SOURCE_CORE_SKELETON_001`.
- Evidence status must be `CONTRACT_PASS`, with `runtime_verified=false`.

## SCS-001 Branch / Base / Clean Gate

- Accepted base: `532ad9b7bde3ef3ffc6071a0d2ab03d5696f8f48`.
- Required branch: `kivo-video-p2-source-core-skeleton-001`.
- Run `git branch --show-current`.
- Run `git rev-parse HEAD`.
- Run `git status --short`.
- STOP if workspace has unrelated dirty files.
- STOP if HEAD is lower than accepted base.
- STOP if branch/base cannot be confirmed.

## SCS-002 Redline Baseline Gate

Run before implementation:

```powershell
python backend/tools/governance/redline_gate/redline_checker.py --self-test
python backend/tools/governance/redline_gate/redline_checker.py --root=. --scan=all
```

Required:

- self-test PASS.
- scan-all PASS.
- violation count is zero.
- Do not change the redline checker to make this task pass.

## SCS-003 Locked Three-Layer Architecture

Layer 1: `source_core/contracts`

- Pure stable contracts, enums, value objects, evidence structs, and direct input structs.
- No virtual interface layer.
- No dynamic dispatch infrastructure.
- No provider implementation.
- No runtime state ownership.
- No real I/O.
- No platform object exposure.
- No thread, callback, lock, or event-loop model.

Layer 2: `source_core/runtime`

- Abstract runtime boundary contracts for session, open, read, seek, cancel, timeout, and runtime error.
- May depend on contracts.
- Must not depend on providers.
- Must not perform real I/O.
- Must not own provider dispatch infrastructure.
- Must not expose native platform objects.

Layer 3: `providers/*`

- Future provider family boundaries only.
- This task may add README or `.gitkeep` placeholders and optional CMake INTERFACE targets.
- Provider placeholder targets may depend at most on contracts.
- Provider placeholder targets must not link runtime.
- No provider source files with runtime behavior.

## SCS-004 Canonical Paths / Namespace / Legacy Rule

Canonical include roots:

- `backend/include/kivo/video/source_core/**`
- `backend/include/kivo/video/providers/**`

Canonical source roots:

- `backend/src/source_core/**`
- `backend/src/providers/**`

Canonical test roots:

- `backend/tests/source_core/**`
- `backend/tests/providers/**`

Canonical namespace:

- `kivo::video::source_core`

Legacy rule:

- Do not migrate, delete, rename, or refactor legacy source-core files under the old cinema-engine path.
- New files must not include legacy source-core headers.
- STOP if project namespace constraints make the canonical namespace impossible.

## SCS-005 Directory Family Gate

Contracts families:

- `provider_kind`
- `uri`
- `identity`
- `capability`
- `auth`
- `redaction`
- `error`
- `evidence`
- `decision_input`

Runtime families:

- `session`
- `open`
- `read`
- `seek`
- `cancel`
- `timeout`
- `error`

Provider boundary families:

- `local_file`
- `http`
- `webdav`
- `alist`
- `emby`
- `cloud_drive`
- `hls`

Each family must use semantic directories under include/src/tests roots. Use README or `.gitkeep` only for empty provider placeholders.

## SCS-006 SourceSession-Centered Model

Required model objects:

- `SourceIdentity`
- `SourceCapabilitySnapshot`
- `SourceOpenRequest`
- `SourceOpenResult`
- `SourceSession`
- `SourceReadRequest`
- `SourceReadResult`
- `SourceSeekRequest`
- `SourceSeekResult`
- `SourceEvidenceSnapshot`
- `DirectPlayInput`
- `DirectStreamInput`

Rules:

- `SourceSessionState` must be a typed enum with at least `created`, `open`, `closed`, and `error`.
- `SourceSession` must expose only session id, state, and contract version.
- Debug output must expose only safe session id, state, and contract version.
- Open/read/seek request and result objects must bind to `SourceSessionId`.
- `SourceSession` must not expose handles, file descriptors, provider objects, or native objects.

## SCS-007 Contract Version Gate

Required header:

- `backend/include/kivo/video/source_core/contracts/decision_input/source_core_contract_version.hpp`

Required constant:

```cpp
namespace kivo::video::source_core {
inline constexpr std::uint32_t kSourceCoreContractVersion = 1;
}
```

Rules:

- Define this constant once only.
- Do not create a separate version directory for this task.
- `SourceSession::contract_version` must initialize from this constant.
- `DirectPlayInput` and `DirectStreamInput` schema version fields must use this constant.
- Tests must verify the value is `1`.

## SCS-008 Typed Contract Gate

Provider kind:

- Use a typed enum, not a string type.
- Include the provider families named by this task plus `unknown`.
- `SourceIdentity` must store the typed provider kind.

URI:

- Define distinct raw, canonical, and redacted source URI types.
- They must be real types, not aliases.
- Raw input exists only at the open-request boundary.
- Raw input must not expose a public raw-value accessor.
- Raw debug output must be redacted.
- Identity, evidence, error, session, and direct input objects must not store raw input.

Identity:

- `SourceIdentityId` must be a strong wrapper.
- `SourceIdentity` may carry provider kind, opaque key/digest, redacted URI, and safe display label.
- It must not store secrets, raw input, local path text, original remote URL text, credential material, headers, platform objects, or provider objects.

Capability:

- Use a typed capability state with `unknown`, `supported`, and `unsupported`.
- Capability fields such as read, seek, range, direct-play candidate, and direct-stream candidate use that typed state, not bare booleans.
- Known length, MIME type, and container hint use optionals.
- Unknown is not an empty string and not false.

Auth:

- Define auth requirement, auth kind, and secret reference contracts.
- Secret reference is opaque only.
- Do not store real or fake credential values in tests, evidence, logs, or debug strings.

Redaction:

- Define policy/helper contracts for safe debug, evidence, and error text.
- Raw input must never become evidence text.

Error:

- Define typed source error code and source error.
- Business failures use typed results and errors.
- No exception-based business-control path.

Evidence:

- Skeleton tests produce contract-pass-only evidence.
- Runtime-pass evidence must not be produced by this skeleton.
- Evidence must not store raw input.
- Sequence numbers must be monotonic.

Direct input:

- Define `DirectPlayInput` and `DirectStreamInput` as input carriers only.
- They must not contain final strategy logic.
- They must use the central contract version constant.
- They must not store raw input.

Runtime:

- Define session id, session state, session, open request/result, read request/result, seek origin/request/result, cancel token, timeout, and runtime error.
- `SourceOpenResult` must be a strong wrapper, not a bare alias around a variant.
- Success holds `SourceSession`; failure holds `SourceError`.
- Successful open produces state `open`.
- Read and seek requests bind `SourceSessionId`.
- Read result documents would-block semantics.
- Seek result documents that new position is undefined when an error exists.
- Timeout infinite value uses max `std::uint64_t`.
- Cancel token documents that this skeleton does not provide thread-safety guarantees.

## SCS-009 CMake Target Gate

Required targets:

- `kivo_video_source_core_contracts`
- `kivo_video_source_core_runtime`
- `kivo_video_source_core_contract_tests`
- `kivo_video_source_core_runtime_tests`

Provider placeholder target pattern:

- `kivo_video_providers_<name>`

Dependency rules:

- contracts target must not link runtime or providers.
- runtime target may link contracts only.
- contract tests may link contracts and test framework only.
- runtime tests may link runtime, contracts, and test framework only.
- provider placeholder targets are INTERFACE-only and must not link runtime.

## SCS-010 Contract Test Gate

Tests must be split by family. Do not create one all-in-one test.

Required coverage:

- provider kind typed enum and no implicit integer conversion.
- URI three-type separation, no raw public accessor, redacted debug output.
- identity safe debug output and no credential/raw input dependency.
- capability defaults and optionals represent unknown correctly.
- evidence remains contract-pass-only in skeleton tests.
- session-centered open/read/seek model.
- direct input objects are not decision engines.
- open result strong wrapper success/failure semantics.
- read would-block and seek error semantics.
- timeout infinite semantics.
- cancel token skeleton thread-safety statement.

If the project is C++17, use C++17 type traits and detection idioms for compile-time guards. Do not raise the C++ standard to make tests easier.

## SCS-011 Allowed Files Gate

Only these areas are allowed for implementation/test/config:

- `backend/include/kivo/video/source_core/contracts/**`
- `backend/include/kivo/video/source_core/runtime/**`
- `backend/src/source_core/contracts/**`
- `backend/src/source_core/runtime/**`
- `backend/tests/source_core/contracts/**`
- `backend/tests/source_core/runtime/**`
- `backend/include/kivo/video/providers/**`
- `backend/src/providers/**`
- `backend/tests/providers/**`
- backend CMake files required for these targets.

Required record/evidence files:

- `artifacts/p2/evidence/SRC-CORE-SKELETON-001.json`
- `docs/roadmap/backend_task_checklist.md`
- `.codebuddy/memory/2026-06-26.md`

Do not copy the task book's forbidden-token list into memory, checklist, or evidence.

## SCS-012 Evidence JSON Gate

Required evidence file:

- `artifacts/p2/evidence/SRC-CORE-SKELETON-001.json`

Required top-level facts:

- schema version is `p2-evidence-v8`.
- task id is `SRC-CORE-SKELETON-001`.
- task family is `source_core`.
- status is `CONTRACT_PASS`.
- runtime verified is false.
- runtime environment kind is `not_applicable`.
- redaction verified is true.
- schema validated is true.

Required nested implementation facts:

- contracts skeleton present.
- runtime skeleton present.
- provider runtime absent.
- contracts family count is 9.
- runtime family count is 7.
- provider boundary count is 7.
- contract version is 1.
- source-session-centered model is true.
- contracts dependency direction is clean.
- runtime dependency direction is clean.
- provider runtime absent is true.

STOP if the schema validator rejects these fields and passing would require fake evidence.

## SCS-013 Scan Gates

Required:

- forbidden-token scan on changed files, using the exact task book scan rules.
- no exception-control scan on changed source/header/test files.
- legacy include scan on new source-core files.
- do not paste the forbidden-token list into repository checklist, memory, or evidence.

## SCS-014 Build / Test Gate

Minimum required checks:

```powershell
python backend/tools/governance/redline_gate/redline_checker.py --self-test
python backend/tools/governance/redline_gate/redline_checker.py --root=. --scan=all
python backend/tools/governance/schema_gate/validator/schema_gate_validator.py artifacts/p2/evidence/SRC-CORE-SKELETON-001.json
```

Recommended build and test shape:

```powershell
cmake -S backend -B build/backend -DCMAKE_BUILD_TYPE=Debug
cmake --build build/backend --target kivo_video_source_core_contracts
cmake --build build/backend --target kivo_video_source_core_runtime
cmake --build build/backend --target kivo_video_source_core_contract_tests
cmake --build build/backend --target kivo_video_source_core_runtime_tests
ctest --test-dir build/backend -R "source_core" --output-on-failure
```

If the repository uses a different real CMake entry, use the repository entry but preserve the required target names.

## SCS-015 CMake Dependency Audit Gate

Final report must prove:

- contracts target does not depend on runtime/providers.
- runtime target depends only on contracts.
- contract tests depend only on contracts and test framework.
- runtime tests depend only on runtime, contracts, and test framework.
- provider placeholders do not link runtime.

Use CMake target snippets or dependency graph output.

## SCS-016 STOP / Fail Conditions Gate

STOP and report if:

- unrelated dirty workspace exists.
- HEAD is lower than accepted base.
- redline baseline fails.
- build requires touching forbidden subsystems.
- source-core work requires provider implementation.
- tests require real provider runtime, real local file reads, or network access.
- canonical namespace cannot be used.
- legacy source-core files must be modified.
- runtime must depend on providers.
- contracts need virtual interfaces to express this skeleton.
- raw input cannot be kept out of identity, session, evidence, error, or direct input objects.
- strong open result wrapper cannot be created without falling back to a bare alias.
- schema validation cannot pass without fake evidence.

STOP report must include trigger condition, completed checks, reason execution cannot continue, suggested next step, and current uncommitted state.

## SCS-017 Final Report / Commit Gate

If implementation proceeds:

- Use branch `kivo-video-p2-source-core-skeleton-001`.
- Confirm clean workspace before and after.
- If all checks pass, commit message must be `feat(source-core): add source core contract skeleton`.
- Push the task branch if credentials and remote allow it.
- If commit or push is blocked, report honestly.

Final report must include:

- classification.
- branch.
- commit HEAD.
- push status.
- base proof.
- changed files grouped by family.
- SourceSession-centered model proof.
- provider kind typed enum proof.
- URI three-layer proof.
- identity redaction proof.
- capability evidence-based proof.
- evidence contract-pass/runtime-pass guard proof.
- direct input carrier-only proof.
- open result strong wrapper proof.
- read/seek result semantics proof.
- dependency direction proof.
- legacy source-core untouched proof.
- forbidden-token scan result.
- no exception-control scan result.
- legacy include scan result.
- CMake dependency audit.
- build result.
- test result.
- redline result.
- schema validation result.
- final git status.
- STOP condition status.
- known limitations.
- final recommendation.

Required final statement:

```text
This implements Source Core skeleton only.
This does NOT implement provider runtime.
This does NOT mean All Providers Runtime Verified.
This does NOT mean P2 complete.
This does NOT mean P3/HDR/Atmos/AirPlay complete.
```
