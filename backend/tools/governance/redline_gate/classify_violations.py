#!/usr/bin/env python3
"""
KIVO-VIDEO-P2-LEGACY-EVIDENCE-MIGRATION-VERIFY-001
Violation classification helper.

Reads redline_gate_result.json and classifies each violation as either:
- real_violation_requires_remediation
- false_positive_negated_context
- false_positive_example_fixture

Does NOT modify source files.
"""
import json, os, re, sys

RESULT_PATH = 'backend/tools/governance/redline_gate/artifacts/redline_gate_result.json'
REPORT_PATH = 'backend/tools/governance/redline_gate/artifacts/violation_classification_report.json'

NEGATION_PATTERNS = [
    r'\bNOT\b', r'\bnever\b', r'\bno\s+', r'\bforbidden\b', r'\bdo\s+not\b',
    r'\bdo\s+NOT\b', r'\bNOT\s+claim', r'\bNOT\s+allowed', r'\bnegative\s+test',
    r'\binvalid\s+fixture', r'\bexpected\s+fail', r'\bredline\s+vocabulary',
    r'\bfalse\s+claim', r'\bpremature\s+claim', r'\bforbidden_claim',
    r'"forbidden_claims"', r'\bforbidden\s+terms', r'\bforbidden\s+scope',
]

EXAMPLE_PATTERNS = [
    r'example', r'fixture', r'sample', r'placeholder', r'`.*`',
]

def classify_context(text, rule):
    low = text.lower()
    # R4 credential paths in examples/fixtures/redaction docs are false positives
    if rule == 'R4':
        if (re.search(r'example|fixture|sample|redact|privacy|leakage|no_full_path|user_profile|redacted', low) or
            re.search(r'C:\\\\Users\\\\.*example', text, re.IGNORECASE) or
            re.search(r'/Users/(example|<user>|redacted|test)', text, re.IGNORECASE) or
            'ya29' in text):
            return 'false_positive_example_fixture'
    # Strong negation indicators
    for p in NEGATION_PATTERNS:
        if re.search(p, text, re.IGNORECASE):
            return 'false_positive_negated_context'
    return 'real_violation_requires_remediation'

def main():
    root = sys.argv[1] if len(sys.argv) > 1 else '.'
    result_path = os.path.join(root, RESULT_PATH)
    report_path = os.path.join(root, REPORT_PATH)

    with open(result_path, 'r', encoding='utf-8') as f:
        result = json.load(f)

    classified = []
    summary = {'real_violation_requires_remediation':0, 'false_positive_negated_context':0, 'false_positive_example_fixture':0, 'by_rule':{}}

    for v in result.get('violations', []):
        filepath = v['file']
        rel = os.path.relpath(filepath, root)
        msg = v['message']
        rule = v['rule_id']

        # Extract claim text from message
        if rule == 'R4' and msg.startswith('credential matched:'):
            claim = msg.split(':', 1)[1].strip()
        else:
            claim_match = re.search(r"'([^']+)'", msg)
            claim = claim_match.group(1) if claim_match else msg

        ctx = ''
        if os.path.exists(filepath):
            try:
                with open(filepath, 'r', encoding='utf-8') as f:
                    content = f.read()
                idx = content.upper().find(claim.upper())
                if idx >= 0:
                    ctx = content[max(0, idx-150):idx+len(claim)+150]
                else:
                    ctx = content[:200]
            except Exception as e:
                ctx = f'[error reading file: {e}]'
        else:
            ctx = '[file not found]'

        classification = classify_context(ctx, rule)
        summary[classification] += 1
        summary['by_rule'][rule] = summary['by_rule'].get(rule, 0) + 1

        classified.append({
            'rule_id': rule,
            'file': rel,
            'message': msg,
            'claim': claim,
            'classification': classification,
            'context': ctx.replace('\n',' '),
        })

    report = {
        'migration_task': 'KIVO-VIDEO-P2-LEGACY-EVIDENCE-MIGRATION-VERIFY-001',
        'source_scan_result': result_path,
        'total_violations': len(classified),
        'summary': summary,
        'real_violation_count': summary['real_violation_requires_remediation'],
        'false_positive_count': summary['false_positive_negated_context'] + summary['false_positive_example_fixture'],
        'classifications': classified,
        'timestamp_utc': result.get('timestamp_utc'),
    }

    os.makedirs(os.path.dirname(report_path), exist_ok=True)
    with open(report_path, 'w', encoding='utf-8') as f:
        json.dump(report, f, indent=2)

    print(f'Total violations: {report["total_violations"]}')
    print(f'Real violations: {report["real_violation_count"]}')
    print(f'False positives: {report["false_positive_count"]}')
    print(f'Report: {report_path}')

if __name__ == '__main__':
    main()
