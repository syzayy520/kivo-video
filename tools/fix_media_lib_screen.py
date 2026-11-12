#!/usr/bin/env python3
"""237-C 清理工具：媒体库旧 Column 残段移除 + 排序状态接线（一次性）。"""
import io

P = r'app/src/main/java/com/kivo/cinema/library/MediaLibraryActivity.kt'
s = io.open(P, encoding='utf-8').read()

start_marker = (
    '        Column(\n'
    '            modifier = Modifier\n'
    '                .fillMaxSize()\n'
    '                .padding(12.dp)\n'
    '        ) {\n'
    '            Row(verticalAlignment = Alignment.CenterVertically) {\n'
    '                Spacer(Modifier.weight(1f))'
)
end_marker = '        // 231：长按标记菜单'
start = s.find(start_marker)
end = s.find(end_marker)
assert start > 0 and end > start, (start, end)
s = s[:start] + s[end:]

old_field = '    private var menuFor by mutableStateOf<MediaItem?>(null)'
s = s.replace(old_field, old_field + '\n    private var sortDesc by mutableStateOf(true)', 1)

old_order = '"${MS.Video.Media.DATE_ADDED} DESC"'
new_order = '"${MS.Video.Media.DATE_ADDED} " + if (sortDesc) "DESC" else "ASC"'
assert old_order in s, 'sort key not found'
s = s.replace(old_order, new_order, 1)

io.open(P, 'w', encoding='utf-8').write(s)
print('cleaned; sortDesc count =', s.count('sortDesc'))
