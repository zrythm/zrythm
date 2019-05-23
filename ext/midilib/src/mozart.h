/*
 * Mozart.h - Header file for the Mozart Dice program
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
#ifndef _MOZART_DICE_H
#define _MOZART_DICE_H

#define MAX_MEASURES		32

/*
** Structures
*/
typedef struct	{
			int		*pTable;
			
			int		iBarMeasures;
			int		iNumDice;
			int		iSidesOnDie;

			char	szFilePattern[64];
			} MOZART_TABLE;

typedef struct	{
			int		iCurrentPattern[MAX_MEASURES];
			} MOZART_PREFS;


/*
** Externals
*/
extern MOZART_TABLE g_mMinuet, g_mTrio;

/*
** Mozart Prototypes
*/
void mozartInit(void);
void mozartRandomize(MOZART_PREFS *prefs, const MOZART_TABLE *pTable);
BOOL mozartCreateMidi(const char *pFilename, MOZART_TABLE *pTable, MOZART_PREFS *prefs, BOOL bOverwrite);


#endif	/* _MOZART_DICE_H */
