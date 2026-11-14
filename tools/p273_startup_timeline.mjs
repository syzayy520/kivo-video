// 复算 273 · 起播关键路径（mpv-kernel.log + net-diag.log）真机证据。
//
// 用法：node tools/p273_startup_timeline.mjs <mpv-kernel.log> <net-diag.log> [tapEpochMs]
//
// 【tapEpochMs】主机侧"点播放"的绝对时刻（epoch ms）—— 有了它才能算
// **端到端**起播耗时（点播放 → 内核首帧）。缺了就只给内核侧相对时间。
//
// 【为什么要有这个工具】"起播慢"是一个**跨两层**的现象：
// 内核侧（mpv 的相对时间）与取数侧（net-diag 的绝对时间）各说一半，
// 只看其中一层就会得出相反的结论。这里把两层的关键事件并排摆出来，
// 让人能直接看出"哪一跳在等谁"。
//
// 本机没有 Python 解释器，证据复算一律走 node（见 agents.md 工具限制）。

import { readFileSync } from 'node:fs';

const kernelPath = process.argv[2];
const netPath = process.argv[3];
const tapEpoch = process.argv[4] ? Number(process.argv[4]) : 0;
if (!kernelPath || !netPath) {
  console.error('usage: node tools/p273_startup_timeline.mjs <mpv-kernel.log> <net-diag.log> [tapEpochMs]');
  process.exit(2);
}

const kernel = readFileSync(kernelPath, 'utf8').split(/\r?\n/);
const net = readFileSync(netPath, 'utf8').split(/\r?\n/).filter(Boolean);

// ---- 内核侧：相对时间（mpv 进程自己的时钟） ----
const stamp = (line) => {
  const m = line.match(/^\[\s*(\d+\.\d+)\]/);
  return m ? Number(m[1]) : null;
};
const firstEvent = (pattern) => {
  for (const line of kernel) {
    if (!pattern.test(line)) continue;
    const t = stamp(line);
    if (t !== null) return { t, line: line.trim() };
  }
  return null;
};

const show = (name, ev) => {
  if (!ev) {
    console.log(`  ${name.padEnd(20)}: （无）`);
    return;
  }
  console.log(`  ${name.padEnd(20)}: ${ev.t.toFixed(3)} s   ${ev.line.slice(0, 110)}`);
};

console.log('=== 内核侧关键事件（相对 mpv 进程时钟） ===');
const opening = firstEvent(/Opening done:/);
const queuing = firstEvent(/queuing seek/);
const seekDone = firstEvent(/seek done/);
const firstFrame = firstEvent(/first video frame/);
show('Opening done', opening);
show('queuing seek', queuing);
show('seek done', seekDone);
show('first video frame', firstFrame);
if (opening && firstFrame) {
  console.log(`  ${'起播总耗时'.padEnd(20)}: ${(firstFrame.t - opening.t).toFixed(3)} s（Opening done → 首帧）`);
}
const buffering = kernel.filter((l) => /Enter buffering|End buffering/.test(l));
console.log(`  ${'缓冲事件'.padEnd(20)}: ${buffering.length}`);

// ---- 取数侧：绝对时间（epoch ms） ----
const chunks = [];
for (const line of net) {
  const m = line.match(/^(\d+) \[([\w-]+)\] origin chunk from=(\d+) code=(\d+) hdrMs=(\d+)(?: len=(\d+))?/);
  if (!m) continue;
  chunks.push({
    t: Number(m[1]),
    tag: m[2],
    from: Number(m[3]),
    hdr: Number(m[5]),
    len: m[6] !== undefined ? Number(m[6]) : null,
  });
}

// 总长：从"到达片源末尾"那行拿（它带总长），拿不到就退回 cues 阈值。
let total = 0;
for (const line of net) {
  const m = line.match(/总长 (\d+)/);
  if (m) total = Number(m[1]);
}

const TAIL_WINDOW = 2 * 1024 * 1024;
const tailChunks = chunks.filter((c) => (total > 0 ? c.from >= total - TAIL_WINDOW : c.from >= 8e8));

const prefetch = net.filter((l) => /\[tail\] prefetch/.test(l));
const hits = net.filter((l) => /\[tail\] cache hit/.test(l));
const proxyGets = net.filter((l) => /\bGET range=/.test(l));

console.log('');
console.log('=== 取数侧（绝对时间，epoch ms） ===');
console.log(`  ${'片源总长'.padEnd(20)}: ${total || '(未知)'}`);
console.log(`  ${'取数层总块数'.padEnd(20)}: ${chunks.length}`);
console.log(`  ${'片尾(cues)块'.padEnd(20)}: ${tailChunks.length}  ${tailChunks.map((c) => `[${c.tag}] from=${c.from} hdr=${c.hdr}ms len=${c.len}`).join(' | ')}`);
console.log(`  ${'片尾预取'.padEnd(20)}: ${prefetch.length}  ${prefetch.map((l) => l.slice(l.indexOf('[tail]'))).join(' | ')}`);
console.log(`  ${'片尾缓存命中'.padEnd(20)}: ${hits.length}  ${hits.slice(0, 4).map((l) => l.slice(l.indexOf('[tail]'))).join(' | ')}`);

// 代理层：片尾那一跳（这是内核真正感觉到的那一跳）
const tailGets = proxyGets.filter((l) => {
  const m = l.match(/range=bytes=(\d+)-/);
  return m && total > 0 && Number(m[1]) >= total - TAIL_WINDOW;
});
console.log(`  ${'代理片尾 GET'.padEnd(20)}: ${tailGets.length}  ${tailGets.join(' | ')}`);
// 对齐：把内核侧的相对时间锚到取数侧的绝对时间。
// 锚点 = 内核发起第一个片源请求的那一跳（取数侧最早的那块 = 片头），
// 内核的 "Opening done" 与它不是同一件事，所以只用**差值**比较。
if (chunks.length && firstFrame && opening) {
  const head = chunks.find((c) => c.tag === 'play' && c.from === 0);
  if (head) {
    // 取数侧最早的一块（片头）与内核 Opening done 之间没有固定关系，
    // 这里只把两层的**相对差**（片尾跳耗时）摆出来，不做绝对对齐。
    const tail = tailChunks.find((c) => c.tag === 'play');
    if (tail) {
      console.log('');
      console.log('=== 片尾那一跳 ===');
      console.log(`  片头块表头       : ${head.hdr} ms`);
      console.log(`  片尾块表头       : ${tail.hdr} ms`);
      console.log(`  片尾块起始       : ${tail.from}（总长 ${total}，距尾 ${total - tail.from} 字节）`);
    }
  }
}

// ---- 端到端：主机 tap → 内核首帧 ----
// 内核日志里的时间是**相对 mpv 进程时钟**的，主机侧的 tap 时刻是绝对 epoch：
// 两边对不上 ⇒ 必须先求出内核 t=0 的绝对时刻。
// 锚点 = 片头那一跳的请求发出时刻（响应表头花了 hdrMs ⇒ 发出时刻 ≈ 落盘时刻 - hdrMs）。
const headChunk = chunks.find((c) => c.tag === 'play' && c.from === 0);
const mpvT0 = headChunk ? headChunk.t - headChunk.hdr : 0;
const firstFrameEpoch = (() => {
  for (const line of net) {
    if (!/MPV 首帧就绪/.test(line)) continue;
    const m = line.match(/^(\d+)/);
    if (m) return Number(m[1]);
  }
  return 0;
})();
const firstFrameAbs = firstFrameEpoch || (mpvT0 && firstFrame ? mpvT0 + firstFrame.t : 0);
console.log('');
console.log('=== 端到端（主机 tap → 内核首帧） ===');
console.log(`  ${'内核 t0（估算）'.padEnd(20)}: ${mpvT0 || '(未知)'}`);
console.log(`  ${'首帧绝对时刻'.padEnd(20)}: ${firstFrameAbs || '(未知)'}`);
if (tapEpoch && firstFrameAbs) {
  console.log(`  ${'tap→首帧'.padEnd(20)}: ${((firstFrameAbs - tapEpoch) / 1000).toFixed(3)} s`);
}
// 预取落地的绝对时刻：它必须**早于**内核发起 cues 那一跳，否则加速不到。
const prefetchLine = net.find((l) => /\[tail\] prefetch/.test(l));
if (prefetchLine) {
  const m = prefetchLine.match(/^(\d+)/);
  console.log(`  ${'预取落盘时刻'.padEnd(20)}: ${m ? m[1] : '(未知)'}   ${prefetchLine.slice(prefetchLine.indexOf('[tail]'))}`);
}

