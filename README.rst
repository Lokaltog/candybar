wkline
======

:Author: Kim Silkebækken (kim.silkebaekken@gmail.com)
:Source: https://github.com/Lokaltog/wkline

**WebKit-based status line for tiling window managers. Requires Python 3, GTK+ 3 with
WebKit bindings and xdotool.**

This is a proof-of-concept statusline plugin that launches a plain GTK window with a
WebKit WebView pointing to a static HTML file containing the statusline. Planned
features include WebSocket communication with a Powerline-type API that provides
information from the window manager itself. This should work well with any window
manager that uses an executable to configure the layout and navigation (as opposed to a
static config file) like ``bspwm``, or any "dynamic" window manager like ``qtile``.

A basic proof-of-concept is currently available. Clone the repo, run ``npm install``,
``grunt production``, configure the HTML file path in ``scripts/wkline`` then run
``scripts/wkline-launcher``. This will draw a statusline on the top of the screen. See
screenshots below.

Why Python and GTK+WebKit?
--------------------------

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

Screenshots
-----------
.. image:: http://i.imgur.com/qkZjKw6.png
   :alt: Concept screenshot
