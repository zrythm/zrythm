/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
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
 * Custom types.
 */

#ifndef __UTILS_TYPES_H__
#define __UTILS_TYPES_H__

#include <stdint.h>

/**
 * @addtogroup utils
 *
 * @{
 */

/** MIDI byte. */
typedef uint8_t midi_byte_t;

/** Frame count. */
typedef uint32_t nframes_t;

/** Sample rate. */
typedef uint32_t sample_rate_t;

/** MIDI time in global frames. */
typedef uint32_t midi_time_t;

/** Number of channels. */
typedef unsigned int channels_t;

/** The sample type. */
typedef float sample_t;

/** The BPM type. */
typedef float bpm_t;

typedef double curviness_t;

/**
 * Getter prototype for float values.
 */
typedef float (*GenericFloatGetter) (
  void * object);

/**
 * Setter prototype for float values.
 */
typedef void (*GenericFloatSetter) (
  void * object,
  float  val);

/**
 * Getter prototype for strings.
 */
typedef const char * (*GenericStringGetter) (
  void * object);

/**
 * Setter prototype for float values.
 */
typedef void (*GenericStringSetter) (
  void * object,
  char * val);

typedef enum AudioValueFormat
{
  /** 0 to 2, amplitude. */
  AUDIO_VALUE_AMPLITUDE,

  /** dbFS. */
  AUDIO_VALUE_DBFS,

  /** 0 to 1, suitable for drawing. */
  AUDIO_VALUE_FADER,
} AudioValueFormat;

/**
 * @}
 */

#endif
