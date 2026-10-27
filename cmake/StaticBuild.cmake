# SPDX-FileCopyrightText: 2026 ds
#
# SPDX-License-Identifier: CC0-1.0

# Support for fully static builds: everything, including libc, is linked
# statically, so the executable has no shared library dependencies at all
# (no PT_INTERP, no dynamic section) and runs on any Linux with a Wayland
# compositor.
#
# This is what the Dockerfile in the repository root does: it builds Qt and
# the KDE Frameworks we use statically in an Alpine (musl) image, then links
# Tremotesf with -static and wraps the result into an AppImage. musl is used
# because glibc's static libc drags in NSS/dlopen stubs.
#
# A fully static binary cannot dlopen() anything, so every Qt plugin that is
# needed at runtime has to be linked into the executable. Qt's CMake package
# does that by itself for the "default" plugins of the modules that are linked
# (the Wayland platform plugin selected by QT_QPA_PLATFORMS when Qt was
# configured, the OpenSSL TLS backend, the image formats). The one plugin that
# is not covered by that - KWindowSystem's Wayland backend - cannot be linked
# in at all; see below for what that means in practice.

option(TREMOTESF_STATIC_BUILD
    "Build a fully static executable: no dependency on any shared library, including libc. Requires static builds of Qt, KDE Frameworks and all the other dependencies."
    OFF)

if (NOT TREMOTESF_STATIC_BUILD)
    return()
endif()

# find_library() and the pkg-config driven -l flags must resolve to the
# archives. The Dockerfile removes the .so symlinks of the libraries used
# here, so even a plain -lfoo can only find libfoo.a.
set(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})

# KWindowSystem builds its X11 backend, and with it KStartupInfo, only when it is
# built with X11 support, which needs Qt's xcb support and static X11 libraries.
# Neither is part of this build, so the app's X11 startup notification code is
# compiled out (see TREMOTESF_KWINDOWSYSTEM_X11 in src/CMakeLists.txt).
set(TREMOTESF_KWINDOWSYSTEM_X11 OFF)

function(tremotesf_setup_static_build target)
    # Link everything statically, including libc and libstdc++.
    #
    # -no-pie because Alpine's toolchain turns -static into -static-pie, and a
    # static-pie executable still carries a PT_INTERP entry for the libc loader
    # (so it still looks like a dynamic program to ldd(1)). Without PIE the
    # executable has no interpreter and no dynamic section at all.
    target_link_options(${target} PRIVATE -static -no-pie)

    # The Svg module is not used by the code on Linux, but its plugins (the
    # SVG icon engine and the SVG image format) are needed at runtime to show
    # the icons of the desktop's icon theme. Linking the module makes Qt's
    # CMake package link its default plugins into the executable.
    find_package(Qt6 ${TREMOTESF_MINIMUM_QT_VERSION} REQUIRED COMPONENTS Svg)
    target_link_libraries(${target} PRIVATE Qt6::Svg)

    # KWindowSystem's Wayland backend is a Qt plugin, and a static binary cannot
    # load plugins at runtime, so it is deliberately not linked in and
    # KWindowSystem falls back to its built-in implementation. That is safe:
    # without the plugin, KWaylandExtras::requestXdgActivationToken() finds no
    # plugin interface and answers asynchronously with an empty token, so
    # "open in file manager" still works - only without the XDG activation token
    # that would keep the file manager from stealing focus. KWindowSystem's
    # platform detection (KWindowSystem::platform(), used for the activation
    # token path above) is part of the library itself and keeps working.
    #
    # Linking the plugin in is not an option either: upstream builds it as a
    # loadable module that generates its own copies of the Wayland protocol
    # wrappers (QtWayland::wl_fixes and friends), and those collide with the
    # copies in Qt's own WaylandClient library, which the Wayland platform
    # plugin needs - the link fails with a wall of multiple definition errors.

    # Private dependencies of the static Qt and KF archives. Their CMake
    # packages record the libraries they link, but the pkg-config lookups Qt
    # itself runs are done without --static, so Requires.private entries (the
    # leaves of the dependency chains) never reach the link line. They are
    # appended here in topological order (consumers before providers), because
    # the linker scans archives in a single left-to-right pass. Archives whose
    # members were already pulled in earlier contribute nothing, so the
    # duplicates are harmless.
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(TREMOTESF_STATIC_DEPS REQUIRED IMPORTED_TARGET
        fontconfig harfbuzz graphite2 freetype2 libbrotlidec libbrotlicommon
        libpng bzip2 zlib libzstd expat libffi wayland-client wayland-cursor
        libidn2)
    target_link_libraries(${target} PRIVATE PkgConfig::TREMOTESF_STATIC_DEPS)

    # libunistring (a dependency of libidn2, which libpsl pulls in) ships no
    # pkg-config file on Alpine, so it is looked up by name.
    find_library(TREMOTESF_UNISTRING_LIBRARY NAMES unistring)
    if (TREMOTESF_UNISTRING_LIBRARY)
        target_link_libraries(${target} PRIVATE "${TREMOTESF_UNISTRING_LIBRARY}")
    endif ()
endfunction()
