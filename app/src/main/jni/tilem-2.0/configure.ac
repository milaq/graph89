AC_PREREQ(2.67)
AC_INIT([TilEm], [2.0], [tilem-devel@lists.sourceforge.net],
  [tilem], [http://tilem.sourceforge.net/])
AC_CONFIG_SRCDIR([emu/tilem.h])

# Checks for programs

AC_PROG_CC
AC_PROG_CPP
AC_ARG_VAR(OPT_CFLAGS,
  [Additional C compiler flags used for optimizing critical areas of
  the code (default: -O3 if using GCC)])
if test "x$GCC" = "xyes" ; then
  CFLAGS="$CFLAGS -W -Wall -Wwrite-strings"
  if test "x$OPT_CFLAGS" = "x" ; then
    OPT_CFLAGS="-O3"
  fi
fi

AC_CHECK_TOOL(AR, [ar], [false])
AC_ARG_VAR(AR, [Static library archiver])
AC_ARG_VAR(AR_FLAGS, [Flags to pass to ar to build a static library])
if test "x$AR_FLAGS" = "x" ; then
  AR_FLAGS=cru
fi

AC_PROG_RANLIB
AC_ARG_VAR(RANLIB, [Program to make a static library linkable])

AC_PROG_INSTALL
AC_PROG_MAKE_SET

AC_CHECK_PROG([UPDATE_DESKTOP_DATABASE],
              [update-desktop-database], [update-desktop-database], [:])
AC_CHECK_PROG([UPDATE_MIME_DATABASE],
              [update-mime-database], [update-mime-database], [:])

# Checks for libraries

m4_define(with_flags, [
  save_cflags="$CFLAGS"
  save_libs="$LIBS"
  CFLAGS="$CFLAGS $$1_CFLAGS"
  LIBS="$LIBS $$1_LIBS"
  $2
  CFLAGS="$save_cflags"
  LIBS="$save_libs"
])

# GLib and GTK+

PKG_CHECK_MODULES(GTK, gtk+-2.0 >= 2.6.0
                       glib-2.0 >= 2.12.0
                       gthread-2.0)

AC_ARG_ENABLE([gtk-deprecated],
  AS_HELP_STRING([--disable-gtk-deprecated], [Disable deprecated GTK+ API]),
  [ enable_gtk_deprecated=$enableval ], [ enable_gtk_deprecated=yes ])
if test "x$enable_gtk_deprecated" = "xno" ; then
  GTK_CFLAGS="$GTK_CFLAGS -DG_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGSEAL_ENABLE"
fi

# If using the native Windows version of GTK+, be sure to use
# -mms-bitfields for all compilation.  Also, use -mwindows for linking
# GUI programs.

# (If not using pkg-config, you're on your own)

if test "x$PKG_CONFIG" != "x" ; then
  gtk_target=`$PKG_CONFIG --variable=target gtk+-2.0`
fi

if test "x$gtk_target" = "xwin32" && test "x$GCC" = "xyes" ; then
  CFLAGS="$CFLAGS -mms-bitfields"
  GUI_LDFLAGS="-mwindows"
  LIBS="-lcomdlg32 -lshell32 -lole32 $LIBS"
  gui_extra_objects="tilem2rc.o"
else
  GUI_LDFLAGS=""
  gui_extra_objects=""
fi

AC_SUBST(GUI_LDFLAGS)
AC_SUBST(gui_extra_objects)

with_flags(GTK,
  [ AC_CHECK_FUNC(gtk_init, [ have_gtk=yes ], [ have_gtk=no ]) ])
if test "x$have_gtk" != "xyes" ; then
  AC_MSG_ERROR([GTK+ 2.x libraries not found or not usable.
You must install a recent version of GTK+ 2.x, including the
-dev/-devel packages if appropriate.])
fi

# Libticalcs2 and related libraries

PKG_CHECK_MODULES(TICALCS, ticalcs2 ticables2 tifiles2 ticonv,
                  [ have_ticalcs=maybe ], [ have_ticalcs=no ])

if test "x$have_ticalcs" = "xmaybe" ; then
  with_flags(TICALCS,
    [ AC_CHECK_FUNC(ticalcs_library_init, [ have_ticalcs=yes ], [ have_ticalcs=no ]) ])
fi

if test "x$have_ticalcs" != "xyes" ; then
  AC_MSG_ERROR([libticalcs2 not found or not usable.

$TICALCS_PKG_ERRORS

You must install libticalcs2, libticables2, libtifiles2, and libticonv
(including the -dev/-devel packages if appropriate.)  These libraries
are available from <http://lpg.ticalc.org/prj_tilp/>.

If you have installed the libraries in a non-standard location (or if
you're cross-compiling), you will need to add the location of
ticalcs2.pc to your PKG_CONFIG_PATH environment variable, or set the
TICALCS_CFLAGS and TICALCS_LIBS environment variables by hand.])
fi

with_flags(TICALCS,
  [ AC_CHECK_FUNCS([ticables_cable_set_raw]) ])

# SDL

AC_ARG_WITH([sdl],
  AS_HELP_STRING([--with-sdl], [Use SDL for audio output]),
  [], [ with_sdl=auto ])

if test "x$with_sdl" != "xno" ; then
  PKG_CHECK_MODULES(SDL, sdl, [ have_sdl=maybe ], [ have_sdl=no ])
  if test "x$have_sdl" = "xmaybe" ; then
    with_flags(SDL,
      [ AC_CHECK_FUNC(SDL_AudioInit, [ have_sdl=yes ], [ have_sdl=no ]) ])
  fi
fi
if test "x$have_sdl" = "xyes" ; then
  AC_DEFINE(HAVE_LIBSDL, 1, [Define to 1 if you have the SDL library.])
  AC_DEFINE(ENABLE_AUDIO, 1, [Define to 1 to enable audio support.])
else
  if test "x$with_sdl" = "xyes" ; then
    AC_MSG_ERROR([SDL not found or not usable.])
  fi
  SDL_CFLAGS=
  SDL_LIBS=
fi

# Gettext

# List of all available languages
ALL_LINGUAS='fr'

GETTEXT_PACKAGE=tilem2
AC_SUBST(GETTEXT_PACKAGE)
AM_GLIB_GNU_GETTEXT
AC_SUBST(LINGUAS)

# Tools used for building the Windows installer

if test "x$gtk_target" = "xwin32" ; then
  AC_CHECK_TOOL([STRIP], [strip], [:])
  AC_CHECK_TOOL([OBJDUMP], [objdump], [objdump])
  AC_CHECK_TOOL([WINDRES], [windres], [windres])
  AC_CHECK_PROG([MAKENSIS], [makensis], [makensis])
  AC_PROG_LN_S

  AC_MSG_CHECKING([where to find GTK+ runtime libraries])
  if test "x$GTK_BINDIR" = "x" ; then
    prefix=`$PKG_CONFIG --variable=exec_prefix gtk+-2.0`
    test "x$prefix" != "x" && GTK_BINDIR="$prefix/bin"
  fi
  AC_MSG_RESULT([$GTK_BINDIR])

  AC_MSG_CHECKING([where to find ticalcs2 runtime libraries])
  if test "x$TICALCS_BINDIR" = "x" ; then
    prefix=`$PKG_CONFIG --variable=exec_prefix ticalcs2`
    test "x$prefix" != "x" && TICALCS_BINDIR="$prefix/bin"
  fi
  AC_MSG_RESULT([$TICALCS_BINDIR])

  if test "x$DLLPATH" = "x" ; then
    DLLPATH='${GTK_BINDIR}'$PATH_SEPARATOR'${TICALCS_BINDIR}'
  fi
  AC_SUBST(GTK_BINDIR)
  AC_SUBST(TICALCS_BINDIR)
  AC_SUBST(DLLPATH)
fi

# Checks for header files

AC_HEADER_STDC

# Checks for system and compiler characteristics

AC_C_BIGENDIAN
AC_C_INLINE
AC_C_RESTRICT
AC_TYPE_UINTPTR_T

# Checks for standard library functions

AC_SEARCH_LIBS([pow], [m])
AC_CHECK_FUNCS([lround])

AC_MSG_CHECKING([for __sync_synchronize])
AC_LINK_IFELSE([AC_LANG_PROGRAM([], [__sync_synchronize();])],
  [AC_MSG_RESULT([yes])
   AC_DEFINE(HAVE___SYNC_SYNCHRONIZE, 1,
     [Define to 1 if you have the the `__sync_synchronize' function.])],
  [AC_MSG_RESULT([no])])

# Output

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile
                 emu/Makefile
                 db/Makefile
                 data/Makefile
                 gui/Makefile
                 gui/tilem2.rc
                 po/Makefile.in
                 installer/win32/Makefile
                 installer/win32/installer.nsi])
AC_OUTPUT
