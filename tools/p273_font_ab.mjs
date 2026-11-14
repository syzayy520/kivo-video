// 复算 273 · v20 字幕字体集最小化的真机证据。
//
// 用法：
//   node tools/p273_font_ab.mjs <dirA> [dirB]
//   （每个 dir 是逐秒 meminfo + kernel.log 的目录，例如
//    docs/evidence/p273-emby-stutter/v23-base）
//
// 输出：每个目录的
//   · `Loading font file` 条数与文件清单
//   · 逐秒原生堆 / TOTAL PSS 峰值，以及峰值所在相对秒
//   · 字体匹配结果（fontselect）与 OSD 纹理分配
// 两个目录都给时，额外打印 A→B 的差值。
//
// 【为什么要有这个工具】"起播期原生堆 +192 MB"这个结论，
// 要能从**逐秒 meminfo**里复算出峰值，而不是抄某个数字；
// 字体条数要从**内核日志**里数，不能凭印象。
//
// 本机没有 Python 解释器，证据复算一律走 node（见 agents.md 工具限制）。

import { readFileSync, readdirSync, existsSync } from 'node:fs';
import { join } from 'node:path';

const dirs = process.argv.slice(2).filter(Boolean);
if (dirs.length === 0) {
  console.error('usage: node tools/p273_font_ab.mjs <dirA> [dirB]');
  process.exit(2);
}

function readMeminfo(path) {
  const text = readFileSync(path, 'utf8');
  const native = text.match(/^\s*Native Heap\s+(\d+)\s+(\d+)/m);
  const total = text.match(/^\s*TOTAL\s+(\d+)\s+(\d+)/m);
  const ttf = text.match(/^\s*\.ttf mmap\s+(\d+)/m);
  if (!native || !total) return null;
  return {
    nativePss: Number(native[1]),
    nativeDirty: Number(native[2]),
    totalPss: Number(total[1]),
    ttfPss: ttf ? Number(ttf[1]) : 0,
  };
}

function summarize(dir) {
  const out = { dir, samples: [], fonts: [], fontselect: [], osd: [] };
  const memFiles = readdirSync(dir)
    .filter((f) => /^mi-\d+\.txt$/.test(f))
    .sort((a, b) => Number(a.slice(3, -4)) - Number(b.slice(3, -4)));
  memFiles.forEach((f, i) => {
    const m = readMeminfo(join(dir, f));
    if (m) out.samples.push({ sec: i + 1, ...m });
  });
  const logPath = join(dir, 'kernel.log');
  if (existsSync(logPath)) {
    const lines = readFileSync(logPath, 'utf8').split(/\r?\n/);
    for (const line of lines) {
      if (/Loading font file/.test(line)) out.fonts.push(line.trim());
      if (/fontselect:/.test(line)) out.fontselect.push(line.trim());
      if (/Reallocating OSD texture/.test(line)) out.osd.push(line.trim());
    }
  }
  const peakNative = out.samples.reduce((a, s) => (s.nativePss > a.nativePss ? s : a), { nativePss: -1, sec: 0 });
  const peakTotal = out.samples.reduce((a, s) => (s.totalPss > a.totalPss ? s : a), { totalPss: -1, sec: 0 });
  out.peakNative = peakNative;
  out.peakTotal = peakTotal;
  return out;
}

const results = dirs.map(summarize);

for (const r of results) {
  console.log(`=== ${r.dir} ===`);
  console.log(`  ${'Loading font file 条数'.padEnd(24)}: ${r.fonts.length}`);
  r.fonts.slice(0, 6).forEach((f) => console.log(`      ${f.replace(/^\[[^\]]*\]\[[^\]]*\]\[[^\]]*\]\s*/, '')}`));
  if (r.fonts.length > 6) console.log(`      …… 其余 ${r.fonts.length - 6} 条`);
  console.log(`  ${'原生堆峰值 PSS'.padEnd(24)}: ${r.peakNative.nativePss} KB（mi-${r.peakNative.sec}.txt）`);
  console.log(`  ${'原生堆峰值 dirty'.padEnd(24)}: ${r.peakNative.nativeDirty} KB`);
  console.log(`  ${'TOTAL PSS 峰值'.padEnd(24)}: ${r.peakTotal.totalPss} KB（mi-${r.peakTotal.sec}.txt）`);
  if (r.fontselect.length) {
    console.log(`  ${'fontselect'.padEnd(24)}: ${r.fontselect[0].replace(/^\[[^\]]*\]\[[^\]]*\]\[[^\]]*\]\s*/, '')}`);
  }
  r.osd.slice(0, 4).forEach((o, i) => console.log(`  ${('OSD 纹理 ' + (i + 1)).padEnd(24)}: ${o.replace(/^\[[^\]]*\]\[[^\]]*\]\[[^\]]*\]\s*/, '')}`));
  console.log(`  ${'逐秒原生堆 PSS'.padEnd(24)}: ${r.samples.map((s) => s.nativePss).join(' ')}`);
  console.log(`  ${'逐秒 TOTAL PSS'.padEnd(24)}: ${r.samples.map((s) => s.totalPss).join(' ')}`);
  console.log('');
}

if (results.length >= 2) {
  const [a, b] = results;
  console.log('=== A → B 差值 ===');
  console.log(`  字体条数        : ${a.fonts.length} → ${b.fonts.length}`);
  console.log(`  原生堆峰值 PSS  : ${a.peakNative.nativePss} → ${b.peakNative.nativePss} KB（${b.peakNative.nativePss - a.peakNative.nativePss} KB）`);
  console.log(`  TOTAL PSS 峰值  : ${a.peakTotal.totalPss} → ${b.peakTotal.totalPss} KB（${b.peakTotal.totalPss - a.peakTotal.totalPss} KB）`);
}
