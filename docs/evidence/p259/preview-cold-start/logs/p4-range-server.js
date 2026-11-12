// 40 卡受控 Range 源（可复算版）：对指定 offset 的前 N 次请求回「空 206」（瞬态空响应），
// 之后回真实数据。用于复算 HttpByteSource 空 206 有界重试（257 首开瞬败根因③）。
// 用法：node p4-range-server.js <本地媒体文件> <日志输出文件> [端口=18099]
// 无任何凭据；仅本机回环监听。配合 `adb reverse tcp:<port> tcp:<port>` 供设备访问。
const http = require('http');
const fs = require('fs');
const file = process.argv[2];
const logfile = process.argv[3];
const port = parseInt(process.argv[4] || '18099', 10);
if (!file || !logfile) { console.error('usage: node p4-range-server.js <media> <log> [port]'); process.exit(2); }
const buf = fs.readFileSync(file);
const INJECT = new Set([0, 1048576]);   // 头部与 1MB 处注入空 206
const EMPTY_TIMES = 2;                  // 前 2 次空，第 3 次起真实（= MAX_EMPTY_RETRY）
const attempts = {};
const log = [];
function note(s) { log.push(s); console.log(s); }
function flush() { fs.writeFileSync(logfile, log.join('\n')); }
const srv = http.createServer((req, res) => {
  const range = req.headers['range'];
  if (req.method === 'HEAD') {
    res.writeHead(200, { 'Accept-Ranges': 'bytes', 'Content-Length': buf.length, 'Connection': 'close' });
    return res.end();
  }
  let start = 0, end = buf.length - 1;
  if (range && range.startsWith('bytes=')) {
    const spec = range.slice(6);
    const dash = spec.indexOf('-');
    start = parseInt(spec.slice(0, dash), 10) || 0;
    const to = spec.slice(dash + 1);
    if (to) end = Math.min(parseInt(to, 10), buf.length - 1);
  }
  const key = start;
  attempts[key] = (attempts[key] || 0) + 1;
  const n = attempts[key];
  const len = end - start + 1;
  const headers = {
    'Accept-Ranges': 'bytes',
    'Content-Range': `bytes ${start}-${end}/${buf.length}`,
    'Content-Type': 'video/x-matroska',
    'Connection': 'close',
  };
  if (INJECT.has(key) && n <= EMPTY_TIMES) {
    note(`EMPTY206 offset=${start} attempt=${n} len=${len}`);
    res.writeHead(206, Object.assign({ 'Content-Length': 0 }, headers));
    return res.end();
  }
  note(`DATA offset=${start} attempt=${n} len=${len}`);
  res.writeHead(206, Object.assign({ 'Content-Length': len }, headers));
  res.end(buf.slice(start, end + 1));
});
srv.listen(port, '127.0.0.1', () => note(`LISTEN ${port}`));
process.on('SIGTERM', () => { flush(); srv.close(); process.exit(0); });
setInterval(flush, 1000);