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
BuildRequires: desktop-file-utils

%description
Remote GUI for transmission-daemon

%prep
%setup -q -n %{name}-%{version}

%build
mkdir -p build-sailfishos
pushd build-sailfishos

build_type=debug
#build_type=release

%qmake5 ../ CONFIG+=sailfishos CONFIG+=$build_type
make %{?_smp_mflags}
popd build-sailfishos

%install
echo %{buildroot}
rm -rf %{buildroot}
pushd build-sailfishos
%qmake5_install
popd build-sailfishos

desktop-file-install --delete-original \
    --dir %{buildroot}%{_datadir}/applications \
    %{buildroot}%{_datadir}/applications/%{name}.desktop

%files
%defattr(644, root, root, 755)
%attr(755, -, -) %{_bindir}/%{name}
%{_datadir}/%{name}
%{_datadir}/icons
%{_datadir}/applications/%{name}.desktop
