CFLAGS=-g -DIS_DEBUG -Wall
LDFLAGS=-lefence
LD=gcc 

OBJS=smtprelay.o connection.o session.o

smtp_relay: $(OBJS)
	$(LD) $(LDFLAGS) -o smtp_relay $(OBJS)


smtprelay.o: smtprelay.h connection.h

session.o: session.h smtprelay.h

connection.o: connection.h smtprelay.h session.h

clean: 
	rm -f *.o smtp_relay
