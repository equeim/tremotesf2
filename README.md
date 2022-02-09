# Tremotesf
Remote GUI for transmission-daemon. Supports desktop OSes (GNU/Linux and Windows) and Sailfish OS.

Table of Contents
=================

   * [Tremotesf](#tremotesf)
   * [Table of Contents](#table-of-contents)
      * [Installation](#installation)
         * [Desktop](#desktop)
            * [Dependencies](#dependencies)
            * [Building](#building)
            * [GNU/Linux](#gnulinux)
            * [Windows](#windows)
         * [Sailfish OS](#sailfish-os)
            * [Dependencies](#dependencies-1)
            * [Building](#building-1)
      * [Translations](#translations)
      * [Donate](#donate)
      * [Screenshots](#screenshots)
         * [Desktop](#desktop-1)
         * [Sailfish OS](#sailfish-os-1)


## Installation
### Desktop
#### Dependencies
- C++17 compiler
- CMake 3.12 or newer (3.21 on Windows)
- Qt 5.6 or newer (Core, Network, Concurrent, Gui, Widgets)
- KWidgetsAddons

On GNU/Linux and BSD:
- Gettext 0.19.7 or newer
- Qt D-Bus, Qt X11 Extras
- KWindowSystem

#### Building
```sh
cmake -S /path/to/sources -B /path/to/build/directory --preset default-debug
cmake --build /path/to/build/directory
cmake --install /path/to/build/directory --prefix /path/to/install/directory
```

#### GNU/Linux
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

- openSUSE - [OBS](https://build.opensuse.org/project/show/home:equeim:tremotesf)
```sh
#sudo zypper ar https://download.opensuse.org/repositories/home:/equeim:/tremotesf/openSUSE_Leap_15.3/home:equeim:tremotesf.repo
sudo zypper ar https://download.opensuse.org/repositories/home:/equeim:/tremotesf/openSUSE_Tumbleweed/home:equeim:tremotesf.repo
sudo zypper in tremotesf
```

- Ubuntu - [OBS](https://build.opensuse.org/project/show/home:equeim:tremotesf)

```sh
wget https://download.opensuse.org/repositories/home:/equeim:/tremotesf/xUbuntu_20.04/Release.key -O - | sudo apt-key add -
sudo add-apt-repository "deb http://download.opensuse.org/repositories/home:/equeim:/tremotesf/xUbuntu_20.04/ /"
sudo apt update
sudo apt install tremotesf
```

#### Windows
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

### Sailfish OS
Tremotesf is available in Jolla Store and [OpenRepos.net](https://openrepos.net/content/equeim/tremotesf).
#### Dependencies
Sailfish OS 3.3.0 or newer
#### Building
SSH/chroot into SDK, then:
```sh
cd /path/to/sources
sfdk -c no-fix-version -c target=<target name, e.g. SailfishOS-4.3.0.12-armv7hl> build -p -d -j<number of jobs>
```

## Translations
Translations are managed on [Transifex](https://www.transifex.com/equeim/tremotesf).

## Donate
I you like this app, you can support its development via [PayPal](https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=DDQTRHTY5YV2G&item_name=Support%20Tremotesf%20development&no_note=1&item_number=3&no_shipping=1&currency_code=EUR) or [Yandex.Money](https://yasobe.ru/na/equeim_tremotesf).

## Screenshots
### Desktop
![](https://github.com/equeim/tremotesf-screenshots/raw/master/desktop-1.png)
![](https://github.com/equeim/tremotesf-screenshots/raw/master/desktop-2.png)
![](https://github.com/equeim/tremotesf-screenshots/raw/master/desktop-3.png)
![](https://github.com/equeim/tremotesf-screenshots/raw/master/desktop-4.png)
### Sailfish OS
![](http://i.imgur.com/pNVIpCm.png)
![](http://i.imgur.com/RCqDejT.png)
![](http://i.imgur.com/K3vs1sq.png)
