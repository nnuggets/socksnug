CFLAGS   = -O0 -g
CPPFLAGS = -DDEBUG
LDFLAGS  = 
CC       = gcc

all: socksnug

socksnug: socksnug.o
	$(CC) $(CFLAGS) $< -o $@

socksnug.o: socksnug.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf socksnug *.o

.PHONY: all clean
