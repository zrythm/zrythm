// SPDX-FileCopyrightText: © 2020, 2023-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <concepts>
#include <optional>
#include <span>
#include <string_view>
#include <variant>
#include <vector>

#include "dsp/curve.h"
#include "dsp/file_audio_source.h"
#include "dsp/tick_types.h"
#include "structure/arrangement/arranger_object.h"
#include "structure/arrangement/editor_function_param.h"
#include "utils/audio.h"
#include "utils/iobject_registry.h"
#include "utils/units.h"
#include "utils/utf8_string.h"

#include <nlohmann/json_fwd.hpp>

namespace zrythm::structure::arrangement
{

class AudioClip;
enum class AudioFunctionType
{
  Invert,
  NormalizePeak,
  NormalizeRMS,
  NormalizeLUFS,
  LinearFadeIn,
  LinearFadeOut,
  NudgeLeft,
  NudgeRight,
  Reverse,
  PitchShift,
  CopyLtoR,

  /** External program. */
  ExternalProgram,

  /** Guile script (currently unavailable). */
  Script,

  /** Custom plugin. */
  CustomPlugin,

  /* reserved */
  Invalid,
};

class AudioFunctionOpts
{
public:
  /**
   * Amount related to the current function (e.g. pitch shift).
   */
  double amount_ = 0;
};

utils::Utf8String
audio_function_get_action_target_for_type (AudioFunctionType type);

/**
 * Returns the user-visible name of the type, translated when @p translate
 * is set.
 */
utils::Utf8String
audio_function_type_to_string (AudioFunctionType type, bool translate = false);

/**
 * Returns a detailed action name to be used for actionable widgets or menus.
 *
 * @param base_action Base action to use.
 */
utils::Utf8String
audio_function_get_detailed_action_for_type (
  AudioFunctionType        type,
  const utils::Utf8String &base_action);

#define audio_function_get_detailed_action_for_type_default(type) \
  audio_function_get_detailed_action_for_type (type, "app.editor-function")

utils::Utf8String
audio_function_get_icon_name_for_type (AudioFunctionType type);

/**
 * Applies the given action to the given selections.
 *
 * This will save a file in the pool and store the pool ID in the selections.
 *
 * @param sel Selections to edit.
 * @param type Function type. If invalid is passed, this will simply add the
 * audio file in the pool for the unchanged audio material (used in audio
 * selection actions for the selections before the change).
 * @throw ZrythmException on error.
 */
void
audio_function_apply (
  ArrangerObject::Uuid                                    clip_id,
  std::pair<units::precise_tick_t, units::precise_tick_t> selected_range,
  AudioFunctionType                                       type,
  AudioFunctionOpts                                       opts,
  std::optional<utils::Utf8String>                        uri);

// ==========================================================================
// Function kinds. A transform entry stores one kind instance, together
// with the content range it applies to.
// ==========================================================================

namespace AudioFunction
{

/// Inverts the polarity of the audio.
struct Invert
{
  static constexpr std::string_view kId = "invert-polarity";
  static constexpr int              kMinimumSelectionCount = 0;

  static utils::Utf8String label (bool translate = false);
  static std::vector<EditorFunctionParam<Invert>> params ();
  static void apply (std::span<float> range, const Invert &kind);
  QString     detail () const;

  bool operator== (const Invert &) const = default;
};

/// Reverses the audio.
struct Reverse
{
  static constexpr std::string_view kId = "reverse";
  static constexpr int              kMinimumSelectionCount = 0;

  static utils::Utf8String label (bool translate = false);
  static std::vector<EditorFunctionParam<Reverse>> params ();
  static void apply (std::span<float> range, const Reverse &kind);
  QString     detail () const;

  bool operator== (const Reverse &) const = default;
};

/// Scales the audio so its peak reaches unity. The gain is computed
/// once, when the entry is created, and stored frozen.
struct NormalizePeak
{
  static constexpr std::string_view kId = "normalize-peak";
  static constexpr int              kMinimumSelectionCount = 0;

  /// Sample multiplier applied by the entry.
  double gain = 1.0;

  static utils::Utf8String label (bool translate = false);
  static std::vector<EditorFunctionParam<NormalizePeak>> params ();
  static void apply (std::span<float> range, const NormalizePeak &kind);
  QString     detail () const;

  bool operator== (const NormalizePeak &) const = default;
};

/// Scales the audio so its RMS reaches unity. The gain is computed
/// once, when the entry is created, and stored frozen.
struct NormalizeRms
{
  static constexpr std::string_view kId = "normalize-rms";
  static constexpr int              kMinimumSelectionCount = 0;

  /// Sample multiplier applied by the entry.
  double gain = 1.0;

  static utils::Utf8String label (bool translate = false);
  static std::vector<EditorFunctionParam<NormalizeRms>> params ();
  static void apply (std::span<float> range, const NormalizeRms &kind);
  QString     detail () const;

  bool operator== (const NormalizeRms &) const = default;
};

/// Fades the audio in from silence, following a curve.
struct FadeIn
{
  static constexpr std::string_view kId = "fade-in";
  static constexpr int              kMinimumSelectionCount = 0;

  /// Curve shape of the ramp.
  dsp::CurveOptions curve{};

  static utils::Utf8String label (bool translate = false);
  static std::vector<EditorFunctionParam<FadeIn>> params ();
  static void apply (std::span<float> range, const FadeIn &kind);
  QString     detail () const;

  bool operator== (const FadeIn &) const = default;
};

/// Fades the audio out to silence, following a curve.
struct FadeOut
{
  static constexpr std::string_view kId = "fade-out";
  static constexpr int              kMinimumSelectionCount = 0;

  /// Curve shape of the ramp.
  dsp::CurveOptions curve{};

  static utils::Utf8String label (bool translate = false);
  static std::vector<EditorFunctionParam<FadeOut>> params ();
  static void apply (std::span<float> range, const FadeOut &kind);
  QString     detail () const;

  bool operator== (const FadeOut &) const = default;
};

/// The function kind of a transform entry, with its parameters.
using Kind =
  std::variant<Invert, Reverse, NormalizePeak, NormalizeRms, FadeIn, FadeOut>;

/**
 * @brief One entry in a clip's transform stack.
 *
 * An entry applies one function kind to a content time range. Entries
 * compose in list order; disabled entries are skipped when the clip is
 * rendered.
 */
struct Entry
{
  Q_GADGET
  QML_VALUE_TYPE (audioTransformEntry)
  QML_UNCREATABLE ("")
  Q_PROPERTY (QString label READ label CONSTANT)
  Q_PROPERTY (QString kindId READ kindId CONSTANT)
  Q_PROPERTY (bool enabled READ is_enabled CONSTANT)
  Q_PROPERTY (double startTicks READ start_ticks CONSTANT)
  Q_PROPERTY (double endTicks READ end_ticks CONSTANT)
  Q_PROPERTY (QString detail READ detail CONSTANT)

public:
  /// Content time range the entry applies to, [start, end).
  dsp::ContentTick start{};
  dsp::ContentTick end{};

  /// Whether the entry is applied when the clip is rendered.
  bool enabled = true;

  /// Function kind with its parameters.
  Kind kind{};

  bool operator== (const Entry &) const = default;

  /// User-visible name of the entry's function kind.
  QString label () const;

  /// Function id of the entry's kind, e.g. "fade-in".
  QString kindId () const;

  /// Short summary of the kind's parameters, empty for kinds without
  /// parameters.
  QString detail () const;

  bool   is_enabled () const { return enabled; }
  double start_ticks () const { return start.asDouble (); }
  double end_ticks () const { return end.asDouble (); }
};

/**
 * @brief The contract every audio function kind fulfills.
 *
 * A kind declares its identity, dialog parameters, its in-place effect
 * on one channel's sample range, and a short summary of its parameters.
 */
template <typename KindT>
concept AudioFunctionKind = requires (std::span<float> range, const KindT &kind) {
  { KindT::kId } -> std::convertible_to<std::string_view>;
  { KindT::kMinimumSelectionCount } -> std::convertible_to<int>;
  { KindT::label (false) } -> std::same_as<utils::Utf8String>;
  { KindT::params () };
  { KindT::apply (range, kind) } -> std::same_as<void>;
  { kind.detail () } -> std::convertible_to<QString>;
};

/// Returns the kind with the given function id, if any.
std::optional<Kind>
kind_from_id (std::string_view id);

/// Returns the user-visible name of the given function kind.
utils::Utf8String
label (const Kind &kind, bool translate = false);

/// Returns the user-visible name of the kind held by @p entry.
utils::Utf8String
label (const Entry &entry, bool translate = false);

/// Serializes one entry with a kind tag taken from its id.
void
entry_to_json (nlohmann::json &j, const Entry &entry);

/// Deserializes one entry written by @ref entry_to_json. An unknown kind
/// tag throws.
void
entry_from_json (const nlohmann::json &j, Entry &entry);

/**
 * @brief Returns the clip's un-looped content window with each enabled
 * transform entry applied.
 *
 * The window starts at source frame 0 and ends at the clip's audible end,
 * in source sample space. A content tick maps to the same source frame
 * the clip's own loop positions map to.
 */
utils::audio::AudioBuffer
audio_clip_content_window (const AudioClip &clip);

/**
 * @brief Materializes the clip's transform entries into a new source.
 *
 * The clip's source is flushed, its content window is rendered with
 * each enabled entry applied, and a new source is created in @p registry
 * sharing every block outside the union of the enabled entry ranges;
 * that range is replaced with the rendered audio. The clip itself is
 * not modified.
 *
 * @return Reference to the new source, registered in @p registry.
 */
dsp::FileAudioSourceUuidReference
bake_audio_clip (const AudioClip &clip, utils::IObjectRegistry &registry);

/**
 * @brief The sample multiplier that brings the clip's audio over
 * [start, end) to unity peak (rms = false) or unity RMS (rms = true).
 *
 * Computed over the clip's current content window, so entries already on
 * the clip are included.
 */
double
normalize_gain (
  const AudioClip &clip,
  dsp::ContentTick start,
  dsp::ContentTick end,
  bool             rms);

/**
 * @brief Applies one function kind to a frame range of a buffer.
 *
 * The kind applies to the samples in [start_frame, start_frame +
 * num_frames) of every channel, in place. The range is clamped to the
 * buffer's bounds; an empty clamped range is a no-op.
 */
void
apply_kind_to_buffer (
  utils::audio::AudioBuffer &buffer,
  const Kind                &kind,
  units::sample_u64_t        start_frame,
  units::sample_u64_t        num_frames);

} // namespace AudioFunction

namespace detail
{
template <typename Variant> inline constexpr bool all_kinds_fulfilled_v = false;
template <typename... Kinds>
inline constexpr bool all_kinds_fulfilled_v<std::variant<Kinds...>> =
  (AudioFunction::AudioFunctionKind<Kinds> && ...);
} // namespace detail

static_assert (
  detail::all_kinds_fulfilled_v<AudioFunction::Kind>,
  "every audio function kind must fulfill the AudioFunctionKind contract");

}
