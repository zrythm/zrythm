// SPDX-FileCopyrightText: © 2019-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <memory>

#include "dsp/port.h"
#include "dsp/processor_base.h"
#include "utils/icloneable.h"

namespace zrythm::structure::arrangement
{
class MidiTimelineDataProvider;
class AudioTimelineDataProvider;
}

namespace zrythm::structure::tracks
{
class ClipPlaybackDataProvider;

/**
 * A TrackProcessor is a standalone processor that is used as the first step
 * when processing a track in the DSP graph.
 *
 * @see Channel and ChannelTrack.
 */
class TrackProcessor final : public QObject, public dsp::ProcessorBase
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")
  Q_DISABLE_COPY_MOVE (TrackProcessor)

public:
  using StereoPortPair = std::pair<std::span<float>, std::span<float>>;
  using ConstStereoPortPair =
    std::pair<std::span<const float>, std::span<const float>>;

  /**
   * @brief Function called during processing to fill events.
   *
   * For regions, a helper method is provided for filling events from a single
   * region. Implementations of this are supposed to call that for all their
   * regions (including muted/disabled/etc.).
   */
  using FillEventsCallback = std::function<void (
    const dsp::ITransport              &transport,
    const dsp::graph::ProcessBlockInfo &time_nfo,
    dsp::MidiEventVector *              midi_events,
    std::optional<StereoPortPair>       stereo_ports)>;

  /**
   * @brief Callback to record the given audio or MIDI data.
   *
   * @param timeline_position The absolute timeline position of this data.
   * @param transport Transport state (for recording_enabled check).
   * @param midi_events MIDI events that should be recorded, if any.
   * @param stereo_ports Audio data that should be recorded, if any.
   */
  using RecordingCallbackRT = std::function<void (
    units::sample_t                    timeline_position,
    const dsp::ITransport             &transport,
    const dsp::MidiEventVector *       midi_events,
    std::optional<ConstStereoPortPair> stereo_ports,
    units::sample_u32_t                nframes)>;

  /**
   * @brief Custom logic to use when appending the MIDI input events to the
   * output events.
   *
   * If not explicitly set, this will simply copy the events over.
   *
   * Mainly used by ChordTrack to transform input MIDI notes into multiple
   * output notes that match the chord.
   */
  using AppendMidiInputsToOutputsFunc = std::function<void (
    dsp::MidiEventVector               &out_events,
    const dsp::MidiEventVector         &in_events,
    const dsp::graph::ProcessBlockInfo &time_nfo)>;

  /**
   * @brief Function to transform the given MIDI inputs.
   *
   * This is used to, for example, change the channel of all events.
   */
  using TransformMidiInputsFunc = std::function<void (dsp::MidiEventVector &)>;

  /**
   * @brief Function that returns if the track for this processor is enabled.
   *
   * Used to conditionally skip processing if disabled.
   */
  using EnabledProvider = std::function<bool ()>;

  using TrackNameProvider = std::function<utils::Utf8String ()>;

  enum class Capabilities : uint8_t
  {
    PianoRoll = 1 << 0,
    MidiCC = 1 << 1,
    AudioTrack = 1 << 2,
    Recording = 1 << 3,
  };

  enum class MonitorMode : uint8_t
  {
    Off,
    On,
    Auto,
  };

  using MidiEventProviderProcessFunc = std::function<void (
    const dsp::graph::ProcessBlockInfo &time_nfo,
    dsp::MidiEventVector               &output_buffer)>;

  enum class ActiveMidiEventProviders : uint8_t
  {
    Timeline = 1 << 0,
    ClipLauncher = 1 << 1,
    PianoRoll = 1 << 2,
    Recording = 1 << 3,
    Custom = 1 << 4,
  };

  enum class ActiveAudioProviders : uint8_t
  {
    Timeline = 1 << 0,
    ClipLauncher = 1 << 1,
    Recording = 1 << 2,
    Custom = 1 << 3,
  };

  /**
   * Creates a new track processor for the given track.
   *
   * @param capabilities Flags describing what features this processor supports.
   */
  TrackProcessor (
    const dsp::TempoMap                   &tempo_map,
    dsp::PortType                          signal_type,
    TrackNameProvider                      track_name_provider,
    EnabledProvider                        enabled_provider,
    Capabilities                           capabilities,
    utils::IObjectRegistry                &registry,
    std::optional<FillEventsCallback>      fill_events_cb = std::nullopt,
    std::optional<TransformMidiInputsFunc> transform_midi_inputs_func =
      std::nullopt,
    std::optional<AppendMidiInputsToOutputsFunc>
                        append_midi_inputs_to_outputs_func = std::nullopt,
    RecordingCallbackRT recording_cb =
      [] (
        units::sample_t,
        const dsp::ITransport &,
        const dsp::MidiEventVector *,
        std::optional<ConstStereoPortPair>,
        units::sample_u32_t) { },
    QObject * parent = nullptr);
  ~TrackProcessor () override;

  bool is_audio () const { return is_audio_; }

  constexpr bool is_midi () const { return is_midi_; }

  utils::Utf8String get_full_designation_for_port (const dsp::Port &port) const;

  // ============================================================================
  // ProcessorBase Interface
  // ============================================================================

  /**
   * Process the TrackProcessor.
   *
   * This function performs the following:
   * - produce output audio/MIDI into stereo out or midi out, based on any
   *   audio/MIDI regions, if has piano roll or is audio track
   * - produce additional output MIDI events based on any MIDI CC automation
   *   regions, if applicable
   * - change MIDI CC control port values based on any MIDI input, if recording
   *   --- at this point the output is ready ---
   * - handle recording (create events in regions and automation, including
   *   MIDI CC automation, based on the MIDI CC control ports)
   */
  void custom_process_block (
    dsp::graph::ProcessBlockInfo time_nfo,
    const dsp::ITransport       &transport,
    const dsp::TempoMap         &tempo_map) noexcept override;

  void custom_prepare_for_processing (
    const dsp::graph::GraphNode * node,
    units::sample_rate_t          sample_rate,
    units::sample_u32_t           max_block_length) override;

  void custom_release_resources () override;

  // ============================================================================

  dsp::AudioPort &get_stereo_in_port () const
  {
    assert (is_audio ());
    return *get_input_ports ().front ().get_object_as<dsp::AudioPort> ();
  }
  dsp::AudioPort &get_stereo_out_port () const
  {
    assert (is_audio ());
    return *get_output_ports ().front ().get_object_as<dsp::AudioPort> ();
  }

  dsp::ProcessorParameter &get_mono_param () const;
  dsp::ProcessorParameter &get_input_gain_param () const;
  dsp::ProcessorParameter &get_output_gain_param () const;
  dsp::ProcessorParameter &get_monitor_audio_param () const;
  dsp::ProcessorParameter &get_recording_param () const;

  bool is_recording_armed () const;
  bool is_recording_armed_rt () const noexcept [[clang::nonblocking]];
  void set_recording_armed (bool armed);

  /**
   * @brief
   *
   * @param channel Channel (0-15)
   * @param control_no Control (0-127)
   */
  dsp::ProcessorParameter &
  get_midi_cc_param (midi_byte_t channel, midi_byte_t control_no);

  /**
   * MIDI in Port.
   *
   * This port is for receiving MIDI signals from an external MIDI source.
   *
   * This is also where piano roll, midi in and midi manual press will be
   * routed to and this will be the port used to pass midi to the plugins.
   */
  dsp::MidiPort &get_midi_in_port () const
  {
    assert (is_midi ());
    return *get_input_ports ().front ().get_object_as<dsp::MidiPort> ();
  }

  /**
   * MIDI out port, if MIDI.
   */
  dsp::MidiPort &get_midi_out_port () const
  {
    assert (is_midi ());
    return *get_output_ports ().front ().get_object_as<dsp::MidiPort> ();
  }

  /**
   * MIDI input for receiving MIDI signals from the piano roll (i.e.,
   * MIDI notes inside regions) or other sources.
   *
   * This bypasses any MIDI transformations by @ref transform_midi_inputs_func_.
   */
  dsp::MidiPort &get_piano_roll_port () const
  {
    return *get_input_ports ().at (1).get_object_as<dsp::MidiPort> ();
  }

  arrangement::AudioTimelineDataProvider &timeline_audio_data_provider ();
  arrangement::MidiTimelineDataProvider  &timeline_midi_data_provider ();

  ClipPlaybackDataProvider &clip_playback_data_provider ();

  /**
   * @brief Used to enable or disable MIDI event providers.
   *
   * For example, when a clip is launched this should be called once to disable
   * the timeline and once more to enable the clip provider.
   *
   * @note Changes will take effect during the next processing cycle.
   */
  void set_midi_providers_active (
    ActiveMidiEventProviders event_providers,
    bool                     active);

  /**
   * @brief Used to enable or disable audio providers.
   *
   * For example, when a clip is launched this should be called once to disable
   * the timeline and once more to enable the clip provider.
   *
   * @note Changes will take effect during the next processing cycle.
   */
  void
  set_audio_providers_active (ActiveAudioProviders audio_providers, bool active);

  /**
   * @brief Replaces the "Custom" MIDI event provider.
   */
  void
  set_custom_midi_event_provider (MidiEventProviderProcessFunc process_func);

  /**
   * Wrapper for MIDI/instrument/chord tracks to fill in MidiEvents from the
   * timeline data.
   *
   * @note The engine splits the cycle so transport loop related logic is not
   * needed.
   *
   * @param midi_events MidiEvents to fill.
   */
  void fill_midi_events (
    const dsp::graph::ProcessBlockInfo &time_nfo,
    const dsp::ITransport              &transport,
    dsp::MidiEventVector               &midi_events);

  /**
   * Wrapper for audio tracks to fill in StereoPorts from the timeline data.
   *
   * @note The engine splits the cycle so transport loop related logic is not
   * needed.
   *
   * @param stereo_ports StereoPorts to fill.
   */
  void fill_audio_events (
    const dsp::graph::ProcessBlockInfo &time_nfo,
    const dsp::ITransport              &transport,
    StereoPortPair                      stereo_ports);

private:
  friend void to_json (nlohmann::json &j, const TrackProcessor &tp);
  friend void from_json (const nlohmann::json &j, TrackProcessor &tp);

  friend void init_from (
    TrackProcessor        &obj,
    const TrackProcessor  &other,
    utils::ObjectCloneType clone_type);

  /**
   * Adds events to midi out based on any changes in MIDI CC control ports.
   */
  [[gnu::hot]] void add_events_from_midi_cc_control_ports (
    dsp::MidiEventVector &events,
    units::sample_u32_t   local_offset);

  /**
   * @brief Set the cached IDs for various parameters for quick access.
   */
  void set_param_id_caches ();

// TODO
#if 0
  void set_midi_mappings ();
#endif

private:
  const bool         is_midi_;
  const bool         is_audio_;
  const Capabilities capabilities_;

  const EnabledProvider   enabled_provider_;
  const TrackNameProvider track_name_provider_;

  struct Impl;
  std::unique_ptr<Impl> impl_;
};
}

ENUM_ENABLE_BITSET (
  zrythm::structure::tracks::TrackProcessor::ActiveMidiEventProviders);
ENUM_ENABLE_BITSET (zrythm::structure::tracks::TrackProcessor::Capabilities);
