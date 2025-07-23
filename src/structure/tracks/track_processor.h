// SPDX-FileCopyrightText: © 2019-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/port.h"
#include "engine/session/midi_mapping.h"
#include "structure/arrangement/arranger_object_all.h"
#include "utils/icloneable.h"
#include "utils/mpmc_queue.h"
#include "utils/types.h"

namespace zrythm::structure::tracks
{
class Track;

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

public:
  using StereoPortPair = std::pair<std::span<float>, std::span<float>>;

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

  using HandleRecordingCallback =
    std::function<void (const EngineProcessTimeInfo &time_nfo)>;

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

  /**
   * Creates a new track processor for the given track.
   */
  TrackProcessor (
    const dsp::ITransport                 &transport,
    Track                                 &track,
    ProcessorBaseDependencies              dependencies,
    std::optional<FillEventsCallback>      fill_events_cb = std::nullopt,
    std::optional<TransformMidiInputsFunc> transform_midi_inputs_func =
      std::nullopt,
    std::optional<AppendMidiInputsToOutputsFunc>
      append_midi_inputs_to_outputs_func = std::nullopt);

  // TODO: remove this and the other setters - use constructor
  void set_fill_events_callback (FillEventsCallback cb)
  {
    fill_events_cb_ = std::move (cb);
  }

  void set_handle_recording_callback (HandleRecordingCallback handle_rec_cb)
  {
    handle_recording_cb_ = std::move (handle_rec_cb);
  }

  void
  set_append_midi_inputs_to_outputs_func (AppendMidiInputsToOutputsFunc func)
  {
    append_midi_inputs_to_outputs_func_ = std::move (func);
  }

  void set_transform_midi_inputs_func (TransformMidiInputsFunc func)
  {
    transform_midi_inputs_func_ = std::move (func);
  }

  bool is_audio () const { return is_audio_; }

  constexpr bool is_midi () const { return is_midi_; }

  utils::Utf8String get_full_designation_for_port (const dsp::Port &port) const;

  /**
   * Clears all buffers.
   */
  void clear_buffers (std::size_t block_length);

  /**
   * @brief To be called by the user-provided FillEventsFunc for processing one
   * of its regions.
   *
   * @param transport
   * @param time_nfo
   * @param[out] midi_events
   * @param[out] stereo_ports
   * @param r
   */
  template <arrangement::RegionObject RegionT>
  static void fill_events_from_region_rt (
    const dsp::ITransport        &transport,
    const EngineProcessTimeInfo  &time_nfo,
    dsp::MidiEventVector *        midi_events,
    std::optional<StereoPortPair> stereo_ports,
    const RegionT                &region) [[clang::nonblocking]]
    requires std::is_same_v<RegionT, arrangement::AudioRegion>
             || std::is_same_v<RegionT, arrangement::MidiRegion>
             || std::is_same_v<RegionT, arrangement::ChordRegion>
  {
    const unsigned_frame_t g_end_frames =
      time_nfo.g_start_frame_w_offset_ + time_nfo.nframes_;

    // skip region if muted (TODO: check parents)
    if (region.regionMixin ()->mute ()->muted ())
      {
        return;
      }

      /* skip if in bounce mode and the region should not be bounced */
// TODO
#if 0
        if (
          AUDIO_ENGINE->bounce_mode_ != engine::device_io::BounceMode::Off
          && (!r->bounce_ || !bounce_))
          {
            return;
          }
#endif

    /* skip if region is not hit (inclusive of its last point) */
    if (
      !region.regionMixin ()->bounds ()->is_hit_by_range (
        std::make_pair (
          (signed_frame_t) time_nfo.g_start_frame_w_offset_,
          (signed_frame_t) (midi_events ? g_end_frames : (g_end_frames - 1))),
        true))
      {
        return;
      }

    signed_frame_t num_frames_to_process = std::min (
      region.regionMixin ()->bounds ()->get_end_position_samples (true)
        - (signed_frame_t) time_nfo.g_start_frame_w_offset_,
      (signed_frame_t) time_nfo.nframes_);
    nframes_t frames_processed = 0;

    while (num_frames_to_process > 0)
      {
        unsigned_frame_t cur_g_start_frame =
          time_nfo.g_start_frame_ + frames_processed;
        unsigned_frame_t cur_g_start_frame_w_offset =
          time_nfo.g_start_frame_w_offset_ + frames_processed;
        nframes_t cur_local_start_frame =
          time_nfo.local_offset_ + frames_processed;

        auto [cur_num_frames_till_next_r_loop_or_end, is_loop_end] =
          get_frames_till_next_loop_or_end (
            region, (signed_frame_t) cur_g_start_frame_w_offset);

        const bool is_region_end =
          (signed_frame_t) time_nfo.g_start_frame_w_offset_ + num_frames_to_process
          == region.regionMixin ()->bounds ()->get_end_position_samples (true);

        const bool is_transport_end =
          transport.loop_enabled ()
          && (signed_frame_t) time_nfo.g_start_frame_w_offset_ + num_frames_to_process
               == static_cast<signed_frame_t> (
                 transport.get_loop_range_positions ().second);

        /* whether we need a note off */
        const bool need_note_off =
          (cur_num_frames_till_next_r_loop_or_end < num_frames_to_process)
          || (cur_num_frames_till_next_r_loop_or_end == num_frames_to_process && !is_loop_end)
          /* region end */
          || is_region_end
          /* transport end */
          || is_transport_end;

        /* number of frames to process this time */
        cur_num_frames_till_next_r_loop_or_end = std::min (
          cur_num_frames_till_next_r_loop_or_end, num_frames_to_process);

        const EngineProcessTimeInfo nfo = {
          .g_start_frame_ = cur_g_start_frame,
          .g_start_frame_w_offset_ = cur_g_start_frame_w_offset,
          .local_offset_ = cur_local_start_frame,
          .nframes_ =
            static_cast<nframes_t> (cur_num_frames_till_next_r_loop_or_end)
        };

        if constexpr (
          std::is_same_v<RegionT, arrangement::MidiRegion>
          || std::is_same_v<RegionT, arrangement::ChordRegion>)
          {
            assert (midi_events);
            arrangement::fill_midi_events (
              region, nfo, need_note_off,
              !is_transport_end && (is_loop_end || is_region_end), *midi_events);
          }
        else if constexpr (
          std::is_same_v<RegionT, structure::arrangement::AudioRegion>)
          {
            assert (stereo_ports);
            region.fill_stereo_ports (nfo, *stereo_ports);
          }

        frames_processed += cur_num_frames_till_next_r_loop_or_end;
        num_frames_to_process -= cur_num_frames_till_next_r_loop_or_end;
      } /* end while frames left */
  }

  // ============================================================================
  // IProcessable Interface
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
  void custom_process_block (EngineProcessTimeInfo time_nfo) override;

  // ============================================================================

  std::pair<dsp::AudioPort &, dsp::AudioPort &> get_stereo_in_ports () const
  {
    if (!is_audio ())
      {
        throw std::runtime_error ("Not an audio track processor");
      }
    auto * l = get_input_ports ().at (0).get_object_as<dsp::AudioPort> ();
    auto * r = get_input_ports ().at (1).get_object_as<dsp::AudioPort> ();
    return { *l, *r };
  }
  std::pair<dsp::AudioPort &, dsp::AudioPort &> get_stereo_out_ports () const
  {
    if (!is_audio ())
      {
        throw std::runtime_error ("Not an audio track processor");
      }
    auto * l = get_output_ports ().at (0).get_object_as<dsp::AudioPort> ();
    auto * r = get_output_ports ().at (1).get_object_as<dsp::AudioPort> ();
    return { *l, *r };
  }

  dsp::ProcessorParameter &get_mono_param () const
  {
    return *mono_id_->get_object_as<dsp::ProcessorParameter> ();
  }
  dsp::ProcessorParameter &get_input_gain_param () const
  {
    return *input_gain_id_->get_object_as<dsp::ProcessorParameter> ();
  }
  dsp::ProcessorParameter &get_output_gain_param () const
  {
    return *output_gain_id_->get_object_as<dsp::ProcessorParameter> ();
  }
  dsp::ProcessorParameter &get_monitor_audio_param () const
  {
    return *monitor_audio_id_->get_object_as<dsp::ProcessorParameter> ();
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
    return *get_input_ports ().front ().get_object_as<dsp::MidiPort> ();
  }

  /**
   * MIDI out port, if MIDI.
   */
  dsp::MidiPort &get_midi_out_port () const
  {
    return *get_output_ports ().front ().get_object_as<dsp::MidiPort> ();
  }

  /**
   * MIDI input for receiving MIDI signals from the piano roll (i.e.,
   * MIDI notes inside regions) or other sources.
   */
  dsp::MidiPort &get_piano_roll_port () const
  {
    return *piano_roll_port_id_->get_object_as<dsp::MidiPort> ();
  }
  dsp::ProcessorParameter &
  get_midi_cc_param (int channel_index, int cc_index) const
  {
    return *(*midi_cc_ids_)
              .at (static_cast<size_t> ((channel_index * 128) + cc_index))
              .get_object_as<dsp::ProcessorParameter> ();
  }
  dsp::ProcessorParameter &get_pitch_bend_param (int channel_index) const
  {
    return *(*pitch_bend_ids_)
              .at (static_cast<size_t> (channel_index))
              .get_object_as<dsp::ProcessorParameter> ();
  }
  dsp::ProcessorParameter &get_poly_key_pressure_param (int channel_index) const
  {
    return *(*poly_key_pressure_ids_)
              .at (static_cast<size_t> (channel_index))
              .get_object_as<dsp::ProcessorParameter> ();
  }
  dsp::ProcessorParameter &get_channel_pressure_param (int channel_index) const
  {
    return *(*channel_pressure_ids_)
              .at (static_cast<size_t> (channel_index))
              .get_object_as<dsp::ProcessorParameter> ();
  }

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
    dsp::MidiEventVector        &midi_events)
  {
    std::invoke (
      *fill_events_cb_, transport_, time_nfo, &midi_events, std::nullopt);
    midi_events.clear_duplicates ();
    midi_events.sort ();
  }

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
    StereoPortPair               stereo_ports)
  {
    std::invoke (*fill_events_cb_, transport_, time_nfo, nullptr, stereo_ports);
  }

private:
  static constexpr auto kMonoKey = "mono"sv;
  static constexpr auto kInputGainKey = "inputGain"sv;
  static constexpr auto kOutputGainKey = "outputGain"sv;
  static constexpr auto kMidiInKey = "midiIn"sv;
  static constexpr auto kMidiOutKey = "midiOut"sv;
  static constexpr auto kPianoRollKey = "pianoRoll"sv;
  static constexpr auto kMonitorAudioKey = "monitorAudio"sv;
  static constexpr auto kStereoInLKey = "stereoInL"sv;
  static constexpr auto kStereoInRKey = "stereoInR"sv;
  static constexpr auto kStereoOutLKey = "stereoOutL"sv;
  static constexpr auto kStereoOutRKey = "stereoOutR"sv;
  static constexpr auto kMidiCcKey = "midiCc"sv;
  static constexpr auto kPitchBendKey = "pitchBend"sv;
  static constexpr auto kPolyKeyPressureKey = "polyKeyPressure"sv;
  static constexpr auto kChannelPressureKey = "channelPressure"sv;
  friend void           to_json (nlohmann::json &j, const TrackProcessor &tp)
  {
    to_json (j, static_cast<const dsp::ProcessorBase &> (tp));
    j[kMonoKey] = tp.mono_id_;
    j[kInputGainKey] = tp.input_gain_id_;
    j[kOutputGainKey] = tp.output_gain_id_;
    j[kMonitorAudioKey] = tp.monitor_audio_id_;
    j[kMidiCcKey] = tp.midi_cc_ids_;
    j[kPitchBendKey] = tp.pitch_bend_ids_;
    j[kPolyKeyPressureKey] = tp.poly_key_pressure_ids_;
    j[kChannelPressureKey] = tp.channel_pressure_ids_;
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
  void handle_recording (const EngineProcessTimeInfo &time_nfo);

  /**
   * Adds events to midi out based on any changes in MIDI CC control ports.
   */
  [[gnu::hot]] void add_events_from_midi_cc_control_ports (
    dsp::MidiEventVector &events,
    nframes_t             local_offset);

private:
  const dsp::ITransport &transport_;

  const bool is_midi_;
  const bool is_audio_;

  const EnabledProvider   enabled_provider_;
  const TrackNameProvider track_name_provider_;

  std::optional<FillEventsCallback>      fill_events_cb_;
  std::optional<HandleRecordingCallback> handle_recording_cb_;
  AppendMidiInputsToOutputsFunc          append_midi_inputs_to_outputs_func_;
  std::optional<TransformMidiInputsFunc> transform_midi_inputs_func_;

public:
  // Cached piano roll port ID.
  std::optional<dsp::PortUuidReference> piano_roll_port_id_;

  /** Mono toggle, if audio. */
  std::optional<dsp::ProcessorParameterUuidReference> mono_id_;

  /** Input gain, if audio. */
  std::optional<dsp::ProcessorParameterUuidReference> input_gain_id_;

  /**
   * Output gain, if audio.
   *
   * This is applied after regions are processed to @ref stereo_out_.
   */
  std::optional<dsp::ProcessorParameterUuidReference> output_gain_id_;

  /**
   * Whether to monitor the audio output.
   *
   * This is only used on audio tracks. During recording, if on, the
   * recorded audio will be passed to the output. If off, the recorded audio
   * will not be passed to the output.
   *
   * When not recording, this will only take effect when paused.
   */
  std::optional<dsp::ProcessorParameterUuidReference> monitor_audio_id_;

  /* --- MIDI controls --- */

  /** Mappings to each CC port. */
  std::unique_ptr<engine::session::MidiMappings> cc_mappings_;

  /** MIDI CC control ports, 16 channels x 128 controls. */
  std::unique_ptr<std::array<dsp::ProcessorParameterUuidReference, 16_zu * 128>>
    midi_cc_ids_;

  using SixteenPortUuidArray =
    std::array<dsp::ProcessorParameterUuidReference, 16>;

  /** Pitch bend x 16 channels. */
  std::unique_ptr<SixteenPortUuidArray> pitch_bend_ids_;

  /**
   * Polyphonic key pressure (aftertouch).
   *
   * This message is most often sent by pressing down on the key after it
   * "bottoms out".
   *
   * FIXME this is completely wrong. It's supposed to be per-key, so 128 x
   * 16 ports.
   */
  std::unique_ptr<SixteenPortUuidArray> poly_key_pressure_ids_;

  /**
   * Channel pressure (aftertouch).
   *
   * This message is different from polyphonic after-touch - sends the
   * single greatest pressure value (of all the current depressed keys).
   */
  std::unique_ptr<SixteenPortUuidArray> channel_pressure_ids_;

  /* --- end MIDI controls --- */
  /**
   * Current dBFS after processing each output port.
   *
   * Transient variables only used by the GUI.
   */
  float l_port_db_ = 0.0f;
  float r_port_db_ = 0.0f;

  /**
   * A queue of MIDI CC ports whose values have been recently updated.
   *
   * This is used during processing to avoid checking every single MIDI CC
   * port for changes.
   */
  std::unique_ptr<MPMCQueue<dsp::ProcessorParameter *>>
    updated_midi_automatable_ports_;
};

}
