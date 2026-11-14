// 274 · 起播时间线合并器：把 net-diag 的**墙上时钟**与 mpv 内核日志的**相对时钟**
// 对齐到同一条时间轴上，输出"从 am start 到首帧"的每一段等待归属。
//
// 【为什么需要】起播慢的归因必须能说清"这几秒在等谁"：
//   · Activity/Compose 起来（主机侧，无内核日志）
//   · 源站握手与表头（net-diag 的 `origin probe/chunk`）
//   · 内核 open / seek / 建解码器（内核日志）
// 两份日志的时钟不同源，不合并就只能靠猜。
//
// 用法：node tools/p274_startup_timeline.mjs <net.log> <kernel.log> [T0_ms]
//   T0_ms = `am start` 的墙上时刻（毫秒）；不给则只输出相对首帧的偏移。
import { readFileSync } from 'node:fs';

const [netPath, kernelPath, t0Arg] = process.argv.slice(2);
if (!netPath || !kernelPath) {
  console.error('usage: node tools/p274_startup_timeline.mjs <net.log> <kernel.log> [T0_ms]');
  process.exit(2);
}

const netLines = readFileSync(netPath, 'utf8').split(/\r?\n/).filter(Boolean);
const kernelLines = readFileSync(kernelPath, 'utf8').split(/\r?\n/);

// 对齐锚点：net 的「MPV 首帧就绪」= 内核的「first video frame ... shown」。
const firstFrameNet = netLines.find((l) => /MPV 首帧就绪/.test(l));
const firstFrameKernel = kernelLines.find((l) => /first video frame after restart shown/.test(l));
if (!firstFrameNet || !firstFrameKernel) {
  console.error('找不到对齐锚点（net: MPV 首帧就绪 / kernel: first video frame ... shown）');
  process.exit(3);
}
const netMs = Number(firstFrameNet.split(' ')[0]);
const kernelSec = Number((firstFrameKernel.match(/^\[\s*([\d.]+)\]/) || [])[1]);
const kernelZeroMs = netMs - kernelSec * 1000;
const t0 = t0Arg ? Number(t0Arg) : kernelZeroMs;

const events = [];

for (const line of netLines) {
  const m = line.match(/^(\d+) (.*)$/);
  if (!m) continue;
  const ms = Number(m[1]);
  const body = m[2];
  if (/origin chunk|origin probe|\[prep\]|PROXY REQ|首帧就绪|\[tail\]|origin 到达片源末尾/.test(body)) {
    events.push({ ms, src: 'net', body });
  }
}

for (const line of kernelLines) {
  const m = line.match(/^\[\s*([\d.]+)\]\[(\w)\]\[([\w/]+)\]\s*(.*)$/);
  if (!m) continue;
  const ms = kernelZeroMs + Number(m[1]) * 1000;
  const body = m[4];
  const tag = m[3];
  if (/tcp: (Starting|Successfully)|Opening done|All headers|Detected file format|Starting playback|queuing seek|stream level seek|seek done|first video frame|Enter buffering|Using hardware decoding|MediaCodec started|Video  --vid|Opening decoder|audio driver/.test(body)) {
    events.push({ ms, src: 'mpv', body: `${tag}: ${body}` });
  }
}

events.sort((a, b) => a.ms - b.ms);
const base = t0;
let prev = base;
console.log(`T0=${base}  内核零点=${kernelZeroMs}  (out=${netPath})`);
console.log('  相对       段耗时    来源  事件');
for (const e of events) {
  const rel = e.ms - base;
  const delta = e.ms - prev;
  prev = e.ms;
  console.log(
    `${(rel / 1000).toFixed(2).padStart(8)}s ${(delta / 1000).toFixed(2).padStart(8)}s  ${e.src.padEnd(4)} ${e.body.slice(0, 150)}`,
  );
}
