// SPDX-FileCopyrightText: © 2019, 2023-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <semaphore>

#include "engine/session/recording_event.h"
#include "structure/arrangement/automation_point.h"
#include "structure/tracks/track_all.h"
#include "utils/mpmc_queue.h"
#include "utils/object_pool.h"
#include "utils/types.h"

namespace zrythm::structure::tracks
{
class TrackProcessor;
}

#define RECORDING_MANAGER (gZrythm->recording_manager_)

namespace zrythm::engine::session
{
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
/**
 * @class RecordingManager
 *
 * Handles the recording logic for the application.
 *
 * This class is responsible for managing the recording process, including
 * handling recording events, creating new regions, and managing automation
 * points. It uses a queue to process recording events in a separate thread,
 * and provides methods for starting, pausing, and stopping recording.
 *
 * The class is designed to be thread-safe, with a binary semaphore to ensure
 * that only one thread can access the recording logic at a time.
 */
class RecordingManager : public QObject
{
public:
  using AutomationTrack = structure::tracks::AutomationTrack;
  using AutomationRegion = structure::arrangement::AutomationRegion;
  using AutomationPoint = structure::arrangement::AutomationPoint;

public:
  /**
   * Creates the event queue and starts the event loop.
   *
   * Must be called from a GTK thread.
   */
  RecordingManager (QObject * parent = nullptr);
  Q_DISABLE_COPY_MOVE (RecordingManager)
  ~RecordingManager () override;

  /**
   * Handles the recording logic inside the process cycle.
   *
   * The MidiEvents are already dequeued at this point.
   *
   * @param g_frames_start Global start frames.
   * @param nframes Number of frames to process. If this is zero, a pause will
   * be added. See @ref RECORDING_EVENT_TYPE_PAUSE_TRACK_RECORDING and
   * @ref RECORDING_EVENT_TYPE_PAUSE_AUTOMATION_RECORDING.
   */
  void handle_recording (
    structure::tracks::TrackPtrVariant track_var,
    const EngineProcessTimeInfo       &time_nfo,
    const dsp::MidiEventVector *       midi_events,
    std::optional<structure::tracks::TrackProcessor::ConstStereoPortPair>
      stereo_ports);

  Q_SLOT void process_events ();

  /**
   * Starts an unended note with the given pitch and velocity and adds it to
   * @ref midi_notes_.
   *
   * @param start_pos Position in ticks.
   * @param end_pos If this is nullopt, it will be set to 1 tick after the
   * start_pos.
   */
  void start_unended_note (
    structure::arrangement::MidiRegion &mr,
    double                              start_pos,
    std::optional<double>               end_pos,
    int                                 pitch,
    int                                 vel);

  /**
   * Returns the midi note with the given pitch from the unended notes.
   *
   * Used when recording.
   *
   * @param pitch The pitch. If -1, it returns any unended note. This is useful
   * when the loop point is met and we want to end them all.
   */
  structure::arrangement::MidiNote *
  pop_unended_note (structure::arrangement::MidiRegion &mr, int pitch);

  /**
   * Returns the automation points since the last recorded automation point
   * (if the last recorded automation point was before the current pos).
   */
  void get_aps_since_last_recorded (
    const structure::arrangement::AutomationRegion &ar,
    signed_frame_t                                  pos,
    std::vector<AutomationPoint *>                 &aps) const;

private:
  void handle_start_recording (const RecordingEvent &ev, bool is_automation);

  /**
   * Handles cases where recording events are first
   * received after pausing recording.
   *
   * This should be called on every
   * @ref RecordingEventType::RECORDING_EVENT_TYPE_MIDI,
   * @ref RecordingEventType::RECORDING_EVENT_TYPE_AUDIO and
   * @ref RecordingEventType::RECORDING_EVENT_TYPE_AUTOMATION event
   * and it will handle resume logic automatically
   * if needed.
   *
   * Adds new regions if necessary, etc.
   *
   * @return Whether pause was handled.
   *
   * @note Runs in GTK thread only.
   */
  bool handle_resume_event (const RecordingEvent &ev);

  /**
   * This is called when recording is paused (eg, when playhead is not in
   * recordable area).
   *
   * @note Runs in GTK thread only.
   */
  void handle_pause_event (const RecordingEvent &ev);

  /**
   * Creates a new automation point and deletes anything between the last
   * recorded automation point and this point.
   *
   * @note Runs in GTK thread only.
   */
  AutomationPoint * create_automation_point (
    AutomationTrack * at,
    AutomationRegion &region,
    float             val,
    float             normalized_val,
    signed_frame_t    pos_frames);

  void handle_stop_recording (bool is_automation);

  /**
   * Delete automation points since the last recorded automation point and the
   * current position (eg, when in latch mode) if the position is after the last
   * recorded ap.
   *
   * @note Runs in GTK thread only.
   */
  void delete_automation_points (
    AutomationTrack * at,
    AutomationRegion &region,
    signed_frame_t    pos_frames);

  /**
   * @note Runs in GTK thread only.
   */
  void handle_audio_event (const RecordingEvent &ev);

  /**
   * @note Runs in GTK thread only.
   */
  void handle_midi_event (const RecordingEvent &ev);

  /**
   * @note Runs in GTK thread only.
   */
  void handle_automation_event (const RecordingEvent &ev);

  static utils::Utf8String
  gen_name_for_recording_clip (const utils::Utf8String &track_name, int lane)
  {
    return utils::Utf8String::from_utf8_encoded_string (
      fmt::format (
        "{} - lane {} - recording", track_name,
        /* add 1 to get human friendly index */
        lane + 1));
  }

  std::optional<structure::arrangement::ArrangerObjectPtrVariant>
  get_recording_region_for_track (
    const structure::tracks::Track::Uuid &track_id) const;

public:
  /** Number of recordings currently in progress. */
  int num_active_recordings_ = 0;

  /** Event queue. */
  MPMCQueue<RecordingEvent *> event_queue_;

  /**
   * Memory pool of event structs to avoid real time allocation.
   *
   * FIXME: just use another filled MPMCQueue and delete this hacky ObjectPool
   * implementation.
   */
  ObjectPool<RecordingEvent> event_obj_pool_;

  /** Cloned objects before starting recording. */
  std::vector<structure::arrangement::ArrangerObjectPtrVariant>
    objects_before_start_;

  /**
   * Recorded region identifiers, to be used for creating the undoable actions.
   */
  std::vector<structure::arrangement::ArrangerObject::Uuid> recorded_ids_;

  /**
   * Unended notes started in recording with MIDI NOTE ON signal but haven't
   * received a NOTE OFF yet.
   */
  std::unordered_map<
    structure::arrangement::ArrangerObjectUuid,
    std::vector<QPointer<structure::arrangement::MidiNote>>>
    unended_notes_per_region_;

  /**
   * Cache of automation tracks in record mode, used to avoid looping over all
   * automation tracks.
   */
  std::unordered_map<
    structure::tracks::Track::Uuid,
    std::vector<QPointer<AutomationTrack>>>
    ats_in_record_mode_;

  /** Last recorded automation points. */
  std::unordered_map<
    structure::arrangement::ArrangerObjectUuid,
    QPointer<AutomationPoint>>
    last_recorded_aps_per_region_;

  /**
   * Region currently recording on.
   *
   * This must only be set when processing an event.
   */
  std::unordered_map<
    structure::tracks::Track::Uuid,
    structure::arrangement::ArrangerObject::Uuid>
    recording_region_per_track_;

  /**
   * @brief Tracks that recording stop event was sent.
   *
   * Ported from RecordableTrack. Below are old docs:
   *
   * This is a flag to let the recording manager know that a STOP signal was
   * already sent for recording.i
   *
   * This is because @ref recording_region_ takes a cycle or 2 to become NULL.
   */
  std::unordered_set<structure::tracks::Track::Uuid>
    tracks_recording_stop_was_sent_to_;

  std::unordered_set<structure::tracks::Track::Uuid>
    tracks_recording_start_was_sent_to_;

  /**
   * This must only be set by the RecordingManager when temporarily pausing
   * recording, eg when looping or leaving the punch range.
   *
   * See @ref RECORDING_EVENT_TYPE_PAUSE_TRACK_RECORDING.
   */
  std::unordered_set<structure::tracks::Track::Uuid> tracks_recording_was_paused_;

  /** Pending recorded automation points. */
  std::vector<structure::arrangement::AutomationPoint *> pending_aps_;

  bool                  currently_processing_ = false;
  std::binary_semaphore processing_sem_{ 1 };

  bool freeing_ = false;

private:
  /**
   * @brief Thread to be used for writing data in the background (TODO).
   */
  std::unique_ptr<juce::TimeSliceThread> time_slice_thread_;
};
QT_WARNING_POP

} // namespace zrythm::engine::session
