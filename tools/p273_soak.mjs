// 复算 273 · 长稳 + 拖动会话的真机证据。
//
// 用法：node tools/p273_soak.mjs <dir>
//
// <dir> 里应该有：mi-*.txt（逐秒 meminfo）、kernel.log、net.log、thumb.log、
// af-*.txt（dumpsys media.audio_flinger 快照）。
//
// 输出：
//   · 逐秒原生堆 / TOTAL PSS 曲线与峰值（以及**尾段是否还在爬**）
//   · 内核健康度：缓冲、欠载、seek 次数、丢帧
//   · 取数侧：分块字节分类、mpv 缓存状态行
//   · 音频：设备侧音轨表的 Underruns 与混音器原始欠载计数
//
// 【为什么要有这个工具】"长稳有没有泄漏""拖动之后有没有断音"这两件事，
// 都不能靠看一张 meminfo 就下结论 —— 要把**逐秒曲线**和**设备侧独立计数**
// 摆在一起，让"没涨"和"没断"都能被别人复算。
//
// 本机没有 Python 解释器，证据复算一律走 node（见 agents.md 工具限制）。

import { readFileSync, readdirSync, existsSync } from 'node:fs';
import { join } from 'node:path';

const dir = process.argv[2];
if (!dir) {
  console.error('usage: node tools/p273_soak.mjs <dir>');
  process.exit(2);
}

const read = (p) => (existsSync(p) ? readFileSync(p, 'utf8').split(/\r?\n/) : []);

console.log('=== 逐秒内存 ===');
const samples = [];
const memFiles = readdirSync(dir).filter((f) => /^mi-\d+\.txt$/.test(f))
  .sort((a, b) => Number(a.slice(3, -4)) - Number(b.slice(3, -4)));
memFiles.forEach((f) => {
  const t = readFileSync(join(dir, f), 'utf8');
  const nh = t.match(/^\s*Native Heap\s+(\d+)/m);
  const tp = t.match(/^\s*TOTAL\s+(\d+)/m);
  const eg = t.match(/^\s*EGL mtrack\s+(\d+)/m);
  samples.push({
    sec: Number(f.slice(3, -4)),
    native: nh ? Number(nh[1]) : null,
    total: tp ? Number(tp[1]) : null,
    egl: eg ? Number(eg[1]) : null,
  });
});
const peak = (key) => samples.reduce((a, s) => (s[key] > (a[key] ?? -1) ? s : a), {});
const pn = peak('native'), pt = peak('total'), pe = peak('egl');
console.log(`  采样点         ${samples.length}`);
console.log(`  原生堆峰值     ${pn.native} KB（第 ${pn.sec} s）`);
console.log(`  TOTAL PSS 峰值 ${pt.total} KB（第 ${pt.sec} s）`);
console.log(`  EGL mtrack 峰值 ${pe.egl} KB（第 ${pe.sec} s）`);
if (samples.length >= 10) {
  const k = Math.max(2, Math.round(samples.length / 5));
  const head = samples.slice(0, k).reduce((a, s) => a + s.native, 0) / k;
  const tail = samples.slice(-k).reduce((a, s) => a + s.native, 0) / k;
  console.log(`  原生堆 前 ${k} 点均值 ${head.toFixed(0)} KB → 末 ${k} 点均值 ${tail.toFixed(0)} KB（漂移 ${(tail - head).toFixed(0)} KB）`);
}
console.log(`  原生堆序列   ${samples.map((s) => s.native).join(' ')}`);
console.log(`  TOTAL PSS 序列 ${samples.map((s) => s.total).join(' ')}`);

console.log('');
console.log('=== 内核健康度 ===');
const kernel = read(join(dir, 'kernel.log'));
const cnt = (re) => kernel.filter((l) => re.test(l)).length;
console.log(`  Enter buffering   ${cnt(/Enter buffering/)}`);
console.log(`  End buffering      ${cnt(/End buffering/)}`);
console.log(`  音频欠载(内核自报) ${cnt(/Audio device underrun detected/)}`);
console.log(`  内核 seek 次数     ${cnt(/stream level seek/)}`);
console.log(`  EOF reached        ${cnt(/EOF reached/i)}`);
console.log(`  Loading font file  ${cnt(/Loading font file/)}`);
const seekLines = kernel.filter((l) => /stream level seek/.test(l)).map((l) => l.trim());
seekLines.forEach((l) => console.log(`    ${l}`));

console.log('');
console.log('=== 取数侧 ===');
const net = read(join(dir, 'net.log'));
const byTag = {};
for (const l of net) {
  const m = l.match(/^(\d+) \[([\w-]+)\] origin chunk from=(\d+) code=(\d+) hdrMs=(\d+)(?: len=(\d+))?/);
  if (!m) continue;
  const len = m[6] ? Number(m[6]) : 0;
  byTag[m[2]] = (byTag[m[2]] || 0) + len;
}
Object.entries(byTag).sort((a, b) => b[1] - a[1]).forEach(([k, v]) => {
  console.log(`  ${k.padEnd(12)} ${(v / 1048576).toFixed(2)} MiB`);
});
net.filter((l) => /\[cache\]/.test(l)).slice(-3).forEach((l) => console.log(`  ${l.slice(l.indexOf('[cache]'))}`));

// 【为什么要有窗口口径】各轮会话长短不一，总量不可比 ⇒ 按**日志时间轴**积分：
// 取最后一分钟窗口的下载速率，与会话总速率并列（后者含起播的窗口预取）。
const stamps = net.map((l) => l.match(/^(\d{13})\s/)).filter(Boolean).map((m) => Number(m[1]));
if (stamps.length) {
  const t0 = Math.min(...stamps), t1 = Math.max(...stamps);
  const winMs = 60_000;
  const inWindow = (from) => {
    let sum = 0;
    for (const l of net) {
      const m = l.match(/^(\d{13})\s.*?\[([\w-]+)\] origin chunk .*?len=(\d+)/);
      if (!m) continue;
      if (Number(m[1]) < t1 - from || Number(m[1]) > t1) continue;
      sum += Number(m[3]);
    }
    return sum;
  };
  const last60 = inWindow(winMs);
  const all = inWindow(Infinity);
  console.log(`  会话时长 ${((t1 - t0) / 1000).toFixed(1)} s · 总下载 ${(all / 1048576).toFixed(2)} MiB` +
    ` · 末 60 s ${(last60 / 1048576).toFixed(2)} MiB（${(last60 / 1024 / 60).toFixed(0)} KB/s）`);
}

console.log('');
console.log('=== 音频（设备侧独立计数）===');
for (const f of readdirSync(dir).filter((x) => /^af-.*\.txt$/.test(x)).sort()) {
  const lines = read(join(dir, f));
  const mixer = lines.filter((l) => /Normal mixer raw underrun counters/.test(l)).slice(0, 1)[0] || '(无)';
  // 音轨表：表头里带 "Client Session Port Id"，其后每行是一轨。
  // ⚠️ 行首有缩进、Type 列（F1/S/…）可能为空 ⇒ 不能用 ^\S+ 锚定；
  // ⚠️ 同一份 dump 里有**两张同头表**（输出线程 + 客户端），活动轨在后者。
  // ⚠️ Latency 列形如 "429.07 t"（两词）或 "new"（一词）⇒ 末列不能固定按
  // 倒数第 N 列取，要先判 Latency 占几个词。
  let track = '';
  let trackFrm = -1;
  for (let i = 0; i < lines.length; i++) {
    if (!/Client Session Port Id/.test(lines[i])) continue;
    for (let j = i + 1; j < lines.length; j++) {
      const t = lines[j].trim();
      if (!t) break;
      const m = t.match(/^(?:[A-Za-z]\S*\s+)?(\d+)\s+(yes|no)\s+/);
      if (!m) continue;
      if (m[2] !== 'yes') continue;
      // 活动轨可能不止一条（别的应用）⇒ 取 FrmCnt 最大的那条。
      // FrmCnt 是无 0x 前缀的 8 位十六进制，位置在表尾附近 ⇒ 取最后一个匹配。
      const frms = t.match(/\b([0-9A-F]{8})\b/g) || [];
      const frmVal = frms.length ? parseInt(frms[frms.length - 1], 16) : 0;
      if (frmVal > trackFrm) { trackFrm = frmVal; track = t; }
    }
  }
  const tok = track ? track.split(/\s+/) : [];
  const last = tok[tok.length - 1] || '';
  const prev = tok[tok.length - 2] || '';
  const latencyIsTwoWords = /^\d+(\.\d+)?$/.test(prev) && /^[a-z]{1,3}$/.test(last);
  const under = tok.length >= 3 ? tok[latencyIsTwoWords ? tok.length - 4 : tok.length - 3] : '?';
  const flushed = tok.length >= 2 ? tok[latencyIsTwoWords ? tok.length - 3 : tok.length - 2] : '?';
  console.log(`  ${f.padEnd(18)} Underruns=${under} Flushed=${flushed}`);
  console.log(`      混音器 ${mixer.trim()}`);
  if (track) console.log(`      音轨   ${track}`);
}
