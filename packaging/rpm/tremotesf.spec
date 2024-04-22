# SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
#
# SPDX-License-Identifier: CC0-1.0

%global app_id org.equeim.Tremotesf

%if %{defined suse_version} || 0%{?fedora} >= 40
%global qt_version 6
%else
%global qt_version 5
%endif

Name:       tremotesf
Version:    2.6.3
Release:    1%{!?suse_version:%{?dist}}
Summary:    Remote GUI for transmission-daemon
%if %{defined suse_version}
Group:      Productivity/Networking/Other
License:    GPL-3.0-or-later
%else
License:    GPLv3+
%endif
URL:        https://github.com/equeim/tremotesf2

Source0:    https://github.com/equeim/tremotesf2/releases/download/%{version}/%{name}-%{version}.tar.zst

Requires:      hicolor-icon-theme
BuildRequires: cmake
BuildRequires: desktop-file-utils
BuildRequires: gettext
BuildRequires: make
BuildRequires: zstd
BuildRequires: cmake(Qt%{qt_version})
BuildRequires: cmake(Qt%{qt_version}Concurrent)
BuildRequires: cmake(Qt%{qt_version}Core)
BuildRequires: cmake(Qt%{qt_version}DBus)
BuildRequires: cmake(Qt%{qt_version}LinguistTools)
BuildRequires: cmake(Qt%{qt_version}Network)
BuildRequires: cmake(Qt%{qt_version}Test)
BuildRequires: cmake(Qt%{qt_version}Widgets)
BuildRequires: cmake(fmt)
BuildRequires: cmake(KF%{qt_version}WidgetsAddons)
BuildRequires: cmake(KF%{qt_version}WindowSystem)
BuildRequires: cmake(cxxopts)
BuildRequires: pkgconfig(libpsl)
BuildRequires: openssl-devel

%if %{defined fedora}
BuildRequires: cmake(httplib)
BuildRequires: libappstream-glib
%if "%{toolchain}" == "clang"
BuildRequires: clang
%else
BuildRequires: gcc-c++
%endif
%if %{qt_version} == 5
Requires: kwayland-integration
%endif
%global tremotesf_with_httplib system
%endif

%if %{defined suse_version}
BuildRequires: pkgconfig(cpp-httplib)
BuildRequires: appstream-glib
# OBS complains about not owned directories if hicolor-icon-theme isn't installed at build time
BuildRequires: hicolor-icon-theme
%global _metainfodir %{_datadir}/metainfo
%global tremotesf_with_httplib system
%endif

%if %{defined mageia}
BuildRequires: appstream-util
%if %{qt_version} == 5
Requires: kwayland-integration
%endif
%global tremotesf_with_httplib bundled
%endif

%description
Remote GUI for Transmission BitTorrent client.


%prep
%autosetup


%build
%cmake -D TREMOTESF_QT6=%[%{qt_version} == 6 ? "ON" : "OFF"] -D TREMOTESF_WITH_HTTPLIB=%{tremotesf_with_httplib}
%cmake_build

%check
%ctest

%install
%cmake_install
appstream-util validate-relax --nonet %{buildroot}/%{_metainfodir}/%{app_id}.appdata.xml
desktop-file-validate %{buildroot}/%{_datadir}/applications/%{app_id}.desktop

%files
%{_bindir}/%{name}
%{_datadir}/icons/hicolor/*/apps/%{app_id}.*
%{_datadir}/applications/%{app_id}.desktop
%{_metainfodir}/%{app_id}.appdata.xml

%changelog
* Mon Apr 22 2024 Alexey Rochev <equeim@gmail.com> - 2.6.3-1
- tremotesf-2.6.3

* Mon Apr 01 2024 Alexey Rochev <equeim@gmail.com> - 2.6.2-1
- tremotesf-2.6.2

* Sun Mar 17 2024 Alexey Rochev <equeim@gmail.com> - 2.6.1-1
- tremotesf-2.6.1

* Mon Jan 08 2024 Alexey Rochev <equeim@gmail.com> - 2.6.0-1
- tremotesf-2.6.0

* Sun Oct 15 2023 Alexey Rochev <equeim@gmail.com> - 2.5.0-1
- tremotesf-2.5.0

* Tue May 30 2023 Alexey Rochev <equeim@gmail.com> - 2.4.0-1
- tremotesf-2.4.0

* Sun Apr 30 2023 Alexey Rochev <equeim@gmail.com> - 2.3.0-1
- tremotesf-2.3.0

* Tue Mar 28 2023 Alexey Rochev <equeim@gmail.com> - 2.2.0-1
- tremotesf-2.2.0

* Sun Mar 12 2023 Alexey Rochev <equeim@gmail.com> - 2.1.0-1
- tremotesf-2.1.0

* Sat Nov 05 2022 Alexey Rochev <equeim@gmail.com> - 2.0.0-1
- tremotesf-2.0.0

* Fri Mar 18 2022 Alexey Rochev <equeim@gmail.com> - 1.11.3-1
- tremotesf-1.11.3

* Wed Mar 16 2022 Alexey Rochev <equeim@gmail.com> - 1.11.2-1
- tremotesf-1.11.2

* Mon Feb 28 2022 Alexey Rochev <equeim@gmail.com> - 1.11.1-1
- tremotesf-1.11.1

* Sun Feb 13 2022 Alexey Rochev <equeim@gmail.com> - 1.11.0-1
- tremotesf-1.11.0

* Mon Sep 27 2021 Alexey Rochev <equeim@gmail.com> - 1.10.0-1
- tremotesf-1.10.0

* Mon May 10 2021 Alexey Rochev <equeim@gmail.com> - 1.9.1-1
- tremotesf-1.9.1

* Tue May 04 2021 Alexey Rochev <equeim@gmail.com> - 1.9.0-1
- tremotesf-1.9.0

* Sun Sep 06 2020 Alexey Rochev <equeim@gmail.com> - 1.8.0-1
- tremotesf-1.8.0

* Sat Jun 27 2020 Alexey Rochev <equeim@gmail.com> - 1.7.1-1
- tremotesf-1.7.1

* Fri Jun 05 2020 Alexey Rochev <equeim@gmail.com> - 1.7.0-1
- tremotesf-1.7.0

* Sat Jan 11 2020 Alexey Rochev <equeim@gmail.com> - 1.6.4-1
- tremotesf-1.6.4

* Sun Jan 05 2020 Alexey Rochev <equeim@gmail.com> - 1.6.3-1
- tremotesf-1.6.3

* Sun Jan 05 2020 Alexey Rochev <equeim@gmail.com> - 1.6.2-1
- tremotesf-1.6.2

* Tue Jul 16 2019 Alexey Rochev <equeim@gmail.com> - 1.6.1-1
- tremotesf-1.6.1

* Sat Jan 26 2019 Alexey Rochev <equeim@gmail.com> - 1.6.0-1
- tremotesf-1.6.0

* Sat Dec 08 2018 Alexey Rochev <equeim@gmail.com> - 1.5.6-1
- tremotesf-1.5.6

* Wed Sep 26 2018 Alexey Rochev <equeim@gmail.com> - 1.5.5-1
- tremotesf-1.5.5

* Mon Sep 10 2018 Alexey Rochev <equeim@gmail.com> - 1.5.4-1
- tremotesf-1.5.4

* Mon Sep 03 2018 Alexey Rochev <equeim@gmail.com> - 1.5.3-1
- tremotesf-1.5.3

* Sat Aug 18 2018 Alexey Rochev <equeim@gmail.com> - 1.5.2-1
- tremotesf-1.5.2

* Tue Aug 14 2018 Alexey Rochev <equeim@gmail.com> - 1.5.1-1
- tremotesf-1.5.1

* Mon Aug 13 2018 Alexey Rochev <equeim@gmail.com> - 1.5.0-1
- tremotesf-1.5.0
