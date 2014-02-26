NAME = wkline
CC = gcc -g -Wall
CFLAGS = $(shell pkg-config --cflags --libs gtk+-3.0 webkitgtk-3.0 xcb xcb-ewmh xcb-icccm jansson libcurl alsa libmpdclient)

wkline:
	mkdir -p build
	$(CC) $(CFLAGS) src/$(NAME).c -o build/$(NAME)

clean:
	rm -rf build/
