CC		=  gcc
CFLAGS	        += -std=c99 -D_GNU_SOURCE -Wall -pthread

INCDIR		= ./include
LIBDIR          = ./lib
SRCDIR          = /src
BINDIR          = ./

INCLUDES        = -I $(INCDIR)
LIBS		= -lmarket
LDFLAGS         = -L $(LIBDIR) -Wl,-rpath=$(LIBDIR)
OPTFLAGS	= -O3
DBGFLAGS        = -g -DNDEBUG
FLAGS           = $(DBGFLAGS) $(OPTFLAGS)


TARGET		= $(BINDIR)/market

.PHONY: all clean
#Static library  
	 

all: market

market: bin/market.o bin/cashier.o bin/client.o bin/manager.o lib/libmarket.a src/marketlib.h
	$(CC) $(CFLAGS) bin/market.o -Llib -lmarket -o market -g -pthread

bin/market.o: src/market.c src/marketlib.h
	$(CC) $(CFLAGS) -c -g src/market.c -o bin/market.o -pthread

bin/client.o: src/client.c src/marketlib.h
	$(CC) $(CFLAGS) -c -g src/client.c -o bin/client.o

bin/cashier.o: src/cashier.c src/marketlib.h
	$(CC) $(CFLAGS) -c -g src/cashier.c -o bin/cashier.o

bin/manager.o: src/manager.c src/marketlib.h
	$(CC) $(CFLAGS) -c -g src/manager.c -o bin/manager.o

lib/libmarket.a: bin/manager.o bin/cashier.o bin/client.o src/marketlib.h
	ar rcs lib/libmarket.a bin/cashier.o bin/client.o bin/manager.o

clean:
	-rm -f  $(LIBDIR)/libmarket.a
	-rm -f ./data.txt
	-rm -f ./market.PID
	-rm -f ./market

test:
	(./market & echo $$! > market.PID) &
	sleep 25s; \
	kill -1 $$(cat market.PID); \
	./analisi.sh $$(cat market.PID); \
	
	




