// 复算 273-E · 预览浪费：按版本对照"取一帧实际下载了多少"。
//
// 用法：node tools/p273_preview_bytes.mjs <net-diag.log> [更多 net-diag.log ...]
//
// 【为什么单独一个脚本】预览的取数日志里 `[thumb]` 段与 `[play]` 段混在一起，
// 而"预览浪费"的口径是**预览内核自己拉了多少字节**——必须按 tag 分开算，
// 而且要分开"首次 seek 之前"与"之后"（seek 之前的那次是内核打开时的探测读）。
//
// 本机没有 Python，证据复算一律走 node（见 agents.md 工具限制）。

import { readFileSync } from 'node:fs';

const files = process.argv.slice(2);
if (!files.length) {
  console.error('usage: node tools/p273_preview_bytes.mjs <net-diag.log> [...]');
  process.exit(2);
}

for (const file of files) {
  const lines = readFileSync(file, 'utf8').split(/\r?\n/).filter(Boolean);
  const chunks = [];
  for (const line of lines) {
    const m = line.match(/^\d+ \[(\w+)\] origin chunk from=(\d+) code=(\d+) hdrMs=(\d+)(?: len=(\d+))?/);
    if (!m) continue;
    chunks.push({
      tag: m[1],
      from: Number(m[2]),
      code: Number(m[3]),
      hdr: Number(m[4]),
      len: m[5] !== undefined ? Number(m[5]) : null,
    });
  }

  const thumb = chunks.filter((c) => c.tag === 'thumb');
  const total = thumb.reduce((s, c) => s + (c.len ?? 0), 0);

  // 首个"明显越过 100 MB"的块 ⇒ 视为预览实例真正开始读目标位置（seek 落点附近）。
  // 阈值取 100 MB 是为了与片源头部（前 100 MB）明确分开。
  const pivot = thumb.find((c) => c.from > 100e6);
  const after = pivot ? thumb.filter((c) => c.from >= pivot.from) : [];
  const afterBytes = after.reduce((s, c) => s + (c.len ?? 0), 0);

  const play = chunks.filter((c) => c.tag === 'play');
  const playBytes = play.reduce((s, c) => s + (c.len ?? 0), 0);

  const short = file.split(/[/\\]/).pop();
  console.log(`[${short}]`);
  console.log(`  预览块数        : ${thumb.length}`);
  console.log(`  预览下载        : ${(total / 1048576).toFixed(1)} MiB`);
  console.log(`  首次 seek 之后  : ${(afterBytes / 1048576).toFixed(1)} MiB（${after.length} 块，落点 from=${pivot ? pivot.from : 'n/a'}）`);
  console.log(`  播放块数/字节   : ${play.length} / ${(playBytes / 1048576).toFixed(1)} MiB`);
}
