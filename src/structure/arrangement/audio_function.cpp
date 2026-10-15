// SPDX-FileCopyrightText: © 2020, 2023-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cmath>
#include <optional>
#include <ranges>
#include <variant>
#include <vector>

#include "dsp/tempo_map.h"
#include "structure/arrangement/audio_clip.h"
#include "structure/arrangement/audio_function.h"
#include "structure/arrangement/editor_function_kind.h"
#include "utils/enum_utils.h"
#include "utils/exceptions.h"
#include "utils/float_ranges.h"
#include "utils/logger.h"
#include "utils/utf8_string.h"

#include <rubberband/rubberband-c.h>

DEFINE_ENUM_FORMATTER (
  zrythm::structure::arrangement::AudioFunctionType,
  AudioFunctionType,
  QT_TR_NOOP_UTF8 ("Invert"),
  QT_TR_NOOP_UTF8 ("Normalize peak"),
  QT_TR_NOOP_UTF8 ("Normalize RMS"),
  QT_TR_NOOP_UTF8 ("Normalize LUFS"),
  QT_TR_NOOP_UTF8 ("Linear fade in"),
  QT_TR_NOOP_UTF8 ("Linear fade out"),
  QT_TR_NOOP_UTF8 ("Nudge left"),
  QT_TR_NOOP_UTF8 ("Nudge right"),
  QT_TR_NOOP_UTF8 ("Reverse"),
  QT_TR_NOOP_UTF8 ("Pitch shift"),
  QT_TR_NOOP_UTF8 ("Copy L to R"),
  QT_TR_NOOP_UTF8 ("External program"),
  QT_TR_NOOP_UTF8 ("Guile script"),
  QT_TR_NOOP_UTF8 ("Custom plugin"),
  QT_TR_NOOP_UTF8 ("Invalid"));

namespace zrythm::structure::arrangement
{

// --- Function kinds ---

// String literals are kept directly inside the QObject::tr call: lupdate
// only extracts literals written there.
namespace AudioFunction
{

utils::Utf8String
Invert::label (bool translate)
{
  return translate
           ? utils::Utf8String::from_qstring (QObject::tr ("Invert Polarity"))
           : utils::Utf8String (u8"Invert Polarity");
}

std::vector<EditorFunctionParam<Invert>>
Invert::params ()
{
  return {};
}

utils::Utf8String
Reverse::label (bool translate)
{
  return translate ? utils::Utf8String::from_qstring (QObject::tr ("Reverse"))
                   : utils::Utf8String (u8"Reverse");
}

std::vector<EditorFunctionParam<Reverse>>
Reverse::params ()
{
  return {};
}

utils::Utf8String
NormalizePeak::label (bool translate)
{
  return translate
           ? utils::Utf8String::from_qstring (QObject::tr ("Normalize Peak"))
           : utils::Utf8String (u8"Normalize Peak");
}

std::vector<EditorFunctionParam<NormalizePeak>>
NormalizePeak::params ()
{
  return {};
}

utils::Utf8String
NormalizeRms::label (bool translate)
{
  return translate
           ? utils::Utf8String::from_qstring (QObject::tr ("Normalize RMS"))
           : utils::Utf8String (u8"Normalize RMS");
}

std::vector<EditorFunctionParam<NormalizeRms>>
NormalizeRms::params ()
{
  return {};
}

utils::Utf8String
FadeIn::label (bool translate)
{
  return translate ? utils::Utf8String::from_qstring (QObject::tr ("Fade In"))
                   : utils::Utf8String (u8"Fade In");
}

std::vector<EditorFunctionParam<FadeIn>>
FadeIn::params ()
{
  return {
    { utils::Utf8String (u8"curveAlgorithm"),
     utils::Utf8String::from_qstring (QObject::tr ("Curve")),
     { .kind = ParamSpec::Kind::Choice,
        .choices = CurveOptions_Algorithm_to_qml_list (),
        .default_value =
          ENUM_VALUE_TO_INT (dsp::CurveOptions::Algorithm::SuperEllipse) },
     [] (FadeIn &f, ParamValue v) {
        f.curve.algo_ =
          static_cast<dsp::CurveOptions::Algorithm> (std::get<int> (v));
      } },
    { utils::Utf8String (u8"curviness"),
     utils::Utf8String::from_qstring (QObject::tr ("Curviness")),
     { .kind = ParamSpec::Kind::Double,
        .minimum = -1.0,
        .maximum = 1.0,
        .step = 0.01,
        .default_value = 0.5 },
     [] (FadeIn &f, ParamValue v) {
        f.curve.curviness_ = std::get<double> (v);
      } },
  };
}

utils::Utf8String
FadeOut::label (bool translate)
{
  return translate ? utils::Utf8String::from_qstring (QObject::tr ("Fade Out"))
                   : utils::Utf8String (u8"Fade Out");
}

std::vector<EditorFunctionParam<FadeOut>>
FadeOut::params ()
{
  return {
    { utils::Utf8String (u8"curveAlgorithm"),
     utils::Utf8String::from_qstring (QObject::tr ("Curve")),
     { .kind = ParamSpec::Kind::Choice,
        .choices = CurveOptions_Algorithm_to_qml_list (),
        .default_value =
          ENUM_VALUE_TO_INT (dsp::CurveOptions::Algorithm::SuperEllipse) },
     [] (FadeOut &f, ParamValue v) {
        f.curve.algo_ =
          static_cast<dsp::CurveOptions::Algorithm> (std::get<int> (v));
      } },
    { utils::Utf8String (u8"curviness"),
     utils::Utf8String::from_qstring (QObject::tr ("Curviness")),
     { .kind = ParamSpec::Kind::Double,
        .minimum = -1.0,
        .maximum = 1.0,
        .step = 0.01,
        .default_value = 0.5 },
     [] (FadeOut &f, ParamValue v) {
        f.curve.curviness_ = std::get<double> (v);
      } },
  };
}

std::optional<Entry>
kind_from_id (std::string_view id)
{
  return detail::kind_from_id<Entry> (id);
}

utils::Utf8String
label (const Entry &entry, bool translate)
{
  return detail::label<Entry> (entry, translate);
}

namespace
{
constexpr auto kKindKey = "kind"sv;
constexpr auto kStartKey = "start"sv;
constexpr auto kEndKey = "end"sv;
constexpr auto kGainKey = "gain"sv;
constexpr auto kCurveKey = "curve"sv;
}

void
entry_to_json (nlohmann::json &j, const Entry &entry)
{
  std::visit (
    [&] (const auto &k) {
      using K = std::decay_t<decltype (k)>;
      j[kKindKey] = std::string (K::kId);
      j[kStartKey] = k.start.asDouble ();
      j[kEndKey] = k.end.asDouble ();
      if constexpr (requires { k.gain; })
        j[kGainKey] = k.gain;
      if constexpr (requires { k.curve; })
        to_json (j[kCurveKey], k.curve);
    },
    entry);
}

void
entry_from_json (const nlohmann::json &j, Entry &entry)
{
  const auto kind_tag = j.at (kKindKey).get<std::string> ();
  const auto kind = kind_from_id (kind_tag);
  if (!kind)
    {
      throw ZrythmException (
        fmt::format ("Unknown audio transform entry kind: {}", kind_tag));
    }
  entry = *kind;
  std::visit (
    [&] (auto &k) {
      k.start =
        dsp::ContentTick{ units::ticks (j.at (kStartKey).get<double> ()) };
      k.end = dsp::ContentTick{ units::ticks (j.at (kEndKey).get<double> ()) };
      if constexpr (requires { k.gain; })
        j.at (kGainKey).get_to (k.gain);
      if constexpr (requires { k.curve; })
        from_json (j.at (kCurveKey), k.curve);
    },
    entry);
}

namespace
{

/// Maps content ticks to native source samples for the clip's effective
/// BPM and sample rate.
struct NativeMapping
{
  units::bpm_t                 effective_bpm;
  units::precise_sample_rate_t sample_rate;

  units::sample_t ticks_to_native (dsp::ContentTick t) const
  {
    return au::round_as<int64_t> (
      units::samples,
      units::ticks (t.asDouble ()) / effective_bpm * sample_rate);
  }
};

NativeMapping
native_mapping_for (const AudioClip &clip, const dsp::FileAudioSource &fs)
{
  const auto &tempo_map = clip.get_tempo_map ();
  const auto  source_bpm = fs.source_bpm ();
  return {
    source_bpm > units::bpm (0.0)
      ? source_bpm
      : tempo_map.tempo_at_tick (
          units::ticks (static_cast<int64_t> (clip.position ()->ticks ()))),
    tempo_map.get_sample_rate ()
  };
}

} // namespace

utils::audio::AudioBuffer
audio_clip_content_window (const AudioClip &clip)
{
  z_return_val_if_fail (
    !clip.get_children_view ().empty (), utils::audio::AudioBuffer{});
  auto       &fs = clip.get_children_view ().front ()->file_audio_source ();
  const int   channels = fs.get_num_channels ();
  const auto  clip_frames = fs.get_num_frames ();
  const auto &mapping = native_mapping_for (clip, fs);
  const auto  native_offset = [&mapping] (const dsp::Position * pos) {
    return mapping.ticks_to_native (
      dsp::ContentTick{ units::ticks (pos->ticks ()) });
  };

  const auto clip_start_s = clamp (
    native_offset (clip.clipStartPosition ()), units::samples (0), clip_frames);
  const auto loop_end_s = clamp (
    native_offset (clip.loopEndPosition ()), units::samples (0), clip_frames);
  const auto length_s = max (units::samples (0), native_offset (clip.length ()));
  const auto end = max (
    clip_start_s, min (clip_frames, max (loop_end_s, clip_start_s + length_s)));

  utils::audio::AudioBuffer window{ channels, end.in<int> (units::samples) };
  fs.read_span (units::samples (0), end, window);

  for (const auto &entry : clip.transform_entries ())
    {
      const auto [entry_start, entry_end] = std::visit (
        [] (const auto &k) { return std::pair{ k.start, k.end }; }, entry);
      const auto start =
        clamp (mapping.ticks_to_native (entry_start), units::samples (0), end);
      const auto stop =
        clamp (mapping.ticks_to_native (entry_end), units::samples (0), end);
      if (stop <= start)
        continue;
      apply_entry_to_buffer (
        window, entry,
        units::samples (static_cast<uint64_t> (start.in (units::samples))),
        units::samples (
          static_cast<uint64_t> ((stop - start).in (units::samples))));
    }

  return window;
}

double
normalize_gain (
  const AudioClip &clip,
  dsp::ContentTick start,
  dsp::ContentTick end,
  bool             rms)
{
  z_return_val_if_fail (!clip.get_children_view ().empty (), 1.0);
  const auto window = audio_clip_content_window (clip);
  const auto frames = units::samples (window.getNumSamples ());
  auto      &fs = clip.get_children_view ().front ()->file_audio_source ();
  const auto mapping = native_mapping_for (clip, fs);
  const auto range_start =
    clamp (mapping.ticks_to_native (start), units::samples (0), frames);
  const auto range_end =
    clamp (mapping.ticks_to_native (end), units::samples (0), frames);
  if (range_end <= range_start)
    return 1.0;

  float  peak = 0.F;
  double sum_of_squares = 0.0;
  size_t sample_count = 0;
  for (const auto ch : std::views::iota (0, window.getNumChannels ()))
    {
      for (
        const auto i : std::views::iota (
          range_start.in<int64_t> (units::samples),
          range_end.in<int64_t> (units::samples)))
        {
          const auto sample = window.getSample (ch, static_cast<int> (i));
          peak = std::max (peak, std::abs (sample));
          sum_of_squares +=
            static_cast<double> (sample) * static_cast<double> (sample);
          ++sample_count;
        }
    }
  if (sample_count == 0)
    return 1.0;
  if (rms)
    {
      const double rms_value =
        std::sqrt (sum_of_squares / static_cast<double> (sample_count));
      return rms_value > 0.0 ? 1.0 / rms_value : 1.0;
    }
  return peak > 0.F ? 1.0 / peak : 1.0;
}

namespace
{

/// Per-frame gain multipliers for a fade over @p num_frames frames.
std::vector<float>
curve_gains (int num_frames, const dsp::CurveOptions &curve, bool rising)
{
  std::vector<float> gains (static_cast<size_t> (num_frames));
  for (const auto i : std::views::iota (0, num_frames))
    {
      const auto fraction =
        (num_frames == 1)
          ? 1.0
          : static_cast<double> (i) / static_cast<double> (num_frames - 1);
      gains[static_cast<size_t> (i)] = static_cast<float> (
        rising
          ? curve.get_normalized_y (fraction, false)
          : curve.get_normalized_y (1.0 - fraction, false));
    }
  return gains;
}

void
mul_by_gains (std::span<float> range, const std::vector<float> &gains)
{
  for (const auto [sample, gain] : std::views::zip (range, gains))
    {
      sample *= gain;
    }
}

} // namespace

void
Invert::apply (std::span<float> range, const Invert &)
{
  utils::float_ranges::mul_k2 (range, -1.f);
}

void
Reverse::apply (std::span<float> range, const Reverse &)
{
  std::ranges::reverse (range);
}

void
NormalizePeak::apply (std::span<float> range, const NormalizePeak &kind)
{
  utils::float_ranges::mul_k2 (range, static_cast<float> (kind.gain));
}

void
NormalizeRms::apply (std::span<float> range, const NormalizeRms &kind)
{
  utils::float_ranges::mul_k2 (range, static_cast<float> (kind.gain));
}

void
FadeIn::apply (std::span<float> range, const FadeIn &kind)
{
  mul_by_gains (
    range, curve_gains (static_cast<int> (range.size ()), kind.curve, true));
}

void
FadeOut::apply (std::span<float> range, const FadeOut &kind)
{
  mul_by_gains (
    range, curve_gains (static_cast<int> (range.size ()), kind.curve, false));
}

void
apply_entry_to_buffer (
  utils::audio::AudioBuffer &buffer,
  const Entry               &entry,
  units::sample_u64_t        start_frame,
  units::sample_u64_t        num_frames)
{
  const auto total_frames = buffer.getNumSamples ();
  const auto start = std::clamp<int64_t> (
    static_cast<int64_t> (start_frame.in (units::samples)), 0, total_frames);
  const auto length = std::clamp<int64_t> (
    static_cast<int64_t> (num_frames.in (units::samples)), 0,
    total_frames - start);
  const auto channels = buffer.getNumChannels ();
  if (channels == 0 || length == 0)
    return;

  for (const auto ch : std::views::iota (0, channels))
    {
      const std::span<float> range{
        buffer.getWritePointer (ch) + start, static_cast<size_t> (length)
      };
      std::visit (
        [&] (const auto &k) {
          using K = std::decay_t<decltype (k)>;
          K::apply (range, k);
        },
        entry);
    }
}

} // namespace AudioFunction

utils::Utf8String
audio_function_get_action_target_for_type (AudioFunctionType type)
{
  auto type_str = AudioFunctionType_to_string (type);
  auto type_str_lower = type_str.to_lower ();
  auto substituted = type_str_lower.replace (u8" ", u8"-");

  return substituted;
}

utils::Utf8String
audio_function_type_to_string (AudioFunctionType type, bool translate)
{
  return AudioFunctionType_to_string (type, translate);
}

utils::Utf8String
audio_function_get_detailed_action_for_type (
  AudioFunctionType        type,
  const utils::Utf8String &base_action)
{
  auto target = audio_function_get_action_target_for_type (type);
  return utils::Utf8String::from_utf8_encoded_string (
    fmt::format ("{}::{}", base_action, target));
}

utils::Utf8String
audio_function_get_icon_name_for_type (AudioFunctionType type)
{
  switch (type)
    {
    case AudioFunctionType::Invert:
      return u8"edit-select-invert";
    case AudioFunctionType::Reverse:
      return u8"path-reverse";
    case AudioFunctionType::PitchShift:
      return u8"path-reverse";
    case AudioFunctionType::NormalizePeak:
      return u8"kt-set-max-upload-speed";
    case AudioFunctionType::LinearFadeIn:
      return u8"arena-fade-in";
    case AudioFunctionType::LinearFadeOut:
      return u8"arena-fade-out";
    case AudioFunctionType::NudgeLeft:
      return u8"arrow-left";
    case AudioFunctionType::NudgeRight:
      return u8"arrow-right";
    case AudioFunctionType::NormalizeRMS:
    case AudioFunctionType::NormalizeLUFS:
    default:
      return u8"modulator";
      break;
    }

  z_return_val_if_reached ({});
}

#if 0
/**
 * @param frames Interleaved frames.
 * @param num_frames Number of frames per channel.
 * @param channels Number of channels.
 *
 * @return Non-zero if fail
 */
static int
apply_plugin (
  const char * uri,
  float *      frames,
  size_t       num_frames,
  channels_t   channels,
  GError **    error)
{
  zrythm::plugins::PluginDescriptor * descr =
    plugin_manager_find_plugin_from_uri (zrythm::gui::old_dsp::plugins::PluginManager::get_active_instance (), uri);
  z_return_val_if_fail (descr, -1);
  PluginConfiguration * setting = plugin_setting_new_default (descr);
  z_return_val_if_fail (setting, -1);
  setting->force_generic_ui = true;
  GError * err = NULL;
  zrythm::plugins::Plugin * pl =
    plugin_new_from_setting (setting, 0, zrythm::plugins::PluginSlotType::Insert, 0, &err);
  if (!IS_PLUGIN_AND_NONNULL (pl))
    {
      PROPAGATE_PREFIXED_ERROR (error, err, "%s", QObject::tr ("Failed to create plugin"));
      return -1;
    }
  pl->is_function = true;
  int ret = plugin_instantiate (pl, nullptr, &err);
  if (ret != 0)
    {
      PROPAGATE_PREFIXED_ERROR (
        error, err, "%s", QObject::tr ("Failed to instantiate plugin"));
      return -1;
    }
  ret = plugin_activate (pl, true);
  z_return_val_if_fail (ret == 0, -1);
  plugin_setting_free (setting);

  /* create window */
  pl->window = GTK_WINDOW (gtk_dialog_new ());
  gtk_window_set_title (pl->window, descr->name);
  gtk_window_set_icon_name (pl->window, "zrythm");
  /*gtk_window_set_role (*/
  /*pl->window, "plugin_ui");*/
  gtk_window_set_modal (pl->window, true);
  /*gtk_window_set_attached_to (*/
  /*pl->window, GTK_WIDGET (MAIN_WINDOW));*/

  /* add vbox for stacking elements */
  pl->vbox = GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
  GtkWidget * container = gtk_dialog_get_content_area (GTK_DIALOG (pl->window));
  gtk_box_append (GTK_BOX (container), GTK_WIDGET (pl->vbox));

#  if 0
  /* add menu bar */
  zrythm::gui::old_dsp::plugins::plugin_gtk_build_menu (
    pl, GTK_WIDGET (pl->window),
    GTK_WIDGET (pl->vbox));
#  endif

  /* Create/show alignment to contain UI (whether
   * custom or generic) */
  pl->ev_box = GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0));
  gtk_box_append (pl->vbox, GTK_WIDGET (pl->ev_box));
  gtk_widget_set_vexpand (GTK_WIDGET (pl->ev_box), true);

  /* open */
  zrythm::gui::old_dsp::plugins::plugin_gtk_open_generic_ui (pl, false);

  ret = z_gtk_dialog_run (GTK_DIALOG (pl->window), false);

  Port * l_in = NULL;
  Port * r_in = NULL;
  Port * l_out = NULL;
  Port * r_out = NULL;
  for (int i = 0; i < pl->num_out_ports; i++)
    {
      Port * port = pl->out_ports[i];
      if (port->id_.type == PortType::Audio)
        {
          if (l_out)
            {
              r_out = port;
              break;
            }
          else
            {
              l_out = port;
            }
        }
    }
  int num_plugin_channels = r_out ? 2 : 1;
  for (int i = 0; i < pl->num_in_ports; i++)
    {
      Port * port = pl->in_ports[i];
      if (port->id_.type == PortType::Audio)
        {
          if (l_in)
            {
              r_in = port;
              break;
            }
          else
            {
              l_in = port;
              if (num_plugin_channels == 1)
                break;
            }
        }
    }
  if (num_plugin_channels == 1)
    {
      r_in = l_in;
      r_out = l_out;
    }

  uint32_t step = (uint32_t) MIN (AUDIO_ENGINE->block_length_, num_frames);
  size_t   i = 0; /* frames processed */
  plugin_update_latency (pl);
  nframes_t latency = pl->latency;
  while (i < num_frames)
    {
      for (size_t j = 0; j < step; j++)
        {
          z_return_val_if_fail (l_in, -1);
          z_return_val_if_fail (r_in, -1);
          l_in->buf[j] = frames[(i + j) * channels];
          r_in->buf[j] = frames[(i + j) * channels + 1];
        }
      lv2_ui_read_and_apply_events (pl->lv2, step);
      lilv_instance_run (pl->lv2->instance, step);
      for (size_t j = 0; j < step; j++)
        {
          int64_t actual_j =
            (int64_t) (i + j) - (int64_t) latency;
          if (actual_j < 0)
            continue;
#  if 0
          z_info (
            "{} {}",
            actual_j,
            fabsf (l_out->buf[j]));
#  endif
          z_return_val_if_fail (l_out, -1);
          z_return_val_if_fail (r_out, -1);
          frames[actual_j * (int64_t) channels] = l_out->buf[j];
          frames[actual_j * (int64_t) channels + 1] = r_out->buf[j];
        }
      if (i > latency)
        {
          plugin_update_latency (pl);
          /*z_info ("end latency {}", pl->latency);*/
        }
      i += step;
      step = (uint32_t) MIN (step, num_frames - i);
    }

  /* handle latency */
  i = 0;
  step = (uint32_t) MIN (AUDIO_ENGINE->block_length_, latency);
  while (i < latency)
    {
      for (size_t j = 0; j < step; j++)
        {
          z_return_val_if_fail (l_in, -1);
          z_return_val_if_fail (r_in, -1);
          l_in->buf[j] = 0.f;
          r_in->buf[j] = 0.f;
        }
      lv2_ui_read_and_apply_events (pl->lv2, step);
      lilv_instance_run (pl->lv2->instance, step);
      for (size_t j = 0; j < step; j++)
        {
          int64_t actual_j =
            (int64_t) (i + j + num_frames) - (int64_t) latency;
          z_return_val_if_fail (actual_j >= 0, -1);
#  if 0
          z_info (
            "{} {}",
            actual_j,
            fabsf (l_out->buf[j]));
#  endif
          z_return_val_if_fail (l_out, -1);
          z_return_val_if_fail (r_out, -1);
          frames[actual_j * (int64_t) channels] = l_out->buf[j];
          frames[actual_j * (int64_t) channels + 1] = r_out->buf[j];
        }
      i += step;
      step = (uint32_t) MIN (step, latency - i);
    }
  plugin_update_latency (pl);
  z_info ("end latency {}", pl->latency);

  zrythm::gui::old_dsp::plugins::plugin_gtk_close_ui (pl);
  plugin_free (pl);

  return 0;
}
#endif

void
audio_function_apply (
  ArrangerObject::Uuid                                    clip_id,
  std::pair<units::precise_tick_t, units::precise_tick_t> selected_range,
  AudioFunctionType                                       type,
  AudioFunctionOpts                                       opts,
  std::optional<utils::Utf8String>                        uri)
{
// TODO
#if 0
  using Position = AudioClip::Position;
  z_debug ("applying {}...", AudioFunctionType_to_string (type));

  auto * r =
    std::get<AudioClip *> (*PROJECT->find_arranger_object_by_id (clip_id));
  z_return_if_fail (r);
  auto tr = std::get<structure::tracks::AudioTrack *> (r->get_track ());
  z_return_if_fail (tr);
  auto * orig_clip = r->get_clip ();
  z_return_if_fail (orig_clip);

  if (sel_start < r->get_position () || sel_end > r->get_end_position ())
    {
      throw ZrythmException (
        QObject::tr ("Invalid positions - skipping function"));
    }

  /* adjust the positions */
  Position start = sel_start;
  Position end = sel_end;
  start.add_frames (-r->get_position ().frames_, AUDIO_ENGINE->ticks_per_frame_);
  end.add_frames (-r->get_position ().frames_, AUDIO_ENGINE->ticks_per_frame_);

  /* create a copy of the frames to be replaced */
  auto num_frames = (uint64_t) (end.frames_ - start.frames_);

  auto                      channels = orig_clip->get_num_channels ();
  utils::audio::AudioBuffer src_frames{
    channels, static_cast<int> (num_frames)
  };
  utils::audio::AudioBuffer dest_frames{
    channels, static_cast<int> (num_frames)
  };
  for (int j = 0; j < channels; ++j)
    {
      src_frames.copyFrom (
        j, 0, orig_clip->get_samples (), j, start.frames_, num_frames);
      dest_frames.copyFrom (
        j, 0, orig_clip->get_samples (), j, start.frames_, num_frames);
    }

  auto nudge_frames = (uint64_t) Position::get_frames_from_ticks (
    ArrangerObject::DEFAULT_NUDGE_TICKS, AUDIO_ENGINE->frames_per_tick_);
  uint64_t num_frames_excl_nudge;
  z_debug ("num frames {}, nudge_frames {}", num_frames, nudge_frames);
  z_return_if_fail_cmp (nudge_frames, >, 0);

  switch (type)
    {
    case AudioFunctionType::Invert:
      dest_frames.invert_phase ();
      break;
    case AudioFunctionType::NormalizePeak:
      dest_frames.normalize_peak ();
      break;
    case AudioFunctionType::NormalizeRMS:
      /* TODO rms-normalize */
      break;
    case AudioFunctionType::NormalizeLUFS:
      /* TODO lufs-normalize */
      break;
    case AudioFunctionType::LinearFadeIn:
      dest_frames.applyGainRamp (0, num_frames, 0.f, 1.f);
      break;
    case AudioFunctionType::LinearFadeOut:
      dest_frames.applyGainRamp (0, num_frames, 1.f, 0.f);
      break;
    case AudioFunctionType::NudgeLeft:
      z_return_if_fail (num_frames > nudge_frames);
      num_frames_excl_nudge = num_frames - (size_t) nudge_frames;

      for (int ch = 0; ch < channels; ch++)
        {
          // Copy shifted audio
          dest_frames.copyFrom (
            ch, 0, src_frames, ch, nudge_frames, num_frames_excl_nudge);
          // Clear end
          utils::float_ranges::fill (
            dest_frames.getWritePointer (ch, num_frames_excl_nudge), 0.f,
            nudge_frames);
        }
      break;
    case AudioFunctionType::NudgeRight:
      z_return_if_fail (num_frames > nudge_frames);
      num_frames_excl_nudge = num_frames - (size_t) nudge_frames;
      for (int ch = 0; ch < channels; ch++)
        {
          // Copy shifted audio
          dest_frames.copyFrom (
            ch, nudge_frames, src_frames, ch, 0, num_frames_excl_nudge);
          // Clear beginning
          utils::float_ranges::fill (
            dest_frames.getWritePointer (ch), 0.f, nudge_frames);
        }
      break;
    case AudioFunctionType::Reverse:
      dest_frames.reverse (0, num_frames);
      break;
    case AudioFunctionType::PitchShift:
      {
        z_return_if_fail_cmp (channels, >=, 2);
        RubberBandState   rubberband_state{};
        RubberBandOptions rubberband_opts =
          RubberBandOptionProcessOffline
        /* use finer engine if rubberband v3 */
#  if RUBBERBAND_API_MAJOR_VERSION > 2 \
    || (RUBBERBAND_API_MAJOR_VERSION == 2 && RUBBERBAND_API_MINOR_VERSION >= 7)
          | RubberBandOptionEngineFiner
#  endif
          | RubberBandOptionPitchHighQuality | RubberBandOptionFormantPreserved
          | RubberBandOptionThreadingAlways | RubberBandOptionChannelsApart;
        rubberband_state = rubberband_new (
          AUDIO_ENGINE->get_sample_rate (), channels, rubberband_opts, 1.0,
          opts.amount_);
        const size_t max_process_size = 8192;
        rubberband_set_debug_level (rubberband_state, 2);
        rubberband_set_max_process_size (rubberband_state, max_process_size);
        rubberband_set_expected_input_duration (rubberband_state, num_frames);
        rubberband_study (
          rubberband_state, src_frames.getArrayOfReadPointers (), num_frames,
          true);
        size_t samples_fed = 0;
        size_t frames_read = 0;
        while (frames_read < num_frames)
          {
            unsigned int samples_required = std::min (
              static_cast<size_t> (num_frames) - samples_fed, max_process_size);
            /*rubberband_get_samples_required (*/
            /*rubberband_state));*/
            std::array<const float * const, 2> tmp_in_arrays = {
              &src_frames.getReadPointer (0)[samples_fed],
              &src_frames.getReadPointer (1)[samples_fed]
            };
            samples_fed += samples_required;
            z_info (
              "samples required: {} (total fed {}), latency: {}",
              samples_required, samples_fed,
              rubberband_get_latency (rubberband_state));
            if (samples_required > 0)
              {
                rubberband_process (
                  rubberband_state, tmp_in_arrays.data (), samples_required,
                  samples_fed == num_frames);
              }
            for (;;)
              {
                int avail = rubberband_available (rubberband_state);
                if (avail == 0)
                  {
                    z_info ("avail == 0");
                    break;
                  }
                else if (avail == -1)
                  {
                    z_info ("avail == -1");
                    /* FIXME for some reason rubberband
                     * skips the last few samples when the
                     * pitch ratio is < 1.0
                     * this workaround just keeps the copied
                     * original samples at the end and should
                     * be fixed eventually */
                    frames_read = num_frames;
                    break;
#  if 0
                    g_set_error_literal (
                      error, Z_AUDIOAUDIO_FUNCTION_ERROR,
                      Z_AUDIOAUDIO_FUNCTION_ERROR_FAILED,
                      "rubberband: finished prematurely");
                    return false;
#  endif
                  }
                std::array<float *, 2> tmp_out_arrays = {
                  &dest_frames.getWritePointer (0)[frames_read],
                  &dest_frames.getWritePointer (1)[frames_read]
                };
                size_t retrieved_out_samples = rubberband_retrieve (
                  rubberband_state, tmp_out_arrays.data (),
                  (unsigned int) avail);
                if ((int) retrieved_out_samples != avail)
                  {
                    throw ZrythmException (
                      fmt::format (
                        "rubberband: retrieved out samples ({}) != available samples ({})",
                        retrieved_out_samples, avail));
                  }
                frames_read += retrieved_out_samples;
                z_debug (
                  "retrieved out samples {}, frames read {}",
                  retrieved_out_samples, frames_read);
              }
          }
        if (frames_read != num_frames)
          {
            throw ZrythmException (
              fmt::format (
                "rubberband: expected {} frames but read {}", num_frames,
                frames_read));
          }
      }
      break;
    case AudioFunctionType::CopyLtoR:
      if (channels == 2)
        {
          utils::float_ranges::copy (
            dest_frames.getWritePointer (1), src_frames.getReadPointer (0),
            num_frames);
        }
      else
        {
          throw ZrythmException (
            fmt::format ("copy_lto_r: expected 2 channels but got {}", channels));
        }
      break;
    case AudioFunctionType::ExternalProgram:
      {
        FileAudioSource tmp_clip_before (
          src_frames, FileAudioSource::BitDepth::BIT_DEPTH_32,
          AUDIO_ENGINE->get_sample_rate (), 140.f, u8"tmp-clip");
        auto tmp_clip = tmp_clip_before.edit_in_ext_program ();
        for (int i = 0; i < channels; ++i)
          {
            utils::float_ranges::copy (
              dest_frames.getWritePointer (i),
              tmp_clip->get_samples ().getReadPointer (i),
              std::min (
                static_cast<size_t> (num_frames),
                static_cast<size_t> (tmp_clip->get_num_frames ())));
          }

        if ((size_t) tmp_clip->get_num_frames () < num_frames)
          {
            for (int i = 0; i < channels; ++i)
              {
                utils::float_ranges::fill (
                  dest_frames.getWritePointer (i), 0.f,
                  num_frames - (size_t) tmp_clip->get_num_frames ());
              }
          }
      }
      break;
    case AudioFunctionType::CustomPlugin:
      {
#  if 0
        z_return_val_if_fail (uri, false);
        GError * err = NULL;
        int ret = apply_plugin (uri, dest_frames, num_frames, channels, &err);
        if (ret != 0)
          {
            PROPAGATE_PREFIXED_ERROR (
              error, err, "%s", QObject::tr ("Failed to apply plugin"));
            return false;
          }
#  endif
      }
      break;
    case AudioFunctionType::Invalid:
      /* do nothing */
      break;
    default:
      z_warning ("not implemented");
      break;
    }

#  if 0
  char * tmp =
    g_strdup_printf (
      "%s - %s - %s",
      tr->name, r->name,
      audio_function_type_to_string (type));

  /* remove dots from name */
  char * name = string_replace (tmp, ".", "_");
  g_free (tmp);
#  endif

// FIXME: where is the clip used??
#  if 0
  int  id = AUDIO_POOL->add_clip (std::make_shared<FileAudioSource> (
    dest_frames, FileAudioSource::BitDepth::BIT_DEPTH_32, AUDIO_ENGINE->sample_rate_,
    P_TEMPO_TRACK->get_current_bpm (), orig_clip->get_name ()));
  auto clip = AUDIO_POOL->get_clip (id);
  z_debug (
    "writing {} to pool (id {})", clip->get_name (), clip->get_pool_id ());
  AUDIO_POOL->write_clip (*clip, false, false);
#  endif

  // FIXME: needed?
  // audio_sel.pool_id_ = clip->get_pool_id ();

  if (type != AudioFunctionType::Invalid)
    {
      /* replace the frames in the clip */
      r->replace_frames (dest_frames, start.frames_);
    }

  if (
    !ZRYTHM_TESTING && !ZRYTHM_BENCHMARKING && type != AudioFunctionType::Invalid
    && type != AudioFunctionType::CustomPlugin)
    {
      /* set last action */
      gui::SettingsManager::get_instance ()->set_lastAudioFunction (
        ENUM_VALUE_TO_INT (type));
      if (type == AudioFunctionType::PitchShift)
        {
          gui::SettingsManager::get_instance ()
            ->set_lastAudioFunctionPitchShiftRatio (opts.amount_);
        }
    }

  // EVENTS_PUSH (EventType::ET_EDITOR_FUNCTION_APPLIED, nullptr);
#endif
}
}
