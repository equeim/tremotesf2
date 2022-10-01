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
         * [Windows](#windows)
      * [Translations](#translations)
      * [Screenshots](#screenshots)

## Installation
### Dependencies
- C++17 compiler
- CMake 3.16 or newer (3.21 on Windows)
- Qt 5.15 (Core, Network, Concurrent, Gui, Widgets)
- fmt 7.0 or newer
- KWidgetsAddons

On GNU/Linux and BSD, also:
- Gettext 0.19.7 or newer
- Qt D-Bus
- KWindowSystem

On Windows:
- Windows 11 SDK is needed to build
- Minimum supported OS version is Windows 8.1

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
wget https://download.opensuse.org/repositories/home:/equeim:/tremotesf/Debian_11/Release.key -O - | sudo apt-key add -
sudo add-apt-repository "deb http://download.opensuse.org/repositories/home:/equeim:/tremotesf/Debian_11/ /"
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
wget https://download.opensuse.org/repositories/home:/equeim:/tremotesf/xUbuntu_21.10/Release.key -O - | sudo apt-key add -
sudo add-apt-repository "deb http://download.opensuse.org/repositories/home:/equeim:/tremotesf/xUbuntu_21.10/ /"
sudo apt update
sudo apt install tremotesf
```

### Windows
Windows builds are available at [releases](https://github.com/equeim/tremotesf2/releases) page.

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
