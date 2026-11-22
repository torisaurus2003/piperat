CC = gcc
CFLAGS = -O2 -march=native -flto -Wall -Wextra -g $(shell pkg-config --cflags gtk4 gtk4-layer-shell-0 libpipewire-0.3)
LIBS = $(shell pkg-config --libs gtk4 gtk4-layer-shell-0 libpipewire-0.3) -lm

piperat: piperat.c
	$(CC) $(CFLAGS) piperat.c -o piperat $(LIBS)
clean:
	rm -f piperat
