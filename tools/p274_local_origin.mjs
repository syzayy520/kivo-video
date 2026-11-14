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
const UPSTREAM = arg('--upstream', 'https://emby.bangumi.ca/emby/Videos/233908/stream?static=true');
const TOKEN = arg('--token', process.env.EMBY_TOKEN || '');

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
      const wait = dueMs - Date.now();
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

  const headers = { 'X-Emby-Token': TOKEN, 'User-Agent': 'KivoLocalOrigin/1.0' };
  if (range && range.start !== null) {
    headers.Range = range.end !== null
      ? `bytes=${range.start}-${range.end}`
      : `bytes=${range.start}-`;
  }
  const up = https.request(UPSTREAM, { method: 'GET', headers }, (ur) => {
    const total = parseInt(ur.headers['content-range']?.split('/')?.[1] || '0', 10);
    // 注入表头延迟
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
      // 与 throttle 里的 `for await` **抢同一个流** ⇒ 两个消费者互相干扰，
      // 实测限速失真到设定的 1/4（0.6 MB/s 跑成 0.15 MB/s）。
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
});

server.listen(PORT, '127.0.0.1', () => {
  console.log(`本地限速源站已启动 http://127.0.0.1:${PORT}/v`);
  console.log(`  每连接限速 ${MBPS} MB/s · 表头延迟 ${HDR_MS}ms${STALE ? '（随时间增长）' : ''}`);
  console.log(`  上游 ${UPSTREAM}`);
  console.log(`  adb reverse tcp:${PORT} tcp:${PORT}`);
});

function flush() {
  fs.writeFileSync(LOG, lines.join('\n'));
  console.log(`\n=== 统计 ===\n请求 ${reqCount} · 供出 ${(bytesServed / 1048576).toFixed(1)} MiB · 平均 ${(bytesServed / 1048576 / ((Date.now() - started) / 1000)).toFixed(3)} MB/s`);
}
process.on('SIGINT', () => { flush(); process.exit(0); });
process.on('SIGTERM', () => { flush(); process.exit(0); });
setInterval(flush, 5000);
