/*
 * Copyright (C) 2020-2021 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * MIDI file utils.
 */

#ifndef __AUDIO_MIDI_FILE_H__
#define __AUDIO_MIDI_FILE_H__

#include <stdbool.h>

#include "ext/midilib/src/midifile.h"

/**
 * @addtogroup dsp
 *
 * @{
 */

/**
 * Returns whether the given track in the midi file
 * has data.
 */
bool
midi_file_track_has_data (const char * abs_path, int track_idx);

/**
 * Returns the number of tracks in the MIDI file.
 */
int
midi_file_get_num_tracks (
  const char * abs_path,
  bool         non_empty_only);

/**
 * @}
 */

#endif
