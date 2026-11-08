"""Read-only reproduction of the archived K40 artwork component checks."""
import json
from pathlib import Path
import xml.etree.ElementTree as ET
from PIL import Image

ROOT = Path(__file__).resolve().parents[3]
OUT = ROOT / 'docs/evidence/p240/detail-artwork'


def verify():
    before, after = [{r['case']: r for r in json.loads((OUT / label / 'report.json').read_text(encoding='utf-8'))}
                     for label in ('before', 'after')]
    checks = {}
    for name in ('successful-logo', 'no-logo-short'):
        checks[name + '-geometry-preserved'] = before[name]['geometry'] == after[name]['geometry']
    for name in after:
        g = after[name]['geometry']
        checks[name + '-metadata-below-title'] = g['meta'][1] >= g['title'][3]
    for name in ('no-logo-long', 'large-long'):
        def height(row):
            r = row['geometry']['title']
            return r[3] - r[1]
        checks[name + '-grows'] = height(after[name]) > height(before[name])
    for name in ('failed-logo', 'no-logo-long', 'large-long'):
        row = after[name]
        checks[name + '-full-title-node'] = any(n['text'] == row['fixture_title'] for n in row['nodes'])
    for name in ('missing-art', 'failed-art'):
        picture = Image.open(OUT / 'after' / (name + '.png')).convert('RGB')
        # Screenshot evidence: pale field and actual dark message ink in the empty media area.
        checks[name + '-pale-field'] = min(picture.getpixel((30, 300))) > 220
        ink = sum(max(pixel) < 180 for pixel in picture.crop((350, 540, 730, 620)).getdata())
        checks[name + '-message-ink'] = ink > 100
    old = ET.parse(ROOT / 'docs/evidence/p240/detail-layout/android/after-restored-top.xml')
    new = ET.parse(OUT / 'android/real-detail-top.xml')
    for label in ('生化危机：爆发夜', '2026年9月16日 · 美国/德国/法国 · 1小时34分钟',
                  '▶  播放', '影片标记', '收藏', '更多操作', '展开简介'):
        def bounds(tree):
            return [n.get('bounds') for n in tree.iter('node')
                    if label in (n.get('text'), n.get('content-desc'))]
        a, b = bounds(old), bounds(new)
        checks['real-' + label + '-geometry'] = bool(a) and a == b
    return checks


if __name__ == '__main__':
    checks = verify()
    result = {'checks': checks, 'passed': sum(checks.values()), 'total': len(checks)}
    print(json.dumps(result, ensure_ascii=False, indent=2))
    raise SystemExit(0 if all(checks.values()) else 1)
