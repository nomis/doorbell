OBJS=oss.o time.o process.o
CFLAGS=-Wall

doorbelld: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

clean:
	rm -f doorbelld *.o
