// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_MIDI_PORT_H__
#define __AUDIO_MIDI_PORT_H__

#include "common/dsp/port.h"
#include "common/utils/icloneable.h"

/**
 * @addtogroup dsp
 *
 * @{
 */

/**
 * @brief MIDI port specifics.
 */
class MidiPort final
    : public Port,
      public ICloneable<MidiPort>,
      public ISerializable<MidiPort>
{
public:
  MidiPort () = default;
  MidiPort (std::string label, PortFlow flow);
  ~MidiPort ();

#if HAVE_RTMIDI
  /**
   * Dequeue the MIDI events from the ring buffers into @ref
   * RtMidiDevice.events.
   */
  void prepare_rtmidi_events ();

  /**
   * Sums the inputs coming in from RtMidi before the port is processed.
   */
  void
  sum_data_from_rtmidi (const nframes_t start_frame, const nframes_t nframes);

  void expose_to_rtmidi (bool expose);
#endif

#if HAVE_JACK
  /**
   * Receives MIDI events from the port's exposed JACK port (if any) into the
   * port.
   */
  void receive_midi_events_from_jack (
    const nframes_t start_frames,
    const nframes_t nframes);

  /**
   * Pastes the MIDI data in the port to the JACK port.
   *
   * @note MIDI timings are assumed to be at the correct positions in the
   * current cycle (ie, already added the start_frames in this cycle).
   */
  void send_midi_events_to_jack (
    const nframes_t start_frames,
    const nframes_t nframes);
#endif

  void
  process (const EngineProcessTimeInfo time_nfo, const bool noroll) override;

  void allocate_bufs () override;

  void clear_buffer (AudioEngine &engine) override;

  void init_after_cloning (const MidiPort &original) override;

  DECLARE_DEFINE_FIELDS_METHOD ();

public:
  /**
   * Contains raw MIDI data (MIDI ports only)
   */
  MidiEvents midi_events_;

  /**
   * @brief Ring buffer for saving MIDI events to be used in the UI instead of
   * directly accessing the events.
   *
   * This should keep pushing MidiEvent's whenever they occur and the reader
   * should empty it after checking if there are any events.
   *
   * Currently there is only 1 reader for each port so this wont be a problem
   * for now, but we should have one ring for each reader.
   */
  std::unique_ptr<RingBuffer<MidiEvent>> midi_ring_;

  /**
   * @brief Whether the port has midi events not yet processed by the UI.
   */
  std::atomic<bool> has_midi_events_ = false;

  /** Used by the UI to detect when unprocessed MIDI events exist. */
  gint64 last_midi_event_time_;

  /**
   * Last known MIDI status byte received.
   *
   * Used for running status (see
   * http://midi.teragonaudio.com/tech/midispec/run.htm).
   *
   * Not needed for JACK.
   */
  midi_byte_t last_midi_status_ = 0;

  /**
   * Last time the port finished dequeueing MIDI events.
   *
   * Used for some backends only.
   */
  gint64 last_midi_dequeue_ = 0;

#if HAVE_RTMIDI
  /**
   * RtMidi pointers for input ports.
   *
   * Each RtMidi port represents a device, and this Port can be connected to
   * multiple devices.
   */
  std::vector<std::shared_ptr<RtMidiDevice>> rtmidi_ins_;

  /** RtMidi pointers for output ports. */
  std::vector<std::shared_ptr<RtMidiDevice>> rtmidi_outs_;
#else
  std::vector<void *> rtmidi_ins_;
  std::vector<void *> rtmidi_outs_;
#endif
};

/**
 * @}
 */

#endif