#!/usr/bin/env python3
"""
P2 Redline Gate Checker V3 — Production-grade
8 rule categories. True scan mode separation. No fake PASS.
Python stdlib only.
"""
import json, os, sys, re, glob, importlib.util
from datetime import datetime, timezone

MEDIA_KINDS = {"local_file","smb","nfs","ftp","ftps","sftp","http_file","https_file","http_stream","https_stream","dlna_upnp","s3_object","minio_object","webdav","alist","emby","plex","jellyfin","google_drive","onedrive","iptv_m3u","hls_live"}
EXTENSION_KINDS = {"cast_target","pip_surface","cinema_mode","trakt_identity","playlist_state","queue_state","resume_state","playback_history"}
MOCK_KINDS = {"mock","simulator","unit_test"}
CREDENTIAL_PATTERNS = [
    r'Authorization:\s*Bearer\s+\S+', r'Authorization:\s*Basic\s+\S+',
    r'api_key\s*[=:]\s*[\w\-]{8,}', r'apikey\s*[=:]\s*[\w\-]{8,}',
    r'access_token\s*[=:]\s*\S+', r'refresh_token\s*[=:]\s*\S+',
    r'id_token\s*[=:]\s*\S+', r'client_secret\s*[=:]\s*\S+',
    r'secret\s*=\s*[\w\-]{8,}', r'token\s*=\s*[\w\-]{8,}',
    r'sessionid\s*[=:]\s*\S+', r'Set-Cookie:', r'x-api-key\s*[=:]\s*\S+',
    r'C:\\Users\\', r'/Users/', r'/home/', r'\\\\NAS\\'
]
PREMATURE_CLAIMS = ['ALL PROVIDERS RUNTIME VERIFIED','P2 Foundation Complete','P2 Complete','P3 COMPLETE','HDR COMPLETE','ATMOS BITSTREAM COMPLETE','AIRPLAY PRODUCT COMPLETE','Atmos Complete']
CONTEXT_NEGATORS = ['NOT ','forbidden_claim','Forbidden claim','expected fail','invalid fixture','redline vocabulary','NOT All Providers','negative test']

def load_json_safe(path):
    try:
        with open(path,'r',encoding='utf-8') as f: return json.load(f)
    except: return None

def _extract_values(obj):
    if isinstance(obj,dict):
        for v in obj.values(): yield from _extract_values(v)
    elif isinstance(obj,list):
        for v in obj: yield from _extract_values(v)
    else: yield obj

class Result:
    def __init__(self): self.violations = []; self.files_scanned = 0; self.schema_failures = []
    def add(self,rule,severity,filepath,field,msg):
        self.violations.append({"rule_id":rule,"severity":severity,"file":filepath,"field_path":field,"message":msg,"evidence_excerpt_redacted":"[redacted]"})
    @property
    def passed(self): return len(self.violations)==0 and len(self.schema_failures)==0
    def count_by_rule(self):
        d={}; [d.update({v['rule_id']:d.get(v['rule_id'],0)+1}) for v in self.violations]; return d

def is_negated_context(text):
    return any(n.lower() in text.lower() for n in CONTEXT_NEGATORS)

# ═══ Schema Gate ═══
def run_schema_check(root):
    vpath = os.path.join(root,'backend/tools/governance/schema_gate/validator/schema_gate_validator.py')
    if not os.path.exists(vpath): return [],0
    spec = importlib.util.spec_from_file_location("sg",vpath)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    failures = []; count = 0
    for path in glob.glob(os.path.join(root,'artifacts/p2/evidence/*.json')):
        count += 1
        errs,_ = mod.validate_file(path)
        if errs: failures.append((path,errs))
    return failures, count

# ═══ R1 ═══
def check_fake_pass(data, path, r):
    st = data.get('status',''); rst = data.get('runtime_status','')
    if st=='RUNTIME_PASS' or rst=='RUNTIME_PASS':
        vals = [str(v) for v in _extract_values(data) if isinstance(v,str)]
        raw = ' '.join(vals).lower()
        for kw in ['stub','simulator','dummy','placeholder','soaksimulator']:
            if kw in raw and not is_negated_context(raw):
                r.add('R1','FAIL',path,'status',f"RUNTIME_PASS mentions '{kw}'"); return

# ═══ R2 ═══
def check_mock_runtime(data, path, r):
    st=data.get('status',''); rst=data.get('runtime_status',''); rek=data.get('runtime_environment_kind','')
    if (st=='RUNTIME_PASS' or rst=='RUNTIME_PASS') and rek in MOCK_KINDS:
        r.add('R2','FAIL',path,'runtime_environment_kind',f'RUNTIME_PASS with {rek}')

# ═══ R3 ═══
def check_provider_matrix(data, path, r, root):
    ps = data.get('providers',[])
    if not isinstance(ps,list): return
    rp = [p for p in ps if p.get('runtime_status')=='RUNTIME_PASS']
    if len(rp)==len(ps) and len(ps)>1: r.add('R3','FAIL',path,'providers','All providers RUNTIME_PASS')
    for p in rp:
        pk=p.get('provider_kind',''); ev=p.get('evidence_file','')
        if not ev: r.add('R3','FAIL',path,f'{pk}.evidence_file','RUNTIME_PASS but evidence_file empty')
        else:
            evpath = os.path.join(root, ev)
            if not os.path.exists(evpath): r.add('R3','FAIL',path,f'{pk}.evidence_file',f"evidence_file '{ev}' not found")
        if pk!='local_file' and pk not in ('','airplay_cast','trakt_api'):
            r.add('R3','FAIL',path,f'{pk}.runtime_status',f"non-local_file provider '{pk}' RUNTIME_PASS without real env evidence")

# ═══ R4 ═══
def check_credential_leakage(path, r, is_production):
    if not is_production or not path.endswith(('.json','.md','.txt')): return
    try:
        with open(path,'r',encoding='utf-8') as f: raw = f.read()
    except: return
    content = raw
    if path.endswith('.json'):
        data = load_json_safe(path)
        if data: content = '\n'.join(str(v) for v in _extract_values(data) if isinstance(v,str))
    for line in content.split('\n'):
        for pat in CREDENTIAL_PATTERNS:
            m = re.search(pat, line, re.IGNORECASE)
            if m:
                ctx = line[max(0,m.start()-20):m.end()+20]
                if is_negated_context(ctx): continue
                # Only flag Basic when it's clearly Authorization header
                if 'basic' in pat.lower() and 'authorization' not in line.lower(): continue
                r.add('R4','FAIL',path,'content',f"credential matched: {m.group()[:40]}"); return

# ═══ R5 ═══
def check_media_extension_confusion(data, path, r):
    mi=(data.get('media_input') or {}).get('kind','')
    if mi in EXTENSION_KINDS: r.add('R5','FAIL',path,'media_input.kind',f"'{mi}' is extension, not media")
    ei=(data.get('extension_input') or {}).get('kind','')
    if ei in MEDIA_KINDS: r.add('R5','FAIL',path,'extension_input.kind',f"'{ei}' is media, not extension")
    ek2=data.get('extension_kind','')
    if ek2 in MEDIA_KINDS: r.add('R5','FAIL',path,'extension_kind',f"'{ek2}' is media, not extension")

# ═══ R6 ═══
def check_blocked_as_pass(data, path, r):
    st=data.get('status',''); cl=data.get('completion_level','')
    if st=='BLOCKED_ENV' and isinstance(cl,str) and cl.startswith('p2-foundation'):
        r.add('R6','FAIL',path,'status','BLOCKED_ENV not allowed as foundation completion')
    if st=='SKIPPED_TEST_ONLY': r.add('R6','FAIL',path,'status','SKIPPED_TEST_ONLY cannot be PASS')

# ═══ R7 ═══
def check_json_structure(data, path, r):
    st=data.get('status',''); af=data.get('anti_fake_checks',{})
    if st=='RUNTIME_PASS':
        if data.get('runtime_verified')!=True: r.add('R7','FAIL',path,'runtime_verified','RUNTIME_PASS needs runtime_verified=true')
        if isinstance(af,dict) and af.get('silent_fallback')==True: r.add('R7','FAIL',path,'anti_fake_checks','silent_fallback=true with RUNTIME_PASS')
        if data.get('schema_validated')==False: r.add('R7','FAIL',path,'schema_validated','schema_validated=false with RUNTIME_PASS')
        if data.get('redaction_verified')==False: r.add('R7','FAIL',path,'redaction_verified','redaction_verified=false with RUNTIME_PASS')

# ═══ R8 ═══
def check_premature_claims(path, r, is_production):
    if not is_production or not path.endswith(('.md','.json')): return
    try:
        with open(path,'r',encoding='utf-8') as f: content=f.read()
    except: return
    for claim in PREMATURE_CLAIMS:
        idx=content.upper().find(claim.upper())
        if idx>=0:
            ctx=content[max(0,idx-30):idx+len(claim)+30]
            if is_negated_context(ctx): continue
            r.add('R8','FAIL',path,'content',f"premature claim: '{claim}'")

# ═══ JSON scanner ═══
def scan_json(path, r, root, is_production):
    data = load_json_safe(path)
    if data is None: return
    check_fake_pass(data, path, r)
    check_mock_runtime(data, path, r)
    if data.get('schema_version','')=='provider-runtime-matrix-v8': check_provider_matrix(data, path, r, root)
    check_media_extension_confusion(data, path, r)
    check_blocked_as_pass(data, path, r)
    check_json_structure(data, path, r)

# ═══ True scan mode separation ═══
def scan_evidence(root, r):
    """Only artifacts/p2/evidence/"""
    for path in glob.glob(os.path.join(root,'artifacts/p2/evidence/*.json')):
        r.files_scanned += 1
        scan_json(path, r, root, True)
        check_credential_leakage(path, r, True)
        check_premature_claims(path, r, True)

def scan_reports(root, r):
    """Only docs/roadmap/*.md + .codebuddy/memory/*.md"""
    for pattern in ['docs/roadmap/*.md','.codebuddy/memory/*.md']:
        for path in glob.glob(os.path.join(root,pattern)):
            r.files_scanned += 1
            check_credential_leakage(path, r, True)
            check_premature_claims(path, r, True)

def scan_tools(root, r):
    """Governance tools, excluding fixtures"""
    for path in glob.glob(os.path.join(root,'backend/tools/governance/**/*'), recursive=True):
        if 'redline_gate/samples' in path or 'schema_gate/samples' in path: continue
        if 'redline_gate/artifacts' in path: continue
        if not os.path.isfile(path): continue
        r.files_scanned += 1
        if path.endswith('.json'): scan_json(path, r, root, True)
        check_credential_leakage(path, r, True)
        check_premature_claims(path, r, True)

def scan_all(root, r):
    scan_evidence(root, r)
    scan_reports(root, r)
    scan_tools(root, r)

# ═══ Legacy evidence inventory ═══
def generate_legacy_inventory(root):
    inventory_path = os.path.join(root,'backend/tools/governance/redline_gate/artifacts/legacy_evidence_violations.json')
    os.makedirs(os.path.dirname(inventory_path), exist_ok=True)
    r = Result()
    vpath = os.path.join(root,'backend/tools/governance/schema_gate/validator/schema_gate_validator.py')
    if os.path.exists(vpath):
        spec = importlib.util.spec_from_file_location("sg",vpath)
        mod = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(mod)
        entries = []
        for path in sorted(glob.glob(os.path.join(root,'artifacts/p2/evidence/*.json'))):
            entry = {"file": os.path.relpath(path, root), "schema_errors": [], "redline_violations": []}
            data = load_json_safe(path)
            if data:
                errs, _ = mod.validate_file(path)
                entry["schema_errors"] = errs
                r2 = Result()
                scan_json(path, r2, root, True)
                check_credential_leakage(path, r2, True)
                check_premature_claims(path, r2, True)
                entry["redline_violations"] = [{"rule": v["rule_id"], "message": v["message"]} for v in r2.violations]
            entries.append(entry)
        with open(inventory_path,'w') as f:
            json.dump({"legacy_evidence_count":len(entries),"entries":entries}, f, indent=2)
    return inventory_path

# ═══ Self-Test ═══
def self_test():
    base = os.path.dirname(os.path.abspath(__file__))
    vd=os.path.join(base,'samples','valid'); ivd=os.path.join(base,'samples','invalid')
    passed=0; failed=0; root=base
    print("=== P2 Redline Gate Self-Test ===\n")
    for f in sorted(os.listdir(vd)):
        if f.endswith(('.json','.md')):
            p=os.path.join(vd,f); r=Result(); r.files_scanned=1
            scan_json(p,r,root,True); check_premature_claims(p,r,False)
            if r.passed: print(f"  PASS: {f}"); passed+=1
            else: print(f"  FAIL: {f}"); [print(f"    -> {v['message']}") for v in r.violations]; failed+=1
    for f in sorted(os.listdir(ivd)):
        if f.endswith(('.json','.md')):
            p=os.path.join(ivd,f); r=Result(); r.files_scanned=1
            scan_json(p,r,root,True)
            # self-test: run cred/premature as production=True so fixtures ARE checked
            check_credential_leakage(p,r,True)
            check_premature_claims(p,r,True)
            if not r.passed: print(f"  PASS (expected fail): {f}"); [print(f"    -> [{v['rule_id']}] {v['message']}") for v in r.violations]; passed+=1
            else: print(f"  FAIL (should fail): {f}"); failed+=1
    t=passed+failed
    print(f"\n  Total: {t}, Passed: {passed}, Failed: {failed}")
    print(f"Classification: {'PASS' if failed==0 else 'FAIL'}")
    return failed==0

# ═══ Main ═══
def main():
    if "--self-test" in sys.argv: sys.exit(0 if self_test() else 1)
    root="."; scan_mode="all"
    for a in sys.argv[1:]:
        if a.startswith('--root='): root=a.split('=',1)[1]
        if a.startswith('--scan='): scan_mode=a.split('=',1)[1]
    r=Result()

    # Schema check on real evidence
    failures, scount = run_schema_check(root)
    r.files_scanned += scount
    for p,errs in failures: r.schema_failures.append((p,errs))
    if failures: r.add('SCHEMA','FAIL','evidence/','schema','Schema gate failures on legacy evidence')

    # Scan per mode
    if scan_mode=='evidence': scan_evidence(root, r)
    elif scan_mode=='reports': scan_reports(root, r)
    elif scan_mode=='tools': scan_tools(root, r)
    else: scan_all(root, r)  # 'all'

    # Legacy inventory
    generate_legacy_inventory(root)

    # Output result
    ad = os.path.join(root,'backend/tools/governance/redline_gate/artifacts')
    os.makedirs(ad, exist_ok=True)
    out = {
        "schema_version":"p2-redline-gate-result-v1",
        "task_id":"P2-REDLINE-GATE-FOUNDATION",
        "scan_mode":scan_mode,
        "classification":"PASS" if r.passed else "FAIL",
        "files_scanned":r.files_scanned,
        "violations_count":len(r.violations),
        "violations_by_rule":r.count_by_rule(),
        "violations":r.violations,
        "schema_failures":[{"file":f,"errors":e} for f,e in r.schema_failures],
        "legacy_inventory":"backend/tools/governance/redline_gate/artifacts/legacy_evidence_violations.json",
        "allowed_claims":[],"forbidden_claims":[],
        "timestamp_utc":datetime.now(timezone.utc).strftime('%Y-%m-%dT%H:%M:%SZ')
    }
    with open(os.path.join(ad,'redline_gate_result.json'),'w') as f: json.dump(out, f, indent=2)
    print(f"Classification: {out['classification']}")
    print(f"  Scan mode: {scan_mode}")
    print(f"  Files: {r.files_scanned}  Violations: {len(r.violations)}")
    for v in r.violations: print(f"  [{v['rule_id']}] {v['file']}: {v['message']}")
    for sf,se in r.schema_failures:
        for e in se: print(f"  [SCHEMA] {sf}: {e}")
    sys.exit(0 if r.passed else 1)

if __name__ == "__main__": main()
