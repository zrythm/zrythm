// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Track processor.
 */

#ifndef __AUDIO_TRACK_PROCESSOR_H__
#define __AUDIO_TRACK_PROCESSOR_H__

#include "gui/dsp/midi_mapping.h"
#include "gui/dsp/port.h"

#include "utils/icloneable.h"
#include "utils/mpmc_queue.h"
#include "utils/types.h"

class StereoPorts;
class ControlPort;
class MidiPort;
class ProcessableTrack;
struct EngineProcessTimeInfo;

/**
 * @addtogroup dsp
 *
 * @{
 */

constexpr int TRACK_PROCESSOR_MAGIC = 81213128;
#define IS_TRACK_PROCESSOR(tr) ((tr) && (tr)->magic_ == TRACK_PROCESSOR_MAGIC)

/**
 * A TrackProcessor is a processor that is used as the first entry point when
 * processing a track.
 */
class TrackProcessor final
    : public ICloneable<TrackProcessor>,
      public zrythm::utils::serialization::ISerializable<TrackProcessor>
{
public:
  TrackProcessor () = default;

  /**
   * Creates a new track processor for the given* track.
   */
  TrackProcessor (ProcessableTrack * track);

  bool is_in_active_project () const;

  PortConnectionsManager * get_port_connections_manager () const;

  inline ProcessableTrack * get_track () const
  {
    z_return_val_if_fail (track_, nullptr);
    return track_;
  }

  /**
   * Inits a TrackProcessor after a project is loaded.
   */
  void init_loaded (ProcessableTrack * track);

#if 0
  /**
   * Copy port values from @ref src to this TrackProcessor.
   */
  void copy_values (TrackProcessor * src);
#endif

  /**
   * Clears all buffers.
   */
  void clear_buffers ();

  /**
   * Disconnects all ports connected to the TrackProcessor.
   */
  void disconnect_all ();

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
   *
   * @param g_start_frames The global start frames.
   * @param local_offset The local start frames.
   * @param nframes The number of frames to process.
   */
  void process (const EngineProcessTimeInfo &time_nfo);

  /**
   * Disconnect the TrackProcessor's stereo out ports
   * from the prefader.
   *
   * Used when there is no plugin in the channel.
   */
  void disconnect_from_prefader ();

  /**
   * Connects the TrackProcessor's stereo out ports to the Channel's prefader in
   * ports.
   *
   * Used when deleting the only plugin left.
   */
  void connect_to_prefader ();

  /**
   * Disconnect the TrackProcessor's out ports from the Plugin's input ports.
   */
  void disconnect_from_plugin (zrythm::gui::dsp::plugins::Plugin &pl);

  /**
   * Connect the TrackProcessor's out ports to the Plugin's input ports.
   */
  void connect_to_plugin (zrythm::gui::dsp::plugins::Plugin &pl);

  void append_ports (std::vector<Port *> &ports);

  void init_after_cloning (const TrackProcessor &other) override;

  DECLARE_DEFINE_FIELDS_METHOD ();

private:
  void init_common ();

  /**
   * Inits the MIDI In port of the Channel while exposing it to JACK.
   *
   * This assumes the caller already checked that this channel should have the
   * given MIDI port enabled.
   *
   * @param in True if input (false for output).
   */
  void init_midi_port (bool in);
  void init_midi_cc_ports (bool);

  /**
   * Inits the stereo ports of the Channel while exposing them to the backend.
   *
   * This assumes the caller already checked that this channel should have the
   * given ports enabled.
   *
   * @param in Whether input (false for output).
   */
  void init_stereo_out_ports (bool in);

  /**
   * Splits the cycle and handles recording for each
   * slot.
   */
  void handle_recording (const EngineProcessTimeInfo &time_nfo);

  /**
   * Adds events to midi out based on any changes in MIDI CC control ports.
   */
  ATTR_HOT void
  add_events_from_midi_cc_control_ports (const nframes_t local_offset);

public:
  /**
   * L & R audio input ports, if audio.
   */
  std::unique_ptr<StereoPorts> stereo_in_;

  /** Mono toggle, if audio. */
  std::unique_ptr<ControlPort> mono_;

  /** Input gain, if audio. */
  std::unique_ptr<ControlPort> input_gain_;

  /**
   * Output gain, if audio.
   *
   * This is applied after regions are processed to @ref stereo_out_.
   */
  std::unique_ptr<ControlPort> output_gain_;

  /**
   * L & R audio output ports, if audio.
   */
  std::unique_ptr<StereoPorts> stereo_out_;

  /**
   * MIDI in Port.
   *
   * This port is for receiving MIDI signals from an external MIDI source.
   *
   * This is also where piano roll, midi in and midi manual press will be
   * routed to and this will be the port used to pass midi to the plugins.
   */
  std::unique_ptr<MidiPort> midi_in_;

  /**
   * MIDI out port, if MIDI.
   */
  std::unique_ptr<MidiPort> midi_out_;

  /**
   * MIDI input for receiving MIDI signals from the piano roll (i.e., MIDI
   * notes inside regions) or other sources.
   *
   * This will not be a separately exposed port during processing. It will
   * be processed by the TrackProcessor internally.
   */
  std::unique_ptr<MidiPort> piano_roll_;

  /**
   * Whether to monitor the audio output.
   *
   * This is only used on audio tracks. During recording, if on, the
   * recorded audio will be passed to the output. If off, the recorded audio
   * will not be passed to the output.
   *
   * When not recording, this will only take effect when paused.
   */
  std::unique_ptr<ControlPort> monitor_audio_;

  /* --- MIDI controls --- */

  /** Mappings to each CC port. */
  std::unique_ptr<MidiMappings> cc_mappings_;

  /** MIDI CC control ports, 16 channels x 128 controls. */
  std::vector<std::unique_ptr<ControlPort>> midi_cc_;

  /** Pitch bend x 16 channels. */
  std::vector<std::unique_ptr<ControlPort>> pitch_bend_;

  /**
   * Polyphonic key pressure (aftertouch).
   *
   * This message is most often sent by pressing down on the key after it
   * "bottoms out".
   *
   * FIXME this is completely wrong. It's supposed to be per-key, so 128 x
   * 16 ports.
   */
  std::vector<std::unique_ptr<ControlPort>> poly_key_pressure_;

  /**
   * Channel pressure (aftertouch).
   *
   * This message is different from polyphonic after-touch - sends the
   * single greatest pressure value (of all the current depressed keys).
   */
  std::vector<std::unique_ptr<ControlPort>> channel_pressure_;

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
   * To be set to true when a panic (all notes off) message should be sent
   * during processing.
   *
   * Only applies to tracks that receive MIDI input.
   */
  bool pending_midi_panic_ = false;

  /**
   * A queue of MIDI CC ports whose values have been recently updated.
   *
   * This is used during processing to avoid checking every single MIDI CC
   * port for changes.
   */
  std::unique_ptr<MPMCQueue<ControlPort *>> updated_midi_automatable_ports_;

  int magic_ = TRACK_PROCESSOR_MAGIC;
};

/**
 * @}
 */

#endif
