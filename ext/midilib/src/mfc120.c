/*
 * mfc120.c - Processing code to convert MIDI files into format 1, to
 *				format 0. (from 1-to-0, geddit!?)
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

BOOL midi_convert120(const char *dest, const char *src, BOOL bOverwrite)
{
MIDI_FILE *pInFile;
MIDI_FILE *pOutFile;
MIDI_MSG msg[MAX_MIDI_TRACKS];
BYTE bValidChannels[MAX_MIDI_TRACKS];
BYTE bReadableChannels[MAX_MIDI_TRACKS];
int l, tracks_left_to_process;
DWORD src_song_pos;
BOOL bWasLastBestMsgPriority;		/* ie sysex or meta-event */
int curr_best_track=0; /* paranoia - shouldn't get used unless flush; which needs one track at least */
DWORD curr_lowest_dt, dt;
int pqn = 384;

	for(l=0;l<MAX_MIDI_TRACKS;l++)
		midiReadInitMessage(&msg[l]);

	if ((pInFile = midiFileOpen(src)))
		{
		if (midiFileGetVersion(pInFile) > 1)
			{
			fprintf(stderr, "Error: %s is in format 2. MIDIConvert 120 does not currently support this conversion.\n", src);
			}
		else 
			{
			if ((pOutFile = midiFileCreate(dest, bOverwrite)))
				{
				/* Alogrithm:
				**		Scan all tracks to find the msg with the lowest 'dt' (use META/SYSEX events first in case of tie-break)
				**		Write msg out. Update song pos
				**		Prepare to refill the last best track with more data
				**
				** lowest 'dt' should NOT be used: compare song_pos, and msg's own track pos
				** 'bReadableChannels' are those where data should be taken from to form the next line of possible 'next lowest dt' candidates
				*/
				memset(bValidChannels,'\1', sizeof(bValidChannels));
				memset(bReadableChannels,'\1', sizeof(bReadableChannels));
				tracks_left_to_process = MAX_MIDI_TRACKS;
				src_song_pos = 0;
				/**/ 
				do
					{
					curr_lowest_dt = 0xffffffff;
					bWasLastBestMsgPriority = FALSE;
					/**/
					for(l=0;l<MAX_MIDI_TRACKS;l++)		
						if (bValidChannels[l])
							{
							if (bReadableChannels[l])	/* should we read another msg from the queue? */
								{
								if (midiReadGetNextMessage(pInFile, l, &msg[l]))
									{
									bReadableChannels[l] = FALSE;
									}
								else	/* can not read data on this track!*/
									{
									bValidChannels[l] = FALSE;
									tracks_left_to_process--;
									}
								}
							/**/
							dt = msg[l].dwAbsPos - src_song_pos;
							if (bValidChannels[l] && dt <= curr_lowest_dt)	/* <= too permit priority testing */
								{
								/* We add if = , and this is a priority, or the time is better:*/
								if ((dt == curr_lowest_dt && (msg[l].iType & msgSysMask)) || dt<curr_lowest_dt)
									{
									curr_lowest_dt = msg[l].dwAbsPos - src_song_pos;
									curr_best_track = l;
									/**/
									if (msg[l].iType & msgSysMask)
										bWasLastBestMsgPriority = TRUE;
									}
								}
							}
					/**/ 
					if (curr_lowest_dt == 0xffffffff)		/* we must have exhuasted all tracks of msgs!*/
						{
						tracks_left_to_process = 0;			/* Double check - this should be 0 anyway*/
						}
					else 
						{
						if (curr_lowest_dt)
							dt = 0;

						dt = (curr_lowest_dt*pqn)/midiFileGetPPQN(pInFile);
						src_song_pos += curr_lowest_dt;
						/**/
						if (msg[curr_best_track].iType == msgMetaEvent && msg[curr_best_track].MsgData.MetaEvent.iType == metaEndSequence)
							;
						else	/* write best msg out, and flag it to get new data */
							midiTrackAddRaw(pOutFile, 0, msg[curr_best_track].iMsgSize, msg[curr_best_track].data, TRUE, dt);
						/**/
						bReadableChannels[curr_best_track] = TRUE;
						}
					/**/
					}
				while(tracks_left_to_process);
				/**/
				midiSongAddEndSequence(pOutFile, 0);
				midiFileClose(pOutFile);
				/**/
				fprintf(stderr, "Success! %s converted sucessfully.\n", src);
				}
			else	/* can not open the dest file */
				{
				fprintf(stderr, "Error: %s can not be written to. Please check that the file does not already exist, or is read only\n", dest);
				}
			}
		
		free(pInFile);
		}
	else
		{
		fprintf(stderr, "Error: %s does not exist.\n", src);
		}
	/**/
	for(l=0;l<MAX_MIDI_TRACKS;l++)
		midiReadFreeMessage(&msg[l]);
	/**/
	return TRUE;

}
