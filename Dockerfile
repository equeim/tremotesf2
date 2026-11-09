# SPDX-FileCopyrightText: 2026 ds
#
# SPDX-License-Identifier: CC0-1.0

# FULLY static build of Tremotesf - musl, zero shared libraries.
#
# Qt 6.12 (qtbase + qtsvg + qttools) and the KDE Frameworks we use (KWidgetsAddons
# + KWindowSystem from 6.19) are built from source: Wayland only, software
# rendering, no OpenGL/EGL - but WITH D-Bus: that is what lets the app pick up
# the current KDE/Plasma theme (palette, fonts and icon theme from kdeglobals,
# live updates over the KDE D-Bus interfaces).
#
# The difference to a normal static Qt build: EVERYTHING is linked statically -
# including libc and libdbus - so the resulting binary has no PT_INTERP and no
# NEEDED entries at all. It runs on any x86-64/aarch64 Linux (glibc or musl,
# any version) with a Wayland compositor, with no runtime library requirements
# whatsoever.
#
# That rules out dlopen() too: a static musl binary cannot load the host's
# glibc-built .so files. Hence OpenSSL is linked in (Qt's -openssl-linked mode,
# INPUT_openssl=linked below) instead of being dlopen'ed by the TLS backend at
# runtime, and the plugins are linked into the binary: Qt's CMake package does
# that automatically for the "default" plugins of every module that is linked
# (the Wayland platform plugin, the TLS backend, the image formats, the SVG
# icon engine). The one plugin that is not covered by that - KWindowSystem's
# Wayland backend - cannot be linked into a static binary at all, so the app
# runs without it; cmake/StaticBuild.cmake explains what that means in
# practice.
#
# Alpine is the base because musl is the only libc that is sane to link
# statically (glibc's static libc drags in NSS/dlopen stubs). Alpine's -dev
# packages ship no .a - the archives live in separate -static subpackages - so
# those subpackages are installed for everything that has one (freetype,
# fontconfig, harfbuzz, xkbcommon, expat, zlib, bzip2, libpng, zstd, brotli,
# wayland, openssl, libpsl, libidn2, libunistring). D-Bus and fmt have no
# -static subpackage and are built from source.
#
# In addition to the bare binary, an AppImage of it is produced for every
# architecture (the AppImage runtime is built from source here, so there is no
# x86_64/aarch64-only prebuilt binary to depend on) - see the AppImage sections
# at the end.
#
# Usage:  ./docker-build.sh [extra docker build args, e.g. --platform linux/arm64]

FROM alpine:3.23

# Versions to build. Qt/KF are new enough for Tremotesf (Qt >= 6.8, KF >= 6.11)
# and match what the KDE/Qt stack in this image expects.
ARG QT_VERSION=v6.12.0
ARG KF_VERSION=6.19.0
ARG FMT_VERSION=11.2.0
ARG DBUS_VERSION=1.16.2

# ---------------------------------------------------------------------------
# Build tools and libraries.
#
# The -static subpackages provide the .a archives; the -dev packages provide
# the headers and pkg-config files. libjpeg is NOT installed: Qt falls back to
# its bundled copy, one dependency less. Neither are pcre2, md4c,
# double-conversion or glib - Qt uses its bundled pcre2 (system_pcre2=OFF) and
# skips the glib event dispatcher integration. No mesa/libdrm anywhere: EGL
# and OpenGL are compiled out of Qt.
RUN apk add --no-cache \
        build-base \
        cmake \
        ninja \
        meson \
        git \
        curl \
        ca-certificates \
        gperf \
        perl \
        python3 \
        py3-packaging \
        pkgconf \
        file \
        util-linux \
        squashfs-tools \
        autoconf \
        automake \
        libtool \
        patch \
        gettext \
        # the AppImage runtime's Makefile hardcodes clang as CC and links
        # mimalloc as its allocator (see the type2-runtime section below)
        clang \
        mimalloc-dev \
        libffi-dev \
        freetype-dev freetype-static \
        fontconfig-dev fontconfig-static \
        harfbuzz-dev harfbuzz-static \
        graphite2-static \
        libxkbcommon-dev libxkbcommon-static \
        expat-dev expat-static \
        zlib-dev zlib-static \
        bzip2-dev bzip2-static \
        libpng-dev libpng-static \
        zstd-dev zstd-static \
        brotli-dev brotli-static \
        wayland-dev wayland-static \
        openssl-dev openssl-libs-static \
        libpsl-dev libpsl-static \
        libidn2-dev libidn2-static \
        libunistring-dev libunistring-static \
        eudev-dev \
        linux-headers \
        cxxopts-dev \
        extra-cmake-modules \
        wayland-protocols \
        plasma-wayland-protocols

# ---------------------------------------------------------------------------
# AppImage runtime, built from source (github.com/AppImage/type2-runtime).
#
# Upstream only ships prebuilt runtimes (and the appimagetool binary) for
# x86_64 and aarch64, so instead of downloading them and skipping other
# architectures, the runtime is built here - native, so riscv64 works under
# qemu too - and the AppImage is assembled by hand: the format is literally the
# runtime binary followed by a squashfs image (see the AppImage section at the
# end).
#
# The runtime is a single C file, but it statically links libfuse3 and
# squashfuse so the target system needs no libfuse (just the usual fusermount3
# helper and /dev/fuse; on a system without FUSE the user runs the AppImage
# with --appimage-extract). That is why libfuse and squashfuse are built from
# source here: the runtime needs their static archives, and libfuse needs the
# upstream mount.c patch (it makes libfuse honour $FUSERMOUNT_PROG, which the
# runtime sets after finding the helper on its own PATH). The libfuse/squashfuse
# versions match what type2-runtime's own build scripts pin; the runtime is
# pinned to a commit.
RUN curl -fsSLO --retry 5 --retry-all-errors --retry-delay 2 \
        https://github.com/libfuse/libfuse/releases/download/fuse-3.15.0/fuse-3.15.0.tar.xz \
 && tar xJ -C /tmp -f fuse-3.15.0.tar.xz \
 && cd /tmp/fuse-3.15.0 \
 && curl -fsSL --retry 5 --retry-all-errors --retry-delay 2 -o /tmp/mount.c.diff \
        https://raw.githubusercontent.com/AppImage/type2-runtime/main/patches/libfuse/mount.c.diff \
 && patch -p1 < /tmp/mount.c.diff \
 && meson setup --prefix=/usr --default-library=static build \
 && ninja -C build install

RUN curl -fsSL --retry 5 --retry-all-errors --retry-delay 2 \
        -o /tmp/squashfuse.tar.gz https://github.com/vasi/squashfuse/archive/0.5.2.tar.gz \
 && tar xz -C /tmp -f /tmp/squashfuse.tar.gz \
 && cd /tmp/squashfuse-0.5.2 \
 && ./autogen.sh \
 && ./configure LDFLAGS=-static \
 && make -j"$(nproc)" \
 && make install \
 && install -c -m 644 ./*.h /usr/local/include/squashfuse

RUN curl -fsSL --retry 5 --retry-all-errors --retry-delay 2 \
        -o /tmp/type2-runtime.tar.gz \
        https://github.com/AppImage/type2-runtime/archive/8f39b89e2ac31e1640b3d3f7e9a5108e6ce805fa.tar.gz \
 && tar xz -C /tmp -f /tmp/type2-runtime.tar.gz \
 && cd /tmp/type2-runtime-*/src/runtime \
 && make -j"$(nproc)" runtime \
 && strip runtime \
 && echo -ne 'AI\x02' | dd of=runtime bs=1 count=3 seek=8 conv=notrunc \
 && file runtime
# ---------------------------------------------------------------------------
# D-Bus ${DBUS_VERSION}, static, from source - there is no -static package
# (Alpine's dbus subpackages are dev/libs/doc/x11/openrc only).
#
# Why D-Bus at all: it is what gives the app the KDE platform theme. With the
# dbus feature on, qtbase compiles qkdetheme.cpp into QtGui (guarded by
# "UNIX AND settings AND dbus AND (xcb OR wayland)"): it reads
# ~/.config/kdeglobals for the palette, fonts and icon theme and listens to the
# KDE D-Bus interfaces for live theme changes. Tremotesf also uses Qt DBus
# itself for its single-instance IPC, for the freedesktop notifications and
# for the file manager D-Bus interface.
#
# --default-library=static: only libdbus-1.a is produced (plus the dbus tools,
# which nothing links), so Qt's pkg-config lookup of dbus-1 resolves to the
# archive and the final binary stays fully static. expat (already a static
# archive here) is dbus' only external dependency; systemd, X, selinux and
# apparmor support are off. D-Bus 1.16 is meson-only, there is no ./configure
# left in the tarball.
#
# The -pthread flag is dropped from the installed dbus-1.pc afterwards: meson
# puts it into both Cflags and Libs, and Qt ends up with it in the QtDBus
# target's compile *definitions*, which makes automoc call the compiler with a
# bogus -D-pthread. musl has threads in libc and Qt passes -pthread itself, so
# nothing is lost.
RUN curl -fsSL --retry 5 --retry-all-errors --retry-delay 2 \
        https://dbus.freedesktop.org/releases/dbus/dbus-${DBUS_VERSION}.tar.xz \
        | tar xJ -C /tmp \
 && meson setup /tmp/dbus-build /tmp/dbus-${DBUS_VERSION} \
        --prefix=/usr --libdir=lib \
        --sysconfdir=/etc --localstatedir=/var \
        --default-library=static \
        -Dxml_docs=disabled -Ddoxygen_docs=disabled \
        -Dselinux=disabled -Dapparmor=disabled -Dsystemd=disabled \
        -Dx11_autolaunch=disabled -Dmodular_tests=disabled \
        -Dinstalled_tests=false -Dasserts=false \
 && chrt --idle 0 meson compile -C /tmp/dbus-build \
 && meson install -C /tmp/dbus-build \
 && sed -i 's/ -pthread//g' /usr/lib/pkgconfig/dbus-1.pc \
 && pkg-config --cflags --libs dbus-1

# ---------------------------------------------------------------------------
# fmt ${FMT_VERSION}, static, from source - there is no -static package.
#
# Tremotesf requires fmt >= 10.1; the version matches the one in the image.
RUN curl -fsSL --retry 5 --retry-all-errors --retry-delay 2 \
        https://github.com/fmtlib/fmt/archive/refs/tags/${FMT_VERSION}.tar.gz \
        | tar xz -C /tmp \
 && cmake -B /tmp/fmt-build -S /tmp/fmt-${FMT_VERSION} -GNinja \
        -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr \
        -DBUILD_SHARED_LIBS=OFF -DFMT_TEST=OFF -DFMT_DOC=OFF \
 && cmake --build /tmp/fmt-build \
 && cmake --install /tmp/fmt-build

# Drop the .so linker symlinks of every library that must resolve to its
# archive: pkg-config hands Qt (and KF) plain -lfoo flags, and without the
# symlink the linker can only find libfoo.a. (The real .so.N runtime files
# stay but are never referenced.) Exception: harfbuzz - Qt resolves it through
# pkg-config to the *versioned* file (/usr/lib/libharfbuzz.so.0.61201.0) and
# records that absolute path in its CMake package files, so the versioned
# runtime files must go too; nothing in the image links harfbuzz dynamically
# (only cairo would, and nothing runs cairo).
# Also drop Alpine's harfbuzz CMake *config package*: it defines
# harfbuzz::harfbuzz with IMPORTED_LOCATION pointing at the versioned .so
# file, and Qt's FindWrapSystemHarfbuzz prefers config packages over
# pkg-config - it would record the (now missing) .so path. Without the config,
# the pkg-config/find_library fallback finds libharfbuzz.a.
RUN rm -f /usr/lib/libfreetype.so \
          /usr/lib/libfontconfig.so \
          /usr/lib/libharfbuzz.so* \
          /usr/lib/libgraphite2.so \
          /usr/lib/libxkbcommon.so \
          /usr/lib/libexpat.so \
          /usr/lib/libz.so \
          /usr/lib/libbz2.so \
          /usr/lib/libpng16.so \
          /usr/lib/libpng.so \
          /usr/lib/libzstd.so \
          /usr/lib/libbrotlidec.so \
          /usr/lib/libbrotlicommon.so \
          /usr/lib/libbrotlienc.so \
          /usr/lib/libwayland-client.so \
          /usr/lib/libwayland-cursor.so \
          /usr/lib/libwayland-egl.so \
          /usr/lib/libwayland-server.so \
          /usr/lib/libssl.so \
          /usr/lib/libcrypto.so \
          /usr/lib/libpsl.so \
          /usr/lib/libidn2.so \
          /usr/lib/libunistring.so \
          /usr/lib/libffi.so \
          /usr/lib/libudev.so \
          /usr/lib/libfmt.so \
 && rm -rf /usr/lib/cmake/harfbuzz

# ---------------------------------------------------------------------------
# Qt ${QT_VERSION} (qtbase + qtsvg + qttools), built static, Wayland only,
# software rendering only, with D-Bus on (see the D-Bus section above - it is
# what enables the KDE platform theme inside QtGui).
#
# The wayland client (the WaylandClient module and the QWaylandIntegrationPlugin
# platform plugin) is part of qtbase; the qtwayland repo only provides the
# compositor side, which nothing here uses, so it is not built. KWindowSystem's
# Wayland backend needs Qt6::WaylandClient, which the qtbase build below
# provides. QT_QPA_PLATFORMS=wayland marks the wayland plugin as the default
# platform plugin, which Qt's CMake package then links into any app that links
# Qt6::Gui.
#
#   INPUT_openssl=linked   the CMake spelling of -openssl-linked: the TLS
#                          backend links libssl/libcrypto directly instead of
#                          dlopen'ing them at runtime.
#
# CMAKE_FIND_LIBRARY_SUFFIXES=".a;.so" makes find_library() prefer the
# archives; the .so symlinks removed above make even the pkg-config driven
# -l flags resolve to archives.
RUN curl -fsSL --retry 5 --retry-all-errors --retry-delay 2 \
        https://github.com/qt/qtbase/archive/refs/tags/${QT_VERSION}.tar.gz \
        | tar xz -C /tmp \
 && cmake -B /tmp/qtbase-build -S /tmp/qtbase-${QT_VERSION#v} -GNinja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/opt/qt \
        -DCMAKE_CXX_STANDARD=17 \
        -DCMAKE_FIND_LIBRARY_SUFFIXES=".a;.so" \
        -DBUILD_SHARED_LIBS=OFF \
        -DQT_QPA_PLATFORMS="wayland" \
        -DQT_FEATURE_xcb=OFF \
        -DQT_FEATURE_eglfs=OFF \
        -DQT_FEATURE_sql=OFF \
        -DQT_FEATURE_dbus=ON \
        -DQT_FEATURE_printsupport=OFF \
        -DQT_FEATURE_testlib=OFF \
        -DINPUT_opengl=no \
        -DQT_FEATURE_egl=OFF \
        -DQT_FEATURE_glib=OFF \
        -DQT_FEATURE_system_pcre2=OFF \
        -DINPUT_freetype=system \
        -DINPUT_harfbuzz=system \
        -DINPUT_openssl=linked \
        -DQT_FEATURE_icu=OFF \
        -DQT_BUILD_EXAMPLES=OFF \
        -DQT_BUILD_TESTS=OFF \
 && chrt --idle 0 cmake --build /tmp/qtbase-build \
 && cmake --install /tmp/qtbase-build

# qtsvg: the SVG icon engine and image format plugins. Tremotesf does not use
# the Svg module itself on Linux, but the plugins are needed to show the icons
# of the desktop's icon theme; cmake/StaticBuild.cmake links the module in so
# that Qt's CMake package pulls its default plugins into the executable.
RUN curl -fsSL --retry 5 --retry-all-errors --retry-delay 2 \
        https://github.com/qt/qtsvg/archive/refs/tags/${QT_VERSION}.tar.gz \
        | tar xz -C /tmp \
 && cmake -B /tmp/qtsvg-build -S /tmp/qtsvg-${QT_VERSION#v} -GNinja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/opt/qt \
        -DCMAKE_PREFIX_PATH=/opt/qt \
        -DCMAKE_CXX_STANDARD=17 \
        -DCMAKE_FIND_LIBRARY_SUFFIXES=".a;.so" \
        -DBUILD_SHARED_LIBS=OFF \
        -DQT_BUILD_EXAMPLES=OFF \
        -DQT_BUILD_TESTS=OFF \
 && chrt --idle 0 cmake --build /tmp/qtsvg-build \
 && cmake --install /tmp/qtsvg-build

# qttools: only for LinguistTools (lrelease/lupdate), which Tremotesf's
# translations/CMakeLists.txt uses to compile its .ts files into the .qm files
# that are embedded into the binary. Everything else in qttools (assistant,
# designer, qdoc, ...) is switched off.
#
# Qt's own message catalogs are not built (they live in the separate
# qttranslations repo and are compiled by a module's own build, which would need
# lrelease during the qtbase build). Tremotesf's UI is fully translated - the
# .qm files are embedded into the binary - but Qt's built-in strings, e.g. the
# standard dialog buttons, stay in the language Qt was configured with.
RUN curl -fsSL --retry 5 --retry-all-errors --retry-delay 2 \
        https://github.com/qt/qttools/archive/refs/tags/${QT_VERSION}.tar.gz \
        | tar xz -C /tmp \
 && cmake -B /tmp/qttools-build -S /tmp/qttools-${QT_VERSION#v} -GNinja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/opt/qt \
        -DCMAKE_PREFIX_PATH=/opt/qt \
        -DCMAKE_CXX_STANDARD=17 \
        -DCMAKE_FIND_LIBRARY_SUFFIXES=".a;.so" \
        -DBUILD_SHARED_LIBS=OFF \
        -DQT_FEATURE_linguist=ON \
        -DQT_FEATURE_qdoc=OFF \
        -DQT_FEATURE_assistant=OFF \
        -DQT_FEATURE_designer=OFF \
        -DQT_FEATURE_pixeltool=OFF \
        -DQT_FEATURE_qdbus=OFF \
        -DQT_FEATURE_qtdiag=OFF \
        -DQT_FEATURE_qev=OFF \
        -DQT_FEATURE_qtplugininfo=OFF \
        -DQT_FEATURE_qtattributionsscanner=OFF \
        -DQT_FEATURE_distancefieldgenerator=OFF \
        -DQT_FEATURE_kmap2qmap=OFF \
        -DQT_FEATURE_fullqthelp=OFF \
        -DQT_BUILD_EXAMPLES=OFF \
        -DQT_BUILD_TESTS=OFF \
 && chrt --idle 0 cmake --build /tmp/qttools-build \
 && cmake --install /tmp/qttools-build

# ---------------------------------------------------------------------------
# KDE Frameworks ${KF_VERSION}: KWidgetsAddons (KMessageWidget, KPageWidget,
# KTitleWidget, ...) and KWindowSystem (Wayland/X11 platform detection and the
# XDG activation token helper). Both are tier-1 frameworks that only need Qt's
# Widgets/Gui on top of extra-cmake-modules (installed from Alpine).
#
# KWindowSystem is built without its X11 backend (no X11/xcb in this image, the
# app is Wayland only) and without QML bindings. Turning the X11 backend off
# also drops KStartupInfo, which is only built as part of it, so Tremotesf's X11
# startup notification code is compiled out there (TREMOTESF_KWINDOWSYSTEM_X11).
#
# Its Wayland backend stays a loadable module and is not shipped: a fully static
# binary cannot dlopen() plugins, and rebuilding it as a static plugin does not
# work either - the plugin generates its own copies of the Wayland protocol
# wrappers, which collide with Qt's copies in libQt6WaylandClient.a that the
# Wayland platform plugin needs. KWindowSystem falls back to its built-in
# implementation instead; cmake/StaticBuild.cmake explains what that means in
# practice. The plugin target is switched from a loadable module to an archive
# so that it is not linked as a shared object against the non-PIC static
# archives here (that fails with relocation errors); nothing uses it.
RUN curl -fsSL --retry 5 --retry-all-errors --retry-delay 2 \
        -o /tmp/kf.tar.xz \
        https://download.kde.org/stable/frameworks/${KF_VERSION%.*}/kwidgetsaddons-${KF_VERSION}.tar.xz \
 && tar xJ -C /tmp -f /tmp/kf.tar.xz \
 && cmake -B /tmp/kwa-build -S /tmp/kwidgetsaddons-${KF_VERSION} -GNinja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/opt/kf6 \
        -DKDE_INSTALL_LIBDIR=lib \
        -DCMAKE_PREFIX_PATH="/opt/qt;/usr" \
        -DCMAKE_FIND_LIBRARY_SUFFIXES=".a;.so" \
        -DBUILD_SHARED_LIBS=OFF \
        -DBUILD_TESTING=OFF \
        -DBUILD_QCH=OFF \
        -DBUILD_DESIGNERPLUGIN=OFF \
        -DBUILD_PYTHON_BINDINGS=OFF \
 && chrt --idle 0 cmake --build /tmp/kwa-build \
 && cmake --install /tmp/kwa-build

RUN curl -fsSL --retry 5 --retry-all-errors --retry-delay 2 \
        -o /tmp/kws.tar.xz \
        https://download.kde.org/stable/frameworks/${KF_VERSION%.*}/kwindowsystem-${KF_VERSION}.tar.xz \
 && tar xJ -C /tmp -f /tmp/kws.tar.xz \
 && sed -i 's/^add_library(KF6WindowSystemKWaylandPlugin MODULE)$/add_library(KF6WindowSystemKWaylandPlugin STATIC)/' \
        /tmp/kwindowsystem-${KF_VERSION}/src/platforms/wayland/CMakeLists.txt \
 && cmake -B /tmp/kws-build -S /tmp/kwindowsystem-${KF_VERSION} -GNinja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/opt/kf6 \
        -DKDE_INSTALL_LIBDIR=lib \
        -DCMAKE_PREFIX_PATH="/opt/qt;/usr" \
        -DCMAKE_FIND_LIBRARY_SUFFIXES=".a;.so" \
        -DBUILD_SHARED_LIBS=OFF \
        -DBUILD_TESTING=OFF \
        -DBUILD_QCH=OFF \
        -DKWINDOWSYSTEM_X11=OFF \
        -DKWINDOWSYSTEM_WAYLAND=ON \
        -DKWINDOWSYSTEM_QML=OFF \
 && chrt --idle 0 cmake --build /tmp/kws-build \
 && cmake --install /tmp/kws-build

# ---------------------------------------------------------------------------
# Tremotesf itself. TREMOTESF_STATIC_BUILD links the executable with -static
# (see cmake/StaticBuild.cmake): no interpreter, no dynamic section at all.
#
# The ldd check asserts full staticness: a binary with no dynamic section and
# no PT_INTERP is not a dynamic program at all, so ldd must fail
# ("Not a valid dynamic program").
#
# The last check asserts the KDE theme support really got linked in: QKdeTheme's
# kdeglobals path and the org.kde.* D-Bus service names must be in the binary.
WORKDIR /src
COPY . ./
RUN cmake -B build -S . -GNinja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_PREFIX_PATH="/opt/qt;/opt/kf6" \
        -DTREMOTESF_STATIC_BUILD=ON \
        -DBUILD_TESTING=OFF \
        -DTREMOTESF_WITH_HTTPLIB=none \
 && chrt --idle 0 cmake --build build \
 && strip build/src/tremotesf \
 && file build/src/tremotesf \
 && ! ldd build/src/tremotesf \
 && (strings build/src/tremotesf | grep -E "libEGL|libGLESv2|libgbm|libdrm\.|libwayland-egl|libxkbcommon\.so|libssl\.so|libcrypto\.so|libfontconfig\.so|libdbus-1\.so|libQt6|libKF6" || echo "no dynamic-library strings") \
 && (strings build/src/tremotesf | grep -oE "org\.kde\.[A-Za-z0-9]+" | sort -u || true) \
 && (strings build/src/tremotesf | grep -q kdeglobals && echo "KDE theme support linked in")

# ---------------------------------------------------------------------------
# AppImage of the fully static binary.
#
# The binary needs nothing at runtime, so the AppDir is trivial: the executable,
# the desktop file, the icon and an AppRun symlink to the executable - there are
# no shared libraries to bundle (that is what linuxdeploy in the usual AppImage
# recipes is for; nothing to bundle here). The desktop file, the icon and the
# appdata file are the ones the build installs, so `cmake --install` into
# AppDir/usr is all that is needed to fill the AppDir. The AppImage is then just
# the runtime built above followed by the squashfs of the AppDir - what
# appimagetool does, minus the tool download and its x86_64/aarch64-only
# binaries. The squashfs flags are appimagetool's (zstd, 128 KiB blocks,
# all-root, and -noappend so the superblock starts exactly where the runtime
# expects it).
#
# The desktop file and the icon must also sit at the AppDir root (the spec's
# lookup order, the icon name comes from the Icon= key); the .DirIcon symlink is
# for desktop-integration tools on the user's side. AppRun is just a symlink to
# the binary: the runtime execs AppRun with the AppDir mounted, the binary is
# fully self-contained, so a wrapper script would add nothing (a script would
# only be needed to set up the environment for bundled libraries - there are
# none).
#
# The output is a fixed name, build/tremotesf.appimage (no arch/version in it):
# docker-build.sh copies it out as-is and CI renames it per platform.
RUN cmake --install build --prefix /src/AppDir/usr \
 && cp AppDir/usr/share/applications/org.equeim.Tremotesf.desktop AppDir/org.equeim.Tremotesf.desktop \
 && cp AppDir/usr/share/icons/hicolor/scalable/apps/org.equeim.Tremotesf.svg AppDir/org.equeim.Tremotesf.svg \
 && ln -s usr/bin/tremotesf AppDir/AppRun \
 && ln -s org.equeim.Tremotesf.svg AppDir/.DirIcon \
 && mksquashfs AppDir /tmp/squashfs.img -noappend -all-root -comp zstd -b 131072 \
 && cat /tmp/type2-runtime-*/src/runtime/runtime /tmp/squashfs.img > build/tremotesf.appimage \
 && chmod +x build/tremotesf.appimage \
 && file build/tremotesf.appimage
