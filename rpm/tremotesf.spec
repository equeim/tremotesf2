%if "%{?_vendor}" == "meego"
%global sailfishos 1
%global app_id harbour-tremotesf
Name:       harbour-tremotesf
%else
%global app_id org.equeim.Tremotesf
Name:       tremotesf
%endif

Version:    1.11.1
Release:    1%{!?sailfishos:%{!?suse_version:%{?dist}}}
Summary:    Remote GUI for transmission-daemon
%if %{defined suse_version}
Group:      Productivity/Networking/Other
License:    GPL-3.0-or-later
%else
License:    GPLv3+
%endif
URL:        https://github.com/equeim/tremotesf2

Source0:    https://github.com/equeim/tremotesf2/archive/%{version}.tar.gz

BuildRequires: cmake
BuildRequires: desktop-file-utils

%if %{defined sailfishos}
Requires:      sailfishsilica-qt5
Requires:      nemo-qml-plugin-notifications-qt5
BuildRequires: ninja
BuildRequires: pkgconfig(Qt5Concurrent)
BuildRequires: pkgconfig(Qt5Core)
BuildRequires: pkgconfig(Qt5DBus)
BuildRequires: pkgconfig(Qt5Network)
BuildRequires: pkgconfig(Qt5Quick)
BuildRequires: qt5-qttools-linguist
BuildRequires: pkgconfig(sailfishapp)
%global __provides_exclude mimehandler
%else
Requires:      hicolor-icon-theme
BuildRequires: make
BuildRequires: cmake(Qt5)
BuildRequires: cmake(Qt5Concurrent)
BuildRequires: cmake(Qt5Core)
BuildRequires: cmake(Qt5DBus)
BuildRequires: cmake(Qt5LinguistTools)
BuildRequires: cmake(Qt5Network)
BuildRequires: cmake(Qt5Test)
BuildRequires: cmake(Qt5Widgets)
BuildRequires: cmake(Qt5X11Extras)
BuildRequires: cmake(KF5WidgetsAddons)
BuildRequires: cmake(KF5WindowSystem)
BuildRequires: gettext

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
        %if %{defined suse_version} && 0%{?suse_version} == 1500
BuildRequires: gcc11-c++
        %else
BuildRequires: gcc-c++
        %endif
    %endif
%endif

%if %{defined sailfishos}
    %global build_directory %{_builddir}/build-%{_target}-%(version | awk '{print $3}')
    %global debug 0
%else
    %if %{defined suse_version}
        %global _metainfodir %{_datadir}/metainfo
    %endif
%endif


%description
Remote GUI for Transmission BitTorrent client.


%prep
%setup -q -n tremotesf2-%{version}


%build
%if %{defined sailfishos}
# Enable -O0 for debug builds
# This also requires disabling _FORTIFY_SOURCE
    %if %{debug}
        export CFLAGS="${CFLAGS:-%optflags} -O0 -Wp,-U_FORTIFY_SOURCE"
        export CXXFLAGS="${CXXFLAGS:-%optflags} -O0 -Wp,-U_FORTIFY_SOURCE"
    %endif

    %cmake -S . -B %{build_directory} -G Ninja -D TREMOTESF_SAILFISHOS=ON -D TREMOTESF_BUILD_TESTS=OFF
    cmake --build %{build_directory} --parallel %{?_smp_mflags}
%else
    %if %{defined suse_version} && 0%{?suse_version} == 1500
        export CXX=g++-11
    %endif
    %cmake
    %cmake_build
%endif

%check
%if ! %{defined sailfishos}
    %ctest
%endif

%install
%if %{defined sailfishos}
    DESTDIR=%{buildroot} cmake --install %{build_directory}
%else
    %cmake_install
    appstream-util validate-relax --nonet %{buildroot}/%{_metainfodir}/%{app_id}.appdata.xml
%endif

desktop-file-validate %{buildroot}/%{_datadir}/applications/%{app_id}.desktop

%files
%{_bindir}/%{name}
%{_datadir}/icons/hicolor/*/apps/%{app_id}.*
%{_datadir}/applications/%{app_id}.desktop
%if ! %{defined sailfishos}
%{_metainfodir}/%{app_id}.appdata.xml
%endif

%changelog
* Wed Mar 16 2022 Alexey Rochev <equeim@gmail.com> - 1.11.2-1
- tremotesf-1.11.2

* Sun Feb 28 2022 Alexey Rochev <equeim@gmail.com> - 1.11.1-1
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
