OBJS=oss.o time.o process.o
TEST_OBJS=test.o process.o
CFLAGS=-Wall

all: doorbelld test

doorbelld: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

test: $(TEST_OBJS)
	$(CC) $(CFLAGS) -o $@ $(TEST_OBJS)

clean:
	rm -f doorbelld *.o
