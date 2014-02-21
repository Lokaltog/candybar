NAME = wkline
CC = gcc -g -Wall
CFLAGS = $(shell pkg-config --cflags --libs gtk+-3.0 webkitgtk-3.0 xcb)

wkline:
	$(CC) $(CFLAGS) $(NAME).c -o $(NAME)

clean:
	rm -f wkline
