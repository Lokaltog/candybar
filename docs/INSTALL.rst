Dependencies
============

* ``gtk3``
* ``jansson``
* ``webkitgtk``

Optional widget dependencies
----------------------------

======================= ==================================
Widget                  Dependencies
======================= ==================================
``battery``             ``libdbus``
``desktops``            ``xcb``, ``xcb-util-wm``
``email_imap``          ``libcurl``
``external_ip``         ``libcurl``
``magick_background``   ``graphicsmagick``
``notifications``       ``libdbus``
``now_playing_mpd``     ``libmpdclient``
``volume``              ``alsa``
``weather``             ``libcurl``
``window_title``        ``xcb``, ``xcb-util-wm``
======================= ==================================

Installation
============

.. code:: sh

   git clone https://github.com/Lokaltog/wkline.git
   cd wkline

   ./waf configure build
   ./waf install

   wkline

Debug/development build
-----------------------

This is useful for development as it uses a relative library search path, so wkline
doesn't have to be installed system-wide for widgets to work.

.. code:: sh

   ./waf clean configure build --debug --prefix=/ \
      --libdir=`pwd`/out/lib/wkline install --destdir=out

   out/bin/wkline

Packages
========

* Arch Linux
   * `wkline-git`_

.. _wkline-git: https://aur.archlinux.org/packages/wkline-git/
