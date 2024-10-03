// SPDX-FileCopyrightText: © 2019-2020 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Object to hold information for the Tempo track.
 */

#ifndef __AUDIO_TEMPO_TRACK_H__
#define __AUDIO_TEMPO_TRACK_H__

#include <stdint.h>

#include "dsp/track.h"
#include "utils/types.h"

/**
 * @addtogroup dsp
 *
 * @{
 */

#define TEMPO_TRACK_MAX_BPM 420.f
#define TEMPO_TRACK_MIN_BPM 40.f
#define TEMPO_TRACK_DEFAULT_BPM 140.f
#define TEMPO_TRACK_DEFAULT_BEATS_PER_BAR 4
#define TEMPO_TRACK_MIN_BEATS_PER_BAR 1
#define TEMPO_TRACK_MAX_BEATS_PER_BAR 16
#define TEMPO_TRACK_DEFAULT_BEAT_UNIT BEAT_UNIT_4
#define TEMPO_TRACK_MIN_BEAT_UNIT BEAT_UNIT_2
#define TEMPO_TRACK_MAX_BEAT_UNIT BEAT_UNIT_16

#define P_TEMPO_TRACK (TRACKLIST->tempo_track)

/**
 * Beat unit.
 */
typedef enum BeatUnit
{
  BEAT_UNIT_2,
  BEAT_UNIT_4,
  BEAT_UNIT_8,
  BEAT_UNIT_16
} BeatUnit;

static const cyaml_strval_t beat_unit_strings[] = {
  { "2",  BEAT_UNIT_2  },
  { "4",  BEAT_UNIT_4  },
  { "8",  BEAT_UNIT_8  },
  { "16", BEAT_UNIT_16 },
};

/**
 * Creates the default tempo track.
 */
Track *
tempo_track_default (int track_pos);

/**
 * Inits the tempo track.
 */
void
tempo_track_init (Track * track);

/**
 * Removes all objects from the tempo track.
 *
 * Mainly used in testing.
 */
void
tempo_track_clear (Track * self);

/**
 * Returns the BPM at the given pos.
 */
bpm_t
tempo_track_get_bpm_at_pos (Track * track, Position * pos);

/**
 * Returns the current BPM.
 */
bpm_t
tempo_track_get_current_bpm (Track * self);

const char *
tempo_track_get_current_bpm_as_str (void * self);

/**
 * Sets the BPM.
 *
 * @param update_snap_points Whether to update the
 *   snap points.
 * @param stretch_audio_region Whether to stretch
 *   audio regions. This should only be true when
 *   the BPM change is final.
 * @param start_bpm The BPM at the start of the
 *   action, if not temporary.
 */
void
tempo_track_set_bpm (
  Track * self,
  bpm_t   bpm,
  bpm_t   start_bpm,
  bool    temporary,
  bool    fire_events);

void
tempo_track_set_bpm_from_str (void * _self, const char * str);

int
tempo_track_beat_unit_enum_to_int (BeatUnit ebeat_unit);

void
tempo_track_set_beat_unit_from_enum (Track * self, BeatUnit ebeat_unit);

BeatUnit
tempo_track_get_beat_unit_enum (Track * self);

BeatUnit
tempo_track_beat_unit_to_enum (int beat_unit);

void
tempo_track_set_beat_unit (Track * self, int beat_unit);

/**
 * Updates beat unit and anything depending on it.
 */
void
tempo_track_set_beats_per_bar (Track * self, int beats_per_bar);

int
tempo_track_get_beats_per_bar (Track * self);

int
tempo_track_get_beat_unit (Track * self);

/**
 * @}
 */

#endif
