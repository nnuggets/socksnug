CFLAGS   = -O0 -g -Wall -Wno-gnu-variable-sized-type-not-at-end
CPPFLAGS = -DDEBUG
LDFLAGS  = 
CC       = clang

all: socksnug socksnug_test

socksnug: socksnug.o
	$(CC) $(CFLAGS) $< -o $@

socksnug.o: socksnug.c socksnug.h sn_util.h
	$(CC) $(CFLAGS) -c $< -o $@

socksnug_test : socksnug_test.o
	$(CC) $(CFLAGS) $< -lcunit -o $@

socksnug_test.o : socksnug_test.c socksnug.h sn_util.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf socksnug socksnug_test *.o *~

.PHONY: all clean
