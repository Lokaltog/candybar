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
* xcb
* xcb-wm

Optional dependencies:

* alsa (volume widget)
* libcurl (weather widget)
* libdbus (notification daemon widget)
* libmpdclient (now playing/mpd widget)

Installation instructions::

  git clone https://github.com/Lokaltog/wkline.git
  cd wkline
  autoreconf --install
  ./configure
  make
  ./wkline

Configuration
-------------

Edit ``src/config.h`` and recompile.

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

*Please note that some of these screenshots may showcase features that were part of
the Python concept, and may not have been ported to C yet. Every feature that was
part of the concept (see commit 4b59f4) is being ported to C.*
