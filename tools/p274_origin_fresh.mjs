// 一次性探针：每块都**新建连接**取 4 MiB，打印每块速率。
// 对照 tmp-origin-soak.mjs（同一 keep-alive 连接连续取）⇒ 判定「复用是否导致吞吐衰减」。
import https from 'node:https';
import { readFileSync } from 'node:fs';

const token = readFileSync('C:/Users/codex/AppData/Local/Temp/embywork/token.txt', 'utf8').trim();
const URL_BASE = 'https://emby.bangumi.ca/emby/Videos/485/stream?static=true&MediaSourceId=aafdd622d6e1e15b27dfef460ffa95c8';
const CHUNK = 4 * 1024 * 1024;

function fetchRange(start) {
  return new Promise((resolve) => {
    const t0 = Date.now();
    let got = 0;
    let ttfb = -1;
    // 每块一个独立 agent：keepAlive=false ⇒ 每次都是新 TCP。
    const agent = new https.Agent({ keepAlive: false, maxSockets: 1 });
    const req = https.get(URL_BASE, {
      agent,
      headers: { 'X-Emby-Token': token, Range: `bytes=${start}-${start + CHUNK - 1}` },
    }, (res) => {
      ttfb = Date.now() - t0;
      res.on('data', (b) => { got += b.length; });
      res.on('end', () => { agent.destroy(); resolve({ got, ttfb, ms: Date.now() - t0 }); });
      res.on('error', () => { agent.destroy(); resolve({ got, ttfb, ms: Date.now() - t0 }); });
    });
    req.on('error', () => { agent.destroy(); resolve({ got, ttfb, ms: Date.now() - t0 }); });
  });
}

const base = Number(process.argv[2] || 228227666);
const count = Number(process.argv[3] || 6);
const parallel = Number(process.argv[4] || 1);
for (let i = 0; i < count; i += parallel) {
  const t0 = Date.now();
  const batch = [];
  for (let k = 0; k < parallel; k++) batch.push(fetchRange(base + (i + k) * CHUNK));
  const rs = await Promise.all(batch);
  const totalBytes = rs.reduce((a, r) => a + r.got, 0);
  const wall = Date.now() - t0;
  rs.forEach((r, k) => console.log(
    `i=${i + k} got=${(r.got / 1048576).toFixed(2)}MiB ttfb=${r.ttfb}ms total=${r.ms}ms speed=${(r.got / 1024 / (r.ms / 1000)).toFixed(0)}KB/s`));
  console.log(`  -- 批 ${i}${parallel > 1 ? `*${parallel}` : ''}: 墙钟 ${wall}ms 合计 ${(totalBytes / 1024 / (wall / 1000)).toFixed(0)}KB/s`);
}
