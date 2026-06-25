# P2-051 Media Parser Security / Fuzz Skeleton

## Purpose

Defines media parser security contracts, quarantine mechanisms, and fuzz testing shell requirements for safe parsing of untrusted media files.

## Scope

- Parser security core contracts
- Quarantine policy for suspicious files
- Fuzz testing shell requirements
- Input validation rules
- Parser failure handling

## Family Tree (parser_security_core/)

```
parser_security_core/
├── parser_security_policy.hpp        — Parser security policy rules
├── quarantine_policy.hpp             — Quarantine policy for suspicious files
├── fuzz_test_shell.hpp               — Fuzz test shell requirements
├── fake_parser_security_service.hpp  — Fake service for testing
```

## Contract Types

### ParserSecurityPolicy

| Field | Type | Description |
|-------|------|-------------|
| policy_id | string | Unique policy identifier |
| max_input_size_bytes | uint64_t | Maximum allowed input size |
| max_depth | int | Maximum nesting depth |
| reject_unknown_boxes | bool | Whether to reject unknown container boxes |
| timeout_ms | int | Parser timeout in milliseconds |
| require_header_signature | bool | Whether to require file signature |

### QuarantinePolicy

| Field | Type | Description |
|-------|------|-------------|
| policy_id | string | Unique policy identifier |
| quarantine_duration_seconds | int | How long to quarantine suspicious files |
| auto_delete_quarantined | bool | Whether to auto-delete quarantined files |
| max_quarantine_size_mb | int | Maximum quarantine storage |
| notify_on_quarantine | bool | Whether to notify when file is quarantined |
| escalation_threshold | int | Number of violations before escalation |

### FuzzTestShell

| Field | Type | Description |
|-------|------|-------------|
| shell_id | string | Unique shell identifier |
| max_iterations | int | Maximum fuzz iterations |
| crash_reproduction | bool | Whether to support crash reproduction |
| coverage_guided | bool | Whether fuzzing is coverage-guided |
| seed_corpus_path | string | Path to seed corpus |
| mutation_strategy | string | Mutation strategy (bitflip, insert, delete, etc.) |

### FakeParserSecurityService

12 builder methods:
- build_strict_parser_security_policy()
- build_relaxed_parser_security_policy()
- build_timeout_parser_security_policy()
- build_reject_unknown_boxes_policy()
- build_standard_quarantine_policy()
- build_aggressive_quarantine_policy()
- build_minimal_quarantine_policy()
- build_escalation_quarantine_policy()
- build_basic_fuzz_shell()
- build_coverage_guided_fuzz_shell()
- build_quick_fuzz_shell()
- build_reproduction_fuzz_shell()

## Tests

12 tests covering:
1. Strict parser security policy construction
2. Relaxed parser security policy construction
3. Timeout parser security policy construction
4. Reject unknown boxes policy construction
5. Standard quarantine policy construction
6. Aggressive quarantine policy construction
7. Minimal quarantine policy construction
8. Escalation quarantine policy construction
9. Basic fuzz shell construction
10. Coverage guided fuzz shell construction
11. Quick fuzz shell construction
12. Reproduction fuzz shell construction
