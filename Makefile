CFLAGS   = -O0 -g -Wall -Wno-gnu-variable-sized-type-not-at-end
CPPFLAGS = -DDEBUG
LDFLAGS  = 
CC       = clang

all: socksnug socksnug_test

socksnug: socksnug.o socksnug_impl.o socksnug_util.o socksnug_net.o socksnug_thread.o
	$(CC) $(CFLAGS) $^ -lpthread -o $@

socksnug.o: socksnug.c socksnug.h socksnug_util.h
	$(CC) $(CFLAGS) -c $< -o $@

socksnug_impl.o: socksnug_impl.c socksnug.h socksnug_thread.h
	$(CC) $(CFLAGS) -c $< -o $@

socksnug_util.o: socksnug_util.c socksnug_util.h
	$(CC) $(CFLAGS) -c $< -o $@

socksnug_net.o: socksnug_net.c socksnug_net.h
	$(CC) $(CFLAGS) -c $< -o $@

socksnug_thread.o: socksnug_thread.c socksnug_thread.h
	$(CC) $(CFLAGS) -c $< -o $@

socksnug_test : socksnug_test.o socksnug_impl.o socksnug_util.o socksnug_net.o socksnug_thread.o
	$(CC) $(CFLAGS) $^ -lcunit -lpthread -o $@

socksnug_test.o : socksnug_test.c socksnug.h socksnug_util.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf socksnug socksnug_test *.o *~ \#*# .#*

.PHONY: all clean
