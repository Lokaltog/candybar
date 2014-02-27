bin_PROGRAMS = wkline
wkline_SOURCES = \
	src/widgets/volume.c  \
	src/widgets/desktops.c  \
	src/widgets/now_playing_mpd.c  \
	src/widgets/weather.c \
	util/curl.c \
	util/log.c \
	src/wkline.c

wkline_LDADD = \
	$(GTK3_LIBS) \
	$(WEBKIT_LIBS) \
	$(XCB_LIBS) \
	$(CURL_LIBS) \
	$(JSON_GLIB_LIBS) \
	$(ALSA_LIBS) \
	$(MPD_LIBS)

wkline_CFLAGS = \
	$(GTK3_CFLAGS) \
	$(WEBKIT_CFLAGS) \
	$(XCB_CFLAGS) \
	$(JSON_GLIB_CFLAGS) \
	$(CURL_CFLAGS) \
	$(AM_CFLAGS) \
	$(ALSA_CFLAGS) \
	$(MPD_CFLAGS)

dist_doc_DATA = README.rst
