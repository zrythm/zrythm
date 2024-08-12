// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Custom types.
 */

#ifndef __UTILS_TYPES_H__
#define __UTILS_TYPES_H__

#include <algorithm>
#include <atomic>
#include <cinttypes>
#include <concepts>
#include <cstdint>
#include <memory>
#include <ranges>
#include <semaphore>
#include <type_traits>
#include <variant>

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

#define TYPEDEF_STRUCT_UNDERSCORED(s) using s = struct _##s

using RtTimePoint = gint64;
using RtDuration = gint64;

/** MIDI byte. */
using midi_byte_t = uint8_t;

/** Frame count. */
using nframes_t = uint32_t;

/** Sample rate. */
using sample_rate_t = uint32_t;

/** MIDI time in global frames. */
using midi_time_t = uint32_t;

/** Number of channels. */
using channels_t = unsigned int;

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
using GenericFloatGetter = std::function<float (void *)>;

/**
 * Setter prototype for float values.
 */
using GenericFloatSetter = std::function<void (void *, float)>;

/**
 * Getter prototype for strings.
 */
using GenericStringGetter = std::function<std::string (void *)>;

/**
 * Setter prototype for float values.
 */
using GenericStringSetter = std::function<void (void *, const std::string &)>;

/**
 * Getter prototype for strings to be saved in the
 * given buffer.
 */
// typedef void (*GenericStringCopyGetter) (void * object, char * buf);

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
  inline void print () const
  {
    g_message (
      "Global start frame: %" PRIuFAST64 " (with offset %" PRIuFAST64
      ") | local offset: %" PRIu32 " | num frames: %" PRIu32,
      g_start_frame_, g_start_frame_w_offset_, local_offset_, nframes_);
  }

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

// Concept that checks if a type is a final class
template <typename T>
concept FinalClass = std::is_final_v<T> && std::is_class_v<T>;

template <typename T> concept StdArray = requires
{
  typename std::array<typename T::value_type, std::tuple_size<T>::value>;
  requires std::same_as<
    T, std::array<typename T::value_type, std::tuple_size<T>::value>>;
};

static_assert (StdArray<std::array<unsigned char, 3>>);
static_assert (!StdArray<std::vector<float>>);

// Primary template
template <typename T> struct remove_smart_pointer
{
  using type = T;
};

// Specialization for std::unique_ptr
template <typename T, typename Deleter>
struct remove_smart_pointer<std::unique_ptr<T, Deleter>>
{
  using type = T;
};

// Specialization for std::shared_ptr
template <typename T> struct remove_smart_pointer<std::shared_ptr<T>>
{
  using type = T;
};

// Specialization for std::weak_ptr
template <typename T> struct remove_smart_pointer<std::weak_ptr<T>>
{
  using type = T;
};

// Helper alias template (C++11 and later)
template <typename T>
using remove_smart_pointer_t = typename remove_smart_pointer<T>::type;

template <typename T>
using base_type = remove_smart_pointer_t<std::remove_pointer_t<std::decay_t<T>>>;

template <typename T> struct is_unique_ptr : std::false_type
{
};
template <typename T> struct is_unique_ptr<std::unique_ptr<T>> : std::true_type
{
};
template <typename T> constexpr bool is_unique_ptr_v = is_unique_ptr<T>::value;

static_assert (is_unique_ptr_v<std::unique_ptr<EngineProcessTimeInfo>>);
static_assert (!is_unique_ptr_v<std::shared_ptr<EngineProcessTimeInfo>>);

template <typename T> struct is_shared_ptr : std::false_type
{
};
template <typename T> struct is_shared_ptr<std::shared_ptr<T>> : std::true_type
{
};
template <typename T> constexpr bool is_shared_ptr_v = is_shared_ptr<T>::value;

template <typename T>
concept SmartPtr = is_unique_ptr_v<T> || is_shared_ptr_v<T>;

/**
 * @brief A type trait that checks if a type is the same as or a pointer to a
 * specified type.
 *
 * @tparam T The type to compare against.
 */
template <typename T> struct type_is
{
  /**
   * @brief Function call operator that checks if the given type is the same as
   * or a pointer to T.
   *
   * @tparam U The type to check.
   * @param u An instance of the type to check (unused).
   * @return true if U is the same as T, std::unique_ptr<T>, or T*; false
   * otherwise.
   */
  template <typename U> bool operator() (const U &u) const;
};

/**
 * @brief Overload of the pipe operator (|) to filter elements of a specific
 * type from a range.
 *
 * This overload works with ranges of std::unique_ptr and raw pointers. It
 * transforms the range elements into pointers of type T* using dynamic_cast for
 * std::unique_ptr and raw pointers. Elements that cannot be converted to T* are
 * filtered out.
 *
 * @tparam T The type to filter for.
 * @tparam R The type of the range.
 * @param r The range to filter.
 * @param _ An instance of type_is<T> (unused).
 * @return A new range containing only elements of std::unique_ptr<T> or T*,
 * converted to T*.
 *
 * @note This overload requires C++20 and the <ranges> library.
 *
 * Example usage:
 * @code
 * std::vector<std::unique_ptr<Track>> tracks1;
 * for (auto track : tracks1 | type_is<AudioTrack>{})
 * {
 *   // `track` is of type `AudioTrack*`
 *   // ...
 * }
 *
 * std::vector<Track*> tracks2;
 * for (auto track : tracks2 | type_is<AudioTrack>{})
 * {
 *   // `track` is of type `AudioTrack*`
 *   // ...
 * }
 * @endcode
 */
template <typename T, typename R>
auto
operator| (R &&r, type_is<T>)
{
  return std::forward<R> (r) | std::views::transform ([] (const auto &u) {
           using U = std::decay_t<decltype (u)>;
           if constexpr (is_unique_ptr<U>::value)
             {
               using PtrType = typename std::pointer_traits<U>::element_type;
               return dynamic_cast<T *> (static_cast<PtrType *> (u.get ()));
             }
           else if constexpr (std::is_pointer_v<U>)
             {
               return dynamic_cast<T *> (u);
             }
           else
             {
               return static_cast<T *> (nullptr);
             }
         })
         | std::views::filter ([] (const auto &ptr) { return ptr != nullptr; });
}

/**
 * @brief RAII class for managing the lifetime of an atomic bool.
 *
 * This class provides a convenient and safe way to manage the lifetime of an
 * atomic bool using the RAII (Resource Acquisition Is Initialization) idiom.
 * When an instance of AtomicBoolRAII is created, it sets the associated atomic
 * bool to true. When the instance goes out of scope, the destructor sets the
 * atomic bool back to false.
 *
 * Usage example:
 * @code
 * std::atomic<bool> flag = false;
 *
 * void someFunction() {
 *     AtomicBoolRAII raii(flag);
 *     // Do some work while the flag is set to true
 * }
 *
 * // After someFunction() returns, the flag is automatically set back to false
 * @endcode
 */
class AtomicBoolRAII
{
private:
  std::atomic<bool> &m_flag;

public:
  explicit AtomicBoolRAII (std::atomic<bool> &flag) : m_flag (flag)
  {
    m_flag.store (true, std::memory_order_release);
  }

  ~AtomicBoolRAII () { m_flag.store (false, std::memory_order_release); }

  // Disable copy and move operations
  AtomicBoolRAII (const AtomicBoolRAII &) = delete;
  AtomicBoolRAII &operator= (const AtomicBoolRAII &) = delete;
  AtomicBoolRAII (AtomicBoolRAII &&) = delete;
  AtomicBoolRAII &operator= (AtomicBoolRAII &&) = delete;
};

/**
 * @brief RAII wrapper class for std::binary_semaphore.
 *
 * This class provides a convenient and safe way to acquire and release a binary
 * semaphore using the RAII (Resource Acquisition Is Initialization) idiom. It
 * ensures that the semaphore is properly released when the wrapper object goes
 * out of scope.
 */
template <typename SemaphoreType = std::binary_semaphore> class SemaphoreRAII
{
public:
  /**
   * @brief Constructor that attempts to acquire the semaphore.
   *
   * @param semaphore The semaphore to acquire.
   */
  explicit SemaphoreRAII (SemaphoreType &semaphore, bool force_acquire = false)
      : semaphore (semaphore), acquired (false)
  {
    if (force_acquire)
      {
        semaphore.acquire ();
        acquired = true;
      }
    else
      {
        acquired = semaphore.try_acquire ();
      }
  }

  /**
   * @brief Destructor that releases the semaphore if it was acquired.
   */
  ~SemaphoreRAII ()
  {
    if (acquired)
      {
        semaphore.release ();
      }
  }

  /**
   * @brief Attempts to acquire the semaphore if it hasn't been acquired already.
   *
   * @return true if the semaphore is successfully acquired, false otherwise.
   */
  bool try_acquire ()
  {
    if (!acquired)
      {
        acquired = semaphore.try_acquire ();
      }
    return acquired;
  }

  /**
   * @brief Releases the semaphore if it was previously acquired.
   */
  void release ()
  {
    if (acquired)
      {
        semaphore.release ();
        acquired = false;
      }
  }

  /**
   * @brief Checks if the semaphore is currently acquired by the wrapper object.
   *
   * @return true if the semaphore is acquired, false otherwise.
   */
  bool is_acquired () const { return acquired; }

private:
  SemaphoreType &semaphore; ///< Reference to the binary semaphore.
  bool           acquired;  ///< Flag indicating if the semaphore is acquired.
};

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

template <typename Derived, typename Base>
concept DerivedButNotBase =
  std::derived_from<Derived, Base> && !std::same_as<Derived, Base>;

class Region;
template <typename RegionT>
concept RegionSubclass = DerivedButNotBase<RegionT, Region>;
template <typename RegionT> class LaneOwnedObjectImpl;
template <typename RegionT>
concept LaneOwnedRegionSubclass =
  DerivedButNotBase<RegionT, Region>
  && DerivedButNotBase<RegionT, LaneOwnedObjectImpl<RegionT>>;
template <typename RegionT> class RegionOwnedObjectImpl;
template <typename RegionT> class RegionOwnerImpl;
template <typename RegionT> class TrackLaneImpl;
template <typename RegionT> class LanedTrackImpl;

class MidiRegion;
class ChordRegion;
class AutomationRegion;
class AudioRegion;
using RegionVariant =
  std::variant<MidiRegion, ChordRegion, AutomationRegion, AudioRegion>;
using RegionOwnerVariant = std::variant<
  RegionOwnerImpl<MidiRegion>,
  RegionOwnerImpl<ChordRegion>,
  RegionOwnerImpl<AutomationRegion>,
  RegionOwnerImpl<AudioRegion>>;
using TrackLaneVariant =
  std::variant<TrackLaneImpl<MidiRegion>, TrackLaneImpl<AudioRegion>>;

class TimelineSelections;
class MidiSelections;
class ChordSelections;
class AutomationSelections;
class AudioSelections;
using ArrangerSelectionsVariant = std::variant<
  TimelineSelections,
  MidiSelections,
  ChordSelections,
  AutomationSelections,
  AudioSelections>;

class MarkerTrack;
class InstrumentTrack;
class MidiTrack;
class MasterTrack;
class MidiGroupTrack;
class AudioGroupTrack;
class FolderTrack;
class MidiBusTrack;
class AudioBusTrack;
class AudioTrack;
class ChordTrack;
class ModulatorTrack;
class TempoTrack;
using TrackVariant = std::variant<
  MarkerTrack,
  InstrumentTrack,
  MidiTrack,
  MasterTrack,
  MidiGroupTrack,
  AudioGroupTrack,
  FolderTrack,
  MidiBusTrack,
  AudioBusTrack,
  AudioTrack,
  ChordTrack,
  ModulatorTrack,
  TempoTrack>;
using LanedTrackVariant = std::variant<MidiTrack, InstrumentTrack, AudioTrack>;

class MidiNote;
using LengthableObjectVariant =
  std::variant<MidiRegion, AudioRegion, ChordRegion, AutomationRegion, MidiNote>;

using LaneOwnedObjectVariant = std::variant<MidiRegion, AudioRegion>;

class CarlaNativePlugin;
using PluginVariant = std::variant<CarlaNativePlugin>;

class ChordObject;
class ScaleObject;
class AutomationPoint;
class Velocity;
class Marker;
using ArrangerObjectVariant = std::variant<
  MidiNote,
  ChordObject,
  ScaleObject,
  MidiRegion,
  AudioRegion,
  ChordRegion,
  AutomationRegion,
  AutomationPoint,
  Marker,
  Velocity>;
using ArrangerObjectWithoutVelocityVariant = std::variant<
  MidiNote,
  ChordObject,
  ScaleObject,
  MidiRegion,
  AudioRegion,
  ChordRegion,
  AutomationRegion,
  Marker,
  AutomationPoint>;
class Timeline;
class PianoRoll;
class AutomationEditor;
class ChordEditor;
class AudioClipEditor;
using EditorSettingsVariant = std::
  variant<Timeline, PianoRoll, AutomationEditor, ChordEditor, AudioClipEditor>;
class ArrangerSelectionsAction;
class ChannelSendAction;
class ChordAction;
class MidiMappingAction;
class MixerSelectionsAction;
class PortAction;
class PortConnectionAction;
class RangeAction;
class TracklistSelectionsAction;
class TransportAction;
using UndoableActionVariant = std::variant<
  ArrangerSelectionsAction,
  ChannelSendAction,
  ChordAction,
  MidiMappingAction,
  MixerSelectionsAction,
  PortAction,
  PortConnectionAction,
  RangeAction,
  TracklistSelectionsAction,
  TransportAction>;

class MidiPort;
class AudioPort;
class CVPort;
class ControlPort;
using PortVariant = std::variant<MidiPort, AudioPort, CVPort, ControlPort>;

using NameableObjectVariant =
  std::variant<MidiRegion, AudioRegion, ChordRegion, AutomationRegion, Marker>;

using ProcessableTrackVariant = std::variant<
  InstrumentTrack,
  MidiTrack,
  MasterTrack,
  MidiGroupTrack,
  AudioGroupTrack,
  MidiBusTrack,
  AudioBusTrack,
  AudioTrack,
  ChordTrack,
  ModulatorTrack,
  TempoTrack>;

using RegionOwnedObjectVariant =
  std::variant<MidiNote, ChordObject, AutomationPoint>;

/** @brief Helper struct to convert a variant to a variant of pointers */
template <typename Variant> struct to_pointer_variant_impl;

/** @brief Specialization for std::variant */
template <typename... Ts> struct to_pointer_variant_impl<std::variant<Ts...>>
{
  /** @brief The resulting variant type with pointers */
  using type = std::variant<std::add_pointer_t<Ts>...>;
};

/**
 * @brief Converts a variant to a variant of pointers
 * @tparam Variant The original variant type
 */
template <typename Variant>
using to_pointer_variant = typename to_pointer_variant_impl<Variant>::type;

using LanedTrackPtrVariant = to_pointer_variant<LanedTrackVariant>;
using TrackLanePtrVariant = to_pointer_variant<TrackLaneVariant>;
using RegionOwnerPtrVariant = to_pointer_variant<RegionOwnerVariant>;
using ArrangerSelectionsPtrVariant =
  to_pointer_variant<ArrangerSelectionsVariant>;
using TrackPtrVariant = to_pointer_variant<TrackVariant>;
using ArrangerObjectPtrVariant = to_pointer_variant<ArrangerObjectVariant>;
using ArrangerObjectWithoutVelocityPtrVariant =
  to_pointer_variant<ArrangerObjectWithoutVelocityVariant>;
using EditorSettingsPtrVariant = to_pointer_variant<EditorSettingsVariant>;
using RegionPtrVariant = to_pointer_variant<RegionVariant>;
using UndoableActionPtrVariant = to_pointer_variant<UndoableActionVariant>;
using LengthableObjectPtrVariant = to_pointer_variant<LengthableObjectVariant>;
using PortPtrVariant = to_pointer_variant<PortVariant>;
using PluginPtrVariant = to_pointer_variant<PluginVariant>;
using NameableObjectPtrVariant = to_pointer_variant<NameableObjectVariant>;
using ProcessableTrackPtrVariant = to_pointer_variant<ProcessableTrackVariant>;
using RegionOwnedObjectPtrVariant = to_pointer_variant<RegionOwnedObjectVariant>;

/**
 * @brief Converts a base pointer to a variant type.
 *
 * This function takes a base pointer and attempts to convert it to the
 * specified variant type. If the base pointer is null, a variant with a null
 * pointer is returned. Otherwise, the function uses dynamic_cast to attempt to
 * convert the base pointer to each type in the variant, and returns the first
 * successful conversion.
 *
 * @tparam Base The base type of the pointer.
 * @tparam Variant The variant type to convert to.
 * @param base_ptr The base pointer to convert.
 * @return The converted variant type.
 */
template <typename Variant, typename Base>
requires std::is_pointer_v<std::variant_alternative_t<0, Variant>> auto
         convert_to_variant (const Base * base_ptr) -> Variant
{
  if (!base_ptr)
    {
      return Variant{};
    }

  Variant result{};

  auto trycast = [&]<typename T> () {
    if (auto derived = dynamic_cast<const T *> (base_ptr))
      {
        result = const_cast<T *> (derived);
        return true;
      }
    return false;
  };

  auto try_all_casts = [&]<typename... Ts> (std::variant<Ts...> *) {
    return (trycast.template operator()<std::remove_pointer_t<Ts>> () || ...);
  };

  try_all_casts (static_cast<Variant *> (nullptr));

  return result;
}

// Helper struct to merge multiple std::variant types
template <typename... Variants> struct merge_variants;

// Type alias for easier use of the merge_variants struct
template <typename... Variants>
using merge_variants_t = typename merge_variants<Variants...>::type;

// Base case: single variant, no merging needed
template <typename Variant> struct merge_variants<Variant>
{
  using type = Variant;
};

// Recursive case: merge two variants and continue with the rest
template <typename... Types1, typename... Types2, typename... Rest>
struct merge_variants<std::variant<Types1...>, std::variant<Types2...>, Rest...>
{
  // Merge the first two variants and recursively merge with the rest
  using type = merge_variants_t<std::variant<Types1..., Types2...>, Rest...>;
};

// Usage example:
// using Variant1 = std::variant<int, float>;
// using Variant2 = std::variant<char, double>;
// using MergedVariant = merge_variants_t<Variant1, Variant2>;
// MergedVariant is now std::variant<int, float, char, double>

#if __has_attribute(noipa)
#  define KEEP __attribute__ ((used, retain, noipa))
#else
#  define KEEP __attribute__ ((used, retain, noinline))
#endif

/**
 * @}
 */

#endif
