// 复算 273 · 拖动落点的**关键路径**（真机证据）。
//
// 用法：node tools/p273_drag.mjs <dir> [<dir> ...]
//
// <dir> 里应该有：kernel.log（mpv 内核日志）、net.log（取数侧诊断）。
//
// 每目录输出一行关键指标：
//   · seek→首帧（毫秒）—— 用户真正感知的停顿
//   · 首帧前内核吃掉的字节 —— 决定"要下多少才能出画面"
//   · 落点预取次数 —— v26 的连接风暴判据（≤3 才算收口）
//   · cues 读：命中窗口 / 回源站 —— v28 的判据
//   · 落点窗口的源站表头耗时 —— 源站响应的下限
//
// 【为什么要有这个工具】"拖动之后多久出画面"这件事，
// 一次会话里散在两个文件、两种时间轴（内核相对秒 / 取数绝对毫秒）里。
// 手翻日志会看漏一格，而这一格往往就是结论。

import { readFileSync, existsSync } from 'node:fs';
import { join } from 'node:path';

const dirs = process.argv.slice(2);
if (dirs.length === 0) {
  console.error('usage: node tools/p273_drag.mjs <dir> [...]');
  process.exit(2);
}

const read = (p) => (existsSync(p) ? readFileSync(p, 'utf8').split(/\r?\n/) : []);

/** 内核日志里的时间戳（相对秒 → 毫秒）。 */
function kernelMs(line) {
  const m = line.match(/^\[\s*([\d.]+)\]/);
  return m ? Math.round(parseFloat(m[1]) * 1000) : null;
}

/** 取数侧日志的墙钟毫秒。 */
function wallMs(line) {
  const m = line.match(/^(\d+) /);
  return m ? Number(m[1]) : null;
}

for (const dir of dirs) {
  const klog = read(join(dir, 'kernel.log'));
  const nlog = read(join(dir, 'net.log'));

  // ---- 内核侧：最后一次 seek 与它之后的首帧 ----
  const seeks = [];
  for (const line of klog) {
    if (line.includes('queuing seek to')) {
      const t = kernelMs(line);
      if (t !== null) seeks.push({ t, target: (line.match(/queuing seek to ([\d.]+)/) || [])[1] });
    }
  }
  const frames = [];
  for (const line of klog) {
    if (line.includes('first video frame after restart shown')) {
      const t = kernelMs(line);
      if (t !== null) frames.push(t);
    }
  }
  const lastSeek = seeks.length ? seeks[seeks.length - 1] : null;
  const frameAfter = lastSeek ? frames.filter((t) => t > lastSeek.t).sort((a, b) => a - b)[0] : undefined;

  // ---- 内核侧：seek 期间内核自己在做什么（读 cues / 定位簇）----
  const cueParse = klog.filter((l) => l.includes('Parsing cues'));
  const streamSeek = klog.filter((l) => l.includes('stream level seek'));

  // ---- 取数侧：落点预取次数与 cues 命中 ----
  const landingPrefetch = nlog.filter((l) => l.includes('[seek] cues='));
  const landingWindow = nlog.filter((l) => l.includes('[seek] prefetch window='));
  const cuesPublish = nlog.filter((l) => l.includes('发布已就绪窗口'));
  const cuesHit = nlog.filter((l) => l.includes('[cues] cache hit'));
  // cues 在片尾的那一段字节（内核每次 seek 都要读它）
  let cuesAbs = null;
  for (const line of nlog) {
    const m = line.match(/cuesAbs=(\d+)/);
    if (m) cuesAbs = Number(m[1]);
  }
  // cues 的源站回源：play-cue = 我们自己解析索引那一次（合法）；
  // play = **内核那一次**（应当 0 —— v28 起内核从窗口拿）。
  const cueOrigin = nlog.filter((l) => l.includes('[play-cue] origin chunk')).length;
  const demuxOrigin = cuesAbs === null
    ? null
    : nlog.filter((l) => {
        if (!l.includes('[play] origin chunk')) return false;
        const m = l.match(/from=(\d+)/);
        if (!m) return false;
        const from = Number(m[1]);
        return from >= cuesAbs && from < cuesAbs + 4 * 1024 * 1024;
      }).length;
  // 落点窗口的源站表头耗时（那一跳的开销下限）
  // 【为什么收全部】v32 起落点窗口是**分片并行**的（每片一条连接）⇒
  // 表头不是一个数，是 K 个 ⇒ 取最早/最晚/最大三个，读者才知道瓶颈在哪片。
  let landingHdrFirst = null;
  let landingHdrLast = null;
  let landingHdrWall = null;
  let landingHdrWallLast = null;
  const landingHdrs = [];
  for (const line of nlog) {
    if (!line.includes('[play-seek] origin chunk')) continue;
    const m = line.match(/hdrMs=(\d+)/);
    if (m) {
      const hdr = Number(m[1]);
      landingHdrs.push(hdr);
      const w = wallMs(line);
      if (landingHdrWall === null || (w !== null && w < landingHdrWall)) landingHdrWall = w;
      if (landingHdrWallLast === null || (w !== null && w > landingHdrWallLast)) landingHdrWallLast = w;
    }
  }
  landingHdrFirst = landingHdrs.length ? Math.min(...landingHdrs) : null;
  landingHdrLast = landingHdrs.length ? Math.max(...landingHdrs) : null;
  const landOffset = landingPrefetch.length
    ? Number((landingPrefetch[landingPrefetch.length - 1].match(/偏移 (\d+)/) || [])[1])
    : 0;

  // ---- 两条时间轴的锚点 ----
  // 内核侧："stream level seek from … to <落点>" 与取数侧 "PROXY REQ range=bytes=<落点>-"
  // 是**同一个瞬间**（内核发起跳读 ⇒ 代理收到请求）⇒ 用它把内核相对时间换成墙钟。
  let anchor = null;
  for (const line of klog) {
    const m = line.match(/stream level seek from \d+ to (\d+)/);
    if (!m) continue;
    const offset = Number(m[1]);
    if (!landOffset || Math.abs(offset - landOffset) > 4096) continue;
    const t = kernelMs(line);
    if (t === null) continue;
    // 优先取主链路（cap=0）那一条 —— 它才是内核发起的跳读
    let fallback = null;
    for (const nline of nlog) {
      const nm = nline.match(/^(\d+) PROXY REQ range=bytes=(\d+)- cap=(\d+)/);
      if (!nm) continue;
      if (Math.abs(Number(nm[2]) - offset) > 4096) continue;
      const cand = { wall: Number(nm[1]), kernel: t, cap: Number(nm[3]) };
      if (cand.cap === 0) {
        anchor = cand;
        break;
      }
      if (!fallback) fallback = cand;
    }
    if (!anchor && fallback) anchor = fallback;
    if (anchor) break;
  }

  // ---- 首帧那一刻的墙钟 ----
  let seekWall = null;
  let frameWall = null;
  if (anchor && lastSeek) {
    seekWall = anchor.wall - (anchor.kernel - lastSeek.t);
    if (frameAfter !== undefined) frameWall = anchor.wall + (frameAfter - anchor.kernel);
  }

  // 首帧那一刻读侧已经走到的最大偏移 = "为了这一帧要下多少"
  let maxOffsetBeforeFrame = 0;
  let hitsBeforeFrame = 0;
  if (frameWall !== null) {
    for (const line of nlog) {
      const w = wallMs(line);
      if (w === null || w > frameWall) continue;
      const om = line.match(/\[seek\] cache hit offset=(\d+)/);
      if (!om) continue;
      hitsBeforeFrame += 1;
      maxOffsetBeforeFrame = Math.max(maxOffsetBeforeFrame, Number(om[1]));
    }
  }
  const bytesFromLanding =
    maxOffsetBeforeFrame && landOffset ? maxOffsetBeforeFrame - landOffset : null;
  // 整条时间线上落点窗口一共下了多少
  let windowBytes = 0;
  for (const line of nlog) {
    const m = line.match(/\[seek\] prefetch window=\d+\.\.(\d+) bytes=(\d+)/);
    if (m) windowBytes = Number(m[2]);
  }

  // ---- 落点窗口下载期间的**并发源站连接数** ----
  // 源站带宽是封顶的（Node 探针：1 条 1.67 MiB/s，4 条 1.15 MiB/s）⇒
  // 落点那一跳的表头延迟，多半是被**别人**挤出来的。
  let concurrentAtLanding = null;
  if (landingHdrWall !== null) {
    const win = 1500; // 落点表头前 1.5 s 内在飞的源站请求
    let n = 0;
    for (const line of nlog) {
      const m = line.match(/^\d+ \[\S+\] origin chunk from=\d+ code=\d+ hdrMs=\d+/);
      if (!m) continue;
      const w = wallMs(line);
      if (w === null || w < landingHdrWall - win || w > landingHdrWall) continue;
      n += 1;
    }
    concurrentAtLanding = n;
  }
  // 整场拖动里源站一共搬了多少字节（并发竞争的规模）
  let originBytes = 0;
  for (const line of nlog) {
    const m = line.match(/^\d+ \[\S+\] origin chunk from=\d+ code=\d+ hdrMs=\d+ len=(\d+)/);
    if (m) originBytes += Number(m[1]);
  }
  // 预览（thumb）自己回源站搬了多少字节。
  // 【为什么分两个数】预览**整场**都在读（起播时它要自己扫片头 + 读 cues），
  // 而 v30 的判据是"**拖动期间**预览不该再回源站"——
  // 两个数混在一起就会把起播那次的开销算到拖动头上（v30 第一版 A/B 就是这么错的）。
  let thumbBytes = 0;
  let thumbChunks = 0;
  let thumbBytesInDrag = 0;
  let thumbChunksInDrag = 0;
  for (const line of nlog) {
    if (!line.includes('[thumb] origin chunk')) continue;
    thumbChunks += 1;
    const m = line.match(/len=(\d+)/);
    const len = m ? Number(m[1]) : 0;
    thumbBytes += len;
    const w = wallMs(line);
    if (seekWall !== null && frameWall !== null && w !== null &&
        w >= seekWall && w <= frameWall) {
      thumbChunksInDrag += 1;
      thumbBytesInDrag += len;
    }
  }
  // 让路闸有没有真的开过（v29 的判据）
  // 【为什么两种文案都收】v29 记「让路：暂停预读」，v31 起记「让路：掐掉在飞的块」——
  // 判据是"闸开过"，文案是实现细节 ⇒ 两种都算。
  const yieldOn = nlog.filter((l) => l.includes('origin 让路')).length;
  const yieldOff = nlog.filter((l) => l.includes('恢复预读')).length;
  // 落点窗口的**有效传输速率**（字节/秒）—— 竞争激烈时它会掉到 1 MB/s
  let windowRate = null;
  let windowEndWall = null;
  let concurrentInWindow = null;
  if (landingHdrWall !== null) {
    let endWall = null;
    for (const line of nlog) {
      if (!line.includes('[seek] prefetch window=')) continue;
      const w = wallMs(line);
      if (w !== null && (endWall === null || w > endWall)) endWall = w;
    }
    windowEndWall = endWall;
    if (endWall !== null && endWall > landingHdrWall && windowBytes > 0) {
      windowRate = Math.round((windowBytes * 1000) / (endWall - landingHdrWall));
    }
    // 【落点窗口下载期间的并发源站请求】—— 头 1.5 s 那个数只覆盖"表头那一跳"，
    // 而落点窗口要下 4 MiB（真机 2–4 s）⇒ 竞争发生在**整段下载期**。
    let n = 0;
    for (const line of nlog) {
      const m = line.match(/^\d+ \[\S+\] origin chunk from=\d+ code=\d+ hdrMs=\d+/);
      if (!m) continue;
      const w = wallMs(line);
      if (w === null || w < landingHdrWall || w > (endWall ?? landingHdrWall)) continue;
      n += 1;
    }
    concurrentInWindow = n;
  }

  console.log(`── ${dir}`);
  console.log(
    `   seek→首帧        ${lastSeek && frameAfter !== undefined ? frameAfter - lastSeek.t : 'n/a'} ms` +
      `（落点 ${lastSeek ? lastSeek.target : '?'} s，共 ${seeks.length} 次 seek）`,
  );
  console.log(
    `   落点窗口         源站表头 ${landingHdrFirst !== null ? landingHdrFirst + '–' + landingHdrLast + ' ms（' + landingHdrs.length + ' 片）' : 'n/a'}` +
      `　表头到达前 1.5 s 内并发源站请求 ${concurrentAtLanding !== null ? concurrentAtLanding : 'n/a'} 条` +
      `　下载期内并发 ${concurrentInWindow !== null ? concurrentInWindow : 'n/a'} 条` +
      `　窗口共下 ${windowBytes.toLocaleString('en-US')} B` +
      `　有效速率 ${windowRate !== null ? (windowRate / 1024).toFixed(0) + ' KB/s' : 'n/a'}` +
      `　窗口耗时 ${windowEndWall !== null && landingHdrWall !== null ? windowEndWall - landingHdrWall : 'n/a'} ms`,
  );
  console.log(
    `   首帧前读到的字节 ${bytesFromLanding !== null ? bytesFromLanding.toLocaleString('en-US') + ' B' : 'n/a'}` +
      `（自落点偏移 ${landOffset.toLocaleString('en-US')} 起，窗口命中 ${hitsBeforeFrame} 次）`,
  );
  console.log(
    `   落点预取        ${landingPrefetch.length} 次` +
      `（v26 风暴判据 ≤3）　落点窗口 ${landingWindow.length} 个`,
  );
  console.log(
    `   cues 字节       发布窗口 ${cuesPublish.length}　窗口命中 ${cuesHit.length}` +
      `　源站回源：索引 ${cueOrigin}（合法）　内核 ${demuxOrigin !== null ? demuxOrigin : 'n/a'}（v28 后应为 0）`,
  );
  console.log(
    `   预览自己回源站  拖动期 ${thumbChunksInDrag} 块 / ${(thumbBytesInDrag / 1048576).toFixed(2)} MiB` +
      `（v30 判据：应接近 0）　整场 ${thumbChunks} 块 / ${(thumbBytes / 1048576).toFixed(2)} MiB` +
      `　让路闸 开 ${yieldOn} 次 / 关 ${yieldOff} 次`,
  );
  console.log(
    `   内核流级 seek   ${streamSeek.length} 次` +
      `　内核 cues 解析时刻 ${cueParse.length ? cueParse.map((l) => kernelMs(l)).join(', ') : 'n/a'} ms（内核时钟，非耗时）` +
      `　源站总搬 ${(originBytes / 1048576).toFixed(2)} MiB`,
  );
}
