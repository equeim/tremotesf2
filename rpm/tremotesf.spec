# SPDX-FileCopyrightText: 2015-2022 Alexey Rochev
#
# SPDX-License-Identifier: CC0-1.0

%global app_id org.equeim.Tremotesf

Name:       tremotesf
Version:    2.0.0
Release:    1%{!?suse_version:%{?dist}}
Summary:    Remote GUI for transmission-daemon
%if %{defined suse_version}
Group:      Productivity/Networking/Other
License:    GPL-3.0-or-later
%else
License:    GPLv3+
%endif
URL:        https://github.com/equeim/tremotesf2

Source0:    https://github.com/equeim/tremotesf2/releases/download/%{version}/%{name}-%{version}.tar.gz

Requires:      hicolor-icon-theme
BuildRequires: cmake
BuildRequires: desktop-file-utils
BuildRequires: gettext
BuildRequires: make
BuildRequires: cmake(Qt5)
BuildRequires: cmake(Qt5Concurrent)
BuildRequires: cmake(Qt5Core)
BuildRequires: cmake(Qt5DBus)
BuildRequires: cmake(Qt5LinguistTools)
BuildRequires: cmake(Qt5Network)
BuildRequires: cmake(Qt5Test)
BuildRequires: cmake(Qt5Widgets)
BuildRequires: cmake(fmt)
BuildRequires: cmake(KF5WidgetsAddons)
BuildRequires: cmake(KF5WindowSystem)
BuildRequires: pkgconfig(openssl)

%if %{defined suse_version}
BuildRequires: appstream-glib
# OBS complains about not owned directories if hicolor-icon-theme isn't installed at build time
BuildRequires: hicolor-icon-theme
%else
    %if %{defined mageia}
BuildRequires: appstream-util
    %else
BuildRequires: libappstream-glib
    %endif
%endif

%if %{defined fedora} && "%{toolchain}" == "clang"
BuildRequires: clang
%else
BuildRequires: gcc-c++
%endif

%if %{undefined _metainfodir}
    %global _metainfodir %{_datadir}/metainfo
%endif

%description
Remote GUI for Transmission BitTorrent client.


%prep
%autosetup


%build
%cmake
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
