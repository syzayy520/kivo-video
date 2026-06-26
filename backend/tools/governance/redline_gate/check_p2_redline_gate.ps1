param([string]$ProjectRoot = ".")
$ProjectRoot = Resolve-Path $ProjectRoot
$checker = Join-Path $ProjectRoot "backend/tools/governance/redline_gate/redline_checker.py"
$errors = @()
Write-Output "=== P2 REDLINE GATE ==="
if (-not (Test-Path $checker)) { $errors += "MISSING: redline_checker.py"; Write-Output "RESULT: FAIL"; exit 1 }
$result = & python $checker --self-test 2>&1 | Out-String
if ($LASTEXITCODE -ne 0 -or $result -notmatch "Classification: PASS") { $errors += "Self-test FAILED" }
$result = & python $checker --root="$ProjectRoot" 2>&1 | Out-String
if ($LASTEXITCODE -ne 0) { $errors += "Production scan FAILED" }
if ($errors.Count -eq 0) { Write-Output "RESULT: PASS"; exit 0 } else { Write-Output "RESULT: FAIL"; exit 1 }
