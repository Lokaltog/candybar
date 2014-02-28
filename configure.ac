AC_INIT([wkline], [0.01], [https://github.com/Lokaltog/wkline])
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])
AC_PROG_CC
AC_CONFIG_HEADERS([config.h])

AC_ARG_WITH([alsa], AS_HELP_STRING([--without-alsa], [compile without alsa support]))
AC_ARG_WITH([curl], AS_HELP_STRING([--without-curl], [compile without libcurl support]))
AC_ARG_WITH([dbus], AS_HELP_STRING([--without-dbus], [compile without dbus-glib support]))
AC_ARG_WITH([mpd], AS_HELP_STRING([--without-mpd], [compile without libmpdclient support]))

AS_IF([test "x$with_alsa" != "xno"],
      [PKG_CHECK_MODULES([ALSA], [alsa], [have_alsa=yes], [have_alsa=no])],
      [have_alsa=no])
AS_IF([test "x$have_alsa" = "xyes"], [], [
      AC_DEFINE([DISABLE_WIDGET_VOLUME], [], [disable alsa])
      AS_IF([test "x$with_alsa" = "xyes"], [AC_MSG_ERROR([alsa requested but not found])])
      ])

AS_IF([test "x$with_curl" != "xno"],
      [PKG_CHECK_MODULES([CURL], [libcurl], [have_libcurl=yes], [have_libcurl=no])],
      [have_libcurl=no])
AS_IF([test "x$have_libcurl" = "xyes"], [], [
      AC_DEFINE([DISABLE_WIDGET_WEATHER], [], [disable libcurl])
      AS_IF([test "x$with_libcurl" = "xyes"], [AC_MSG_ERROR([libcurl requested but not found])])
      ])

AS_IF([test "x$with_dbus" != "xno"],
      [PKG_CHECK_MODULES([DBUS], [dbus-glib-1], [have_dbus=yes], [have_dbus=no])],
      [have_dbus=no])
AS_IF([test "x$have_dbus" = "xyes"], [], [
      AC_DEFINE([DISABLE_WIDGET_NOTIFICATIONS], [], [disable dbus])
      AS_IF([test "x$with_dbus" = "xyes"], [AC_MSG_ERROR([dbus requested but not found])])
      ])

AS_IF([test "x$with_mpd" != "xno"],
      [PKG_CHECK_MODULES([MPD], [libmpdclient], [have_libmpdclient=yes], [have_libmpdclient=no])],
      [have_libmpdclient=no])
AS_IF([test "x$have_libmpdclient" = "xyes"], [], [
      AC_DEFINE([DISABLE_WIDGET_NOW_PLAYING_MPD], [], [disable libmpdclient])
      AS_IF([test "x$with_mpd" = "xyes"], [AC_MSG_ERROR([libmpdclient requested but not found])])
      ])

AM_CONDITIONAL([HAVE_ALSA], [test "x$have_alsa" = "xyes"])
AM_CONDITIONAL([HAVE_LIBCURL], [test "x$have_libcurl" = "xyes"])
AM_CONDITIONAL([HAVE_DBUS], [test "x$have_dbus" = "xyes"])
AM_CONDITIONAL([HAVE_LIBMPDCLIENT], [test "x$have_libmpdclient" = "xyes"])

# Remove if you wish to see full compiler command lines
AM_SILENT_RULES([yes])

# Request dependencies via pkg-config
# Access the variables via GTK3_CFLAGS and GTK3_LIBS
PKG_CHECK_MODULES([GTK3], [gtk+-3.0])
PKG_CHECK_MODULES([WEBKIT], [webkitgtk-3.0])
PKG_CHECK_MODULES([XCB], [xcb-util xcb-ewmh xcb-icccm])
PKG_CHECK_MODULES([JSON_GLIB], [jansson])

AC_CONFIG_FILES([
 Makefile
])
AC_OUTPUT
