// SPDX-FileCopyrightText: © 2019-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/port.h"
#include "dsp/port_connections_manager.h"
#include "engine/session/midi_mapping.h"
#include "gui/dsp/plugin.h"
#include "utils/icloneable.h"
#include "utils/mpmc_queue.h"
#include "utils/types.h"

namespace zrythm::structure::tracks
{
class ProcessableTrack;

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
  /**
   * Creates a new track processor for the given track.
   */
  TrackProcessor (
    ProcessableTrack         &track,
    ProcessorBaseDependencies dependencies);

  bool is_audio () const;

  bool is_midi () const;

  utils::Utf8String get_full_designation_for_port (const dsp::Port &port) const;

  ProcessableTrack * get_track () const
  {
    z_return_val_if_fail (track_, nullptr);
    return track_;
  }

  /**
   * Clears all buffers.
   */
  void clear_buffers (std::size_t block_length);

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
    fill_events_common (time_nfo, &midi_events, std::nullopt);
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
    const EngineProcessTimeInfo                  &time_nfo,
    std::pair<std::span<float>, std::span<float>> stereo_ports)
  {
    fill_events_common (time_nfo, nullptr, stereo_ports);
  }

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
    return *get_input_ports ().at (1).get_object_as<dsp::MidiPort> ();
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

private:
  /**
   * Common logic for audio and MIDI/instrument tracks to fill in MidiEvents
   * or StereoPorts from the timeline data.
   *
   * @note The engine splits the cycle so transport loop related logic is not
   * needed.
   *
   * @param stereo_ports StereoPorts to fill.
   * @param midi_events MidiEvents to fill (from Piano Roll Port for example).
   */
  void fill_events_common (
    const EngineProcessTimeInfo                                 &time_nfo,
    dsp::MidiEventVector *                                       midi_events,
    std::optional<std::pair<std::span<float>, std::span<float>>> stereo_ports)
    const;

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
  [[gnu::hot]] void
  add_events_from_midi_cc_control_ports (nframes_t local_offset);

public:
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

  /** Pointer to owner track, if any. */
  ProcessableTrack * track_ = nullptr;

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
