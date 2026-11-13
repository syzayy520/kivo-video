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

## 六、v3 收口（统一画布 + 扁平纯白卡，2026-11-13 追加）

> 用户裁决链：否粉紫渐变底（"脏"）→ 否任何描边/投影（"黑边丑到爆"）→ 否近白卡（分组发糊）
> → 「一次设计到极致」→ 「资源库/我的底色同步换，要耐看」→ 「首页底色挺好的，统一用那个色」。
> 定稿：四页共用首页 resting 色 `#F2F2F7`（`DesignTokens.pageBackground`）+ 纯白扁平卡（无框无阴影）。

### 6.1 真机截图（`shots-v3/`，K40 b6e950ee）

| 文件 | 内容 |
| --- | --- |
| `v3-home.png` | 发现页保留环境渐变（职责不同，不统一） |
| `v3-sources.png` | 资源库：统一浅灰画布全 bleed（含状态栏/导航条） |
| `v3-profile.png` | 我的：同画布 |
| `v3-settings-root.png` | 设置根页上半：hero 30sp tnum + 4dp 条 + 2×2 chip 入口 + 双卡› |
| `v3-settings-root-bottom.png` | 根页下半：分组卡 + 分隔线内缩 + 版本脚注 |
| `v3-settings-playback.png` / `v3-playback-habits.png` | 播放子页：扁平头 + chip 卡分隔线 42 / 无 chip 卡 16 |

### 6.2 像素复算（ffmpeg crop 1×1 → rawvideo rgb24）

| 采样点 | RGB | 判定 |
| --- | --- | --- |
| 设置根页画布 (20,1200) | 242 242 247 | = #F2F2F7 ✓ |
| 资源库画布 (20,1200) | 242 242 247 | 同色 ✓ |
| 我的画布 (20,1200) | 242 242 247 | 同色 ✓ |
| 设置卡内 (538,444) | 255 255 255 | 纯白扁平 ✓ |

### 6.3 视觉层改动清单

- `AnimatedGradientBg(flatColor)`：外层全屏 Box 铺扁平色 ⇒ 状态栏/底栏同色全 bleed，无需改 inset；
  page 0/3/≥4 传 `pageBackground`，page 1/2 保留动画渐变。
- 设置家族 `GlassSurface` **清零**（root hero/grid/tiles + 6 子页文件全换 `SettingsCard`/`PlainCard`）。
- 分隔线 iOS grouped 惯例：**同卡统一**——含 chip 行的卡 `ChipCardDivider()`(42dp)，无 chip 卡 16dp；
  45 处调用换别名，其余保留。新增语义别名于 SettingsRowParts.kt（单职责：分隔线原子）。
- 字阶对齐：autoplay 行 16/12 + `#6B7280` 副题；chevron `#C6C9CE`；hero 数 30/600。

### 6.4 闸门核对（v3）

1. 编译闸：`build-p268-v3.log` BUILD SUCCESSFUL（读日志）。✓
2. 真机闸：三页画布像素同色 + 卡纯白 + 子页分隔线对照。✓
3. 写回回归：播放子页「每次从头播放」true→false→true 往返读 `playback_prefs.xml`，**还原原态 true**。✓
4. 行数闸：改动文件均 ≤300（SettingsRows 242 / SettingsRowParts ~105）。✓

### 6.5 披露（v3）

- 导航灰 pill + 蓝字**本轮不动**（四页共享 chrome，避免波及发现/在看页），留待独立卡。
- 资源库/我的页的内容卡沿用各自原有卡色（非设置家族），本轮只统一**画布**；设置家族卡纯白。
- 发现页/在看页保留动画渐变：环境层取色职责与静态画布不同，用户未要求统一这两页。