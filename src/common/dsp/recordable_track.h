// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_RECORDABLE_TRACK_H__
#define __AUDIO_RECORDABLE_TRACK_H__

#include "common/dsp/control_port.h"
#include "common/dsp/processable_track.h"

/**
 * Abstract class for a track that can be recorded.
 */
class RecordableTrack
    : virtual public ProcessableTrack,
      public ISerializable<RecordableTrack>
{
public:
  // Rule of 0
  RecordableTrack ();
  JUCE_DECLARE_NON_COPYABLE (RecordableTrack)
  JUCE_DECLARE_NON_MOVEABLE (RecordableTrack)

  ~RecordableTrack () override = default;

  void init_loaded () override;

  ATTR_HOT inline bool get_recording () const
  {
    return recording_->is_toggled ();
  }

  /**
   * Sets recording and connects/disconnects the JACK ports.
   */
  void set_recording (bool recording, bool fire_events);

protected:
  void copy_members_from (const RecordableTrack &other)
  {
    recording_ = other.recording_->clone_unique ();
    record_set_automatically_ = other.record_set_automatically_;
  }

  void
  append_member_ports (std::vector<Port *> &ports, bool include_plugins) const;

  DECLARE_DEFINE_BASE_FIELDS_METHOD ();

public:
  /** Recording or not. */
  std::unique_ptr<ControlPort> recording_;

  /**
   * Whether record was set automatically when the channel was selected.
   *
   * This is so that it can be unset when selecting another track. If we don't
   * do this all the tracks end up staying on record mode.
   */
  bool record_set_automatically_ = false;

  /**
   * Region currently recording on.
   *
   * This must only be set by the RecordingManager when processing an event
   * and should not be touched by anything else.
   */
  Region * recording_region_ = nullptr;

  /**
   * This is a flag to let the recording manager know that a START signal was
   * already sent for recording.
   *
   * This is because @ref recording_region_ takes a cycle or 2 to become
   * non-NULL.
   */
  bool recording_start_sent_ = false;

  /**
   * This is a flag to let the recording manager know that a STOP signal was
   * already sent for recording.i
   *
   * This is because @ref recording_region_ takes a cycle or 2 to become NULL.
   */
  bool recording_stop_sent_ = false;

  /**
   * This must only be set by the RecordingManager when temporarily pausing
   * recording, eg when looping or leaving the punch range.
   *
   * See @ref RECORDING_EVENT_TYPE_PAUSE_TRACK_RECORDING.
   */
  bool recording_paused_ = false;
};

#endif // __AUDIO_RECORDABLE_TRACK_H__
