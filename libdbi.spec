%define name	libdbi
%define version	0.5
%define release 1

Summary:	Database Independent Abstraction Layer for C
Name:		%{name}
Version:	%{version}
Release:	%{release}
Group:		Libraries/Multimedia
Copyright:	LGPL
URL:		http://libdbi.sourceforge.net/
Vendor:		Neon Goat Productions
Packager:	David Parker <david@neongoat.com>
Source:		%{name}-%{version}.tar.gz
BuildRoot:	%{_tmppath}/%{name}-root

%description
libdbi provides developers with a database independent abstraction layer for C,
allowing for multiple simultaneous connections to different types of servers.
The plugin architecture allows for new database drivers to be easily added by a
third party.

%package devel
Summary: Database Independent Abstraction Layer development
Group: Development/Libraries
Requires:	%{name} = %{version}-%{release}

%description devel
The libdbi-devel package contains the header files and documentation
needed to develop applications with libdbi.

%package dbd-template
Summary: Template plugin for libdbi
Group: Development/Libraries
Requires:	%{name} = %{version}-%{release}

%description dbd-template
This plugin provides connectivity to template database servers through the
libdbi database independent abstraction layer. Switching a program's plugin
does not require recompilation or rewriting source code.

%prep
%setup -q -n %{name}-%{version}

%build
CFLAGS="$RPM_OPT_FLAGS" ./configure --prefix=/usr --with-template
make

%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT install

%files
%defattr(-,root,root)
%doc AUTHORS
%doc COPYING
%doc README
/usr/lib/libdbi.so.*

%files devel
%doc README
%doc TODO
%doc doc/programmers-guide.pdf
%doc doc/programmers-guide/*
/usr/include/dbi/dbi.h
/usr/include/dbi/dbi-dev.h
/usr/lib/libdbi.a
/usr/lib/libdbi.la
/usr/lib/libdbi.so

%files dbd-template
/usr/lib/dbd/libtemplate.so
/usr/lib/dbd/libtemplate.la

%clean 
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

%post
/sbin/ldconfig

%postun
/sbin/ldconfig

%changelog
* Wed Jul 11 2001 David Parker <david@neongoat.com>
- initial spec file created
