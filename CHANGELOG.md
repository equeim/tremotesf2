# Changelog

## [Unreleased]
### Added
- Status bar displays free space in the download directory (thanks @keizie)
- Priority column in torrents list now shows colored icon (thanks @keizie)

### Fixed
- Label/directory/tracker filter lists in the sidebar now use natural sorting

## [2.9.1] - 2025-12-09
### Fixed
- Changed maximum values of idle limits and speed limits settings to align them with Transmission limitations

### Changed
- Increased default width of some dialogs (thanks @glutenmancy)
- Updated KDE Flatpak runtime to 6.10 branch
- Updated translations

## [2.9.0] - 2025-10-03
### Added
- The first column in torrents, trackers, and peers lists can now be reordered too (thanks @keizie)
- Torrents can be renamed using F2 shortcut (thanks @keizie)
- Torrent's file list show appropriate file icons based on file extensions

### Fixed
- Fixed opening torrent's directory when torrent has a single file in a directory (thanks @keizie)
- Fixed window activation when opening torrent's file or its download directory on Linux/Wayland
- Relocating the torrent does not save the directory for use in "Add torrent" dialog (thanks @keizie)
- Improved support of servers that use HTTPS certificate chain with custom root CA

### Changed
- Minimum dependencies baseline has been raised:
  - GCC 14 or Clang 19 (MSVC and Apple Clang are always required to be the latest available versions)
  - Qt 6.8 (Qt 5 support is removed)
  - KDE Frameworks 6.11
  - fmt 10.1.0
  - cxxopts 3.2.1
  - cpp-httplib 0.18.7
  - gettext 0.22.5
- Clarified dependency on cpp-httplib with OpenSSL support enabled, not OpenSSL itself
- If your server uses HTTPS certificate chain with custom root CA (instead of single self-signed certificate),
  then you need to specify the root certificate in the connection settings, and optionally the leaf certificate if it doesn't have correct host name

### Removed
- Qt 5 support
- Native packages for Debian older than 13 and Ubuntu older than 25.04 (and other distros with outdated packages). Users of these distros can still install Tremotesf through Flatpak

## [2.8.2] - 2025-04-16
### Fixed
- Crash when failing to parse server response as JSON

## [2.8.1] - 2025-04-12
### Fixed
- Not working file dialogs when installed through Flatpak

## [2.8.0] - 2025-04-09
### Added
- Option to show torrent properties in a panel in the main window instead of dialog
- Ability to set labels on torrents and filter torrent list by labels
- Option to display relative time
- Option to display only names of download directories in sidebar and torrents list

### Changed
- Options dialog is rearranged to use multiple tabs
- Message that's shown when trying to add torrent while disconnected from server is now displayed in a dialog instead of main window

### Fixed
- Delayed loading of peers for active torrents
- Window activation from clicking on notification

## [2.7.5] - 2025-01-14
### Added
- Windows on ARM64 support

### Changed
- Windows builds now use system TLS library (schannel) instead of OpenSSL
- Various hardening GCC and Clang compiler options are applied:
  - `-fhardened` with GCC >= 14
  - `-ftrivial-auto-var-init=pattern`, `-fstack-protector-strong`
  - `-fstack-clash-protection` on Linux and FreeBSD
  - `-fcf-protection=full` on x86_64
  - `-mbranch-protection=standard` on ARM64
  - `-D_FORTIFY_SOURCE=3`
  - `-D_GLIBCXX_ASSERTIONS` with libstdc++
  - `-D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_FAST` with libc++ >= 18

### Fixed
- Failures to add torrents when "Delete .torrent file" option is enabled
- Compilation errors with fmt 11.1
- Debug logs being enabled in release builds in some cases

## [2.7.4] - 2024-12-06
### Fixed
- Tray icon disappearing in some X11 environments
- Wrong translation being loaded on Windows

## [2.7.3] - 2024-11-20
### Fixed
- Black screen issues when closing fullscreen window on macOS
- File dialog being shown twice in some Linux environments
- Crash with GCC 12
- Torrent's details in list not being updated for most recently added torrent

## [2.7.2] - 2024-09-15
### Fixed
- Opening download directory of a torrent with some file managers

## [2.7.1] - 2024-09-13
### Added
- Dialog is shown when fatal error occurs on Windows
- TREMOTESF_ASAN CMake option to build with AddressSanitizer (off by default)

### Fixed
- Performance regression on Windows (and potential performance improvements on other platforms)
- Crash on Windows
- Issues with mounted directories mapping

## [2.7.0] - 2024-08-31
### Added
- Merging trackers when adding existing torrent
- Add Torrent Link dialogs allows multiple links
- "None" proxy option to bypass system proxy

### Changed
- Removed Debian 11 and Ubuntu 22.04 support - minimum baseline now corresponds to Debian 12
  - Minimum CMake version is 3.25
  - Minimum fmt version is 9.1
  - Minimum KF5 version is 5.103
  - Minimum libpsl version is 0.21.2
  - Minimum cxxopts version is 3.1.1
  - Minimum gettext version is 0.21
- Removed dependency on Qt Concurrent module
- Breeze is used as a fallback icon theme and should be installed as a runtime dependency
- Clarified runtime dependency on Qt's SVG image format plugin
- Notification portal is used for notifications in Flatpak
- Added workaround for Transmission not showing an error for torrent when all trackers have failed
- Networking and some other async code is rewritten using C++ coroutines. Hopefully nothing is broken :)

### Fixed
- Mapping of mounted directories working incorrectly in some cases

## [2.6.3] - 2024-04-22
### Fixed
- Qt 6.7 compatibility

## [2.6.2] - 2024-04-01
### Fixed
- Application being closed when opening file picker in Qt 6 builds

## [2.6.1] - 2024-03-17
### Added
- Added TREMOTESF_WITH_HTTPLIB CMake option to control how cpp-httplib test dependency is searched. Possible values:
  - auto: CMake find_package call, otherwise pkg-config, otherwise bundled copy is used.
  - system: CMake find_package call, otherwise pkg-config, otherwise fatal error.
  - bundled: bundled copy is used.
  - none: cpp-httplib is not used at all and tests that require it are disabled.

### Changed
- Qt 6 is now used by default instead of Qt 5. You can override it with TREMOTESF_QT6=OFF CMake option
- Flatpak build uses Qt 6
- openSUSE build uses Qt 6

### Fixed
- Clarified dependency on kwayland-integration
- Sorting of directories and trackers in side panel
- Menu items that should disabled on first start not being disabled
- Selecting of current server via status bar context menu being broken in some cases
- Debug logs being printed when they are disabled

## [2.6.0] - 2024-01-08
### Added
- macOS support
- Option to open torrent's file or download directory on double click
- Option to not activate main window when adding torrents (except on macOS where application is always activated)
- Option to not show "Add Torrent" dialog when adding torrents
- Right click on status bar opens menu to quickly connect to different server
- Support of xdg-activation protocol on Wayland (kwayland-integration is required as a runtime dependency)

### Changed
- "Open" and "Show in file manager" actions now show error dialog if file/directory does not exist,
  instead if being inaccessible
- "Show in file manager" actions has been renamed to "Open download directory"
- Progress bar's text is now drawn according to Qt style (though Breeze style still draws text next to the progress bar, not inside of it)

### Fixed
- Initial state of "Lock toolbar" menu action
- Progress bar being drawn in incorrect column in torrent's files list

## [2.5.0] - 2023-10-15
### Added
- Qt 6 support (with unreleased KF6 libraries)
- Option to move torrent file to trash instead of deleting it.
  It is enabled by default, and fallbacks to deletion if move failed for any reason.

### Changed
- Windows builds use Qt 6
- Progress bar columns in torrent/file lists are now displayed with percent text
- Default columns and sort order in torrents list are changed according to my personal taste: default sorting is now by added date, from new to old
- When search field is focused via shortcut its contents are now selected
- When Tremotesf is launched for the first time it now doesn't place itself in the middle of the screen, letting OS decide

### Removed
- Windows 8.1 and Windows 10 versions prior to 1809 (October 2018 Update) are not supported anymore

### Fixed
- "Open" action on torrent's root file directory
- Saving of settings and window state when on logout/reboot/shutdown on Windows
- Unnecessary RPC requests when torrent's limits are edited

## [2.4.0] - 2023-05-30
### Added
- "Add torrent" dialog now has checkbox to remove torrent file when torrent is added

### Changed
- "Remember last download directory" is replaced with "Remember parameters of last added torrent",
  which also remembers priority, started/paused state and "Delete .torrent file" checkbox of last added torrent
- "Remember last torrent open directory" setting is renamed to "Remember location of last opened torrent file"
- When "Remember last torrent open directory" is unchecked user's home directory is always used
- When authentication is enabled, `Authorization` header will be sent in advance
  instead of waiting for 401 response from server (thanks @otaconix)

### Fixed
- Errors when opening certain torrent files
- Incorrect error message being displayed when there is no configured servers

## [2.3.0] - 2023-04-30
### Changed
- Tremotesf now requires compiler with some C++20 support (concepts, ranges and comparison operators)

### Fixed
- Crash on launch with some Qt styles
- Mounted directories feature not working on Windows with UNC paths (those starting with \\\\)
- Incorrect error message when adding torrent that already exists with some Transmission versions

## [2.2.0] - 2023-03-28
### Added
- Torrent priority selection in torrent's context menu

### Changed
- Torrent status icons are redrawn to be more contrasting

### Fixed
- Torrent status icons being pixelated on high DPI displays
- Torrent priority column being empty
- Zero number of leechers
- No icon in task switcher in KDE Plasma Wayland

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
