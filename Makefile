CFLAGS=-Wall -Wextra -Wshadow -O2 -ggdb -D_POSIX_C_SOURCE=200112L -D_XOPEN_SOURCE=600 -D_ISOC99_SOURCE -DVERBOSE #-DFORK
LDFLAGS=-Wl,--as-needed
MQ_LIBS=-lrt
DB_LIBS=-lpq
INSTALL=install

.PHONY: all clean install
all: doorbellmon doorbelldb doorbellfake
clean:
	rm -f doorbellmon doorbelldb doorbellfake

prefix=/usr
exec_prefix=$(prefix)
libdir=$(exec_prefix)/lib

install: all
	$(INSTALL) -m 755 -D doorbelldb $(DESTDIR)$(libdir)/arduino-mux/doorbelldb
	$(INSTALL) -m 755 -D doorbellfake $(DESTDIR)$(libdir)/arduino-mux/doorbellfake

doorbellmon: doorbellmon.c doorbellmon.h doorbellq.h Makefile
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(MQ_LIBS)

doorbelldb: doorbelldb.c doorbelldb.h doorbellq.h Makefile doorbelldb_postgres.c doorbelldb_postgres.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(MQ_LIBS) doorbelldb_postgres.c $(DB_LIBS)

doorbellfake: doorbellfake.c doorbellfake.h doorbellq.h Makefile
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(MQ_LIBS)
