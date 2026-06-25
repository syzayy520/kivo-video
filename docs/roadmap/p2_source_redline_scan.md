# P2 Source Redline Scan

Updated: 2026-06-26

This document defines the source redline scan for P2, ensuring production code and tests do not contain forbidden terms in runtime contexts.

## Purpose

The source redline scan prevents forbidden terms from appearing in production code, test code, and documentation unless explicitly allowed by allowlist rules.

## Scan Scope

### 1. Production Code Scan
**Directories**: `backend/src/`, `backend/include/`
**File Patterns**: `*.cpp`, `*.hpp`, `*.h`
**Forbidden Terms**: All 32 forbidden terms from redline vocabulary.
**Context**: Runtime evidence contexts only.

### 2. Test Code Scan
**Directories**: `backend/tests/`
**File Patterns**: `*.cpp`, `*.hpp`
**Forbidden Terms**: All 32 forbidden terms from redline vocabulary.
**Context**: Runtime evidence contexts only (test file names are allowed).

### 3. Documentation Scan
**Directories**: `docs/`
**File Patterns**: `*.md`
**Forbidden Terms**: All 32 forbidden terms from redline vocabulary.
**Context**: Allowed in Forbidden Scope, Known Limitations, Next Recommendation sections.

## Scan Rules

### Rule 1: Production Code Strict Scan
- Scan all production code files.
- For each forbidden term found:
  a. Check if file is in allowlist (fake service implementations).
  b. Check if line is in allowlist context.
  c. If not allowed, record violation.
- Severity: HIGH for runtime evidence contexts.

### Rule 2: Test Code Contextual Scan
- Scan all test code files.
- For each forbidden term found:
  a. Check if term is in test file name (allowed).
  b. Check if term is in test implementation (check context).
  c. If in test implementation, check if context is allowed.
- Severity: MEDIUM for test implementation contexts.

### Rule 3: Documentation Section Scan
- Scan all documentation files.
- For each forbidden term found:
  a. Detect section by heading.
  b. Check if section is allowed (Forbidden Scope, Known Limitations, Next Recommendation).
  c. If not in allowed section, record violation.
- Severity: LOW for documentation contexts.

## Scan Implementation

### SourceRedlineScanner Class
```cpp
class SourceRedlineScanner {
public:
    // Scanning
    std::vector<RedlineViolation> scan_production_code() const;
    std::vector<RedlineViolation> scan_test_code() const;
    std::vector<RedlineViolation> scan_documentation() const;
    std::vector<RedlineViolation> scan_all() const;
    
    // File Scanning
    std::vector<RedlineViolation> scan_file(const std::string& file_path) const;
    std::vector<RedlineViolation> scan_directory(const std::string& dir_path) const;
    
    // Context Detection
    std::string detect_context(const std::string& file_path, int line_number) const;
    bool is_in_allowed_context(const std::string& file_path, int line_number) const;
    
    // Reporting
    SourceRedlineReport generate_report() const;
};

struct RedlineViolation {
    std::string file_path;
    int line_number;
    std::string term;
    std::string context;
    std::string violation_type;
    std::string severity;
    std::string line_content;
};

struct SourceRedlineReport {
    std::string scan_timestamp;
    int total_files_scanned;
    int total_violations;
    std::vector<RedlineViolation> violations;
    std::map<std::string, int> term_counts;
    std::map<std::string, int> context_counts;
    bool compliance_status;
};
```

### Python Implementation
```python
class SourceRedlineScanner:
    def __init__(self, allowlist_checker: AllowlistChecker):
        self.allowlist_checker = allowlist_checker
        self.forbidden_terms = self._load_forbidden_terms()
    
    def scan_production_code(self) -> List[RedlineViolation]:
        """Scan production code for forbidden terms."""
        pass
    
    def scan_test_code(self) -> List[RedlineViolation]:
        """Scan test code for forbidden terms."""
        pass
    
    def scan_documentation(self) -> List[RedlineViolation]:
        """Scan documentation for forbidden terms."""
        pass
    
    def scan_file(self, file_path: str) -> List[RedlineViolation]:
        """Scan single file for forbidden terms."""
        pass
```

## Scan Algorithms

### 1. Line-by-Line Scan
```python
def scan_file_line_by_line(file_path: str, forbidden_terms: List[str]) -> List[RedlineViolation]:
    violations = []
    with open(file_path, 'r', encoding='utf-8') as f:
        for line_number, line in enumerate(f, 1):
            for term in forbidden_terms:
                if term in line.lower():
                    violations.append(RedlineViolation(
                        file_path=file_path,
                        line_number=line_number,
                        term=term,
                        line_content=line.strip()
                    ))
    return violations
```

### 2. Context Detection
```python
def detect_context_for_violation(violation: RedlineViolation) -> str:
    # Check file pattern
    context = allowlist_checker.detect_context_by_file_pattern(violation.file_path)
    if context != "unknown":
        return context
    
    # Check section for documentation
    if violation.file_path.endswith('.md'):
        context = allowlist_checker.detect_section_by_heading(
            violation.file_path, violation.line_number
        )
        return context
    
    # Default context
    return "runtime_evidence"
```

### 3. Allowlist Checking
```python
def is_violation_allowed(violation: RedlineViolation, context: str) -> bool:
    return allowlist_checker.is_term_allowed(violation.term, context)
```

## Violation Severity Levels

### HIGH Severity
- Forbidden terms in production code runtime contexts.
- Forbidden terms in RUNTIME_PASS evidence.
- Forbidden terms in runtime documentation.

### MEDIUM Severity
- Forbidden terms in test code runtime contexts.
- Forbidden terms in CONTRACT_PASS evidence explanations.
- Forbidden terms in planning documentation.

### LOW Severity
- Forbidden terms in test file names (allowed).
- Forbidden terms in fake service implementations (allowed).
- Forbidden terms in allowed documentation sections.

## Scan Output Format

### Violation Output
```json
{
  "file_path": "backend/src/playback_core/playback_controller.cpp",
  "line_number": 42,
  "term": "fake",
  "context": "runtime_evidence",
  "violation_type": "fake_pass_violation",
  "severity": "HIGH",
  "line_content": "return FakeDeviceBufferWriter::create();"
}
```

### Report Output
```json
{
  "scan_timestamp": "2026-06-26T01:09:00Z",
  "total_files_scanned": 150,
  "total_violations": 3,
  "violations": [...],
  "term_counts": {"fake": 2, "stub": 1},
  "context_counts": {"runtime_evidence": 3},
  "compliance_status": false
}
```

## Gate Integration

### RGF-003 Gate Requirements
1. Source redline scan implemented.
2. Scan covers production, test, and documentation code.
3. Allowlist integration working.
4. Violation reporting working.

### RGF-004 Gate Integration
- Artifact redline scan extends source redline scan.
- Same algorithms and allowlist checking.

### PFFG-001 Gate Integration
- Foundation Final Gate Reader runs source redline scan.
- Any HIGH severity violation blocks P2 Foundation claim.

## Test Cases

### Positive Test Cases
1. Scan production code without forbidden terms → PASS.
2. Scan test code with allowed term in test file name → PASS.
3. Scan documentation with term in Forbidden Scope section → PASS.
4. Scan fake service implementation with allowed term → PASS.
5. Scan documentation with term in Known Limitations section → PASS.
6. Scan documentation with term in Next Recommendation section → PASS.

### Negative Test Cases
1. Scan production code with forbidden term → VIOLATION.
2. Scan test code with forbidden term in runtime context → VIOLATION.
3. Scan documentation with term outside allowed sections → VIOLATION.
4. Scan evidence with forbidden term in RUNTIME_PASS context → VIOLATION.
5. Scan production code with fake term in non-allowlisted file → VIOLATION.
6. Scan documentation with stub term in non-allowed section → VIOLATION.

## Implementation Requirements

### File Discovery
1. Recursively scan specified directories.
2. Filter by file patterns.
3. Skip binary files and build artifacts.
4. Handle encoding issues gracefully.

### Performance Requirements
1. Scan 1000+ files in < 10 seconds.
2. Memory usage < 100MB.
3. Parallel scanning for large codebases.
4. Incremental scanning for changed files.

## Maintenance Rules

1. New forbidden terms require scan update.
2. New file patterns require scan update.
3. Allowlist changes require scan verification.
4. Scan coverage must be reported.