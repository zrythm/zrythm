// SPDX-FileCopyrightText: © 2019-2022, 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "zrythm-config.h"

#include <chrono>
#include <cinttypes>
#include <cstdint>
#include <filesystem>
#include <functional>

#include <QtTypes>

#include <gsl-lite/gsl-lite.hpp>

using namespace std::literals;
namespace gsl = ::gsl_lite;

/**
 * @addtogroup utils
 *
 * @{
 */

// qint64
using RtTimePoint = int64_t;
using RtDuration = int64_t;

/** Number of channels. */
using channels_t = uint_fast8_t;

/** The sample type. */
using audio_sample_type_t = float;

/** The BPM type. */
using bpm_t = float;

using curviness_t = double;

/** Signed type for frame index. */
using signed_frame_t = int_fast64_t;

/** Unsigned type for frame index. */
using unsigned_frame_t = uint_fast64_t;

/** Signed millisecond index. */
using signed_ms_t = signed_frame_t;

/** Signed second index. */
using signed_sec_t = signed_frame_t;

/** GPid equivalent. */
using ProcessId = qint64;

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

using GenericBoolGetter = std::function<bool ()>;

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
 * Beat unit.
 */
enum class BeatUnit
{
  Two,
  Four,
  Eight,
  Sixteen
};

/* types for simple timestamps/durations */
using SteadyClock = std::chrono::steady_clock;
using SteadyTimePoint = SteadyClock::time_point;
using SteadyDuration = SteadyClock::duration;

namespace fs = std::filesystem;

#define ZRYTHM_IS_QT_THREAD (QThread::currentThread () == qApp->thread ())

template <typename T>
std::string
typename_to_string ()
{
  return typeid (T).name ();
}

#define Z_DISABLE_COPY_MOVE(Class) Q_DISABLE_COPY_MOVE (Class)
#define Z_DISABLE_COPY(Class) Q_DISABLE_COPY (Class)
#define Z_DISABLE_MOVE(Class) \
  Class (Class &&) = delete; \
  Class &operator= (Class &&) = delete;

template <typename Tuple, typename Callable>
void
iterate_tuple (Callable c, Tuple &&t)
{
  std::apply ([&] (auto &&... args) { (c (args), ...); }, t);
}

// We're using the C type here because MOC complains it can't find the type
// otherwise (not even std::uint8_t)
using basic_enum_base_type_t = uint8_t;

/**
 * @}
 */
