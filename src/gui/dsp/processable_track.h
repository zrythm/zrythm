// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_PROCESSABLE_TRACK_H__
#define __AUDIO_PROCESSABLE_TRACK_H__

#include "gui/dsp/automatable_track.h"
#include "gui/dsp/track_processor.h"

class TrackProcessor;

/**
 * The ProcessableTrack class is the base class for all processable
 * tracks.
 *
 * @ref processor_ is the starting point when processing a Track.
 */
class ProcessableTrack
    : virtual public AutomatableTrack,
      public zrythm::utils::serialization::ISerializable<ProcessableTrack>
{
public:
  // Rule of 0
  ProcessableTrack ();

  ~ProcessableTrack () override = default;
  JUCE_DECLARE_NON_COPYABLE (ProcessableTrack)
  JUCE_DECLARE_NON_MOVEABLE (ProcessableTrack)

  void init_loaded () override;

  /**
   * Returns whether monitor audio is on.
   */
  bool get_monitor_audio () const;

  /**
   * Sets whether monitor audio is on.
   */
  void set_monitor_audio (bool monitor, bool auto_select, bool fire_events);

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
    MidiEventVector             &midi_events);

  void process_block (EngineProcessTimeInfo time_nfo) override;

protected:
  /**
   * Common logic for audio and MIDI/instrument tracks to fill in MidiEvents or
   * StereoPorts from the timeline data.
   *
   * @note The engine splits the cycle so transport loop related logic is not
   * needed.
   *
   * @param stereo_ports StereoPorts to fill.
   * @param midi_events MidiEvents to fill (from Piano Roll Port for example).
   */
  void fill_events_common (
    const EngineProcessTimeInfo &time_nfo,
    MidiEventVector *            midi_events,
    StereoPorts *                stereo_ports) const;

  void copy_members_from (const ProcessableTrack &other);

  void
  append_member_ports (std::vector<Port *> &ports, bool include_plugins) const;

  DECLARE_DEFINE_BASE_FIELDS_METHOD ();

public:
  /**
   * The TrackProcessor, used for processing.
   *
   * This is the starting point when processing a Track.
   */
  std::unique_ptr<TrackProcessor> processor_;
};

using ProcessableTrackVariant = std::variant<
  InstrumentTrack,
  MidiTrack,
  MasterTrack,
  MidiGroupTrack,
  AudioGroupTrack,
  MidiBusTrack,
  AudioBusTrack,
  AudioTrack,
  ChordTrack,
  ModulatorTrack,
  TempoTrack>;
using ProcessableTrackPtrVariant = to_pointer_variant<ProcessableTrackVariant>;

#endif // __AUDIO_PROCESSABLE_TRACK_H__
