/*
 * miditest.c - Test suite for Steev's MIDI Library 
 * Version 1.4
 *
 *  AUTHOR: Steven Goodwin (StevenGoodwin@gmail.com)
 *			Copyright 2010, Steven Goodwin.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of
 *  the License,or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef  __APPLE__
#include <malloc.h>
#endif
#include "midifile.h"



void TestScale(void)
{
MIDI_FILE *mf;

	if ((mf = midiFileCreate("test.mid", TRUE)))
		{
		char *sing[] = {"Doh", "Ray", "Me", "Fah", "So", "La", "Ti", "Doh!"};
		int i, scale[] = {MIDI_OCTAVE_3, MIDI_OCTAVE_3+MIDI_NOTE_D, 
			MIDI_OCTAVE_3+MIDI_NOTE_E, MIDI_OCTAVE_3+MIDI_NOTE_F, MIDI_OCTAVE_3+MIDI_NOTE_G, 
			MIDI_OCTAVE_3+MIDI_NOTE_A, MIDI_OCTAVE_3+MIDI_NOTE_B, MIDI_OCTAVE_4+MIDI_NOTE_C,};

		/* Write tempo information out to track 1. Tracks actually start from zero
		** (this helps differentiate between channels, and ease understanding)
		** although, I'll use '1', by convention.
		*/
		midiSongAddTempo(mf, 1, 120);

		/* All data is written out to _tracks_ not channels. We therefore
		** set the current channel before writing data out. Channel assignments
		** can change any number of times during the file, and affect all
		** tracks messages until it is changed. */
		midiFileSetTracksDefaultChannel(mf, 1, MIDI_CHANNEL_1);

		midiTrackAddProgramChange(mf, 1, MIDI_PATCH_ACOUSTIC_GRAND_PIANO);

		/* common time: 4 crochet beats, per bar */
		midiSongAddSimpleTimeSig(mf, 1, 4, MIDI_NOTE_CROCHET);

		for(i=0;i<8;i++)
			{
			midiTrackAddText(mf, 1, textLyric, sing[i]);
			midiTrackAddNote(mf, 1, scale[i], MIDI_NOTE_CROCHET, MIDI_VOL_HALF, TRUE, FALSE);
			}
		midiFileClose(mf);
		}
}



void TestJingle(void)
{
MIDI_FILE *mf;
int chords[4][3] = {
/*A*/	{ MIDI_OCTAVE_2+MIDI_NOTE_A, MIDI_OCTAVE_3+MIDI_NOTE_C_SHARP, MIDI_OCTAVE_3+MIDI_NOTE_E, },
/*D*/	{ MIDI_OCTAVE_2+MIDI_NOTE_A, MIDI_OCTAVE_3+MIDI_NOTE_D, MIDI_OCTAVE_3+MIDI_NOTE_F_SHARP, },
/*E*/	{ MIDI_OCTAVE_2+MIDI_NOTE_B, MIDI_OCTAVE_3+MIDI_NOTE_E, MIDI_OCTAVE_3+MIDI_NOTE_G_SHARP, },
/*A*/	{ MIDI_OCTAVE_3+MIDI_NOTE_C_SHARP, MIDI_OCTAVE_3+MIDI_NOTE_E, MIDI_OCTAVE_3+MIDI_NOTE_A, },
};
int melody[][2] = { 
	{ MIDI_NOTE_A,		MIDI_NOTE_CROCHET}, 
	{ MIDI_NOTE_A,		MIDI_NOTE_QUAVER}, 
	{ MIDI_NOTE_C_SHARP+MIDI_OCTAVE, MIDI_NOTE_QUAVER}, 
	{ MIDI_NOTE_D+MIDI_OCTAVE, MIDI_NOTE_CROCHET}, 
	{ MIDI_NOTE_C_SHARP+MIDI_OCTAVE, MIDI_NOTE_QUAVER}, 
	{ MIDI_NOTE_B, MIDI_NOTE_QUAVER}, 
	{ MIDI_NOTE_C_SHARP+MIDI_OCTAVE, MIDI_NOTE_QUAVER}, 
	{ MIDI_NOTE_B, MIDI_NOTE_QUAVER}, 
	{ MIDI_NOTE_A, MIDI_NOTE_QUAVER}, 
	{ MIDI_NOTE_G_SHARP, MIDI_NOTE_QUAVER}, 
	{ MIDI_NOTE_A, MIDI_NOTE_MINIM}, 
};
int i;
	
	if ((mf = midiFileCreate("test2.mid", TRUE)))
		{
		/* Set-up an environment for the sound */
		midiSongAddKeySig(mf, 1, keyAMaj);		

		midiFileSetTracksDefaultChannel(mf, 1, MIDI_CHANNEL_1);
		midiFileSetTracksDefaultChannel(mf, 2, MIDI_CHANNEL_2);
		midiFileSetTracksDefaultChannel(mf, 3, MIDI_CHANNEL_DRUMS);

		/* Adding information about the piece */
		midiTrackAddText(mf, 1, textCopyright, "(C) Steev 1998");
		midiTrackAddText(mf, 0, textTrackName, "Jingle in A");
		/* (some sequencers use the first 'textTrackName', regardless of
		** channel, as the title) */

		/* Give them names */
		midiTrackAddText(mf, 1, textTrackName, "Melody");
		midiTrackAddText(mf, 2, textTrackName, "Chords");
		midiTrackAddText(mf, 3, textTrackName, "Drums");

		midiTrackAddProgramChange(mf, 1, MIDI_PATCH_FX_3_CRYSTAL);
		midiTrackAddProgramChange(mf, 2, MIDI_PATCH_SYNTHSTRINGS_1);

		/* Write melody to track 1 */
		for(i=0;i<sizeof(melody)/sizeof(melody[0]);i++)
			{
			midiTrackAddNote(mf, 1, MIDI_OCTAVE_4+melody[i][0], melody[i][1], MIDI_VOL_HALF, TRUE, FALSE);
			/* Since MIDI notes are just integers, we could add 'MIDI_OCTAVE_4+1'
			** here to transpose it very simply into Bb Maj 
			*/
			}

		/* Write chords to track 2 */
		/* Because we need three notes to sound at once, we only move
		** the play ptr on once they've all been played.
		*/
		for(i=0;i<sizeof(chords)/sizeof(chords[0]);i++)
			{
			midiTrackAddNote(mf, 2, chords[i][0], MIDI_NOTE_MINIM, MIDI_VOL_HALF, FALSE, FALSE);
			midiTrackAddNote(mf, 2, chords[i][1], MIDI_NOTE_MINIM, MIDI_VOL_HALF, FALSE, FALSE);
			midiTrackAddNote(mf, 2, chords[i][2], MIDI_NOTE_MINIM, MIDI_VOL_HALF, TRUE, FALSE);
			}

		/* Write a (dull) drum track */
		for(i=0;i<7;i++)
			{
			int vol = MIDI_VOL_HALF;

			if (i==0 || i==4)	/* create accents on first beat */
				vol = MIDI_VOL_HALF+30;

			midiTrackAddNote(mf, 3, MIDI_DRUM_BASS_DRUM, MIDI_NOTE_CROCHET, vol, FALSE, FALSE);
			if (i&1)	/* every other beat */
				midiTrackAddNote(mf, 3, MIDI_DRUM_ELECTRIC_SNARE, MIDI_NOTE_CROCHET, vol, FALSE, FALSE);
			
			/* explicitly move play ptr on */
			midiTrackIncTime(mf, 3, MIDI_NOTE_CROCHET, FALSE);
			}

		midiFileClose(mf);
		}

}



void TestEventList(const char *pFilename)
{
MIDI_FILE *mf = midiFileOpen(pFilename);

	if (mf)
		{
		MIDI_MSG msg;
		int i, iNum;
		unsigned int j;

		midiReadInitMessage(&msg);
		iNum = midiReadGetNumTracks(mf);
		for(i=0;i<iNum;i++)
			{
			printf("# Track %d\n", i);
			while(midiReadGetNextMessage(mf, i, &msg))
				{
				printf("\t");
				for(j=0;j<msg.iMsgSize;j++)
					printf("%.2x ", msg.data[j]);
				printf("\n");
				}
			}

		midiReadFreeMessage(&msg);
		midiFileClose(mf);
		}
}

int main(int argc, char* argv[])
{
	TestScale();
	TestJingle();

	TestEventList("test.mid");

	return 0;
}
