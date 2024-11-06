CFLAGS=-Wall -Wextra -pedantic -std=gnu17
.PHONY: all asm

all:
	gcc -o BFC BFC.c $(CFLAGS)

