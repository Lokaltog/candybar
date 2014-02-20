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
``grunt production``, configure the HTML file path in ``./webkitwindow`` then run
``./statusline``. This will draw a statusline on the top of the screen. See
screenshots below.

Screenshots
-----------
.. image:: http://i.imgur.com/qkZjKw6.png
   :alt: Concept screenshot
