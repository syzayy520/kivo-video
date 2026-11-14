"""Kivo-274-HDR：给 mpv 0.39 的 EGL helper 打 ES3 补丁。

【为什么】libplacebo 的 HDR 峰值计算（pl_peak_detect）走 compute shader，
只有 GLES 3.1+ 才有。mpv 0.39 的 egl_helpers.c 把 ES 路径写死成
EGL_OPENGL_ES2_BIT + EGL_CONTEXT_CLIENT_VERSION=2 ⇒ libplacebo 自己禁掉
（日志："Disabling HDR peak computation (compute shaders=0, SSBO=1)"）。

【安全】三处都有回退：拿不到 ES3 config ⇒ 退 ES2；ES3 context 建不起来 ⇒ 退 ES2。
【影响面】这个文件只被 mpv **自己的 VO** 用（vo_gpu_next + gpu-context=android/egl）。
我们 App 现有链路是 render API，context 由我们自己的 mpv_egl_surface.cpp 建 ⇒ 完全不受影响。
"""
import io
import sys

PATH = 'video/out/opengl/egl_helpers.c'

src = open(PATH, encoding='utf-8', errors='surrogateescape').read()
orig = src

# ---- 1) ES 路径：请求 ES3 ----
old1 = (
    '        api = EGL_OPENGL_ES_API;\n'
    '        rend = EGL_OPENGL_ES2_BIT;\n'
    '        name = "GLES 2.x +";\n'
)
new1 = (
    '        api = EGL_OPENGL_ES_API;\n'
    '        /* Kivo-274-HDR: request ES3.\n'
    '         * libplacebo HDR peak detection (pl_peak_detect) needs compute shaders\n'
    '         * (GLES 3.1+). With an ES2 context libplacebo disables it.\n'
    '         * Falls back to ES2 below if no ES3 config/context is available. */\n'
    '        rend = EGL_OPENGL_ES3_BIT_KHR;\n'
    '        name = "GLES 3.x +";\n'
)
if old1 not in src:
    sys.exit('block1 not found')
src = src.replace(old1, new1, 1)

# ---- 2) config: ES3 选不到就退 ES2 ----
old2 = (
    '    if (!num_configs) {\n'
    '        talloc_free(configs);\n'
    '        MP_MSG(ctx, msgl, "Could not choose EGLConfig for %s!\\n", name);\n'
    '        return false;\n'
    '    }\n'
)
new2 = (
    '    if (!num_configs && es && rend == EGL_OPENGL_ES3_BIT_KHR) {\n'
    '        /* Kivo-274-HDR: no ES3 config on this driver -> fall back to ES2\n'
    '         * (no compute shaders, but we still get picture). */\n'
    '        rend = EGL_OPENGL_ES2_BIT;\n'
    '        attributes[3] = rend;\n'
    '        num_configs = 0;\n'
    '        if (!eglChooseConfig(display, attributes, NULL, 0, &num_configs))\n'
    '            num_configs = 0;\n'
    '        if (!eglChooseConfig(display, attributes, configs, num_configs, &num_configs))\n'
    '            num_configs = 0;\n'
    '        if (num_configs)\n'
    '            MP_MSG(ctx, msgl, "Falling back to ES2 (no ES3 config).\\n");\n'
    '    }\n'
    '\n'
    + old2
)
if old2 not in src:
    sys.exit('block2 not found')
src = src.replace(old2, new2, 1)

# ---- 3) context: ES 用版本 3，失败退 2 ----
old3 = (
    '        EGLint attrs[] = {\n'
    '            EGL_CONTEXT_FLAGS_KHR, ctx_flags,\n'
    '            es ? EGL_CONTEXT_CLIENT_VERSION : EGL_NONE, 2,\n'
    '            EGL_NONE\n'
    '        };\n'
    '\n'
    '        egl_ctx = eglCreateContext(display, config, EGL_NO_CONTEXT, attrs);\n'
    '        if (!egl_ctx)\n'
    '            egl_ctx = eglCreateContext(display, config, EGL_NO_CONTEXT, &attrs[2]);\n'
)
new3 = (
    '        EGLint attrs[] = {\n'
    '            EGL_CONTEXT_FLAGS_KHR, ctx_flags,\n'
    '            /* Kivo-274-HDR: ES path wants version 3 (compute shaders);\n'
    '             * fall back to 2 if the driver refuses. */\n'
    '            es ? EGL_CONTEXT_CLIENT_VERSION : EGL_NONE, 3,\n'
    '            EGL_NONE\n'
    '        };\n'
    '\n'
    '        egl_ctx = eglCreateContext(display, config, EGL_NO_CONTEXT, attrs);\n'
    '        if (!egl_ctx && es) {\n'
    '            attrs[3] = 2;\n'
    '            egl_ctx = eglCreateContext(display, config, EGL_NO_CONTEXT, attrs);\n'
    '        }\n'
    '        if (!egl_ctx)\n'
    '            egl_ctx = eglCreateContext(display, config, EGL_NO_CONTEXT, &attrs[2]);\n'
)
if old3 not in src:
    sys.exit('block3 not found')
src = src.replace(old3, new3, 1)

open(PATH, 'w', encoding='utf-8', errors='surrogateescape').write(src)
print('patched ok, delta bytes =', len(src) - len(orig))
