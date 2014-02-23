wkline
======

:Author: Kim Silkebækken (kim.silkebaekken@gmail.com)
:Source: https://github.com/Lokaltog/wkline

**WebKit-based status line for tiling window managers. Requires Python 3, GTK+ 3 and
libxcb.**

This is a proof-of-concept statusline plugin that launches a plain GTK window with a
WebKit WebView pointing to a static HTML file containing the statusline. Planned
features include WebSocket communication with a Powerline-type API that provides
information from the window manager itself. This should work well with any window
manager that uses an executable to configure the layout and navigation (as opposed to a
static config file) like ``bspwm``, or any "dynamic" window manager like ``qtile``.

Installation
------------

*The following instructions are temporary.*

Build dependencies:

* gtk+ 3
* libxcb

Update daemon dependencies:

* Python 3
* pygtk
* python-mpd2 (optional, mpd support)
* python-dbus (optional, notification daemon support)
* libnotify (optional, notification daemon support)

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

  # run wkline, the -u parameter accepts any uri pointing to a wkline theme
  build/wkline -u 'file:///path/to/wkline-theme-default/webroot/index.html'

  # run test script, this will add a bunch of demo data to the statusline
  scripts/wkline-test

  # run update daemon
  # currently handles date/time, weather, music, etc.
  scripts/wkline-daemon

  # send notification with notify-send
  # valid categories are "error" and "warning"
  notify-send -c warning "Warning" "This is a test warning"
  notify-send -c error "Error" "This is a test error"

Screenshots
-----------

.. image:: http://i.imgur.com/qkZjKw6.png
   :alt: Concept screenshot

.. image:: http://i.imgur.com/whgqRGH.png
   :alt: Concept screenshot

.. image:: http://i.imgur.com/gpEKgyS.png
   :alt: Concept screenshot

Why Python/GTK/WebKit?
----------------------

No window manager currently provides the power and flexibility of using HTML and CSS
for styling the status line. The WebKit WebView is fast and lightweight, and using
Python should make it fairly simple to run threads in the background to update
weather, temperature, etc. and send it to the statusline. Plus, it's a fun experiment.

Ideas
-----

* Per-tag background images. Faux Mac-style blurred transparency possible by setting
  an absolutely positioned background image with a CSS blur filter on it along with a
  semi-transparent white background color.
* ``notification-daemon`` support (concept shown in screenshots). Animated popup
  (expanding horizontally).
* CSS animations.
* Animated transitions when e.g. the weather changes.
* Animated warnings, e.g. high CPU temperature could have a flashing status box.
* Use d3.js or similar graphing library for history graphs for e.g. CPU and RAM
  usage.
* Highlight tags with a window with the urgent hint set.
* Possibly support non-tiling window managers? The concept isn't really limited to
  tiling window managers, but adding a window list isn't a priority.
* Possibly use other browsers as alternatives? Both uzbl and chromium provides a
  kiosk mode.
* Disable faux transparency with fullscreen windows or in monocle mode. Only enable
  transparency if no windows are displayed, or if all windows are floating (should
  look much better).
* Shorten certain segments to an icon, expand horizontally on hover. This should save
  a lot of space.
