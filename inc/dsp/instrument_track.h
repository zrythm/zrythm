// SPDX-FileCopyrightText: © 2018-2020 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Instrument track backend.
 */

#ifndef __AUDIO_INSTRUMENT_TRACK_H__
#define __AUDIO_INSTRUMENT_TRACK_H__

typedef struct Track Track;
typedef struct Plugin     Plugin;

/**
 * @addtogroup dsp
 *
 * @{
 */

/**
 * Initializes an instrument track.
 */
void
instrument_track_init (Track * track);

void
instrument_track_setup (Track * self);

Plugin *
instrument_track_get_instrument (Track * self);

/**
 * Returns if the first plugin's UI in the
 * instrument track is visible.
 */
int
instrument_track_is_plugin_visible (Track * self);

/**
 * Toggles whether the first plugin's UI in the
 * instrument Track is visible.
 */
void
instrument_track_toggle_plugin_visible (Track * self);

/**
 * @}
 */

#endif // __AUDIO_INSTRUMENT_TRACK_H__
