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

Dependencies:

* gtk+ 3
* webkitgtk+
* jansson
* xcb
* xcb-ewmh
* libcurl *(optional, weather widget support)*

Theme build dependencies:

* node
* node-grunt

Installation instructions::

  # clone source repos
  git clone https://github.com/Lokaltog/wkline.git
  git clone https://github.com/Lokaltog/wkline-theme-default.git

  # build default theme
  cd wkline-theme-default
  npm install
  grunt production

  # build wkline
  cd ../wkline
  make

  # run wkline
  build/wkline

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

What currently works?
---------------------

Tag list, current window name, weather information, date and time.

mpd status, sound level and notification widgets are currently not implemented (they
are being ported to C).
