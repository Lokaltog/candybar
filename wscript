#!/usr/bin/env python

PACKAGE = 'wkline'

def options(opt):
	opt.load('compiler_c')

def configure(ctx):
	ctx.load('compiler_c')
	ctx.check_cfg(atleast_pkgconfig_version='0.0.0')

	# compiler options
	ctx.env.append_unique('CFLAGS', ['-g', '-O3', '-Wall', '-Werror'])
	ctx.env.append_value('INCLUDES', ['./src'])

	# defines
	ctx.define('PACKAGE', PACKAGE)

	# deps
	ctx.check_cfg(package='gtk+-3.0', uselib_store='GTK', args=['--cflags', '--libs'])
	ctx.check_cfg(package='webkitgtk-3.0', uselib_store='WEBKITGTK', args=['--cflags', '--libs'])
	ctx.check_cfg(package='jansson', uselib_store='JANSSON', args=['--cflags', '--libs'])

	# optdeps
	ctx.check_cfg(package='alsa', uselib_store='ALSA', args=['--cflags', '--libs'], mandatory=False)
	ctx.check_cfg(package='libcurl', uselib_store='CURL', args=['--cflags', '--libs'], mandatory=False)
	ctx.check_cfg(package='dbus-1 dbus-glib-1', uselib_store='DBUS', args=['--cflags', '--libs'], mandatory=False)
	ctx.check_cfg(package='libmpdclient', uselib_store='LIBMPDCLIENT', args=['--cflags', '--libs'], mandatory=False)
	ctx.check_cfg(package='xcb-util xcb-ewmh xcb-icccm', uselib_store='XCB', args=['--cflags', '--libs'], mandatory=False)

def build(bld):
	basedeps = ['GTK', 'WEBKITGTK', 'JANSSON']
	widgets_enabled = []

	bld.stlib(source=bld.path.ant_glob('src/util/(log|config).c'), target='baseutils', use=basedeps)
	bld.stlib(source='src/widgets.c', target='widgets', use=basedeps)

	# widgets
	if bld.is_defined('HAVE_ALSA'):
		bld.stlib(source='src/widgets/volume.c', target='widget_volume', use=basedeps + ['ALSA'])
		widgets_enabled += ['widget_volume']
	else:
		bld.define('DISABLE_WIDGET_VOLUME', 1)

	if bld.is_defined('HAVE_CURL'):
		bld.stlib(source='src/util/curl.c', target='util_curl', use=basedeps + ['CURL'])
		bld.stlib(source='src/widgets/external_ip.c', target='widget_external_ip', use=basedeps + ['CURL', 'util_curl'])
		bld.stlib(source='src/widgets/weather.c', target='widget_weather', use=basedeps + ['CURL', 'util_curl'])
		widgets_enabled += ['widget_external_ip', 'widget_weather']
	else:
		bld.define('DISABLE_WIDGET_EXTERNAL_IP', 1)
		bld.define('DISABLE_WIDGET_WEATHER', 1)

	if bld.is_defined('HAVE_DBUS'):
		bld.stlib(source='src/util/dbus_helpers.c', target='util_dbus_helpers', use=basedeps + ['DBUS'])
		bld.stlib(source='src/widgets/battery.c', target='widget_battery', use=basedeps + ['DBUS', 'util_dbus_helpers'])
		bld.stlib(source='src/widgets/notifications.c', target='widget_notifications', use=basedeps + ['DBUS', 'util_dbus_helpers'])
		widgets_enabled += ['widget_battery', 'widget_notifications']
	else:
		bld.define('DISABLE_WIDGET_BATTERY', 1)
		bld.define('DISABLE_WIDGET_NOTIFICATIONS', 1)

	if bld.is_defined('HAVE_LIBMPDCLIENT'):
		bld.stlib(source='src/widgets/now_playing_mpd.c', target='widget_now_playing_mpd', use=basedeps + ['LIBMPDCLIENT'])
		widgets_enabled += ['widget_now_playing_mpd']
	else:
		bld.define('DISABLE_WIDGET_NOW_PLAYING_MPD', 1)

	if bld.is_defined('HAVE_XCB'):
		bld.stlib(source='src/util/copy_prop.c', target='util_copy_prop', use=basedeps)
		bld.stlib(source='src/widgets/desktops.c', target='widget_desktops', use=basedeps + ['XCB'])
		bld.stlib(source='src/widgets/window_title.c', target='widget_window_title', use=basedeps + ['util_copy_prop', 'XCB'])
		widgets_enabled += ['widget_desktops', 'widget_window_title']
	else:
		bld.define('DISABLE_WIDGET_DESKTOPS', 1)
		bld.define('DISABLE_WIDGET_WINDOW_TITLE', 1)

	bld(features='c cprogram', source='src/wkline.c', target=PACKAGE, use=['baseutils', 'widgets'] + basedeps + widgets_enabled)
