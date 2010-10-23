OBJS=alsa.o time.o exec.o process.o
TEST_OBJS=test.o time.o process.o
CFLAGS=-Wall -Wshadow -Wextra -ggdb -lasound

all: doorbelld test

doorbelld: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

test: $(TEST_OBJS)
	$(CC) $(CFLAGS) -o $@ $(TEST_OBJS)

clean:
	rm -f doorbelld test *.o
