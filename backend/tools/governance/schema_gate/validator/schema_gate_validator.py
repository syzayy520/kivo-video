#!/usr/bin/env python3
"""
P2 Schema Gate Validator V8
Validates evidence files against P2 V8 schemas and semantic rules.
Python stdlib only - no external dependencies required.
"""
import json
import os
import sys
import glob
from datetime import datetime

# ─── Status Vocabulary ───
VALID_STATUSES = {"RUNTIME_PASS", "CONTRACT_PASS", "FAIL", "BLOCKED_ENV", "NOT_IMPLEMENTED", "SKIPPED_TEST_ONLY"}
VALID_RUNTIME_KINDS = {
    "local_real_runtime", "windows_device_runtime",
    "controlled_real_protocol_server", "real_external_service",
    "real_external_account", "real_external_device",
    "mock", "unit_test", "simulator", "not_applicable"
}
MOCK_KINDS = {"mock", "simulator", "unit_test"}
MEDIA_INPUT_KINDS = {
    "local_file", "smb", "nfs", "ftp", "ftps", "sftp",
    "http_file", "https_file", "http_stream", "https_stream",
    "dlna_upnp", "s3_object", "minio_object",
    "webdav", "alist", "emby", "plex", "jellyfin",
    "google_drive", "onedrive", "iptv_m3u", "hls_live"
}
EXTENSION_INPUT_KINDS = {
    "cast_target", "pip_surface", "cinema_mode", "trakt_identity",
    "playlist_state", "queue_state", "resume_state", "playback_history"
}
PROVIDER_KINDS = MEDIA_INPUT_KINDS | EXTENSION_INPUT_KINDS | {"iptv_hls", "airplay_cast", "trakt_api"}

class SchemaGateResult:
    def __init__(self):
        self.errors = []
        self.warnings = []
        self.files_checked = 0
        self.files_valid = 0
        self.files_invalid = 0

    def add_error(self, msg):
        self.errors.append(msg)

    def add_warning(self, msg):
        self.warnings.append(msg)

    @property
    def passed(self):
        return len(self.errors) == 0 and self.files_checked > 0

    @property
    def classification(self):
        if self.passed:
            return "RUNTIME_PASS"
        return "FAIL"

def load_json(path):
    with open(path, 'r', encoding='utf-8') as f:
        return json.load(f)

def validate_schema_version(data, expected):
    sv = data.get("schema_version", "")
    if sv != expected:
        return f"schema_version '{sv}' != expected '{expected}'"
    return None

def check_required_fields(data, fields):
    missing = [f for f in fields if f not in data]
    if missing:
        return f"missing required fields: {missing}"
    return None

# ─── p2-evidence-v8 semantic rules ───
def validate_p2_evidence(data, path):
    errors = []

    err = validate_schema_version(data, "p2-evidence-v8")
    if err: errors.append(err)

    required = ["task_id", "task_family", "status", "runtime_mode",
                "runtime_environment_kind", "inspector_snapshot", "decision_ledger",
                "redaction_verified", "runtime_verified", "timestamp_utc"]
    err = check_required_fields(data, required)
    if err: errors.append(err)

    status = data.get("status", "")
    if status not in VALID_STATUSES:
        errors.append(f"unknown status: '{status}'")

    rtv = data.get("runtime_verified", None)
    if status == "RUNTIME_PASS" and rtv != True:
        errors.append("RUNTIME_PASS but runtime_verified is not true")
    if status == "CONTRACT_PASS" and rtv != False:
        errors.append("CONTRACT_PASS but runtime_verified is not false")

    rek = data.get("runtime_environment_kind", "")
    if rek not in VALID_RUNTIME_KINDS:
        errors.append(f"unknown runtime_environment_kind: '{rek}'")
    if status == "RUNTIME_PASS" and rek in MOCK_KINDS:
        errors.append(f"RUNTIME_PASS with runtime_environment_kind={rek} (mock/simulator not allowed for runtime)")

    redaction = data.get("redaction_verified", None)
    if status in ("RUNTIME_PASS", "CONTRACT_PASS") and redaction != True:
        errors.append(f"status={status} but redaction_verified is not true")

    inspector = data.get("inspector_snapshot", None)
    if status in ("RUNTIME_PASS", "CONTRACT_PASS") and (inspector is None or inspector == {}):
        errors.append(f"status={status} but inspector_snapshot is missing or empty")

    ledger = data.get("decision_ledger", None)
    if status in ("RUNTIME_PASS", "CONTRACT_PASS") and (ledger is None or ledger == {}):
        errors.append(f"status={status} but decision_ledger is missing or empty")

    media_input = data.get("media_input", {})
    mi_kind = media_input.get("kind", "")
    if mi_kind in EXTENSION_INPUT_KINDS:
        errors.append(f"media_input.kind='{mi_kind}' is an extension kind, not a media source")

    silent_fb = data.get("anti_fake_checks", {}).get("silent_fallback", False)
    if silent_fb and status in ("RUNTIME_PASS", "CONTRACT_PASS"):
        errors.append("silent_fallback=true but status is not FAIL")

    return errors

# ─── p2-inspector-v8 semantic rules ───
def validate_p2_inspector(data, path):
    errors = []
    err = validate_schema_version(data, "p2-inspector-v8")
    if err: errors.append(err)

    required = ["inspector_id", "task_id", "source_summary", "media_summary", "timestamp_utc"]
    err = check_required_fields(data, required)
    if err: errors.append(err)

    redaction = data.get("redaction_summary", {})
    if redaction.get("no_full_path_leakage") != True:
        errors.append("redaction_summary.no_full_path_leakage is not true")
    if redaction.get("no_token_cookie_credential_leakage") != True:
        errors.append("redaction_summary.no_token_cookie_credential_leakage is not true")

    return errors

# ─── p2-decision-ledger-v8 semantic rules ───
def validate_p2_decision_ledger(data, path):
    errors = []
    err = validate_schema_version(data, "p2-decision-ledger-v8")
    if err: errors.append(err)

    required = ["ledger_id", "task_id", "decisions", "timestamp_utc"]
    err = check_required_fields(data, required)
    if err: errors.append(err)

    decisions = data.get("decisions", [])
    if not isinstance(decisions, list) or len(decisions) == 0:
        errors.append("decisions array is missing or empty")

    for d in decisions:
        for f in ["decision_id", "domain", "decision", "reason", "status"]:
            if f not in d:
                errors.append(f"decision missing required field: '{f}'")

    return errors

# ─── p2-extension-decision-ledger-v8 semantic rules ───
def validate_p2_extension_decision_ledger(data, path):
    errors = []
    err = validate_schema_version(data, "p2-extension-decision-ledger-v8")
    if err: errors.append(err)

    required = ["ledger_id", "task_id", "extension_kind", "extension_identity", "timestamp_utc"]
    err = check_required_fields(data, required)
    if err: errors.append(err)

    ek = data.get("extension_kind", "")
    if ek not in EXTENSION_INPUT_KINDS:
        errors.append(f"unknown extension_kind: '{ek}'")
    if ek in MEDIA_INPUT_KINDS:
        errors.append(f"extension_kind='{ek}' is a media kind, not an extension kind")

    return errors

# ─── provider-runtime-matrix-v8 semantic rules ───
def validate_provider_runtime_matrix(data, path):
    errors = []
    err = validate_schema_version(data, "provider-runtime-matrix-v8")
    if err: errors.append(err)

    required = ["matrix_id", "generated_by_task", "providers", "timestamp_utc"]
    err = check_required_fields(data, required)
    if err: errors.append(err)

    providers = data.get("providers", [])
    if not isinstance(providers, list) or len(providers) == 0:
        errors.append("providers array is missing or empty")

    # Check each provider
    runtime_pass_count = 0
    for p in providers:
        pk = p.get("provider_kind", "")
        rs = p.get("runtime_status", "")
        rv = p.get("runtime_verified", None)
        rek = p.get("runtime_environment_kind", "")
        evidence = p.get("evidence_file", "")

        if pk not in PROVIDER_KINDS:
            errors.append(f"unknown provider_kind: '{pk}'")

        if rs == "RUNTIME_PASS":
            runtime_pass_count += 1
            if rv != True:
                errors.append(f"provider '{pk}': RUNTIME_PASS but runtime_verified != true")
            if rek in MOCK_KINDS:
                errors.append(f"provider '{pk}': RUNTIME_PASS with mock/simulator environment")
            if not evidence:
                errors.append(f"provider '{pk}': RUNTIME_PASS but evidence_file is empty")

        if rs not in VALID_STATUSES:
            errors.append(f"provider '{pk}': unknown runtime_status '{rs}'")

        if p.get("contract_status", "") not in ("CONTRACT_PASS", "NOT_IMPLEMENTED", "FAIL"):
            errors.append(f"provider '{pk}': unknown contract_status")

    # Provider count must be >= 21
    if len(providers) < 21:
        warnings = []
        warnings.append(f"provider matrix has only {len(providers)} providers (expected >= 21)")

    return errors

# ─── Dispatch table ───
VALIDATORS = {
    "p2-evidence-v8": validate_p2_evidence,
    "p2-inspector-v8": validate_p2_inspector,
    "p2-decision-ledger-v8": validate_p2_decision_ledger,
    "p2-extension-decision-ledger-v8": validate_p2_extension_decision_ledger,
    "provider-runtime-matrix-v8": validate_provider_runtime_matrix,
}

def validate_file(path):
    try:
        data = load_json(path)
    except (json.JSONDecodeError, IOError) as e:
        return [f"failed to parse JSON: {e}"], []

    sv = data.get("schema_version", "")
    validator = VALIDATORS.get(sv)
    if validator is None:
        return [f"unknown schema_version: '{sv}' in {os.path.basename(path)}"], []
    result = validator(data, path)
    if isinstance(result, tuple) and len(result) == 2:
        return result
    return result, []

def validate_directory(directory):
    result = SchemaGateResult()
    for root, _, files in os.walk(directory):
        for f in files:
            if f.endswith('.json'):
                path = os.path.join(root, f)
                result.files_checked += 1
                errors, warnings = validate_file(path)
                if errors:
                    result.files_invalid += 1
                    for e in errors:
                        result.add_error(f"{path}: {e}")
                else:
                    result.files_valid += 1
                for w in warnings:
                    result.add_warning(f"{path}: {w}")
    return result

def self_test():
    """Run built-in validation tests against sample files."""
    base = os.path.dirname(os.path.abspath(__file__))
    root = os.path.join(base, "..")
    result_dir = os.path.join(root, "..")

    valid_dir = os.path.join(root, "samples", "valid")
    invalid_dir = os.path.join(root, "samples", "invalid")

    print(f"Valid dir: {valid_dir} (exists: {os.path.isdir(valid_dir)})")
    print(f"Invalid dir: {invalid_dir} (exists: {os.path.isdir(invalid_dir)})")

    passed = 0
    failed = 0

    print("=== P2 Schema Gate Validator Self-Test ===\n")

    # Test valid samples
    if os.path.isdir(valid_dir):
        for f in sorted(os.listdir(valid_dir)):
            if f.endswith('.json'):
                path = os.path.join(valid_dir, f)
                errors, _ = validate_file(path)
                if not errors:
                    print(f"  PASS: {f}")
                    passed += 1
                else:
                    print(f"  FAIL: {f}")
                    for e in errors:
                        print(f"    -> {e}")
                    failed += 1

    # Test invalid samples
    if os.path.isdir(invalid_dir):
        for f in sorted(os.listdir(invalid_dir)):
            if f.endswith('.json'):
                path = os.path.join(invalid_dir, f)
                errors, _ = validate_file(path)
                if errors:
                    print(f"  PASS (expected fail): {f}")
                    for e in errors:
                        print(f"    -> {e}")
                    passed += 1
                else:
                    print(f"  FAIL (expected fail but passed): {f}")
                    failed += 1

    print(f"\n  Total: {passed + failed}, Passed: {passed}, Failed: {failed}")
    if failed > 0:
        print("Classification: FAIL")
        sys.exit(1)
    else:
        print("Classification: PASS")
        sys.exit(0)

def main():
    if "--self-test" in sys.argv:
        self_test()
        return

    if len(sys.argv) < 2:
        print("Usage: python schema_gate_validator.py <file|directory> [--self-test]")
        sys.exit(1)

    target = sys.argv[1]
    if not os.path.exists(target):
        print(f"Error: '{target}' not found")
        sys.exit(1)

    if os.path.isfile(target):
        errors, warnings = validate_file(target)
        if errors:
            print(f"Classification: FAIL")
            for e in errors: print(f"  FAIL: {e}")
            sys.exit(1)
        else:
            print(f"Classification: RUNTIME_PASS")
            print(f"  {target}: valid")
            sys.exit(0)
    else:
        result = validate_directory(target)
        print(f"Classification: {result.classification}")
        print(f"  Files checked: {result.files_checked}")
        print(f"  Files valid:   {result.files_valid}")
        print(f"  Files invalid: {result.files_invalid}")
        for e in result.errors:
            print(f"  FAIL: {e}")
        for w in result.warnings:
            print(f"  WARN: {w}")
        sys.exit(0 if result.passed else 1)

if __name__ == "__main__":
    main()
