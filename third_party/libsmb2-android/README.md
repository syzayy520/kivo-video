# libsmb2-android（220 SMB 直连）

SenPlayer 同款 SMB2/3 客户端库（二进制证据：libMLibKit.so 内 "libsmb2 6.2" + SMB2_* 常量 443 处）。

## 来源

- 上游：https://github.com/sahlberg/libsmb2
- 源码目录：`../libsmb2-src/`（git clone，不入库；.gitignore 排除）
- 本卡使用的 commit：`a52c1571a1ff223bde5bb3df9fec7af5ce3855d4`（2026-11-06 master；
  复现：`git clone https://github.com/sahlberg/libsmb2 && git checkout a52c1571`）
- 许可：LGPL-2.1（`../libsmb2-src/COPYING`；静态链接保留本声明与上游源码可得性）

## 构建

```bash
bash third_party/libsmb2-android/build.sh        # 产物 build/libsmb2-android/arm64/libsmb2.a
```

开关：全可选依赖关（krb5/gssapi/dcerpc/quic/rdma），TLS 不启用 → 库内置加密
（md4/rc4/aes/sha，NTLMv2 与 SMB3 加密足够）。

## 关键行为（真机实证，见 220-smb-compat-report.md）

- **匿名认证**：password 保持 NULL（不调 smb2_set_password）→ auth.c L760 匿名分支，
  NTLMSSP_NEGOTIATE_ANONYMOUS (0x800) 标志。空字符串 ≠ NULL：空串会发
  "空用户+空密码 NTLMv2"，小米路由拒（0xc0000022）。
- 小米路由 ACL 实测：XiaoMi（USB 盘根）共享匿名可读；视频/音乐 共享匿名拒
  （需凭据，表单填写）。
- 端口：server 参数支持 "host:port"，缺省 445。
