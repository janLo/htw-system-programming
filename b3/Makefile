LDFLAGS=-lpthread 
CFLAGS=$(DEBUG) -Wall
LD=gcc 
BIN_NAME=smtp_relay
OBJS=smtprelay.o connection.o session.o sender.o smtp_session.o user_session.o


all:
	$(MAKE) binary
	rm -f *.o
	$(MAKE) binary_debug

binary: $(OBJS)
	$(LD) $(LDFLAGS) -o $(BIN_NAME) $(OBJS)

binary_debug:
	$(MAKE) binary 'DEBUG=-g -DIS_DEBUG' 'BIN_NAME=smtp_relay_debug'

smtprelay.o: smtprelay.h connection.h

session.o: session.h smtprelay.h sender.h smtp_session.h user_session.h

sender.o: sender.h connection.h smtprelay.h

connection.o: connection.h smtprelay.h session.h

clean: 
	rm -f *.o smtp_relay smtp_relay_debug
