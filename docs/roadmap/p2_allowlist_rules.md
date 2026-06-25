# P2 Allowlist Rules

Updated: 2026-06-26

This document defines the allowlist rules for P2 redline vocabulary, specifying when forbidden terms are permitted.

## Purpose

Allowlist rules define the specific contexts where forbidden terms are allowed, preventing false redline violations while maintaining strict runtime evidence integrity.

## Allowlist Contexts

### 1. Forbidden Scope Documentation
**Context**: Documentation describing what is NOT implemented in P2.
**Allowed Terms**: All forbidden terms.
**Example**: "Hardware decode is NOT implemented in P2 scope."
**File Patterns**: `docs/roadmap/*forbidden*.md`, `docs/roadmap/*scope*.md`

### 2. Known Limitations Documentation
**Context**: Documentation describing known limitations of current implementation.
**Allowed Terms**: All forbidden terms.
**Example**: "This is a stub implementation requiring real D3D11 environment."
**File Patterns**: `docs/roadmap/*limitation*.md`, `docs/roadmap/*known*.md`

### 3. Next Recommendation Documentation
**Context**: Documentation recommending future work or improvements.
**Allowed Terms**: All forbidden terms.
**Example**: "Future work should implement real hardware decode."
**File Patterns**: `docs/roadmap/*recommendation*.md`, `docs/roadmap/*next*.md`

### 4. CONTRACT_PASS Explanation
**Context**: Evidence explaining why status is CONTRACT_PASS instead of RUNTIME_PASS.
**Allowed Terms**: fake, stub, simulate, simulator, placeholder.
**Example**: "CONTRACT_PASS: This is a stub implementation, not real runtime."
**File Patterns**: `artifacts/p2/evidence/*.json` (status field must be CONTRACT_PASS)

### 5. Test File Names
**Context**: Test file names containing forbidden terms.
**Allowed Terms**: fake, stub, mock, simulate, dummy.
**Example**: `fake_device_buffer_writer_test.cpp`, `stub_implementation_test.cpp`
**File Patterns**: `backend/tests/**/*fake*.cpp`, `backend/tests/**/*stub*.cpp`

### 6. Fake Service Implementations
**Context**: Fake service implementation files.
**Allowed Terms**: fake, stub, mock, simulate, dummy.
**Example**: `fake_device_buffer_writer.cpp`, `stub_implementation.cpp`
**File Patterns**: `backend/src/**/*fake*.cpp`, `backend/src/**/*stub*.cpp`

## Allowlist Rules Format

### Rule Structure
```json
{
  "term": "fake",
  "contexts": [
    {
      "name": "test_file_names",
      "description": "Test file names containing forbidden terms",
      "file_patterns": ["backend/tests/**/*fake*.cpp"],
      "example": "fake_device_buffer_writer_test.cpp"
    },
    {
      "name": "fake_service_implementations", 
      "description": "Fake service implementation files",
      "file_patterns": ["backend/src/**/*fake*.cpp"],
      "example": "fake_device_buffer_writer.cpp"
    },
    {
      "name": "contract_pass_explanation",
      "description": "Evidence explaining CONTRACT_PASS status",
      "file_patterns": ["artifacts/p2/evidence/*.json"],
      "condition": "status == CONTRACT_PASS",
      "example": "CONTRACT_PASS: This is a stub implementation"
    }
  ]
}
```

### Context Detection Rules
1. **File Pattern Matching**: Match file path against allowed patterns.
2. **Section Detection**: Detect document section (Forbidden Scope, Known Limitations, etc.).
3. **Status Condition**: Check evidence status field for CONTRACT_PASS.
4. **Line Context**: Analyze surrounding lines for context.

## Allowlist Implementation

### AllowlistChecker Class
```cpp
class AllowlistChecker {
public:
    // Context Detection
    std::string detect_context(const std::string& file_path, int line_number) const;
    bool is_in_forbidden_scope_section(const std::string& file_path, int line_number) const;
    bool is_in_known_limitations_section(const std::string& file_path, int line_number) const;
    bool is_in_next_recommendation_section(const std::string& file_path, int line_number) const;
    
    // Term Checking
    bool is_term_allowed(const std::string& term, const std::string& context) const;
    bool is_term_allowed_in_file(const std::string& term, const std::string& file_path) const;
    bool is_term_allowed_in_line(const std::string& term, const std::string& file_path, int line_number) const;
    
    // Allowlist Loading
    void load_allowlist(const std::string& allowlist_path);
    void add_allowlist_rule(const AllowlistRule& rule);
    
    // Reporting
    AllowlistReport generate_report() const;
};

struct AllowlistRule {
    std::string term;
    std::vector<AllowlistContext> contexts;
};

struct AllowlistContext {
    std::string name;
    std::string description;
    std::vector<std::string> file_patterns;
    std::optional<std::string> condition;
    std::string example;
};
```

### Python Implementation
```python
class AllowlistChecker:
    def __init__(self, allowlist_path: str):
        self.allowlist = self._load_allowlist(allowlist_path)
    
    def detect_context(self, file_path: str, line_number: int) -> str:
        """Detect context for line in file."""
        pass
    
    def is_term_allowed(self, term: str, context: str) -> bool:
        """Check if term is allowed in context."""
        pass
    
    def is_term_allowed_in_file(self, term: str, file_path: str) -> bool:
        """Check if term is allowed anywhere in file."""
        pass
    
    def is_in_forbidden_scope_section(self, file_path: str, line_number: int) -> bool:
        """Check if line is in Forbidden Scope section."""
        pass
```

## Context Detection Algorithms

### 1. File Pattern Detection
```python
def detect_context_by_file_pattern(file_path: str) -> str:
    if re.match(r"docs/roadmap/\*forbidden\*.md", file_path):
        return "forbidden_scope_documentation"
    elif re.match(r"docs/roadmap/\*limitation\*.md", file_path):
        return "known_limitations_documentation"
    elif re.match(r"backend/tests/\*\*/\*fake\*.cpp", file_path):
        return "test_file_names"
    elif re.match(r"backend/src/\*\*/\*fake\*.cpp", file_path):
        return "fake_service_implementations"
    else:
        return "unknown"
```

### 2. Section Detection
```python
def detect_section_by_heading(file_path: str, line_number: int) -> str:
    # Read file and find nearest heading above line_number
    # Check if heading matches Forbidden Scope, Known Limitations, etc.
    pass
```

### 3. Status Condition Detection
```python
def detect_context_by_status(evidence_data: dict) -> str:
    if evidence_data.get("status") == "CONTRACT_PASS":
        return "contract_pass_explanation"
    else:
        return "runtime_evidence"
```

## Allowlist Validation Rules

### Rule 1: Context Must Be Defined
- Each allowlist rule must specify valid contexts.
- Unknown contexts are rejected.

### Rule 2: File Patterns Must Be Valid
- File patterns must be valid glob patterns.
- Patterns must match actual file paths.

### Rule 3: Conditions Must Be Valid
- Conditions must be valid boolean expressions.
- Conditions must reference valid fields.

### Rule 4: Examples Must Be Real
- Examples must be real examples from codebase.
- Examples must demonstrate allowed usage.

## Redline Scan Integration

### Scan Process
1. Scan file for forbidden terms.
2. For each forbidden term found:
   a. Detect context for the line.
   b. Check if term is allowed in that context.
   c. If allowed, skip violation.
   d. If not allowed, record violation.

### Violation Recording
```json
{
  "file_path": "backend/src/playback_core/playback_controller.cpp",
  "line_number": 42,
  "term": "fake",
  "context": "runtime_evidence",
  "violation_type": "fake_pass_violation",
  "severity": "HIGH",
  "message": "Term 'fake' found in runtime evidence context"
}
```

## Gate Integration

### RGF-002 Gate Requirements
1. Allowlist rules defined and documented.
2. Context detection implemented.
3. Allowlist checking implemented.
4. Integration with redline scanner.

### RGF-003 Gate Integration
- Source redline scan uses allowlist checker.
- Only violations outside allowed contexts are reported.

### RGF-004 Gate Integration
- Artifact redline scan uses allowlist checker.
- Evidence files with CONTRACT_PASS status can contain stub/fake terms.

## Test Cases

### Positive Test Cases
1. Term in Forbidden Scope section → ALLOWED.
2. Term in Known Limitations section → ALLOWED.
3. Term in test file name → ALLOWED.
4. Term in fake service implementation → ALLOWED.
5. Term in CONTRACT_PASS evidence explanation → ALLOWED.
6. Term in Next Recommendation section → ALLOWED.

### Negative Test Cases
1. Term in runtime evidence → VIOLATION.
2. Term in RUNTIME_PASS evidence → VIOLATION.
3. Term in production code → VIOLATION.
4. Term in report without context → VIOLATION.
5. Term in evidence with wrong status → VIOLATION.
6. Term in documentation outside allowed sections → VIOLATION.

## Documentation Requirements

Each allowlist rule must document:
1. Term and description.
2. Allowed contexts with examples.
3. File patterns.
4. Conditions.
5. Examples of allowed and disallowed usage.

## Maintenance Rules

1. New forbidden terms require allowlist review.
2. New contexts require allowlist update.
3. Allowlist rules must be tested.
4. Allowlist coverage must be reported.