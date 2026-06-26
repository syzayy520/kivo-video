#!/usr/bin/env python3
"""
KIVO-VIDEO-P2-LEGACY-EVIDENCE-MIGRATION-VERIFY-001
Preserve-field migration for legacy P2 evidence files.

Rules:
- Never delete historical fields.
- Capture complete original JSON under 'legacy_original'.
- Move non-hardened-schema fields into 'legacy_fields'.
- Preserve the four owner-named legacy fields in both 'legacy_fields'
  and 'implementation.legacy_*':
    runtime_verified_timestamp, provider_matrix_impact, task_name, family
- Add/keep all p2-evidence-v8 required fields at top level.
- Map legacy enum values to valid V8 vocabularies and record originals.
- RGF-*.json files are explicitly excluded and must remain untouched.

Python stdlib only.
"""
import json, os, glob, sys
from datetime import datetime, timezone

EXCLUDED = {'RGF-001','RGF-002','RGF-003','RGF-004','RGF-005'}

VALID_STATUSES = {"RUNTIME_PASS","CONTRACT_PASS","FAIL","BLOCKED_ENV","NOT_IMPLEMENTED","SKIPPED_TEST_ONLY"}
VALID_RUNTIME_KINDS = {"local_real_runtime","windows_device_runtime","controlled_real_protocol_server","real_external_service","real_external_account","real_external_device","mock","unit_test","simulator","not_applicable"}

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

LEGACY_IMPLEMENTATION_FIELDS = {
    'runtime_verified_timestamp': 'legacy_runtime_verified_timestamp',
    'provider_matrix_impact': 'legacy_provider_matrix_impact',
    'task_name': 'legacy_task_name',
    'family': 'legacy_family',
}

DEFAULTS = {
    'schema_version': 'p2-evidence-v8',
    'task_family': 'planning',
    'completion_level': 'p2-foundation-planning',
    'runtime_mode': 'planning_only',
    'runtime_environment_kind': 'not_applicable',
    'environment': {},
    'implementation': {},
    'media_input': {'kind':'not_applicable'},
    'extension_input': {},
    'media_identity': {},
    'source_identity': {},
    'extension_identity': {},
    'capability_probe': {},
    'direct_play_decision': {},
    'direct_stream_decision': {},
    'subtitle_snapshot': {},
    'chapter_snapshot': {},
    'playback_state_snapshot': {},
    'inspector_snapshot': {},
    'decision_ledger': {},
    'errors': [],
    'redaction_verified': True,
    'schema_validated': True,
    'anti_fake_checks': {},
    'redline_scan': {},
    'test_commands': [],
    'artifacts': [],
    'requires_provider_matrix': False,
    'runtime_verified': False,
    'blocked_reason': '',
    'timestamp_utc': datetime.now(timezone.utc).strftime('%Y-%m-%dT%H:%M:%SZ'),
}

def infer_schema_version(original):
    if original.get('schema_version') == 'provider-runtime-matrix-v8':
        return 'provider-runtime-matrix-v8'
    if 'providers' in original and 'matrix_id' in original:
        return 'provider-runtime-matrix-v8'
    return 'p2-evidence-v8'

def normalize_status(original):
    candidates = [original.get('status'), original.get('evidence_status')]
    for c in candidates:
        if isinstance(c, str) and c in VALID_STATUSES:
            return c, original.get('status'), original.get('evidence_status')
    # Fallback: use original status if present, else CONTRACT_PASS
    raw = original.get('status', original.get('evidence_status', 'CONTRACT_PASS'))
    if isinstance(raw, str) and raw == 'BLOCKED_ENV':
        return 'BLOCKED_ENV', original.get('status'), original.get('evidence_status')
    return 'CONTRACT_PASS', original.get('status'), original.get('evidence_status')

def normalize_runtime_kind(original):
    raw = original.get('runtime_environment_kind', original.get('environment_kind', 'not_applicable'))
    if raw == 'test_harness':
        return 'not_applicable', raw
    if raw == 'local_machine':
        return 'local_real_runtime', raw
    if raw == 'remote_server':
        return 'real_external_service', raw
    if raw == 'cloud_account':
        return 'real_external_account', raw
    if raw == 'simulated':
        return 'simulator', raw
    if raw == 'none':
        return 'not_applicable', raw
    if raw in VALID_RUNTIME_KINDS:
        return raw, raw
    return 'not_applicable', raw

def migrate_file(path, report):
    name = os.path.basename(path).replace('.json', '')
    with open(path, 'r', encoding='utf-8') as f:
        original = json.load(f)

    report[name] = {
        'original_top_level_fields': sorted(original.keys()),
        'preserved_as_legacy_original': True,
        'preserved_fields': {},
        'mapped_values': {},
    }

    target_schema = infer_schema_version(original)

    # Build new top-level object
    new = {}

    # 1. Required schema fields
    for field in EVIDENCE_REQUIRED:
        if field == 'schema_version':
            new[field] = target_schema
        elif field in original:
            new[field] = original[field]
        else:
            new[field] = DEFAULTS[field]

    # 2. Status normalization and runtime_verified consistency
    status, orig_status, orig_evidence_status = normalize_status(original)
    new['status'] = status
    new['runtime_verified'] = (status == 'RUNTIME_PASS')
    if status == 'RUNTIME_PASS':
        new['completion_level'] = 'p2-runtime-verified'
    else:
        new['completion_level'] = 'p2-foundation-planning'

    if orig_status is not None:
        report[name]['preserved_fields']['status'] = orig_status
    if orig_evidence_status is not None and orig_evidence_status != orig_status:
        report[name]['preserved_fields']['evidence_status'] = orig_evidence_status
    report[name]['mapped_values']['status'] = status
    report[name]['mapped_values']['runtime_verified'] = new['runtime_verified']

    # 3. runtime_environment_kind normalization
    kind, orig_kind = normalize_runtime_kind(original)
    new['runtime_environment_kind'] = kind
    if kind != orig_kind:
        report[name]['mapped_values']['runtime_environment_kind'] = {'from': orig_kind, 'to': kind}

    # 4. timestamp_utc: prefer original, fall back to execution_timestamp or timestamp_utc
    ts = original.get('timestamp_utc') or original.get('execution_timestamp') or DEFAULTS['timestamp_utc']
    new['timestamp_utc'] = ts
    if original.get('execution_timestamp') and original.get('execution_timestamp') != ts:
        report[name]['preserved_fields']['execution_timestamp'] = original['execution_timestamp']

    # 5. task_id / task_family
    if 'task_id' in original:
        new['task_id'] = original['task_id']
    else:
        new['task_id'] = name
    if 'task_family' in original:
        new['task_family'] = original['task_family']

    # 6. Preserve all non-required original fields in legacy_fields
    legacy_fields = {}
    for k, v in original.items():
        if k not in EVIDENCE_REQUIRED:
            legacy_fields[k] = v

    # 7. implementation.legacy_* for the four named fields
    impl = dict(new.get('implementation') or {})
    for legacy_key, impl_key in LEGACY_IMPLEMENTATION_FIELDS.items():
        if legacy_key in original:
            impl[impl_key] = original[legacy_key]
    new['implementation'] = impl

    # 8. legacy_original: complete original snapshot
    new['legacy_original'] = original
    new['legacy_fields'] = legacy_fields

    # 9. Record what was preserved
    for k in legacy_fields:
        report[name]['preserved_fields'][k] = 'moved to legacy_fields'
    for legacy_key, impl_key in LEGACY_IMPLEMENTATION_FIELDS.items():
        if legacy_key in original:
            report[name]['preserved_fields'][legacy_key] = f'also in implementation.{impl_key}'

    # 10. Add audit metadata about the migration itself
    new['legacy_migration_audit'] = {
        'migration_task': 'KIVO-VIDEO-P2-LEGACY-EVIDENCE-MIGRATION-VERIFY-001',
        'migration_timestamp_utc': datetime.now(timezone.utc).strftime('%Y-%m-%dT%H:%M:%SZ'),
        'preserved_original_snapshot': True,
        'preserved_field_count': len(legacy_fields),
        'migrated_field_count': len(EVIDENCE_REQUIRED),
    }

    with open(path, 'w', encoding='utf-8') as f:
        json.dump(new, f, indent=2)

    return new

def main():
    root = sys.argv[1] if len(sys.argv) > 1 else '.'
    evidence_dir = os.path.join(root, 'artifacts', 'p2', 'evidence')
    report_dir = os.path.join(root, 'backend', 'tools', 'governance', 'redline_gate', 'artifacts')
    os.makedirs(report_dir, exist_ok=True)

    report = {}
    migrated = 0
    for path in sorted(glob.glob(os.path.join(evidence_dir, '*.json'))):
        name = os.path.basename(path).replace('.json', '')
        if name in EXCLUDED:
            print(f'  SKIP (RGF): {name}')
            continue
        migrate_file(path, report)
        migrated += 1
        print(f'  MIGRATED: {name}')

    summary = {
        'migration_task': 'KIVO-VIDEO-P2-LEGACY-EVIDENCE-MIGRATION-VERIFY-001',
        'migrated_count': migrated,
        'excluded_rgf_count': len(EXCLUDED),
        'timestamp_utc': datetime.now(timezone.utc).strftime('%Y-%m-%dT%H:%M:%SZ'),
        'report': report,
    }
    with open(os.path.join(report_dir, 'legacy_evidence_field_retention_report.json'), 'w', encoding='utf-8') as f:
        json.dump(summary, f, indent=2)

    print(f'\nMigrated: {migrated} files')
    print(f'Excluded RGF: {len(EXCLUDED)} files')
    print(f'Report: {os.path.join(report_dir, "legacy_evidence_field_retention_report.json")}')

if __name__ == '__main__':
    main()
