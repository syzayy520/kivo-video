// 278 · 上屏帧率量具（SurfaceFlinger latency）
//
// 【为什么不用现有的 p273_gaps.mjs】
// 它靠 mpv render 回调计数 ⇒ **只对 render API 路径有效**；
// vo=gpu-next 模式下没有 render _callback（画面由 mpv 自己的 VO 直接上屏）
// ⇒ 统计恒为 0（278 的 B 臂就踩到了）。
//
// 【本工具的量法】SurfaceFlinger 的 `--latency <layer>` 返回：
//   第 1 行：刷新周期（ns）
//   之后每行：<presentTime> <> <desiredPresentTime>
//   presentTime = 该帧**真正上屏**的时刻（SurfaceFlinger 合成时刻）
// ⇒ 这是**用户眼睛看到的帧**，比任何回调计数都权威，且与出画通路无关。
//
// 【为什么要合并多次采样】
// latency 是 **128 帧的环形缓冲**（60 fps 下只覆盖约 2.1 秒），
// 一次 dump 只能看到最近 2 秒 ⇒ 必须反复采样。
// 多次采样之间有**重叠**（同一个 presentTime 会出现多次）⇒ 必须按时间戳去重。
//
// 【用法】
//   node tools/p274_sf_latency.mjs <raw.txt>
//   adb shell "dumpsys SurfaceFlinger --latency '<layer>'" >> raw.txt
//
// 【层名】SurfaceView[包名/Activity](BLAST)#<id>  ← 必须带 (BLAST) 与 id
import fs from 'fs';

function parse(raw) {
  const lines = raw.split(/\r?\n/);
  let refresh = 0;
  const set = new Set();
  for (const line of lines) {
    const t = line.trim();
    if (!t) continue;
    const cols = t.split(/\s+/);
    if (cols.length === 1) {
      // 刷新周期行（每次 dump 的第一行）
      const v = Number(cols[0]);
      if (Number.isFinite(v) && v > 0 && refresh === 0) refresh = v;
      continue;
    }
    const p = Number(cols[0]);
    // presentTime = 0 是 SurfaceFlinger 的填充行（没有真实帧）
    if (!Number.isFinite(p) || p === 0) continue;
    set.add(p);
  }
  return { refresh, ts: [...set].sort((a, b) => a - b) };
}

const file = process.argv[2];
const p = parse(fs.readFileSync(file, 'utf8'));
if (p.ts.length < 2) {
  console.log('no latency data in', file, `(${p.ts.length} 帧)`);
  process.exit(1);
}

const gaps = [];
for (let i = 1; i < p.ts.length; i++) gaps.push((p.ts[i] - p.ts[i - 1]) / 1e6);
const s = [...gaps].sort((a, b) => a - b);
const durMs = (p.ts[p.ts.length - 1] - p.ts[0]) / 1e6;
const fps = ((p.ts.length - 1) / durMs) * 1000;

console.log(
  `刷新周期 ${(p.refresh / 1e6).toFixed(2)}ms | 上屏帧 ${p.ts.length} | 时长 ${durMs.toFixed(0)}ms | ` +
    `**上屏帧率 ${fps.toFixed(2)} fps** | 帧间隔 p50 ${s[Math.floor(s.length * 0.5)].toFixed(1)}` +
    ` / p99 ${s[Math.floor(s.length * 0.99)].toFixed(1)} / max ${s[s.length - 1].toFixed(1)} ms | ` +
    `>200ms ${gaps.filter((g) => g > 200).length} 次`
);
