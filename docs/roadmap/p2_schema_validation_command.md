# P2 Schema Validation Command

Updated: 2026-06-26

This document defines the schema validation command for V8 evidence, inspector, decision ledger, extension decision ledger, and provider runtime matrix schemas.

## Command Purpose

The schema validation command validates all P2 V8 schemas and ensures:
1. All required fields are present.
2. Status vocabulary compliance.
3. Runtime mode compliance.
4. Environment kind compliance.
5. Anti-fake rule compliance.
6. Redaction compliance.
7. Evidence/source consistency.

## Command Location

```
backend/tools/governance/validate_p2_schemas.py
```

## Command Usage

```bash
python backend/tools/governance/validate_p2_schemas.py [--verbose] [--fix] [--report]
```

## Validation Rules

### 1. Evidence Schema Validation (`p2-evidence-v8`)

#### Required Field Validation
- All 13 required top-level fields must be present.
- Field types must match schema definition.
- `schema_version` must be exactly "p2-evidence-v8".

#### Status Vocabulary Validation
- `status` must be one of: RUNTIME_PASS, CONTRACT_PASS, FAIL, BLOCKED_ENV, NOT_IMPLEMENTED, SKIPPED_TEST_ONLY.
- No custom or ad-hoc statuses allowed.

#### Runtime Mode Validation
- `runtime_mode` must be one of: runtime, contract, audit, regression, classification, schema, redline.
- Runtime mode must match task type.

#### Environment Kind Validation
- `runtime_environment_kind` must be one of: test_harness, local_machine, remote_server, cloud_account, simulated, none.
- Environment kind must be honest.

#### Anti-Fake Validation
- `RUNTIME_PASS` requires `environment_kind != "simulated"`.
- `RUNTIME_PASS` requires `environment_kind != "test_harness"` for providers requiring real environment.
- `RUNTIME_PASS` requires `evidence_file` to exist.
- Stub/simulator cannot claim `RUNTIME_PASS`.

#### Redaction Validation
- No full local paths (C:\, <REDACTED>/, <REDACTED>/).
- No token/cookie/credential leakage.
- No unredacted sample paths.

### 2. Inspector Schema Validation (`p2-inspector-v8`)

#### Required Field Validation
- All 13 top-level fields must be present.
- All 11 info objects must be present.
- Error entries must have all 7 fields.

#### Info Object Validation
- Each info object must have required fields.
- Field types must match schema definition.
- No missing nested objects.

#### Privacy Validation
- No full path leakage in source info.
- No credential leakage in provider info.
- Redacted identities required.

### 3. Decision Ledger Schema Validation (`p2-decision-ledger-v8`)

#### Required Field Validation
- All 6 top-level fields must be present.
- All entries must have 12 fields.
- Decision types must be from vocabulary.

#### Vocabulary Validation
- Decision types must be from 15 allowed types.
- Stages must be from 12 allowed stages.
- No custom decision types.

#### Monotonicity Validation
- Generation numbers must be monotonic increasing.
- Timestamps must be chronological.

### 4. Extension Decision Ledger Schema Validation (`p2-extension-decision-ledger-v8`)

#### Required Field Validation
- All 13 top-level fields must be present.
- All entries must have 12 fields.
- Extension decision types must be from vocabulary.

#### Boundary Validation
- Extension decisions cannot use MediaSourceKind decision types.
- Each extension type must use its corresponding stage.
- Cast decisions require cast stage, PiP decisions require pip stage, etc.

### 5. Provider Runtime Matrix Schema Validation (`provider-runtime-matrix-v8`)

#### Required Field Validation
- All 7 top-level fields must be present.
- All provider entries must have 13 fields.
- Provider kinds must be from vocabulary.

#### Runtime Status Validation
- `RUNTIME_PASS` requires `evidence_file` and `runtime_verified_timestamp`.
- `BLOCKED_ENV` requires `blocked_reason`.
- No provider defaults to `RUNTIME_PASS` without evidence.

#### Count Validation
- `total_providers` must match array length.
- `verified_providers` must count RUNTIME_PASS entries.
- `blocked_providers` must count BLOCKED_ENV entries.

## Validation Output

### Success Output
```
✓ Evidence schema validation: PASS (N files)
✓ Inspector schema validation: PASS
✓ Decision ledger schema validation: PASS
✓ Extension decision ledger schema validation: PASS
✓ Provider runtime matrix schema validation: PASS
✓ Anti-fake validation: PASS
✓ Redaction validation: PASS
✓ Vocabulary compliance: PASS
```

### Failure Output
```
✗ Evidence schema validation: FAIL
  - File: artifacts/p2/evidence/LRD-009.json
  - Error: Missing required field 'evidence_file'
  - Error: Status 'RUNTIME_PASS' but environment_kind is 'simulated'

✗ Provider runtime matrix validation: FAIL
  - Provider: local_file
  - Error: RUNTIME_PASS without evidence_file
  - Error: environment_available is false
```

## Validation Report

When `--report` flag is used, generates validation report at:
```
artifacts/p2/evidence/schema_validation_report.json
```

Report contains:
1. Validation timestamp.
2. Files validated.
3. Pass/fail counts.
4. Detailed error list.
5. Vocabulary compliance summary.
6. Anti-fake compliance summary.

## Integration with Gates

### SGF-006 Gate
- Validates all schemas exist and are correct.
- Validates all vocabulary is complete.
- Validates all rules are documented.

### SGF-007 Gate (Negative Tests)
- Validates negative test cases exist.
- Validates each schema has required negative tests.
- Validates negative tests cover edge cases.

### PFFG-001 Gate (Foundation Final Gate Reader)
- Validates all evidence files against schemas.
- Validates provider runtime matrix.
- Validates anti-fake compliance.
- Validates redaction compliance.

### APRV-001 Gate (APRV-001 verification)
- Validates all providers have RUNTIME_PASS.
- Validates all evidence is valid.
- Validates no BLOCKED_ENV or CONTRACT_PASS providers.

## Implementation Requirements

### Python Script Structure
```python
#!/usr/bin/env python3
"""P2 Schema Validation Command"""

import argparse
import json
import sys
from pathlib import Path
from typing import Dict, List, Any

class SchemaValidator:
    def __init__(self, verbose: bool = False):
        self.verbose = verbose
        self.errors = []
        self.warnings = []
    
    def validate_evidence_schema(self, file_path: Path) -> bool:
        """Validate evidence file against p2-evidence-v8 schema."""
        pass
    
    def validate_inspector_schema(self, file_path: Path) -> bool:
        """Validate inspector file against p2-inspector-v8 schema."""
        pass
    
    def validate_decision_ledger_schema(self, file_path: Path) -> bool:
        """Validate decision ledger file against p2-decision-ledger-v8 schema."""
        pass
    
    def validate_extension_decision_ledger_schema(self, file_path: Path) -> bool:
        """Validate extension decision ledger file."""
        pass
    
    def validate_provider_runtime_matrix(self, file_path: Path) -> bool:
        """Validate provider runtime matrix file."""
        pass
    
    def validate_anti_fake(self, data: Dict[str, Any]) -> bool:
        """Validate anti-fake rules."""
        pass
    
    def validate_redaction(self, data: Dict[str, Any]) -> bool:
        """Validate redaction compliance."""
        pass
    
    def validate_vocabulary(self, data: Dict[str, Any]) -> bool:
        """Validate vocabulary compliance."""
        pass
    
    def generate_report(self) -> Dict[str, Any]:
        """Generate validation report."""
        pass

def main():
    parser = argparse.ArgumentParser(description='P2 Schema Validation Command')
    parser.add_argument('--verbose', action='store_true', help='Verbose output')
    parser.add_argument('--fix', action='store_true', help='Attempt to fix issues')
    parser.add_argument('--report', action='store_true', help='Generate validation report')
    args = parser.parse_args()
    
    validator = SchemaValidator(verbose=args.verbose)
    
    # Validate all schemas
    evidence_files = list(Path('artifacts/p2/evidence').glob('*.json'))
    for file_path in evidence_files:
        validator.validate_evidence_schema(file_path)
    
    # Validate other schemas
    # ...
    
    # Generate report if requested
    if args.report:
        report = validator.generate_report()
        report_path = Path('artifacts/p2/evidence/schema_validation_report.json')
        with open(report_path, 'w') as f:
            json.dump(report, f, indent=2)
    
    # Exit with appropriate code
    if validator.errors:
        print(f"✗ Schema validation: FAIL ({len(validator.errors)} errors)")
        for error in validator.errors:
            print(f"  - {error}")
        sys.exit(1)
    else:
        print("✓ Schema validation: PASS")
        sys.exit(0)

if __name__ == '__main__':
    main()
```

## Test Cases

### Positive Test Cases
1. Valid evidence file with all required fields.
2. Valid inspector file with all info objects.
3. Valid decision ledger with monotonic generations.
4. Valid extension decision ledger with correct stages.
5. Valid provider runtime matrix with honest statuses.
6. Evidence file with proper redaction.

### Negative Test Cases
1. Evidence file missing required field.
2. Evidence file with invalid status.
3. Inspector file missing info object.
4. Decision ledger with non-monotonic generations.
5. Extension decision with wrong stage.
6. Provider matrix with RUNTIME_PASS without evidence.

## Integration with CMake

The validation command can be integrated into CMake test targets:

```cmake
add_test(
    NAME kivo_p2_schema_validation
    COMMAND python ${CMAKE_SOURCE_DIR}/backend/tools/governance/validate_p2_schemas.py --report
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
```

## Documentation Requirements

Each validation rule must be documented with:
1. Rule name and description.
2. Validation logic.
3. Error message format.
4. Example of valid/invalid data.
5. Integration point with gates.