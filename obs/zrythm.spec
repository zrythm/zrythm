#
# spec file for package loopidity
#
# Copyright (c) 2012-2107 bill-auger bill-auger@programmer.net
#
# All modifications and additions to the file contributed by third parties
# remain the property of their copyright owners, unless otherwise agreed
# upon. The license for this file, and modifications and additions to the
# file, is the same license as for the pristine package itself (unless the
# license for the pristine package is not an Open Source License, in which
# case the license is the MIT License). An "Open Source License" is a
# license that conforms to the Open Source Definition (Version 1.9)
# published by the Open Source Initiative.

# Please submit bugfixes or comments via http://bugs.opensuse.org/


Name:          zrythm
Version:       0.1.009
Release:       1%{?dist}
Summary: An highly automated, intuitive, Digital Audio Workstation (DAW)
License:       GPL-3.0
URL:           https://www.zrythm.org
Source0:       https://git.zrythm.org/zrythm/%{name}/-/archive/v%{version}/%{name}-v%{version}.tar.gz
BuildRequires: gcc-c++ gcc automake pkgconfig autoconf
BuildRequires: gtk3-devel
BuildRequires: lilv-devel
BuildRequires: lv2-devel
BuildRequires: libdazzle-devel
BuildRequires: libsndfile-devel
BuildRequires: libyaml-devel
BuildRequires: portaudio-devel
Requires: gtk3
Requires: lilv
Requires: lv2
Requires: libdazzle
Requires: libyaml
Requires: libsndfile
Requires: portaudio
Requires: breeze-icon-theme
%if 0%{?suse_version}
BuildRequires: autoconf-archive jack-devel libX11-devel update-desktop-files
Requires:      jack
%endif
%if 0%{?fedora_version}
BuildRequires: jack-audio-connection-kit-devel libX11-devel
Requires:      jack-audio-connection-kit
%endif
%if 0%{?mageia}
BuildRequires: autoconf-archive libjack-devel libx11-devel
Requires:      jack
%endif
%description
  Zrythm is a native GNU/Linux application built with
  the GTK+3 toolkit and using the JACK Connection Kit for audio I/O.
  Zrythm can automate plugin parameters using built in LFOs and envelopes
  and is designed to be intuitive to use.


%prep
%autosetup

%build
autoreconf -fi
./configure --prefix=/usr
make %{?_smp_mflags}

%install
make DESTDIR="%{buildroot}/" install

%files
%defattr(-,root,root)
%license COPYING
%doc README.md CONTRIBUTING.md INSTALL.md
%{_bindir}/*
%{_datadir}/*

%post
%if ! 0%{?suse_version}
  xdg-desktop-menu forceupdate
%endif

%postun
%if ! 0%{?suse_version}
  xdg-desktop-menu forceupdate
%endif

%changelog
* Tue Mar 05 2019 Alexandros Theodotou
- v0.1.009
