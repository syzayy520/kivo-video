#!/usr/bin/env python3
"""
P2 Schema Gate Validator V8 — Strict Edition
Validates evidence against P2 V8 schemas with all required fields.
Python stdlib only.
"""
import json, os, sys

VALID_STATUSES = {"RUNTIME_PASS","CONTRACT_PASS","FAIL","BLOCKED_ENV","NOT_IMPLEMENTED","SKIPPED_TEST_ONLY"}
VALID_RUNTIME_KINDS = {"local_real_runtime","windows_device_runtime","controlled_real_protocol_server","real_external_service","real_external_account","real_external_device","mock","unit_test","simulator","not_applicable"}
MOCK_KINDS = {"mock","simulator","unit_test"}
MEDIA_KINDS = {"local_file","smb","nfs","ftp","ftps","sftp","http_file","https_file","http_stream","https_stream","dlna_upnp","s3_object","minio_object","webdav","alist","emby","plex","jellyfin","google_drive","onedrive","iptv_m3u","hls_live"}
EXTENSION_KINDS = {"cast_target","pip_surface","cinema_mode","trakt_identity","playlist_state","queue_state","resume_state","playback_history"}
PROVIDER_KINDS = MEDIA_KINDS | EXTENSION_KINDS | {"iptv_hls","airplay_cast","trakt_api"}

def load_json(path):
    with open(path, 'r', encoding='utf-8') as f:
        return json.load(f)

def check_req(data, fields):
    missing = [f for f in fields if f not in data]
    return [f"missing field: {m}" for m in missing] if missing else []

def check_nonempty(data, fields):
    return [f"field '{f}' is empty" for f in fields if f in data and not data[f]]

# ═══════════════════════════════════════════
# p2-evidence-v8
# ═══════════════════════════════════════════
EVIDENCE_REQUIRED = [
    "schema_version","task_id","task_family","status",
    "completion_level","runtime_mode","runtime_environment_kind",
    "environment","implementation",
    "media_input","extension_input",
    "media_identity","source_identity","extension_identity",
    "capability_probe",
    "direct_play_decision","direct_stream_decision",
    "subtitle_snapshot","chapter_snapshot","playback_state_snapshot",
    "inspector_snapshot","decision_ledger",
    "errors",
    "redaction_verified","schema_validated",
    "anti_fake_checks","redline_scan",
    "test_commands","artifacts",
    "requires_provider_matrix",
    "runtime_verified","blocked_reason",
    "timestamp_utc"
]

INSPECTOR_REQUIRED = [
    "schema_version","inspector_id","task_id",
    "source_summary","media_summary",
    "video_summary","audio_summary",
    "subtitle_summary","chapter_summary",
    "playback_summary","runtime_summary","environment_summary",
    "errors","warnings","redaction_summary",
    "timestamp_utc"
]

LEDGER_REQUIRED = [
    "schema_version","ledger_id","task_id",
    "decisions","fallbacks","blocked_reasons",
    "runtime_claims","forbidden_claims","evidence_links",
    "timestamp_utc"
]

DECISION_REQUIRED = ["decision_id","domain","input","decision","reason","status","allowed_claim","forbidden_claim"]

EXT_LEDGER_REQUIRED = [
    "schema_version","ledger_id","task_id",
    "extension_kind","extension_identity",
    "session_state","handoff_policy","state_split_policy",
    "privacy_policy","sync_policy",
    "blocked_reasons","runtime_claims","forbidden_claims",
    "timestamp_utc"
]

PROVIDER_REQUIRED = [
    "schema_version","matrix_id","generated_by_task","providers","summary","timestamp_utc"
]

PROVIDER_ITEM_REQUIRED = [
    "provider_kind","contract_status","runtime_status",
    "runtime_environment_kind","environment_required","environment_available",
    "runtime_verified","blocked_reason",
    "test_commands","evidence_file",
    "last_verified_utc","allowed_claim","forbidden_claim"
]

def validate_evidence(data, path):
    errs = []
    errs += check_req(data, EVIDENCE_REQUIRED)
    sv = data.get("schema_version","")
    if sv != "p2-evidence-v8": errs.append(f"bad schema_version: '{sv}'")

    st = data.get("status","")
    if st not in VALID_STATUSES: errs.append(f"unknown status: '{st}'")

    rtv = data.get("runtime_verified")
    if st == "RUNTIME_PASS" and rtv != True: errs.append("RUNTIME_PASS requires runtime_verified=true")
    if st == "CONTRACT_PASS" and rtv != False: errs.append("CONTRACT_PASS requires runtime_verified=false")

    rek = data.get("runtime_environment_kind","")
    if rek not in VALID_RUNTIME_KINDS: errs.append(f"unknown runtime_environment_kind: '{rek}'")
    if st == "RUNTIME_PASS" and rek in MOCK_KINDS: errs.append(f"RUNTIME_PASS with {rek} forbidden")

    if data.get("redaction_verified") != True and st in ("RUNTIME_PASS","CONTRACT_PASS"):
        errs.append("redaction_verified must be true for PASS statuses")

    if data.get("schema_validated") != True and st in ("RUNTIME_PASS","CONTRACT_PASS"):
        errs.append("schema_validated must be true for PASS statuses")

    if st == "BLOCKED_ENV" and data.get("completion_level","").startswith("p2-foundation"):
        errs.append("BLOCKED_ENV not allowed as foundation child status")

    mi = data.get("media_input",{}).get("kind","")
    if mi in EXTENSION_KINDS: errs.append(f"media_input.kind='{mi}' is extension kind, not media source")

    af = data.get("anti_fake_checks",{}).get("silent_fallback",False)
    if af and st in ("RUNTIME_PASS","CONTRACT_PASS"): errs.append("silent_fallback=true with non-FAIL status")

    return errs

def validate_inspector(data, path):
    errs = []
    errs += check_req(data, INSPECTOR_REQUIRED)
    sv = data.get("schema_version","")
    if sv != "p2-inspector-v8": errs.append(f"bad schema_version: '{sv}'")
    rs = data.get("redaction_summary",{})
    if rs.get("no_full_path_leakage") != True: errs.append("no_full_path_leakage must be true")
    if rs.get("no_token_cookie_credential_leakage") != True: errs.append("no_token_cookie_credential_leakage must be true")
    return errs

def validate_ledger(data, path):
    errs = []
    errs += check_req(data, LEDGER_REQUIRED)
    if data.get("schema_version","") != "p2-decision-ledger-v8": errs.append("bad schema_version")
    decs = data.get("decisions",[])
    if not isinstance(decs,list) or len(decs)==0: errs.append("decisions empty")
    for d in decs:
        m = [f for f in DECISION_REQUIRED if f not in d]
        if m: errs.append(f"decision missing: {m}")
    return errs

def validate_ext_ledger(data, path):
    errs = []
    errs += check_req(data, EXT_LEDGER_REQUIRED)
    if data.get("schema_version","") != "p2-extension-decision-ledger-v8": errs.append("bad schema_version")
    ek = data.get("extension_kind","")
    if ek not in EXTENSION_KINDS or ek in MEDIA_KINDS: errs.append(f"extension_kind='{ek}' is media kind, not extension")
    return errs

def validate_provider_matrix(data, path):
    errs = []
    errs += check_req(data, PROVIDER_REQUIRED)
    if data.get("schema_version","") != "provider-runtime-matrix-v8": errs.append("bad schema_version")
    ps = data.get("providers",[])
    if not isinstance(ps,list) or len(ps)==0: errs.append("providers empty")
    runtime_count = 0
    for p in ps:
        errs += check_req(p, PROVIDER_ITEM_REQUIRED)
        pk = p.get("provider_kind","")
        if pk not in PROVIDER_KINDS: errs.append(f"unknown provider_kind: '{pk}'")
        rs = p.get("runtime_status","")
        if rs == "RUNTIME_PASS":
            runtime_count += 1
            if p.get("runtime_verified") != True: errs.append(f"'{pk}' RUNTIME_PASS needs runtime_verified=true")
            if p.get("runtime_environment_kind","") in MOCK_KINDS: errs.append(f"'{pk}' RUNTIME_PASS with mock/simulator env")
            if not p.get("evidence_file",""): errs.append(f"'{pk}' RUNTIME_PASS needs evidence_file")
        if rs not in VALID_STATUSES and rs != "": errs.append(f"'{pk}' unknown runtime_status: '{rs}'")
        cs = p.get("contract_status","")
        if cs not in ("CONTRACT_PASS","NOT_IMPLEMENTED","FAIL",""): errs.append(f"'{pk}' bad contract_status")
    if len(ps) < 21: errs.append(f"only {len(ps)} providers (need >=21)")
    return errs

VALIDATORS = {
    "p2-evidence-v8": validate_evidence,
    "p2-inspector-v8": validate_inspector,
    "p2-decision-ledger-v8": validate_ledger,
    "p2-extension-decision-ledger-v8": validate_ext_ledger,
    "provider-runtime-matrix-v8": validate_provider_matrix,
}

def validate_file(path):
    try:
        data = load_json(path)
    except Exception as e:
        return [f"JSON error: {e}"], []
    sv = data.get("schema_version","")
    fn = VALIDATORS.get(sv)
    if fn is None: return [f"unknown schema_version: '{sv}'"], []
    result = fn(data, path)
    if isinstance(result, tuple) and len(result) == 2: return result
    return result, []

def validate_dir(d):
    r = type('R',(),{'files':0,'valid':0,'invalid':0,'errors':[],'warnings':[]})()
    for root,_,files in os.walk(d):
        for f in sorted(files):
            if f.endswith('.json'):
                path = os.path.join(root,f)
                r.files += 1
                errs, warns = validate_file(path)
                if errs: r.invalid += 1; [r.errors.append(f"{f}: {e}") for e in errs]
                else: r.valid += 1
                [r.warnings.append(f"{f}: {w}") for w in warns]
    return r

def self_test():
    base = os.path.dirname(os.path.abspath(__file__))
    root = os.path.join(base,"..")
    vd = os.path.join(root,"samples","valid")
    ivd = os.path.join(root,"samples","invalid")
    passed = failed = 0
    print("=== P2 Schema Gate Validator Self-Test ===\n")
    for f in sorted(os.listdir(vd)):
        if f.endswith('.json'):
            errs,_ = validate_file(os.path.join(vd,f))
            if not errs: print(f"  PASS: {f}"); passed += 1
            else: print(f"  FAIL: {f}"); [print(f"    -> {e}") for e in errs]; failed += 1
    for f in sorted(os.listdir(ivd)):
        if f.endswith('.json'):
            errs,_ = validate_file(os.path.join(ivd,f))
            if errs: print(f"  PASS (expected fail): {f}"); [print(f"    -> {e}") for e in errs]; passed += 1
            else: print(f"  FAIL (should fail but passed): {f}"); failed += 1
    t = passed + failed
    print(f"\n  Total: {t}, Passed: {passed}, Failed: {failed}")
    print(f"Classification: {'PASS' if failed==0 else 'FAIL'}")
    sys.exit(0 if failed==0 else 1)

def main():
    if "--self-test" in sys.argv: self_test(); return
    if len(sys.argv) < 2: print("Usage: schema_gate_validator.py <file|dir> [--self-test]"); sys.exit(1)
    t = sys.argv[1]
    if not os.path.exists(t): print(f"'{t}' not found"); sys.exit(1)
    if os.path.isfile(t):
        errs,_ = validate_file(t)
        if errs: print("Classification: FAIL"); [print(f"  FAIL: {e}") for e in errs]; sys.exit(1)
        else: print(f"Classification: RUNTIME_PASS\n  {os.path.basename(t)}: valid"); sys.exit(0)
    else:
        r = validate_dir(t)
        print(f"Classification: {'RUNTIME_PASS' if r.invalid==0 and r.files>0 else 'FAIL'}")
        print(f"  Files: {r.files}, Valid: {r.valid}, Invalid: {r.invalid}")
        [print(f"  FAIL: {e}") for e in r.errors]
        [print(f"  WARN: {w}") for w in r.warnings]
        sys.exit(0 if r.invalid==0 and r.files>0 else 1)

if __name__ == "__main__":
    main()
