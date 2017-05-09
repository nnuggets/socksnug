CFLAGS   = -O0 -g -Wall -Wno-gnu-variable-sized-type-not-at-end
CPPFLAGS = -DDEBUG
LDFLAGS  = 
CC       = clang

all: socksnug socksnug_test

socksnug: socksnug.o socksnug_impl.o socksnug_util.o
	$(CC) $(CFLAGS) $^ -o $@

socksnug.o: socksnug.c socksnug.h socksnug_util.h
	$(CC) $(CFLAGS) -c $< -o $@

socksnug_impl.o: socksnug_impl.c socksnug.h
	$(CC) $(CFLAGS) -c $< -o $@

socksnug_util.o: socksnug_util.c socksnug_util.h
	$(CC) $(CFLAGS) -c $< -o $@

socksnug_test : socksnug_test.o socksnug_impl.o
	$(CC) $(CFLAGS) $^ -lcunit -o $@

socksnug_test.o : socksnug_test.c socksnug.h socksnug_util.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf socksnug socksnug_test *.o *~ \#*# .#*

.PHONY: all clean
