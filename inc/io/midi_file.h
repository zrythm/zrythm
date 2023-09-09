// SPDX-FileCopyrightText: © 2020-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * MIDI file IO and utils.
 */

#ifndef __IO_MIDI_FILE_H__
#define __IO_MIDI_FILE_H__

#include <stdbool.h>

/**
 * @addtogroup io
 *
 * @{
 */

/**
 * Returns whether the given track in the midi file has data.
 */
bool
midi_file_track_has_data (const char * abs_path, int track_idx);

/**
 * Returns the number of tracks in the MIDI file.
 */
int
midi_file_get_num_tracks (const char * abs_path, bool non_empty_only);

/**
 * @}
 */

#endif
