CC = gcc

all: main.out

main.out: main.o aes.o encrypt.o utils.o
	$(CC) main.o aes.o encrypt.o utils.o -o $@

main.o: main.c
	$(CC) main.c -c -o $@

aes.o: aes.c aes.h
	$(CC) aes.c -c -o $@

encrypt.o: encrypt.c encrypt.h
	$(CC) encrypt.c -c -o $@

utils.o: utils.c utils.h
	$(CC) utils.c -c -o $@

.PHONY: clean
clean:
	-rm -f *.out
	-rm -f *.o
