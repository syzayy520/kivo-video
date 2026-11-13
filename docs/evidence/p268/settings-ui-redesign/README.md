# p268 · 设置页 UI 重设计（浅色定稿）真机证据

> 卡：268 设置页 UI 重设计。设计稿 `docs/design/ui/replica/settings-ui-redesign.md`。
> 设备：Redmi K40 `b6e950ee`（1080×2400，HyperOS）。构建 `assembleDebug` BUILD SUCCESSFUL（读日志）。
> 性质：纯视觉层重设计；266/267 真实接线、持久化、导航栈零改动。

## 一、before / after 对照

| 文件 | 内容 |
| --- | --- |
| `shots/before-root.png` | 旧根页：10 张孤立白卡、零图标、字阶单调（"没 UI/杂乱"原状） |
| `shots/after-root.png` | 新根页上半：32sp 大标题 + 空间占用 hero + 2×2 入口卡 + 资源库双卡 |
| `shots/after-root-bottom.png` | 新根页下半：使用偏好 / 安全与关于分组 + 版本脚注 |
| `shots/after-playback.png` | 播放子页：淡紫 tint 返回钮 + 节头 12.5/600 + 主交互行 chip + 置灰原因完整可读 |

逐项对比草图 v2（`settings-redesign-mockup.html`）：
- 大标题 32/800 ✓；hero 堆叠条+图例+总量+管理› ✓；2×2 淡底 chip 入口 ✓；
- 资源库双卡（真实配置计数）✓；分组卡 ✓；版本脚注等宽数字 ✓；子页 tint 返回钮 ✓。

## 二、真实数据（非示意，真机实测）

- 空间占用 hero：**图片缓存 8.4 MB / 评分缓存 0 B**（`SettingsStore.imageCacheBytes/ratingCacheBytes` 实测）。
  堆叠条全蓝（评分=0），图例两色点；total=0 时显示空条不编默认。
- 资源库双卡：**Emby 1 个已配置 / SMB 1 个已配置**（accounts.bin 真实记录，`!hidden` 计数）。
  无实时连接探针 ⇒ 不显示假"在线"点（披露见设计稿 §五）。
- 版本脚注：**0.1.0-p12a (1)**（PackageManager 真实 versionName/versionCode，tnum 等宽）。

## 三、写回回归抽查（防视觉升级破坏接线）

升级 `SwitchRow` 签名（icon/iconColor/footnote 插在 onChange 之前，尾随 lambda 零改动）后：
1. 播放子页「每次从头播放」off→on，读 `shared_prefs/playback_prefs.xml` = `always_from_start=true`（写回生效）。
2. 再 on→off，读回 `always_from_start=false`（往返均持久化，设备已还原默认）。
结论：行原语视觉升级未破坏 266/267 持久化接线。

## 四、闸门核对

1. 行数闸：新/改 `.kt` 全部 ≤300（最大 SettingsRows 243）。✓
2. 编译闸：`assembleDebug` BUILD SUCCESSFUL（`build-p268-full.log`）。✓
   - 途中修一处 Compose 1.7.6 API：`ImageVector` 线 glyph 改用 `PathParser().parsePathString(s).toNodes()`
     + 成员 `addPath(List<PathNode>)`（String 扩展 `addPath` 在该版本不可导入）。
3. 真机闸：K40 before/after + 子页 + 写回往返。✓
4. 不静默降级：置灰行 reason 保留换行完整可读；hero/双卡只读真实字节/计数。✓
5. 零改动：AppearancePrefs/PlaybackPrefs/HomePreferences 读写、266 accounts 写回、SettingsNav 栈、播放器消费。✓

## 五、披露

- 子页图标为选择性加在主交互行（root 承载完整 chip 语言）；置灰解释行保持安静不堆 chip。
- 资源库双卡为配置计数概览，非实时连接状态；点按钻入数据与资源。
- 深色影院（Noctra 式）留待后续卡；本轮只浅色。