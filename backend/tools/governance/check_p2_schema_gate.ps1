# P2 Schema Gate Governance Check
# Validates that schema gate foundation files exist and self-test passes.

param(
    [string]$ProjectRoot = "."
)

$ProjectRoot = Resolve-Path $ProjectRoot
$errors = @()
$schemaRoot = Join-Path $ProjectRoot "backend/tools/governance/schema_gate"

Write-Output "=== P2 SCHEMA GATE GOVERNANCE CHECK ==="

# 1. Schema files exist
$schemas = @("p2_evidence_v8", "p2_inspector_v8", "p2_decision_ledger_v8",
             "p2_extension_decision_ledger_v8", "provider_runtime_matrix_v8")
foreach ($s in $schemas) {
    $path = Join-Path $schemaRoot "schemas/$s.schema.json"
    if (-not (Test-Path $path)) {
        $errors += "MISSING schema: $path"
    } else {
        Write-Output "  Schema OK: $s.schema.json"
    }
}

# 2. Validator exists
$validatorPath = Join-Path $schemaRoot "validator/schema_gate_validator.py"
if (-not (Test-Path $validatorPath)) {
    $errors += "MISSING validator: $validatorPath"
} else {
    Write-Output "  Validator OK: schema_gate_validator.py"
}

# 3. Valid samples exist
$validDir = Join-Path $schemaRoot "samples/valid"
$validCount = @(Get-ChildItem -Path $validDir -Filter "*.json" -ErrorAction SilentlyContinue).Count
if ($validCount -lt 1) {
    $errors += "MISSING valid samples (need >= 1)"
} else {
    Write-Output "  Valid samples: $validCount"
}

# 4. Invalid samples exist
$invalidDir = Join-Path $schemaRoot "samples/invalid"
$invalidCount = @(Get-ChildItem -Path $invalidDir -Filter "*.json" -ErrorAction SilentlyContinue).Count
if ($invalidCount -lt 5) {
    $errors += "MISSING invalid samples (need >= 5)"
} else {
    Write-Output "  Invalid samples: $invalidCount"
}

# 5. Self-test passes
$python = (Get-Command python -ErrorAction SilentlyContinue)
if ($python) {
    $result = & python $validatorPath --self-test 2>&1 | Out-String
    if ($LASTEXITCODE -eq 0 -and $result -match "Classification: PASS") {
        Write-Output "  Self-test: PASS"
    } else {
        $errors += "Self-test FAILED"
        Write-Output "  Self-test output:"
        Write-Output $result
    }
} else {
    Write-Output "  Self-test: SKIP (python not found)"
}

# Report
if ($errors.Count -eq 0) {
    Write-Output ""
    Write-Output "RESULT: PASS"
    Write-Output "  All schema gate foundation files present and self-test passes."
    exit 0
} else {
    Write-Output ""
    Write-Output "RESULT: FAIL"
    foreach ($e in $errors) { Write-Output "  $e" }
    exit 1
}
