CC=gcc
CFLAGS=-I.

leptonic: main.c
	gcc -o leptonic main.c -I.
