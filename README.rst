wkline
======

:Author: Kim Silkebækken (kim.silkebaekken@gmail.com)
:Source: https://github.com/Lokaltog/wkline

**WebKit-based status line for tiling window managers.**

wkline is a statusline plugin that launches a plain GTK window with a WebKit WebView
pointing to a static HTML file containing the statusline. It provides a smooth
looking statusline with CSS transitions and effects and the possibility of having a
blurred background without a desktop compositor running.

Installation
------------

wkline is available for Arch Linux users as ``wkline-git`` on the AUR.

Dependencies:

* gtk+ 3
* jansson
* webkitgtk+
* waf (required for building wkline)

Optional dependencies:

* xcb (desktops/window title widgets)
* xcb-wm (desktops/window title widgets)
* alsa (volume widget)
* libcurl (weather/remote IP widgets)
* libdbus (notification daemon widget)
* libmpdclient (now playing/mpd widget)

Installation instructions::

  git clone https://github.com/Lokaltog/wkline.git
  cd wkline

  waf configure build
  waf install

  wkline

Debug/development build instructions (with relative library search path)::

  git clone https://github.com/Lokaltog/wkline.git
  cd wkline

  waf clean configure build --debug --prefix=/ \
      --libdir=`pwd`/out/lib/wkline install --destdir=out

  out/bin/wkline

Configuration
-------------

Copy ``config.json`` to ``$XDG_CONFIG_HOME/wkline/config.json`` (usually at
``~/.config/wkline/config.json``) and change the configuration. Please make sure that
your config file is valid JSON, this can be checked with e.g. ``jsonlint``.

Screenshots
-----------

.. image:: http://i.imgur.com/tWGCVze.gif
   :alt: Notification demo

.. image:: http://i.imgur.com/bIjz45R.gif
   :alt: Notification demo

.. image:: http://i.imgur.com/CdtPSJi.png
   :alt: Test script screenshot

.. image:: http://i.imgur.com/qkZjKw6.png
   :alt: Concept screenshot

.. image:: http://i.imgur.com/whgqRGH.png
   :alt: Concept screenshot

.. image:: http://i.imgur.com/gpEKgyS.png
   :alt: Concept screenshot
