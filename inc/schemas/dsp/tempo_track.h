// SPDX-FileCopyrightText: © 2019-2020 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Object to hold information for the Tempo track.
 */

#ifndef __SCHEMAS_AUDIO_TEMPO_TRACK_H__
#define __SCHEMAS_AUDIO_TEMPO_TRACK_H__

typedef enum BeatUnit_v1
{
  BEAT_UNIT_2_v1,
  BEAT_UNIT_4_v1,
  BEAT_UNIT_8_v1,
  BEAT_UNIT_16_v1
} BeatUnit_v1;

static const cyaml_strval_t beat_unit_strings_v1[] = {
  { "2",  BEAT_UNIT_2_v1  },
  { "4",  BEAT_UNIT_4_v1  },
  { "8",  BEAT_UNIT_8_v1  },
  { "16", BEAT_UNIT_16_v1 },
};

#endif
