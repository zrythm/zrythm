// SPDX-FileCopyrightText: © 2019-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cstddef>
#include <utility>

#include "dsp/midi_event.h"
#include "dsp/port.h"
#include "structure/arrangement/timeline_data_provider.h"
#include "structure/tracks/clip_playback_data_provider.h"
#include "structure/tracks/track_processor.h"
#include "utils/float_ranges.h"
#include "utils/midi.h"
#include "utils/mpmc_queue.h"
#include "utils/qt.h"
#include "utils/registry_utils.h"
#include "utils/views.h"

#include <boost/container/static_vector.hpp>
#include <farbot/RealtimeObject.hpp>
#include <fmt/format.h>
#include <scn/scan.h>

namespace zrythm::structure::tracks
{

static constexpr auto midi_cc_id_format_str = "midi_controller_ch{}_{}"sv;
static constexpr auto pitch_bend_id_format_str = "ch{}_pitch_bend"sv;
static constexpr auto poly_key_pressure_id_format_str =
  "ch{}_poly_key_pressure"sv;
static constexpr auto channel_pressure_id_format_str = "ch{}_channel_pressure"sv;
static constexpr auto mono_param_id = "track_processor_mono_toggle"sv;
static constexpr auto input_gain_param_id = "track_processor_input_gain"sv;
static constexpr auto output_gain_param_id = "track_processor_output_gain"sv;
static constexpr auto monitor_audio_param_id = "track_processor_monitor_audio"sv;
static constexpr auto recording_param_id = "track_processor_record"sv;

struct TrackProcessor::Impl
{
  std::optional<FillEventsCallback>      fill_events_cb_;
  RecordingCallbackRT                    handle_recording_cb_;
  AppendMidiInputsToOutputsFunc          append_midi_inputs_to_outputs_func_;
  std::optional<TransformMidiInputsFunc> transform_midi_inputs_func_;

  /** Mono toggle, if audio. */
  std::optional<dsp::ProcessorParameter::Uuid> mono_id_;

  /** Input gain, if audio. */
  std::optional<dsp::ProcessorParameter::Uuid> input_gain_id_;

  /**
   * Output gain, if audio.
   *
   * This is applied after regions are processed to @ref stereo_out_.
   */
  std::optional<dsp::ProcessorParameter::Uuid> output_gain_id_;

  /**
   * Monitor mode for audio tracks: Off, On, or Auto.
   *
   * In Auto mode, input monitoring is active only when the track is armed
   * for recording. This is only created for audio tracks.
   */
  std::optional<dsp::ProcessorParameter::Uuid> monitor_audio_id_;

  std::optional<dsp::ProcessorParameter::Uuid> recording_id_;

#if 0
  /** Mappings to each CC port. */
  std::unique_ptr<engine::session::MidiMappings> cc_mappings_;
#endif

  /**
   * @brief Runtime caches of MIDI CC and automation controls.
   */
  struct MidiCcCaches
  {
    /** MIDI CC control ports, 16 channels x 128 controls. */
    std::array<dsp::ProcessorParameter::Uuid, static_cast<size_t> (16) * 128>
      midi_cc_ids_;

    using SixteenPortUuidArray = std::array<dsp::ProcessorParameter::Uuid, 16>;

    /** Pitch bend x 16 channels. */
    SixteenPortUuidArray pitch_bend_ids_;

    /**
     * Polyphonic key pressure (aftertouch).
     *
     * This message is most often sent by pressing down on the key after it
     * "bottoms out".
     *
     * FIXME this is completely wrong. It's supposed to be per-key, so 128 x
     * 16 ports.
     */
    SixteenPortUuidArray poly_key_pressure_ids_;

    /**
     * Channel pressure (aftertouch).
     *
     * This message is different from polyphonic after-touch - sends the
     * single greatest pressure value (of all the current depressed keys).
     */
    SixteenPortUuidArray channel_pressure_ids_;

    /**
     * A queue of MIDI CC ports whose values have been recently updated.
     *
     * This is used during processing to avoid checking every single MIDI CC
     * port for changes.
     */
    MPMCQueue<dsp::ProcessorParameter *> updated_midi_automatable_ports_;
  };

  std::unique_ptr<MidiCcCaches> midi_cc_caches_;

  // Processing caches
  struct ProcessingCaches
  {
    std::vector<dsp::AudioPort *> audio_ins_rt_;
    std::vector<dsp::AudioPort *> audio_outs_rt_;
    dsp::MidiPort *               midi_in_rt_{};
    dsp::MidiPort *               midi_out_rt_{};
    dsp::MidiPort *               piano_roll_rt_{};
    dsp::ProcessorParameter *     mono_param_{};
    dsp::ProcessorParameter *     input_gain_{};
    dsp::ProcessorParameter *     output_gain_{};
    dsp::ProcessorParameter *     monitor_audio_{};
    dsp::ProcessorParameter *     recording_param_{};
  };

  std::unique_ptr<ProcessingCaches> processing_caches_;

  /**
   * @brief MIDI data provider from the timeline.
   */
  utils::QObjectUniquePtr<arrangement::MidiTimelineDataProvider>
    timeline_midi_data_provider_;

  /**
   * @brief Audio data provider from the timeline.
   */
  utils::QObjectUniquePtr<arrangement::AudioTimelineDataProvider>
    timeline_audio_data_provider_;

  std::unique_ptr<ClipPlaybackDataProvider> clip_playback_data_provider_;

  // TODO: piano roll, recording data providers

  farbot::RealtimeObject<
    std::optional<MidiEventProviderProcessFunc>,
    farbot::RealtimeObjectOptions::nonRealtimeMutatable>
    custom_midi_event_provider_;

  std::atomic<ActiveMidiEventProviders> active_midi_event_providers_;
  std::atomic<ActiveAudioProviders>     active_audio_providers_;

  static_assert (decltype (active_midi_event_providers_)::is_always_lock_free);

  /**
   * Splits the cycle and handles recording for each slot.
   */
  static void handle_recording (
    const dsp::graph::ProcessBlockInfo &time_nfo,
    const dsp::ITransport              &transport,
    const ProcessingCaches             &processing_caches,
    const RecordingCallbackRT          &recording_cb) noexcept;
};

TrackProcessor::~TrackProcessor () = default;

TrackProcessor::TrackProcessor (
  const dsp::TempoMap                   &tempo_map,
  dsp::PortType                          signal_type,
  TrackNameProvider                      track_name_provider,
  EnabledProvider                        enabled_provider,
  Capabilities                           capabilities,
  utils::IObjectRegistry                &object_registry,
  std::optional<FillEventsCallback>      fill_events_cb,
  std::optional<TransformMidiInputsFunc> transform_midi_inputs_func,
  std::optional<AppendMidiInputsToOutputsFunc> append_midi_inputs_to_outputs_func,
  RecordingCallbackRT recording_cb,
  QObject *           parent)
    : QObject (parent),
      dsp::ProcessorBase (
        object_registry,
        utils::Utf8String::from_utf8_encoded_string (
          fmt::format ("{} Processor", track_name_provider ()))),
      is_midi_ (signal_type == dsp::PortType::Midi),
      is_audio_ (signal_type == dsp::PortType::Audio),
      capabilities_ (capabilities),
      enabled_provider_ (std::move (enabled_provider)),
      track_name_provider_ (track_name_provider),
      impl_ (std::make_unique<Impl> ())
{
  impl_->fill_events_cb_ = std::move (fill_events_cb);
  impl_->handle_recording_cb_ = std::move (recording_cb);
  impl_->append_midi_inputs_to_outputs_func_ =
    append_midi_inputs_to_outputs_func.has_value ()
      ? append_midi_inputs_to_outputs_func.value ()
      : [] (
          dsp::MidiEventVector &out_events, const dsp::MidiEventVector &in_events,
          const dsp::graph::ProcessBlockInfo &time_nfo) {
          out_events.append (
            in_events, time_nfo.buffer_offset_, time_nfo.nframes_);
        };
  impl_->transform_midi_inputs_func_ = std::move (transform_midi_inputs_func);

  impl_->clip_playback_data_provider_ =
    std::make_unique<ClipPlaybackDataProvider> (tempo_map);

  if (is_midi ())
    {
      impl_->timeline_midi_data_provider_ =
        utils::make_qobject_unique<arrangement::MidiTimelineDataProvider> (this);
      set_midi_providers_active (ActiveMidiEventProviders::Timeline, true);

      {
        add_input_port (
          utils::create_object<dsp::MidiPort> (
            registry (), u8"TP MIDI in", dsp::PortFlow::Input));
        auto * midi_in = &get_midi_in_port ();
        midi_in->set_full_designation_provider (this);
        midi_in->set_symbol (u8"track_processor_midi_in");

        add_output_port (
          utils::create_object<dsp::MidiPort> (
            registry (), u8"TP MIDI out", dsp::PortFlow::Output));
        auto * midi_out = &get_midi_out_port ();
        midi_out->set_full_designation_provider (this);
        midi_out->set_symbol (u8"track_processor_midi_out");
      }

      /* set up piano roll port */
      if (ENUM_BITSET_TEST (capabilities_, Capabilities::PianoRoll))
        {
          add_input_port (
            utils::create_object<dsp::MidiPort> (
              registry (), u8"TP Piano Roll", dsp::PortFlow::Input));
          auto &piano_roll = get_piano_roll_port ();
          piano_roll.set_full_designation_provider (this);
          piano_roll.set_symbol (u8"track_processor_piano_roll");
        }

      if (ENUM_BITSET_TEST (capabilities_, Capabilities::MidiCC))
        {
          for (const auto i : std::views::iota (0, 16))
            {
              /* starting from 1 */
              int channel = i + 1;

              for (const auto j : std::views::iota (0, 128))
                {
                  add_parameter (
                    utils::create_object<dsp::ProcessorParameter> (
                      registry (), registry (),
                      dsp::ProcessorParameter::UniqueId (
                        utils::Utf8String::from_utf8_encoded_string (
                          fmt::format (midi_cc_id_format_str, channel, j + 1))),
                      dsp::ParameterRange (
                        dsp::ParameterRange::Type::Integer, 0.f, 127.f, 0.f, 0.f),
                      utils::Utf8String::from_utf8_encoded_string (
                        fmt::format (
                          "Ch{} {}", channel,
                          utils::midi::midi_get_controller_name (j)))));
                }

              add_parameter (
                utils::create_object<dsp::ProcessorParameter> (
                  registry (), registry (),
                  dsp::ProcessorParameter::UniqueId (
                    utils::Utf8String::from_utf8_encoded_string (
                      fmt::format (pitch_bend_id_format_str, i + 1))),
                  dsp::ParameterRange (
                    dsp::ParameterRange::Type::Integer, -8192.f, 8191.f, 0.f,
                    0.f),
                  utils::Utf8String::from_utf8_encoded_string (
                    fmt::format ("Ch{} Pitch bend", i + 1))));

              add_parameter (
                utils::create_object<dsp::ProcessorParameter> (
                  registry (), registry (),
                  dsp::ProcessorParameter::UniqueId (
                    utils::Utf8String::from_utf8_encoded_string (
                      fmt::format (poly_key_pressure_id_format_str, i + 1))),
                  dsp::ParameterRange (
                    dsp::ParameterRange::Type::Integer, 0.f, 127.f, 0.f, 0.f),
                  utils::Utf8String::from_utf8_encoded_string (
                    fmt::format ("Ch{} Poly key pressure", i + 1))));

              add_parameter (
                utils::create_object<dsp::ProcessorParameter> (
                  registry (), registry (),
                  dsp::ProcessorParameter::UniqueId (
                    utils::Utf8String::from_utf8_encoded_string (
                      fmt::format (channel_pressure_id_format_str, i + 1))),
                  dsp::ParameterRange (
                    dsp::ParameterRange::Type::Integer, 0.f, 127.f, 0.f, 0.f),
                  utils::Utf8String::from_utf8_encoded_string (
                    fmt::format ("Ch{} Channel pressure", i + 1))));
            }
        } // endif has MIDI CC
    }
  else if (is_audio ())
    {
      impl_->timeline_audio_data_provider_ = utils::make_qobject_unique<
        arrangement::AudioTimelineDataProvider> (this);
      const auto init_stereo_out_ports = [&] (bool in) {
        auto port_ref = utils::create_object<dsp::AudioPort> (
          registry (), in ? u8"TP Stereo in" : u8"TP Stereo out",
          in ? dsp::PortFlow::Input : dsp::PortFlow::Output,
          dsp::AudioPort::BusLayout::Stereo, 2);
        auto * port = port_ref.get_object_as<dsp::AudioPort> ();
        port->set_full_designation_provider (this);
        port->set_symbol (
          utils::Utf8String (u8"track_processor_stereo_")
          + (in ? u8"in" : u8"out"));

        if (in)
          {
            add_input_port (port_ref);
          }
        else
          {
            add_output_port (port_ref);
          }
      };
      init_stereo_out_ports (true);
      init_stereo_out_ports (false);

      // Initialize audio providers with timeline active by default
      set_audio_providers_active (ActiveAudioProviders::Timeline, true);
    }

  if (ENUM_BITSET_TEST (capabilities_, Capabilities::AudioTrack))
    {
      add_parameter (
        utils::create_object<dsp::ProcessorParameter> (
          registry (), registry (),
          dsp::ProcessorParameter::UniqueId (
            utils::Utf8String::from_utf8_encoded_string (mono_param_id)),
          dsp::ParameterRange::make_toggle (false), u8"TP Mono Toggle"));
      impl_->mono_id_ = get_parameters ().back ().id ();

      add_parameter (
        utils::create_object<dsp::ProcessorParameter> (
          registry (), registry (),
          dsp::ProcessorParameter::UniqueId (
            utils::Utf8String::from_utf8_encoded_string (input_gain_param_id)),
          dsp::ParameterRange (
            dsp::ParameterRange::Type::GainAmplitude, 0.f, 4.f, 0.f, 1.f),
          u8"TP Input Gain"));
      impl_->input_gain_id_ = get_parameters ().back ().id ();

      add_parameter (
        utils::create_object<dsp::ProcessorParameter> (
          registry (), registry (),
          dsp::ProcessorParameter::UniqueId (
            utils::Utf8String::from_utf8_encoded_string (output_gain_param_id)),
          dsp::ParameterRange (
            dsp::ParameterRange::Type::GainAmplitude, 0.f, 4.f, 0.f, 1.f),
          u8"TP Output Gain"));
      impl_->output_gain_id_ = get_parameters ().back ().id ();

      add_parameter (
        utils::create_object<dsp::ProcessorParameter> (
          registry (), registry (),
          dsp::ProcessorParameter::UniqueId (
            utils::Utf8String::from_utf8_encoded_string (monitor_audio_param_id)),
          dsp::ParameterRange::make_enumeration ({ u8"Off", u8"On", u8"Auto" }, 2),
          u8"Monitor mode"));
      impl_->monitor_audio_id_ = get_parameters ().back ().id ();
    }

  if (ENUM_BITSET_TEST (capabilities_, Capabilities::Recording))
    {
      add_parameter (
        utils::create_object<dsp::ProcessorParameter> (
          registry (), registry (),
          dsp::ProcessorParameter::UniqueId (
            utils::Utf8String::from_utf8_encoded_string (recording_param_id)),
          dsp::ParameterRange::make_toggle (false), u8"Track record"));
      impl_->recording_id_ = get_parameters ().back ().id ();
      get_parameters ().back ().get ()->set_automatable (false);
    }

  // generate MIDI CC caches and set up mappings
  set_param_id_caches ();
  // set_midi_mappings ();
}

utils::Utf8String
TrackProcessor::get_full_designation_for_port (const dsp::Port &port) const
{
  return utils::Utf8String::from_utf8_encoded_string (
    fmt::format ("{}/{}", track_name_provider_ (), port.get_label ()));
}

bool
TrackProcessor::is_recording_armed () const
{
  if (!impl_->recording_id_.has_value ())
    return false;
  const auto &param = get_recording_param ();
  return param.range ().isToggled (param.baseValue ());
}

bool
TrackProcessor::is_recording_armed_rt () const noexcept
{
  if (impl_->processing_caches_ == nullptr || !impl_->recording_id_.has_value ())
    return false;
  const auto &param = *impl_->processing_caches_->recording_param_;
  return param.range ().isToggled (param.currentValue ());
}

void
TrackProcessor::set_recording_armed (bool armed)
{
  get_recording_param ().setBaseValue (armed ? 1.f : 0.f);
}

void
init_from (
  TrackProcessor        &obj,
  const TrackProcessor  &other,
  utils::ObjectCloneType clone_type)
{
// TODO
#if 0
  init_from (
    static_cast<dsp::ProcessorBase &> (obj),
    static_cast<const dsp::ProcessorBase &> (other), clone_type);
#endif
}

void
TrackProcessor::set_midi_providers_active (
  ActiveMidiEventProviders event_providers,
  bool                     active)
{
  auto current = impl_->active_midi_event_providers_.load ();
  if (active)
    {
      current = current | event_providers;
    }
  else
    {
      current = current & ~event_providers;
    }
  // Use a local string to avoid the stack-use-after-return issue
  z_debug (
    "Currently active MIDI event providers for {}: {}", get_node_name (),
    magic_enum::enum_flags_name (current));
  impl_->active_midi_event_providers_.store (current);
}

void
TrackProcessor::set_audio_providers_active (
  ActiveAudioProviders audio_providers,
  bool                 active)
{
  auto current = impl_->active_audio_providers_.load ();
  if (active)
    {
      current = current | audio_providers;
    }
  else
    {
      current = current & ~audio_providers;
    }
  // Use a local string to avoid the stack-use-after-return issue
  z_debug (
    "Currently active audio providers for {}: {}", get_node_name (),
    magic_enum::enum_flags_name (current));
  impl_->active_audio_providers_.store (current);
}

void
TrackProcessor::set_custom_midi_event_provider (
  MidiEventProviderProcessFunc process_func)
{
  decltype (impl_->custom_midi_event_provider_)::ScopedAccess<
    farbot::ThreadType::nonRealtime>
    custom_event_provider{ impl_->custom_midi_event_provider_ };
  *custom_event_provider = process_func;
}

// ============================================================================
// Getters
// ============================================================================

dsp::ProcessorParameter &
TrackProcessor::get_mono_param () const
{
  return utils::get_typed<dsp::ProcessorParameter> (
    registry (), *impl_->mono_id_);
}

dsp::ProcessorParameter &
TrackProcessor::get_input_gain_param () const
{
  return utils::get_typed<dsp::ProcessorParameter> (
    registry (), *impl_->input_gain_id_);
}

dsp::ProcessorParameter &
TrackProcessor::get_output_gain_param () const
{
  return utils::get_typed<dsp::ProcessorParameter> (
    registry (), *impl_->output_gain_id_);
}

dsp::ProcessorParameter &
TrackProcessor::get_monitor_audio_param () const
{
  return utils::get_typed<dsp::ProcessorParameter> (
    registry (), *impl_->monitor_audio_id_);
}

dsp::ProcessorParameter &
TrackProcessor::get_recording_param () const
{
  assert (ENUM_BITSET_TEST (capabilities_, Capabilities::Recording));
  return utils::get_typed<dsp::ProcessorParameter> (
    registry (), *impl_->recording_id_);
}

dsp::ProcessorParameter &
TrackProcessor::get_midi_cc_param (midi_byte_t channel, midi_byte_t control_no)
{
  assert (ENUM_BITSET_TEST (capabilities_, Capabilities::MidiCC));
  return utils::get_typed<dsp::ProcessorParameter> (
    registry (),
    impl_->midi_cc_caches_->midi_cc_ids_.at ((channel * 128) + control_no));
}

arrangement::AudioTimelineDataProvider &
TrackProcessor::timeline_audio_data_provider ()
{
  return *impl_->timeline_audio_data_provider_;
}

arrangement::MidiTimelineDataProvider &
TrackProcessor::timeline_midi_data_provider ()
{
  return *impl_->timeline_midi_data_provider_;
}

ClipPlaybackDataProvider &
TrackProcessor::clip_playback_data_provider ()
{
  return *impl_->clip_playback_data_provider_;
}

// ============================================================================
// handle_recording (Impl static method)
// ============================================================================

void
TrackProcessor::Impl::handle_recording (
  const dsp::graph::ProcessBlockInfo        &time_nfo,
  const dsp::ITransport                     &transport,
  const ProcessingCaches                    &processing_caches,
  const TrackProcessor::RecordingCallbackRT &recording_cb) noexcept
{
  const auto start = time_nfo.transport_position_;
  const auto end = time_nfo.end_position ();

  // The graph (process_chunks_after_splitting_at_loop_points) already
  // splits at loop boundaries, so each process_block call covers a
  // single non-loop-crossing region. We only handle punch splitting
  // here.

  // split point + nframes pairs
  boost::container::static_vector<std::pair<units::sample_t, units::sample_t>, 3>
    ranges;
  ranges.emplace_back (start, time_nfo.nframes_);

  const auto punch =
    transport.punch_enabled ()
      ? std::optional<std::pair<units::sample_t, units::sample_t>> (
          transport.get_punch_range_positions ())
      : std::nullopt;

  // Handle punch points
  if (punch.has_value ())
    {
      const bool punch_in_hit = punch->first >= start && punch->first < end;
      if (punch_in_hit)
        {
          // punch in
          ranges.emplace_back (punch->first, end - punch->first);

          // adjust frames of previous split
          ranges[0].second -= ranges[1].second;
        }
      const bool punch_out_hit = punch->second >= start && punch->second < end;
      if (punch_out_hit)
        {
          if (punch_in_hit)
            {
              // add punch out
              ranges.emplace_back (punch->second, end - punch->second);

              // adjust frames of previous split
              ranges[1].second -= ranges[2].second;
            }
          else
            {
              // add punch out
              ranges.emplace_back (punch->second, end - punch->second);

              // adjust frames of previous split
              ranges[0].second -= ranges[1].second;
            }
        }
    }

  // Process recording ranges
  for (const auto &[index, range] : utils::views::enumerate (ranges))
    {
      // skip if same as previous point
      if (index != 0 && range.first == ranges[index - 1].first)
        continue;

      // skip pause ranges (nothing to record)
      if (range.second == units::samples (0))
        continue;

      // skip ranges outside the punch window when punch is enabled
      if (punch.has_value ())
        {
          const auto range_end = range.first + range.second;
          const bool overlaps_punch =
            range.first < punch->second && range_end > punch->first;
          if (!overlaps_punch)
            continue;
        }

      const auto is_midi = processing_caches.midi_in_rt_ != nullptr;
      const auto is_audio = !processing_caches.audio_ins_rt_.empty ();
      if (is_midi)
        {
          recording_cb (
            range.first, transport,
            &processing_caches.midi_in_rt_->midi_events_.active_events_,
            std::nullopt);
        }
      else if (is_audio)
        {
          const auto &out_buf =
            processing_caches.audio_ins_rt_.front ()->buffers ();
          assert (range.first >= start);
          const auto offset =
            time_nfo.buffer_offset_.in<size_t> (units::samples)
            + (range.first - start).in<size_t> (units::samples);
          const auto recording_nframes =
            range.second.in<size_t> (units::samples);
          assert (
            offset + recording_nframes
            <= static_cast<size_t> (out_buf->getNumSamples ()));
          auto * l = out_buf->getWritePointer (0) + offset;
          auto * r =
            (processing_caches.mono_param_->range ().isToggled (
              processing_caches.mono_param_->currentValue ()))
              ? l
              : out_buf->getWritePointer (1) + offset;
          recording_cb (
            range.first, transport, nullptr,
            std::make_pair (
              std::span (l, recording_nframes),
              std::span (r, recording_nframes)));
        }
    }
}

// ============================================================================
// Private methods
// ============================================================================

void
TrackProcessor::add_events_from_midi_cc_control_ports (
  dsp::MidiEventVector     &events,
  const units::sample_u32_t local_offset)
{
  assert (ENUM_BITSET_TEST (capabilities_, Capabilities::MidiCC));

  using AddEventCallback = std::function<void (
    const dsp::ProcessorParameter &cc, dsp::MidiEventVector &vec_to_fill,
    size_t index_in_vector, units::sample_u32_t time)>;

  // returns if an event was added
  const auto add_event_for_cc_if_in_range =
    [&events, local_offset] [[nodiscard]] (
      const utils::RangeOf<dsp::ProcessorParameter::Uuid> auto &range,
      const dsp::ProcessorParameter &cc, const AddEventCallback &add_event_cb)
    -> bool {
    auto it = std::ranges::find (range, cc.get_uuid ());
    if (it != range.end ())
      {
        add_event_cb (
          cc, events, std::ranges::distance (range.begin (), it), local_offset);
        return true;
      }
    return false;
  };

  dsp::ProcessorParameter * popped_cc{};
  while (impl_->midi_cc_caches_->updated_midi_automatable_ports_.pop_front (
    popped_cc))
    {
      if (
        add_event_for_cc_if_in_range (
          impl_->midi_cc_caches_->pitch_bend_ids_, *popped_cc,
          [] (
            const dsp::ProcessorParameter &cc, dsp::MidiEventVector &vec_to_fill,
            const size_t index_in_vector, units::sample_u32_t time) {
            vec_to_fill.add_pitchbend (
              index_in_vector + 1,
              utils::math::round_to_signed_32 (
                cc.range ().convertFrom0To1 (cc.currentValue ()))
                + 0x2000,
              time);
          }))
        continue;

      if (
        add_event_for_cc_if_in_range (
          impl_->midi_cc_caches_->poly_key_pressure_ids_, *popped_cc,
          [] (
            const dsp::ProcessorParameter &cc, dsp::MidiEventVector &vec_to_fill,
            const size_t index_in_vector, units::sample_u32_t time) {
            // TODO
          }))
        continue;

      if (
        add_event_for_cc_if_in_range (
          impl_->midi_cc_caches_->channel_pressure_ids_, *popped_cc,
          [] (
            const dsp::ProcessorParameter &cc, dsp::MidiEventVector &vec_to_fill,
            const size_t index_in_vector, units::sample_u32_t time) {
            vec_to_fill.add_channel_pressure (
              index_in_vector + 1,
              (midi_byte_t) utils::math::round_to_signed_32 (
                cc.currentValue () * 127.f),
              time);
          }))
        continue;

      bool event_added{};
      for (const auto i : std::views::iota (0, 16))
        {
          /* starting from 1 */
          const midi_byte_t channel = i + 1;

          auto begin_it =
            std::next (impl_->midi_cc_caches_->midi_cc_ids_.begin (), i * 128);
          event_added = add_event_for_cc_if_in_range (
            std::ranges::subrange (begin_it, std::next (begin_it, 128)),
            *popped_cc,
            [channel] (
              const dsp::ProcessorParameter &cc,
              dsp::MidiEventVector &vec_to_fill, const size_t index_in_vector,
              units::sample_u32_t time) {
              vec_to_fill.add_control_change (
                channel, index_in_vector,
                (midi_byte_t) utils::math::round_to_signed_32 (
                  cc.currentValue () * 127.f),
                time);
            });

          if (event_added)
            break;
        }
      if (event_added)
        continue;
    }
}

void
TrackProcessor::set_param_id_caches ()
{
  if (ENUM_BITSET_TEST (capabilities_, Capabilities::MidiCC))
    {
      impl_->midi_cc_caches_ = std::make_unique<Impl::MidiCcCaches> ();
    }

  for (const auto &param_ref : get_parameters ())
    {
      const auto * param = param_ref.get ();

      const auto &id_str = type_safe::get (param->get_unique_id ()).view ();

      if (id_str == mono_param_id)
        {
          impl_->mono_id_ = param_ref.id ();
        }
      else if (id_str == input_gain_param_id)
        {
          impl_->input_gain_id_ = param_ref.id ();
        }
      else if (id_str == output_gain_param_id)
        {
          impl_->output_gain_id_ = param_ref.id ();
        }
      else if (id_str == monitor_audio_param_id)
        {
          impl_->monitor_audio_id_ = param_ref.id ();
        }
      else if (id_str == recording_param_id)
        {
          impl_->recording_id_ = param_ref.id ();
        }
      else if (
        auto midi_cc_scan_result =
          scn::scan<midi_byte_t, midi_byte_t> (id_str, midi_cc_id_format_str))
        {
          auto [midi_channel, cc_no] = midi_cc_scan_result->values ();
          --midi_channel;
          --cc_no;
          impl_->midi_cc_caches_->midi_cc_ids_[(midi_channel * 128) + cc_no] =
            param_ref.id ();
        }
      else if (
        auto pitch_bend_scan_result =
          scn::scan<midi_byte_t> (id_str, pitch_bend_id_format_str))
        {
          auto midi_channel = pitch_bend_scan_result->value ();
          --midi_channel;
          impl_->midi_cc_caches_->pitch_bend_ids_[midi_channel] =
            param_ref.id ();
        }
      else if (
        auto poly_key_pressure_scan_result =
          scn::scan<midi_byte_t> (id_str, poly_key_pressure_id_format_str))
        {
          auto midi_channel = poly_key_pressure_scan_result->value ();
          --midi_channel;
          impl_->midi_cc_caches_->poly_key_pressure_ids_[midi_channel] =
            param_ref.id ();
        }
      else if (
        auto channel_pressure_scan_result =
          scn::scan<midi_byte_t> (id_str, channel_pressure_id_format_str))
        {
          auto midi_channel = channel_pressure_scan_result->value ();
          --midi_channel;
          impl_->midi_cc_caches_->channel_pressure_ids_[midi_channel] =
            param_ref.id ();
        }
    }
}

// TODO
#if 0
void
TrackProcessor::set_midi_mappings ()
{
  if (!ENUM_BITSET_TEST (capabilities_, Capabilities::MidiCC))
    return;

  impl_->cc_mappings_ = std::make_unique<engine::session::MidiMappings> (
    registry ());

  for (const auto i : std::views::iota (0, 16))
    {
      for (const auto j : std::views::iota (0, 128))
        {
          auto   cc_port_id = impl_->midi_cc_caches_->midi_cc_ids_[(i * 128) + j];
          auto * cc_port = utils::get_typed<dsp::ProcessorParameter> (
            registry (), cc_port_id);

          /* set caches */
          // cc_port->midi_channel_ = i + 1;
          // cc_port->midi_cc_no_ = j;

          /* set model bytes for CC:
           * [0] = ctrl change + channel
           * [1] = controller
           * [2] (unused) = control */
          std::array<midi_byte_t, 3> buf{};
          buf[0] =
            (midi_byte_t) (utils::midi::MIDI_CH1_CTRL_CHANGE | (midi_byte_t) i);
          buf[1] = (midi_byte_t) j;
          buf[2] = 0;

          /* bind */
          impl_->cc_mappings_->bind_track (
            buf, { cc_port->get_uuid (), registry () }, false);
        } /* endforeach 0..127 */
    } /* endforeach 0..15 */
}
#endif

void
TrackProcessor::fill_midi_events (
  const dsp::graph::ProcessBlockInfo &time_nfo,
  const dsp::ITransport              &transport,
  dsp::MidiEventVector               &midi_events)
{
  const auto active_providers = impl_->active_midi_event_providers_.load ();

  if (ENUM_BITSET_TEST (active_providers, ActiveMidiEventProviders::Timeline))
    {
      impl_->timeline_midi_data_provider_->process_midi_events (
        time_nfo, transport.get_play_state (), midi_events);
    }
  if (
    ENUM_BITSET_TEST (active_providers, ActiveMidiEventProviders::ClipLauncher))
    {
      impl_->clip_playback_data_provider_->process_midi_events (
        time_nfo, midi_events);
    }
  if (ENUM_BITSET_TEST (active_providers, ActiveMidiEventProviders::Custom))
    {
      decltype (impl_->custom_midi_event_provider_)::ScopedAccess<
        farbot::ThreadType::realtime>
        custom_event_provider{ impl_->custom_midi_event_provider_ };
      if (custom_event_provider->has_value ())
        {
          std::invoke (custom_event_provider->value (), time_nfo, midi_events);
        }
    }
}

void
TrackProcessor::fill_audio_events (
  const dsp::graph::ProcessBlockInfo &time_nfo,
  const dsp::ITransport              &transport,
  StereoPortPair                      stereo_ports)
{
  const auto active_providers = impl_->active_audio_providers_.load ();

  if (ENUM_BITSET_TEST (active_providers, ActiveAudioProviders::Timeline))
    {
      impl_->timeline_audio_data_provider_->process_audio_events (
        time_nfo, transport.get_play_state (), stereo_ports.first,
        stereo_ports.second);
    }
  if (ENUM_BITSET_TEST (active_providers, ActiveAudioProviders::ClipLauncher))
    {
      impl_->clip_playback_data_provider_->process_audio_events (
        time_nfo, stereo_ports.first, stereo_ports.second);
    }

  // TODO: remove this and implement other audio providers (Recording, Custom)
  if (impl_->fill_events_cb_.has_value ())
    {
      std::invoke (
        *impl_->fill_events_cb_, transport, time_nfo, nullptr, stereo_ports);
    }
}

// ============================================================================
// IProcessable Interface
// ============================================================================

void
TrackProcessor::custom_process_block (
  dsp::graph::ProcessBlockInfo time_nfo,
  const dsp::ITransport       &transport,
  const dsp::TempoMap         &tempo_map) noexcept
{
  // First, clear all output
  if (is_audio ())
    {
      impl_->processing_caches_->audio_outs_rt_.front ()->buffers ()->clear ();
    }
  else if (is_midi ())
    {
      impl_->processing_caches_->midi_out_rt_->midi_events_.queued_events_
        .clear ();
    }

  if (!enabled_provider_ ())
    {
      return;
    }

  // Audio clips
  if (is_audio ())
    {
      const auto &out_buf =
        impl_->processing_caches_->audio_outs_rt_.front ()->buffers ();
      assert (out_buf->getNumChannels () >= 2);
      fill_audio_events (
        time_nfo, transport,
        std::make_pair (
          std::span (out_buf->getWritePointer (0), out_buf->getNumSamples ()),
          std::span (out_buf->getWritePointer (1), out_buf->getNumSamples ())));
    }
  // MIDI clips
  else if (ENUM_BITSET_TEST (capabilities_, Capabilities::PianoRoll))
    {
      fill_midi_events (
        time_nfo, transport,
        impl_->processing_caches_->piano_roll_rt_->midi_events_.queued_events_);
    }

  // dequeue piano roll contents into MIDI output port
  if (ENUM_BITSET_TEST (capabilities_, Capabilities::PianoRoll))
    {
      auto &pr = *impl_->processing_caches_->piano_roll_rt_;
      pr.midi_events_.dequeue (time_nfo.buffer_offset_, time_nfo.nframes_);

      /* append the midi events from piano roll to MIDI out */
      impl_->processing_caches_->midi_out_rt_->midi_events_.queued_events_.append (
        pr.midi_events_.active_events_, time_nfo.buffer_offset_,
        time_nfo.nframes_);
    }

  if (ENUM_BITSET_TEST (capabilities_, Capabilities::MidiCC))
    {
      // append midi events from modwheel and pitchbend control ports to
      // MIDI out
      add_events_from_midi_cc_control_ports (
        impl_->processing_caches_->midi_out_rt_->midi_events_.queued_events_,
        time_nfo.buffer_offset_);
    }

  /* add inputs to outputs */
  if (is_audio ())
    {
      const auto &stereo_in = impl_->processing_caches_->audio_ins_rt_[0];
      const auto &stereo_out = impl_->processing_caches_->audio_outs_rt_[0];
      const auto  input_gain = [this] () {
        const auto &input_gain_param = *impl_->processing_caches_->input_gain_;
        return input_gain_param.range ().convertFrom0To1 (
          input_gain_param.currentValue ());
      };
      const auto mono = [this] () {
        const auto &mono_param = *impl_->processing_caches_->mono_param_;
        return mono_param.range ().isToggled (mono_param.currentValue ());
      };

      // Monitor audio from input ports to output if:
      // - this track type doesn't have a monitor param (e.g. MIDI tracks
      // always pass through), or
      // - monitor is explicitly On, or
      // - monitor is Auto and the track is currently armed for recording
      const bool should_monitor = [this] () -> bool {
        if (!impl_->monitor_audio_id_.has_value ())
          return true;
        const auto &param = *impl_->processing_caches_->monitor_audio_;
        const auto  mode = param.range ().template enum_value<MonitorMode> (
          param.currentValue ());
        return mode == MonitorMode::On
               || (mode == MonitorMode::Auto && is_recording_armed_rt ());
      }();

      if (should_monitor)
        {
          const auto &in_buf = stereo_in->buffers ();
          const auto &out_buf = stereo_out->buffers ();

          utils::float_ranges::mix_product (
            { out_buf->getWritePointer (
                0, time_nfo.buffer_offset_.in<int> (units::samples)),
              time_nfo.nframes_.in (units::samples) },
            { in_buf->getReadPointer (
                0, time_nfo.buffer_offset_.in<int> (units::samples)),
              time_nfo.nframes_.in (units::samples) },
            impl_->input_gain_id_ ? input_gain () : 1.f);

          const auto &src_right_buf =
            (impl_->mono_id_ && mono ())
              ? in_buf->getWritePointer (
                  0, time_nfo.buffer_offset_.in<int> (units::samples))
              : in_buf->getWritePointer (
                  1, time_nfo.buffer_offset_.in<int> (units::samples));
          utils::float_ranges::mix_product (
            { out_buf->getWritePointer (
                1, time_nfo.buffer_offset_.in<int> (units::samples)),
              time_nfo.nframes_.in<size_t> (units::samples) },
            { src_right_buf, time_nfo.nframes_.in<size_t> (units::samples) },
            impl_->input_gain_id_ ? input_gain () : 1.f);
        }
    }
  else if (is_midi ())
    {
      // apply any transformations
      if (impl_->transform_midi_inputs_func_.has_value ())
        {
          std::invoke (
            *impl_->transform_midi_inputs_func_,
            impl_->processing_caches_->midi_in_rt_->midi_events_.active_events_);
        }

        /* process midi bindings */
        // TODO
#if 0
      if (impl_->cc_mappings_ && transport.recording_enabled ())
        {
          impl_->cc_mappings_->apply_from_cc_events (
            impl_->processing_caches_->midi_in_rt_->midi_events_.active_events_);
        }
#endif

      // append data from MIDI input -> MIDI output
      impl_->append_midi_inputs_to_outputs_func_ (
        impl_->processing_caches_->midi_out_rt_->midi_events_.queued_events_,
        impl_->processing_caches_->midi_in_rt_->midi_events_.active_events_,
        time_nfo);
    }

  if (
    !transport.has_recording_preroll_frames_remaining ()
    && transport.get_play_state () == dsp::ITransport::PlayState::Rolling
    && ENUM_BITSET_TEST (capabilities_, Capabilities::Recording)
    && is_recording_armed_rt ())
    {
      Impl::handle_recording (
        time_nfo, transport, *impl_->processing_caches_,
        impl_->handle_recording_cb_);
    }

  /* apply output gain */
  if (impl_->output_gain_id_.has_value ())
    {
      const auto &output_gain_param = *impl_->processing_caches_->output_gain_;
      const auto  output_gain = output_gain_param.range ().convertFrom0To1 (
        output_gain_param.currentValue ());

      impl_->processing_caches_->audio_outs_rt_.front ()->buffers ()->applyGain (
        time_nfo.buffer_offset_.in<int> (units::samples),
        time_nfo.nframes_.in<int> (units::samples), output_gain);
    }
}

void
TrackProcessor::custom_prepare_for_processing (
  const dsp::graph::GraphNode * node,
  units::sample_rate_t          sample_rate,
  units::sample_u32_t           max_block_length)
{
  impl_->processing_caches_ = std::make_unique<Impl::ProcessingCaches> ();

  if (is_audio ())
    {
      auto &stereo_in = get_stereo_in_port ();
      impl_->processing_caches_->audio_ins_rt_.push_back (&stereo_in);
      assert (stereo_in.num_channels () > 0);
      auto &stereo_out = get_stereo_out_port ();
      impl_->processing_caches_->audio_outs_rt_.push_back (&stereo_out);
      assert (stereo_out.num_channels () > 0);
    }
  else if (is_midi ())
    {
      impl_->processing_caches_->midi_in_rt_ = &get_midi_in_port ();
      impl_->processing_caches_->midi_out_rt_ = &get_midi_out_port ();
    }

  if (ENUM_BITSET_TEST (capabilities_, Capabilities::PianoRoll))
    {
      impl_->processing_caches_->piano_roll_rt_ = &get_piano_roll_port ();
    }

  if (impl_->mono_id_.has_value ())
    {
      impl_->processing_caches_->mono_param_ = &get_mono_param ();
    }
  if (impl_->input_gain_id_.has_value ())
    {
      impl_->processing_caches_->input_gain_ = &get_input_gain_param ();
    }
  if (impl_->output_gain_id_.has_value ())
    {
      impl_->processing_caches_->output_gain_ = &get_output_gain_param ();
    }
  if (impl_->monitor_audio_id_.has_value ())
    {
      impl_->processing_caches_->monitor_audio_ = &get_monitor_audio_param ();
    }
  if (impl_->recording_id_.has_value ())
    {
      impl_->processing_caches_->recording_param_ = &get_recording_param ();
    }
}

void
TrackProcessor::custom_release_resources ()
{
  impl_->processing_caches_.reset ();
}

// ============================================================================

void
to_json (nlohmann::json &j, const TrackProcessor &tp)
{
  to_json (j, static_cast<const dsp::ProcessorBase &> (tp));
}

void
from_json (const nlohmann::json &j, TrackProcessor &tp)
{
  from_json (j, static_cast<dsp::ProcessorBase &> (tp));

  // generate MIDI CC caches and set up mappings
  tp.set_param_id_caches ();
  // tp.set_midi_mappings ();
}
}
