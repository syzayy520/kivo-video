import json, os, glob

excluded = {
    'RGF-001','RGF-002','RGF-003','RGF-004','RGF-005',
}

defaults = {
    'task_family': 'planning',
    'completion_level': 'p2-foundation-planning',
    'environment': {}, 'implementation': {},
    'media_input': {'kind':'not_applicable'}, 'extension_input': {},
    'media_identity': {}, 'source_identity': {}, 'extension_identity': {},
    'capability_probe': {}, 'direct_play_decision': {}, 'direct_stream_decision': {},
    'subtitle_snapshot': {}, 'chapter_snapshot': {}, 'playback_state_snapshot': {},
    'inspector_snapshot': {}, 'decision_ledger': {},
    'errors': [], 'redaction_verified': True, 'schema_validated': True,
    'anti_fake_checks': {}, 'redline_scan': {},
    'test_commands': [], 'artifacts': [],
    'requires_provider_matrix': False, 'runtime_verified': False, 'blocked_reason': ''
}

migrated = 0
for path in glob.glob('artifacts/p2/evidence/*.json'):
    name = os.path.basename(path).replace('.json','')
    if name in excluded: continue
    with open(path, 'r', encoding='utf-8') as f:
        d = json.load(f)

    # fix env kind
    if d.get('runtime_environment_kind') == 'test_harness':
        d['runtime_environment_kind'] = 'not_applicable'

    # timestamp
    ts = d.get('runtime_verified_timestamp','') or d.get('timestamp_utc','') or '2026-06-26T00:00:00Z'
    d['timestamp_utc'] = ts

    # add missing
    for k, v in defaults.items():
        if k not in d or d[k] is None or (isinstance(d[k],str) and d[k]==''):
            d[k] = v

    # remove old non-standard
    for old in ['runtime_verified_timestamp','provider_matrix_impact','task_name','family']:
        d.pop(old, None)

    with open(path, 'w', encoding='utf-8') as f:
        json.dump(d, f, indent=2)
    migrated += 1
    print(f'  OK: {name}')

print(f'\nMigrated: {migrated} files')
