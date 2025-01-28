Name:          nagios-mod-gearman
Version:       1.0.0
Release:       1%{?dist}
License:       GNU Public License version 3
Packager:      Nagios Development Team <devteam@nagios.com>
Vendor:        Nagios Enterprises
URL:           https://www.nagios.org/projects/nagios-mod-gearman/
Source0:       nagios-mod-gearman-%{version}.tar.gz
Group:         Applications/Monitoring
Summary:       Nagios Mod Gearman module for Nagios
BuildRoot:     %{_tmppath}/%{name}-%{version}-root-%(%{__id_u} -n)
Requires:      libgearman, logrotate, openssl
BuildRequires: autoconf, automake, gcc-c++, pkgconfig, ncurses-devel
BuildRequires: libtool, libtool-ltdl-devel, libevent-devel, openssl-devel
BuildRequires: libgearman-devel
Requires(pre,post): /sbin/ldconfig
BuildRequires: gearmand
BuildRequires: systemd

Provides:      nagios-mod-gearman

%description
Nagios Mod Gearman is a way of distributing active Nagios
checks across your network. It consists of two parts: There is a NEB module
which resides in the Nagios core and adds servicechecks, hostchecks and
eventhandler to a Gearman queue. There can be multiple equal gearman servers.
The counterpart is one or more worker clients for the checks itself. They can
be bound to host and servicegroups.

%prep
%setup -q

test -f configure || ./autogen.sh
%build
%configure \
     --datadir="%{_datadir}" \
     --datarootdir="%{_datadir}" \
     --localstatedir="%{_localstatedir}" \
     --sysconfdir="%{_sysconfdir}" \

%{__make} %{_smp_mflags}

%install
%{__rm} -rf %{buildroot}
%{__make} install \
     install-config \
     DESTDIR="%{buildroot}" \
     AM_INSTALL_PROGRAM_FLAGS=""
mkdir -p %{buildroot}/run/nagios-mod-gearman-worker

# Install systemd entry
%{__install} -D -m 0644 -p worker/daemon-systemd %{buildroot}%{_unitdir}/nagios-mod-gearman-worker.service


%pre
getent group nagios >/dev/null || groupadd -r nagios
getent passwd nagios >/dev/null || \
    useradd -r -g nagios -d %{_localstatedir}/nagios-mod-gearman -s /sbin/nologin \
    -c "nagios user" nagios

%post
%systemd_post nagios-mod-gearman-worker.service

%preun
%systemd_preun nagios-mod-gearman-worker.service

%clean
%{__rm} -rf %{buildroot}

%files
%attr(0644,root,root) %{_unitdir}/nagios-mod-gearman-worker.service
%config(noreplace) %{_sysconfdir}/nagios-mod-gearman/module.conf
%config(noreplace) %{_sysconfdir}/nagios-mod-gearman/worker.conf
%config(noreplace) %{_sysconfdir}/logrotate.d/nagios-mod-gearman

%dir %attr(755,nagios,nagios) /run/nagios-mod-gearman-worker

%{_bindir}/nagios-check-gearman
%{_bindir}/nagios-gearman-top
%{_bindir}/nagios-mod-gearman-worker
%{_bindir}/nagios-send-gearman
%{_bindir}/nagios-send-multi

%{_libdir}/nagios-mod-gearman/nagios-mod-gearman.o
%{_libdir}/nagios-mod-gearman/nagios-mod-gearman.so

%attr(755,nagios,root) %{_localstatedir}/nagios-mod-gearman
%attr(755,nagios,root) %{_localstatedir}/log/nagios-mod-gearman

%defattr(-,root,root)
%docdir %{_defaultdocdir}

%changelog
* Fri Dec 06 2024 Nagios
- 1.0.0 Build

