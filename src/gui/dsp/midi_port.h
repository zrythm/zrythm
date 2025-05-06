// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_MIDI_PORT_H__
#define __AUDIO_MIDI_PORT_H__

#include "gui/dsp/port.h"
#include "utils/icloneable.h"

/**
 * @addtogroup dsp
 *
 * @{
 */

/**
 * @brief MIDI port specifics.
 */
class MidiPort final
    : public QObject,
      public Port,
      public ICloneable<MidiPort>,
      public zrythm::utils::serialization::ISerializable<MidiPort>
{
  Q_OBJECT
  QML_ELEMENT

public:
  MidiPort ();
  MidiPort (utils::Utf8String label, PortFlow flow);
  ~MidiPort () override;

  void process (EngineProcessTimeInfo time_nfo, bool noroll) override;

  void allocate_bufs () override;

  void clear_buffer (AudioEngine &engine) override;

  void init_after_cloning (const MidiPort &original, ObjectCloneType clone_type)
    override;

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
  qint64 last_midi_event_time_ = 0;

  /**
   * Last known MIDI status byte received.
   *
   * Used for running status (see
   * http://midi.teragonaudio.com/tech/midispec/run.htm).
   *
   * Not needed for JACK.
   */
  midi_byte_t last_midi_status_ = 0;
};

/**
 * @}
 */

#endif
