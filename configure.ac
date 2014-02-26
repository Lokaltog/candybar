AC_INIT([wkline], [0.01], [https://github.com/Lokaltog/wkline])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_PROG_CC
AC_CONFIG_HEADERS([config.h])

# Remove if you wish to see full compiler command lines
AM_SILENT_RULES([yes])

# Request dependencies via pkg-config
# Access the variables via GTK3_CFLAGS and GTK3_LIBS
PKG_CHECK_MODULES([GTK3], [gtk+-3.0])
PKG_CHECK_MODULES([WEBKIT], [webkitgtk-3.0])
PKG_CHECK_MODULES([XCB], [xcb-util xcb-ewmh xcb-icccm])
PKG_CHECK_MODULES([JSON_GLIB], [jansson])
PKG_CHECK_MODULES([CURL], [libcurl])
PKG_CHECK_MODULES([ALSA], [alsa])
PKG_CHECK_MODULES([MPD], [libmpdclient])

AC_CONFIG_FILES([
 Makefile
 src/Makefile
])
AC_OUTPUT
