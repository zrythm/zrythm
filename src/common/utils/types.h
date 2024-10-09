// SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Custom types.
 */

#ifndef __UTILS_TYPES_H__
#define __UTILS_TYPES_H__

#include <chrono>
#include <cinttypes>
#include <cstdint>

#if defined(__GNUC__) || defined(__clang__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wshadow"
#elif defined(_MSC_VER)
#  pragma warning(push)
#  pragma warning(disable : 4458) // declaration hides class member
#endif

#include <magic_enum_all.hpp>

#if defined(__GNUC__) || defined(__clang__)
#  pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#  pragma warning(pop)
#endif

using namespace magic_enum::bitwise_operators;

/**
 * @addtogroup utils
 *
 * @{
 */

#define TYPEDEF_STRUCT(s) typedef struct s s

#define TYPEDEF_STRUCT_UNDERSCORED(s) using s = struct _##s

// gint64
using RtTimePoint = int64_t;
using RtDuration = int64_t;

/** MIDI byte. */
using midi_byte_t = uint8_t;

/** Frame count. */
using nframes_t = uint32_t;

/** Sample rate. */
using sample_rate_t = uint32_t;

/** MIDI time in global frames. */
using midi_time_t = uint32_t;

/** Number of channels. */
using channels_t = uint_fast8_t;

/** The sample type. */
using sample_t = float;

/** The BPM type. */
using bpm_t = float;

using curviness_t = double;

/** Signed type for frame index. */
using signed_frame_t = int_fast64_t;

#define SIGNED_FRAME_FORMAT PRId64

/** Unsigned type for frame index. */
using unsigned_frame_t = uint_fast64_t;

#define UNSIGNED_FRAME_FORMAT PRIu64

/** Signed millisecond index. */
using signed_ms_t = signed_frame_t;

/** Signed second index. */
using signed_sec_t = signed_frame_t;

/**
 * Getter prototype for float values.
 */
using GenericFloatGetter = std::function<float ()>;

/**
 * Setter prototype for float values.
 */
using GenericFloatSetter = std::function<void (float)>;

/**
 * Getter prototype for strings.
 */
using GenericStringGetter = std::function<std::string ()>;

/**
 * Setter prototype for float values.
 */
using GenericStringSetter = std::function<void (const std::string &)>;

/**
 * Generic callback.
 */
using GenericCallback = std::function<void ()>;

/**
 * Generic comparator.
 */
typedef int (*GenericCmpFunc) (const void * a, const void * b);

/**
 * Function to call to free objects.
 */
typedef void (*ObjectFreeFunc) (void *);

// For non-const member functions
template <typename Class, typename Ret, typename... Args, typename ActualClass>
auto
bind_member_function (ActualClass &obj, Ret (Class::*func) (Args...))
  requires std::is_base_of_v<Class, ActualClass>
{
  return [&obj, func] (Args... args) {
    return (obj.*func) (std::forward<Args> (args)...);
  };
}

// For const member functions
template <typename Class, typename Ret, typename... Args, typename ActualClass>
auto
bind_member_function (ActualClass &obj, Ret (Class::*func) (Args...) const)
  requires std::is_base_of_v<Class, ActualClass>
{
  return [&obj, func] (Args... args) {
    return (obj.*func) (std::forward<Args> (args)...);
  };
}

enum class AudioValueFormat
{
  /** 0 to 2, amplitude. */
  Amplitude,

  /** dbFS. */
  DBFS,

  /** 0 to 1, suitable for drawing. */
  Fader,
};

/**
 * Common struct to pass around during processing to avoid repeating the data in
 * function arguments.
 */
struct EngineProcessTimeInfo
{
public:
  void print () const;

public:
  /** Global position at the start of the processing cycle (no offset added). */
  unsigned_frame_t g_start_frame_ = 0;

  /** Global position with EngineProcessTimeInfo.local_offset added, for
   * convenience. */
  unsigned_frame_t g_start_frame_w_offset_ = 0;

  /** Offset in the current processing cycle, between 0 and the number of
   * frames in AudioEngine.block_length. */
  nframes_t local_offset_ = 0;

  /**
   * Number of frames to process in this call, starting from the offset.
   */
  nframes_t nframes_ = 0;
};

/**
 * Beat unit.
 */
enum class BeatUnit
{
  Two,
  Four,
  Eight,
  Sixteen
};

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

#if defined(__clang__)
#  define ASSUME(expr) __builtin_assume (expr)
#elif defined(__GNUC__)
#  define ASSUME(expr) \
    do \
      { \
        if (!(expr)) \
          __builtin_unreachable (); \
      } \
    while (0)
#elif defined(_MSC_VER)
#  define ASSUME(expr) __assume (expr)
#else
#  define ASSUME(expr) \
    do \
      { \
        static_cast<void> (expr); \
      } \
    while (0)
#endif

enum class CacheType
{
  TrackNameHashes = 1 << 0,
  PluginPorts = 1 << 1,
  PlaybackSnapshots = 1 << 2,
  AutomationLaneRecordModes = 1 << 3,
  AutomationLanePorts = 1 << 4,
};

ENUM_ENABLE_BITSET (CacheType);

constexpr CacheType ALL_CACHE_TYPES =
  CacheType::TrackNameHashes | CacheType::PluginPorts
  | CacheType::PlaybackSnapshots | CacheType::AutomationLaneRecordModes
  | CacheType::AutomationLanePorts;

/* types for simple timestamps/durations */
using SteadyClock = std::chrono::steady_clock;
using SteadyTimePoint = SteadyClock::time_point;
using SteadyDuration = SteadyClock::duration;

/* return types for Glib source functions */
constexpr bool SourceFuncContinue = true;
constexpr bool SourceFuncRemove = false;

namespace fs = std::filesystem;

#if __has_attribute(noipa)
#  define KEEP __attribute__ ((used, retain, noipa))
#else
#  define KEEP __attribute__ ((used, retain, noinline))
#endif

#define ZRYTHM_IS_QT_THREAD (QThread::currentThread () == qApp->thread ())

struct _GdkRGBA
{
  double red = 0.0;
  double green = 0.0;
  double blue = 0.0;
  double alpha = 1.0;
};
using GdkRGBA = _GdkRGBA;

/**
 * @}
 */

#endif
