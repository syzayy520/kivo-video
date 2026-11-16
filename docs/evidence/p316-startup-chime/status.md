# 316 · 启动音效（设计稿的 chime）

状态：**已实现并在平板真机验证**
设备：MatePad `CFE6R21B12000179`
时间：2026-11-16 15:29–16:17

## 一、设计稿给的是什么

```js
function chime(at, vol){
  const g=actx.createGain(), f=actx.createBiquadFilter();
  f.type='lowpass'; f.frequency.value=900;
  g.gain.setValueAtTime(.0001,t);
  g.gain.linearRampToValueAtTime(vol,t+.04);        // 40 ms 起音
  g.gain.exponentialRampToValueAtTime(.0008,t+1.1); // 1.1 s 指数衰减
  [98,196,294].forEach((hz,i)=>{ ... m.gain.value=[1,.5,.2][i] ... });
}
// 调用：chime(R.c.sfx/speed, R.c.vol)  ⇒ 冷启动 sfx=0.70 s、vol=0.16
```

| 参数 | 值 |
| --- | --- |
| 基频 + 泛音 | **98 / 196 / 294 Hz**（幅度 1 / .5 / .2） |
| 滤波 | **lowpass 900 Hz** |
| 包络 | **40 ms 起音** + **1.1 s 指数衰减** |
| 音量 | **0.16** |
| 触发时刻 | 冷启动 **0.70 s**、热启动 **0.03 s** |

## 二、怎么落到 Android

**预生成 WAV，不用实时合成。**

理由：启动动画那 2.3 秒里主线程要推 138 帧，再叠一个振荡器合成不划算；
而这段声音只有 1.4 秒，WAV 才 **121 KB**，直接放 `res/raw/` 最省。

合成脚本（Python 标准库，无依赖）：

```python
partials = [(98.0, 1.0), (196.0, 0.5), (294.0, 0.2)]
# 40ms 线性起音 → 指数衰减（exp(-4.6k)，1.1s 时衰减到 ~1%）
# 再过一个一阶 RC 低通（fc=900Hz，α = dt/(RC+dt)）
# 峰值归一化到 0.35 留余量
```

产物：`android/app/src/main/res/raw/brand_chime.wav`（44.1 kHz / 16 bit / 单声道）。

**播放**：`ui/brand/StartupChime.kt`（`SoundPool`），单职责文件。

## 三、三条硬约束（设计稿明确要求，都已实现）

| 约束 | 实现 |
| --- | --- |
| **不抢占音频焦点** | 用 `SoundPool`（走 `STREAM_MUSIC` 混音），**不调** `requestAudioFocus` ⇒ 用户正在听歌时只是叠一声轻响，不会打断 |
| **遵循静音与勿扰** | `ringerMode != NORMAL`（静音/震动）或 `getStreamVolume(STREAM_MUSIC) ≤ 0` 时**不播** |
| **可以失败** | 加载失败、设备无声都只是"没声音"，`playIfAllowed()` 返回 false，**绝不影响动画** |

**为什么"可以失败"要写成约束**：音效是锦上添花，一旦它抛异常拖垮帧循环，
就会变成覆盖层卡死（313 踩过 State bug 的同类后果）。所以整条路径包了 `runCatching`。

## 四、验证（平板真机）

```
[brand] 启动动画开始 plan=FULL end=2.3
[brand] 音效 played=true（t=0.700033s）        ← 精确在 0.70 s 触发
[brand] 启动动画结束 elapsed=2.3000212 正常=true
```

**0.700033 s** —— 与设计稿的 `sfx: .7` 对齐（误差 0.03 ms，一帧内）。
`played=true` 说明 `SoundPool` 加载完成且**没有被静音守卫挡住**。

## 五、边界

1. **音色是近似**：用一阶 RC 低通代替 WebAudio 的 biquad（二阶），高频滚降斜率不同。
   听感接近（都是低频柔和的"叮"），但**不是逐样本等价**。
2. **静音/勿扰路径未实机验证**（只验了正常音量下能播）。逻辑有日志
   （`played=false` 会打出来），但没做过"开勿扰再启动"的对照。
3. **热启动音效未验证**：`WARM_SFX_AT = 0.03`，但热启动版动画 0.8 秒且当前策略下
   绝大多数启动走热启动 —— 需要单独清 SP 到"1 天内已播完整版"的状态才能触发。

## 六、复算命令

```bash
export JAVA_HOME="C:/Android/jdk-17.0.13+11"; export ANDROID_HOME="C:/Android/sdk"
export PATH="/c/Android/sdk/platform-tools:$PATH"; export MSYS_NO_PATHCONV=1
cd android && ./gradlew :app:assembleDebug --offline
adb -s CFE6R21B12000179 install -r "C:/kivo video/android/app/build/outputs/apk/debug/app-debug.apk"
adb -s CFE6R21B12000179 shell am force-stop com.kivo.cinema
# 清策略 ⇒ 判定为首装 ⇒ 完整版动画（带音效）
adb -s CFE6R21B12000179 shell "run-as com.kivo.cinema rm -f shared_prefs/kivo_startup_anim.xml"
adb -s CFE6R21B12000179 shell "run-as com.kivo.cinema sh -c ': > files/net-diag.log'"
adb -s CFE6R21B12000179 shell am start -n com.kivo.cinema/.MainActivity
sleep 6
adb -s CFE6R21B12000179 shell "run-as com.kivo.cinema cat files/net-diag.log" | grep brand
# 期望：音效 played=true（t≈0.70s）
```
