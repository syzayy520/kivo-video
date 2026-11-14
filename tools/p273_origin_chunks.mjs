// 复算 273 · 取数层（net-diag.log）真机证据：请求数 / 实际字节 / 跨度 / 表头延迟分位。
//
// 用法：node tools/p273_origin_chunks.mjs <net-diag.log>
//
// 【为什么不能假设块长】源站会在文件尾把块夹短（cues 块 854 195 684 起、
// 总长 854 260 052 ⇒ 只有 64 368 字节）。日志行带 `len=` 就是为了让统计
// 用**实际**块长，而不是"每块都是 chunkBytes"。
//
// 本机没有 Python 解释器，证据复算一律走 node（见 agents.md 工具限制）。

import { readFileSync } from 'node:fs';

const path = process.argv[2];
if (!path) {
  console.error('usage: node tools/p273_origin_chunks.mjs <net-diag.log>');
  process.exit(2);
}

const lines = readFileSync(path, 'utf8').split(/\r?\n/).filter(Boolean);

// 取数层每一块：起点 / 状态码 / 表头耗时 / 实际长度
const rows = [];
for (const line of lines) {
  const m = line.match(
    /^(\d+) \[(\w+)\] origin chunk from=(\d+) code=(\d+) hdrMs=(\d+)(?: len=(\d+))?/,
  );
  if (!m) continue;
  rows.push({
    t: Number(m[1]),
    tag: m[2],
    from: Number(m[3]),
    code: Number(m[4]),
    hdr: Number(m[5]),
    len: m[6] !== undefined ? Number(m[6]) : null,
  });
}

// 旧格式（没有 len=）时用相邻块的起点差反推块长；
// 跨读者切换处起点会跳变 ⇒ 那一步不可信，置 null（宁可不统计，也不编）。
for (let i = 0; i < rows.length; i++) {
  if (rows[i].len !== null) continue;
  const next = rows[i + 1];
  if (!next || next.tag !== rows[i].tag) {
    rows[i].len = null;
    continue;
  }
  const delta = next.from - rows[i].from;
  rows[i].len = delta > 0 && delta <= 64 * 1024 * 1024 ? delta : null;
}

const pct = (sorted, p) =>
  sorted.length ? sorted[Math.min(sorted.length - 1, Math.floor(sorted.length * p))] : 0;

for (const tag of [...new Set(rows.map((r) => r.tag))]) {
  const rs = rows.filter((r) => r.tag === tag);
  const known = rs.filter((r) => r.len !== null);
  const bytes = known.reduce((sum, r) => sum + r.len, 0);
  const span = rs.length ? rs[rs.length - 1].from - rs[0].from : 0;
  const wall = rs.length ? (rs[rs.length - 1].t - rs[0].t) / 1000 : 0;
  const hdrs = rs.map((r) => r.hdr).sort((a, b) => a - b);
  console.log(`[${tag}]`);
  console.log(`  请求数        : ${rs.length}`);
  console.log(`  实际字节      : ${(bytes / 1048576).toFixed(1)} MiB（${known.length}/${rs.length} 块有确切长度）`);
  console.log(`  跨度          : ${(span / 1048576).toFixed(1)} MiB`);
  console.log(`  墙钟          : ${wall.toFixed(1)} s`);
  console.log(`  表头延迟(ms)  : min=${pct(hdrs, 0)} p50=${pct(hdrs, 0.5)} p99=${pct(hdrs, 0.99)} max=${hdrs[hdrs.length - 1]}`);
  console.log(`  非 206 的块   : ${rs.filter((r) => r.code !== 206).length}`);
  console.log(`  起点序列      : ${rs.map((r) => r.from).slice(0, 5).join(', ')} … ${rs.map((r) => r.from).slice(-3).join(', ')}`);
}

// 取数层之外的行（代理失败、到片尾等）也要看：它们常常是真凶。
const other = lines.filter((l) => !/origin chunk/.test(l));
if (other.length) {
  console.log('--- 其它 net-diag 行 ---');
  other.forEach((l) => console.log('  ' + l));
}
