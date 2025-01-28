#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

##############################################
# autoconf really does not work with 2.59 or older
AC_PREREQ([2.60])
AC_INIT([nagios-mod-gearman],[1.0.0],[devteam@nagios.com])
AM_INIT_AUTOMAKE([-Wall foreign subdir-objects])
AC_CONFIG_SRCDIR([neb_module/mod_gearman.c],[worker/worker.c],[tools/send_gearman.c],[tools/check_gearman.c],[tools/gearman_top.c])
AC_CONFIG_HEADERS([config.h])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

CFLAGS="${CFLAGS} -Wall -W -Wstrict-prototypes -Wmissing-prototypes -Wnested-externs -O -fPIC -D_GNU_SOURCE -D_REENTRANT -D_FILE_OFFSET_BITS=64"

##############################################
# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_RANLIB
AM_PROG_AR

##############################################
# gearman wants this
AC_C_INLINE
AC_FUNC_MALLOC
AC_FUNC_FORK
AC_HEADER_STDBOOL

##############################################
# Checks for libraries.
AC_CHECK_LIB([pthread], [pthread_create])

##############################################
# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h unistd.h pthread.h arpa/inet.h fcntl.h limits.h netdb.h netinet/in.h stddef.h sys/socket.h sys/time.h sys/timeb.h syslog.h],,AC_MSG_ERROR([Compiling Nagios-Mod-Gearman requires standard unix headers files]))
AC_CHECK_HEADERS([ltdl.h],,AC_MSG_ERROR([Compiling Nagios-Mod-Gearman requires ltdl.h]))
AC_CHECK_HEADERS([curses.h],,AC_MSG_ERROR([Compiling Nagios-Mod-Gearman requires curses.h]))

##############################################
# check for gearmand
AC_ARG_WITH(gearman,
 [  --with-gearman=DIR Specify the path to your gearman library],
 [
    found=0
    for d in libgearman/.libs lib lib64 .; do
      test -e "$withval/$d/libgearman.so" && LDFLAGS="${LDFLAGS} -L$withval/$d" && found=1 && break;
    done
    test $found -eq 1 || { echo 'did not find libgearman.so under your specified path!' && exit 1; }
    echo "found libgearman.so in $withval/$d/"
    found=0
    for d in libgearman-1.0 include .; do
      test -e "$withval/$d/libgearman-1.0/gearman.h" && CFLAGS="${CFLAGS} -I$withval/$d" && found=1 && break;
    done
    test $found -eq 1 || { echo 'did not find libgearman-1.0/gearman.h under your specified path!' && exit 1; }
    echo "found gearman.h in $withval/$d/"
    GEARMAN_LIBS="-lgearman -lglib-2.0"
 ],
 [
    PKG_CHECK_MODULES([gearmand], [gearmand >= 1.1.12])
    CFLAGS="${CFLAGS} `pkg-config --cflags gearmand`"
    GEARMAN_LIBS="`pkg-config --libs gearmand`"
 ]
)
AC_CHECK_LIB([gearman], [gearman_client_create],,AC_MSG_ERROR([Compiling Nagios-Mod-Gearman requires libgearman. You may specify the path by --with-gearman=path...]))
AC_SUBST(GEARMAN_LIBS)

##############################################
# check for openssl
PKG_CHECK_MODULES([openssl], [openssl >= 1])
CFLAGS="${CFLAGS} `pkg-config --cflags openssl`"
LDFLAGS="${LDFLAGS} `pkg-config --libs openssl`"
AC_CHECK_HEADERS([openssl/evp.h],,AC_MSG_ERROR([Compiling Nagios-Mod-Gearman requires openssl.h (hint: install libssl-dev pkg)]))

##############################################
PKG_PROG_PKG_CONFIG()
AC_ARG_WITH([systemd-dir],
     [AS_HELP_STRING([--with-systemd-dir=DIR], [Directory for systemd service files])],,
     [with_systemd_dir=auto])
AS_IF([test "x$with_systemd_dir" = "xyes" -o "x$with_systemd_dir" = "xauto"], [
     def_systemd_dir=$($PKG_CONFIG --variable=systemdsystemunitdir systemd)

     AS_IF([test "x$def_systemd_dir" = "x"],
   [AS_IF([test "x$with_systemd_dir" = "xyes"],
    [AC_MSG_ERROR([systemd support requested but pkg-config unable to query systemd package])])
    with_systemd_dir=no],
   [with_systemd_dir="$def_systemd_dir"])])
AS_IF([test "x$with_systemd_dir" != "xno"],
      [AC_SUBST([systemddir], [$with_systemd_dir])])

##############################################
# Determine the user
AC_ARG_WITH([user],
[AS_HELP_STRING([--with-user],
[specify the user @<:@default="nagios"@:>@])],
[], [with_user="nagios"])
user="$with_user"
AC_SUBST(user)

##############################################
AC_DEFINE_UNQUOTED([GIT_HASH], "`git describe --always 2>/dev/null`", ["git version information"])

##############################################
# Check some functions
AC_CHECK_FUNCS([gettimeofday strsep strtok strdup strchr strstr strtoul alarm gethostname memset strcspn strerror atexit gethostbyname socket dup2 localtime_r memmove strpbrk])
AC_PROG_LN_S

##############################################
# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_TYPE_PID_T

##############################################
# add a "make install" target
AC_PROG_INSTALL

##############################################
# enable -Werror after locating all libs, otherwise we could not find libs that generate warnings
AC_ARG_ENABLE(debug,--enable-debug will enable debugging symbols,[
    CFLAGS="${CFLAGS} -pedantic -Werror -Wfatal-errors -Wshadow -Wpointer-arith -Wcast-qual -Wcast-align -gdwarf-4 -g3"
    ],
    CFLAGS="${CFLAGS} -g3"
)

##############################################
# write out files
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
