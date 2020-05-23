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
- C++14 compiler
- CMake 3.10 or newer
- Gettext 0.19.7 or newer
- Qt 5.6 or newer (core, network, concurrent, gui, widgets and dbus for GNU/Linux)
- KWidgetsAddons from KDE Frameworks 5

#### Building
```sh
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/your/install/prefix
make
make install
```

#### GNU/Linux
- Flatpak - [Flathub](https://flathub.org/apps/details/org.equeim.Tremotesf)

- Arch Linux - [AUR](https://aur.archlinux.org/packages/tremotesf)

- Debian - [OBS](https://build.opensuse.org/project/show/home:equeim:tremotesf)

```sh
wget https://download.opensuse.org/repositories/home:/equeim:/tremotesf/Debian_10/Release.key -O - | sudo apt-key add -
sudo add-apt-repository "deb http://download.opensuse.org/repositories/home:/equeim:/tremotesf/Debian_10/ /"
sudo apt update
sudo apt install tremotesf
```

- Fedora - [Copr](https://copr.fedorainfracloud.org/coprs/equeim/tremotesf)
```sh
dnf copr enable equeim/tremotesf
dnf install tremotesf
```

- Gentoo - [equeim-overlay](https://github.com/equeim/equeim-overlay)

- Mageia - [Copr](https://copr.fedorainfracloud.org/coprs/equeim/tremotesf)
```sh
dnf copr enable equeim/tremotesf
dnf install tremotesf
```

- openSUSE - [OBS](https://build.opensuse.org/project/show/home:equeim:tremotesf)
```sh
#zypper ar https://download.opensuse.org/repositories/home:/equeim:/tremotesf/openSUSE_Leap_15.1/home:equeim:tremotesf.repo
zypper ar https://download.opensuse.org/repositories/home:/equeim:/tremotesf/openSUSE_Tumbleweed/home:equeim:tremotesf.repo
zypper in tremotesf
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

### Sailfish OS
Tremotesf is available in Jolla Store and [OpenRepos.net](https://openrepos.net/content/equeim/tremotesf).
#### Dependencies
Sailfish OS 3.0.3 or newer
#### Building
SSH/chroot into SDK, then:
```sh
cd /path/to/sources
mb2 -X -t <target name, e.g. SailfishOS-latest-armv7hl> build -d
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
