// 一次性探针：同一 keep-alive 连接连续取 N 个 4 MiB 区块，打印每块速率。
// 目的：判定「源站对长连接的吞吐是否随时间衰减」—— 与真机 soak1080 的 0.08 MB/s 对照。
import https from 'node:https';
import { readFileSync } from 'node:fs';

const token = readFileSync('C:/Users/codex/AppData/Local/Temp/embywork/token.txt', 'utf8').trim();
const URL_BASE = 'https://emby.bangumi.ca/emby/Videos/485/stream?static=true&MediaSourceId=aafdd622d6e1e15b27dfef460ffa95c8';
const agent = new https.Agent({ keepAlive: true, maxSockets: 1 });
const CHUNK = 4 * 1024 * 1024;

function fetchRange(start) {
  return new Promise((resolve) => {
    const t0 = Date.now();
    let got = 0;
    let ttfb = -1;
    const req = https.get(URL_BASE, {
      agent,
      headers: { 'X-Emby-Token': token, Range: `bytes=${start}-${start + CHUNK - 1}` },
    }, (res) => {
      ttfb = Date.now() - t0;
      res.on('data', (b) => { got += b.length; });
      res.on('end', () => resolve({ start, got, ttfb, ms: Date.now() - t0 }));
      res.on('error', () => resolve({ start, got, ttfb, ms: Date.now() - t0 }));
    });
    req.on('error', () => resolve({ start, got, ttfb, ms: Date.now() - t0 }));
  });
}

const base = Number(process.argv[2] || 228227666);
const count = Number(process.argv[3] || 8);
for (let i = 0; i < count; i++) {
  const r = await fetchRange(base + i * CHUNK);
  console.log(`i=${i} got=${(r.got / 1048576).toFixed(2)}MiB ttfb=${r.ttfb}ms total=${r.ms}ms speed=${(r.got / 1024 / (r.ms / 1000)).toFixed(0)}KB/s`);
}
agent.destroy();
