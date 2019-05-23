/*
 * m2rtttl.c - Conversion from MIDI to RTTTL (for mobile phones)
 *				Requires Steevs MIDI Library 
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

/*
 TODO:

- Produce an accurate scale (o) parameter, based on average pitch of track
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef  __APPLE__
#include <malloc.h>
#endif
#include <fcntl.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/kd.h>
#include <sys/ioctl.h>
#include "midifile.h"
#include "midiutil.h"

/*
iNote = MIDI msg value
iVol = 1-127. 0=off (rest)
iTimeMS = milli-seconds
*/

typedef struct {
		int 		iTempo;
		/* Conversion specifics */
		BOOL		bDoneHeader;		/* text */
		BOOL		bNeedPrefixComma;	/* text */
		int		iSndFile;		/* spkr */
		float		fMult;			/* spkr */
		} CONVERT_PREFS;
typedef void (*cb_Output)(int iNote, int iVol, int iTimeMS, CONVERT_PREFS *pPrefs);


void InitPrefs(CONVERT_PREFS *pPrefs)
{
	pPrefs->iTempo = 100;
	pPrefs->bDoneHeader = FALSE;
	pPrefs->bNeedPrefixComma = FALSE;
	pPrefs->iSndFile = 1;
}

int rtttlGetClosestTempo(int curr)
{
static int Valid[] = {25,28,31,35,40,45,50,56,63,70,80,90,100,112,
			125,140,160,180,200,225,250,285,320,355,400,
			450,500,565,635,715,800,900, };
int iBest=9999, iDiff, i, iSetting=0;

	for(i=0;i<sizeof(Valid)/sizeof(Valid[0]);i++)
		{
		iDiff = Valid[i]-curr;
		if (iDiff < 0)	iDiff=-iDiff;
		if (iDiff < iBest)
			{
			iBest = iDiff;
			iSetting = i;
			}
		}	
	return Valid[iSetting];
}

void sndPlayBeep(float freq, int ms, CONVERT_PREFS *pPrefs)
{
const int iInterNoteGap = 5;

	if (freq)
		freq = 1190000.0f/freq;

	ioctl(pPrefs->iSndFile, KIOCSOUND, (int)freq);
	if (ms > iInterNoteGap)
		{
		/* This small gap between notes sounds more pleasing */
		ms -= iInterNoteGap;
		usleep(ms*1000L);
		ioctl(pPrefs->iSndFile, KIOCSOUND, 0);
		ms = iInterNoteGap;
		}
	usleep(ms*1000L);
}

void outStdout(int iNote, int iVol, int iDeltaTime, CONVERT_PREFS *pPrefs)
{
static char *pNoteNames[] = {
"c","c#","d","d#","e","f","f#","g","g#","a","a#","b",
};
int iLen;

	if (iDeltaTime==0)	
		return;		/* Can't handle anything of zero length */

	if (!pPrefs->bDoneHeader)
		{
		int tempo = rtttlGetClosestTempo(pPrefs->iTempo/4);
		printf("t=%d:", tempo);
		pPrefs->bDoneHeader = TRUE;
		}

	if (pPrefs->bNeedPrefixComma)
		printf(",");

	iLen = (32*384)/iDeltaTime;
	printf("%d%s%d", iLen, pNoteNames[iNote%12], (iNote/12)-3);

	pPrefs->bNeedPrefixComma = TRUE;
}

void outSMLCCode(int iNote, int iVol, int iDeltaTime, CONVERT_PREFS *pPrefs)
{
/* For future expansion */
}

void outSpeaker(int iNote, int iVol, int iDeltaTime, CONVERT_PREFS *pPrefs)
{
int ms;

	ms = (60000*iDeltaTime)/(4*pPrefs->iTempo*384);
	/* This _technically_ will not play MIDI note 0 */
	if (iNote == 0)
		sndPlayBeep(0, ms, pPrefs);
	else
		sndPlayBeep(muGetFreqFromNote(iNote), ms, pPrefs);
}

void ConvertMIDI(const char *pFilename, const cb_Output pAddNote, int iChannel, CONVERT_PREFS *pPrefs)
{
MIDI_FILE *mf = midiFileOpen(pFilename);
int iCurrPlayingNote, iCurrPlayingVol, iCurrPlayStart;
int dtPos;

	if (mf)
		{
		MIDI_MSG msg;
		int i, iNum;

		dtPos = 0;
		iCurrPlayingNote = -1;
		iCurrPlayStart = 0;
		iCurrPlayingVol = 127; /* paranoia: shouldn't be able to play a note with a note-down msg */
		midiReadInitMessage(&msg);
		iNum = midiReadGetNumTracks(mf);

		for(i=0;i<iNum;i++)
			{
			while(midiReadGetNextMessage(mf, i, &msg))
				{
				dtPos = (msg.dwAbsPos - iCurrPlayStart);

				switch(msg.iType)
					{
					case	msgNoteOff:
							if (iChannel == msg.MsgData.NoteOff.iChannel)
							{
							if (iCurrPlayingNote==msg.MsgData.NoteOff.iNote)	
								{
								(*pAddNote)(iCurrPlayingNote, iCurrPlayingVol, dtPos, pPrefs);
								iCurrPlayingNote = -1;
								iCurrPlayStart = msg.dwAbsPos;
								}
							}
							break;

					case	msgNoteOn:
							if (iChannel == msg.MsgData.NoteOn.iChannel)
							{
							if (iCurrPlayingNote==-1)
								{ /* play a rest*/
								(*pAddNote)(0, 0, dtPos, pPrefs);
								}
							else
								{
								(*pAddNote)(iCurrPlayingNote, iCurrPlayingVol, dtPos, pPrefs);
								}
							iCurrPlayingNote = msg.MsgData.NoteOn.iNote;
							iCurrPlayingVol = msg.MsgData.NoteOn.iVolume;
							iCurrPlayStart = msg.dwAbsPos;
							}
							break;
					case	msgMetaEvent:
							switch(msg.MsgData.MetaEvent.iType)
								{
								case	metaSetTempo:
									pPrefs->iTempo = msg.MsgData.MetaEvent.Data.Tempo.iBPM;
									break;
								default:
									/* Ignore other cases */
									break;
								}
							break;
					default:
						/* Ignore other cases */
						break;
					}
				}
			}

		midiReadFreeMessage(&msg);
		midiFileClose(mf);
		}
}

void Usage(const char *pProgName)
{
	fprintf(stderr, "Usage: %s [-c channel] [-h][-r][-s] file...\n", pProgName);
}

void PrintHelp(void)
{
	fprintf(stderr, "You can rip any single MIDI channel and convert it into a mobile phone");
	fprintf(stderr, "ring (the RTTTL format), or play it through the PC's speaker.");
	fprintf(stderr, "\n-c\tRip only from a single channel");
	fprintf(stderr, "-r\tConvert into RTTTL format (default)");
	fprintf(stderr, "-s\tPlay as sounds through PC speaker");
	fprintf(stderr, "-h\tHelp page");
}

int main(int argc, char* argv[])
{
int c;
int iChan = 1;
BOOL bError = FALSE;
BOOL bRTTTL = FALSE, bSpeaker = FALSE;
CONVERT_PREFS prefs;

	while((c=getopt(argc, argv, "Cc:HRShrs"))!=-1)
		{
		switch(c)
			{
			case	'C':
			case	'c':	/* channel */
					iChan = atoi(optarg);
					break;

			case	'R':
			case	'r':	/* output RTTTL */
					bRTTTL = TRUE;
					break;

			case	'S':
			case	's':	/* output speaker */	
					bSpeaker = TRUE;
					break;
			
			case	'H':
			case	'h':
					Usage(argv[1]);
					PrintHelp();
					break;

			case	'?':	/* error */
					bError = TRUE;
					break;
			case	':':
					fprintf(stderr, "%s: The %c option needs an operand\n", argv[0], optopt);
					if (optopt == 'c')
						fprintf(stderr, "(i.e. a channel number)");
					break;
			}
		}	
	/* Default to RTTTL if nothing specified */
	if (bRTTTL == bSpeaker && bSpeaker == FALSE)
		bRTTTL = TRUE;

	if (bError)	
		{
		Usage(argv[0]);
		}
	else
		{

		while(optind < argc)
			{
			InitPrefs(&prefs);
			if (bRTTTL) 
				{
				printf("converted:d=4,o=4,");
				ConvertMIDI(argv[optind], outStdout, iChan, &prefs);
				printf("\n");
				}

			if (bSpeaker) 
				{
				ConvertMIDI(argv[optind], outSpeaker, iChan, &prefs);
				ioctl(prefs.iSndFile, KIOCSOUND, 0);
				}
			++optind;
			}
		}

	return 0;
}
