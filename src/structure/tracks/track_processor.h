// SPDX-FileCopyrightText: © 2019-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/port.h"
#include "dsp/processor_base.h"
#include "structure/arrangement/arranger_object_all.h"
#include "structure/arrangement/timeline_data_provider.h"
#include "structure/tracks/clip_playback_data_provider.h"
#include "utils/icloneable.h"
#include "utils/mpmc_queue.h"
#include "utils/types.h"

#include <farbot/RealtimeObject.hpp>

namespace zrythm::structure::tracks
{
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

  using PortType = dsp::PortType;
  using PortFlow = dsp::PortFlow;

  struct TrackProcessorProcessingCaches
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
  };

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
    const dsp::ITransport        &transport,
    const EngineProcessTimeInfo  &time_nfo,
    dsp::MidiEventVector *        midi_events,
    std::optional<StereoPortPair> stereo_ports)>;

  /**
   * @brief Callback to record the given audio or MIDI data.
   *
   * @param midi_events MIDI events that should be recorded, if any.
   * @param stereo_ports Audio data that should be recorded, if any.
   */
  using HandleRecordingCallback = std::function<void (
    const EngineProcessTimeInfo       &time_nfo,
    const dsp::MidiEventVector *       midi_events,
    std::optional<ConstStereoPortPair> stereo_ports)>;

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
    dsp::MidiEventVector        &out_events,
    const dsp::MidiEventVector  &in_events,
    const EngineProcessTimeInfo &time_nfo)>;

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

  using MidiEventProviderProcessFunc = std::function<void (
    const EngineProcessTimeInfo &time_nfo,
    dsp::MidiEventVector        &output_buffer)>;

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
   * @param generates_midi_events Whether this processor requires a "piano roll"
   * port to generate MIDI events into (midi, instrument and chord tracks should
   * set this to true).
   */
  TrackProcessor (
    const dsp::TempoMap                   &tempo_map,
    PortType                               signal_type,
    TrackNameProvider                      track_name_provider,
    EnabledProvider                        enabled_provider,
    bool                                   generates_midi_events,
    bool                                   has_midi_cc,
    bool                                   is_audio_track,
    ProcessorBaseDependencies              dependencies,
    std::optional<FillEventsCallback>      fill_events_cb = std::nullopt,
    std::optional<TransformMidiInputsFunc> transform_midi_inputs_func =
      std::nullopt,
    std::optional<AppendMidiInputsToOutputsFunc>
              append_midi_inputs_to_outputs_func = std::nullopt,
    QObject * parent = nullptr);

  // note: this should eventually be passed from the constructor
  void set_handle_recording_callback (HandleRecordingCallback handle_rec_cb)
  {
    handle_recording_cb_ = std::move (handle_rec_cb);
  }

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
    EngineProcessTimeInfo  time_nfo,
    const dsp::ITransport &transport) noexcept override;

  void custom_prepare_for_processing (
    const dsp::graph::GraphNode * node,
    units::sample_rate_t          sample_rate,
    nframes_t                     max_block_length) override;

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

  dsp::ProcessorParameter &get_mono_param () const
  {
    return *std::get<dsp::ProcessorParameter *> (
      dependencies ().param_registry_.find_by_id_or_throw (*mono_id_));
  }
  dsp::ProcessorParameter &get_input_gain_param () const
  {
    return *std::get<dsp::ProcessorParameter *> (
      dependencies ().param_registry_.find_by_id_or_throw (*input_gain_id_));
  }
  dsp::ProcessorParameter &get_output_gain_param () const
  {
    return *std::get<dsp::ProcessorParameter *> (
      dependencies ().param_registry_.find_by_id_or_throw (*output_gain_id_));
  }
  dsp::ProcessorParameter &get_monitor_audio_param () const
  {
    return *std::get<dsp::ProcessorParameter *> (
      dependencies ().param_registry_.find_by_id_or_throw (*monitor_audio_id_));
  }

  /**
   * @brief
   *
   * @param channel Channel (0-15)
   * @param control_no Control (0-127)
   */
  dsp::ProcessorParameter &
  get_midi_cc_param (midi_byte_t channel, midi_byte_t control_no)
  {
    assert (has_midi_cc_);
    return *std::get<dsp::ProcessorParameter *> (
      dependencies ().param_registry_.find_by_id_or_throw (
        midi_cc_caches_->midi_cc_ids_.at ((channel * 128) + control_no)));
  }

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

  auto &timeline_audio_data_provider ()
  {
    return *timeline_audio_data_provider_;
  }
  auto &timeline_midi_data_provider () { return *timeline_midi_data_provider_; }

  auto &clip_playback_data_provider () { return *clip_playback_data_provider_; }

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
    const EngineProcessTimeInfo &time_nfo,
    const dsp::ITransport       &transport,
    dsp::MidiEventVector        &midi_events);

  /**
   * Wrapper for audio tracks to fill in StereoPorts from the timeline data.
   *
   * @note The engine splits the cycle so transport loop related logic is not
   * needed.
   *
   * @param stereo_ports StereoPorts to fill.
   */
  void fill_audio_events (
    const EngineProcessTimeInfo &time_nfo,
    const dsp::ITransport       &transport,
    StereoPortPair               stereo_ports);

private:
  friend void to_json (nlohmann::json &j, const TrackProcessor &tp)
  {
    to_json (j, static_cast<const dsp::ProcessorBase &> (tp));
  }
  friend void from_json (const nlohmann::json &j, TrackProcessor &tp);

  friend void init_from (
    TrackProcessor        &obj,
    const TrackProcessor  &other,
    utils::ObjectCloneType clone_type);

  /**
   * Splits the cycle and handles recording for each
   * slot.
   */
  void handle_recording (
    const EngineProcessTimeInfo &time_nfo,
    const dsp::ITransport       &transport);

  /**
   * Adds events to midi out based on any changes in MIDI CC control ports.
   */
  [[gnu::hot]] void add_events_from_midi_cc_control_ports (
    dsp::MidiEventVector &events,
    nframes_t             local_offset);

  /**
   * @brief Set the cached IDs for various parameters for quick access.
   */
  void set_param_id_caches ();

// TODO
#if 0
  void set_midi_mappings ();
#endif

private:
  const bool is_midi_;
  const bool is_audio_;
  const bool has_piano_roll_port_;
  const bool has_midi_cc_;

  const EnabledProvider   enabled_provider_;
  const TrackNameProvider track_name_provider_;

  std::optional<FillEventsCallback>      fill_events_cb_;
  std::optional<HandleRecordingCallback> handle_recording_cb_;
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
   * Whether to monitor the audio output.
   *
   * This is only used on audio tracks. During recording, if on, the
   * recorded audio will be passed to the output. If off, the recorded audio
   * will not be passed to the output.
   *
   * When not recording, this will only take effect when paused.
   */
  std::optional<dsp::ProcessorParameter::Uuid> monitor_audio_id_;

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
    std::array<dsp::ProcessorParameter::Uuid, 16_zu * 128> midi_cc_ids_;

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
  std::unique_ptr<TrackProcessorProcessingCaches> processing_caches_;

  /**
   * @brief MIDI data provider from the timeline.
   */
  std::unique_ptr<arrangement::MidiTimelineDataProvider>
    timeline_midi_data_provider_;

  /**
   * @brief MIDI data provider from the timeline.
   */
  std::unique_ptr<arrangement::AudioTimelineDataProvider>
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
};
}

ENUM_ENABLE_BITSET (
  zrythm::structure::tracks::TrackProcessor::ActiveMidiEventProviders);
