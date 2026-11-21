# SubtitleEncoding 单元测试 · 结果与一个被删掉的死分支（2026-11-22）

`android/app/src/test/java/com/kivo/cinema/playback/subtitle/SubtitleEncodingTest.kt`，
`./gradlew :app:testDebugUnitTest --tests "*SubtitleEncodingTest*"` ⇒ **9 tests / 0 failures / 0 errors**。

## 1. 用例（钉死探测顺序与不变量）

| 用例 | 断言 |
| --- | --- |
| `asciiIsUtf8Unchanged` | ASCII → `codec=UTF-8`、`converted=false`、字节不变 |
| `chineseUtf8Unchanged` | UTF-8 中文 → 同上 |
| `utf8BomIsPreserved` | `EF BB BF` **原样保留**（不替用户改写文件头） |
| `utf16LittleEndianIsConverted` | `FF FE` → `codec=UTF-16LE`、转码后中文正确 |
| `utf16BigEndianIsConverted` | `FE FF` → `codec=UTF-16BE` |
| `gb18030IsConverted` | GB18030 → `codec=GB18030`、转码后「中文字幕 GBK 转码测试」正确 |
| `illegalBytesStayUntouched` | `FF FF FF`（非法首字节）→ `codec=unknown`、**字节原样**（宁乱码不丢字幕） |
| `emptyIsUtf8` | 空输入 → `UTF-8`、`converted=false` |
| `big5BytesAreClaimedByGb18030` | **BIG5 字节得到的 `codec` 是 `GB18030`** ← 见 §2 |

## 2. 被单测抓出来的死分支：BIG5 —— 已删除

`big5BytesAreClaimedByGb18030` 断言 `codec == "GB18030"` 通过 ⇒ **BIG5 分支永远轮不到**。
这不是巧合，是可证的：

- BIG5 双字节：首 `0xA1–0xF9`，尾 `0x40–0x7E` 或 `0xA1–0xFE`
- GB18030 双字节：首 `0x81–0xFE`，尾 `0x40–0xFE`（除 `0x7F`）
- **首区间 ⊂ 首区间、尾区间 ⊂ 尾区间** ⇒ **每一段合法 BIG5 都是合法 GB18030**
- 判定顺序是 GB18030 在前，严格解码必然先成功 ⇒ **BIG5 命中率恒为 0**

按"不留空壳"，`LEGACY_CODECS` 里已删除 `BIG5`，并在源码注释里写明原因与未来路径。

**这不是能力收缩**：删之前它也从不生效（同样不支持），删之后只是不再"假装支持"。

## 3. 未来要真支持繁体 Big5，只有两条路

1. **导入时手选编码**（最实用）：在归一化入口接一个 `codec` 参数，用户选 BIG5 就用 BIG5 解；
   这条不依赖 mpv（我们已经在 Store 层做归一化，见 `ENC-DEVICE.md`）。
2. **统计模型**（uchardet 类）：引入第三方库 + 字频表，能自动区分，但多一条依赖与包体。

两条都需要独立一批，本轮不做。

## 4. 边界

- 单测跑在 **JVM**（`java.nio.charset` 的 GB18030 实现来自 JDK）；真机是 Android ICU。
  两者对 GB18030 的**严格解码边界**理论上可能不同 —— 真机那一侧已由 `ENC-DEVICE.md` 的端到端证据覆盖，
  但"某些病态字节序在 JVM 合法、在 ICU 非法"这种差异**没有专门比对**。
- `unknown` 分支的真机路径仍未构造真实坏字节文件验证（`ENC-DEVICE.md` §4 已登记）。
