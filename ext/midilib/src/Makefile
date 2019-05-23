CC=gcc
LINK = $(CC)
CFLAGS = -O2 -ansi -Wall
LDFLAGS = -s

all:	miditest   mozart   mfc120   mididump  m2rtttl

miditest:   miditest.c   midifile.o	
	$(CC) $(CFLAGS) $(LFLAGS) midifile.o miditest.c -o miditest 

mozart: mozmain.c   mozart.c   midifile.o	
	$(CC) $(CFLAGS) $(LFLAGS) midifile.o mozart.c mozmain.c -o mozart 

mfc120: mfcmain.c   mfc120.c   midifile.o
	$(CC) $(CFLAGS) $(LFLAGS) midifile.o mfc120.c mfcmain.c -o mfc120

mididump: mididump.c midiutil.o midifile.o
	$(CC) $(CFLAGS) $(LFLAGS) midifile.o midiutil.o mididump.c -o mididump

m2rtttl: m2rtttl.c midifile.o midiutil.o
	$(CC) $(CFLAGS) $(LFLAGS) midifile.o midiutil.o m2rtttl.c -o m2rtttl

midifile.o:	midifile.c	midifile.h
midiutil.o:	midiutil.c	midiutil.h


install:
	@echo Just copy the files somewhere useful!

clean:
	rm -f *.o 
	rm -f miditest mozart mfc120 mididump m2rtttl

