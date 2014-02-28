bin_PROGRAMS = wkline
wkline_SOURCES = \
	util/log.c \
	src/wkline.c \
	src/widgets/desktops.c

wkline_LDADD = \
	$(GTK3_LIBS) \
	$(WEBKIT_LIBS) \
	$(XCB_LIBS) \
	$(JSON_GLIB_LIBS)

wkline_CFLAGS = \
	$(GTK3_CFLAGS) \
	$(WEBKIT_CFLAGS) \
	$(XCB_CFLAGS) \
	$(JSON_GLIB_CFLAGS) \
	$(AM_CFLAGS)

if HAVE_LIBMPDCLIENT
wkline_SOURCES += src/widgets/now_playing_mpd.c
wkline_LDADD += $(MPD_LIBS)
wkline_CFLAGS += $(MPD_CFLAGS)
endif
if HAVE_LIBCURL
wkline_SOURCES += util/curl.c \
	src/widgets/weather.c \
	src/widgets/external_ip.c
wkline_LDADD += $(CURL_LIBS)
wkline_CFLAGS += $(CURL_CFLAGS)
endif
if HAVE_DBUS
wkline_SOURCES += src/widgets/notifications.c
wkline_LDADD += $(DBUS_LIBS)
wkline_CFLAGS += $(DBUS_CFLAGS)
endif
if HAVE_ALSA
wkline_SOURCES += src/widgets/volume.c
wkline_LDADD += $(ALSA_LIBS)
wkline_CFLAGS += $(ALSA_CFLAGS)
endif

dist_doc_DATA = README.rst
