# vim:ft=automake noexpandtab
###############################################################################
#
# nagios-mod-gearman - distribute checks with gearman
#
# Copyright (c) 2010 Sven Nierlein
# Copyright: 2024 Nagios Development Team <devteam@nagios.com>
# License: GPL-3
#
###############################################################################

RPM_TOPDIR=$$(pwd)/rpm.topdir
DOS2UNIX=$(shell which dos2unix || which fromdos)

.PHONY: docs

AM_CPPFLAGS=-Iinclude $(CFLAGS)
AM_CFLAGS =-DDATADIR='"$(datadir)"'
LIBS = $(GEARMAN_LIBS) $(openssl_LIBS)
OS=$(shell uname)
# disable parallel builds which sometime run into ranlib file not found errors
MAKEFLAGS=-j1
DAILYVERSION=$(shell ./get_version)

# source definitions
common_SOURCES             = common/gm_crypt.c  \
                             common/gearman_utils.c \
                             common/utils.c \
                             common/gm_alloc.c

common_check_SOURCES       = common/check_utils.c \
                             common/popenRWE.c \
                             worker/worker_client.c

pkglib_LIBRARIES           =
NEB_MODULES                =
pkglib_LIBRARIES           += nagios-mod-gearman.so
nagios_mod_gearman_so_SOURCES = $(common_SOURCES) \
                             neb_module/result_thread.c \
                             neb_module/mod_gearman.c
NEB_MODULES               += nagios-mod-gearman.o

bin_PROGRAMS               = nagios-mod-gearman-worker \
                             nagios-send-gearman \
                             nagios-send-multi \
                             nagios-check-gearman \
                             nagios-gearman-top

nagios_mod_gearman_worker_SOURCES = $(common_SOURCES) \
                             $(common_check_SOURCES) \
                             worker/worker.c

nagios_send_gearman_SOURCES       = $(common_SOURCES) \
                             tools/send_gearman.c

nagios_send_multi_SOURCES         = $(common_SOURCES) \
                             tools/send_multi.c

nagios_check_gearman_SOURCES      = $(common_SOURCES) \
                             tools/check_gearman.c

nagios_gearman_top_SOURCES        = $(common_SOURCES) \
                             tools/gearman_top.c
nagios_gearman_top_LDADD          = -lncurses

# tests
check_PROGRAMS   = 01_utils 02_full 03_exec 04_log 05_neb 06_exec
#check_PROGRAMS  += 08_roundtrip
01_utils_SOURCES = $(common_SOURCES) t/tap.h t/tap.c t/01-utils.c $(common_check_SOURCES)
02_full_SOURCES  = $(common_SOURCES) t/tap.h t/tap.c t/02-full.c $(common_check_SOURCES)
03_exec_SOURCES  = $(common_SOURCES) t/tap.h t/tap.c t/03-exec_checks.c $(common_check_SOURCES)
04_log_SOURCES   = $(common_SOURCES) t/tap.h t/tap.c t/04-log.c
05_neb_SOURCES   = $(common_SOURCES) t/tap.h t/tap.c t/05-neb.c
05_neb_LDFLAGS   = -Wl,--export-dynamic -rdynamic
05_neb_LDADD     = -ldl
# only used for performance tests
06_exec_SOURCES  = $(common_SOURCES) t/tap.h t/tap.c t/06-execvp_vs_popen.c $(common_check_SOURCES)
#08_roundtrip_SOURCES  = $(common_SOURCES) t/08-roundtrip.c
#08_roundtrip_LDFLAGS = -Wl,--export-dynamic -rdynamic
TESTS            = $(check_PROGRAMS) t/09-benchmark.t t/10-large-result.t t/11-alloc.t t/12-cppcheck.t t/13-tools.t t/14-symbols.t


GEARMANDS=/usr/sbin/gearmand /opt/sbin/gearmand
replace_vars = sed -e 's:%CONFIG_WORKER%:$(sysconfdir)/nagios-mod-gearman/worker.conf:g' \
               -e 's:%PIDFILE%:$(localstatedir)/nagios-mod-gearman/nagios-mod-gearman-worker.pid:g' \
               -e 's:%LOGFILE_NEB%:$(localstatedir)/log/nagios-mod-gearman/nagios-mod-gearman-neb.log:g' \
               -e 's:%LOGFILE_WORKER%:$(localstatedir)/log/nagios-mod-gearman/nagios-mod-gearman-worker.log:g' \
               -e 's:%GPIDFILE%:$(localstatedir)/nagios-mod-gearman/gearmand.pid:g' \
               -e 's:%GLOGFILE%:$(localstatedir)/nagios-mod-gearman/gearmand.log:g' \
               -e 's:%WORKERBIN%:$(bindir)/nagios-mod-gearman-worker:g' \
               -e 's:%USER%:$(user):g'

EXTRA_DIST = COPYING etc/*.in include neb_module \
             THANKS README.md docs/README.html Changes worker/daemon-systemd.in \
             support/nagios-mod-gearman.spec \
             t/data/* t/rc t/both t/killer t/sleep t/*.pl t/*.t t/05-neb.c \
             t/test_all.pl \
             etc/nagios-mod-gearman-logrotate ./autogen.sh \
             debian

# avoid: ar: `u' modifier ignored since `D' is the default (see `U')
export ARFLAGS=cr 

# other targets
nagios-mod-gearman.o: $(nagios_mod_gearman_so_OBJECTS) $(nagios_mod_gearman_so_DEPENDENCIES)
	@echo '    $$(CC) $<'
	@if [ "$(OS)" = "Darwin" ]; then \
		$(CXX) $(LDFLAGS) -dynamiclib -single_module -undefined dynamic_lookup $(nagios_mod_gearman_so_OBJECTS) -o $@ -lpthread -lgearman; \
	else \
		$(CXX) $(LDFLAGS) -fPIC -shared $(nagios_mod_gearman_so_OBJECTS) -o $@ -lpthread -lgearman; \
	fi
	chmod 644 nagios-mod-gearman.o


mod_gearman_neb.conf-local:
	@$(replace_vars) etc/nagios-mod-gearman-neb.conf.in > etc/nagios-mod-gearman-neb.conf

mod_gearman_worker.conf-local:
	@$(replace_vars) etc/nagios-mod-gearman-worker.conf.in > etc/nagios-mod-gearman-worker.conf

initscript-local:
	@$(replace_vars) worker/daemon-systemd.in > worker/daemon-systemd

all-local: $(NEB_MODULES) initscript-local mod_gearman_neb.conf-local mod_gearman_worker.conf-local
	@echo ""
	@echo "################################################################"
	@echo ""
	@echo "  the following files have been created:"
	@echo ""
	@echo "  nagios-mod-gearman.o"
	@echo "  nagios-mod-gearman-worker"
	@echo "  worker/daemon-systemd"
	@echo "  etc/nagios-mod-gearman-neb.conf"
	@echo "  etc/nagios-mod-gearman-worker.conf"
	@echo ""
	@echo "  read the README for configuration details"
	@echo ""
	@echo "  for a normal installation continue with"
	@echo "  make install"
	@echo ""
	@echo "################################################################"

distclean-local: clean
	$(RM) -rf .deps/ Makefile.in aclocal.m4 ar-lib autom4te.cache config.* compile configure configure~ depcomp install-sh missing test-driver *.gz common/.deps/ contrib/.deps/ contrib/.dirstamp neb_module/.deps/ tools/.deps/ worker/.deps/ debian/.debhelper

clean-local:
	$(RM) -f worker.static \
             worker/daemon-systemd \
             etc/nagios-mod-gearman.conf \
             *.o \
             */*.o \
             etc/nagios-mod-gearman-neb.conf \
             etc/nagios-mod-gearman-worker.conf \
             debian/*.8 \
             debian/files \
             debian/debhelper-build-stamp \
	     debian/autoreconf.* 

	$(RM) -fr rpm.topdir \
             debian/nagios-mod-gearman-doc \
             debian/nagios-mod-gearman-module \
             debian/nagios-mod-gearman-tools \
             debian/nagios-mod-gearman-worker \
             debian/tmp \
             debian/.debian

worker.static: worker
	@echo "################################################################"
	@echo ""
	@echo " if the static compiler complains about 'cannot find -lgearman', you have to"
	@echo " compile libgearman with -static"
	@echo ""
	@echo "################################################################"
	cd worker && $(CC) $(LDFLAGS) $(CFLAGS) -static -o worker.static $(worker_OBJECTS) -lgearman -lpthread -luuid
	@echo ""
	@echo " worker.static created."
	@echo ""

install-exec-local: install-local-state-dir
	$(install_sh_PROGRAM) -m 644 nagios-mod-gearman.o $(DESTDIR)$(pkglibdir)/nagios-mod-gearman.o
	@mkdir -p /run/nagios-mod-gearman-worker
	@if [ "x$systemddir" != "x" ]; then \
		$(install_sh_PROGRAM) -m 644 worker/daemon-systemd $(systemddir)/nagios-mod-gearman-worker.service; \
	fi


install-local-state-dir:
	$(install_sh_PROGRAM) -d $(DESTDIR)$(localstatedir)/nagios-mod-gearman/

install-data-local: install-local-state-dir
	$(install_sh_PROGRAM) -d $(DESTDIR)$(localstatedir)/log/nagios-mod-gearman
	@echo ""
	@echo "################################################################"
	@echo ""
	@echo " Installation completed:"
	@echo " neb module:         $(DESTDIR)$(pkglibdir)/nagios-mod-gearman.o"
	@echo ""
	@echo " worker:             $(DESTDIR)$(bindir)/nagios-mod-gearman-worker"
	@echo " systemd service:    $(systemddir)/nagios-mod-gearman-worker.service"
	@echo ""
	@echo " check bin:          $(DESTDIR)$(bindir)/nagios-check-gearman"
	@echo " send bin:           $(DESTDIR)$(bindir)/nagios-send-gearman"
	@echo " send multi bin:     $(DESTDIR)$(bindir)/nagios-send-multi"
	@echo ""
	@echo "just add the broker line to your nagios.cfg:"
	@echo "broker_module=$(DESTDIR)$(pkglibdir)/nagios-mod-gearman.o config=$(DESTDIR)$(sysconfdir)/nagios-mod-gearman/module.conf"
	@echo ""
	@if [ -e $(DESTDIR)$(sysconfdir)/nagios-mod-gearman/module.conf -o -e $(DESTDIR)$(sysconfdir)/nagios-mod-gearman/worker.conf ]; then \
	    echo " install default config with"; \
	    echo ""; \
	    echo " make install-config"; \
	    echo ""; \
	else \
		make install-config; \
	fi

install-config:
	$(install_sh_PROGRAM) -m 644 etc/nagios-mod-gearman-neb.conf    $(DESTDIR)$(sysconfdir)/nagios-mod-gearman/module.conf
	$(install_sh_PROGRAM) -m 644 etc/nagios-mod-gearman-worker.conf $(DESTDIR)$(sysconfdir)/nagios-mod-gearman/worker.conf
	$(install_sh_PROGRAM) -m 644 etc/nagios-mod-gearman-logrotate   $(DESTDIR)$(sysconfdir)/logrotate.d/nagios-mod-gearman
	@echo "################################################################"
	@echo ""
	@echo " configuration:"
	@echo "         neb module:  $(DESTDIR)$(sysconfdir)/nagios-mod-gearman/module.conf"
	@echo "         worker:      $(DESTDIR)$(sysconfdir)/nagios-mod-gearman/worker.conf"
	@echo "         logrotate.d: $(DESTDIR)$(sysconfdir)/logrotate.d/nagios-mod-gearman"
	@echo ""
	@echo "################################################################"

test: check
	@echo "################################################################"
	@echo ""
	@echo " All tests completed successfully"
	@echo ""
	@echo "################################################################"
	@rm -f *.trs *.log t/*.log t/*.trs

fulltest:
	./t/test_all.pl
	@echo "################################################################"
	@echo ""
	@echo " Full tests completed successfully"
	@echo ""
	@echo "################################################################"
	@rm -f *.trs *.log t/*.log t/*.trs

docs:
	@if [ -z "$(DOS2UNIX)" ]; then \
	    printf "\n\n**** please install dos2unix or tofrodos package ****\n\n"; \
	    exit 1; \
	fi
	@if [ `type doxygen > /dev/null 2>&1` ]; then \
	    doxygen Doxyfile; \
	else \
	    printf "\n\n**** please install doxygen to generate doxygen docs ****\n\n"; \
	fi;
	$(RM) docs/images
	cd docs && ln -s /usr/share/asciidoc/images .
	cp -p README.md docs/README && cd docs && asciidoc --unsafe -a toc -a toclevels=2 -a max-width=800 README
	chmod 644 docs/README.html
	$(DOS2UNIX) docs/README.html
	./replace_doc_toc.pl docs/README.html
	$(RM) -f docs/README

rpm: dist
	mkdir -p $(RPM_TOPDIR)/{SOURCES,BUILD,RPMS,SRPMS,SPECS}
	cp nagios-mod-gearman-$(VERSION).tar.gz $(RPM_TOPDIR)/SOURCES
	rpmbuild -ba --define "_topdir $(RPM_TOPDIR)" \
	    --buildroot=$$(pwd)/rpm.buildroot support/nagios-mod-gearman.spec
	mv -v $(RPM_TOPDIR)/RPMS/*/*.rpm .
	mv -v $(RPM_TOPDIR)/SRPMS/*.src.rpm .
	$(RM) -fr $(RPM_TOPDIR) rpm.buildroot

deb:
	dpkg-buildpackage -us -uc

mrproper:
	git clean -xfd

versionprecheck:
	[ -e .git ] || { echo "changing versions only works in git clones!"; exit 1; }
	[ `git status | grep -cP 'working (directory|tree) clean'` -eq 1 ] || { echo "git project is not clean, cannot tag version"; exit 1; }

newversion: versionprecheck
	test -e .git
	make NEWVERSION="$(DAILYVERSION)" version

dailyversion: newversion
	git checkout Changes

resetdaily:
	git reset --hard HEAD
	git checkout .

version:
	if [ -z "$$NEWVERSION" ]; then NEWVERSION=$$(dialog --stdout --inputbox "New Version:" 0 0 "$(VERSION)"); fi; \
	if [ -n "$$NEWVERSION" ] && [ "$$NEWVERSION" != "$(VERSION)" ]; then \
		sed -ri "s/$(VERSION)/$$NEWVERSION/" include/common.h configure.ac support/nagios-mod-gearman.spec; \
		sed -i Changes -e "s/next:/$$NEWVERSION $(shell LC_TIME=C date)/"; \
		$(MAKE) docs; \
		if [ "x$$DEBEMAIL" = "x" ]; then \
			DEBEMAIL="Nagios Development Team <devteam@nagios.com>"; \
		fi; \
		if [ "x$$DEBFULLNAME" = "x" ]; then \
			DEBFULLNAME="Nagios Development Team"; \
		fi; \
		sed -e 's/UNRELEASED/unstable/g' -i debian/changelog; \
		dch --newversion "$$NEWVERSION" --package "nagios-mod-gearman" -D "UNRELEASED" --urgency "low" "new upstream release"; \
		sed -e 's/unstable/UNRELEASED/g' -i debian/changelog; \
	fi;

dailydist: cleandist
	$(MAKE) newversion
	$(MAKE) dist
	$(MAKE) resetdaily
	ls -la *.gz

cleandist:
	rm -f *.gz

releasedist: cleandist dist
	git describe --tag --exact-match
	ls -la *.gz

# order does matter for perl libs/includes (at least on centos 5)
nagios_mod_gearman_worker$(EXEEXT): $(nagios_mod_gearman_worker_OBJECTS) $(nagios_mod_gearman_worker_DEPENDENCIES)
	@rm -f nagios-mod-gearman-worker$(EXEEXT)
	$(AM_V_CCLD)$(LINK) $(AM_CFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $(nagios_mod_gearman_worker_OBJECTS) $(nagios_mod_gearman_worker_LDADD) $(LIBS)

01_utils$(EXEEXT): $(01_utils_OBJECTS) $(01_utils_DEPENDENCIES)
	@rm -f 01_utils$(EXEEXT)
	$(AM_V_CCLD)$(LINK) $(AM_CFLAGS) $(CFLAGS) -o $@ $(01_utils_OBJECTS) $(01_utils_LDADD) $(LIBS)

02_full$(EXEEXT): $(02_full_OBJECTS) $(02_full_DEPENDENCIES)
	@rm -f 02_full$(EXEEXT)
	$(AM_V_CCLD)$(LINK) $(AM_CFLAGS) $(CFLAGS) -o $@ $(02_full_OBJECTS) $(02_full_LDADD) $(LIBS)

03_exec$(EXEEXT): $(03_exec_OBJECTS) $(03_exec_DEPENDENCIES)
	@rm -f 03_exec$(EXEEXT)
	$(AM_V_CCLD)$(LINK) $(AM_CFLAGS) $(CFLAGS) -o $@ $(03_exec_OBJECTS) $(03_exec_LDADD) $(LIBS)

06_exec$(EXEEXT): $(06_exec_OBJECTS) $(06_exec_DEPENDENCIES)
	@rm -f 06_exec$(EXEEXT)
	$(AM_V_CCLD)$(LINK) $(AM_CFLAGS) $(CFLAGS) -o $@ $(06_exec_OBJECTS) $(06_exec_LDADD) $(LIBS)
