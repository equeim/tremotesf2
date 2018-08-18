# Tremotesf
Remote GUI for transmission-daemon. Supports desktop OSes (GNU/Linux and Windows) and Sailfish OS.

## Installation
### Desktop
#### Dependencies
- C++11 compiler (GCC, MinGW and MSVC are supported)
- CMake
- Qt 5.6 or newer (core, network, concurrent, gui, widgets and dbus for GNU/Linux)
- KWidgetsAddons from KDE Frameworks 5

#### Building
```
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/your/install/prefix
make
make install
```

#### GNU/Linux distributions
##### Arch Linux
[AUR](https://aur.archlinux.org/packages/tremotesf)

##### Fedora
[Copr repo](https://copr.fedorainfracloud.org/coprs/equeim/tremotesf)

##### Gentoo
[equeim-overlay](https://github.com/equeim/equeim-overlay)

##### Mageia
[Copr repo](https://copr.fedorainfracloud.org/coprs/equeim/tremotesf)

##### openSUSE
[OBS repo](https://build.opensuse.org/project/show/home:equeim:tremotesf)

#### Windows
Windows builds are available at [releases](https://github.com/equeim/tremotesf2/releases) page.

### Sailfish OS
#### Dependencies
Sailfish OS 1.1.9 or newer
#### Building
SSH/chroot into SDK, then:
```
cd /path/to/sources
mb2 -X -t <target name, e.g. SailfishOS-latest-armv7hl> build -d
```

## Translations
Translations are managed on [Transifex](https://www.transifex.com/equeim/tremotesf).

## Donate
I you like this app, you can support its development via [PayPal](https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=DDQTRHTY5YV2G&item_name=Support%20Tremotesf%20development&no_note=1&item_number=3&no_shipping=1&currency_code=EUR) or [Yandex.Money](https://yasobe.ru/na/tremotesf).

## Screenshots
### Desktop
![](http://i.imgur.com/b1qqVZ1.png)
![](http://i.imgur.com/dhopqqf.png)
![](http://i.imgur.com/FGEjDvC.png)
### Sailfish OS
![](http://i.imgur.com/pNVIpCm.png)
![](http://i.imgur.com/RCqDejT.png)
![](http://i.imgur.com/K3vs1sq.png)
