"""Kivo-W2：给 mpv 0.39 的 meson.build 打 Android iconv 补丁。

【为什么】
上游是 `iconv = dependency('iconv', required: get_option('iconv'))`。
Meson 探测 iconv 的最后一招是**链接 `-liconv`**；而 Android 的 iconv 在
**bionic libc** 里（API 28+），根本没有独立的 libiconv ⇒ 必然报
`ld.lld: error: unable to find library -liconv` / `Dependency "iconv" not found`。
真机日志见 docs/evidence/p9-subtitle-reaccept/W2-iconv-probe.log。

【怎么解】
只在 `host_machine.system() == 'android'` 时追加一次**真实转换**的链接测试：
GBK 的「中」(0xD6 0xD0) → UTF-8，跑通即视为 iconv 可用。
不是"假装可用"：若 API < 28（libc 没导出 iconv_*），这个测试会真失败。
其他平台完全走原逻辑。

【为什么不用 GNU libiconv】
它能用，但是 **LGPL**；静态链进闭源 APK 会带来重新链接义务。
bionic 的 iconv 是 BSD/Apache，GBK/GB18030/BIG5/Shift-JIS/EUC-* 都支持，够用。
"""
import sys

PATH = 'meson.build'

src = open(PATH, encoding='utf-8', errors='surrogateescape').read()

old = """iconv = dependency('iconv', required: get_option('iconv'))
features += {'iconv': iconv.found()}
if features['iconv']
    dependencies += iconv
endif
"""

new = """# Kivo-W2: Android 的 iconv 在 bionic libc（API 28+），没有独立 libiconv。
# 上游 `dependency('iconv')` 依赖 -liconv 链接测试，在 Android 上必然失败。
# 这里只对 Android 追加一次**真实转换**的测试（GBK「中」-> UTF-8），通过即视为可用；
# 其他平台逻辑与上游完全一致。
iconv = dependency('iconv', required: false)
kivo_iconv_opt = get_option('iconv')
if not iconv.found() and host_machine.system() == 'android'
    if kivo_iconv_opt.enabled() or kivo_iconv_opt.auto()
        kivo_iconv_cc = meson.get_compiler('c')
        if kivo_iconv_cc.links(
            '''#include <iconv.h>
               int main(void) {
                   iconv_t cd = iconv_open("UTF-8", "GBK");
                   if (cd == (iconv_t)-1) return 1;
                   char in[4] = { (char)0xD6, (char)0xD0, 0, 0 };
                   char out[16]; char *ip = in; char *op = out;
                   size_t il = 2, ol = sizeof(out);
                   size_t r = iconv(cd, &ip, &il, &op, &ol);
                   iconv_close(cd);
                   return (r == (size_t)-1) ? 1 : 0;
               }''',
            name: 'android bionic iconv (GBK -> UTF-8)')
            iconv = declare_dependency()
        endif
    endif
endif
if kivo_iconv_opt.enabled() and not iconv.found()
    error('Kivo-W2: -Diconv=enabled 但 Android bionic iconv 测试失败（API < 28？）')
endif
features += {'iconv': iconv.found()}
if features['iconv']
    dependencies += iconv
endif
"""

if old not in src:
    sys.exit('Kivo-W2: iconv block not found (mpv 版本不是 0.39.0？)')

src = src.replace(old, new, 1)
open(PATH, 'w', encoding='utf-8', errors='surrogateescape').write(src)
print('Kivo-W2: meson.build iconv patch applied')