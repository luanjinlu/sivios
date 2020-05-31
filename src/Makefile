ifdef DEBUG
CFLAGS=-I. -I/usr/local/include -DHTAB_SIZE=65521 -g3 -c
else
CFLAGS=-I. -I/usr/local/include -O3 -c
endif
LDFLAGS=-L/usr/local/lib -L../lib
LIBS=-levent -lcrypto
LIBSsivios=libevent.a libcrypto.a -ldl
LIBSsivioc=libcrypto.a -ldl
PREFIX=/usr/local

all: sivios sivioc

sivios: sivios.o siviosapi.o memfin.o filht.o htab.o utils.o findb.o
	gcc $(LDFLAGS) -o $@ $^ $(LIBS)
sivioc: sivioc.o memfin.o filht.o htab.o utils.o findb.o
	gcc $(LDFLAGS) -o $@ $^ $(LIBS)

sivios-static: sivios.o siviosapi.o memfin.o filht.o htab.o utils.o findb.o
	gcc -o $@ $^ $(LIBSsivios)
sivioc-static: sivioc.o memfin.o filht.o htab.o utils.o findb.o
	gcc -o $@ $^ $(LIBSsivioc)

install: all
	cp sivios sivioc $(PREFIX)/bin/
uninstall:
	rm $(PREFIX)/bin/sivios $(PREFIX)/bin/sivioc
clean:
	rm *.o

.PHONY: clean
.SUFFIXES: .c .o

.c.o:; gcc $(CFLAGS) $<

