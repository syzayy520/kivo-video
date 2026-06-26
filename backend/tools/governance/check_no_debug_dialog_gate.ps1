# No Debug Dialog Gate
# Validates that runtime gate tests do NOT contain assert/abort calls
# that would trigger MSVC Debug CRT popups.

param(
    [string]$ProjectRoot = "."
)

$ProjectRoot = Resolve-Path $ProjectRoot
$errors = @()

Write-Output "=== NO DEBUG DIALOG GATE ==="

# Scan runtime gate tests for forbidden assert/abort
$forbiddenPatterns = @("assert\s*\(", "abort\s*\(", "std::abort")

$gateTestFiles = @(
    "$ProjectRoot\backend\tests\presenter\local_playback_e2e_test.cpp",
    "$ProjectRoot\backend\tests\presenter\micro_soak_test.cpp"
)

foreach ($file in $gateTestFiles) {
    if (-not (Test-Path $file)) {
        Write-Output "  WARN: file not found: $file"
        continue
    }
    
    $content = Get-Content $file -Raw
    
    foreach ($pattern in $forbiddenPatterns) {
        $matches = [regex]::Matches($content, $pattern)
        if ($matches.Count -gt 0) {
            $errors += "FORBIDDEN: $file contains '$pattern' ($($matches.Count) match(es)) — must use explicit return codes instead"
        }
    }
    
    # Check for install_no_crt_popup_guard call
    if ($content -notmatch "install_no_crt_popup_guard") {
        $errors += "MISSING: $file does not call install_no_crt_popup_guard() at main() entry"
    }
}

# Check test_exit.hpp exists
$testExitHpp = "$ProjectRoot\backend\tests\support\test_exit.hpp"
if (-not (Test-Path $testExitHpp)) {
    $errors += "MISSING: $testExitHpp not found — test exit utility must exist"
}

# Report
if ($errors.Count -eq 0) {
    Write-Output ""
    Write-Output "RESULT: PASS"
    Write-Output "  Runtime gate tests: no assert/abort, CRT popup guard installed"
    Write-Output "  test_exit.hpp: present"
    exit 0
} else {
    Write-Output ""
    Write-Output "RESULT: FAIL"
    foreach ($e in $errors) {
        Write-Output "  $e"
    }
    exit 1
}
