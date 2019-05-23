Steev's MIDI Library
Version 1.5, August 2010


Steven Goodwin (StevenGoodwin@gmail.com)
Copyright 1998-2010, Steven Goodwin

Released under the GPL 2



Steev's MIDI Library is a pure C library to handle the MIDI protocol, as well 
as providing code to read and write standards-compliant MIDI files. This 
provides code functionality for apps wishing to support MIDI file serialism, 
as well as write music apps that auto-compose music.

It provides sample code to:

* Convert MIDI files to RTTTL ring tones (m2rtttl)
* Dump the contents of MIDI files (mididump)
* Convert files from version 1 to 0,  as some electronic keyboards support 
  only version 0. (mfc120)
* Auto-generate music, based on "Mozarts Dice" (mozart)

It is a wonderfully compact, and complete, library that contains all the 
constants necessary to understand the protocol, such as instrument names, 
drum maps, and controller names & IDs. There's also utility methods to 
compute the real-world frequency of any given note and guess the currently 
playing chord.



Integration with SGX
====================

Between versions 1.3 and 1.4, the base of the library was incorporated into 
SGX. The changes were limited to the methods of handling the filesystem 
(standard C, to the SGX filesystem) and filename conventions.

In version 1.4 some utility methods were added to SGX first, and then 
backported into this release.

Going forward, all algorithm and function changes will be released here 
first, as it is the lowest common development denominator.



Downloads and Links
===================

http://www.bluedust.dontexist.com/midilib/midilib-1.4.tar.gz - the latest 
version of the library

http://www.sgxengine.com - Featuring the "other" implementation of MIDIlib


