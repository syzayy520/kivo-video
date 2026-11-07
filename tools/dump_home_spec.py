#!/usr/bin/env python3
"""238：首页 strings 全量导出（键名完整版）。"""
import json
import os

BASE = r'docs/reference/rex-teardown/rex-package/strings/'
MODS = ['AppShell', 'HomeBuiltIn', 'HomeEditor', 'Home', 'AppHome', 'AppWatching']

out = []
out.append('# 238 · 首页复刻规格（拆解参数全量整理 · 键名完整版）\n')
out.append('> 来源：strings/ 507 键全量导出（en + zh-Hans 对照）+ 动效参数表 M04 + 复刻说明书 §5/§6.1。')
out.append('> 本文档是首页实现的唯一依据；实现与本文档不一致 = 缺陷。\n')

total = 0
for m in MODS:
    en = json.load(open(BASE + 'en__' + m + '.json', encoding='utf-8'))
    zh_path = BASE + 'zh-Hans__' + m + '.json'
    zh = (json.load(open(zh_path, encoding='utf-8'))
          if os.path.exists(zh_path) else {})
    out.append('## ' + m + '（' + str(len(en)) + ' 键）\n')
    for k in sorted(en.keys()):
        v_en = en[k]
        v_zh = zh.get(k, '')
        line = '- [' + k + '] = ' + str(v_en)
        if v_zh and v_zh != v_en:
            line += ' ｜ ' + str(v_zh)
        out.append(line)
        total += 1
    out.append('')

doc = '\n'.join(out)
io.open(r'docs/design/backend-core/238-home-spec.md', 'w', encoding='utf-8').write(doc)
print('total keys:', total, 'lines:', doc.count('\n'))
