// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

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
      public PortConnectionsCacheMixin<MidiPort>,
      public RingBufferOwningPortMixin
{
  Q_OBJECT
  QML_ELEMENT

public:
  MidiPort ();
  MidiPort (utils::Utf8String label, PortFlow flow);
  ~MidiPort () override;

  [[gnu::hot]] void process_block (EngineProcessTimeInfo time_nfo) override;

  void
  prepare_for_processing (sample_rate_t sample_rate, nframes_t max_block_length)
    override;

  void release_resources () override;

  void clear_buffer (std::size_t block_length) override;

  friend void init_from (
    MidiPort              &obj,
    const MidiPort        &other,
    utils::ObjectCloneType clone_type);

private:
  friend void to_json (nlohmann::json &j, const MidiPort &p)
  {
    to_json (j, static_cast<const Port &> (p));
  }
  friend void from_json (const nlohmann::json &j, MidiPort &p)
  {
    from_json (j, static_cast<Port &> (p));
  }

public:
  /**
   * Contains raw MIDI data (MIDI ports only)
   */
  dsp::MidiEvents midi_events_;

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
  std::unique_ptr<RingBuffer<dsp::MidiEvent>> midi_ring_;

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
