/*
 * inc/utils/selections.h - Selections
 *
 * Copyright (C) 2018 Alexandros Theodotou
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __UTILS_SELECTIONS_H__
#define __UTILS_SELECTIONS_H__

typedef struct Region Region;
typedef struct Channel Channel;
typedef struct MidiNote MidiNote;

/**
 * Stores current Selections.
 */
typedef struct Selections
{
  Region          * regions[200]; ///< currently selected regions
  int             num_regions; ///< selected region count
  Channel         * channels[200];
  int             num_channels;
  MidiNote        * midi_notes[400];
  int             num_midi_notes;
} Selections;

/**
 * Returns 1 if channel is currently selected
 */
int
selections_is_channel_selected (Channel * channel);

#endif /* __UTILS_SELECTIONS_H__ */


