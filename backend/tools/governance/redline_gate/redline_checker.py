#!/usr/bin/env python3
"""
P2 Redline Gate Checker
Scans evidence/reports/memory for redline violations.
8 rule categories. Schema Gate integration. Context-aware detection.
Python stdlib only.
"""
import json, os, sys, re, glob
from datetime import datetime, timezone

# ═══════════════════════════════════════════
# Constants
# ═══════════════════════════════════════════
MEDIA_KINDS = {"local_file","smb","nfs","ftp","ftps","sftp","http_file","https_file","http_stream","https_stream","dlna_upnp","s3_object","minio_object","webdav","alist","emby","plex","jellyfin","google_drive","onedrive","iptv_m3u","hls_live"}
EXTENSION_KINDS = {"cast_target","pip_surface","cinema_mode","trakt_identity","playlist_state","queue_state","resume_state","playback_history"}
MOCK_KINDS = {"mock","simulator","unit_test"}

CREDENTIAL_PATTERNS = [
    r'Authorization:', r'Bearer\s+[A-Za-z0-9_\-\.]+', r'Basic\s+[A-Za-z0-9+/=]+',
    r'api_key\s*[=:]\s*\S+', r'apikey\s*[=:]\s*\S+',
    r'access_token\s*[=:]\s*\S+', r'refresh_token\s*[=:]\s*\S+',
    r'id_token\s*[=:]\s*\S+', r'client_secret\s*[=:]\s*\S+',
    r'password\s*[=:]\s*\S+', r'passwd\s*[=:]\s*\S+',
    r'secret\s*=\s*\S+', r'token\s*=\s*\S+',
    r'cookie\s*[=:]\s*\S+', r'sessionid\s*[=:]\s*\S+', r'Set-Cookie', r'x-api-key\s*[=:]\s*\S+',
    r'C:\\Users\\', r'/Users/', r'/home/', r'\\\\NAS\\'
]

PREMATURE_CLAIMS = [
    'ALL PROVIDERS RUNTIME VERIFIED', 'P2 Foundation Complete', 'P2 Complete',
    'P3 COMPLETE', 'HDR COMPLETE', 'ATMOS BITSTREAM COMPLETE',
    'AIRPLAY PRODUCT COMPLETE', 'Atmos Complete'
]

CONTEXT_NEGATORS = ['NOT ', 'forbidden_claim', 'Forbidden claim', 'expected fail',
                    'invalid fixture', 'redline vocabulary', 'NOT All Providers',
                    'negative test']

CREDENTIAL_NEGATORS = ['redline vocabulary','invalid credential','expected fail','forbidden_claim','NOT ']

class Result:
    def __init__(self):
        self.violations = []
        self.files_scanned = 0
        self.schema_failures = []
    @property
    def passed(self): return len(self.violations) == 0 and len(self.schema_failures) == 0
    def add(self, rule, severity, filepath, field, msg):
        self.violations.append({"rule_id":rule,"severity":severity,"file":filepath,"field_path":field,"message":msg,"evidence_excerpt_redacted":"[redacted]"})
    def count_by_rule(self):
        d = {}
        for v in self.violations:
            r = v['rule_id']; d[r] = d.get(r,0) + 1
        return d

def load_json_safe(path):
    try:
        with open(path,'r',encoding='utf-8') as f: return json.load(f)
    except: return None

def is_in_self_test(path, root):
    return os.path.normpath(os.path.join(root,'backend/tools/governance/redline_gate/samples')) in os.path.normpath(path)

def is_negated_context(text):
    return any(n.lower() in text.lower() for n in CONTEXT_NEGATORS)

def is_credential_negated(text):
    return any(n.lower() in text.lower() for n in CREDENTIAL_NEGATORS)

# ═══════════════════════════════════════════
# Schema Gate Integration
# ═══════════════════════════════════════════
def run_schema_check_on_evidence(root):
    """Run schema_gate_validator on all evidence JSON files."""
    import importlib.util
    vpath = os.path.join(root,'backend/tools/governance/schema_gate/validator/schema_gate_validator.py')
    spec = importlib.util.spec_from_file_location("schema_gate", vpath)
    if spec is None:
        print("WARNING: cannot load schema_gate_validator")
        return [], 0
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)

    failures = []
    count = 0
    evidence_dir = os.path.join(root,'artifacts/p2/evidence')
    redline_samples = os.path.join(root,'backend/tools/governance/redline_gate/samples')
    for path in glob.glob(os.path.join(evidence_dir,'*.json')) + glob.glob(os.path.join(redline_samples,'**/*.json'), recursive=True):
        count += 1
        errs, _ = mod.validate_file(path)
        if errs:
            failures.append((path, errs))
    return failures, count

# ═══════════════════════════════════════════
# R1: Fake Pass
# ═══════════════════════════════════════════
def check_fake_pass(data, path, result):
    st = data.get('status','')
    rst = data.get('runtime_status','')
    if st == 'RUNTIME_PASS' or rst == 'RUNTIME_PASS':
        # Scan only string VALUES, not field names (anti_fake_checks is OK)
        values = [v for v in _extract_values(data) if isinstance(v, str)]
        raw = ' '.join(values).lower()
        for kw in ['stub','simulator','dummy','placeholder','soaksimulator']:
            if kw in raw and not is_negated_context(raw):
                result.add('R1','FAIL',path,'status',f"RUNTIME_PASS but evidence mentions '{kw}'")
                return

def _extract_values(obj):
    """Recursively extract all values from a nested dict/list."""
    if isinstance(obj, dict):
        for v in obj.values():
            yield from _extract_values(v)
    elif isinstance(obj, list):
        for v in obj:
            yield from _extract_values(v)
    else:
        yield obj

# ═══════════════════════════════════════════
# R2: Mock/Simulator as Runtime
# ═══════════════════════════════════════════
def check_mock_runtime(data, path, result):
    st = data.get('status','')
    rst = data.get('runtime_status','')
    rek = data.get('runtime_environment_kind','')
    if (st == 'RUNTIME_PASS' or rst == 'RUNTIME_PASS') and rek in MOCK_KINDS:
        result.add('R2','FAIL',path,'runtime_environment_kind',f"RUNTIME_PASS with {rek} not allowed")

# ═══════════════════════════════════════════
# R3: Provider Default All PASS
# ═══════════════════════════════════════════
def check_provider_matrix(data, path, result):
    ps = data.get('providers',[])
    if not isinstance(ps,list): return
    runtime_pass = [p for p in ps if p.get('runtime_status')=='RUNTIME_PASS']
    if len(runtime_pass) == len(ps) and len(ps) > 0:
        result.add('R3','FAIL',path,'providers','All providers RUNTIME_PASS')
    for p in runtime_pass:
        pk = p.get('provider_kind','')
        ev = p.get('evidence_file','')
        if not ev:
            result.add('R3','FAIL',path,f'{pk}.evidence_file','RUNTIME_PASS but evidence_file empty')
        else:
            evpath = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(path))))),ev)
            if not os.path.exists(evpath):
                result.add('R3','FAIL',path,f'{pk}.evidence_file',f"evidence_file '{ev}' not found")
        if pk != 'local_file' and pk not in ('','airplay_cast','trakt_api'):
            result.add('R3','FAIL',path,f'{pk}.runtime_status',f"non-local_file provider '{pk}' has RUNTIME_PASS without real env evidence")

# ═══════════════════════════════════════════
# R4: Credential Leakage
# ═══════════════════════════════════════════
def check_credential_leakage(path, result, is_fixture):
    if is_fixture: return
    if not path.endswith(('.json','.md','.txt')): return
    try:
        with open(path,'r',encoding='utf-8') as f: raw_text = f.read()
    except: return

    content = raw_text
    if path.endswith('.json'):
        data = load_json_safe(path)
        if data:
            values = [str(v) for v in _extract_values(data) if isinstance(v, str)]
            content = '\n'.join(values)

    for line in content.split('\n'):
        if is_credential_negated(line): continue
        for pat in CREDENTIAL_PATTERNS:
            m = re.search(pat, line, re.IGNORECASE)
            if m:
                result.add('R4','FAIL',path,'content',f"credential pattern matched: {m.group()[:40]}")
                return

# ═══════════════════════════════════════════
# R5: Media/Extension Confusion
# ═══════════════════════════════════════════
def check_media_extension_confusion(data, path, result):
    mi = data.get('media_input',{})
    if isinstance(mi, dict):
        mk = mi.get('kind','')
        if mk in EXTENSION_KINDS:
            result.add('R5','FAIL',path,'media_input.kind',f"'{mk}' is extension kind, not media source")
    ei = data.get('extension_input',{})
    if isinstance(ei, dict):
        ek = ei.get('kind','')
        if ek in MEDIA_KINDS:
            result.add('R5','FAIL',path,'extension_input.kind',f"'{ek}' is media kind, not extension")
    ek2 = data.get('extension_kind','')
    if ek2 in MEDIA_KINDS:
        result.add('R5','FAIL',path,'extension_kind',f"'{ek2}' is media kind, not extension")

# ═══════════════════════════════════════════
# R6: BLOCKED/SKIP as PASS
# ═══════════════════════════════════════════
def check_blocked_as_pass(data, path, result):
    st = data.get('status','')
    cl = data.get('completion_level','')
    if st == 'BLOCKED_ENV' and isinstance(cl,str) and cl.startswith('p2-foundation'):
        result.add('R6','FAIL',path,'status',f"BLOCKED_ENV not allowed as foundation completion")
    if st == 'SKIPPED_TEST_ONLY':
        result.add('R6','FAIL',path,'status','SKIPPED_TEST_ONLY cannot be PASS')

# ═══════════════════════════════════════════
# R7: JSON Structure - runtime_verified checks
# ═══════════════════════════════════════════
def check_json_structure(data, path, result):
    st = data.get('status','')
    rtv = data.get('runtime_verified')
    af = data.get('anti_fake_checks',{})
    sv = data.get('schema_validated')
    rv = data.get('redaction_verified')
    if st == 'RUNTIME_PASS':
        if rtv != True:
            result.add('R7','FAIL',path,'runtime_verified','RUNTIME_PASS requires runtime_verified=true')
        if isinstance(af,dict) and af.get('silent_fallback')==True:
            result.add('R7','FAIL',path,'anti_fake_checks','silent_fallback=true with RUNTIME_PASS')
        if sv == False:
            result.add('R7','FAIL',path,'schema_validated','schema_validated=false with RUNTIME_PASS')
        if rv == False:
            result.add('R7','FAIL',path,'redaction_verified','redaction_verified=false with RUNTIME_PASS')

# ═══════════════════════════════════════════
# R8: Premature Claims
# ═══════════════════════════════════════════
def check_premature_claims(path, result, is_fixture):
    if is_fixture: return
    if not path.endswith(('.md','.json')): return
    try:
        with open(path,'r',encoding='utf-8') as f: content = f.read()
    except: return
    for claim in PREMATURE_CLAIMS:
        idx = content.upper().find(claim.upper())
        if idx >= 0:
            # Check context around the match
            ctx = content[max(0,idx-30):idx+len(claim)+30]
            if is_negated_context(ctx): continue
            result.add('R8','FAIL',path,'content',f"premature claim: '{claim}'")

# ═══════════════════════════════════════════
# Dispatch
# ═══════════════════════════════════════════
def scan_json(path, result, is_fixture):
    data = load_json_safe(path)
    if data is None: return
    check_fake_pass(data, path, result)
    check_mock_runtime(data, path, result)
    if data.get('schema_version','') == 'provider-runtime-matrix-v8':
        check_provider_matrix(data, path, result)
    check_media_extension_confusion(data, path, result)
    check_blocked_as_pass(data, path, result)
    check_json_structure(data, path, result)

def scan_production(root, result):
    """Scan production evidence, reports, and memory."""
    scopes = {
        'evidence': os.path.join(root,'artifacts/p2/evidence'),
        'reports': os.path.join(root,'docs/roadmap'),
        'memory': os.path.join(root,'.codebuddy/memory'),
        'tools': os.path.join(root,'backend/tools/governance'),
    }
    for _, d in scopes.items():
        if not os.path.isdir(d): continue
        for fpath in glob.glob(os.path.join(d,'**/*'), recursive=True):
            if not os.path.isfile(fpath): continue
            if 'redline_gate' in fpath or 'schema_gate' in fpath: continue
            result.files_scanned += 1
            is_fixture = False
            if fpath.endswith('.json'):
                scan_json(fpath, result, is_fixture)
            check_credential_leakage(fpath, result, is_fixture)
            check_premature_claims(fpath, result, is_fixture)

# ═══════════════════════════════════════════
# Self-Test
# ═══════════════════════════════════════════
def self_test():
    base = os.path.dirname(os.path.abspath(__file__))
    vd = os.path.join(base,'samples','valid')
    ivd = os.path.join(base,'samples','invalid')
    passed = 0; failed = 0
    print("=== P2 Redline Gate Self-Test ===\n")
    for f in sorted(os.listdir(vd)):
        if f.endswith(('.json','.md')):
            path = os.path.join(vd,f); r = Result(); r.files_scanned = 1
            scan_json(path, r, True); check_premature_claims(path, r, False)
            if r.passed: print(f"  PASS: {f}"); passed += 1
            else: print(f"  FAIL: {f}"); [print(f"    -> {v['message']}") for v in r.violations]; failed += 1
    for f in sorted(os.listdir(ivd)):
        if f.endswith(('.json','.md')):
            path = os.path.join(ivd,f); r = Result(); r.files_scanned = 1
            scan_json(path, r, True)
            # cred/premature checks run with is_fixture=False so invalid fixtures ARE detected
            check_credential_leakage(path, r, False)
            check_premature_claims(path, r, False)
            if not r.passed: print(f"  PASS (expected fail): {f}"); [print(f"    -> [{v['rule_id']}] {v['message']}") for v in r.violations]; passed += 1
            else: print(f"  FAIL (should fail but passed): {f}"); failed += 1
    t = passed + failed
    print(f"\n  Total: {t}, Passed: {passed}, Failed: {failed}")
    print(f"Classification: {'PASS' if failed==0 else 'FAIL'}")
    sys.exit(0 if failed==0 else 1)

# ═══════════════════════════════════════════
# Main
# ═══════════════════════════════════════════
def main():
    if "--self-test" in sys.argv: self_test(); return
    root = "."
    for a in sys.argv[1:]:
        if a.startswith('--root='): root = a.split('=',1)[1]
    result = Result()

    # 1. Schema Gate check on evidence JSON only
    failures, scount = run_schema_check_on_evidence(root)
    result.files_scanned += scount
    result.schema_failures = failures
    if failures:
        result.add('SCHEMA','FAIL','evidence/','schema','Schema gate validation failures detected')

    # 2. Production redline scan
    scan_production(root, result)

    # 3. Output machine-readable result
    artifact_dir = os.path.join(root,'backend/tools/governance/redline_gate/artifacts')
    os.makedirs(artifact_dir, exist_ok=True)
    out = {
        "schema_version": "p2-redline-gate-result-v1",
        "task_id": "P2-REDLINE-GATE-FOUNDATION",
        "classification": "PASS" if result.passed else "FAIL",
        "files_scanned": result.files_scanned,
        "violations_count": len(result.violations),
        "violations_by_rule": result.count_by_rule(),
        "violations": result.violations,
        "schema_failures": [{"file":f,"errors":e} for f,e in result.schema_failures],
        "allowed_claims": [],
        "forbidden_claims": [],
        "timestamp_utc": datetime.now(timezone.utc).strftime('%Y-%m-%dT%H:%M:%SZ')
    }
    with open(os.path.join(artifact_dir,'redline_gate_result.json'),'w') as f:
        json.dump(out, f, indent=2)

    print(f"Classification: {out['classification']}")
    print(f"  Files scanned: {result.files_scanned}")
    print(f"  Violations: {len(result.violations)}")
    for v in result.violations:
        print(f"  [{v['rule_id']}] {v['file']}: {v['message']}")
    for sf, se in result.schema_failures:
        for e in se:
            print(f"  [SCHEMA] {sf}: {e}")
    sys.exit(0 if result.passed else 1)

if __name__ == "__main__":
    main()
