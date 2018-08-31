# Changelog

## [Unreleased]
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
