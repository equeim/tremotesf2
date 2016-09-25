# Change Log

## [Unreleased]
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
