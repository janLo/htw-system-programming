BELEGE=b1 b2 b3

all: 
	for i in $(BELEGE);\
	  do \
	  $(MAKE) -C $$i all; \
	  done;

clean:
	for i in $(BELEGE); \
	  do \
	  $(MAKE) -C $$i clean; \
	  done;

