// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Custom types.
 */

#ifndef __UTILS_TYPES_H__
#define __UTILS_TYPES_H__

#include <cinttypes>
#include <cstdint>

#include "gtk_wrapper.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#include <magic_enum_all.hpp>
#pragma GCC diagnostic pop

using namespace magic_enum::bitwise_operators;

/**
 * @addtogroup utils
 *
 * @{
 */

#define TYPEDEF_STRUCT(s) typedef struct s s

#define TYPEDEF_STRUCT_UNDERSCORED(s) typedef struct _##s s

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
typedef void (*GenericStringSetter) (void * object, const char * val);

/**
 * Getter prototype for strings to be saved in the
 * given buffer.
 */
typedef void (*GenericStringCopyGetter) (void * object, char * buf);

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
typedef bool (
  *GenericPredicateFunc) (const void * object, const void * user_data);

/**
 * Function to call to free objects.
 */
typedef void (*ObjectFreeFunc) (void *);

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
  /** Global position at the start of the processing cycle (no offset added). */
  unsigned_frame_t g_start_frame;

  /** Global position with EngineProcessTimeInfo.local_offset added, for
   * convenience. */
  unsigned_frame_t g_start_frame_w_offset;

  /** Offset in the current processing cycle, between 0 and the number of frames
   * in AudioEngine.block_length. */
  nframes_t local_offset;

  /**
   * Number of frames to process in this call, starting from the offset.
   */
  nframes_t nframes;
} EngineProcessTimeInfo;

static inline void
engine_process_time_info_print (const EngineProcessTimeInfo * self)
{
  g_message (
    "Global start frame: %" PRIuFAST64 " (with offset %" PRIuFAST64
    ") | local offset: %" PRIu32 " | num frames: %" PRIu32,
    self->g_start_frame, self->g_start_frame_w_offset, self->local_offset,
    self->nframes);
}

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
   | CACHE_TYPE_PLAYBACK_SNAPSHOTS | CACHE_TYPE_AUTOMATION_LANE_RECORD_MODES \
   | CACHE_TYPE_AUTOMATION_LANE_PORTS)

#define ENUM_INT_TO_VALUE_CONST(_enum, _int) \
  (magic_enum::enum_value<_enum, _int> ())
#define ENUM_INT_TO_VALUE(_enum, _int) (magic_enum::enum_value<_enum> (_int))
#define ENUM_VALUE_TO_INT(_val) (magic_enum::enum_integer (_val))

#define ENUM_ENABLE_BITSET(_enum) \
  template <> struct magic_enum::customize::enum_range<_enum> \
  { \
    static constexpr bool is_flags = true; \
  }
#define ENUM_BITSET(_enum, _val) (magic_enum::containers::bitset<_enum> (_val))
#define ENUM_BITSET_TEST(_enum, _val, _other_val) \
  /* (ENUM_BITSET (_enum, _val).test (_other_val)) */ \
  (static_cast<int> (_val) & static_cast<int> (_other_val))

/** @important ENUM_ENABLE_BITSET must be called on the enum that this is used
 * on. */
#define ENUM_BITSET_TO_STRING(_enum, _val) \
  (ENUM_BITSET (_enum, _val).to_string ().data ())

#define ENUM_COUNT(_enum) (magic_enum::enum_count<_enum> ())
#define ENUM_NAME(_val) (magic_enum::enum_name (_val).data ())
#define ENUM_NAME_FROM_INT(_enum, _int) \
  ENUM_NAME (ENUM_INT_TO_VALUE (_enum, _int))

/**
 * @}
 */

#endif
