# P2 Report/Source Conflict Scan

Updated: 2026-06-26

This document defines the report/source conflict scan for P2, ensuring final report claims do not contradict source implementation or evidence status.

## Purpose

The report/source conflict scan prevents premature completion claims, false runtime claims, and inconsistencies between final reports, source code, and evidence files.

## Scan Scope

### 1. Final Report Claims Scan
**Directories**: `docs/roadmap/`
**File Patterns**: `*final_report.md`
**Claims to Scan**: All completion claims, runtime claims, foundation claims.
**Conflict Types**: Contradicts source, contradicts evidence, premature claim, false claim.

### 2. Source Implementation Scan
**Directories**: `backend/src/`, `backend/include/`
**File Patterns**: `*.cpp`, `*.hpp`
**Implementation Facts**: Stub implementations, fake services, real implementations.
**Conflict Types**: Report claims real but source is stub, report claims complete but source is incomplete.

### 3. Evidence Status Scan
**Directories**: `artifacts/p2/evidence/`
**File Patterns**: `*.json`
**Evidence Facts**: Status fields, runtime_verified_timestamp, blocked_reason.
**Conflict Types**: Report claims RUNTIME_PASS but evidence is CONTRACT_PASS, report claims complete but evidence is BLOCKED_ENV.

### 4. Checklist Consistency Scan
**Directories**: `docs/roadmap/`
**File Patterns**: `backend_task_checklist.md`
**Checklist Facts**: DONE/TODO/BLOCKED status, evidence references.
**Conflict Types**: Report claims complete but checklist says TODO, report references wrong evidence.

## Scan Rules

### Rule 1: Completion Claim Validation
- Scan final reports for completion claims.
- For each completion claim found:
  a. Check if claim is "P2 FOUNDATION STATUS" or "Direct Play / Direct Stream / Presentation Foundation Complete".
  b. Check if claim is "APRV-001 final verification".
  c. Validate claim against foundation gate dependencies.
  d. If claim is premature, record violation.
- Severity: HIGH for premature completion claims.

### Rule 2: Runtime Claim Validation
- Scan final reports for runtime claims.
- For each runtime claim found:
  a. Check if claim is "RUNTIME_PASS" for specific components.
  b. Validate against evidence status files.
  c. Check if evidence status is CONTRACT_PASS, BLOCKED_ENV, or FAIL.
  d. If claim contradicts evidence, record violation.
- Severity: HIGH for false runtime claims.

### Rule 3: Source Implementation Validation
- Scan final reports for implementation claims.
- For each implementation claim found:
  a. Check if claim describes real implementation.
  b. Validate against source code (stub vs real).
  c. Check if source has actual implementation or just interfaces.
  d. If claim contradicts source, record violation.
- Severity: MEDIUM for implementation contradictions.

### Rule 4: Checklist Consistency Validation
- Scan final reports for task completion claims.
- For each task completion claim found:
  a. Check if task is marked DONE in checklist.
  b. Check if evidence reference matches.
  c. Check if task status is consistent.
  d. If claim contradicts checklist, record violation.
- Severity: LOW for checklist inconsistencies.

## Scan Implementation

### ReportSourceConflictScanner Class
```cpp
class ReportSourceConflictScanner {
public:
    // Scanning
    std::vector<ConflictViolation> scan_final_report_claims() const;
    std::vector<ConflictViolation> scan_source_implementation() const;
    std::vector<ConflictViolation> scan_evidence_status() const;
    std::vector<ConflictViolation> scan_checklist_consistency() const;
    std::vector<ConflictViolation> scan_all() const;
    
    // Claim Extraction
    std::vector<CompletionClaim> extract_completion_claims(const std::string& report_path) const;
    std::vector<RuntimeClaim> extract_runtime_claims(const std::string& report_path) const;
    std::vector<ImplementationClaim> extract_implementation_claims(const std::string& report_path) const;
    
    // Validation
    bool validate_completion_claim(const CompletionClaim& claim) const;
    bool validate_runtime_claim(const RuntimeClaim& claim) const;
    bool validate_implementation_claim(const ImplementationClaim& claim) const;
    
    // Reporting
    ReportSourceConflictReport generate_report() const;
};
```

### Python Implementation
```python
class ReportSourceConflictScanner:
    def __init__(self, allowlist_checker: AllowlistChecker):
        self.allowlist_checker = allowlist_checker
        self.completion_claim_patterns = self._load_completion_claim_patterns()
        self.runtime_claim_patterns = self._load_runtime_claim_patterns()
    
    def scan_final_report_claims(self) -> List[ConflictViolation]:
        """Scan final reports for completion and runtime claims."""
        pass
    
    def scan_source_implementation(self) -> List[ConflictViolation]:
        """Scan source code for implementation facts."""
        pass
    
    def scan_evidence_status(self) -> List[ConflictViolation]:
        """Scan evidence files for status facts."""
        pass
    
    def scan_checklist_consistency(self) -> List[ConflictViolation]:
        """Scan checklist for task status facts."""
        pass
    
    def validate_claim_against_facts(self, claim: Claim, facts: Facts) -> ConflictViolation:
        """Validate a claim against known facts."""
        pass
```

## Scan Algorithms

### 1. Completion Claim Extraction
```python
def extract_completion_claims(report_content: str) -> List[CompletionClaim]:
    claims = []
    patterns = [
        r"P2 FOUNDATION STATUS",
        r"Direct Play.*Direct Stream.*Presentation Foundation Complete",
        r"APRV-001 final verification",
        r"P2.*COMPLETE",
        r"FOUNDATION.*COMPLETE"
    ]
    
    for pattern in patterns:
        matches = re.finditer(pattern, report_content, re.IGNORECASE)
        for match in matches:
            claims.append(CompletionClaim(
                text=match.group(),
                position=match.start(),
                context=detect_context(report_content, match.start())
            ))
    
    return claims
```

### 2. Runtime Claim Extraction
```python
def extract_runtime_claims(report_content: str) -> List[RuntimeClaim]:
    claims = []
    patterns = [
        r"RUNTIME_PASS",
        r"runtime.*verified",
        r"real.*implementation",
        r"actual.*runtime"
    ]
    
    for pattern in patterns:
        matches = re.finditer(pattern, report_content, re.IGNORECASE)
        for match in matches:
            claims.append(RuntimeClaim(
                text=match.group(),
                position=match.start(),
                context=detect_context(report_content, match.start())
            ))
    
    return claims
```

### 3. Claim Validation Against Evidence
```python
def validate_runtime_claim_against_evidence(claim: RuntimeClaim, evidence_files: List[str]) -> bool:
    """Validate runtime claim against evidence status files."""
    for evidence_file in evidence_files:
        with open(evidence_file, 'r') as f:
            evidence = json.load(f)
            
            # Check if evidence status contradicts claim
            if evidence.get('status') == 'CONTRACT_PASS' and 'RUNTIME_PASS' in claim.text:
                return False
            
            if evidence.get('status') == 'BLOCKED_ENV' and 'RUNTIME_PASS' in claim.text:
                return False
            
            if evidence.get('status') == 'FAIL' and 'COMPLETE' in claim.text:
                return False
    
    return True
```

### 4. Claim Validation Against Source
```python
def validate_implementation_claim_against_source(claim: ImplementationClaim, source_files: List[str]) -> bool:
    """Validate implementation claim against source code."""
    for source_file in source_files:
        with open(source_file, 'r') as f:
            source_content = f.read()
            
            # Check if source is stub implementation
            if 'stub' in source_content.lower() or 'fake' in source_content.lower():
                if 'real' in claim.text.lower() or 'actual' in claim.text.lower():
                    return False
            
            # Check if source has actual implementation
            if 'class.*Impl' in source_content or 'function.*implementation' in source_content:
                if 'interface only' in claim.text.lower():
                    return False
    
    return True
```

## Violation Severity Levels

### HIGH Severity
- Premature "P2 FOUNDATION STATUS" claim.
- Premature "APRV-001 final verification" claim.
- False "RUNTIME_PASS" claim when evidence is CONTRACT_PASS.
- False "RUNTIME_PASS" claim when evidence is BLOCKED_ENV.
- False "COMPLETE" claim when evidence is FAIL.

### MEDIUM Severity
- Implementation claim contradicts source stub.
- Runtime claim contradicts source implementation.
- Foundation claim contradicts foundation gate dependencies.
- Provider claim contradicts provider matrix status.

### LOW Severity
- Checklist inconsistency (report says DONE but checklist says TODO).
- Evidence reference mismatch (report references wrong evidence file).
- Task status inconsistency (report claims complete but task is BLOCKED).
- Minor claim wording inconsistency.

## Scan Output Format

### Violation Output
```json
{
  "report_file": "docs/roadmap/v10_final_report.md",
  "claim_text": "P2 FOUNDATION STATUS",
  "claim_type": "completion_claim",
  "conflict_type": "premature_claim",
  "conflicting_evidence": "artifacts/p2/evidence/PFFG-001.json",
  "evidence_status": "NOT_IMPLEMENTED",
  "severity": "HIGH",
  "context": "final_report_section"
}
```

### Report Output
```json
{
  "scan_timestamp": "2026-06-26T01:15:00Z",
  "total_reports_scanned": 5,
  "total_claims_found": 12,
  "total_violations": 0,
  "violations": [],
  "claim_counts": {
    "completion_claims": 3,
    "runtime_claims": 5,
    "implementation_claims": 4
  },
  "conflict_counts": {},
  "compliance_status": true
}
```

## Gate Integration

### RGF-005 Gate Requirements
1. Report/source conflict scan implemented.
2. Scan covers final reports, source implementation, evidence status, checklist consistency.
3. Claim extraction working.
4. Validation against facts working.
5. Violation reporting working.

### PFFG-001 Gate Integration
- Foundation Final Gate Reader runs report/source conflict scan.
- Any HIGH severity violation blocks P2 Foundation claim.

### PFFG-002 Gate Integration
- Foundation Final Report must pass conflict scan before generation.
- Report generation includes conflict scan verification.

### APRV-001 Gate Integration
- All Providers Runtime Verification Gate runs report/source conflict scan.
- Any provider claim contradiction blocks All Providers claim.

## Test Cases

### Positive Test Cases
1. Scan final report with no completion claims → PASS.
2. Scan final report with CONTRACT_PASS claims matching evidence → PASS.
3. Scan final report with implementation claims matching source → PASS.
4. Scan final report with checklist consistency → PASS.
5. Scan final report with allowed sections containing forbidden terms → PASS.
6. Scan final report with evidence references matching evidence files → PASS.

### Negative Test Cases
1. Scan final report with premature "P2 FOUNDATION STATUS" → VIOLATION.
2. Scan final report with "RUNTIME_PASS" when evidence is CONTRACT_PASS → VIOLATION.
3. Scan final report with "real implementation" when source is stub → VIOLATION.
4. Scan final report with task completion when checklist says TODO → VIOLATION.
5. Scan final report with evidence reference to non-existent file → VIOLATION.
6. Scan final report with "APRV-001 final verification" prematurely → VIOLATION.

## Implementation Requirements

### Claim Extraction
1. Extract completion claims using regex patterns.
2. Extract runtime claims using regex patterns.
3. Extract implementation claims using regex patterns.
4. Handle claim context detection.

### Fact Collection
1. Collect evidence status facts from evidence files.
2. Collect source implementation facts from source code.
3. Collect checklist status facts from checklist file.
4. Handle missing or malformed facts gracefully.

### Validation
1. Validate claims against collected facts.
2. Detect contradictions between claims and facts.
3. Classify violation severity.
4. Generate detailed violation reports.

## Maintenance Rules

1. New claim patterns require scan update.
2. New fact sources require scan update.
3. Claim validation rules require scan verification.
4. Scan coverage must be reported.