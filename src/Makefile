OBJS=time.o exec.o process.o
TEST_OBJS=test.o time.o process.o
CFLAGS=-Wall -Wshadow -Wextra -ggdb
LDFLAGS=

OBJS+=oss.o

#OBJS+=alsa.o
#LDFLAGS+=-lasound

all: doorbelld test

doorbelld: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS)

test: $(TEST_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(TEST_OBJS)

clean:
	rm -f doorbelld test *.o
