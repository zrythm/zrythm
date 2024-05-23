#ifndef _MIDIFILE_H
#define _MIDIFILE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "midiinfo.h"		/* enumerations and constants for GM */

/*
 * midiFile.c -  Header file for Steevs MIDI Library
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

/*
** All functions start with one of the following prefixes:
**		midiFile*		For non-GM features that relate to the file, and have
**						no use once the file has been created, i.e. CreateFile
**						or SetTrack (those data is embedded into the file, but
**						not explicitly stored)
**		midiSong*		For operations that work across the song, i.e. SetTempo
**		midiTrack*		For operations on a specific track, i.e. AddNoteOn
*/

/*
** Types because we're dealing with files, and need to be careful
*/
typedef	unsigned char		BYTE;
typedef	uint16_t 		WORD;
typedef	uint32_t 		_DWORD;
typedef int					BOOL;
#ifndef TRUE
#define TRUE	1
#endif
#ifndef FALSE
#define FALSE	0
#endif


/*
** MIDI Constants
*/
#define MIDI_PPQN_DEFAULT		384
#define MIDI_VERSION_DEFAULT	1

/*
** MIDI Limits
*/
#define MAX_MIDI_TRACKS			256
#define MAX_TRACK_POLYPHONY		64

/*
** MIDI structures, accessibly externably
*/
typedef	void 	MIDI_FILE;
typedef struct {
					tMIDI_MSG	iType;

					_DWORD		dt;		/* delta time */
					_DWORD		dwAbsPos;
					_DWORD		iMsgSize;

					BOOL		bImpliedMsg;
					tMIDI_MSG	iImpliedMsg;

					/* Raw data chunk */
					BYTE *data;		/* dynamic data block */
					_DWORD data_sz;

					union {
						struct {
								int			iNote;
								int			iChannel;
								int			iVolume;
								} NoteOn;
						struct {
								int			iNote;
								int			iChannel;
								} NoteOff;
						struct {
								int			iNote;
								int			iChannel;
								int			iPressure;
								} NoteKeyPressure;
						struct {
								int			iChannel;
								tMIDI_CC	iControl;
								int			iParam;
								} NoteParameter;
						struct {
								int			iChannel;
								int			iProgram;
								} ChangeProgram;
						struct {
								int			iChannel;
								int			iPressure;
								} ChangePressure;
						struct {
								int			iChannel;
								int			iPitch;
								} PitchWheel;
						struct {
								tMIDI_META	iType;
								union {
									int					iMIDIPort;
									int					iSequenceNumber;
									struct {
										BYTE			*pData;
										} Text;
									struct {
										int				iBPM;
										} Tempo;
									struct {
										int				iHours, iMins;
										int				iSecs, iFrames,iFF;
										} SMPTE;
									struct {
										tMIDI_KEYSIG	iKey;
										} KeySig;
									struct {
										int				iNom, iDenom;
										} TimeSig;
									struct {
										BYTE			*pData;
										int				iSize;
										} Sequencer;
									} Data;
								} MetaEvent;
						struct {
								BYTE		*pData;
								int			iSize;
								} SysEx;
						} MsgData;

				/* State information - Please treat these as private*/
				tMIDI_MSG	iLastMsgType;
				BYTE		iLastMsgChnl;

				} MIDI_MSG;

/*
** midiFile* Prototypes
*/
MIDI_FILE  *midiFileCreate(const char *pFilename, BOOL bOverwriteIfExists);
int			midiFileSetTracksDefaultChannel(MIDI_FILE *pMF, int iTrack, int iChannel);
int			midiFileGetTracksDefaultChannel(const MIDI_FILE *pMF, int iTrack);
BOOL		midiFileFlushTrack(MIDI_FILE *pMF, int iTrack, BOOL bFlushToEnd, _DWORD dwEndTimePos);
BOOL		midiFileSyncTracks(MIDI_FILE *pMF, int iTrack1, int iTrack2);
int			midiFileSetPPQN(MIDI_FILE *pMF, int PPQN);
int			midiFileGetPPQN(const MIDI_FILE *pMF);
int			midiFileSetVersion(MIDI_FILE *pMF, int iVersion);
int			midiFileGetVersion(const MIDI_FILE *pMF);
MIDI_FILE  *midiFileOpen(const char *pFilename);
BOOL		midiFileClose(MIDI_FILE *pMF);

/*
** midiSong* Prototypes
*/
BOOL		midiSongAddSMPTEOffset(MIDI_FILE *pMF, int iTrack, int iHours, int iMins, int iSecs, int iFrames, int iFFrames);
BOOL		midiSongAddSimpleTimeSig(MIDI_FILE *pMF, int iTrack, int iNom, int iDenom);
BOOL		midiSongAddTimeSig(MIDI_FILE *pMF, int iTrack, int iNom, int iDenom, int iClockInMetroTick, int iNotated32nds);
BOOL		midiSongAddKeySig(MIDI_FILE *pMF, int iTrack, tMIDI_KEYSIG iKey);
BOOL		midiSongAddTempo(MIDI_FILE *pMF, int iTrack, int iTempo);
BOOL		midiSongAddMIDIPort(MIDI_FILE *pMF, int iTrack, int iPort);
BOOL		midiSongAddEndSequence(MIDI_FILE *pMF, int iTrack);

/*
** midiTrack* Prototypes
*/
BOOL		midiTrackAddRaw(MIDI_FILE *pMF, int iTrack, int iDataSize, const BYTE *pData, BOOL bMovePtr, int iDeltaTime);
BOOL		midiTrackIncTime(MIDI_FILE *pMF, int iTrack, int iDeltaTime, BOOL bOverridePPQN);
BOOL		midiTrackAddText(MIDI_FILE *pMF, int iTrack, tMIDI_TEXT iType, const char *pTxt);
BOOL		midiTrackAddMsg(MIDI_FILE *pMF, int iTrack, tMIDI_MSG iMsg, int iParam1, int iParam2);
BOOL		midiTrackSetKeyPressure(MIDI_FILE *pMF, int iTrack, int iNote, int iAftertouch);
BOOL		midiTrackAddControlChange(MIDI_FILE *pMF, int iTrack, tMIDI_CC iCCType, int iParam);
BOOL		midiTrackAddProgramChange(MIDI_FILE *pMF, int iTrack, int iInstrPatch);
BOOL		midiTrackChangeKeyPressure(MIDI_FILE *pMF, int iTrack, int iDeltaPressure);
BOOL		midiTrackSetPitchWheel(MIDI_FILE *pMF, int iTrack, int iWheelPos);
BOOL		midiTrackAddNote(MIDI_FILE *pMF, int iTrack, int iNote, int iLength, int iVol, BOOL bAutoInc, BOOL bOverrideLength);
BOOL		midiTrackAddRest(MIDI_FILE *pMF, int iTrack, int iLength, BOOL bOverridePPQN);
BOOL		midiTrackGetEndPos(MIDI_FILE *pMF, int iTrack);

/*
** midiRead* Prototypes
*/
int			midiReadGetNumTracks(const MIDI_FILE *pMF);
BOOL		midiReadGetNextMessage(const MIDI_FILE *pMF, int iTrack, MIDI_MSG *pMsg);
void		midiReadInitMessage(MIDI_MSG *pMsg);
void		midiReadFreeMessage(MIDI_MSG *pMsg);

#ifdef __cplusplus
}
#endif

#endif /* _MIDIFILE_H */
