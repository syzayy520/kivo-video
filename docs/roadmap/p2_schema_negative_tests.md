# P2 Schema Negative Tests

Updated: 2026-06-26

This document defines the negative test cases for P2 V8 schemas to ensure validation catches invalid data.

## Purpose

Negative tests verify that schema validation correctly rejects invalid data, preventing fake or incorrect evidence from being accepted.

## Negative Test Categories

### 1. Evidence Schema Negative Tests

#### Missing Required Fields
1. **Missing `schema_version`**: Evidence file without schema_version field.
2. **Missing `task_id`**: Evidence file without task_id field.
3. **Missing `status`**: Evidence file without status field.
4. **Missing `evidence_file`**: Evidence file without evidence_file field.
5. **Missing `provider_matrix_impact`**: Evidence file without provider_matrix_impact field.

#### Invalid Status Vocabulary
6. **Custom status**: Status value "PASS_READY_FOR_REVIEW" (not in vocabulary).
7. **Lowercase status**: Status value "runtime_pass" (wrong case).
8. **Abbreviated status**: Status value "RP" (not full vocabulary).
9. **Mixed status**: Status value "Runtime_Pass" (mixed case).
10. **Deprecated status**: Status value "NEEDS_REPAIR" (not in V8 vocabulary).

#### Invalid Runtime Mode
11. **Custom runtime_mode**: runtime_mode value "validation" (not in vocabulary).
12. **Missing runtime_mode**: Evidence file without runtime_mode field.

#### Invalid Environment Kind
13. **Custom environment_kind**: runtime_environment_kind value "real_hardware" (not in vocabulary).
14. **Missing environment_kind**: Evidence file without runtime_environment_kind field.

#### Anti-Fake Violations
15. **Simulated + RUNTIME_PASS**: runtime_environment_kind="simulated" with status="RUNTIME_PASS".
16. **Test harness + RUNTIME_PASS for provider**: runtime_environment_kind="test_harness" with status="RUNTIME_PASS" for provider requiring real environment.
17. **RUNTIME_PASS without evidence_file**: status="RUNTIME_PASS" but evidence_file is null or missing.
18. **RUNTIME_PASS without timestamp**: status="RUNTIME_PASS" but runtime_verified_timestamp is null.

#### Redaction Violations
19. **Full path leakage**: evidence contains "C:\\Users\\Administrator\\Downloads\\sample.mp4".
20. **Token leakage**: evidence contains "api_key=abc123".
21. **Cookie leakage**: evidence contains "cookie=session123".
22. **Unredacted sample path**: evidence contains full sample path without redaction.

### 2. Inspector Schema Negative Tests

#### Missing Info Objects
23. **Missing source_info**: Inspector snapshot without source_info object.
24. **Missing probe_info**: Inspector snapshot without probe_info object.
25. **Missing decode_info**: Inspector snapshot without decode_info object.
26. **Missing presenter_info**: Inspector snapshot without presenter_info object.
27. **Missing audio_info**: Inspector snapshot without audio_info object.

#### Invalid Info Object Fields
28. **Missing required field in source_info**: source_info without "source_id" field.
29. **Missing required field in probe_info**: probe_info without "container_format" field.
30. **Invalid error_entry**: error entry without "error_domain" field.

#### Privacy Violations
31. **Full path in source_info**: source_info contains full file path.
32. **Credential in provider_info**: provider_info contains API key or token.

### 3. Decision Ledger Schema Negative Tests

#### Non-Monotonic Generations
33. **Decreasing generation**: Entry generation 5 followed by generation 3.
34. **Duplicate generation**: Two entries with same generation number.
35. **Missing generation**: Entry without generation field.

#### Invalid Decision Types
36. **Custom decision_type**: decision_type value "custom_decision" (not in vocabulary).
37. **MediaSourceKind in extension**: Extension decision using MediaSourceKind decision type.

#### Invalid Stages
38. **Custom stage**: stage value "custom_stage" (not in vocabulary).
39. **Wrong stage for decision**: Cast decision with stage "source" instead of "cast".

### 4. Extension Decision Ledger Negative Tests

#### Boundary Violations
40. **Extension using MediaSourceKind type**: Extension decision with decision_type="source_selection".
41. **Wrong stage for extension type**: PiP decision with stage="cast" instead of "pip".
42. **Cast decision without cast stage**: Cast decision with stage="pip".

#### Anti-Fake Violations
43. **Cast RUNTIME_PASS without device**: Cast decision with status="RUNTIME_PASS" but no real cast device.
44. **PiP RUNTIME_PASS without surface**: PiP decision with status="RUNTIME_PASS" but no real surface detach/attach.
45. **Trakt RUNTIME_PASS without API**: Trakt decision with status="RUNTIME_PASS" but no real Trakt API.

### 5. Provider Runtime Matrix Negative Tests

#### Missing Evidence
46. **RUNTIME_PASS without evidence_file**: Provider with status="RUNTIME_PASS" but evidence_file is null.
47. **RUNTIME_PASS without timestamp**: Provider with status="RUNTIME_PASS" but runtime_verified_timestamp is null.

#### Invalid Environment
48. **BLOCKED_ENV without reason**: Provider with status="BLOCKED_ENV" but blocked_reason is null.
49. **RUNTIME_PASS with environment_available=false**: Provider with status="RUNTIME_PASS" but environment_available is false.
50. **RUNTIME_PASS with environment_kind=simulated**: Provider with status="RUNTIME_PASS" but environment_kind is "simulated".

#### Count Mismatches
51. **Total count mismatch**: total_providers=10 but providers array has 8 entries.
52. **Verified count mismatch**: verified_providers=5 but only 3 entries have RUNTIME_PASS.
53. **Blocked count mismatch**: blocked_providers=2 but only 1 entry has BLOCKED_ENV.

#### Vocabulary Violations
54. **Invalid provider_kind**: provider_kind value "custom_provider" (not in vocabulary).
55. **Invalid contract_status**: contract_status value "PARTIAL" (not in vocabulary).

## Test Implementation

### Test File Location
```
backend/tests/governance/p2_schema_negative_tests.cpp
```

### Test Structure
```cpp
#include <gtest/gtest.h>
#include "schema_validator.h"

class SchemaNegativeTest : public ::testing::Test {
protected:
    SchemaValidator validator;
};

// Evidence Schema Negative Tests
TEST_F(SchemaNegativeTest, MissingSchemaVersion) {
    nlohmann::json evidence = {
        {"task_id", "TEST-001"},
        {"status", "CONTRACT_PASS"}
        // Missing schema_version
    };
    EXPECT_FALSE(validator.validate_evidence(evidence));
}

TEST_F(SchemaNegativeTest, InvalidStatusVocabulary) {
    nlohmann::json evidence = {
        {"schema_version", "p2-evidence-v8"},
        {"task_id", "TEST-001"},
        {"status", "PASS_READY_FOR_REVIEW"}  // Not in vocabulary
    };
    EXPECT_FALSE(validator.validate_evidence(evidence));
}

TEST_F(SchemaNegativeTest, SimulatedWithRuntimePass) {
    nlohmann::json evidence = {
        {"schema_version", "p2-evidence-v8"},
        {"task_id", "TEST-001"},
        {"status", "RUNTIME_PASS"},
        {"runtime_environment_kind", "simulated"}  // Anti-fake violation
    };
    EXPECT_FALSE(validator.validate_evidence(evidence));
}

// Inspector Schema Negative Tests
TEST_F(SchemaNegativeTest, MissingSourceInfo) {
    nlohmann::json inspector = {
        {"schema_version", "p2-inspector-v8"},
        {"task_id", "TEST-001"}
        // Missing source_info
    };
    EXPECT_FALSE(validator.validate_inspector(inspector));
}

// Decision Ledger Negative Tests
TEST_F(SchemaNegativeTest, NonMonotonicGeneration) {
    nlohmann::json ledger = {
        {"schema_version", "p2-decision-ledger-v8"},
        {"entries", nlohmann::json::array({
            {"generation", 5},
            {"generation", 3}  // Non-monotonic
        })}
    };
    EXPECT_FALSE(validator.validate_decision_ledger(ledger));
}

// Provider Matrix Negative Tests
TEST_F(SchemaNegativeTest, RuntimePassWithoutEvidence) {
    nlohmann::json matrix = {
        {"schema_version", "provider-runtime-matrix-v8"},
        {"providers", nlohmann::json::array({
            {
                {"provider_kind", "local_file"},
                {"runtime_status", "RUNTIME_PASS"},
                {"evidence_file", nullptr}  // Missing evidence
            }
        })}
    };
    EXPECT_FALSE(validator.validate_provider_matrix(matrix));
}
```

### Test Execution
```bash
# Run all negative tests
ctest --preset ninja-debug -R p2_schema_negative_tests --output-on-failure

# Run specific negative test
ctest --preset ninja-debug -R SchemaNegativeTest.MissingSchemaVersion --output-on-failure
```

## Validation Command Integration

The negative tests are integrated into the schema validation command:

```bash
# Run validation with negative test check
python backend/tools/governance/validate_p2_schemas.py --negative-tests

# Run only negative tests
python backend/tools/governance/validate_p2_schemas.py --negative-tests-only
```

## Gate Integration

### SGF-007 Gate Requirements
1. All negative test categories must have test cases.
2. Each schema must have at least 5 negative tests.
3. Negative tests must cover all validation rules.
4. Negative tests must be executable and passing.

### PFFG-001 Gate Integration
- Foundation Final Gate Reader runs negative tests before accepting evidence.
- Any negative test failure blocks P2 Foundation claim.

### APRV-001 Gate Integration
- All Providers Runtime Verification Gate runs negative tests on provider evidence.
- Any negative test failure blocks provider RUNTIME_PASS claim.

## Test Coverage Requirements

### Minimum Test Counts
1. Evidence schema: 22 negative tests.
2. Inspector schema: 9 negative tests.
3. Decision ledger schema: 7 negative tests.
4. Extension decision ledger: 6 negative tests.
5. Provider runtime matrix: 10 negative tests.
6. **Total: 54 negative tests minimum.**

### Coverage Areas
1. Missing required fields.
2. Invalid vocabulary values.
3. Anti-fake rule violations.
4. Redaction violations.
5. Boundary violations.
6. Count mismatches.
7. Monotonicity violations.
8. Privacy violations.

## Documentation Requirements

Each negative test must document:
1. Test name and description.
2. Invalid input data.
3. Expected validation failure.
4. Error message expected.
5. Integration point with gates.

## Maintenance Rules

1. New validation rules require new negative tests.
2. Schema changes require negative test updates.
3. Negative tests must be reviewed with schema changes.
4. Test coverage must be reported in validation reports.