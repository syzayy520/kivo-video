# 257 Emby 链收敛 · 真机证据

设备：Redmi K40 `b6e950ee`（1080×2400，density 440）。APK md5：`1ddf28758609746cb1e49c38fdb7abc1`
（`android/app/build/outputs/apk/debug/app-debug.apk`，kivoDebug 固定签名）。
设计稿：`docs/design/ui/replica/emby-chain-convergence.md`（闸门定义与审计）。

凭据红线：本目录不含任何密码/token/含密码表单态。回归粘贴串仅无密码源
`https://emby.bangumi.ca:443`；token 离 URL 断言见下。

## 闸门结果

| # | 闸门 | 结果 | 帧 |
| --- | --- | --- | --- |
| R-1 | defaultHome 往返 | PASS | r1-01…r1-08（设→冷启 Emby→取消→冷启发现页） |
| R-1b | 隐藏开关往返 | PASS（恢复搁浅，见披露 4） | r1-09、r1-10 |
| R-2 | Emby 起播走 mpv 代理 + token 离 URL | PASS（冷首开瞬败披露 2） | r2-01-play.png |
| R-3 | 拖动预览 | 部分：拖动/目标时间/跟随 PASS；Emby 预览帧未出（披露 3，转 #40） | r3-01、r3-02 |
| R-4 | 钻取分页 | PASS | r4-01 浏览→r4-02 钻取首屏 60/207→r4-03 末屏 207/207+「已显示全部电影」 |
| R-5 | 添加流智能识别 | PASS | r5-01 类型 sheet→r5-02 表单→r5-03 解析→r5-04 选填 |
| R-6 | SMB 入口卡删除 | PASS | r6-01（本地/私密/已存 SMB 卡仍在） |

## 复算命令

```bash
# token 离 URL（播放一次后）：
adb -s b6e950ee shell run-as com.kivo.cinema cat files/mpv-kernel.log > k.log
grep -c -i -E 'bangumi|api_key|X-Emby|token=' k.log          # 必须 0
grep -o 'http://[^"?]*' k.log | sort -u                     # 必须仅 http://127.0.0.1:<port>/1
# 分页：钻取头计数随滚动增长（60→207），尾标「已显示全部电影」。
# 服务器 Range 契约（运行时凭据，不入库）：AuthenticateByName 取 token 后
#   curl -D - -o /dev/null -H "X-Emby-Token: $T" -H "Range: bytes=0-0" \
#     https://emby.bangumi.ca/Videos/<id>/stream?static=true   # 206 + Content-Range */总长
```

`r2-token-assertion.txt` / `r2-cold-failure-excerpt.txt` 为当次会话日志摘录（端口号随会话变）。

## 披露

1. SECURE 缺陷已修（EmbyActivity 读取回调驱动 FLAG_SECURE）；修前非隐藏源截图 0 字节。
2. Emby 冷起播首开瞬败：mp4 header EOF（1MB/0 seeks），重试恢复出画面；服务器与设备代理
   两侧 Range 实测均正确，根因未定论 → #40。
3. 预览内核冷超时后重试不重建，Emby 拖动预览窗显示「预览内核加载超时」（原因可见）→ #40。
4. nyamedia 隐藏态搁浅：设备无锁屏密码无法解锁隐藏源；恢复并入 #39（应用内独立数字密码）。
5. 环境：MIUI logcat 无应用日志；set-pin 被拒；.92f sheet 裁底部触摸区；重复账号守卫挡重加。
6. bangumi 带宽 ~400KB/s，4K 28.8GB 播放伴周期性 buffering（网络所致）。