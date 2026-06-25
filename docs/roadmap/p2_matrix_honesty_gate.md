# P2-PMX-002 Matrix Honesty Gate

## Overview

Matrix Honesty Gate ensures the Provider Runtime Matrix accurately represents provider capabilities without exaggeration or false claims. This gate validates that no provider defaults to RUNTIME_PASS, missing environments are correctly marked as BLOCKED_ENV, and controlled protocol servers are not mislabeled as commercial cloud/account runtime.

## Honesty Rules

### Rule 1: No Default RUNTIME_PASS

**Rule**: No provider may default to RUNTIME_PASS status without explicit verification evidence.

**Validation**:
1. **Evidence Required**: Every RUNTIME_PASS must have corresponding evidence file
2. **Verification Timestamp**: Evidence must have recent verification timestamp
3. **Environment Proof**: Evidence must prove real environment usage
4. **Runtime Proof**: Evidence must prove actual runtime execution
5. **No Assumptions**: Cannot assume runtime pass based on contract pass

**Detection**:
- Scan matrix for RUNTIME_PASS entries
- Verify evidence file exists for each RUNTIME_PASS
- Validate evidence timestamp is recent (within 30 days)
- Validate evidence contains runtime proof
- Flag any RUNTIME_PASS without evidence

### Rule 2: Missing Environment = BLOCKED_ENV

**Rule**: Providers requiring unavailable environment must be marked BLOCKED_ENV, not RUNTIME_PASS or CONTRACT_PASS.

**Validation**:
1. **Environment Check**: Verify environment availability
2. **Status Accuracy**: BLOCKED_ENV if environment unavailable
3. **No Fake Runtime**: Cannot claim runtime pass without environment
4. **Honest Blocking**: BLOCKED_ENV is honest status
5. **No Workaround**: Cannot use test harness as real environment

**Detection**:
- Scan matrix for providers with environment_required=true
- Check environment_available for each provider
- Verify BLOCKED_ENV status if environment unavailable
- Flag any provider claiming runtime pass without environment
- Validate no test harness used as real environment

### Rule 3: Controlled Protocol ≠ Commercial Cloud

**Rule**: Controlled protocol servers (test servers, local servers) cannot be mislabeled as commercial cloud/account runtime.

**Validation**:
1. **Server Type**: Identify server type (controlled vs commercial)
2. **Authentication**: Test servers use test credentials, not real accounts
3. **Network**: Test servers on local network, not internet
4. **Data**: Test servers use test data, not real user data
5. **Terms**: Test servers not subject to commercial terms

**Detection**:
- Scan evidence for server type identification
- Verify test servers marked as controlled
- Verify commercial servers marked as commercial
- Flag any mislabeling of server types
- Validate authentication type matches server type

## Gate Validation Process

### Step 1: Matrix Scan

1. **Load Matrix**: Load provider runtime matrix
2. **Extract Providers**: Extract all provider entries
3. **Identify Statuses**: Identify contract and runtime statuses
4. **Identify Environments**: Identify environment requirements
5. **Identify Evidence**: Identify evidence file references

### Step 2: Rule Validation

1. **Validate Rule 1**: Check no default RUNTIME_PASS
2. **Validate Rule 2**: Check BLOCKED_ENV for missing environments
3. **Validate Rule 3**: Check controlled vs commercial labeling
4. **Validate Evidence**: Check evidence files exist and valid
5. **Validate Timestamps**: Check evidence timestamps recent

### Step 3: Violation Detection

1. **Missing Evidence**: RUNTIME_PASS without evidence
2. **Stale Evidence**: Evidence older than 30 days
3. **Fake Runtime**: Runtime pass without real environment
4. **Environment Mismatch**: Status inconsistent with environment
5. **Server Mislabeling**: Controlled server labeled as commercial

### Step 4: Violation Reporting

1. **Violation Type**: Classify violation type
2. **Provider Impact**: Identify affected providers
3. **Severity Assessment**: Assess violation severity
4. **Remediation Required**: Determine required remediation
5. **Gate Result**: Determine gate pass/fail

## Validation Checks

### Check 1: Evidence Completeness

```cpp
struct EvidenceCompletenessCheck {
    std::string provider_id;
    RuntimeStatus runtime_status;
    std::string evidence_file;
    bool evidence_exists;
    bool evidence_valid;
    bool evidence_recent;
    bool check_passed;
};
```

**Validation**:
1. For each RUNTIME_PASS, evidence file must exist
2. Evidence file must be valid JSON
3. Evidence must validate against schema
4. Evidence timestamp must be within 30 days
5. Evidence must contain runtime proof

### Check 2: Environment Accuracy

```cpp
struct EnvironmentAccuracyCheck {
    std::string provider_id;
    EnvironmentKind environment_kind;
    bool environment_required;
    bool environment_available;
    RuntimeStatus runtime_status;
    bool check_passed;
};
```

**Validation**:
1. If environment_required=true and environment_available=false, status must be BLOCKED_ENV
2. If environment_required=false, status can be RUNTIME_PASS with test harness
3. No provider can claim RUNTIME_PASS without required environment
4. Environment availability must be accurately reported
5. No fake environment claims

### Check 3: Server Type Accuracy

```cpp
struct ServerTypeAccuracyCheck {
    std::string provider_id;
    std::string server_type;  // controlled, commercial, test, production
    std::string authentication_type;  // test_credentials, real_account, oauth
    std::string network_type;  // local, internet, intranet
    bool check_passed;
};
```

**Validation**:
1. Controlled servers must use test credentials
2. Commercial servers must use real accounts
3. Test servers must be on local network
4. Commercial servers must be on internet
5. No mislabeling of server types

## Gate Output

### Gate Result Structure

```cpp
struct MatrixHonestyGateResult {
    bool gate_passed;
    std::string gate_timestamp;
    int total_providers_checked;
    int violations_found;
    std::vector<MatrixHonestyViolation> violations;
    std::string gate_summary;
    std::string remediation_required;
};
```

### Violation Structure

```cpp
struct MatrixHonestyViolation {
    std::string violation_type;  // missing_evidence, stale_evidence, fake_runtime, environment_mismatch, server_mislabeling
    std::string provider_id;
    std::string description;
    std::string evidence_required;
    std::string remediation_action;
    int severity;  // 1=low, 2=medium, 3=high, 4=critical
};
```

### Gate Summary

1. **Providers Checked**: Total providers validated
2. **Violations Found**: Total violations detected
3. **Critical Violations**: Violations requiring immediate action
4. **Gate Decision**: PASS or FAIL
5. **Remediation Required**: Required actions to pass gate

## Integration Points

### PFFG-001 Foundation Final Gate Reader

- Reads Matrix Honesty Gate result
- Validates no critical violations
- Validates gate passed
- Integrates with foundation final gate

### APRV-001 All Providers Runtime Verified Gate

- Requires Matrix Honesty Gate passed
- Requires no server mislabeling
- Requires honest environment reporting
- Requires evidence completeness

### Inspector Integration

- Reports honesty gate status
- Reports violation details
- Reports remediation requirements
- Tracks gate history

### DecisionLedger Integration

- Records gate execution decision
- Records violation findings
- Records remediation actions
- Records gate pass/fail

## Test Cases

### Positive Test Cases

1. Matrix with all providers having evidence for RUNTIME_PASS
2. Matrix with BLOCKED_ENV for missing environments
3. Matrix with controlled servers correctly labeled
4. Matrix with commercial servers correctly labeled
5. Matrix with recent evidence timestamps
6. Matrix with valid evidence schemas
7. Matrix with accurate environment availability
8. Matrix with honest status assignments
9. Matrix with no default RUNTIME_PASS
10. Matrix with proper server type identification

### Negative Test Cases

1. RUNTIME_PASS without evidence file
2. RUNTIME_PASS with stale evidence (>30 days)
3. RUNTIME_PASS without real environment
4. BLOCKED_ENV with environment available
5. Controlled server labeled as commercial
6. Commercial server labeled as controlled
7. Test credentials used for commercial server
8. Real account used for test server
9. Local server claiming internet runtime
10. Internet server claiming local runtime

## Documentation Files

1. Honesty Rules: `docs/p2/matrix_honesty_rules.md`
2. Validation Process: `docs/p2/matrix_honesty_validation.md`
3. Violation Types: `docs/p2/matrix_honesty_violations.md`
4. Integration Guide: `docs/p2/matrix_honesty_integration.md`
5. Test Cases: `docs/p2/matrix_honesty_test_cases.md`

## Evidence Output

- Evidence file: `artifacts/p2/evidence/PMX-002.json`
- Gate result: `artifacts/p2/matrix_honesty_gate_result.json`
- Inspector output: Honesty gate snapshot
- DecisionLedger output: Honesty gate decision
- Schema validation: P2-evidence-v8 schema
- Redline scan: No forbidden terms in gate implementation