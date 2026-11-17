# B0 编译守门报告 · `:app:compileDebugKotlin`

- 任务：B0-T5（team task id 8）
- 时间：2026-11-16
- 工作目录：`C:\kivo video\android`
- 结论：**PASS**（既有构建未被 B0 骨架破坏，零 error，零修复）
- 环境：`JAVA_HOME=C:/Android/jdk-17.0.13+11`；`android/local.properties` 的 `sdk.dir=C:/Android/sdk`；Windows 11 / git-bash；Gradle 通过 `gradlew.bat` 调用

## 一、执行命令（按执行顺序，完整命令）

### 1）首轮：任务指定的离线命令

```bash
export JAVA_HOME="C:/Android/jdk-17.0.13+11"
./gradlew.bat :app:compileDebugKotlin --offline -q
```

| 项 | 值 |
| --- | --- |
| 退出码 | **0** |
| 输出 | 仅 `Picked up JAVA_TOOL_OPTIONS: -Dfile.encoding=UTF-8 ...`（`-q` 静默了任务日志） |
| 结果 | PASS |

`--offline` **未因缺依赖失败**，因此未触发"改跑不带 `--offline`"的兜底分支，两者不存在差异需要记录。

### 2）全量重跑（确认首轮不是"全 UP-TO-DATE 空转"）

```bash
export JAVA_HOME="C:/Android/jdk-17.0.13+11"
./gradlew.bat :app:compileDebugKotlin --offline --console=plain --rerun-tasks
```

| 项 | 值 |
| --- | --- |
| 退出码 | **0** |
| 尾行 | `BUILD SUCCESSFUL in 3m 39s` / `7 actionable tasks: 7 executed` |
| 耗时 | 3m 39s（含 Gradle Daemon 冷启动：日志 `Starting a Gradle Daemon, 3 incompatible and 5 stopped Daemons could not be reused`） |
| 结果 | PASS；`Task :app:compileDebugKotlin` 状态为 **executed**（非 UP-TO-DATE），即真的重新编译了整个 debug source set |

编译告警 18 条，**全部来自既有文件**（`MainActivity.kt`、`MainActivityLocalMedia.kt`、`EmbyClient.kt`、`MediaLibraryActivity.kt`、`PlaybackService.kt`、`VideoInterpolation.kt`、`PrivateActivity.kt`、`SmbDiscovery.kt`、`ui/detail/DetailActivity.kt`），内容均为 JDK/Android API 的 `is deprecated` 或既有的可空接收者告警。B0 新增的 11 个骨架文件**零 warning、零 error**。

### 3）增量复跑（自检基线）

```bash
export JAVA_HOME="C:/Android/jdk-17.0.13+11"
./gradlew.bat :app:compileDebugKotlin --offline --console=plain
```

| 项 | 值 |
| --- | --- |
| 退出码 | **0** |
| 尾行 | `BUILD SUCCESSFUL in 51s` / `7 actionable tasks: 7 up-to-date` |
| 耗时 | 53s wall（`BUILD SUCCESSFUL` 计 51s；绝大部分是 daemon 启动与配置阶段） |
| 结果 | PASS；`Task :app:compileDebugKotlin UP-TO-DATE` |

## 二、编译产物证据

B0 新增骨架确实进入编译产物（`android/app/build/intermediates/built_in_kotlinc/debug/compileDebugKotlin/classes/`）：

| 源文件 | 产物 |
| --- | --- |
| `ui/tokens/DesignPalette.kt` | `com/kivo/cinema/ui/tokens/DesignPalette.class`、`DesignPalette$Dark.class` |
| `ui/tokens/MotionTokens.kt` | `com/kivo/cinema/ui/tokens/MotionTokens*.class` |
| `ui/components/{HitBox,GlassSurface,SectionHeader,EmptyState,ErrorState,MediaCard}.kt` | `HitBoxKt.class`、`GlassSurfaceKt.class`、`SectionHeaderKt.class`（+`SectionLevel`）、`EmptyStateKt.class`、`ErrorStateKt.class`（+`ErrorKind`）、`MediaCardKt.class`（+`MediaCardVariant`） |
| `ui/motion/{MotionGate,HomeMotion}.kt` | `MotionGate.class`、`HomeMotion.class` |
| `ui/design/DesignTokens.kt`（仅改成员初始化） | `com/kivo/cinema/ui/design/DesignTokens.class` |

## 三、失败 / 修复项清单

**无。** 本轮零 error，未做任何代码修改：

| 文件:行 | 原因 | 修法 |
| --- | --- | --- |
| — | 无编译失败 | 无需修复；11 个骨架文件与 `DesignTokens.kt` 一字未改 |

`DesignTokens.kt` 的既有改动（工作区未提交）经确认属于 B0 既定范围，且**公开成员名与数值均未变**，只是成员初始化改为引用 `DesignPalette.Light.*`：

- 删除 `import androidx.compose.ui.graphics.Color`，新增 `import com.kivo.cinema.ui.tokens.DesignPalette`
- `bgLavender / bgPink / bgCream / textMain / textSub / accentGreen / dangerRed / canvasFill / groupedFill / pageBackground / hairline / washFill / navPill / navIdle / navActive / navPillDark / glassFallback` → 同名字段取自 `DesignPalette.Light`
- `navIdleDark` → `DesignPalette.Light.navIdleDark.copy(alpha = .66f)`（alpha 叠加保持原值）
- `glassAlpha`、`radiusCard`、`radiusPoster` 等 `const val` 未变

> 逆向一致性：`Color(0xFF...)` 字面量已全部搬到 `DesignPalette.Light`，`DesignTokens` 仍是唯一的对外读取点，旧调用方（`DesignComponents.kt`、各 Activity/Compose 页面）无需改动即通过编译 —— 这一点由本次 `--rerun-tasks` 全量编译 PASS 直接证明。

## 四、自检命令（如何复跑）

```bash
# 在 C:\kivo video\android 下执行；期望 EXIT=0 且出现 BUILD SUCCESSFUL
export JAVA_HOME="C:/Android/jdk-17.0.13+11"
./gradlew.bat :app:compileDebugKotlin --offline -q; echo "EXIT=$?"
```

一行版（含强制全量复算，用于"骨架是否真的没破坏构建"的复验）：

```bash
cd /c/kivo\ video/android && JAVA_HOME=C:/Android/jdk-17.0.13+11 ./gradlew.bat :app:compileDebugKotlin --offline --console=plain --rerun-tasks | tail -3; echo "EXIT=${PIPESTATUS[0]}"
```

判定标准：`EXIT=0` 且末行为 `BUILD SUCCESSFUL`；若出现 `e: file:///.../ui/{tokens,components,motion}/...`，即为骨架自身语法/类型错误（本次没有）。