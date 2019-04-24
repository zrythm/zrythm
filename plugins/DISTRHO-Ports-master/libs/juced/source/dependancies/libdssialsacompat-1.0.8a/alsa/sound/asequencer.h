/* DSSI ALSA compatibility library
 *
 * This library provides for Mac OS X the ALSA snd_seq_event_t handling
 * necessary to compile and run DSSI.  It was extracted from alsa-lib 1.0.8.
 */

/*
 *  Main header file for the ALSA sequencer
 *  Copyright (c) 1998-1999 by Frank van de Pol <fvdpol@coil.demon.nl>
 *            (c) 1998-1999 by Jaroslav Kysela <perex@suse.cz>
 *
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */
#ifndef __SOUND_ASEQUENCER_H
#define __SOUND_ASEQUENCER_H

#define SNDRV_SEQ_EVENT_SYSEX		130	/* system exclusive data (variable length) */

#define SNDRV_SEQ_EVENT_LENGTH_FIXED	(0<<2)	/* fixed event size */
#define SNDRV_SEQ_EVENT_LENGTH_VARIABLE	(1<<2)	/* variable event size */

#define SNDRV_SEQ_EVENT_LENGTH_MASK	(3<<2)

#endif /* __SOUND_ASEQUENCER_H */
