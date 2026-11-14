// 复算 273 · Emby 卡顿真机证据：从 mpv 内核日志与渲染诊断日志里重算判据。
// 用法：node tools/p273_gaps.mjs <mpv-kernel.log> <mpv-kernel.log.diag>
// 本机没有 Python 解释器，证据复算一律走 node（见 agents.md 工具限制）。

import { readFileSync } from 'node:fs';

const [kernelPath, diagPath] = process.argv.slice(2);
if (!kernelPath || !diagPath) {
  console.error('usage: node tools/p273_gaps.mjs <mpv-kernel.log> <mpv-kernel.log.diag>');
  process.exit(2);
}

const kernel = readFileSync(kernelPath, 'utf8').split(/\r?\n/);
const diag = readFileSync(diagPath, 'utf8').split(/\r?\n/);

const count = (re) => kernel.filter((l) => re.test(l)).length;

const bufferingIn = count(/Enter buffering/);
const bufferingOut = count(/End buffering/);
const underrun = count(/Audio device underrun detected/);
const opening = kernel.find((l) => /Opening done/.test(l)) || '';
const restart = kernel.find((l) => /playback restart complete/.test(l)) || '';
const ts = (line) => {
  const m = line.match(/^\[\s*([\d.]+)\]/);
  return m ? Number(m[1]) : NaN;
};

// 渲染间隔：只取最后一段连续运行（diag 是追加写的，上一次运行的数据还留在文件头）
let start = 0;
for (let i = diag.length - 1; i >= 0; i--) {
  if (/^STEP \+0ms enter-initialize/.test(diag[i])) { start = i; break; }
}
const rnd = diag.slice(start)
  .map((l) => l.match(/^RND \+(\d+)ms swap #\d+ ok=(\d+)/))
  .filter(Boolean)
  .map((m) => ({ t: Number(m[1]), ok: Number(m[2]) }));

const gaps = [];
for (let i = 1; i < rnd.length; i++) gaps.push(rnd[i].t - rnd[i - 1].t);
const over = (ms) => gaps.filter((g) => g > ms);
const maxGap = gaps.length ? Math.max(...gaps) : 0;
const sorted = [...gaps].sort((a, b) => a - b);
const p99 = sorted.length ? sorted[Math.floor(sorted.length * 0.99)] : 0;
const span = rnd.length ? (rnd[rnd.length - 1].t - rnd[0].t) / 1000 : 0;
const fps = span > 0 ? (rnd.length - 1) / span : 0;

const report = {
  'Enter buffering 次数': bufferingIn,
  'End buffering 次数': bufferingOut,
  'Audio device underrun 次数': underrun,
  'Opening done 时刻(s)': ts(opening),
  'playback restart 时刻(s)': ts(restart),
  '渲染帧数': rnd.length,
  '渲染时长(s)': Number(span.toFixed(1)),
  '实际帧率(fps)': Number(fps.toFixed(2)),
  '渲染间隔 >200ms 次数': over(200).length,
  '渲染间隔最大(ms)': maxGap,
  '渲染间隔 p99(ms)': p99,
  '渲染失败帧数': rnd.filter((r) => r.ok === 0).length,
};

for (const [k, v] of Object.entries(report)) console.log(`${k}: ${v}`);
