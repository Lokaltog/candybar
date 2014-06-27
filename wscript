#!/usr/bin/env python

import os
import subprocess

from waflib import Utils, TaskGen

TaskGen.declare_chain(
	rule='${A2X} --doctype manpage --format manpage ${SRC}',
	ext_in='.asciidoc',
	ext_out='',
)

PACKAGE = 'candybar'
LIBDIR = '${{PREFIX}}/lib/{}'.format(PACKAGE)


def get_version():
	'''Attempt to fetch the current version number from git.'''
	version = 'beta'
	if not os.path.exists('.git'):
		return version + '-unknown'
	version += '-git-' + subprocess.Popen('git rev-list --count HEAD', shell=True, stdout=subprocess.PIPE).stdout.read().decode('utf-8').strip()
	version += '.' + subprocess.Popen('git rev-parse --short HEAD', shell=True, stdout=subprocess.PIPE).stdout.read().decode('utf-8').strip()
	return version


def options(opt):
	opt.load('compiler_c')
	opt.add_option('--confdir', dest='confdir', default='/etc/xdg/{}'.format(PACKAGE), help='directory to store {} global configuration files [default: %default]'.format(PACKAGE))
	opt.add_option('--debug', dest='debug', default=False, action='store_true', help='build debug version')
	opt.add_option('--rootdir', dest='rootdir', default='/', help='root directory override (useful for development) [default: %default]')
	opt.add_option('--theme', dest='theme', default='https://github.com/Lokaltog/{}-theme-default/archive/gh-pages.tar.gz'.format(PACKAGE), help='default theme to install locally [default: %default]')
	opt.add_option('--themedir', dest='themedir', default='${{PREFIX}}/share/{}/theme-default'.format(PACKAGE), help='destination directory for default theme [default: %default]')


def configure(ctx):
	ctx.load('compiler_c')
	ctx.check_cfg(atleast_pkgconfig_version='0.0.0')

	# compiler options
	if ctx.options.debug:
		ctx.env.append_unique('CFLAGS', ['-O0', '-g3', '-ggdb', '-Wall', '-Wpedantic', '-Wextra', '-Wno-unused-parameter', '-std=c99'])
		ctx.env.append_unique('DEFINES', 'DEBUG')
	else:
		ctx.env.append_unique('CFLAGS', ['-O3', '-g', '-Werror', '-Wall', '-Wpedantic', '-Wextra', '-Wno-unused-parameter', '-std=c99'])
		ctx.env.append_unique('DEFINES', 'RELEASE')
	ctx.env.append_unique('INCLUDES', ['./src'])
	ctx.env.append_unique('DEFINES', '_POSIX_C_SOURCE=200809L')

	# defines
	ctx.define('PACKAGE', PACKAGE)
	ctx.define('CONFDIR', ctx.options.confdir)
	ctx.define('LIBDIR', Utils.subst_vars(os.path.join(ctx.options.rootdir, LIBDIR), ctx.env))

	# various build deps
	ctx.find_program('a2x', var='A2X', mandatory=False)
	ctx.find_program('wget', var='WGET', mandatory=False)
	ctx.find_program('tar', var='TAR', mandatory=False)

	# deps
	ctx.check_cfg(package='gtk+-3.0', uselib_store='GTK', args=['--cflags', '--libs'])
	ctx.check_cfg(package='glib-2.0 gmodule-2.0', uselib_store='GLIB', args=['--cflags', '--libs'])
	ctx.check_cfg(package='webkitgtk-3.0', uselib_store='WEBKITGTK', args=['--cflags', '--libs'])
	ctx.check_cfg(package='jansson', uselib_store='JANSSON', args=['--cflags', '--libs'])

	# optdeps
	ctx.check_cfg(package='alsa', uselib_store='ALSA', args=['--cflags', '--libs'], mandatory=False)
	ctx.check_cfg(package='libcurl', uselib_store='CURL', args=['--cflags', '--libs'], mandatory=False)
	ctx.check_cfg(package='dbus-1 dbus-glib-1', uselib_store='DBUS', args=['--cflags', '--libs'], mandatory=False)
	ctx.check_cfg(package='GraphicsMagickWand', uselib_store='MAGICK', args=['--cflags', '--libs'], mandatory=False)
	ctx.check_cfg(package='libmpdclient', uselib_store='LIBMPDCLIENT', args=['--cflags', '--libs'], mandatory=False)
	ctx.check_cfg(package='playerctl-1.0', uselib_store='PLAYERCTL', args=['--cflags', '--libs'], mandatory=False)
	ctx.check_cfg(package='xcb-util xcb-ewmh xcb-icccm', uselib_store='XCB', args=['--cflags', '--libs'], mandatory=False)
	ctx.check_cfg(package='i3ipc-glib-1.0', uselib_store='I3IPC', args=['--cflags', '--libs'], mandatory=False)


def build(bld):
	basedeps = ['GTK', 'GLIB', 'WEBKITGTK', 'JANSSON']

	# add build version/time defines
	package_defines = [
		'VERSION="{0}"'.format(get_version()),
	]

	bld.objects(source=bld.path.ant_glob('src/util/(log|config|copy_prop|gdk_helpers|process).c'), target='baseutils', use=basedeps)
	bld.objects(source='src/util/process.c', target='util_process', use=basedeps, cflags=['-fPIC'])

	# widgets
	bld.shlib(source='src/widgets/datetime.c', target='widget_datetime', use=basedeps, install_path=LIBDIR)

	if bld.is_defined('HAVE_ALSA'):
		bld.shlib(source='src/widgets/volume.c', target='widget_volume', use=basedeps + ['ALSA'], install_path=LIBDIR)

	if bld.is_defined('HAVE_CURL'):
		bld.objects(source='src/util/curl.c', target='util_curl', use=basedeps + ['CURL'], cflags=['-fPIC'])

		bld.shlib(source='src/widgets/email_imap.c', target='widget_email_imap', use=basedeps + ['CURL', 'util_curl', 'util_process'], install_path=LIBDIR)
		bld.shlib(source='src/widgets/external_ip.c', target='widget_external_ip', use=basedeps + ['CURL', 'util_curl'], install_path=LIBDIR)
		bld.shlib(source='src/widgets/weather.c', target='widget_weather', use=basedeps + ['CURL', 'util_curl'], install_path=LIBDIR)

	if bld.is_defined('HAVE_DBUS'):
		bld.objects(source='src/util/dbus_helpers.c', target='util_dbus_helpers', use=basedeps + ['DBUS'], cflags=['-fPIC'])

		bld.shlib(source='src/widgets/battery.c', target='widget_battery', use=basedeps + ['DBUS', 'util_dbus_helpers'], install_path=LIBDIR)
		bld.shlib(source='src/widgets/notifications.c', target='widget_notifications', use=basedeps + ['DBUS', 'util_dbus_helpers'], install_path=LIBDIR)

	if bld.is_defined('HAVE_MAGICK'):
		bld.shlib(source='src/widgets/magick_background.c', target='widget_magick_background', use=basedeps + ['MAGICK'], install_path=LIBDIR)

	if bld.is_defined('HAVE_LIBMPDCLIENT'):
		bld.shlib(source='src/widgets/now_playing_mpd.c', target='widget_now_playing_mpd', use=basedeps + ['LIBMPDCLIENT'], install_path=LIBDIR)

	if bld.is_defined('HAVE_PLAYERCTL'):
		bld.shlib(source='src/widgets/now_playing_mpris.c', target='widget_now_playing_mpris', use=basedeps + ['PLAYERCTL'], install_path=LIBDIR)

	if bld.is_defined('HAVE_XCB'):
		bld.shlib(source='src/widgets/desktops.c', target='widget_desktops', use=basedeps + ['XCB'], install_path=LIBDIR)
		bld.shlib(source='src/widgets/window_title.c', target='widget_window_title', use=basedeps + ['util_copy_prop', 'XCB'], install_path=LIBDIR)

	if bld.is_defined('HAVE_I3IPC'):
		bld.shlib(source='src/widgets/desktops_i3.c', target='widget_desktops_i3', use=basedeps + ['I3IPC'], install_path=LIBDIR)

	bld.objects(source='src/widgets.c', target='widgets', use=['baseutils'] + basedeps)
	bld.program(source='src/{}.c'.format(PACKAGE), target=PACKAGE, use=['baseutils', 'widgets'] + basedeps, defines=package_defines)

	# man pages
	for manpage in [1, 5]:
		if bld.env.A2X:
			bld(source='docs/{}.{}.asciidoc'.format(PACKAGE, manpage))
			bld.install_files('${{PREFIX}}/share/man/man{}'.format(manpage), 'docs/{}.{}'.format(PACKAGE, manpage))
		else:
			bld.install_files('${{PREFIX}}/share/doc/{}'.format(PACKAGE), 'docs/{}.{}.asciidoc'.format(PACKAGE, manpage))

	# default theme
	use_remote_theme = True
	if bld.env.WGET and bld.env.TAR:
		bld(
			name='download_theme',
			rule='${{WGET}} {} -q -O ${{TGT}}'.format(bld.options.theme),
			target='theme.tar.gz',
		)
		bld(
			name='extract_theme',
			after='download_theme',
			rule='mkdir -p ${TGT} && ${TAR} -xz --strip-components 1 -C ${TGT} -f ${SRC}',
			source='theme.tar.gz',
			target='theme',
		)

		theme_dir = bld.path.get_bld().make_node('theme')

		def read_theme_files(task):
			# update theme file signatures
			for f in theme_dir.ant_glob('**'):
				f.sig = Utils.h_file(f.abspath())

		bld(
			after='extract_theme',
			rule=read_theme_files,
			always=True,
		)

		bld.install_files(
			bld.options.themedir,
			theme_dir.find_resource('index.html')
		)

		use_remote_theme = False

	def subst_theme_uri(task, text):
		return text.replace(
			'@THEME_URI@',
			'http://lokaltog.github.io/candybar-theme-default/' if use_remote_theme
			else 'file://' + Utils.subst_vars(os.path.join(bld.options.rootdir, bld.options.themedir, 'index.html'), bld.env)
		)

	bld(
		features='subst',
		subst_fun=subst_theme_uri,
		source='src/config.json',
		target='config.json',
	)

	# install default config
	bld.install_files(bld.options.confdir, ['config.json'])
