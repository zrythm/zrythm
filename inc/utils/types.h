// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Custom types.
 */

#ifndef __UTILS_TYPES_H__
#define __UTILS_TYPES_H__

#include <stdbool.h>
#include <stdint.h>

#include <gtk/gtk.h>

/**
 * @addtogroup utils
 *
 * @{
 */

#define TYPEDEF_STRUCT(s) typedef struct s s;

#define TYPEDEF_STRUCT_UNDERSCORED(s) typedef struct _##s s;

#define TYPEDEF_ENUM(s) typedef enum s s;

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

#define SIGNED_FRAME_FORMAT PRId64

/** Unsigned type for frame index. */
typedef uint_fast64_t unsigned_frame_t;

#define UNSIGNED_FRAME_FORMAT PRIu64

/** Signed millisecond index. */
typedef signed_frame_t signed_ms_t;

/** Signed second index. */
typedef signed_frame_t signed_sec_t;

/**
 * Getter prototype for float values.
 */
typedef float (*GenericFloatGetter) (void * object);

/**
 * Setter prototype for float values.
 */
typedef void (*GenericFloatSetter) (void * object, float val);

/**
 * Getter prototype for strings.
 */
typedef const char * (*GenericStringGetter) (void * object);

/**
 * Setter prototype for float values.
 */
typedef void (
  *GenericStringSetter) (void * object, const char * val);

/**
 * Getter prototype for strings to be saved in the
 * given buffer.
 */
typedef void (
  *GenericStringCopyGetter) (void * object, char * buf);

/**
 * Generic callback.
 */
typedef void (*GenericCallback) (void * object);

/**
 * Generic comparator.
 */
typedef int (*GenericCmpFunc) (const void * a, const void * b);

/**
 * Predicate function prototype.
 *
 * To be used to return whether the given pointer
 * matches some condition.
 */
typedef bool (*GenericPredicateFunc) (
  const void * object,
  const void * user_data);

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
  nframes_t local_offset;

  /**
   * Number of frames to process in this call.
   */
  nframes_t nframes;
} EngineProcessTimeInfo;

typedef enum CacheTypes
{
  CACHE_TYPE_TRACK_NAME_HASHES = 1 << 0,
  CACHE_TYPE_PLUGIN_PORTS = 1 << 1,
  CACHE_TYPE_PLAYBACK_SNAPSHOTS = 1 << 2,
  CACHE_TYPE_AUTOMATION_LANE_RECORD_MODES = 1 << 3,
  CACHE_TYPE_AUTOMATION_LANE_PORTS = 1 << 4,
} CacheTypes;

#define CACHE_TYPE_ALL \
  (CACHE_TYPE_TRACK_NAME_HASHES | CACHE_TYPE_PLUGIN_PORTS \
   | CACHE_TYPE_PLAYBACK_SNAPSHOTS \
   | CACHE_TYPE_AUTOMATION_LANE_RECORD_MODES \
   | CACHE_TYPE_AUTOMATION_LANE_PORTS)

/**
 * @}
 */

#endif
