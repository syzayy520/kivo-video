# P2 Artifact Redline Scan

Updated: 2026-06-26

This document defines the artifact redline scan for P2, ensuring evidence JSON, Inspector artifacts, DecisionLedger artifacts, Provider Runtime Matrix, and final reports do not contain forbidden terms.

## Purpose

The artifact redline scan prevents forbidden terms from appearing in machine-readable artifacts and final reports unless explicitly allowed by allowlist rules.

## Scan Scope

### 1. Evidence JSON Scan
**Directories**: `artifacts/p2/evidence/`
**File Patterns**: `*.json`
**Forbidden Terms**: All 32 forbidden terms from redline vocabulary.
**Context**: Runtime evidence contexts only.

### 2. Inspector Artifact Scan
**Directories**: `artifacts/p2/inspector/`
**File Patterns**: `*.json`
**Forbidden Terms**: All 32 forbidden terms from redline vocabulary.
**Context**: Runtime evidence contexts only.

### 3. DecisionLedger Artifact Scan
**Directories**: `artifacts/p2/decision_ledger/`
**File Patterns**: `*.json`
**Forbidden Terms**: All 32 forbidden terms from redline vocabulary.
**Context**: Runtime evidence contexts only.

### 4. Provider Runtime Matrix Scan
**Directories**: `artifacts/p2/`
**File Patterns**: `provider_runtime_matrix.json`
**Forbidden Terms**: All 32 forbidden terms from redline vocabulary.
**Context**: Runtime evidence contexts only.

### 5. Final Report Scan
**Directories**: `docs/roadmap/`
**File Patterns**: `*final_report.md`
**Forbidden Terms**: All 32 forbidden terms from redline vocabulary.
**Context**: Allowed in Forbidden Scope, Known Limitations, Next Recommendation sections.

## Scan Rules

### Rule 1: Evidence JSON Strict Scan
- Scan all evidence JSON files.
- For each forbidden term found:
  a. Check if term is in status field (allowed if status is CONTRACT_PASS or BLOCKED_ENV).
  b. Check if term is in blocked_reason field (allowed).
  c. Check if term is in runtime_mode field (allowed).
  d. If not allowed, record violation.
- Severity: HIGH for RUNTIME_PASS contexts.

### Rule 2: Inspector/DecisionLedger Scan
- Scan all Inspector and DecisionLedger JSON files.
- For each forbidden term found:
  a. Check if term is in snapshot data (check context).
  b. Check if term is in decision entry (check context).
  c. If in runtime evidence context, record violation.
- Severity: MEDIUM for snapshot/decision contexts.

### Rule 3: Provider Runtime Matrix Scan
- Scan provider runtime matrix JSON file.
- For each forbidden term found:
  a. Check if term is in provider runtime status (check context).
  b. Check if term is in blocked_reason (allowed).
  c. Check if term is in evidence_file reference (allowed).
  d. If in runtime evidence context, record violation.
- Severity: HIGH for provider runtime status contexts.

### Rule 4: Final Report Section Scan
- Scan final report markdown files.
- For each forbidden term found:
  a. Detect section by heading.
  b. Check if section is allowed (Forbidden Scope, Known Limitations, Next Recommendation).
  c. If not in allowed section, record violation.
- Severity: LOW for documentation contexts.

## Scan Implementation

### ArtifactRedlineScanner Class
```cpp
class ArtifactRedlineScanner {
public:
    // Scanning
    std::vector<RedlineViolation> scan_evidence_json() const;
    std::vector<RedlineViolation> scan_inspector_artifacts() const;
    std::vector<RedlineViolation> scan_decision_ledger_artifacts() const;
    std::vector<RedlineViolation> scan_provider_runtime_matrix() const;
    std::vector<RedlineViolation> scan_final_reports() const;
    std::vector<RedlineViolation> scan_all() const;
    
    // File Scanning
    std::vector<RedlineViolation> scan_json_file(const std::string& file_path) const;
    std::vector<RedlineViolation> scan_markdown_file(const std::string& file_path) const;
    
    // Context Detection
    std::string detect_context_in_json(const std::string& file_path, const std::string& field_path) const;
    bool is_in_allowed_context(const std::string& file_path, const std::string& field_path) const;
    
    // Reporting
    ArtifactRedlineReport generate_report() const;
};
```

### Python Implementation
```python
class ArtifactRedlineScanner:
    def __init__(self, allowlist_checker: AllowlistChecker):
        self.allowlist_checker = allowlist_checker
        self.forbidden_terms = self._load_forbidden_terms()
    
    def scan_evidence_json(self) -> List[RedlineViolation]:
        """Scan evidence JSON files for forbidden terms."""
        pass
    
    def scan_inspector_artifacts(self) -> List[RedlineViolation]:
        """Scan Inspector artifact JSON files."""
        pass
    
    def scan_decision_ledger_artifacts(self) -> List[RedlineViolation]:
        """Scan DecisionLedger artifact JSON files."""
        pass
    
    def scan_provider_runtime_matrix(self) -> List[RedlineViolation]:
        """Scan Provider Runtime Matrix JSON file."""
        pass
    
    def scan_final_reports(self) -> List[RedlineViolation]:
        """Scan final report markdown files."""
        pass
    
    def scan_json_file(self, file_path: str) -> List[RedlineViolation]:
        """Scan single JSON file for forbidden terms."""
        pass
```

## Scan Algorithms

### 1. JSON Field Path Scan
```python
def scan_json_field_path(json_data: Any, field_path: str, forbidden_terms: List[str]) -> List[RedlineViolation]:
    violations = []
    if isinstance(json_data, dict):
        for key, value in json_data.items():
            current_path = f"{field_path}.{key}" if field_path else key
            if isinstance(value, str):
                for term in forbidden_terms:
                    if term in value.lower():
                        violations.append(RedlineViolation(
                            field_path=current_path,
                            term=term,
                            value=value
                        ))
            else:
                violations.extend(scan_json_field_path(value, current_path, forbidden_terms))
    elif isinstance(json_data, list):
        for i, item in enumerate(json_data):
            current_path = f"{field_path}[{i}]"
            violations.extend(scan_json_field_path(item, current_path, forbidden_terms))
    return violations
```

### 2. Context Detection for JSON
```python
def detect_context_for_json_violation(violation: RedlineViolation, file_path: str) -> str:
    # Check if field is status
    if violation.field_path.endswith('.status'):
        return "status_field"
    
    # Check if field is blocked_reason
    if violation.field_path.endswith('.blocked_reason'):
        return "blocked_reason_field"
    
    # Check if field is runtime_mode
    if violation.field_path.endswith('.runtime_mode'):
        return "runtime_mode_field"
    
    # Check if field is evidence_file
    if violation.field_path.endswith('.evidence_file'):
        return "evidence_reference_field"
    
    # Default context
    return "runtime_evidence"
```

### 3. Allowlist Checking for JSON
```python
def is_json_violation_allowed(violation: RedlineViolation, context: str) -> bool:
    # Status field: allowed if status is CONTRACT_PASS or BLOCKED_ENV
    if context == "status_field":
        return violation.term in ["contract_pass", "blocked_env"]
    
    # Blocked reason: always allowed
    if context == "blocked_reason_field":
        return True
    
    # Runtime mode: allowed
    if context == "runtime_mode_field":
        return True
    
    # Evidence reference: allowed
    if context == "evidence_reference_field":
        return True
    
    # Default: check allowlist
    return self.allowlist_checker.is_term_allowed(violation.term, context)
```

## Violation Severity Levels

### HIGH Severity
- Forbidden terms in RUNTIME_PASS evidence status.
- Forbidden terms in provider runtime status RUNTIME_PASS.
- Forbidden terms in Inspector snapshots with RUNTIME_PASS.
- Forbidden terms in DecisionLedger entries with RUNTIME_PASS.

### MEDIUM Severity
- Forbidden terms in CONTRACT_PASS evidence explanations.
- Forbidden terms in Inspector snapshots with CONTRACT_PASS.
- Forbidden terms in DecisionLedger entries with CONTRACT_PASS.
- Forbidden terms in provider runtime status CONTRACT_PASS.

### LOW Severity
- Forbidden terms in blocked_reason fields (allowed).
- Forbidden terms in runtime_mode fields (allowed).
- Forbidden terms in evidence_file references (allowed).
- Forbidden terms in final report allowed sections.

## Scan Output Format

### Violation Output
```json
{
  "file_path": "artifacts/p2/evidence/V10-009.json",
  "field_path": "status",
  "term": "fake",
  "context": "status_field",
  "violation_type": "fake_pass_violation",
  "severity": "HIGH",
  "value": "RUNTIME_PASS"
}
```

### Report Output
```json
{
  "scan_timestamp": "2026-06-26T01:12:00Z",
  "total_files_scanned": 25,
  "total_violations": 0,
  "violations": [],
  "term_counts": {},
  "context_counts": {},
  "compliance_status": true
}
```

## Gate Integration

### RGF-004 Gate Requirements
1. Artifact redline scan implemented.
2. Scan covers evidence JSON, Inspector artifacts, DecisionLedger artifacts, Provider Runtime Matrix, final reports.
3. Allowlist integration working.
4. Violation reporting working.

### PFFG-001 Gate Integration
- Foundation Final Gate Reader runs artifact redline scan.
- Any HIGH severity violation blocks P2 Foundation claim.

### APRV-001 Gate Integration
- All Providers Runtime Verification Gate runs artifact redline scan.
- Any violation in provider runtime matrix blocks All Providers claim.

## Test Cases

### Positive Test Cases
1. Scan evidence JSON with CONTRACT_PASS status → PASS.
2. Scan evidence JSON with BLOCKED_ENV status → PASS.
3. Scan evidence JSON with blocked_reason containing forbidden term → PASS.
4. Scan Inspector snapshot with CONTRACT_PASS → PASS.
5. Scan DecisionLedger entry with CONTRACT_PASS → PASS.
6. Scan final report with term in Forbidden Scope section → PASS.

### Negative Test Cases
1. Scan evidence JSON with RUNTIME_PASS status containing forbidden term → VIOLATION.
2. Scan Inspector snapshot with RUNTIME_PASS containing forbidden term → VIOLATION.
3. Scan DecisionLedger entry with RUNTIME_PASS containing forbidden term → VIOLATION.
4. Scan Provider Runtime Matrix with RUNTIME_PASS status containing forbidden term → VIOLATION.
5. Scan final report with term outside allowed sections → VIOLATION.
6. Scan evidence JSON with fake term in runtime evidence context → VIOLATION.

## Implementation Requirements

### File Discovery
1. Recursively scan specified directories.
2. Filter by file patterns.
3. Skip binary files and build artifacts.
4. Handle JSON parsing errors gracefully.

### Performance Requirements
1. Scan 100+ JSON files in < 5 seconds.
2. Memory usage < 50MB.
3. Parallel scanning for large artifact sets.
4. Incremental scanning for changed files.

## Maintenance Rules

1. New forbidden terms require scan update.
2. New artifact types require scan update.
3. Allowlist changes require scan verification.
4. Scan coverage must be reported.