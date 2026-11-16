#!/usr/bin/env node
// 274 · 本地限速源站（可复现实验装置）
//
// 【为什么需要它】真实源站是公共的、会波动的（实测：几分钟内 391 KB/s ↔ 2116 KB/s，
// 甚至瞬时不可达）⇒ 任何跨轮次的 A/B 都被这个变量污染（规格 §26）。
// ⇒ 这个装置**从 Emby 取真实可解码的 MKV**，但**按精确速率**转发给 App，
//    并可注入表头延迟（模拟"连接老化"）。
//
// 【限速模型】**每连接封顶**（与真实源站一致：真机 v5/v32 实测
//   单连接 ≈700 KB/s、8 条 ≈2483 KB/s ⇒ 接近线性 ⇒ 每连接封顶，不是全局封顶）。
//
// 【用法】
//   node tools/p274_local_origin.mjs --port 8099 --mbps 0.6 [--hdr-ms 900] [--stale]
//   adb reverse tcp:8099 tcp:8099
//   adb shell am start ... --es kivo.playurl "http://127.0.0.1:8099/v"
//
// 【参数】
//   --port     本地端口（默认 8099）
//   --mbps     每连接限速 MB/s（默认 0.6）⇒ 复现"慢源站"
//   --hdr-ms   注入的**表头延迟**毫秒（默认 0）⇒ 复现"新建连接慢/老化态"
//   --stale    表头延迟随时间增长（模拟连接老化：越用越慢）
//   --log      统计输出文件（默认 p274-local-origin.log）
//   --file     **本地片源文件**（给了就从本地读 ⇒ 速率完全确定，不受上游波动影响）
//   --upstream 从上游取字节时用的 URL（未给 --file 时用）
//   --size     本地文件对外宣称的总长（默认 = 文件实际大小）
//
// 【为什么优先用本地文件】
// 上游 Emby 的速度在 100~1700 KB/s 之间**纯随机波动**（实测），
// 靠它做限速基准是**测不准的**（实测：想限 0.6 MB/s，测出 0.14~0.27）。
// ⇒ 从本地文件供给 ⇒ 速率由本脚本**精确决定** ⇒ A/B 才成立。

import http from 'node:http';
import https from 'node:https';
import fs from 'node:fs';

const argv = process.argv.slice(2);
function arg(name, def) {
  const i = argv.indexOf(name);
  return i >= 0 && i + 1 < argv.length ? argv[i + 1] : def;
}
const PORT = parseInt(arg('--port', '8099'), 10);
const MBPS = parseFloat(arg('--mbps', '0.6'));
const HDR_MS = parseInt(arg('--hdr-ms', '0'), 10);
const STALE = argv.includes('--stale');
const LOG = arg('--log', 'p274-local-origin.log');
const LOCAL_FILE = arg('--file', '');
const FILE_START = parseInt(arg('--file-start', '0'), 10);
const HEAD_FILE = arg('--head-file', '');
const HEAD_START = parseInt(arg('--head-start', '0'), 10);
const TAIL_FILE = arg('--tail-file', '');
const TAIL_START = parseInt(arg('--tail-start', '0'), 10);
const UPSTREAM = arg('--upstream', 'https://emby.bangumi.ca/emby/Videos/233908/stream?static=true');
const TOKEN = arg('--token', process.env.EMBY_TOKEN || '');
const DECLARED_SIZE = parseInt(arg('--size', '0'), 10);

// 291 · 退化连接注入：确定性复现「单条连接吞吐塌陷」造成的分钟级冻结。
// 真机 p290-live 的 127.8s 冻结就是一条 4MiB 块被源站以几十 KB/s 慢慢吐；
// 主链路没有整块吞吐下限，soTimeout 永不触发，也不留任何日志。
// 这里每 N 个主 chunk 把一条响应降到 DEGENERATE_MBPS ⇒ 两臂命中次数/时机确定。
const DEGENERATE_EVERY = parseInt(arg('--degenerate-every', '0'), 10);
const DEGENERATE_MBPS = parseFloat(arg('--degenerate-mbps', '0.04'));
const DEGENERATE_AFTER_SEC = parseFloat(arg('--degenerate-after-sec', '0'));

// 292 · 只给**主 chunk（>=1 MiB 的有界 Range）**注入表头空窗，用来确定性复现
// 「首帧零缓冲起步 + 串行补块表头空窗」造成的连环 buffering（p291-night-both 起播
// 7.767s 冻结）。探测 / cues / 片尾这些小请求保持零延迟，避免拖慢起播 prep
// 让首帧超时 ⇒ 两臂起播条件一致，变量只有「主 chunk 之间有没有空窗」。
const CHUNK_HDR_MS = parseInt(arg('--chunk-hdr-ms', '0'), 10);

// 293 · 全局总带宽（--global-mbps > 0）：所有响应共享一条虚拟管道。
// 默认 0 = 每连接独立限速（旧行为；多连接会叠加，做方案 D 对照时偏乐观）。
// 设为与 --mbps 相同 ⇒ 单连接不变、双连接各分一半，模拟「源站总带宽固定」。
const GLOBAL_MBPS = parseFloat(arg('--global-mbps', '0'));
const globalBytesPerSec = GLOBAL_MBPS > 0 ? GLOBAL_MBPS * 1024 * 1024 : 0;
let globalNextAt = 0;
const degenerateBytesPerSec = DEGENERATE_MBPS * 1024 * 1024;
let mainChunkCount = 0;
let degenerateCount = 0;

const bytesPerSec = MBPS * 1024 * 1024;
const started = Date.now();
let reqCount = 0;
let bytesServed = 0;
const lines = [];

function log(line) {
  const s = `[${((Date.now() - started) / 1000).toFixed(1)}s] ${line}`;
  lines.push(s);
  if (lines.length > 4000) lines.shift();
}

/** 解析 Range: bytes=start-end */
function parseRange(h) {
  if (!h) return null;
  const m = /bytes=(\d*)-(\d*)/.exec(h);
  if (!m) return null;
  const s = m[1] === '' ? null : parseInt(m[1], 10);
  const e = m[2] === '' ? null : parseInt(m[2], 10);
  return { start: s, end: e };
}

/** 表头延迟（--stale 时随时间增长） */
function headerDelay() {
  if (!STALE) return HDR_MS;
  const sec = (Date.now() - started) / 1000;
  // 30 s 内从 HDR_MS 线性涨到 HDR_MS * 6
  return HDR_MS + Math.min(1, sec / 30) * HDR_MS * 5;
}

/**
 * 限速流：按**累计字节对齐时间轴**。
 *
 * 【为什么不用流控】`src.pause()` + 定时器释放那种写法实测两种都不准
 * （0.6 MB/s 设定只跑到 0.30 / 0.14 MB/s）⇒ 因为"释放"与"上游流入"
 * 两条节奏互相干扰。
 * ⇒ 改成：**发第 N 个字节的时刻 = 起点 + N/speed**，
 *    到达时若还没到点就等 ⇒ 速率与上游快慢**无关**，只由 speed 决定。
 */
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

async function throttle(src, dst, speed, onBytes) {
  const t0 = Date.now();
  let served = 0;
  try {
    for await (const chunk of src) {
      served += chunk.length;
      if (onBytes) onBytes(chunk.length);
      const dueMs = t0 + (served / speed) * 1000;
      let target = dueMs;
      // 293 · 全局带宽桶：先「预约」再等（Node 单线程 ⇒ 预约无竞态）。
      // 被背压卡住的响应不会推进 globalNextAt ⇒ 也不会抢别路的配额。
      if (globalBytesPerSec > 0) {
        const slot = Math.max(Date.now(), globalNextAt) +
          (chunk.length / globalBytesPerSec) * 1000;
        globalNextAt = slot;
        target = Math.max(target, slot);
      }
      const wait = target - Date.now();
      if (wait > 1) await sleep(wait);
      if (dst.destroyed) return;
      const ok = dst.write(chunk);
      if (!ok) await new Promise((r) => dst.once('drain', r));
    }
    if (!dst.destroyed) dst.end();
  } catch (e) {
    if (!dst.destroyed) dst.destroy();
  }
}

const server = http.createServer((req, res) => {
  const id = ++reqCount;
  const range = parseRange(req.headers.range);
  const hdrDelay = headerDelay();
  log(`REQ#${id} ${req.method} range=${req.headers.range || '-'} hdrDelay=${hdrDelay}ms`);

  // 【本地文件分支】速率由本脚本精确决定 ⇒ A/B 才成立。
  if (LOCAL_FILE) {
    serveLocal(id, res, range, hdrDelay);
    return;
  }

  serveUpstream(id, res, range, hdrDelay);
});

/** 从上游取字节（不限速）。 */
function serveUpstream(id, res, range, hdrDelay) {
  const headers = { 'X-Emby-Token': TOKEN, 'User-Agent': 'KivoLocalOrigin/1.0' };
  if (range && range.start !== null) {
    headers.Range = range.end !== null
      ? `bytes=${range.start}-${range.end}`
      : `bytes=${range.start}-`;
  }
  const up = https.request(UPSTREAM, { method: 'GET', headers }, (ur) => {
    const total = parseInt(ur.headers['content-range']?.split('/')?.[1] || '0', 10);
    const fire = () => {
      if (range && range.start !== null) {
        res.writeHead(206, {
          'Content-Type': 'video/x-matroska',
          'Content-Length': String(ur.headers['content-length'] || 0),
          'Content-Range': ur.headers['content-range'] || `bytes ${range.start}-${total - 1}/${total}`,
          'Accept-Ranges': 'bytes',
        });
      } else {
        res.writeHead(200, {
          'Content-Type': 'video/x-matroska',
          'Content-Length': String(ur.headers['content-length'] || 0),
          'Accept-Ranges': 'bytes',
        });
      }
      const t0 = Date.now();
      let n = 0;
      // 【不要在这里注册 'data' 监听器】那会让流进入 flowing 模式，
      // 与 throttle 里的 `for await` **抢同一个流** ⇒ 两个消费者互相干扰。
      // ⇒ 统计统一放在 throttle 内部（唯一的消费者）。
      throttle(ur, res, bytesPerSec, (c) => { n += c; bytesServed += c; }).then(() => {
        const ms = Date.now() - t0;
        log(`  DONE#${id} bytes=${n} (${(n / 1048576).toFixed(2)} MiB) in ${ms}ms ⇒ ${(n / 1048576 / (ms / 1000)).toFixed(3)} MB/s`);
      });
    };
    if (hdrDelay > 0) setTimeout(fire, hdrDelay); else fire();
  });
  up.on('error', (e) => {
    log(`  UPERR#${id} ${e.message}`);
    if (!res.headersSent) res.writeHead(502); res.end();
  });
  up.end();
}

/**
 * 从本地文件供给一段区间（速率精确可控）。
 *
 * 【对外宣称的总长】用 DECLARED_SIZE（真实片源长度）⇒ 内核的 demux 行为
 * （缓存策略 / 区间计算）与对着真实源站时**完全一致**；
 * 只有字节本身来自本地（文件末尾之后按 EOF 处理）。
 */
function serveLocal(id, res, range, hdrDelay) {
  const stat = fs.statSync(LOCAL_FILE);
  const realSize = stat.size;
  const declared = DECLARED_SIZE > 0 ? DECLARED_SIZE : realSize;
  const start = range && range.start !== null ? range.start : 0;
  // 开口区间（内核发的是 `bytes=OFF-`）⇒ 对外宣称到总长末尾，
  // 但实际供给会被本地文件长度 clamp。
  const requestedEnd = (range && range.end !== null) ? range.end : (declared - 1);

  // 【主文件对应的真实偏移区间】
  // 本地片段不必从 0 开始：内核从 MKV cues 里读出续播位（真机：72.78 秒
  // ⇒ 偏移 22065985），所以本地只要覆盖**续播位之后**的那一段即可。
  const mainStart = FILE_START;
  const mainEnd = FILE_START + realSize - 1;
  const inMain = start >= mainStart && start <= mainEnd;
  // 【片头段】内核打开片源时**先读 offset 0**（容器头 / SeekHead），
  //   读不到就什么都做不了（实测装置上表现为上游返回 35 字节错误 ⇒ 全程无画面）。
  const headSize = HEAD_FILE ? fs.statSync(HEAD_FILE).size : 0;
  const headEnd = HEAD_START + headSize - 1;
  const inHead = HEAD_FILE && start >= HEAD_START && start <= headEnd;

  // 【片尾段】MKV 的 cues 在片尾 ⇒ libavformat 打开时必须 seek 到片尾读 cues。
  //   本地片段若只有片头，内核会卡在等 cues 上（实测渲染 0 帧、
  //   装置侧被同一区段重复请求 194 次）⇒ 单独抓一段片尾放在本地。
  const tailSize = TAIL_FILE ? fs.statSync(TAIL_FILE).size : 0;
  const tailEnd = TAIL_START + tailSize - 1;
  const inTail = TAIL_FILE && start >= TAIL_START && start <= tailEnd;

  // 292 · 主 chunk 表头空窗：p291-night-both 起播期主 chunk 表头 1.5–8 s，
  // 串行补块时这段窗口供给为 0 ⇒ 首帧零缓冲起步必然 underrun。
  // 放宽到「任一本地段 + 有界请求 >=1 MiB」：run/arm query 禁续播后会从头播，
  // 起播主链路落在 head 段（sample.mkv），若只判 inMain 就命中不到 ⇒ 复现失效。
  // 探测(1B)/cues(256KB)/warmup ping(1B) 天然 <1MiB 被排除 ⇒ 起播 prep 不拖慢。
  const isBigLocalChunk = (inHead || inMain || inTail) &&
    (requestedEnd - start + 1) >= 1024 * 1024;
  const effectiveDelay = hdrDelay + (isBigLocalChunk ? CHUNK_HDR_MS : 0);
  if (isBigLocalChunk && CHUNK_HDR_MS > 0) {
    log(`  CHUNK-HDR#${id} 主块表头空窗 +${CHUNK_HDR_MS}ms（range ${start}-${requestedEnd}）`);
  }

  const fire = () => {
    if (inTail) {
      const localStart = start - TAIL_START;
      const localEnd = Math.min(requestedEnd - TAIL_START, tailSize - 1);
      const length = localEnd - localStart + 1;
      res.writeHead(206, {
        'Content-Type': 'video/x-matroska',
        'Content-Length': String(length),
        'Content-Range': `bytes ${start}-${start + length - 1}/${declared}`,
        'Accept-Ranges': 'bytes',
      });
      const t0 = Date.now();
      let n = 0;
      const src = fs.createReadStream(TAIL_FILE, { start: localStart, end: localEnd });
      throttle(src, res, bytesPerSec, (c) => { n += c; bytesServed += c; }).then(() => {
        const ms = Date.now() - t0;
        log(`  DONE#${id} [tail] bytes=${n} (${(n / 1048576).toFixed(2)} MiB) in ${ms}ms ⇒ ${(n / 1048576 / (ms / 1000)).toFixed(3)} MB/s`);
      });
      return;
    }
    if (inHead) {
      const localStart = start - HEAD_START;
      const localEnd = Math.min(requestedEnd - HEAD_START, headSize - 1);
      const length = localEnd - localStart + 1;
      res.writeHead(206, {
        'Content-Type': 'video/x-matroska',
        'Content-Length': String(length),
        'Content-Range': `bytes ${start}-${start + length - 1}/${declared}`,
        'Accept-Ranges': 'bytes',
      });
      const t0 = Date.now();
      let n = 0;
      const src = fs.createReadStream(HEAD_FILE, { start: localStart, end: localEnd });
      throttle(src, res, bytesPerSec, (c) => { n += c; bytesServed += c; }).then(() => {
        const ms = Date.now() - t0;
        log(`  DONE#${id} [head] bytes=${n} (${(n / 1048576).toFixed(2)} MiB) in ${ms}ms ⇒ ${(n / 1048576 / (ms / 1000)).toFixed(3)} MB/s`);
      });
      return;
    }
    if (!inMain) {
      // 【本地没有这一段 ⇒ 回落到上游】
      log(`  FALLBACK#${id} start=${start} ∉ 本地 [${mainStart}, ${mainEnd}] ⇒ 走上游`);
      serveUpstream(id, res, range, hdrDelay);
      return;
    }
    // 主文件段：终点 clamp 到本地文件实际长度（本地没有的后面回落到上游）。
    const end = Math.min(requestedEnd, mainEnd);
    const length = end - start + 1;
    const localStart = start - mainStart;

    // 291 · 只挑 **主 chunk**（>=1 MiB）降级：探测 / 片尾 / cues 那些小请求
    // 保持正常，避免污染起播首帧口径。两臂用同一组参数 ⇒ 因果判据成立。
    let speed = bytesPerSec;
    const eligible = length >= 1024 * 1024 &&
      (Date.now() - started) / 1000 >= DEGENERATE_AFTER_SEC;
    if (DEGENERATE_EVERY > 0 && eligible) {
      mainChunkCount++;
      if (mainChunkCount % DEGENERATE_EVERY === 0) {
        speed = degenerateBytesPerSec;
        degenerateCount++;
        log(`  DEGENERATE#${id} 第 ${degenerateCount} 次：本块 ${(length / 1048576).toFixed(0)}MiB 降到 ${DEGENERATE_MBPS} MB/s（预期 ${Math.round(length / speed)}s 读完）`);
      }
    }
    const tag = speed !== bytesPerSec ? ' [degenerate]' : '';
    res.writeHead(206, {
      'Content-Type': 'video/x-matroska',
      'Content-Length': String(length),
      'Content-Range': `bytes ${start}-${end}/${declared}`,
      'Accept-Ranges': 'bytes',
    });
    const t0 = Date.now();
    let n = 0;
    const src = fs.createReadStream(LOCAL_FILE, { start: localStart, end: localStart + length - 1 });
    throttle(src, res, speed, (c) => { n += c; bytesServed += c; }).then(() => {
      const ms = Date.now() - t0;
      log(`  DONE#${id}${tag} bytes=${n} (${(n / 1048576).toFixed(2)} MiB) in ${ms}ms ⇒ ${(n / 1048576 / (ms / 1000)).toFixed(3)} MB/s`);
    });
  };
  // 292：主 chunk 额外等 CHUNK_HDR_MS 再发响应头（表头空窗），其余请求维持原延迟。
  if (effectiveDelay > 0) setTimeout(fire, effectiveDelay); else fire();
}

server.listen(PORT, '127.0.0.1', () => {
  console.log(`本地限速源站已启动 http://127.0.0.1:${PORT}/v`);
  console.log(`  MBPS=${MBPS}  bytesPerSec=${bytesPerSec}  B/s  globalMBPS=${GLOBAL_MBPS}`);
  console.log(`  表头延迟 ${HDR_MS}ms${STALE ? '（随时间增长）' : ''}`);
  console.log(`  数据源 ${LOCAL_FILE ? '本地文件 ' + LOCAL_FILE : '上游 ' + UPSTREAM}`);
  console.log(`  宣称总长 ${DECLARED_SIZE || '（= 实际）'}`);
  console.log(`  退化连接 every=${DEGENERATE_EVERY} mbps=${DEGENERATE_MBPS} after=${DEGENERATE_AFTER_SEC}s`);
  console.log(`  adb reverse tcp:${PORT} tcp:${PORT}`);
});

function flush() {
  fs.writeFileSync(LOG, lines.join('\n'));
  console.log(`\n=== 统计 ===\n请求 ${reqCount} · 供出 ${(bytesServed / 1048576).toFixed(1)} MiB · 平均 ${(bytesServed / 1048576 / ((Date.now() - started) / 1000)).toFixed(3)} MB/s · 退化注入 ${degenerateCount} 次`);
}
process.on('SIGINT', () => { flush(); process.exit(0); });
process.on('SIGTERM', () => { flush(); process.exit(0); });
setInterval(flush, 5000);
