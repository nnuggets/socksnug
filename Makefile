CFLAGS   = -O0 -g -Wall
CPPFLAGS = -DDEBUG
LDFLAGS  = 
CC       = gcc

all: socksnug

socksnug: socksnug.o
	$(CC) $(CFLAGS) $< -o $@

socksnug.o: socksnug.c socksnug.h sn_util.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf socksnug *.o

.PHONY: all clean
