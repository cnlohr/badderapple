include ../common/make.inc

all : repose

repose : repose.c
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS)

clean :
	rm rf *.o *~ repose

