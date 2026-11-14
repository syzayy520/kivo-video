// 273 取证工具：raw screencap → PNG 裁剪（像素级取证用）。
//
// 用法：node tools/p273_band_crop.mjs <raw.bin> <out.png> <x1> <y1> <x2> <y2> [scale]
//
// 输入是 `adb exec-out screencap` 的原始字节：前 8 字节是宽高（LE uint32），
// 之后是 RGBA8888 像素。输出是裁剪后的 PNG（无缩放/按 scale 放大）。
//
// 【为什么要有这个工具】字幕"显示成豆腐块"这种事，日志与统计都判不了，
// 只能看像素。把裁剪做成可复算的脚本 ⇒ 任何人拿到同一份 raw 都能得到同一张 PNG，
// 不必信"当时屏幕上就是这样"。
//
// 【本机没有 Python 解释器】证据复算一律走 node（见 agents.md 工具限制）。

import { readFileSync, writeFileSync } from 'node:fs';
import { deflateSync } from 'node:zlib';

const [rawPath, out, X1, Y1, X2, Y2, SC] = process.argv.slice(2);
if (!rawPath || !out || X1 === undefined) {
  console.error('usage: node tools/p273_band_crop.mjs <raw.bin> <out.png> <x1> <y1> <x2> <y2> [scale]');
  process.exit(2);
}

const buf = readFileSync(rawPath);
const w = buf.readUInt32LE(0);
const h = buf.readUInt32LE(4);
const x1 = Number(X1), y1 = Number(Y1), x2 = Number(X2), y2 = Number(Y2);
const scale = SC ? Number(SC) : 1;
const cw = x2 - x1, ch = y2 - y1;
const ow = Math.round(cw * scale), oh = Math.round(ch * scale);

const raw = Buffer.alloc((ow * 3 + 1) * oh);
let p = 0;
for (let y = 0; y < oh; y++) {
  raw[p++] = 0; // PNG 每行的 filter 字节
  const sy = y1 + Math.min(ch - 1, Math.floor(y / scale));
  for (let x = 0; x < ow; x++) {
    const sx = x1 + Math.min(cw - 1, Math.floor(x / scale));
    const i = 16 + sy * w * 4 + sx * 4;
    raw[p++] = buf[i];
    raw[p++] = buf[i + 1];
    raw[p++] = buf[i + 2];
  }
}

let TBL = null;
function crc32(b) {
  if (!TBL) {
    TBL = [];
    for (let n = 0; n < 256; n++) {
      let c = n;
      for (let k = 0; k < 8; k++) c = c & 1 ? 0xedb88320 ^ (c >>> 1) : c >>> 1;
      TBL[n] = c;
    }
  }
  let c = 0xffffffff;
  for (let i = 0; i < b.length; i++) c = TBL[(c ^ b[i]) & 0xff] ^ (c >>> 8);
  return (c ^ 0xffffffff) >>> 0;
}

function chunk(type, data) {
  const len = Buffer.alloc(4);
  len.writeUInt32BE(data.length);
  const td = Buffer.concat([Buffer.from(type, 'ascii'), data]);
  const crc = Buffer.alloc(4);
  crc.writeUInt32BE(crc32(td) >>> 0);
  return Buffer.concat([len, td, crc]);
}

const ihdr = Buffer.alloc(13);
ihdr.writeUInt32BE(ow, 0);
ihdr.writeUInt32BE(oh, 4);
ihdr[8] = 8;  // bit depth
ihdr[9] = 2;  // color type = truecolor
const png = Buffer.concat([
  Buffer.from([0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a]),
  chunk('IHDR', ihdr),
  chunk('IDAT', deflateSync(raw)),
  chunk('IEND', Buffer.alloc(0)),
]);
writeFileSync(out, png);
console.log(`wrote ${out} ${ow}x${oh} (source ${w}x${h})`);
