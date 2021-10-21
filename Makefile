# Makefile
LDFLAGS=-lncurses

snake: main.c
	gcc -o snake main.c $(LDFLAGS)

all: snake