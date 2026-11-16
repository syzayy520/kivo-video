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
  const set = new Set();      // 全局去重帧：算帧率/时长/帧数
  const blocks = [];          // 每个采样块：算块内连续帧间隔
  let cur = [];
  const flush = () => { if (cur.length) { blocks.push(cur); cur = []; } };
  for (const line of lines) {
    const t = line.trim();
    // 采集脚本每次 dump 后追加空行 ⇒ 空行 = 块分隔
    if (!t) { flush(); continue; }
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
    cur.push(p);
  }
  flush();
  return { refresh, ts: [...set].sort((a, b) => a - b), blocks };
}

const file = process.argv[2];
const p = parse(fs.readFileSync(file, 'utf8'));
if (p.ts.length < 2) {
  console.log('no latency data in', file, `(${p.ts.length} 帧)`);
  process.exit(1);
}

// ★ 口径修正（289）：帧间隔只在**同一采样块内**的连续帧之间计算。
// latency 是 128 帧环形缓冲（60fps 约 2.1s）；若相邻两次 dump 间隔 > 缓冲容量，
// 中间一段帧从未被采到。旧版把「全局去重后排序」的相邻帧算 gap ⇒ 把这段漏采
// 误算成一个大 gap（假卡顿，彩排 A 臂 2 次 / B 臂 5 次全是这种）。
// 按「后帧 presentTime」去重 ⇒ 同一个真实帧间隔即使在多个重叠块里出现也只算一次。
const gapByFrame = new Map();
const ranges = [];
for (const b of p.blocks) {
  const s = [...new Set(b)].sort((a, b2) => a - b2);
  if (s.length < 1 || !Number.isFinite(s[0])) continue;
  ranges.push([s[0], s[s.length - 1]]);
  for (let i = 1; i < s.length; i++) gapByFrame.set(s[i], (s[i] - s[i - 1]) / 1e6);
}
const gaps = [...gapByFrame.values()];
const s = [...gaps].sort((a, b) => a - b);
const durMs = (p.ts[p.ts.length - 1] - p.ts[0]) / 1e6;
const fps = ((p.ts.length - 1) / durMs) * 1000;

// 采样覆盖诊断：相邻块「前块尾 → 后块头」为正 ⇒ 存在未采到任何帧的漏采窗口。
// 有漏采窗口时，>200ms 计数覆盖不到这些区间 ⇒ 不可据此写「0 卡顿」伪装健康（275）。
const holes = [];
const blockStarts = ranges.map((r) => r[0]).sort((a, b) => a - b);
const startsGap = [];
for (let i = 1; i < blockStarts.length; i++) startsGap.push((blockStarts[i] - blockStarts[i - 1]) / 1e9);
for (let i = 1; i < ranges.length; i++) {
  const holeMs = (ranges[i][0] - ranges[i - 1][1]) / 1e6;
  if (holeMs > 0) holes.push(holeMs);
}

console.log(
  `刷新周期 ${(p.refresh / 1e6).toFixed(2)}ms | 上屏帧 ${p.ts.length} | 时长 ${durMs.toFixed(0)}ms | ` +
    `**上屏帧率 ${fps.toFixed(2)} fps** | 帧间隔 p50 ${s[Math.floor(s.length * 0.5)].toFixed(1)}` +
    ` / p99 ${s[Math.floor(s.length * 0.99)].toFixed(1)} / max ${s[s.length - 1].toFixed(1)} ms | ` +
    `>200ms ${gaps.filter((g) => g > 200).length} 次`
);
console.log(
  `  [口径] 帧间隔/>200ms 仅取各采样块内连续帧(按后帧去重，跨块接缝不计)；帧率/时长取全局去重并集。` +
    `采样块 ${p.blocks.length} | 块起点间隔 max ${startsGap.length ? Math.max(...startsGap).toFixed(2) : '0'}s`
);
if (holes.length === 0) {
  console.log('  [覆盖] 漏采窗口 0 段 ⇒ 全窗口有采样数据，卡顿计数可信。');
} else {
  const sum = holes.reduce((a, b) => a + b, 0);
  console.log(
    `  [覆盖] ⚠ 漏采窗口 ${holes.length} 段 / 合计 ${(sum / 1000).toFixed(1)}s / 最长 ${Math.max(...holes).toFixed(0)}ms ⇒` +
      ` 这些区间无采样数据，>200ms 不含它们，不可据此写「0 卡顿」。请缩小采样间隔(<2.1s)重采。`
  );
}
