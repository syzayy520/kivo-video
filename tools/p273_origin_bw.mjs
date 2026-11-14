// 273 v32 探针：源站的**并发**到底能不能加速？
//
// 【为什么必须重测】之前那条"1 条 2.54 MB/s / 3 条 1.27 MB/s"的结论口径含糊
// （三条并发下的是同一段还是三段？），而它直接决定远跳能不能靠并发压时间。
// 真机 v31 的落点窗口 4.26 MiB 跑了 2.8–4.4 s（941–1462 KB/s），
// 而单连接的理论值是 ~2.5 MB/s ⇒ 差的那一截要么是并发互踩，要么是连接冷。
//
// 【口径】全部用**同一条 keep-alive 连接**做对照组，
// 另开臂对比"新建连接"与"复用连接"的表头与吞吐。
// token 只放在 X-Emby-Token 请求头里，绝不进 URL、绝不打印。
import fs from 'node:fs'

const TOKEN = fs.readFileSync('C:/Users/codex/AppData/Local/Temp/embywork/token.txt', 'utf8').trim()
const URL = 'https://emby.bangumi.ca/emby/Videos/233908/stream?static=true'

const BASE = 1_876_000_000
const CHUNK = 2 * 1024 * 1024

function headers(range) {
  return { 'X-Emby-Token': TOKEN, Range: `bytes=${range}` }
}

/** 读一段并计时（不落盘，只数字节）。 */
async function timed(label, start, end, agent) {
  const t0 = Date.now()
  let got = 0
  const res = await fetch(URL, { headers: headers(`${start}-${end}`), dispatcher: agent })
  const hdrMs = Date.now() - t0
  if (res.status !== 206) throw new Error(`HTTP ${res.status}`)
  const reader = res.body.getReader()
  for (;;) {
    const { done, value } = await reader.read()
    if (done) break
    got += value.length
  }
  const totalMs = Date.now() - t0
  return { label, got, hdrMs, totalMs, kb: Math.round((got * 1000) / totalMs / 1024) }
}

/** 起 N 条并发下载（每条自己一条连接）。 */
async function parallel(label, starts, ends) {
  const t0 = Date.now()
  const res = await Promise.all(starts.map((s, i) => timed(`${label}#${i}`, s, ends[i])))
  const totalMs = Date.now() - t0
  const got = res.reduce((a, b) => a + b.got, 0)
  console.log(
    `${label}: ${res.length} 条并发　总 ${(got / 1048576).toFixed(2)} MiB / ${totalMs} ms` +
      `　聚合 ${Math.round((got * 1000) / totalMs / 1024)} KB/s` +
      `　单条表头 ${res.map((r) => r.hdrMs).join('/')} ms` +
      `　单条速率 ${res.map((r) => r.kb).join('/')} KB/s`,
  )
  return { got, totalMs }
}

console.log('=== 臂 1：单连接，2 MiB ===')
for (const off of [BASE, BASE + 10_000_000, BASE + 20_000_000]) {
  const r = await timed(`单 ${off}`, off, off + CHUNK - 1)
  console.log(`  ${r.label}: ${(r.got / 1048576).toFixed(2)} MiB　表头 ${r.hdrMs} ms　总 ${r.totalMs} ms　${r.kb} KB/s`)
}

console.log('=== 臂 2：3 条并发，各自 2 MiB（不同位置）===')
await parallel('并发3段', [BASE, BASE + 10_000_000, BASE + 20_000_000], [
  BASE + CHUNK - 1, BASE + 10_000_000 + CHUNK - 1, BASE + 20_000_000 + CHUNK - 1,
])

console.log('=== 臂 3：3 条并发，把同一段 2 MiB 切成三份 ===')
const third = Math.floor(CHUNK / 3)
await parallel('并发切段', [BASE, BASE + third, BASE + 2 * third], [
  BASE + third - 1, BASE + 2 * third - 1, BASE + CHUNK - 1,
])

console.log('=== 臂 4：6 条并发，各 2 MiB（不同位置）===')
const six = [0, 1, 2, 3, 4, 5].map((i) => BASE + i * 10_000_000)
await parallel('并发6段', six, six.map((o) => o + CHUNK - 1))

console.log('=== 臂 5：同一条 keep-alive 连接顺序拉 3 段（复用 vs 新建）===')
const https = await import('node:https')
const agent = new https.Agent({ keepAlive: true, maxSockets: 8 })
function timedKeepAlive(start, end) {
  return new Promise((resolve, reject) => {
    const t1 = Date.now()
    const req = https.request(URL, { agent, headers: headers(`${start}-${end}`) }, (res) => {
      const hdrMs = Date.now() - t1
      let got = 0
      res.on('data', (c) => { got += c.length })
      res.on('end', () => resolve({ got, hdrMs, totalMs: Date.now() - t1 }))
    })
    req.on('error', reject)
    req.end()
  })
}
const t0 = Date.now()
for (const off of [BASE + 30_000_000, BASE + 40_000_000, BASE + 50_000_000]) {
  const r = await timedKeepAlive(off, off + CHUNK - 1)
  console.log(
    `  复用连接 ${off}: 表头 ${r.hdrMs} ms　总 ${r.totalMs} ms　${Math.round((r.got * 1000) / r.totalMs / 1024)} KB/s`,
  )
}
console.log(`  三段复用连接合计 ${Date.now() - t0} ms`)
agent.destroy()

console.log('=== 臂 6：落点窗口真实尺寸 4.26 MiB，1 / 3 / 6 路并发 ===')
const LAND = 4259840
for (const parts of [1, 3, 6]) {
  const piece = Math.ceil(LAND / parts)
  const starts = []
  for (let i = 0; i < parts; i++) starts.push(BASE + i * piece)
  const t = Date.now()
  const res = await Promise.all(
    starts.map((s, i) => timed(`L#${i}`, s, Math.min(s + piece - 1, BASE + LAND - 1))),
  )
  const got = res.reduce((a, b) => a + b.got, 0)
  const totalMs = Date.now() - t
  console.log(
    `  ${parts} 路: ${(got / 1048576).toFixed(2)} MiB / ${totalMs} ms　聚合 ${Math.round((got * 1000) / totalMs / 1024)} KB/s` +
      `　单条表头 ${res.map((r) => r.hdrMs).join('/')} ms`,
  )
}
