# P2 Final Gate Evidence Reader Foundation

Updated: 2026-06-26

This document defines the foundation for reading evidence files in final gates, ensuring evidence is consumed from files, not logs or narrative reports.

## Purpose

The Final Gate Evidence Reader Foundation provides:
1. Standardized evidence reading interface.
2. Evidence file discovery and validation.
3. Evidence content extraction.
4. Evidence consistency checking.
5. Evidence report generation.

## Reader Interface

### EvidenceReader Class
```cpp
class EvidenceReader {
public:
    // Constructor
    EvidenceReader(const std::string& evidence_dir);
    
    // Discovery
    std::vector<std::string> discover_evidence_files() const;
    std::vector<std::string> discover_provider_evidence() const;
    
    // Reading
    EvidenceFile read_evidence(const std::string& task_id) const;
    ProviderMatrix read_provider_matrix() const;
    
    // Validation
    bool validate_evidence(const EvidenceFile& evidence) const;
    bool validate_provider_matrix(const ProviderMatrix& matrix) const;
    
    // Extraction
    std::string extract_status(const EvidenceFile& evidence) const;
    std::string extract_runtime_mode(const EvidenceFile& evidence) const;
    std::string extract_environment_kind(const EvidenceFile& evidence) const;
    
    // Consistency
    bool check_evidence_source_consistency(const EvidenceFile& evidence) const;
    bool check_report_source_consistency(const std::string& report_path) const;
    
    // Reporting
    EvidenceReport generate_report() const;
};
```

### EvidenceFile Structure
```cpp
struct EvidenceFile {
    std::string schema_version;
    std::string task_id;
    std::string task_name;
    std::string family;
    std::string status;
    std::string runtime_mode;
    std::string runtime_environment_kind;
    bool environment_required;
    bool environment_available;
    std::optional<std::string> blocked_reason;
    std::string runtime_verified_timestamp;
    std::string evidence_file;
    nlohmann::json provider_matrix_impact;
    nlohmann::json additional_data;
};
```

### ProviderMatrix Structure
```cpp
struct ProviderMatrix {
    std::string schema_version;
    std::string generated_timestamp;
    std::string generation_commit;
    int total_providers;
    int verified_providers;
    int blocked_providers;
    std::vector<ProviderEntry> providers;
};

struct ProviderEntry {
    std::string provider_kind;
    std::string provider_name;
    std::string contract_status;
    std::string runtime_status;
    std::string environment_kind;
    bool environment_required;
    bool environment_available;
    std::optional<std::string> blocked_reason;
    std::optional<std::string> runtime_verified_timestamp;
    std::optional<std::string> evidence_file;
    nlohmann::json provider_matrix_impact;
    std::vector<std::string> commands;
    std::vector<std::string> limitations;
};
```

## Evidence Discovery Rules

### File Pattern
- Evidence files: `artifacts/p2/evidence/<task-id>.json`
- Provider matrix: `artifacts/p2/provider_runtime_matrix.json`
- Schema validation report: `artifacts/p2/evidence/schema_validation_report.json`

### Discovery Algorithm
1. Scan `artifacts/p2/evidence/` for `*.json` files.
2. Filter files matching `<task-id>.json` pattern.
3. Validate each file against `p2-evidence-v8` schema.
4. Collect valid evidence files.
5. Read provider matrix if exists.

### File Validation
1. File must be valid JSON.
2. File must contain required top-level fields.
3. `schema_version` must be "p2-evidence-v8".
4. `task_id` must match filename.
5. `evidence_file` must point to self.

## Evidence Reading Rules

### Status Extraction
1. Read `status` field from evidence file.
2. Validate status against V8 vocabulary.
3. Return status for gate decisions.

### Runtime Mode Extraction
1. Read `runtime_mode` field.
2. Validate runtime mode against vocabulary.
3. Return runtime mode for context.

### Environment Kind Extraction
1. Read `runtime_environment_kind` field.
2. Validate environment kind against vocabulary.
3. Return environment kind for environment decisions.

## Evidence Consistency Rules

### Evidence/Source Consistency
1. Evidence `task_id` must match source implementation.
2. Evidence `family` must match source family organization.
3. Evidence `status` must match test results.
4. Evidence `runtime_mode` must match implementation type.

### Report/Source Consistency
1. Report claims must match evidence status.
2. Report claims must match source implementation.
3. Report claims must match test results.
4. No premature completion claims.

### Evidence/Report Consistency
1. Evidence status must match report classification.
2. Evidence environment must match report environment.
3. Evidence timestamps must be consistent.
4. Evidence file paths must be valid.

## Evidence Validation Rules

### Schema Validation
1. Evidence must validate against `p2-evidence-v8` schema.
2. All required fields must be present.
3. All field types must be correct.
4. All vocabulary values must be valid.

### Anti-Fake Validation
1. `RUNTIME_PASS` requires real environment.
2. `RUNTIME_PASS` requires evidence file.
3. `RUNTIME_PASS` requires timestamp.
4. Stub/simulator cannot be `RUNTIME_PASS`.

### Redaction Validation
1. No full paths in evidence.
2. No tokens/cookies in evidence.
3. No unredacted sample paths.
4. Privacy compliance verified.

## Gate Integration

### PFFG-001 (Foundation Final Gate Reader)
```cpp
class FoundationFinalGateReader {
public:
    bool read_and_validate() const;
    
private:
    EvidenceReader reader_;
    
    bool validate_all_evidence() const;
    bool validate_provider_matrix() const;
    bool validate_anti_fake_compliance() const;
    bool validate_redaction_compliance() const;
    bool validate_evidence_source_consistency() const;
    bool validate_report_source_consistency() const;
};
```

### APRV-001 (All Providers Runtime Verification Gate)
```cpp
class AllProvidersRuntimeVerifiedGate {
public:
    bool verify_all_providers() const;
    
private:
    EvidenceReader reader_;
    
    bool verify_provider_runtime_status() const;
    bool verify_provider_evidence() const;
    bool verify_no_blocked_providers() const;
    bool verify_provider_consistency() const;
};
```

## Evidence Report Generation

### EvidenceReport Structure
```cpp
struct EvidenceReport {
    std::string generation_timestamp;
    std::string generation_commit;
    int total_evidence_files;
    int valid_evidence_files;
    int invalid_evidence_files;
    std::vector<std::string> invalid_files;
    std::map<std::string, int> status_counts;
    std::map<std::string, int> runtime_mode_counts;
    std::map<std::string, int> environment_kind_counts;
    bool anti_fake_compliance;
    bool redaction_compliance;
    bool vocabulary_compliance;
};
```

### Report Generation Rules
1. Scan all evidence files.
2. Validate each file.
3. Count statuses, runtime modes, environment kinds.
4. Check anti-fake compliance.
5. Check redaction compliance.
6. Check vocabulary compliance.
7. Generate report JSON.

## Implementation Requirements

### C++ Implementation
```cpp
// evidence_reader.h
#pragma once

#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>

namespace kivo::governance {

class EvidenceReader {
public:
    explicit EvidenceReader(const std::string& evidence_dir);
    
    // Discovery
    std::vector<std::string> discover_evidence_files() const;
    std::vector<std::string> discover_provider_evidence() const;
    
    // Reading
    EvidenceFile read_evidence(const std::string& task_id) const;
    ProviderMatrix read_provider_matrix() const;
    
    // Validation
    bool validate_evidence(const EvidenceFile& evidence) const;
    bool validate_provider_matrix(const ProviderMatrix& matrix) const;
    
    // Extraction
    std::string extract_status(const EvidenceFile& evidence) const;
    std::string extract_runtime_mode(const EvidenceFile& evidence) const;
    std::string extract_environment_kind(const EvidenceFile& evidence) const;
    
    // Consistency
    bool check_evidence_source_consistency(const EvidenceFile& evidence) const;
    bool check_report_source_consistency(const std::string& report_path) const;
    
    // Reporting
    EvidenceReport generate_report() const;
    
private:
    std::string evidence_dir_;
    nlohmann::json schema_;
};

}  // namespace kivo::governance
```

### Python Implementation
```python
# evidence_reader.py
import json
from pathlib import Path
from typing import Dict, List, Any, Optional

class EvidenceReader:
    def __init__(self, evidence_dir: str):
        self.evidence_dir = Path(evidence_dir)
        self.schema = self._load_schema()
    
    def discover_evidence_files(self) -> List[str]:
        """Discover all evidence files in evidence directory."""
        pass
    
    def read_evidence(self, task_id: str) -> Dict[str, Any]:
        """Read evidence file for specific task."""
        pass
    
    def validate_evidence(self, evidence: Dict[str, Any]) -> bool:
        """Validate evidence against schema."""
        pass
    
    def extract_status(self, evidence: Dict[str, Any]) -> str:
        """Extract status from evidence."""
        pass
    
    def check_consistency(self, evidence: Dict[str, Any], report_path: str) -> bool:
        """Check evidence/report consistency."""
        pass
    
    def generate_report(self) -> Dict[str, Any]:
        """Generate evidence report."""
        pass
```

## Test Cases

### Positive Test Cases
1. Read valid evidence file.
2. Discover all evidence files.
3. Validate evidence against schema.
4. Extract status from evidence.
5. Check evidence/source consistency.
6. Generate evidence report.

### Negative Test Cases
1. Read invalid evidence file.
2. Discover evidence with wrong pattern.
3. Validate evidence with missing fields.
4. Extract status from invalid evidence.
5. Check consistency with mismatched data.
6. Generate report with invalid evidence.

## Integration with Gates

### SGF-008 Gate Requirements
1. EvidenceReader interface defined.
2. Evidence discovery implemented.
3. Evidence reading implemented.
4. Evidence validation implemented.
5. Evidence consistency checking implemented.
6. Evidence report generation implemented.

### PFFG-001 Integration
- Foundation Final Gate Reader uses EvidenceReader.
- Validates all evidence before P2 Foundation claim.
- Generates evidence report for final gate.

### APRV-001 Integration
- All Providers Runtime Verification Gate uses EvidenceReader.
- Validates all provider evidence.
- Verifies no BLOCKED_ENV providers.

## Maintenance Rules

1. EvidenceReader must be updated with schema changes.
2. New evidence types require reader updates.
3. Validation rules must be kept in sync.
4. Test coverage must be maintained.