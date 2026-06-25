# P2 Redline Vocabulary

Updated: 2026-06-26

This document defines the redline vocabulary for P2 evidence, ensuring forbidden terms are identified and controlled.

## Purpose

The redline vocabulary defines terms that are forbidden in runtime evidence, source code, and reports unless explicitly allowed in specific contexts.

## Forbidden Terms

### 1. Fake/Stub Terms
1. `fake` - Unless in test file names or fake service implementations.
2. `stub` - Unless in stub implementation documentation.
3. `simulate` - Unless in simulation environment documentation.
4. `simulator` - Unless in simulator documentation.
5. `dummy` - Never allowed in runtime evidence.
6. `mock` - Unless in mock service documentation.
7. `mock-only` - Never allowed in runtime evidence.
8. `placeholder` - Unless in placeholder documentation.

### 2. False Pass Terms
9. `todo-pass` - Never allowed.
10. `always-pass` - Never allowed.
11. `state-only-pass` - Never allowed.
12. `silent fallback` - Never allowed.
13. `not implemented but pass` - Never allowed.
14. `P3 false-complete` - Never allowed.

### 3. Premature Completion Terms
15. `P2 Complete` - Only allowed after PFFG-001 and PFFG-002 pass.
16. `ALL PROVIDERS RUNTIME VERIFIED` - Only allowed after APRV-001 passes.
17. `P3 COMPLETE` - Never allowed in P2 evidence.
18. `HDR COMPLETE` - Never allowed in P2 evidence.
19. `ATMOS BITSTREAM COMPLETE` - Never allowed in P2 evidence.
20. `AIRPLAY PRODUCT COMPLETE` - Never allowed in P2 evidence.

### 4. Scope Violation Terms
21. `hardware decode` - Only allowed in Known Limitations or Forbidden Scope.
22. `exclusive audio` - Only allowed in Known Limitations or Forbidden Scope.
23. `passthrough` - Only allowed in Known Limitations or Forbidden Scope.
24. `bitstream` - Only allowed in Known Limitations or Forbidden Scope.
25. `remote runtime` - Only allowed in Known Limitations or Forbidden Scope.
26. `cloud account` - Only allowed in Known Limitations or Forbidden Scope.

### 5. Privacy Violation Terms
27. `C:\Users\` - Never allowed in runtime evidence.
28. `/Users/` - Never allowed in runtime evidence.
29. `/home/` - Never allowed in runtime evidence.
30. `api_key=` - Never allowed in runtime evidence.
31. `token=` - Never allowed in runtime evidence.
32. `cookie=` - Never allowed in runtime evidence.

## Allowlist Rules

### Allowed Contexts
1. **Forbidden Scope documentation**: Terms allowed when describing what is NOT implemented.
2. **Known Limitations documentation**: Terms allowed when describing limitations.
3. **Next Recommendation documentation**: Terms allowed when recommending future work.
4. **CONTRACT_PASS explanation**: Terms allowed when explaining why status is CONTRACT_PASS.
5. **Test file names**: Terms allowed in test file names (e.g., `fake_service_test.cpp`).
6. **Fake service implementations**: Terms allowed in fake service implementation files.

### Allowlist Format
```
ALLOWLIST:
- term: "fake"
  contexts: ["test_file_names", "fake_service_implementations", "CONTRACT_PASS_explanation"]
  example: "FakeDeviceBufferWriter is a test double, not real runtime."
  
- term: "stub"
  contexts: ["stub_implementation_documentation", "CONTRACT_PASS_explanation"]
  example: "This is a stub implementation requiring real D3D11 environment."
```

## Redline Scan Rules

### Source Code Scan
1. Scan production code for forbidden terms.
2. Scan test code for forbidden terms.
3. Scan documentation for forbidden terms.
4. Allow terms only in allowed contexts.

### Evidence Scan
1. Scan evidence JSON files for forbidden terms.
2. Scan Inspector snapshots for forbidden terms.
3. Scan DecisionLedger entries for forbidden terms.
4. Scan Provider Runtime Matrix for forbidden terms.

### Report Scan
1. Scan final reports for forbidden terms.
2. Scan planning documents for forbidden terms.
3. Scan audit documents for forbidden terms.
4. Allow terms only in allowed sections.

## Redline Violation Types

### 1. Fake PASS Violation
- Evidence claims RUNTIME_PASS but contains fake/stub/simulator terms.
- Example: "RUNTIME_PASS with FakeDeviceBufferWriter" → VIOLATION.

### 2. Silent Fallback Violation
- Evidence contains "silent fallback" term.
- Example: "Device fallback to silent mode" → VIOLATION.

### 3. Premature Completion Violation
- Evidence claims P2 Complete before PFFG-001/PFFG-002.
- Example: "P2 Complete after local playback test" → VIOLATION.

### 4. Scope Violation
- Evidence contains scope violation terms in runtime context.
- Example: "Hardware decode supported" in RUNTIME_PASS evidence → VIOLATION.

### 5. Privacy Violation
- Evidence contains privacy violation terms.
- Example: "Full path C:\Users\Administrator\Downloads\sample.mp4" → VIOLATION.

## Redline Gate Integration

### RGF-001 Gate Requirements
1. Forbidden terms vocabulary defined.
2. Allowlist rules defined.
3. Redline scan rules defined.
4. Violation types defined.

### RGF-002 Gate Requirements
1. Allowlist rules implemented.
2. Context detection implemented.
3. Exception handling implemented.

### RGF-003 Gate Requirements
1. Source redline scan implemented.
2. Test redline scan implemented.
3. Documentation redline scan implemented.

### RGF-004 Gate Requirements
1. Evidence JSON redline scan implemented.
2. Inspector snapshot redline scan implemented.
3. DecisionLedger redline scan implemented.
4. Provider Runtime Matrix redline scan implemented.

### RGF-005 Gate Requirements
1. Report/source conflict scan implemented.
2. Evidence/source conflict scan implemented.
3. Conflict resolution implemented.

## Implementation Requirements

### RedlineScanner Class
```cpp
class RedlineScanner {
public:
    // Scanning
    std::vector<RedlineViolation> scan_source(const std::string& file_path) const;
    std::vector<RedlineViolation> scan_evidence(const std::string& evidence_path) const;
    std::vector<RedlineViolation> scan_report(const std::string& report_path) const;
    
    // Validation
    bool is_term_allowed(const std::string& term, const std::string& context) const;
    std::string detect_context(const std::string& file_path, const std::string& line) const;
    
    // Reporting
    RedlineReport generate_report() const;
};

struct RedlineViolation {
    std::string file_path;
    int line_number;
    std::string term;
    std::string context;
    std::string violation_type;
    std::string severity;
};
```

### Python Implementation
```python
class RedlineScanner:
    def __init__(self):
        self.forbidden_terms = self._load_forbidden_terms()
        self.allowlist = self._load_allowlist()
    
    def scan_source(self, file_path: str) -> List[RedlineViolation]:
        """Scan source file for forbidden terms."""
        pass
    
    def scan_evidence(self, evidence_path: str) -> List[RedlineViolation]:
        """Scan evidence file for forbidden terms."""
        pass
    
    def is_term_allowed(self, term: str, context: str) -> bool:
        """Check if term is allowed in context."""
        pass
```

## Test Cases

### Positive Test Cases
1. Scan source without forbidden terms → PASS.
2. Scan evidence with allowed term in allowed context → PASS.
3. Scan report with terms in allowed sections → PASS.
4. Detect context correctly for fake service file.
5. Detect context correctly for test file.
6. Generate redline report correctly.

### Negative Test Cases
1. Scan source with forbidden term → VIOLATION.
2. Scan evidence with fake PASS term → VIOLATION.
3. Scan report with premature completion → VIOLATION.
4. Scan evidence with privacy violation → VIOLATION.
5. Scan evidence with scope violation → VIOLATION.
6. Scan evidence with silent fallback → VIOLATION.

## Documentation Requirements

Each forbidden term must document:
1. Term and description.
2. Allowed contexts.
3. Example of violation.
4. Example of allowed usage.
5. Severity level.

## Maintenance Rules

1. New forbidden terms require vocabulary update.
2. New allowed contexts require allowlist update.
3. Redline scans must be updated with new terms.
4. Test coverage must be maintained.