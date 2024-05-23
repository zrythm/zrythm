/*
 * midiutil.h - Header for auxiliary MIDI functionality.
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

#ifdef __cplusplus
extern "C" {
#endif

#include "midiinfo.h"


#ifndef _MIDIUTIL_H
#define _MIDIUTIL_H


#define CHORD_ROOT_MASK		0x000000ff
#define CHORD_TYPE_MASK		0x0000ff00
#define CHORD_BASS_MASK		0x00ff0000
#define CHORD_ADDITION_MASK	0xff000000

#define CHORD_TYPE_MAJOR	0x00000100
#define CHORD_TYPE_MINOR	0x00000200
#define CHORD_TYPE_AUG		0x00000300
#define CHORD_TYPE_DIM		0x00000400

#define CHORD_ADD_7TH		0x01000000
#define CHORD_ADD_9TH		0x02000000
#define CHORD_ADD_MAJ7TH	0x04000000


/*
** Name resolving prototypes
*/
BOOL	muGetInstrumentName(char *pName, int iInstr);
BOOL	muGetDrumName(char *pName, int iInstr);
BOOL	muGetMIDIMsgName(char *pName, tMIDI_MSG iMsg);
BOOL	muGetControlName(char *pName, tMIDI_CC iCC);
BOOL	muGetKeySigName(char *pName, tMIDI_KEYSIG iKey);
BOOL	muGetTextName(char *pName, tMIDI_TEXT iEvent);
BOOL	muGetMetaName(char *pName, tMIDI_META iEvent);

/*
** Conversion prototypes
*/
int		muGetNoteFromName(const char *pName);
char	*muGetNameFromNote(char *pStr, int iNote);
float	muGetFreqFromNote(int iNote);
int		muGetNoteFromFreq(float fFreq);

int muGuessChord(const int *pNoteStatus, const int channel, const int lowRange, const int highRange);
char *muGetChordName(char *str, int chord);

#ifdef __cplusplus
}
#endif

#endif	/* _MIDIUTIL_H*/
