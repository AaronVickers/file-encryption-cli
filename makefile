CC = gcc

all: main.out

main.out: main.c aes.c aes.h
	$(CC) main.c aes.c aes.h -o $@

.PHONY: clean
clean:
	-rm -f *.out
