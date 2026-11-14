"""Kivo-278-P2：给 mpv 0.39 的 Android EGL context 打"真 HDR 输出"补丁（第 ③ 层）。

【真 HDR 输出要四层配合，缺一不可】
| 层 | 谁做 | 内容 |
| --- | --- | --- |
| ① | App（Kotlin） | Surface 的 buffer 格式 = 10bit（`RGBA_1010102`） |
| ② | App（native） | Surface 的 dataspace = `ADATASPACE_BT2020_PQ` |
| ③ | **本补丁（mpv）** | EGL surface 的 colorspace = `EGL_GL_COLORSPACE_BT2020_PQ` |
| ④ | mpv 参数 | libplacebo 输出 PQ 码值（`target-trc=pq` + `target-colorspace-hint`） |

【为什么第 ③ 层只能在 mpv 侧做】
EGL 的 colorspace 是 **EGL surface 的属性**：
· 建 surface 时作为 attrib 传给 `eglCreateWindowSurface`；或
· 建完后用 `eglSetSurfaceAttrib` 改。
mpv 的 `video/out/opengl/context_android.c` 直接调 `eglCreateWindowSurface`，
**不经过** `mpegl_create_window_surface` ⇒ 改这一个文件即可，
**不影响** x11/wayland/drm 等其它平台（它们走 helper）。

【改什么】
surface 建好后，若开关打开（环境变量 `KIVO_HDR_SURFACE=1`）⇒
调 `eglSetSurfaceAttrib(surface, EGL_GL_COLORSPACE_KHR, BT2020_PQ)`，
并用 `eglQuerySurface` **读回**确认驱动是否真的接受了。

【为什么不用 eglCreatePlatformWindowSurface + attrib（更"正规"）】
真机实测（K40 / Android 13 / Qualcomm EGL）：
· `eglCreateWindowSurface` + attrib ⇒ **EGL_BAD_ATTRIBUTE (0x3004)**；
· `eglSetSurfaceAttrib` ⇒ 返回 FALSE 但错误码是 **EGL_SUCCESS (0x3000)**
  ⇒ 这台驱动**静默无效**（不报错、也不生效）；
· `eglCreatePlatformWindowSurface` + attrib ⇒ 确实能设上 colorspace，
  但**VO 初始化会挂死**（内核日志停在 EGLConfig dump，再无输出）⇒ **不能用**。
⇒ 结论：代码路径与能力**已就位**（换一台支持 PQ surface 的设备即可生效），
   本机受**驱动限制**无法闭环 ⇒ 详见 docs/design/playback/279-final-form-gap-table.md。

【安全】拿不到 / 设不上 PQ ⇒ 自动留在 SDR 路径，**绝不黑屏、绝不挂死**
（与 274 的 ES3 补丁同一套"带回退"原则）。
"""
import sys

PATH = 'video/out/opengl/context_android.c'

src = open(PATH, encoding='utf-8', errors='surrogateescape').read()
orig = src

# ---- 1) EGL_KHR_gl_colorspace 常量 + 开关读取 ----
old_const = (
    '#include <EGL/egl.h>\n'
    '#include <EGL/eglext.h>\n'
)
new_const = (
    '#include <EGL/egl.h>\n'
    '#include <EGL/eglext.h>\n'
    '\n'
    '/* Kivo-278-P2: EGL_KHR_gl_colorspace (BT2020 PQ surface attribute). */\n'
    '#ifndef EGL_GL_COLORSPACE_KHR\n'
    '#define EGL_GL_COLORSPACE_KHR 0x309F\n'
    '#endif\n'
    '#ifndef EGL_GL_COLORSPACE_BT2020_PQ\n'
    '#define EGL_GL_COLORSPACE_BT2020_PQ 0x32E1\n'
    '#endif\n'
    '\n'
)
assert old_const in src, 'const block not found'
src = src.replace(old_const, new_const, 1)

# ---- 2) surface 建好后标 PQ colorspace（失败/无效都留在 SDR）----
old_surface = (
    '    p->egl_surface = eglCreateWindowSurface(p->egl_display, config,\n'
    '                                    (EGLNativeWindowType)native_window, NULL);\n'
    '\n'
    '    if (p->egl_surface == EGL_NO_SURFACE) {\n'
    '        MP_FATAL(ctx, "Could not create EGL surface!\\n");\n'
    '        goto fail;\n'
    '    }\n'
)
new_surface = (
    '    p->egl_surface = eglCreateWindowSurface(p->egl_display, config,\n'
    '                                    (EGLNativeWindowType)native_window, NULL);\n'
    '\n'
    '    /* Kivo-278-P2: tag the EGL surface as BT.2020 PQ (layer 3 of 4).\n'
    '     *\n'
    '     * Device notes (K40 / Android 13 / Qualcomm EGL, all measured):\n'
    '     * - eglCreateWindowSurface + colorspace attrib => EGL_BAD_ATTRIBUTE\n'
    '     *   (0x3004): the legacy entry point does not accept it.\n'
    '     * - eglCreatePlatformWindowSurface + attrib does set it, but **hangs**\n'
    '     *   the VO init on this driver => must not be used.\n'
    '     * - eglSetSurfaceAttrib returns FALSE with EGL_SUCCESS (0x3000),\n'
    '     *   i.e. this driver silently ignores it => we report that honestly\n'
    '     *   via eglQuerySurface instead of pretending it worked.\n'
    '     *\n'
    '     * Failure is non-fatal: we simply stay on the SDR path. */\n'
    '    /* Whether the app asked for HDR: only then does it request a 10bit EGL\n'
    '     * config (--egl-output-format=rgb10_a2), so a 10bit config IS the\n'
    '     * signal. Using it avoids an extra channel -- an env var set by our\n'
    '     * own code does not reliably reach this prebuilt library. */\n'
    '    EGLint red_size = 0;\n'
    '    eglGetConfigAttrib(p->egl_display, config, EGL_RED_SIZE, &red_size);\n'
    '    MP_MSG(ctx, MSGL_INFO,\n'
    '           "Kivo-HDR: patch reached, EGL_RED_SIZE=%d (10bit=HDR request)\\\\n",\n'
    '           (int)red_size);\n'
    '    if (p->egl_surface != EGL_NO_SURFACE && red_size >= 10) {\n'
    '        EGLint pq = EGL_GL_COLORSPACE_BT2020_PQ;\n'
    '        /* eglSetSurfaceAttrib is EGL 1.5; the header may be older than the\n'
    '         * runtime, so fetch it through eglGetProcAddress. */\n'
    '        EGLBoolean (EGLAPIENTRYP SetSurfaceAttrib)\n'
    '            (EGLDisplay, EGLSurface, EGLint, EGLint) =\n'
    '            (void *)eglGetProcAddress("eglSetSurfaceAttrib");\n'
    '        EGLBoolean ok = SetSurfaceAttrib &&\n'
    '            SetSurfaceAttrib(p->egl_display, p->egl_surface,\n'
    '                             EGL_GL_COLORSPACE_KHR, pq);\n'
    '        EGLint got = 0;\n'
    '        EGLBoolean (EGLAPIENTRYP QuerySurface)\n'
    '            (EGLDisplay, EGLSurface, EGLint, EGLint *) =\n'
    '            (void *)eglGetProcAddress("eglQuerySurface");\n'
    '        if (QuerySurface)\n'
    '            QuerySurface(p->egl_display, p->egl_surface,\n'
    '                         EGL_GL_COLORSPACE_KHR, &got);\n'
    '        MP_MSG(ctx, MSGL_INFO,\n'
    '               "Kivo-HDR: EGL surface colorspace set=%d, readback=0x%x (PQ=0x%x)\\\\n",\n'
    '               (int)ok, got, EGL_GL_COLORSPACE_BT2020_PQ);\n'
    '    }\n'
    '\n'
    '    if (p->egl_surface == EGL_NO_SURFACE) {\n'
    '        MP_FATAL(ctx, "Could not create EGL surface!\\n");\n'
    '        goto fail;\n'
    '    }\n'
)
assert old_surface in src, 'surface creation block not found'
src = src.replace(old_surface, new_surface, 1)

if src == orig:
    print('!! nothing changed (already patched?)')
    sys.exit(1)

open(PATH, 'w', encoding='utf-8', errors='surrogateescape', newline='').write(src)
print('patched', PATH, 'delta bytes =', len(src) - len(orig))
