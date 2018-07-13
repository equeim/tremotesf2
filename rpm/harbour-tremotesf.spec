Name: harbour-tremotesf
Summary: Remote GUI for transmission-daemon
Version: 1.4.0
Release: 1
Group: Applications/Internet
License: GPLv3
URL: https://github.com/equeim/tremotesf2
Source0: %{name}-%{version}.tar.xz
Requires: sailfishsilica-qt5
Requires: nemo-qml-plugin-dbus-qt5
Requires: nemo-qml-plugin-notifications-qt5
BuildRequires: pkgconfig(Qt5Core)
BuildRequires: pkgconfig(Qt5Concurrent)
BuildRequires: pkgconfig(Qt5Network)
BuildRequires: pkgconfig(Qt5Gui)
BuildRequires: pkgconfig(Qt5Qml)
BuildRequires: pkgconfig(Qt5Quick)
BuildRequires: pkgconfig(sailfishapp)
BuildRequires: cmake
BuildRequires: desktop-file-utils

# >> macros
#%%define __provides_exclude_from ^%{_datadir}/.*$
#%%define __requires_exclude ^libstdc++.*$
#%%define _cmake_skip_rpath -DCMAKE_SKIP_RPATH:BOOL=OFF
%define build_directory build-%{_arch}
# << macros

%description
Remote GUI for transmission-daemon

%prep
%setup -q -n %{name}-%{version}

%build
mkdir -p %{build_directory}
cd %{build_directory}

#build_type=Debug
build_type=Release
%cmake -DCMAKE_BUILD_TYPE=$build_type -DSAILFISHOS=ON -DCMAKE_INSTALL_RPATH="%{_datadir}/%{name}/lib" ..
make %{?_smp_mflags}
cd -

%install
rm -rf "%{buildroot}"
cd %{build_directory}
%make_install
cd -

#install -D /usr/lib/libstdc++.so.6 "%{buildroot}/%{_datadir}/%{name}/lib/libstdc++.so.6"

desktop-file-install --delete-original \
    --dir "%{buildroot}/%{_datadir}/applications" \
    "%{buildroot}/%{_datadir}/applications/%{name}.desktop"

%files
%defattr(644, root, root, 755)
%attr(755, -, -) %{_bindir}/%{name}
%{_datadir}/%{name}
%{_datadir}/icons
%{_datadir}/applications/%{name}.desktop
