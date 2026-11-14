// 273 v32 探针：把源站的 cues 表拉下来，算清"落到某个时间点到底要多少字节"。
//
// 【为什么需要】真机 v31：远跳后第一帧前，内核读了 1.9 MB —— 但源站整体带宽
// 封顶 ~1.6 MB/s ⇒ 这 1.9 MB 就是 2.4 s 的停顿。
// 要判断这 1.9 MB 是不是"一个整簇"（⇒ 躲不掉），就得知道簇的真实大小。
//
// 【口径】cues 的 CueClusterPosition 是**相对 Segment 数据起点**的偏移；
// 簇大小 = 下一条 cue 的簇位置 − 本条 cue 的簇位置（同一个相对空间 ⇒ 相减即得）。
// token 只放在 X-Emby-Token 请求头里，绝不进 URL、绝不打印。
import fs from 'node:fs'

const TOKEN = fs.readFileSync('C:/Users/codex/AppData/Local/Temp/embywork/token.txt', 'utf8').trim()
const URL = 'https://emby.bangumi.ca/emby/Videos/233908/stream?static=true'
const CUES_ABS = 1935854740
const CUES_LEN = 130494

async function fetchRange(start, end) {
  const res = await fetch(URL, {
    headers: {
      'X-Emby-Token': TOKEN,
      Range: `bytes=${start}-${end}`,
    },
  })
  if (res.status !== 206 && res.status !== 200) {
    throw new Error(`HTTP ${res.status}`)
  }
  const buf = Buffer.from(await res.arrayBuffer())
  return buf
}

/** 读一个 EBML 元素头（ID + 长度）。返回 null = 缓冲不够。 */
function readElem(buf, off) {
  if (off >= buf.length) return null
  // 【ID 长度】由首字节的前导 0 个数决定：0x1C = 0001 1100 ⇒ 3 个前导 0 ⇒ 4 字节 ID。
  const first = buf[off]
  let idLen = 1
  let mask = 0x80
  while (idLen < 4 && (first & mask) === 0) { mask >>= 1; idLen += 1 }
  let idVal = 0
  for (let i = 0; i < idLen; i++) idVal = idVal * 256 + buf[off + i]
  let p = off + idLen
  if (p >= buf.length) return null
  // 【长度】VINT：标记位是首字节的**最高一个置位**（0x80/0x40/0x20/0x10/0x08）。
  const sFirst = buf[p]
  let sizeLen
  if ((sFirst & 0x80) !== 0) sizeLen = 1
  else if ((sFirst & 0x40) !== 0) sizeLen = 2
  else if ((sFirst & 0x20) !== 0) sizeLen = 3
  else if ((sFirst & 0x10) !== 0) sizeLen = 4
  else sizeLen = 5
  let size = 0
  for (let i = 0; i < sizeLen; i++) size = size * 256 + buf[p + i]
  size &= (sizeLen === 1 ? 0x7f : sizeLen === 2 ? 0x3fff : sizeLen === 3 ? 0x1fffff : sizeLen === 4 ? 0x0fffffff : 0x01ffffffff)
  return { id: idVal, size, dataOff: p + sizeLen, end: p + sizeLen + size }
}

function parseUint(buf, off, len) {
  let v = 0
  for (let i = 0; i < len; i++) v = v * 256 + buf[off + i]
  return v
}

const buf = await fetchRange(CUES_ABS, CUES_ABS + CUES_LEN - 1)
console.log(`cues 缓冲 ${buf.length} B`)

// 顶层：EBML(0x1A45DFA3) → Segment(0x18538067) → Cues(0x1C53BB6B) → CuePoint(0xBB)
let cuesEl = null
for (let off = 0; off < buf.length - 4; ) {
  const el = readElem(buf, off)
  if (!el) break
  if (el.id === 0x1c53bb6b) { cuesEl = el; break }
  off = el.end
}
if (!cuesEl) throw new Error('没找到 Cues 元素')

const cues = []
for (let off = cuesEl.dataOff; off < cuesEl.end; ) {
  const el = readElem(buf, off)
  if (!el) break
  if (el.id === 0xbb) {
    // CuePoint: CueTime(0xB3) + 若干 CueTrackPositions(0xB7){CueTrack(0xF7), CueClusterPosition(0xF1)}
    // 【为什么按轨分开存】一个 CuePoint 里每条轨一组，簇位置不同
    // （视频轨的关键帧稀、音轨密）⇒ 必须挑视频轨的那一组。
    let cueTime = null
    const perTrack = new Map()
    for (let p = el.dataOff; p < el.end; ) {
      const sub = readElem(buf, p)
      if (!sub) break
      if (sub.id === 0xb3) cueTime = parseUint(buf, sub.dataOff, sub.size)
      else if (sub.id === 0xb7) {
        let track = -1
        let cpos = null
        for (let q = sub.dataOff; q < sub.end; ) {
          const s2 = readElem(buf, q)
          if (!s2) break
          if (s2.id === 0xf7) track = parseUint(buf, s2.dataOff, s2.size)
          else if (s2.id === 0xf1) cpos = parseUint(buf, s2.dataOff, s2.size)
          q = s2.end
        }
        if (track >= 0 && cpos !== null) perTrack.set(track, cpos)
      }
      p = sub.end
    }
    if (cueTime !== null) cues.push({ t: cueTime, perTrack })
  }
  off = el.end
}
console.log(`cue 点 ${cues.length} 条`)

// 视频轨号：片头里视频轨是 1（真机 hexdump 里 f7 81 01）；这里取"出现最多的轨"。
const trackCount = new Map()
for (const c of cues) for (const [t] of c.perTrack) trackCount.set(t, (trackCount.get(t) || 0) + 1)
const videoTrack = [...trackCount.entries()].sort((a, b) => b[1] - a[1])[0][0]
console.log(`轨分布 ${[...trackCount.entries()].map(([t, n]) => `轨${t}:${n}`).join(' ')}　取轨 ${videoTrack} 作视频轨`)

const vpos = cues.map((c) => c.perTrack.get(videoTrack) ?? null)
const gaps = []
for (let i = 0; i + 1 < cues.length; i++) {
  if (vpos[i] === null || vpos[i + 1] === null) continue
  gaps.push(vpos[i + 1] - vpos[i])
}
gaps.sort((a, b) => a - b)
const pct = (p) => gaps[Math.min(gaps.length - 1, Math.floor(gaps.length * p))]
console.log(
  `视频簇间距 分位: p10=${pct(0.1)} p50=${pct(0.5)} p90=${pct(0.9)} p99=${pct(0.99)} max=${gaps[gaps.length - 1]}`,
)

// 落点时间 → 目标簇 + 它的大小
const targets = process.argv.slice(2).map(Number)
for (const sec of targets) {
  const ms = Math.round(sec * 1000)
  let idx = -1
  for (let i = 0; i < cues.length; i++) { if (cues[i].t <= ms) idx = i; else break }
  if (idx < 0) { console.log(`${sec}s 不在 cues 覆盖范围内`); continue }
  const here = vpos[idx]
  const size = idx + 1 < cues.length && vpos[idx + 1] !== null ? vpos[idx + 1] - here : null
  console.log(
    `落点 ${sec}s → cue#${idx} 时间 ${cues[idx].t} ms　簇偏移(rel) ${here}` +
      `　到下一簇 ${size} B` +
      (size !== null ? `　按 1.6 MB/s 需要 ${Math.round((size / 1.6e6) * 1000)} ms` : ''),
  )
}
