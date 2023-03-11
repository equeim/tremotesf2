# Changelog

## [2.1.0] - 2023-03-12
### Added
- Trackers list shows number of seeders and leechers for trackers, not just total number peers
- New dependencies:
  1. libpsl
  2. cxxopts
  3. cpp-httplib 0.11 or newer (for tests only, optional)

### Changed
- "Seeders" and "leechers" now refer to total number of seeders and leechers reported by trackers,
  while number of peers that we are currently downloading from / uploading to is displayed separately
- Tracker's error is displayed in separate column
- Torrents that have an error but are still being downloaded/uploaded are displayed under both "Status" filters simultaneously
- TREMOTESF_BUILD_TESTS CMake option is replaced by standard BUILD_TESTING option

### Fixed
- Fixed issues with lists or torrents/trackers not being updated sometimes
- Main window placeholder when there is no torrents in list is now displayed correctly

## [2.0.0] - 2022-11-05
### Added
- Dark theme support on Windows (dark window title bar on Windows 10
is supported only on 1809 or newer and it can be buggy because Windows 10 doesn't support this officially)
- System accent color on Windows is used in app UI (can be disabled in settings)
- Windows builds now write logs to `C:\Users\User\AppData\Local\tremotesf\tremotesf` directory
- Opening of torrents (local files or HTTP(S) links) and magnet links is now supported via drag & drop or Ctrl+V on main window
- Option to automatically fill link from clipboard when adding torrent link (disabled by default)
- Last download directory is now remembered when adding torrents (can be disabled in settings) (thanks Alex Bell)
- Directory of last opened torrent is now remembered (can be disabled in settings)
- When adding torrents via opening file/link, drag & drop or clipboard while disconnected from server,
Tremotesf will now show message explaining that they will be added after connection to server
(instead of doing this sliently, confusing users)
- Status message is shown over empty torrent list when not connected to server
- Command line option to enable debug logs

### Changed
- Raised minimum version of Qt to 5.15
- Raised minimum version of CMake to 3.16 (3.21 on Windows)
- Windows build now uses Fusion style since default style imitating Win32 controls is non-themeable
- Windows installer now associates Tremotesf with torrent files and magnet links

### Removed
- Sailfish OS support
- org.equeim.Tremotesf D-Bus interface is removed
- Removed special handling for various file managers when using "Show in file manager" action
Now Tremotesf uncoditionally tries to use org.freedesktop.FileManager1 D-Bus interface,
and if it fails then QDesktopServices::openUrl() is used

### Fixed
- IPv6 address can be now used to connect to server
- Windows paths are always displayed with native directory separators

## [1.11.3] - 2022-03-18
### Fixed
- RPM spec file

## [1.11.2] - 2022-03-16
### Fixed
- Display of torrents list updates

## [1.11.1] - 2022-02-28
### Added
- MSI installer for Windows

### Fixed
- Renaming files when adding torrent
- Display of torrents count in status filters
- Sorting of torrents by ETA

### Removed
- Donation links

## [1.11.0] - 2022-02-13
### Added
- Automatic reconnection to server (thanks to LuK1337)
- PEM certificate can be loaded from file
- Links in torrents' comments are now clickable
- Torrent list filters (except search) are now saved when app is restarted
- Torrent properties screen now shows list of web seeders
- Most menu items that didn't have icons now have them (thank to Buck Melanoma)
- Hovering cursor over status bar when connection to server failed show more detailed error description
- Vcpkg integration when building for Windows

### Changed
- Further reduced memory usage when opening torrent files
- All app resources are now bundled inside executable
- Windows builds use MSVC toolchain by default

### Fixed
- Sailfish OS support (this will be the last release that supports Saifish OS)
- Console window output encoding on Windows (it now also uses UTF-8 on Windows 10)

## [1.10.0] - 2021-09-27
### Added
- Ability to shut down remote Transmission instance
- Renaming of torrent via its context menu

### Changed
- Reduced memory usage when opening torrent files

## [1.9.1] - 2021-05-10
### Changed
- Disabled MIME type checking for torrent files (it doesn't work for some files)
- Tremotesf now won't open torrent files that are bigger than 50 MiB

### Fixed
- Segfault or error when adding torrent files

## [1.9.0] - 2021-05-04
### Added
- It is now possible to specify whole certificate chain for self-signed certificate

### Changed
- Ctrl+F now focuses search field in main window
- C++17 compiler is now required

### Fixed
- Fixed torrent list artifacts when using GTK2 style plugin for Qt

## [1.8.0] - 2020-09-06
### Added
- Tremotesf now implements org.freedesktop.Application D-Bus interface on relevant platforms
- Desktop: added support of startup notifications on X11
- Desktop: added dependencies on Qt X11 Extras and KWindowSystem on Unix-like platforms
- Desktop: added menu item to copy torrent's magnet link

### Changed
- org.equeim.Tremotesf D-Bus interface is deprecated
- Desktop: notifications on Unix-like platforms are now clickable
- Desktop: when application window is hidden to tray icon, open dialogs are now hidden too
- Desktop: minor UI improvements
- Updated translations

### Fixed
- Fixed support of mounted remote directories

## [1.7.1] - 2020-06-27
### Changed
- Updated translations
- Enabled LTO for release build on Windows

### Fixed
- Fixed adding torrents with Qt 5.15

## [1.7.0] - 2020-06-05
### Added
- Added support of configuring per-server HTTP/SOCKS5 proxies
- Added support of renaming torrent's files when adding it
- Added ability to add multiple trackers at a time

### Changed
- Minimum CMake version raised to 3.10
- C++ standard version raised to C++14 (Sailfish OS GCC 4.9 toolchain is still supported)

### Fixed
- Fixed passing command line arguments and opening files with commas
- Sailfish OS: fixed opening app from notifications
- Sailfish OS: fixed reconnecting to server when its connection settings are changed

## [1.6.4] - 2020-01-11
### Fixed
- Fixed compilation for Windows
- Fix RPM validation errors/warnings for Sailfish OS

## [1.6.3] - 2020-01-05
### Fixed
- Fix installing of new translations

## [1.6.2] - 2020-01-05
### Changed
- Improved UNIX/Windows signals handling
- Command line options that don't start GUI (such as '-v' or '-h' or
passing files to already running Tremotesf instance) now don't require X/Wayland session
- Tremotesf now shows localized numbers instead of Latin-1 ones (in most places)
- Updated translations

### Fixed
- Fixed compilation with Qt 5.14

## [1.6.1] - 2019-07-16
### Changed
- Minor performance improvements

### Fixed
- Fixed notifications not being configurable in KDE Plasma and GNOME

## [1.6.0] - 2019-01-26
### Added
- Desktop: restoring of torrent properties dialog's geometry
- Desktop: Tremotesf now remebmers used download directories and shows them in combo box when adding torrent / changing location

### Changed
- Desktop: toolBarVisible and toolBarArea configuration keys are now deprecated, mainWindowState is used instead

### Fixed
- Desktop: fixed restoring state of detached toolbar
- Sailfish OS: fixed peers' upload speed label

## [1.5.6] - 2018-12-08
### Changed
- Desktop: Improved HiDPI support

### Fixed
- Desktop: fixed notification icon

## [1.5.5] - 2018-09-26
### Fixed
- Yandex.Money donate link

## [1.5.4] - 2018-09-10
### Changed
- Tremotesf binary now doesn't link to QtConcurrent library (but still requires its headers at build time)
- Desktop: improved AppStream metadata

## [1.5.3] - 2018-09-03
### Added
- Multiseat support (it is now possible to run multiple Tremotesf instances on different login sessions)
- Added .appdata.xml file

### Changed
- D-Bus is now used for IPC on Unix
- Minimum CMake version lowered to 3.0 (note that Qt >= 5.11 requires CMake 3.1)
- Desktop: .desktop file and icons are renamed according to Desktop Entry Specification

### Fixed
- Fixed crashes when disconnecting from server
- Added window title to Server Stats dialog
- "Open" and "Show In File Manager" menu items are disabled when files don't exist on filesystem
- Desktop: handle cases when xdg-mime executable doesn't exist
- Desktop: fallback to org.freedesktop.FileManager1 when showing files in file manager

## [1.5.2] - 2018-08-18
### Fixed
- Fixed crash when disconnecting from server

## [1.5.1] - 2018-08-14
### Added
- Universal RPM spec file for SailfishOS/Fedora/Mageia/openSUSE

### Changed
- Updated Spanish translation

### Fixed
- Fixed openSUSE OBS build by adding subcategory to .desktop file

## [1.5.0] - 2018-08-13
### Added
- Server stats dialog
- Ability to set mounted directories for servers
- Ability to open torrents' files
- Notifications on added/finished torrents since last connection to server
- Ability to reannounce torrents
- Ability to set torrent's location

### Changed
- CMake build system

#### Sailfish OS
- File selection dialog now shows current directory

### Fixed
- Fixed segfault when disconnecting from server
- Fixed segfault when closing properties dialog
- Impoved support of self-signed certificates
- Active network requests are now aborted when manually disconnecting from server

## [1.4.0] - 2018-05-09
### Added
- Donation links via PayPal and Yandex.Money
- Filter torrents by download directory
- Show available free space when adding torrent
- Dutch, Flemish and Spanish translations

### Fixed
- Checking for Qt version in .pro file
- Translations installation
- Sailfish OS command line arguments

## [1.3.2] - 2017-03-05
### Changed
- Disable debug output in release builds

### Fixed
- Installation of translation files when build directory is project root directory

## [1.3.1] - 2017-03-02
### Changed
- Don't create a new thread for every async task, instead use QtConcurrent::run

### Fixed
#### Desktop
- 256x256 icon

## [1.3.0] - 2017-02-28
### Changed
- More correct handling of self-signed certificates (you may need to update server's configuration)
- Translation are now managed on Transifex.

## [1.2.2] - 2017-02-25
### Fixed
- Windows icons

## [1.2.1] - 2017-02-24
### Added
- Show authentication error

### Changed
- Show license in HTML format

### Fixed
#### Desktop
- Project URL in About dialog

## [1.2.0] - 2017-02-13
### Added
- Torrent properties are now reloaded after reconnection
- Rename torrent's files

#### Desktop
- Command line option to start minimized in notification area

### Changed
- Performance improvements for torrents with large number of files
- Accounts are renamed to Servers (config file name also changed)

### Removed
- Ability to select another file in Add Torrent File dialog (it was causing unnecessary code complication)

#### Desktop
- Settings options to start minimized in notification area. Use command line option instead

### Fixed
- Tracker status is now more correct
- Set correct torrent priority when adding torrent

## [1.1.0] - 2016-09-25
### Added
#### Sailfish OS
- Peers page now shows clients of peers.

### Changed
- Update data immediately after getting response from server when performing some actions on torrents (adding, starting/pausing, removing, checking, moving in queue).

### Fixed
- Update torrent name when it is changed on server (e.g. after retrieving torrent metadata if torrent was added by magnet link).

#### Desktop
- Don't hide main window on startup if tray icon is disabled.
- Fix showing temporary window if main window is hidden on startup.

## [1.0.0] - 2016-09-17
### Added
- Desktop user interface for GNU/Linux and Windows.

#### Sailfish OS
- You can now browse torrent's content when adding torrent file.
- Filter torrents by trackers.
- Search in torrents list.
- Multi-selection everywhere.
- Notifications on adding torrents and when disconnecting from server (all notification can be disabled in settings).
- Option for disabling connection on startup.
- Cover action for connecting/disconnecting.
- Tremotesf can now open torrent files and links from console or file manager (Sailfish OS version can open only one torrent at a time).
- Several new server settings options.

### Changed
- Entire project is rewrited from scratch.
- A lot of performance improvements.
- New configuration file format. Accounts now stored in separate file (with automatic migration from previous version).
- Switch to QMake build system

#### Sailfish OS
- New and more compact user interface.

### Fixed
- A lot of bugs.
