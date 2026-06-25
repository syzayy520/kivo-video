# P2-041 Claim Vocabulary Freeze Gate

## Purpose

Governance gate that validates product claims about HDR, Dolby Vision, and audio capabilities against a frozen vocabulary. Prevents false marketing claims by enforcing allowed/forbidden claim patterns.

## Scope

- HDR claim validation (HDR10, HLG, Dolby Vision)
- Audio claim validation (Dolby Atmos, DTS:X, PCM)
- Forbidden pattern detection (false Dolby claims, unsupported formats)
- Enforcement levels (strict, warn, audit)

## Family Tree (governance_core/)

```
governance_core/
├── claim_vocabulary_gate.hpp                   — Gate policy definition (existing from P2-001C)
├── claim_vocabulary_gate_request.hpp           — Validation request
├── claim_vocabulary_gate_result.hpp            — Validation result
├── fake_claim_vocabulary_gate_service.hpp      — Fake service for testing
```

## Contract Types

### ClaimVocabularyGate (existing)

| Field | Type | Description |
|-------|------|-------------|
| allowed_hdr_claims | vector<string> | Allowed HDR claim patterns |
| allowed_audio_claims | vector<string> | Allowed audio claim patterns |
| forbidden_claim_patterns | vector<string> | Forbidden claim patterns |
| enforcement_level | string | "strict", "warn", or "audit" |

### ClaimVocabularyGateRequest

| Field | Type | Description |
|-------|------|-------------|
| request_id | string | Unique request identifier |
| gate_id | string | Gate to evaluate against |
| claim_text | string | Product claim to validate |
| claim_category | string | "hdr", "audio", "codec", "format" |

### ClaimVocabularyGateResult

| Field | Type | Description |
|-------|------|-------------|
| result_id | string | Unique result identifier |
| request_id | string | Reference to request |
| is_allowed | bool | Whether claim passes gate |
| matched_pattern | string | Pattern that matched (if any) |
| enforcement_action | string | "allow", "block", "warn" |
| reason | string | Explanation of decision |

## Tests

10 tests covering:
1. Valid HDR10 claim passes strict gate
2. Valid HLG claim passes strict gate
3. False Dolby Vision claim blocked
4. Forbidden pattern detection
5. Audio claim validation
6. Warn-level enforcement (log but allow)
7. Audit-level enforcement (log only)
8. Empty claim handling
9. Multi-category claim validation
10. Gate policy construction
