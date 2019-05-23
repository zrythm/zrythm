/*
 * mozart.c - Processing code for 'Mozart Dice'.
 * Version 1.4
 *
 *  AUTHOR: Steven Goodwin (StevenGoodwin@gmail.com)
 *			Copyright 1998-2010, Steven Goodwin.
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
#include "mozart.h"

/*
** Data Tables
*/
MOZART_TABLE g_mMinuet, g_mTrio;

static int iMinuetTable[] = {
 96,  22, 141,  41, 105, 122,  11,  30,  70, 121,  26,   9, 112,  49, 109,  14,
 32,   6, 128,  63, 146,  46, 134,  81, 117,  39, 126,  56, 174,  18, 116,  83,
 69,  95, 158,  13, 153,  55, 110,  24,  66, 139,  15, 132,  73,  58, 145,  79,
 40,  17, 113,  85, 161,   2, 159, 100,  90, 176,   7,  34,  67, 160,  52, 170,
148,  74, 163,  45,  80,  97,  36, 107,  25, 143,  64, 125,  76, 136,   1,  93,
104, 157,  27, 167, 154,  68, 118,  91, 138,  71, 150,  29, 101, 162,  23, 151,
152,  60, 171,  53,  99, 133,  21, 127,  16, 155,  57, 175,  43, 168,  89, 172,
119,  84, 114,  50, 140,  86, 169,  94, 120,  88,  48, 166,  51, 115,  72, 111,
 98, 142,  42, 156,  75, 129,  62, 123,  65,  77,  19,  82, 137,  38, 149,   8,
  3,  87, 165,  61, 135,  47, 147,  33, 102,   4,  31, 164, 144,  59, 173,  78,
 54, 130,  10, 103,  28,  37, 106,   5,  35,  20, 108,  92,  12, 124,  44, 131,
};

static int iTrioTable[] = {
72,  6, 59, 25, 81, 41, 89,  13, 36,  5, 46, 79,  30, 95, 19, 66,
56, 82, 42, 74, 14,  7, 26,  71, 76, 20, 64, 84,   8, 35, 47, 88,
75, 39, 54,  1, 65, 43, 15,  80,  9, 34, 93, 48,  69, 58, 90, 21,
40, 73, 16, 68, 29, 55,  2,  61, 22, 67, 49, 77,  57, 87, 33, 10,
83,  3, 28, 53, 37, 17, 44,  70, 63, 85, 32, 96,  12, 23, 50, 91,
18, 45, 62, 38,  4, 27, 52,  94, 11, 92, 24, 86,  51, 60, 78, 31,
};

static int mozRandom(int max_size)
{
long big=RAND_MAX;
long value=0;

	if	(max_size<2)	return(0);          

	value=rand();
	value=((long)value*(long)max_size)/big;
	
	/* 2001 postscript- I can't remember why this paranoia check is needed :( */
	if	(value<0 || value>=max_size)	return 0;
	
	return((int)value);	
}

void mozartRandomize(MOZART_PREFS *prefs, const MOZART_TABLE *pTable)
{
int i, n, r;

	for(i=0;i<pTable->iBarMeasures;++i)
		{
		n = 0;
		for(r=0;r<pTable->iNumDice;++r)
			n += mozRandom(pTable->iSidesOnDie);
		
		prefs->iCurrentPattern[i] = n;
		}
}


static BOOL mozartFileConcat(MIDI_FILE *mf, const char *fn, DWORD *last_bar_end)
{
MIDI_FILE *mif;
MIDI_MSG msg;
BOOL first=TRUE;
DWORD dt=0, last_pos=0;
int notes_up=0, notes_down=0;
BYTE notes[128];
DWORD end_of_last_midi_pos;

	midiReadInitMessage(&msg);
	memset(notes, '\0', sizeof(notes));
	
	end_of_last_midi_pos = midiTrackGetEndPos(mf, 1);
	
	if ((mif = midiFileOpen(fn)))
		{
		while (midiReadGetNextMessage(mif, 0, &msg))
			{
			if (first && msg.dt)
				{
				/* Does the first note start exactly at the start of the bar, or a little bit
				** afterward? Whatever it is, we must maintain it.
				*/
				if (msg.dt > (DWORD)midiFileGetPPQN(mif)*3)
					dt = msg.dt - midiFileGetPPQN(mif)*3;		/* we know (in this case) that the first bar (or 3 three beats) is empty */
				else
					dt = 0;
				/* Add padding from incomplete last bar?*/
				if (end_of_last_midi_pos < *last_bar_end)
					dt += *last_bar_end-end_of_last_midi_pos;
				
				first = FALSE;
				}
			else
				{
				dt = msg.dwAbsPos-last_pos;
				}

			if (msg.iType == msgNoteOn && msg.MsgData.NoteOn.iVolume==0)
					notes[msg.MsgData.NoteOn.iNote]--, notes_up++;
			else if (msg.iType == msgNoteOff)
					notes[msg.MsgData.NoteOn.iNote]--, notes_up++;
			else if (msg.iType == msgNoteOn)
					notes[msg.MsgData.NoteOn.iNote]++, notes_down++;

			/*	Ignore end of sequence msg */
			if (msg.iType == msgMetaEvent && msg.MsgData.MetaEvent.iType == metaCopyright)
				;
			else if (msg.iType == msgMetaEvent && (msg.MsgData.MetaEvent.iType == metaCuePoint || msg.MsgData.MetaEvent.iType == metaTextEvent))
				;
			else if (msg.iType == msgMetaEvent && msg.MsgData.MetaEvent.iType == metaEndSequence)
				;
			else
				midiTrackAddRaw(mf, 1, msg.iMsgSize, msg.data, TRUE, (dt*midiFileGetPPQN(mf))/midiFileGetPPQN(mif));
			
			last_pos = msg.dwAbsPos;
			}
		
		(*last_bar_end) += midiFileGetPPQN(mif)*3;
		}
	midiReadFreeMessage(&msg);
	
	return TRUE;
}

BOOL mozartCreateMidi(const char *pFilename, MOZART_TABLE *pTable, MOZART_PREFS *prefs, BOOL bOverwrite)
{
MIDI_FILE *mf;
char str[128];
DWORD last_bar_pos = 0;
int i, part;
	
	if ((mf = midiFileCreate(pFilename, bOverwrite)))
		{
		midiFileSetPPQN(mf, 480);
		midiTrackAddText(mf, 1, textCopyright, "Mozart Dice - (C) VisionSoft 1998 - Written by S.Goodwin, MIDI files played and programmed by John Chuang, 1995, 1996. All rights reserved.");
		
		for(i=0;i<pTable->iBarMeasures;++i)
			{
			part = pTable->pTable[(pTable->iBarMeasures*prefs->iCurrentPattern[i])+i];

			sprintf(str, "%d", part);
			midiTrackAddText(mf, 1, metaCuePoint,str);

			sprintf(str, pTable->szFilePattern, part);
			mozartFileConcat(mf, str, &last_bar_pos);
			}
		midiFileClose(mf);
		}
	
	return mf?TRUE:FALSE;
}

void mozartInit(void)
{
	/* Minuets */
	g_mMinuet.pTable = iMinuetTable;
	g_mMinuet.iBarMeasures = 16;
	g_mMinuet.iNumDice = 2;
	g_mMinuet.iSidesOnDie = 6;
	strcpy(g_mMinuet.szFilePattern, "MIDIFiles/M%d.MID");

	/* Trios */
	g_mTrio.pTable = iTrioTable;
	g_mTrio.iBarMeasures = 16;
	g_mTrio.iNumDice = 1;
	g_mTrio.iSidesOnDie = 6;
	strcpy(g_mTrio.szFilePattern, "MIDIFiles/T%d.MID");
}

