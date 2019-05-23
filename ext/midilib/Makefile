CC ?= gcc
LINK = $(CC)
CFLAGS ?= -O2 -ansi -Wall
LDFLAGS ?= -s

all:	miditest   mozart   mfc120   mididump  m2rtttl

miditest:   src/miditest.c   src/midifile.o
	$(CC) $(CFLAGS) $(LDFLAGS) src/midifile.o src/miditest.c -o miditest

mozart: src/mozmain.c   src/mozart.c   src/midifile.o
	$(CC) $(CFLAGS) $(LDFLAGS) src/midifile.o src/mozart.c src/mozmain.c -o mozart

mfc120: src/mfcmain.c   src/mfc120.c   src/midifile.o
	$(CC) $(CFLAGS) $(LDFLAGS) src/midifile.o src/mfc120.c src/mfcmain.c -o mfc120

mididump: src/mididump.c src/midiutil.o src/midifile.o
	$(CC) $(CFLAGS) $(LDFLAGS) src/midifile.o src/midiutil.o src/mididump.c -o mididump

m2rtttl: src/m2rtttl.c src/midifile.o src/midiutil.o
	$(CC) $(CFLAGS) $(LDFLAGS) src/midifile.o src/midiutil.o src/m2rtttl.c -o m2rtttl

midifile.o:	src/midifile.c	src/midifile.h
midiutil.o:	src/midiutil.c	src/midiutil.h

minimal:	src/midifile.c src/midifile.h src/midiutil.c src/midiutil.h
	$(CC) $(CFLAGS) $(LDFLAGS) -c src/midifile.c -o src/midifile.o
	$(CC) $(CFLAGS) $(LDFLAGS) -c src/midiutil.c -o src/midiutil.o


install:
	@echo Just copy the files somewhere useful!

clean:
	rm -f *.o
	rm -f src/*.o
	rm -f miditest mozart mfc120 mididump m2rtttl

