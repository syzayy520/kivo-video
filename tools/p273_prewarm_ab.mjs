// 复算 273 v16 · 详情页预热 vs 无预热（起播关键路径）。
//
// 用法：node tools/p273_prewarm_ab.mjs <evidence-dir>
//   目录里应有两组：c*.{net,kernel,diag}.log（无预热）与 p*.{net,kernel,diag}.log（详情页预热）
//
// 【口径】"首帧"有两种，本工具把两种都摆出来：
//   · **诚实首帧** = .diag 里首个 `RND +Nms post #0`（渲染层真正 swap 过一帧 = 画面上屏）
//     —— 这是用户看到画面的时刻。
//   · **应用锚点** = net-diag 里的 `MPV 首帧就绪`（Kotlin 侧等的是原生 first_frame 标志）
//     —— v15 之前它用的是"播放时钟起跳"，偏早 1.5–2.8 s；v16 之后与诚实首帧对齐。
//
// 全部数字都从日志复算，不含手抄。
//
// 本机没有 Python 解释器，证据复算一律走 node（见 agents.md 工具限制）。

import { readFileSync, existsSync } from 'node:fs';
import { join } from 'node:path';

const dir = process.argv[2];
if (!dir) {
  console.error('usage: node tools/p273_prewarm_ab.mjs <evidence-dir>');
  process.exit(2);
}

const read = (name) => {
  const p = join(dir, name);
  if (!existsSync(p)) return [];
  return readFileSync(p, 'utf8').split(/\r?\n/).filter(Boolean);
};

/** net-diag 里第一条匹配行的 epoch ms */
const at = (lines, re) => {
  for (const l of lines) {
    if (!re.test(l)) continue;
    const m = l.match(/^(\d+)/);
    if (m) return Number(m[1]);
  }
  return 0;
};

/** mpv-kernel.log 里第一条匹配行的相对秒 */
const kAt = (lines, re) => {
  for (const l of lines) {
    if (!re.test(l)) continue;
    const m = l.match(/^\[\s*([\d.]+)\]/);
    if (m) return Number(m[1]);
  }
  return -1;
};

function analyze(round) {
  const net = read(`${round}-net.log`);
  const diag = read(`${round}-diag.log`);
  const kern = read(`${round}-kernel.log`);
  if (!net.length) return null;

  const take = at(net, /接管详情页预热的源/);
  const prepStart = at(net, /URL 预备开始/);
  const playStart = at(net, /URL 起播开始/);
  const gate = at(net, /闸门放行/);
  const appAnchor = at(net, /MPV 首帧就绪/);

  // 诚实首帧：渲染层第一帧真正上屏
  let honestMs = null;
  for (const l of diag) {
    const m = l.match(/^RND \+(\d+)ms post #0/);
    if (m) { honestMs = Number(m[1]); break; }
  }

  // 取数侧：片头那一跳 / 第一簇那一跳 / 片尾是否走了回源
  const chunks = [];
  for (const l of net) {
    const m = l.match(/^(\d+) \[([\w-]+)\] origin chunk from=(\d+) code=(\d+) hdrMs=(\d+)(?: len=(\d+))?/);
    if (!m) continue;
    chunks.push({ t: Number(m[1]), tag: m[2], from: Number(m[3]), hdr: Number(m[5]), len: m[6] ? Number(m[6]) : null });
  }
  const head = chunks.find((c) => c.tag === 'play' && c.from === 0) || null;
  const tailOrigin = chunks.filter((c) => c.tag === 'play' && c.from > 1_000_000_000);

  const openDone = kAt(kern, /Opening done/);
  const mpvFirstFrame = kAt(kern, /first video frame after restart shown/);
  // 内核的两跳 seek：① 去文件尾读 cues ② 回到目标簇
  const seeks = [];
  for (const l of kern) {
    const m = l.match(/^\[\s*([\d.]+)\].*stream level seek from [\d.]+ to (\d+)/);
    if (m) seeks.push({ t: Number(m[1]), to: Number(m[2]) });
  }
  const seekCues = seeks.length ? seeks[0].t : -1;
  const seekCluster = seeks.length > 1 ? seeks[1].t : -1;
  const clusterOffset = seeks.length > 1 ? seeks[1].to : 0;

  // 窗口命中（273 v17）：片头窗口 / 片尾窗口
  const headHits = net.filter((l) => /\[head\] cache hit/.test(l));
  const tailHits = net.filter((l) => /\[tail\] cache hit/.test(l));
  const hitBytes = (lines) => lines.reduce((s, l) => {
    const m = l.match(/bytes=(\d+)/);
    return s + (m ? Number(m[1]) : 0);
  }, 0);

  // 恢复点（273 v18）：cues 算出的偏移 + 定向窗口的命中
  //   预取侧一行（在**预热日志**里）： [resume] cues=4244 条，21063ms → 偏移 8490718，预取 8425182..12619486
  const prewarm = read(`${round}-prewarm-net.log`);
  const resumeLine = [...net, ...prewarm].find((l) => /\[resume\] cues=\d+ 条/.test(l)) || '';
  const rm = resumeLine.match(/cues=(\d+) 条，(\d+)ms → 偏移 (\d+)，预取 (\d+)\.\.(\d+)/);
  const resumeOffset = rm ? Number(rm[3]) : null;
  const resumeWindow = rm ? { start: Number(rm[4]), end: Number(rm[5]) } : null;
  const cueCount = rm ? Number(rm[1]) : null;
  const resumeHits = net.filter((l) => /\[resume\] cache hit/.test(l));
  const resumeInflight = resumeHits.filter((l) => /src=inflight/.test(l));
  // 预取侧抓到的那一跳（标签 play-resume），与播放侧的回源分开算
  const resumePrefetchChunk = chunks.find((c) => c.tag === 'play-resume') || null;

  // 起播后的缓冲与音频欠载（v17 起就是已知缺口，见 §十六 G2）
  const bufferEvents = kern.filter((l) => /Enter buffering/.test(l)).length;
  const underruns = kern.filter((l) => /Audio device underrun detected/.test(l)).length;

  // 关键路径上的源站往返：① 片头那一跳（from=0）② 回跳首簇那一跳（from=首簇偏移）
  //   —— 窗口命中时这两跳应当**一次都不剩**。
  const criticalOrigin = chunks.filter((c) => c.tag === 'play' && (c.from === 0 || c.from === clusterOffset));

  // 起算：预热路径 = 接手源那一刻；无预热路径 = 预备开始那一刻
  const t0 = take || prepStart || playStart;

  return {
    round,
    prewarmed: take > 0,
    gateWaitMs: gate && playStart ? gate - playStart : null,
    appAnchorMs: appAnchor && t0 ? appAnchor - t0 : null,
    honestSessionMs: honestMs,
    mpvFirstFrame,
    openDone,
    seekCues,
    seekCluster,
    clusterOffset,
    head,
    tailOriginCount: tailOrigin.length,
    tailHitCount: tailHits.length,
    tailHitBytes: hitBytes(tailHits),
    headHitCount: headHits.length,
    headHitBytes: hitBytes(headHits),
    criticalOriginCount: criticalOrigin.length,
    cueCount,
    resumeOffset,
    resumeWindow,
    resumeHitCount: resumeHits.length,
    resumeHitBytes: hitBytes(resumeHits),
    resumeInflightCount: resumeInflight.length,
    resumePrefetchChunk,
    // 恢复点那一跳在关键路径上有没有回源（tag=play 且 from=恢复点偏移）
    resumeOriginCount: resumeOffset !== null
      ? chunks.filter((c) => c.tag === 'play' && c.from === resumeOffset).length
      : 0,
    bufferEvents,
    underruns,
    net,
  };
}

function collect(prefix) {
  const out = [];
  for (let i = 1; i <= 9; i++) {
    const r = analyze(`${prefix}${i}`);
    if (r) out.push(r);
  }
  return out;
}

const cRounds = collect('c');
const pRounds = collect('p');

const median = (xs) => {
  if (!xs.length) return null;
  const s = [...xs].sort((a, b) => a - b);
  const m = s.length >> 1;
  return s.length % 2 ? s[m] : (s[m - 1] + s[m]) / 2;
};

// 表头与列宽（一处定义，两边共用）
const COLS = [
  ['轮次', 8], ['路径', 8], ['闸门等待ms', 10], ['诚实首帧ms', 10], ['mpv首帧s', 9],
  ['Opening s', 9], ['seek→cues s', 11], ['seek→首簇 s', 11], ['片头表头ms', 10],
  ['片头命中数', 10], ['片头命中字节', 12], ['尾命中数', 8], ['尾命中字节', 10],
  ['尾回源次数', 10], ['关键路径源站往返', 14],
  ['恢复点偏移', 12], ['恢复点命中', 10], ['恢复点命中字节', 12], ['恢复点回源', 10],
  ['缓冲次数', 8], ['音频欠载', 8],
];

function row(r) {
  return [
    r.round,
    r.prewarmed ? '预热' : '无预热',
    r.gateWaitMs !== null ? `${r.gateWaitMs}` : '-',
    r.honestSessionMs !== null ? `${r.honestSessionMs}` : '-',
    r.mpvFirstFrame >= 0 ? `${r.mpvFirstFrame.toFixed(3)}` : '-',
    r.openDone >= 0 ? `${r.openDone.toFixed(3)}` : '-',
    r.seekCues >= 0 ? `${r.seekCues.toFixed(3)}` : '-',
    r.seekCluster >= 0 ? `${r.seekCluster.toFixed(3)}` : '-',
    r.head ? `${r.head.hdr}` : '-',
    r.headHitCount,
    r.headHitBytes,
    r.tailHitCount,
    r.tailHitBytes,
    r.tailOriginCount,
    r.criticalOriginCount,
    r.resumeOffset !== null ? `${r.resumeOffset}` : '-',
    r.resumeHitCount,
    r.resumeHitBytes,
    r.resumeOriginCount,
    r.bufferEvents,
    r.underruns,
  ];
}

console.log('=== v16/v17 A/B · 起播关键路径（全部由日志复算） ===');
console.log(COLS.map(([h]) => h.padEnd(COLS.find((c) => c[0] === h)[1])).join(''));
for (const r of [...cRounds, ...pRounds]) {
  console.log(row(r).map((c, i) => String(c).padEnd(COLS[i][1])).join(''));
}

const stat = (rounds) => {
  const gate = rounds.map((r) => r.gateWaitMs).filter((v) => v !== null);
  const honest = rounds.map((r) => r.honestSessionMs).filter((v) => v !== null);
  const tailHits = rounds.map((r) => r.tailHitBytes).filter((v) => v !== undefined);
  const tailOrigin = rounds.map((r) => r.tailOriginCount).filter((v) => v !== undefined);
  const headHits = rounds.map((r) => r.headHitBytes).filter((v) => v !== undefined);
  const critical = rounds.map((r) => r.criticalOriginCount).filter((v) => v !== undefined);
  const resumeHit = rounds.map((r) => r.resumeHitBytes).filter((v) => v !== undefined);
  const resumeOrigin = rounds.map((r) => r.resumeOriginCount).filter((v) => v !== undefined);
  const buf = rounds.map((r) => r.bufferEvents).filter((v) => v !== undefined);
  const und = rounds.map((r) => r.underruns).filter((v) => v !== undefined);
  return {
    gateP50: median(gate),
    honestP50: median(honest),
    tailHitBytes: tailHits,
    tailOrigin,
    headHitBytes: headHits,
    critical,
    resumeHit,
    resumeOrigin,
    buf,
    und,
  };
};

console.log('');
console.log('=== 中位对照 ===');
for (const [name, rounds] of [['C 无预热', cRounds], ['P 详情页预热', pRounds]]) {
  const s = stat(rounds);
  console.log(
    `${name.padEnd(12)} 闸门等待 ${s.gateP50 !== null ? s.gateP50 + ' ms' : '-'}` +
    `   诚实首帧 ${s.honestP50 !== null ? s.honestP50 + ' ms' : '-'}` +
    `   片头命中字节 ${s.headHitBytes.join('/')}` +
    `   尾命中字节 ${s.tailHitBytes.join('/')}` +
    `   尾回源 ${s.tailOrigin.join('/')}` +
    `   关键路径源站往返 ${s.critical.join('/')}`,
  );
  console.log(
    `${''.padEnd(12)} 恢复点命中字节 ${s.resumeHit.join('/')}` +
    `   恢复点回源 ${s.resumeOrigin.join('/')}` +
    `   缓冲次数 ${s.buf.join('/')}` +
    `   音频欠载 ${s.und.join('/')}`,
  );
}

const cs = stat(cRounds);
const ps = stat(pRounds);
if (cs.honestP50 && ps.honestP50) {
  const d = ps.honestP50 - cs.honestP50;
  console.log('');
  console.log(`诚实首帧中位：${cs.honestP50} ms → ${ps.honestP50} ms（${d > 0 ? '+' : ''}${d} ms，${((d / cs.honestP50) * 100).toFixed(1)}%）`);
}
