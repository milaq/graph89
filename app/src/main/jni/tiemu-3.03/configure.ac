dnl Process this file with autoconf to produce a configure script.
dnl $Id: configure.in 409 2004-06-07 21:10:20Z roms $

dnl Init.
AC_PREREQ(2.57)
AC_INIT(TiEmu, 3.03, [tiemu-users@lists.sourceforge.net])

AM_INIT_AUTOMAKE([dist-bzip2])
AM_MAINTAINER_MODE

AC_PREFIX_DEFAULT(/usr/local)

dnl Files to configure.
AC_CONFIG_SRCDIR([src/gui/main.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
  Makefile
  build/Makefile
  glade/Makefile
  help/Makefile
  man/Makefile
  misc/Makefile
  pedrom/Makefile
  pixmaps/Makefile
  po/Makefile.in
  skins/Makefile
  src/Makefile
  tiemu.lsm
  tiemu.spec
])

AC_MSG_CHECKING([for something to drink while compiling])
AC_MSG_RESULT([err: no fridge found!])

# Setup libtool.
AC_DISABLE_STATIC
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_LN_S
AC_ISC_POSIX
AC_CHECK_PROG(GROFF, groff, yes, no)
AM_CONDITIONAL(USE_GROFF, test "$GROFF" != "no")
AC_CHECK_TOOL(RC, windres, windres)

# Checks for libraries.
AM_GNU_GETTEXT(external)
AC_SUBST(INCINTL)

PKG_CHECK_MODULES(TICABLES, ticables2 >= 1.3.0)
AC_SUBST(TICABLES_CFLAGS)
AC_SUBST(TICABLES_LIBS)

PKG_CHECK_MODULES(TICALCS, ticalcs2 >= 1.0.7)
AC_SUBST(TICALCS_CFLAGS)
AC_SUBST(TICALCS_LIBS)

PKG_CHECK_MODULES(TIFILES, tifiles2 >= 1.0.7)
AC_SUBST(TIFILES_CFLAGS)
AC_SUBST(TIFILES_LIBS)

PKG_CHECK_MODULES(TICONV, ticonv >= 1.0.4)
AC_SUBST(TICONV_CFLAGS)
AC_SUBST(TICONV_LIBS)

PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.6.0)
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

PKG_CHECK_MODULES(GTK, gtk+-2.0 >= 2.6.0)
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

PKG_CHECK_MODULES(GLADE, libglade-2.0 >= 2.4.0)
AC_SUBST(GLADE_CFLAGS)
AC_SUBST(GLADE_LIBS)

# D-Bus support
AC_ARG_WITH(dbus, AC_HELP_STRING([--with-dbus], [Compile with D-Bus support]), [dbus=$withval], [dbus=no])
if test "x$dbus" = "xyes"; then
        PKG_CHECK_MODULES(DBUS_GLIB, dbus-glib-1 >= 0.60)
        AC_DEFINE(WITH_DBUS, 1, [Use D-Bus support])
else
        DBUS_GLIB_CFLAGS=
        DBUS_GLIB_LIBS=
fi
AC_SUBST(DBUS_GLIB_CFLAGS)
AC_SUBST(DBUS_GLIB_LIBS)
AC_SUBST(dbus)

# Checks for header files.
AC_HEADER_STDBOOL
AC_CHECK_HEADERS([fcntl.h])
AC_CHECK_HEADERS([libintl.h])
AC_CHECK_HEADERS([sys/timeb.h])
AC_CHECK_HEADERS(stdint.h)
AC_CHECK_HEADERS([tilp2/ticables.h tilp2/ticalcs.h tilp2/tifiles.h])
dnl AC_CHECK_HEADERS([Carbon/Carbon.h])

dnl # Add Carbon framework if detected
dnl if test x"$ac_cv_header_Carbon_Carbon_h" = xyes ; then
dnl   LDFLAGS="$LDFLAGS -framework Carbon"
dnl #fi

# Checks for typedefs, structures, and compiler characteristics.
AC_FUNC_MEMCMP
#AC_C_RESTRICT
AC_C_VOLATILE

# Checks for libraries.
AC_CHECK_LIB(m, pow)
AC_CHECK_LIB(pthread, pthread_create) dnl needed by GThread
#AC_CHECK_LIB(comdlg32, GetOpenFileName)

# Check for zlib
AC_CHECK_HEADER(zlib.h)
AC_CHECK_LIB(z, compress)
LIBZ="$LIBS"
LIBS=""
AC_SUBST(LIBZ)

# Checks for library functions.
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_STAT 
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([bzero memset mkdir select strchr strrchr strstr])

# Platform specific tests.
CFLAGS_FOR_BUILD="$CFLAGS"
export CFLAGS_FOR_BUILD
dnl AC_CANONICAL_HOST
case $host in
  *-*-mingw*)            ARCH="-D__WIN32__ -D__MINGW32__ -mwindows -D_WINDOWS" ;;
  *-*-*bsd*)             ARCH="-D__BSD__" ;;
  i[[3456]]86-*-linux-*) ARCH="-D__I386__ -D__LINUX__" ;; 
  i[[3456]]86-apple-darwin*) ARCH="-D__I386__ -D__MACOSX__" ;;
  arm*-*-linux-*)        ARCH="-D__IPAQ__ -D__LINUX__" ;;
  powerpc-*-linux-*)     ARCH="-D__PPC__  -D__LINUX__" ;;
  powerpc-apple-darwin*) ARCH="-D__PPC__  -D__MACOSX__" ;;
  *-apple-darwin*)       ARCH="-D__MACOSX__" ;;
  *)                     ARCH="-D__LINUX__" ;;
esac
CFLAGS="$CFLAGS $ARCH"

# Check for SDL if sound support is wanted
AC_ARG_ENABLE(sound,
  AC_HELP_STRING([--disable-sound], [disable sound emulation]),
  [tiemu_use_sound=$enableval], [tiemu_use_sound=yes])
AC_CACHE_CHECK([whether to enable sound emulation],
  [tiemu_use_sound], [tiemu_use_sound=yes])
if test "x$tiemu_use_sound" = "xyes"; then
        SDL_VERSION=1.2.0
        AM_PATH_SDL($SDL_VERSION,
                    :,
                    AC_MSG_ERROR([*** SDL version $SDL_VERSION not found!])
        )
else
        CFLAGS="$CFLAGS -DNO_SOUND"
        SDL_CFLAGS=
        SDL_LIBS=
fi
AC_SUBST(SDL_CFLAGS)
AC_SUBST(SDL_LIBS)

# KDE dialogs support
AC_ARG_WITH(kde, AC_HELP_STRING([--without-kde], [Compile without KDE support]), [kde=$withval], [kde=default])
if test "x$kde" = "xdefault"; then
  case $host_os in
    *mingw*)
      kde=no
      ;;
    *)
      kde=yes
      ;;
  esac
fi
if test "x$kde" = "xyes"; then
        AC_PROG_CXX
        AC_PATH_KDE
        AC_DEFINE(WITH_KDE, 1, [Use KDE support])
fi
AC_SUBST(kde)

# Ensure MSVC-compatible struct packing convention is used when
# compiling for Win32 with gcc.
# What flag to depends on gcc version: gcc3 uses "-mms-bitfields", while
# gcc2 uses "-fnative-struct".
case $host_os in
*mingw*|*cygwin*)
  if test x$GCC = xyes; then
    msnative_struct=''
    AC_MSG_CHECKING([how to get MSVC-compatible struct packing])
    if test -z "$ac_cv_prog_CC"; then
      our_gcc="$CC"
    else
      our_gcc="$ac_cv_prog_CC"
    fi
    case `$our_gcc --version | sed -e 's,\..*,.,' -e q` in
      2.)
        if $our_gcc -v --help 2>/dev/null | grep fnative-struct >/dev/null; then
          msnative_struct='-fnative-struct'
        fi
        ;;
      *)
        if $our_gcc -v --help 2>/dev/null | grep ms-bitfields >/dev/null; then
          msnative_struct='-mms-bitfields'
        fi
        ;;
    esac
    if test x"$msnative_struct" = x ; then
      AC_MSG_RESULT([no way])
      AC_MSG_WARN([produced libraries might be incompatible with MSVC-compiled code])
    else
      CFLAGS="$CFLAGS $msnative_struct"
      AC_MSG_RESULT([${msnative_struct}])
    fi
  fi
  ;;
esac

# Check for the new -fvisibility=hidden flag introduced in gcc 4.0
# Allow to reduce shared library size and avoid symbol clash
# Unfortunately, this only works for fully static builds. If any parts of GDB are built shared (--enable-shared or --enable-shared-*), we currently have no way to disable it just for these parts. -- Kevin
case $host_os in
*mingw*)
  ;;
*)
    case "$ac_configure_args" in
    *--enable-shared*)
      ;;
    *)
        if test x$GCC = xyes; then
           visibility_flag=''
           AC_MSG_CHECKING([whether gcc accepts -fvisibility])
           if test -z "$ac_cv_prog_CC"; then
              our_gcc="$CC"
           else
              our_gcc="$ac_cv_prog_CC"
           fi
           if $our_gcc -v --help 2>/dev/null | grep "fvisibility" >/dev/null; then
              visibility_flag='-fvisibility=hidden'
           fi
           if test x"$visibility_flag" = x ; then
              AC_MSG_RESULT([no])
           else
              CFLAGS="$CFLAGS $visibility_flag"
              AC_MSG_RESULT([${visibility_flag}])
              AC_DEFINE(HAVE_FVISIBILITY, 1, [Use -fvisibility=hidden flag])
            fi
        fi
        ;;
    esac
  ;;
esac

case "$host" in
  *-*-linux*) EXFL="-Wl,--export-dynamic" ;;
  *)          EXFL="" ;;
esac
LDFLAGS="$LDFLAGS $EXFL"

# On Darwin, libglade chokes on stripped executables.
case "$host" in
  *-apple-darwin*)
    case "$CFLAGS $CXXFLAGS $LDFLAGS" in
      *" -s "*|"-s "*|*" -s"|"-s")
         echo "warning: cannot strip libglade executables on this host"
         CFLAGS=`echo "$CFLAGS"|sed 's/\( -s\)* / /g;s/^-s //g;s/ -s$//g;s/^-s$//g'`
         CXXFLAGS=`echo "$CXXFLAGS"|sed 's/\( -s\)* / /g;s/^-s //g;s/ -s$//g;s/^-s$//g'`
         LDFLAGS=`echo "$LDFLAGS"|sed 's/\( -s\)* / /g;s/^-s //g;s/ -s$//g;s/^-s$//g'`
         ;;
    esac
    ;;
esac

# some extra flags
CFLAGS="$CFLAGS -DGTK_DISABLE_DEPRECATED -DDEBUGGER"
CXXFLAGS="$CFLAGS -fno-rtti -fno-exceptions"

AC_SUBST(CFLAGS)
AC_SUBST(LDFLAGS)
AC_SUBST(CXXFLAGS)

# Debugger support (can be disabled for embedded architectures)
AC_ARG_ENABLE(debugger,
  AC_HELP_STRING([--disable-debugger], [disable the built-in debugger]),
  [tiemu_use_debugger=$enableval], [tiemu_use_debugger=yes])
AC_CACHE_CHECK([whether to use the debugger],
  [tiemu_use_debugger], [tiemu_use_debugger=yes])
if test x$tiemu_use_debugger = xno; then
  AC_DEFINE(NO_DEBUGGER, 1, [Defined to disable the debugger])
fi

# GDB support (can be disabled)
AC_ARG_ENABLE(gdb,
  AC_HELP_STRING([--disable-gdb], [disable the GDB-based source-level debugger]),
  [tiemu_use_gdb=$enableval], [tiemu_use_gdb=$tiemu_use_debugger])
AC_CACHE_CHECK([whether to use the GDB-based source-level debugger],
  [tiemu_use_gdb], [tiemu_use_gdb=$tiemu_use_debugger])
if test x$tiemu_use_gdb = xno; then
  CFLAGS="$CFLAGS -DNO_GDB"
fi
AM_CONDITIONAL(NO_GDB, test x$tiemu_use_gdb = xno)

# It makes no sense to enable GDB without the debugger!
if test x$tiemu_use_debugger = xno -a x$tiemu_use_gdb = xyes; then
  AC_MSG_ERROR([cannot enable GDB without the debugger])
fi

# Run GDB configury (if enabled)
if test x$tiemu_use_gdb = xyes; then
  ac_configure_args="$ac_configure_args --target=m68k-coff --disable-tui --disable-werror"
  changequote(<<,>>)
  ac_configure_args=`echo "$ac_configure_args" | sed "s,'CC=[^']*',,g;s,'CFLAGS=[^']*',,g;s,'CXX=[^']*',,g;s,'CXXFLAGS=[^']*',,g;s,'LDFLAGS=[^']*',,g;s,'LIBS=[^']*',,g;s,'build_alias=[^']*',,g;s,'host_alias=[^']*',,g;s,'target_alias=[^']*',,g"`
  changequote([,])
  AC_CONFIG_SUBDIRS(src/gdb)
fi

# Output.
AC_OUTPUT

echo
echo "To run the TiEmu emulator, you will need an image (ROM dump or FLASH upgrade)."
echo "As those files are copyrighted by Texas Instruments, they will never be"
echo "distributed with the emulator... In replacement, you can use PedRom which"
echo "is provided with TiEmu."
echo
echo "It you need a ROM dump, you can get one from your calculator"
echo "with the 'ROM dump' function of TiLP."
echo "Otherwise, you can use a TI's FLASH OS upgrade (AMS) as image."
echo "Note: TiLP is available at [http://lpg.ticalc.org/prj_tilp]."
echo
echo "Now, you can type 'make' and 'make install'"
echo ;

