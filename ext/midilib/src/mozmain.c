/*
 * mozmain.c - Wrapper code to produce random music through the
 *         'Mozart Dice' algorithm
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
#include <time.h>	/* only used to seed random number generator */
#include "midifile.h"
#include "mozart.h"
#include "midiutil.h"

static MOZART_PREFS mozRules;

int main(int argc, char* argv[])
{
	srand(time(NULL));

	mozartInit();

	mozartRandomize(&mozRules, &g_mMinuet);
	mozartCreateMidi("mozart-minuet.mid", &g_mMinuet, &mozRules, TRUE);
	
	mozartRandomize(&mozRules, &g_mTrio);
	mozartCreateMidi("mozart-trio.mid", &g_mTrio, &mozRules, TRUE);

	return 0;
}
