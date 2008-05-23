CFLAGS=-g -DIS_DEBUG -Wall  -DUSE_DMALLOC
LDFLAGS=-lpthread  -ldmalloc
LDFLAGS=-lpthread 
CFLAGS=-g -DIS_DEBUG -Wall
LD=gcc 

OBJS=smtprelay.o connection.o session.o sender.o

smtp_relay: $(OBJS)
	$(LD) $(LDFLAGS) -o smtp_relay $(OBJS)


smtprelay.o: smtprelay.h connection.h

session.o: session.h smtprelay.h sender.h 

sender.o: sender.h connection.h smtprelay.h

connection.o: connection.h smtprelay.h session.h

clean: 
	rm -f *.o smtp_relay
