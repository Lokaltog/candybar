#!/usr/bin/env python

from waflib import Utils
import os, time

PACKAGE = 'wkline'
LIBDIR = '${PREFIX}/lib/wkline'
VERSION = '0.01'

def get_git_version():
	""" try grab the current version number from git"""
	version = None
	if os.path.exists(".git"):
		version = "-git-"+os.popen("git rev-list --count HEAD").read().strip()
		version += "."+os.popen("git rev-parse --short HEAD").read().strip()
	return version

def pre_build(ctx):
	ctx.define('VERSION', VERSION+get_git_version())
	ctx.define('BUILD_DATE', time.strftime("%c"))
	ctx.write_config_header('src/config.h')

def options(opt):
	opt.load('compiler_c')
	opt.add_option('--confdir', dest='confdir', default='/etc/xdg/wkline', help='directory to store wkline global configuration files [default: %default]')
	opt.add_option('--libdir', dest='libdir', default=LIBDIR, help='shared library search path override (useful for development) [default: %default]')
	opt.add_option('--debug', dest='debug', default=False, action='store_true', help='build debug version')

def configure(ctx):
	ctx.load('compiler_c')
	ctx.check_cfg(atleast_pkgconfig_version='0.0.0')

	# compiler options
	if ctx.options.debug:
		ctx.env.append_unique('CFLAGS', ['-g3', '-O0', '-Wall', '-Werror'])
		ctx.define('DEBUG', 1)
	else:
		ctx.env.append_unique('CFLAGS', ['-O3', '-Wall', '-Werror'])
	ctx.env.append_value('INCLUDES', ['./src'])

	# defines
	ctx.define('PACKAGE', PACKAGE)
	ctx.define('CONFDIR', ctx.options.confdir)
	ctx.define('LIBDIR', Utils.subst_vars(ctx.options.libdir, ctx.env))

	# deps
	ctx.check_cfg(package='gtk+-3.0', uselib_store='GTK', args=['--cflags', '--libs'])
	ctx.check_cfg(package='glib-2.0 gmodule-2.0', uselib_store='GLIB', args=['--cflags', '--libs'])
	ctx.check_cfg(package='webkitgtk-3.0', uselib_store='WEBKITGTK', args=['--cflags', '--libs'])
	ctx.check_cfg(package='jansson', uselib_store='JANSSON', args=['--cflags', '--libs'])

	# optdeps
	ctx.check_cfg(package='alsa', uselib_store='ALSA', args=['--cflags', '--libs'], mandatory=False)
	ctx.check_cfg(package='libcurl', uselib_store='CURL', args=['--cflags', '--libs'], mandatory=False)
	ctx.check_cfg(package='dbus-1 dbus-glib-1', uselib_store='DBUS', args=['--cflags', '--libs'], mandatory=False)
	ctx.check_cfg(package='libmpdclient', uselib_store='LIBMPDCLIENT', args=['--cflags', '--libs'], mandatory=False)
	ctx.check_cfg(package='xcb-util xcb-ewmh xcb-icccm', uselib_store='XCB', args=['--cflags', '--libs'], mandatory=False)

def build(bld):
	basedeps = ['GTK', 'GLIB', 'WEBKITGTK', 'JANSSON']

	bld.add_pre_fun(pre_build)

	bld.objects(source=bld.path.ant_glob('src/util/(log|wkconfig|copy_prop).c'), target='baseutils', use=basedeps)
	bld.objects(source='src/widgets.c', target='widgets', use=basedeps)

	# widgets
	if bld.is_defined('HAVE_ALSA'):
		bld.shlib(source='src/widgets/volume.c', target='widget_volume', use=basedeps + ['ALSA'], install_path=LIBDIR)

	if bld.is_defined('HAVE_CURL'):
		bld.objects(source='src/util/curl.c', target='util_curl', use=basedeps + ['CURL'], cflags=['-fPIC'])

		bld.shlib(source='src/widgets/external_ip.c', target='widget_external_ip', use=basedeps + ['CURL', 'util_curl'], install_path=LIBDIR)
		bld.shlib(source='src/widgets/weather.c', target='widget_weather', use=basedeps + ['CURL', 'util_curl'], install_path=LIBDIR)

	if bld.is_defined('HAVE_DBUS'):
		bld.objects(source='src/util/dbus_helpers.c', target='util_dbus_helpers', use=basedeps + ['DBUS'], cflags=['-fPIC'])

		bld.shlib(source='src/widgets/battery.c', target='widget_battery', use=basedeps + ['DBUS', 'util_dbus_helpers'], install_path=LIBDIR)
		bld.shlib(source='src/widgets/notifications.c', target='widget_notifications', use=basedeps + ['DBUS', 'util_dbus_helpers'], install_path=LIBDIR)

	if bld.is_defined('HAVE_LIBMPDCLIENT'):
		bld.shlib(source='src/widgets/now_playing_mpd.c', target='widget_now_playing_mpd', use=basedeps + ['LIBMPDCLIENT'], install_path=LIBDIR)

	if bld.is_defined('HAVE_XCB'):
		bld.shlib(source='src/widgets/desktops.c', target='widget_desktops', use=basedeps + ['XCB'], install_path=LIBDIR)
		bld.shlib(source='src/widgets/window_title.c', target='widget_window_title', use=basedeps + ['util_copy_prop', 'XCB'], install_path=LIBDIR)

	bld.program(source='src/wkline.c', target=PACKAGE, use=['baseutils', 'widgets'] + basedeps)
	bld.install_files(bld.options.confdir, ['config.json'])
