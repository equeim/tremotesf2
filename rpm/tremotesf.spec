%if %{?_vendor} == "meego"
%global sailfishos 1
Name:       harbour-tremotesf
%else
Name:       tremotesf
%endif
Version:    1.5.2
Release:    1%{!?sailfishos:%{!?suse_version:%{dist}}}
Summary:    Remote GUI for transmission-daemon
%if 0%{?suse_version}
Group:      Productivity/Networking/Other
License:    GPL-3.0-or-later
%else
License:    GPLv3+
%endif
URL:        https://github.com/equeim/tremotesf2

Source0:    https://github.com/equeim/tremotesf2/archive/%{version}.tar.gz

BuildRequires: cmake
BuildRequires: desktop-file-utils

%if 0%{?sailfishos}
Requires:      sailfishsilica-qt5
Requires:      nemo-qml-plugin-dbus-qt5
Requires:      nemo-qml-plugin-notifications-qt5
BuildRequires: pkgconfig(Qt5Concurrent)
BuildRequires: pkgconfig(Qt5Network)
BuildRequires: pkgconfig(Qt5Quick)
BuildRequires: qt5-qttools-linguist
BuildRequires: pkgconfig(sailfishapp)

%else
Requires:      hicolor-icon-theme
BuildRequires: cmake(Qt5Concurrent)
BuildRequires: cmake(Qt5Network)
BuildRequires: cmake(Qt5DBus)
BuildRequires: cmake(Qt5Widgets)
BuildRequires: cmake(Qt5LinguistTools)
BuildRequires: cmake(KF5WidgetsAddons)
# OBS complains about not owned directories if hicolor-icon-theme isn't installed at build time
%if 0%{?suse_version}
BuildRequires: hicolor-icon-theme
%endif
%endif


%global build_type Release
#%%global build_type Debug

%if 0%{?sailfishos}
%global build_directory build-%{_arch}
%else
%if 0%{?mageia}
%global build_directory build
%endif
%endif

%if ! 0%{?make_build:1}
%global make_build %{__make} %{?_smp_mflags}
%endif


%description
Remote GUI for Transmission BitTorrent client.


%prep
%setup -n tremotesf2-%{version}


%build
%if 0%{?sailfishos}
%{__mkdir_p} %{build_directory}
cd %{build_directory}
%cmake .. -DCMAKE_BUILD_TYPE=%{build_type} -DSAILFISHOS=ON
%else
%cmake %{!?suse_version:%{!?mageia:-DCMAKE_BUILD_TYPE=%{build_type}}}
%endif
%make_build


%install
%if 0%{?suse_version}
%cmake_install
%else
%if 0%{?build_directory:1}
cd %{build_directory}
%endif
%make_install
%endif

desktop-file-install \
    --delete-original \
    --dir "%{buildroot}/%{_datadir}/applications" \
    "%{buildroot}/%{_datadir}/applications/*.desktop"


%files
%defattr(-,root,root)
%{_bindir}/%{name}
%{_datadir}/icons/hicolor/*/apps/*
%{_datadir}/applications/*.desktop
%{_datadir}/%{name}

%changelog
* Tue Aug 14 2018 Alexey Rochev <equeim@gmail.com> - 1.5.1-1
- tremotesf-1.5.1

* Tue Aug 13 2018 Alexey Rochev <equeim@gmail.com> - 1.5.0-1
- tremotesf-1.5.0
