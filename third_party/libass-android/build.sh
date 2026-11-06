#!/usr/bin/env bash
# 字幕渲染链 → Android arm64-v8a 交叉编译（212 字幕进渲染）。
#
# 依赖链：freetype 2.13.3（栅格化，FTL）→ fribidi 1.0.16（双向文本，LGPLv2.1）→
# harfbuzz 8.5.0（shaping，MIT——libass 0.16 起硬依赖，--disable-harfbuzz 已不存在）→
# libass 0.16.0（ASS/SSA/SRT 解析+光栅化，ISC）。
#
# 全部静态库；fontconfig/expat 不编（Android 无 fontconfig，字体走
# ass_add_font 内存注入）；harfbuzz 不带 icu/glib（libass 只用 shaping API）。
#
# 七个已踩平的坑（全部沉淀在本脚本里）：
#   ① MSYS2 make+sh 同运行时（32K 命令行，ffmpeg-android 同款）
#   ② MSYS2 登录 shell 重置 cwd——命令内必须显式 cd
#   ③ GitHub tag 归档顶层目录名不统一——解包前实测
#   ④ freetype 跨编译要宿主编译器（CC_BUILD）→ zig cc（稳定路径包装 exe）
#   ⑤ MSYS2 的 msys-python meson 自检拒绝运行 → Windows Python 的 meson
#      全路径直呼；PATH 放 NDK clang 让 vsenv 自检走"跳过"分支
#      （否则 vswhere 在 None root 上炸 "Unhandled python exception"）
#   ⑥ android 宿主下 meson 的 prefix 是设备语义（'C:/...' 不算绝对路径）——
#      prefix='/usr' + DESTDIR 落盘；而 DESTDIR 又被 Windows meson 吞 →
#      手动落盘（cp 产物 + 手写 .pc）
#   ⑦ autotools 装出的 .pc 是 MSYS 风格路径 → 交给 Windows meson/pkgconf
#      前必须重写成 Windows 路径
#
# 前提：源码 tarball 在 /c/re-tools/libass-src/（LIBASS_SRC_DIR 可覆盖）；
# 从 MSYS2 bash 里跑：/c/msys64/usr/bin/bash -lc 'cd "/c/kivo video" && bash ...'
set -euo pipefail

NDK="${NDK:-/c/Android/sdk/ndk/30.0.16248370}"
HERE="$(cd "$(dirname "$0")" && pwd)"
SRC_DIR="${LIBASS_SRC_DIR:-/c/re-tools/libass-src}"
API=26
DIST_WIN="C:/re-tools/libass-android/dist"
DIST="${LIBASS_DIST_DIR:-/c/re-tools/libass-android/dist}"
BUILD_ROOT="${LIBASS_BUILD_DIR:-/c/re-tools/libass-android/build}"

TOOLCHAIN="$NDK/toolchains/llvm/prebuilt/windows-x86_64"
CC="$TOOLCHAIN/bin/clang.exe --target=aarch64-linux-android$API"
AR="$TOOLCHAIN/bin/llvm-ar.exe"
RANLIB="$TOOLCHAIN/bin/llvm-ranlib.exe"
STRIP="$TOOLCHAIN/bin/llvm-strip.exe"
READELF="$TOOLCHAIN/bin/llvm-readelf.exe"
SYSROOT="$TOOLCHAIN/sysroot"
MESON_EXE="${MESON_EXE:-C:/Users/Administrator/AppData/Local/Programs/Python/Python310/Scripts/meson.exe}"

mkdir -p "$BUILD_ROOT" "$DIST"

# 宿主编译器（freetype 的 apinames 等构建机工具）：zig cc，稳定路径包装。
mkdir -p /c/re-tools/host-cc
ZIG_EXE="${ZIG_EXE:-$(ls /c/re-tools/zig-windows-x86_64-*/zig.exe 2>/dev/null | head -1 || true)}"
[ -n "$ZIG_EXE" ] || { echo "找不到 zig.exe（宿主编译器）"; exit 2; }
if [ ! -x /c/re-tools/host-cc/zigcc.exe ]; then
    printf '#!/usr/bin/env bash\nexec "%s" cc "$@"\n' "$ZIG_EXE" > /c/re-tools/host-cc/zigcc
    chmod +x /c/re-tools/host-cc/zigcc
    export ZIG_GLOBAL_CACHE_DIR="${ZIG_GLOBAL_CACHE_DIR:-/c/re-tools/libass-android/zig-cache}"
    export ZIG_LOCAL_CACHE_DIR="$ZIG_GLOBAL_CACHE_DIR"
    "$ZIG_EXE" cc /c/re-tools/host-cc/zigcc.c -target x86_64-windows-gnu \
        -o /c/re-tools/host-cc/zigcc.exe
fi
export CC_BUILD=/c/re-tools/host-cc/zigcc.exe
export ZIG_GLOBAL_CACHE_DIR="${ZIG_GLOBAL_CACHE_DIR:-/c/re-tools/libass-android/zig-cache}"

configure_pkg() { # $1=tarball $2=解包名 $3=期望产物 $4+=额外 configure 参数
    local tarball="$1" name="$2" expected="$3"
    shift 3
    if [ -f "$DIST/lib/$expected" ]; then
        echo "== $name 已构建（$expected 在 dist），跳过 =="
        return 0
    fi
    # 顶层目录名从 tarball 实测（③：GitHub tag 命名不统一）。
    local top="$(tar -tf "$SRC_DIR/$tarball" 2>/dev/null | head -1 | cut -d/ -f1)"
    local src="$BUILD_ROOT/$name"
    rm -rf "$src" "$BUILD_ROOT/$top"
    mkdir -p "$src"
    # 容错解包：上游 tarball 里的符号链接（如 libass 的 README→README.md）在
    # Windows 无权限创建，tar 会报错退出——但缺 README 不影响构建，继续用
    # 解出来的其余文件；若缺的是关键文件，configure 阶段自然会暴露。
    tar -xf "$SRC_DIR/$tarball" -C "$src" --strip-components=1 \
        2> >(grep -v "无法创建\|symbolic link" >&2) || true
    cd "$src"
    echo "== configure $name =="
    PATH="$TOOLCHAIN/bin:$PATH" ./configure \
        --prefix="$DIST" \
        --host=aarch64-linux-android \
        --enable-static --disable-shared \
        --with-sysroot="$SYSROOT" \
        CC="$CC" AR="$AR" RANLIB="$RANLIB" STRIP="$STRIP" \
        "$@" > "$BUILD_ROOT/$name.configure.log" 2>&1 \
        || { echo "configure $name 失败"; tail -20 "$BUILD_ROOT/$name.configure.log"; exit 1; }
    echo "== make $name =="
    PATH="$TOOLCHAIN/bin:$PATH" make -j8 > "$BUILD_ROOT/$name.make.log" 2>&1 \
        || { echo "make $name 失败"; tail -20 "$BUILD_ROOT/$name.make.log"; exit 1; }
    make install > /dev/null 2>&1
    echo "== $name 装好 =="
}

# fribidi/harfbuzz 共用的 meson 交叉文件（c 必给；cxx 可选传参）。
# **路径必须 Windows 形式**（C:/...）——meson 是原生 Windows 进程，
# 读到 /c/... 形式会报 "Unknown compiler / WinError 2"。
write_meson_files() { # $1=name $2=带cxx与否(yes/no)
    local name="$1" with_cxx="$2"
    local tcwin="C:/Android/sdk/ndk/30.0.16248370/toolchains/llvm/prebuilt/windows-x86_64"
    local cross="$BUILD_ROOT/$name.cross.ini"
    if [ "$with_cxx" = yes ]; then
        cat > "$cross" << CROSSFILE
[binaries]
c = '$tcwin/bin/clang.exe'
cpp = '$tcwin/bin/clang++.exe'
ar = '$tcwin/bin/llvm-ar.exe'
strip = '$tcwin/bin/llvm-strip.exe'
pkg-config = 'C:/re-tools/host-cc/pkgconfw.exe'
[properties]
c_args = ['--target=aarch64-linux-android$API', '-fPIC']
cpp_args = ['--target=aarch64-linux-android$API', '-fPIC']
c_link_args = ['--target=aarch64-linux-android$API']
cpp_link_args = ['--target=aarch64-linux-android$API']
[host_machine]
system = 'android'
cpu_family = 'aarch64'
cpu = 'aarch64'
endian = 'little'
[built-in options]
prefix = '/usr'
default_library = 'static'
CROSSFILE
    else
        cat > "$cross" << CROSSFILE
[binaries]
c = '$tcwin/bin/clang.exe'
ar = '$tcwin/bin/llvm-ar.exe'
strip = '$tcwin/bin/llvm-strip.exe'
[properties]
c_args = ['--target=aarch64-linux-android$API', '-fPIC']
c_link_args = ['--target=aarch64-linux-android$API']
[host_machine]
system = 'android'
cpu_family = 'aarch64'
cpu = 'aarch64'
endian = 'little'
[built-in options]
prefix = '/usr'
default_library = 'static'
CROSSFILE
    fi
    local native="$BUILD_ROOT/$name.native.ini"
    cat > "$native" << NATIVEFILE
[binaries]
c = 'C:/re-tools/host-cc/zigcc.exe'
NATIVEFILE
}

# meson 家族通用的收尾：产物落 dist + Windows 路径版 .pc。
# （meson 的 DESTDIR 在 Windows 上不可靠——MSYS 路径被吞，实测装丢。）
# $1=name $2=构建目录里 .a 的相对路径 $3=头文件相对路径(构建侧) $4=头文件相对路径(源侧)
# $5=pc 名 $6=pc 版本 $7=pc 说明
meson_install_manual() {
    local name="$1" lib_rel="$2" inc_build="$3" inc_src="$4" pc_name="$5" pc_ver="$6" pc_desc="$7"
    mkdir -p "$DIST/lib" "$DIST/lib/pkgconfig" "$DIST/include"
    cp "$BUILD_ROOT/$name/$name.build/$lib_rel" "$DIST/lib/"
    mkdir -p "$DIST/include/$pc_name"
    cp "$BUILD_ROOT/$name/$inc_build"/*.h "$DIST/include/$pc_name/" 2>/dev/null || true
    cp "$BUILD_ROOT/$name/$inc_src"/*.h "$DIST/include/$pc_name/" 2>/dev/null || true
    cat > "$DIST/lib/pkgconfig/$pc_name.pc" << PCFILE
Name: $pc_name
Description: $pc_desc (manual install for the 212 chain)
Version: $pc_ver
Libs: -L$DIST_WIN/lib -l$pc_name
Cflags: -I$DIST_WIN/include/$pc_name
PCFILE
}

# 1. freetype：libass 的最小需求是核心栅格化；可选依赖全关。
configure_pkg "freetype-2.13.3.tar.xz" "freetype-2.13.3" "libfreetype.a" \
    --with-zlib=no --with-bzip2=no --with-png=no --with-harfbuzz=no --with-brotli=no

# 2. fribidi：双向文本。
if [ ! -f "$DIST/lib/libfribidi.a" ]; then
    write_meson_files "fribidi-1.0.16" no
    rm -rf "$BUILD_ROOT/fribidi-1.0.16"
    mkdir -p "$BUILD_ROOT/fribidi-1.0.16"
    tar -xf "$SRC_DIR/fribidi-1.0.16.tar.xz" -C "$BUILD_ROOT/fribidi-1.0.16" --strip-components=1
    cd "$BUILD_ROOT/fribidi-1.0.16"
    echo "== meson setup fribidi =="
    PATH="$TOOLCHAIN/bin:$PATH" "$MESON_EXE" setup fribidi.build \
        --native-file "$BUILD_ROOT/fribidi-1.0.16.native.ini" \
        --cross-file "$BUILD_ROOT/fribidi-1.0.16.cross.ini" \
        > "$BUILD_ROOT/fribidi.meson.log" 2>&1 \
        || { echo "meson setup fribidi 失败"; tail -20 "$BUILD_ROOT/fribidi.meson.log"; exit 1; }
    echo "== ninja fribidi =="
    PATH="$TOOLCHAIN/bin:$PATH" ninja -C fribidi.build > "$BUILD_ROOT/fribidi.ninja.log" 2>&1 \
        || { echo "ninja fribidi 失败"; tail -20 "$BUILD_ROOT/fribidi.ninja.log"; exit 1; }
    # 手动落盘（⑥：DESTDIR 不可靠）。头文件：源侧 lib/*.h + 生成侧 lib/*.h。
    mkdir -p "$DIST/lib" "$DIST/include/fribidi" "$DIST/lib/pkgconfig"
    cp fribidi.build/lib/libfribidi.a "$DIST/lib/"
    cp lib/*.h "$DIST/include/fribidi/" 2>/dev/null || true
    cp fribidi.build/lib/*.h "$DIST/include/fribidi/" 2>/dev/null || true
    cat > "$DIST/lib/pkgconfig/fribidi.pc" << PCFILE
Name: fribidi
Description: GNU FriBidi (manual install for the 212 chain)
Version: 1.0.16
Libs: -L$DIST_WIN/lib -lfribidi
Cflags: -I$DIST_WIN/include/fribidi
PCFILE
    echo "== fribidi 装好 =="
else
    echo "== fribidi 已构建，跳过 =="
fi

# 3. harfbuzz：shaping（libass 0.16 硬依赖）。freetype 经 pkg-config 发现
#    （.pc 已重写为 Windows 路径）；icu/glib 全关——libass 只用 shaping API。
if [ ! -f "$DIST/lib/libharfbuzz.a" ]; then
    # freetype.pc 重写为 Windows 路径（autotools 装出来的是 /c/... 形式，⑦）。
    mkdir -p "$DIST/lib/pkgconfig"
    cat > "$DIST/lib/pkgconfig/freetype.pc" << PCFILE
Name: FreeType 2
Description: FreeType (rewritten for Windows meson/pkgconf)
Version: 23.2.17
Libs: -L$DIST_WIN/lib -lfreetype
Cflags: -I$DIST_WIN/include/freetype2
PCFILE
    write_meson_files "harfbuzz-8.5.0" yes
    rm -rf "$BUILD_ROOT/harfbuzz-8.5.0"
    mkdir -p "$BUILD_ROOT/harfbuzz-8.5.0"
    tar -xf "$SRC_DIR/harfbuzz-8.5.0.tar.xz" -C "$BUILD_ROOT/harfbuzz-8.5.0" --strip-components=1 \
        2> >(grep -v "无法创建\|symbolic link" >&2) || true
    cd "$BUILD_ROOT/harfbuzz-8.5.0"
    echo "== meson setup harfbuzz =="
    PKG_CONFIG_PATH="$DIST_WIN/lib/pkgconfig" \
    PATH="$TOOLCHAIN/bin:$PATH" "$MESON_EXE" setup harfbuzz.build \
        --native-file "$BUILD_ROOT/harfbuzz-8.5.0.native.ini" \
        --cross-file "$BUILD_ROOT/harfbuzz-8.5.0.cross.ini" \
        -Dfreetype=enabled -Dicu=disabled -Dglib=disabled -Dgobject=disabled \
        -Dtests=disabled -Ddocs=disabled -Dbenchmark=disabled -Dintrospection=disabled \
        > "$BUILD_ROOT/harfbuzz.meson.log" 2>&1 \
        || { echo "meson setup harfbuzz 失败"; tail -25 "$BUILD_ROOT/harfbuzz.meson.log"; exit 1; }
    echo "== ninja harfbuzz =="
    PKG_CONFIG_PATH="$DIST_WIN/lib/pkgconfig" \
    PATH="$TOOLCHAIN/bin:$PATH" ninja -C harfbuzz.build \
        > "$BUILD_ROOT/harfbuzz.ninja.log" 2>&1 \
        || { echo "ninja harfbuzz 失败"; tail -25 "$BUILD_ROOT/harfbuzz.ninja.log"; exit 1; }
    # 手动落盘（⑥）。头：src/hb*.h + src/harfbuzz/ 子目录。
    mkdir -p "$DIST/lib" "$DIST/include"
    cp harfbuzz.build/src/libharfbuzz.a "$DIST/lib/"
    cp src/hb*.h "$DIST/include/" 2>/dev/null || true
    cp -r src/harfbuzz "$DIST/include/" 2>/dev/null || true
    cat > "$DIST/lib/pkgconfig/harfbuzz.pc" << PCFILE
Name: harfbuzz
Description: HarfBuzz text shaping (manual install for the 212 chain)
Version: 8.5.0
Libs: -L$DIST_WIN/lib -lharfbuzz
Cflags: -I$DIST_WIN/include
PCFILE
    echo "== harfbuzz 装好 =="
else
    echo "== harfbuzz 已构建，跳过 =="
fi

# 4. libass 0.16.0：freetype/fribidi/harfbuzz 经 pkg-config 发现
#    （.pc 全是 Windows 路径，libass 的 configure 在 msys sh 里跑、
#    clang 收到 -I/c/... 时 msys 自动转换——两条路径形态都能吃）。
export PKG_CONFIG_PATH="$DIST/lib/pkgconfig"
export PKG_CONFIG_LIBDIR="$DIST/lib/pkgconfig"
# PKG_CONFIG 指向包装器：本机 pkgconf 的 env 处理不可靠（env 三种格式全败），
# --with-path 注入已实测可靠（freetype2/fribidi 都能解析）。
export PKG_CONFIG="/c/re-tools/host-cc/pkgconfw.exe"
# --disable-require-system-font-provider：Android 无 fontconfig/DirectWrite，
# 字体走 ass_add_font 内存注入（设计稿 D4）——configure 的报错信息自己给了这个 flag。
configure_pkg "libass-0.16.0.tar.xz" "libass-0.16.0" "libass.a" \
    --disable-require-system-font-provider

# 5. 产物自检：栅格化 / 双向 / shaping / libass 入口。
# 用 nm（Windows 目标是 COFF 归档，readelf 读不了）。
NMF="$TOOLCHAIN/bin/llvm-nm.exe"
for sym in FT_Init_FreeType fribidi_get_par_embedding_levels hb_buffer_create ass_library_init; do
    found=0
    for lib in "$DIST"/lib/*.a; do
        if "$NMF" "$lib" 2>/dev/null | grep -q "$sym"; then
            found=1
            echo "  $sym ← $(basename "$lib")"
            break
        fi
    done
    [ "$found" = 1 ] || { echo "自检失败：$sym 不在任何库"; exit 1; }
done
ls -la "$DIST/lib/"
echo "OK：libass-android dist 就绪（$DIST）"
