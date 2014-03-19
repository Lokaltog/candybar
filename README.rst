wkline
======

:Author: Kim Silkebækken (kim.silkebaekken@gmail.com)
:Source: https://github.com/Lokaltog/wkline

**WebKit-based statusline utility for tiling window managers.**

wkline displays a HTML-based statusline in a WebKit web view. It provides a smooth
looking statusline with CSS transitions and effects, and the possibility of having a
transparent background with effects without a desktop compositor running.

Screenshots are available on `the wiki
<https://github.com/Lokaltog/wkline/wiki/Screenshots>`_.

Installation
------------

wkline is available for Arch Linux users as `wkline-git`_ on the AUR.

Dependencies:

* gtk+ 3
* jansson
* webkitgtk+

Optional dependencies:

* xcb + xcb-wm (desktop list and window title widgets)
* alsa (volume widget)
* libcurl (weather and remote IP widgets)
* libdbus (battery status and notification daemon widgets)
* libmpdclient (mpd status widget)

Installation instructions::

  git clone https://github.com/Lokaltog/wkline.git
  cd wkline

  ./waf configure build
  ./waf install

  wkline

Debug/development build instructions (with relative library search path)::

  git clone https://github.com/Lokaltog/wkline.git
  cd wkline

  ./waf clean configure build --debug --prefix=/ \
      --libdir=`pwd`/out/lib/wkline install --destdir=out

  out/bin/wkline

.. _wkline-git: https://aur.archlinux.org/packages/wkline-git/

Configuration
-------------

Copy ``config.json`` to ``$XDG_CONFIG_HOME/wkline/config.json`` (usually at
``~/.config/wkline/config.json``) and change the configuration. Please make sure that
your config file is valid JSON, this can be checked with e.g. ``jsonlint``.
