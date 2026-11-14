// 复算 273 · 截图是不是"白屏"：解 PNG 取平均 RGB 与近白像素占比。
//
// 用法：node tools/p273_png_avg.mjs <a.png> [b.png ...]
//
// 【为什么需要它】"跳读到未缓存位置会不会白屏"这条判据，
// 光看 mpv 日志的"first video frame after restart shown"不够 ——
// 内核可能确实出了帧，但**那一帧是白的**（例如只解出了空画面）。
// 所以判据必须落到**像素**上：截图里的近白像素占比接近 1 ⇒ 白屏。
//
// 本机没有 Python，证据复算一律走 node（见 agents.md 工具限制）。

import { readFileSync } from 'node:fs';
import { inflateSync } from 'node:zlib';

const PNG_SIG = Buffer.from([0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a]);

function paeth(a, b, c) {
  const p = a + b - c;
  const pa = Math.abs(p - a);
  const pb = Math.abs(p - b);
  const pc = Math.abs(p - c);
  if (pa <= pb && pa <= pc) return a;
  if (pb <= pc) return b;
  return c;
}

function decodePng(buf) {
  if (!buf.subarray(0, 8).equals(PNG_SIG)) throw new Error('not a png');
  let width = 0;
  let height = 0;
  let bitDepth = 0;
  let colorType = 0;
  const idat = [];
  let pos = 8;

  while (pos < buf.length) {
    const len = buf.readUInt32BE(pos);
    const type = buf.toString('ascii', pos + 4, pos + 8);
    const data = buf.subarray(pos + 8, pos + 8 + len);
    pos += 12 + len; // len + type + data + crc

    if (type === 'IHDR') {
      width = data.readUInt32BE(0);
      height = data.readUInt32BE(4);
      bitDepth = data[8];
      colorType = data[9];
      if (data[12] !== 0) throw new Error('interlaced png unsupported');
    } else if (type === 'IDAT') {
      idat.push(data);
    } else if (type === 'IEND') {
      break;
    }
  }

  if (bitDepth !== 8) throw new Error(`bit depth ${bitDepth} unsupported`);
  const channels = { 0: 1, 2: 3, 4: 2, 6: 4 }[colorType];
  if (!channels) throw new Error(`color type ${colorType} unsupported`);

  const raw = inflateSync(Buffer.concat(idat));
  const stride = width * channels;
  const out = Buffer.alloc(height * stride);

  let src = 0;
  for (let y = 0; y < height; y++) {
    const filter = raw[src++];
    const line = raw.subarray(src, src + stride);
    src += stride;
    const cur = out.subarray(y * stride, (y + 1) * stride);
    const prev = y > 0 ? out.subarray((y - 1) * stride, y * stride) : null;

    for (let i = 0; i < stride; i++) {
      const a = i >= channels ? cur[i - channels] : 0;
      const b = prev ? prev[i] : 0;
      const c = prev && i >= channels ? prev[i - channels] : 0;
      let v = line[i];
      if (filter === 1) v += a;
      else if (filter === 2) v += b;
      else if (filter === 3) v += (a + b) >> 1;
      else if (filter === 4) v += paeth(a, b, c);
      cur[i] = v & 0xff;
    }
  }

  return { width, height, channels, pixels: out };
}

for (const file of process.argv.slice(2)) {
  const { width, height, channels, pixels } = decodePng(readFileSync(file));
  let r = 0;
  let g = 0;
  let b = 0;
  let nearWhite = 0;
  let opaque = 0;
  const total = width * height;

  for (let i = 0; i < total; i++) {
    const o = i * channels;
    const pr = pixels[o];
    const pg = pixels[o + 1];
    const pb = pixels[o + 2];
    r += pr;
    g += pg;
    b += pb;
    if (pr > 245 && pg > 245 && pb > 245) nearWhite++;
    if (channels === 4 || channels === 2) {
      if (pixels[o + (channels === 4 ? 3 : 1)] > 200) opaque++;
    } else {
      opaque++;
    }
  }

  const pct = (n) => ((n / total) * 100).toFixed(1);
  console.log(
    `${file.split(/[/\\]/).pop().padEnd(18)} ${width}x${height} ` +
      `avgRGB=${(r / total).toFixed(0)},${(g / total).toFixed(0)},${(b / total).toFixed(0)} ` +
      `nearWhite=${pct(nearWhite)}% opaque=${pct(opaque)}% ` +
      `${nearWhite / total > 0.98 ? '❌ 白屏' : '✅ 有内容'}`,
  );
}
