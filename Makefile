CC = gcc

all:
	$(CC) test.c -o test
	$(CC) test.c -o test-bss -D RSA_BSS

debug:
	make
	./test
	./test-bss