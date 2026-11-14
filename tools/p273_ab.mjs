// p273 v25：demux 缓存 A/B 复算 —— 把四个臂的内存、缓存状态与下载量摆成一张表。
//
// 【为什么要有这个工具】A/B 是"同一台设备、同一份片源、四个臂"的对照；
// 手抄数字会抄错、也会看不出"哪个臂省了多少" ⇒ 口径写死在脚本里，谁都能复算。
//
// 【口径】
//   · 原生堆峰值 / TOTAL PSS 峰值：逐秒 dumpsys meminfo 的最大值。
//   · demux 缓存：mpv 的 demuxer-cache-state（total / dur），取会话末尾三行。
//   · 下载量：net.log 里每条 "origin chunk ... len=N" 的 N 之和（**声明长度**，
//     不是实测字节数 —— 源站在片尾会把块夹短，这个口径对四个臂一致）。
//   · 缓冲事件：mpv 日志里的 Enter buffering 次数（0 = 全程没断过）。
//
// 本机没有 Python 解释器，证据复算一律走 node（见 agents.md 工具限制）。

import { readFileSync, readdirSync, existsSync } from 'node:fs';
import { join } from 'node:path';

const root = process.argv[2];
if (!root) {
  console.error('usage: node tools/p273_ab.mjs <dir>');
  process.exit(2);
}

const read = (p) => (existsSync(p) ? readFileSync(p, 'utf8').split(/\r?\n/) : []);
const arms = readdirSync(root).filter((d) => /^v\d\d-/.test(d) && existsSync(join(root, d, 'net.log')));

const rows = arms.map((arm) => {
  const dir = join(root, arm);
  const memFiles = readdirSync(dir).filter((f) => /^mi-\d+\.txt$/.test(f))
    .sort((a, b) => Number(a.slice(3, -4)) - Number(b.slice(3, -4)));
  let nativePeak = 0;
  let pssPeak = 0;
  let eglPeak = 0;
  for (const f of memFiles) {
    const t = readFileSync(join(dir, f), 'utf8');
    const nh = t.match(/^\s*Native Heap\s+(\d+)/m);
    const tp = t.match(/^\s*TOTAL\s+(\d+)/m);
    const eg = t.match(/^\s*EGL mtrack\s+(\d+)/m);
    if (nh) nativePeak = Math.max(nativePeak, Number(nh[1]));
    if (tp) pssPeak = Math.max(pssPeak, Number(tp[1]));
    if (eg) eglPeak = Math.max(eglPeak, Number(eg[1]));
  }

  const net = read(join(dir, 'net.log'));
  const byTag = {};
  let download = 0;
  for (const l of net) {
    const m = l.match(/^(\d+)\s\[([\w-]+)\] origin chunk from=(\d+) code=(\d+) hdrMs=(\d+)(?: len=(\d+))?/);
    if (!m) continue;
    const len = m[6] ? Number(m[6]) : 0;
    download += len;
    byTag[m[2]] = (byTag[m[2]] || 0) + len;
  }

  const cacheLines = net.filter((l) => /\[cache\]/.test(l));
  const last = cacheLines.length ? cacheLines[cacheLines.length - 1] : '';
  const cm = last.match(/fw=(\S+) total=(\S+) dur=(\S+) underrun=(\S+)/);

  const kernel = read(join(dir, 'kernel.log')).concat(read(join(dir, 'kernel-diag.log')));
  const buffering = kernel.filter((l) => /Enter buffering/.test(l)).length;

  // 会话时长（net.log 的时间轴）：用来算平均速率。
  const stamps = net.map((l) => l.match(/^(\d{13})\s/)).filter(Boolean).map((m) => Number(m[1]));
  const span = stamps.length ? (Math.max(...stamps) - Math.min(...stamps)) / 1000 : 0;

  return {
    arm,
    samples: memFiles.length,
    nativePeak,
    pssPeak,
    eglPeak,
    download,
    span,
    buffering,
    cache: cm ? { fw: cm[1], total: cm[2], dur: cm[3], underrun: cm[4] } : null,
    byTag,
  };
});

console.log('=== 臂对比（原生堆/PSS 单位 KB，下载单位 MiB）===');
const head = ['臂', '采样', '原生堆峰值', 'TOTAL PSS 峰值', 'EGL 峰值', '下载 MiB', '会话 s', '缓冲事件', 'cache total', 'cache dur'];
console.log(head.join(' | '));
for (const r of rows) {
  console.log([
    r.arm,
    r.samples,
    r.nativePeak,
    r.pssPeak,
    r.eglPeak,
    (r.download / 1048576).toFixed(2),
    r.span.toFixed(1),
    r.buffering,
    r.cache ? r.cache.total : '?',
    r.cache ? r.cache.dur : '?',
  ].join(' | '));
}

console.log('');
console.log('=== 分标签下载（MiB）===');
const tags = new Set();
for (const r of rows) Object.keys(r.byTag).forEach((t) => tags.add(t));
for (const r of rows) {
  const parts = [...tags].map((t) => `${t}=${((r.byTag[t] || 0) / 1048576).toFixed(2)}`);
  console.log(`  ${r.arm.padEnd(12)} ${parts.join('  ')}`);
}

console.log('');
console.log('=== 会话末尾的 demux 缓存状态 ===');
for (const r of rows) {
  const c = cacheLinesOf(r);
  if (c) console.log(`  ${r.arm.padEnd(12)} ${c}`);
}
function cacheLinesOf(r) {
  const net = read(join(root, r.arm, 'net.log'));
  const cl = net.filter((l) => /\[cache\]/.test(l));
  return cl.length ? cl[cl.length - 1].slice(cl[cl.length - 1].indexOf('[cache]')) : '';
}
