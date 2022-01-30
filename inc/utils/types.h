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

#include <stdbool.h>
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

/** Signed type for frame index. */
typedef int_fast64_t signed_frame_t;

/** Unsigned type for frame index. */
typedef uint_fast64_t unsigned_frame_t;

/** Signed millisecond index. */
typedef signed_frame_t signed_ms_t;

/** Signed second index. */
typedef signed_frame_t signed_sec_t;

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
 * Getter prototype for strings to be saved in the
 * given buffer.
 */
typedef void (*GenericStringCopyGetter) (
  void * object,
  char * buf);

/**
 * Setter prototype for float values.
 */
typedef void (*GenericStringSetter) (
  void *       object,
  const char * val);

/**
 * Generic callback.
 */
typedef void (*GenericCallback) (
  void *       object);

/**
 * Generic comparator.
 */
typedef int (*GenericCmpFunc) (
  const void * a,
  const void * b);

/**
 * Predicate function prototype.
 *
 * To be used to return whether the given pointer
 * matches some condition.
 */
typedef bool (*GenericPredicateFunc) (
  const void * object,
  const void * user_data);

/**
 * Generic progress info.
 */
typedef struct GenericProgressInfo
{
  /** Progress done (0.0 to 1.0). */
  double            progress;

  /** Action cancelled. */
  bool              cancelled;

  /** Error occurred. */
  bool              has_error;

  /** String to show in the label during the
   * action. */
  char              label_str[1800];

  /** String to show in the label when the action
   * is complete (progress == 1.0). */
  char              label_done_str[1800];

  /**
   * String to show in a popup when
   * GenericProgressInfo.has_error is true.
   */
  char              error_str[1800];
} GenericProgressInfo;

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
 * Common struct to pass around during processing
 * to avoid repeating the data in function
 * arguments.
 */
typedef struct EngineProcessTimeInfo
{
  /** Global position at the start of the
   * processing cycle. */
  /* FIXME in some places this is adjusted to the
   * local offset. it should be adjusted to the
   * local offset everywhere */
  unsigned_frame_t g_start_frame;

  /** Offset in the current processing cycle,
   * between 0 and the number of frames in
   * AudioEngine.block_length. */
  nframes_t        local_offset;

  /**
   * Number of frames to process in this call.
   */
  nframes_t        nframes;
} EngineProcessTimeInfo;

/**
 * @}
 */

#endif
