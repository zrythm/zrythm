/*
 * mididump.c - A complete textual dump of a MIDI file.
 *				Requires Steevs MIDI Library & Utilities
 *				as it demonstrates the text name resolution code.
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
#include "midiutil.h"

void HexList(BYTE *pData, int iNumBytes)
{
int i;

	for(i=0;i<iNumBytes;i++)
		printf("%.2x ", pData[i]);
}

void DumpEventList(const char *pFilename)
{
MIDI_FILE *mf = midiFileOpen(pFilename);
char str[128];
int ev;

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
				printf(" %.6ld ", msg.dwAbsPos);
				if (msg.bImpliedMsg)
					{ ev = msg.iImpliedMsg; }
				else
					{ ev = msg.iType; }

				if (muGetMIDIMsgName(str, ev))	printf("%s\t", str);
				switch(ev)
					{
					case	msgNoteOff:
							muGetNameFromNote(str, msg.MsgData.NoteOff.iNote);
							printf("(%.2d) %s", msg.MsgData.NoteOff.iChannel, str);
							break;
					case	msgNoteOn:
							muGetNameFromNote(str, msg.MsgData.NoteOn.iNote);
							printf("\t(%.2d) %s %d", msg.MsgData.NoteOn.iChannel, str, msg.MsgData.NoteOn.iVolume);
							break;
					case	msgNoteKeyPressure:
							muGetNameFromNote(str, msg.MsgData.NoteKeyPressure.iNote);
							printf("(%.2d) %s %d", msg.MsgData.NoteKeyPressure.iChannel, 
									str,
									msg.MsgData.NoteKeyPressure.iPressure);
							break;
					case	msgSetParameter:
							muGetControlName(str, msg.MsgData.NoteParameter.iControl);
							printf("(%.2d) %s -> %d", msg.MsgData.NoteParameter.iChannel, 
									str, msg.MsgData.NoteParameter.iParam);
							break;
					case	msgSetProgram:
							muGetInstrumentName(str, msg.MsgData.ChangeProgram.iProgram);
							printf("(%.2d) %s", msg.MsgData.ChangeProgram.iChannel, str);
							break;
					case	msgChangePressure:
							muGetControlName(str, msg.MsgData.ChangePressure.iPressure);
							printf("(%.2d) %s", msg.MsgData.ChangePressure.iChannel, str);
							break;
					case	msgSetPitchWheel:
							printf("(%.2d) %d", msg.MsgData.PitchWheel.iChannel,  
									msg.MsgData.PitchWheel.iPitch);
							break;

					case	msgMetaEvent:
							printf("---- ");
							switch(msg.MsgData.MetaEvent.iType)
								{
								case	metaMIDIPort:
										printf("MIDI Port = %d", msg.MsgData.MetaEvent.Data.iMIDIPort);
										break;

								case	metaSequenceNumber:
										printf("Sequence Number = %d",msg.MsgData.MetaEvent.Data.iSequenceNumber);
										break;

								case	metaTextEvent:
										printf("Text = '%s'",msg.MsgData.MetaEvent.Data.Text.pData);
										break;
								case	metaCopyright:
										printf("Copyright = '%s'",msg.MsgData.MetaEvent.Data.Text.pData);
										break;
								case	metaTrackName:
										printf("Track name = '%s'",msg.MsgData.MetaEvent.Data.Text.pData);
										break;
								case	metaInstrument:
										printf("Instrument = '%s'",msg.MsgData.MetaEvent.Data.Text.pData);
										break;
								case	metaLyric:
										printf("Lyric = '%s'",msg.MsgData.MetaEvent.Data.Text.pData);
										break;
								case	metaMarker:
										printf("Marker = '%s'",msg.MsgData.MetaEvent.Data.Text.pData);
										break;
								case	metaCuePoint:
										printf("Cue point = '%s'",msg.MsgData.MetaEvent.Data.Text.pData);
										break;
								case	metaEndSequence:
										printf("End Sequence");
										break;
								case	metaSetTempo:
										printf("Tempo = %d",msg.MsgData.MetaEvent.Data.Tempo.iBPM);
										break;
								case	metaSMPTEOffset:
										printf("SMPTE offset = %d:%d:%d.%d %d",
												msg.MsgData.MetaEvent.Data.SMPTE.iHours,
												msg.MsgData.MetaEvent.Data.SMPTE.iMins,
												msg.MsgData.MetaEvent.Data.SMPTE.iSecs,
												msg.MsgData.MetaEvent.Data.SMPTE.iFrames,
												msg.MsgData.MetaEvent.Data.SMPTE.iFF
												);
										break;
								case	metaTimeSig:
										printf("Time sig = %d/%d",msg.MsgData.MetaEvent.Data.TimeSig.iNom,
													msg.MsgData.MetaEvent.Data.TimeSig.iDenom/MIDI_NOTE_CROCHET);
										break;
								case	metaKeySig:
										if (muGetKeySigName(str, msg.MsgData.MetaEvent.Data.KeySig.iKey))
											printf("Key sig = %s", str);
										break;

								case	metaSequencerSpecific:
										printf("Sequencer specific = ");
										HexList(msg.MsgData.MetaEvent.Data.Sequencer.pData, msg.MsgData.MetaEvent.Data.Sequencer.iSize);
										break;
								}
							break;

					case	msgSysEx1:
					case	msgSysEx2:
							printf("Sysex = ");
							HexList(msg.MsgData.SysEx.pData, msg.MsgData.SysEx.iSize);
							break;
					}

				if (ev == msgSysEx1 || ev == msgSysEx1 || (ev==msgMetaEvent && msg.MsgData.MetaEvent.iType==metaSequencerSpecific))
					{
					/* Already done a hex dump */
					}
				else
					{
					printf("\t[");
					if (msg.bImpliedMsg) printf("%.2x!", msg.iImpliedMsg);
					for(j=0;j<msg.iMsgSize;j++)
						printf("%.2x ", msg.data[j]);
					printf("]\n");
					}
				}
			}

		midiReadFreeMessage(&msg);
		midiFileClose(mf);
		}
}

int main(int argc, char* argv[])
{
int i=0;

	if (argc==1)
		{
		printf("Usage: %s <filename>\n", argv[0]);
		}
	else
		{
		for(i=1;i<argc;++i)
			DumpEventList(argv[i]);
		}

	return 0;
}
