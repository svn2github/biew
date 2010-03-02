########################################################################################################
# This is a .spec file for building beye.rpm packages.
#
# Usage:
# rpmbuild -bb --define "version $VERSION" --target i686-linux beye.spec
# or:
# rpmbuild -ba --define "version $VERSION" --target x86_64-linux beye.spec
#
# For fast linkage:
# rpmbuild -bb --target CPU-linux --short-circuit beye.spec
#
# For testing package.rpm:
# rpm -qp --queryformat "%{arch}\n" beye-*.rpm
# rpm -qp --queryformat "%{os}\n" beye-*.rpm
########################################################################################################
%define name	beye
%define release	1

%define prefix	/usr
%define bindir	%{prefix}/bin
%define datadir	%{prefix}/share
%define mandir	%{datadir}/man

Name:		%{name}
Version:	%{version}
Release:	%{release}
Prefix:		%{prefix}
Summary:	console hex viewer/editor and disassembler
Copyright:	GPL
Group:		Development/Other
Packager:	Nickols_k <nickols_k@mail.ru>
URL:		http://beye.sourceforge.net

Source:		%{name}-%{version}-src.tar.bz2

Autoreq:	1

%description
BEYE (Binary EYE) is a free, portable, advanced file viewer with
built-in editor for binary, hexadecimal and disassembler modes.

It contains a highlight i86/i386/Athlon64/Java/AVR/ARM-XScale/PPC disassembler,
full preview of MZ, NE, PE, LE, LX, DOS.SYS, NLM, ELF, a.out, arch,
coff32, PharLap, rdoff executable formats, a code guider, and lot of
other features, making it invaluable for examining binary code.

Linux, Unix, QNX, BeOS, DOS, Win32, OS/2 versions are available.

%if %_target_cpu==x86_64
%define         bitness 64
%define         lib     lib64
%define         gcc     "gcc -m64"
%define         host    "x86_64-unknown-linux-gnu"
%define         ld_library_path "$LD_LIBRARY_PATH:usr/%{lib}:/usr/%{lib}/xorg"
%define         pkg_config_path "$PKG_CONFIG_PATH:$PKG64_CONFIG_PATH:/usr/local/%{lib}"
%elseif %_target_cpu==i686
%define         bitness 32
%define         lib     lib
%define         gcc     "gcc -m32"
%define         host    "i686-unknown-linux-gnu"
%define         ld_library_path "$LD_LIBRARY_PATH:usr/%{lib}:/usr/%{lib}/xorg"
%define         pkg_config_path "$PKG_CONFIG_PATH:$PKG32_CONFIG_PATH:/usr/local/%{lib}"
%else
# generic or unknown arch-os
%define         gcc     "gcc"
%endif

%prep
%setup -q

%build
export LD_LIBRARY_PATH=%{ld_library_path}
export PKG_CONFIG_PATH=%{pkg_config_path}
DESTDIR=$RPM_BUILD_ROOT CC=%{gcc} ./configure --prefix=%{prefix} --host=%{host}
make

./configure --prefix=%{prefix}
make

%install
rm -rf $RPM_BUILD_ROOT
make install

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc doc/beye_en.txt doc/beye_ru.txt doc/unix.txt doc/release.txt

%{bindir}/%{name}
%{datadir}/%{name}/*
%{mandir}/man?/%{name}.1*

%changelog
* 27 feb 2010 Nickols_K <nickols_k@mail.ru> beye-1.0.0
- add crossbuild support
* Sun Jan 6 2002 konst <konst@linuxassembly.org> 5.3.2-1
- build from the original source archive
* Fri Jan 4 2002 konst <konst@linuxassembly.org> 5.3.1-2
- explicitly define TARGET_CPU, TARGET_OS, USE_MOUSE, PREFIX
* Wed Aug 15 2001 konst <konst@linuxassembly.org> 5.3.0-1
- moved skins to skn folder
