# Tremotesf
Remote GUI for transmission-daemon. Supports GNU/Linux and Windows.

Table of Contents
=================

   * [Tremotesf](#tremotesf)
   * [Table of Contents](#table-of-contents)
      * [Installation](#installation)
         * [Dependencies](#dependencies)
         * [Building](#building)
         * [GNU/Linux](#gnulinux)
         * [FreeBSD](#freebsd)
         * [Windows](#windows)
      * [Translations](#translations)
      * [Screenshots](#screenshots)

## Installation
### Dependencies
- C++ compiler with partial C++20 support. Minimum tested versions of GCC and Clang toolchains:
  1. GCC 10
  2. Clang 11 with libstdc++ 10
  3. Clang 13 with libc++ 13
- CMake 3.16 or newer (3.21 on Windows)
- Qt 5.15 (Core, Network, Concurrent, Gui, Widgets modules)
- fmt 7.0 or newer
- KWidgetsAddons
- libpsl
- cxxopts
- Qt Test module (for tests only)
- OpenSSL 1.1.1 or newer (for tests only)
- cpp-httplib 0.11.0 or newer (for tests only, optional)

On GNU/Linux and BSD, also:
- Gettext 0.19.7 or newer
- Qt D-Bus module
- KWindowSystem

On Windows, also:
- Windows 11 SDK is needed to build
- Minimum supported OS version to run Tremotesf is Windows 10 1809 (October 2018 Update)

### Building
```sh
cmake -S /path/to/sources -B /path/to/build/directory --preset base-multi
cmake --build /path/to/build/directory --config Debug
cmake --install /path/to/build/directory --config Debug --prefix /path/to/install/directory
```
This example uses base-multi preset in CMakePresets.json and Ninja Multi-Config generator.
You can invoke CMake in a different way if you want.

### GNU/Linux
- Flatpak - [Flathub](https://flathub.org/apps/details/org.equeim.Tremotesf)

- Arch Linux - [AUR](https://aur.archlinux.org/packages/tremotesf)

- Debian - [OBS](https://build.opensuse.org/project/show/home:equeim:tremotesf)

```sh
wget -qO - https://download.opensuse.org/repositories/home:/equeim:/tremotesf/Debian_12/Release.key | sudo tee /etc/apt/trusted.gpg.d/tremotesf.asc
sudo add-apt-repository "deb http://download.opensuse.org/repositories/home:/equeim:/tremotesf/Debian_12/ /"
sudo apt update
sudo apt install tremotesf
```

- Fedora - [Copr](https://copr.fedorainfracloud.org/coprs/equeim/tremotesf)
```sh
sudo dnf copr enable equeim/tremotesf
sudo dnf install tremotesf
```

- Gentoo - [equeim-overlay](https://github.com/equeim/equeim-overlay)

- Mageia - [Copr](https://copr.fedorainfracloud.org/coprs/equeim/tremotesf)
```sh
sudo dnf copr enable equeim/tremotesf
sudo dnf install tremotesf
```

- openSUSE Tumbleweed - [OBS](https://build.opensuse.org/project/show/home:equeim:tremotesf)
```sh
sudo zypper ar https://download.opensuse.org/repositories/home:/equeim:/tremotesf/openSUSE_Tumbleweed/home:equeim:tremotesf.repo
sudo zypper in tremotesf
```

- Ubuntu - [OBS](https://build.opensuse.org/project/show/home:equeim:tremotesf)

```sh
wget -qO - https://download.opensuse.org/repositories/home:/equeim:/tremotesf/xUbuntu_23.10/Release.key | sudo tee /etc/apt/trusted.gpg.d/tremotesf.asc
sudo add-apt-repository "deb http://download.opensuse.org/repositories/home:/equeim:/tremotesf/xUbuntu_23.10/ /"
sudo apt update
sudo apt install tremotesf
```

### FreeBSD
Tremotesf is [available in FreeBSD ports](https://www.freshports.org/net-p2p/tremotesf/).

### Windows
Windows builds are available at [releases](https://github.com/equeim/tremotesf2/releases) page.
Minimum supported OS version to run Tremotesf is Windows 10 1809 (October 2018 Update).

Build instructions for MSVC toolchain with vcpkg:
1. Install Visual Studio with 'Desktop development with C++' workload
2. Install latest version of CMake (from cmake.org or Visual Studio installer)
3. Install and setup [vcpkg](https://github.com/microsoft/vcpkg#quick-start-windows), and make sure that you have 15 GB of free space on disk where vcpkg is located
4. Set VCPKG_ROOT environment variable to the location of vcpkg installation

When building from Visual Studio GUI, make sure to select 'Windows Debug' or 'Windows Release' configure preset.
Otherwise:
Launch x64 Command Prompt for Visual Studio, execute:
```pwsh
cmake -S path\to\sources -B path\to\build\directory --preset <windows-debug or windows-release>
# Initial compilation of dependencies will take a while
cmake --build path\to\build\directory
cmake --install path\to\build\directory --prefix path\to\install\directory
```

## Translations
Translations are managed on [Transifex](https://www.transifex.com/equeim/tremotesf).

## Screenshots
![](https://github.com/equeim/tremotesf-screenshots/raw/master/desktop-1.png)
![](https://github.com/equeim/tremotesf-screenshots/raw/master/desktop-2.png)
![](https://github.com/equeim/tremotesf-screenshots/raw/master/desktop-3.png)
![](https://github.com/equeim/tremotesf-screenshots/raw/master/desktop-4.png)
