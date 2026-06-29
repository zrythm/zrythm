// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <cstdint>
#include <deque>
#include <functional>
#include <unordered_map>

#include "controllers/recording_coordinator.h"
#include "controllers/recording_mode.h"
#include "structure/arrangement/arranger_object_all.h"
#include "undo/undo_stack.h"
#include "utils/audio.h"
#include "utils/qt.h"

#include <QObject>

namespace zrythm::controllers
{

/**
 * @brief Converts raw recording packets into arrangement clips.
 *
 * Subscribes to RecordingCoordinator::audioDataReady and midiDataReady to
 * create or expand clips for each track. For audio, the first packet for a
 * given continuous range creates a new AudioClip via the ClipCreator
 * callback; subsequent packets append frames to the clip's clip. For MIDI,
 * note-on/off pairs become MidiNote objects and CC/pitchbend/etc. become
 * MidiControlEvent objects inside a MidiClip.
 *
 * All clip creations within a single recording take (transport rolling to
 * transport stopped) are wrapped in a single undo macro so the entire take
 * can be undone in one step. The macro is finalized when
 * RecordingCoordinator::recordingSessionEnded fires.
 *
 * Operates entirely on the non-RT (main) thread. The UndoStack is held via
 * QPointer to guard against unexpected destruction ordering.
 */
class RecordingMaterializer : public QObject
{
  Q_OBJECT

public:
  using RecordingMode = recording::RecordingMode;

  /** Returned by clip creator callbacks on success. */
  struct CreatedClip
  {
    structure::arrangement::ArrangerObjectUuidReference clip;
    size_t                                              actual_lane_index{};
  };
  using ClipCreationResult = std::optional<CreatedClip>;

  /** Called to query the current recording mode (Takes, TakesMuted, etc.). */
  using RecordingModeProvider = std::function<RecordingMode ()>;

  /**
   * @brief Injected callbacks for creating arranger objects.
   *
   * Positions for MIDI callbacks (midi_note, midi_control_event) are
   * clip-relative sample positions.
   */
  struct ArrangerObjectCreators
  {
    /** Creates an AudioClip with initial audio frames. */
    std::function<ClipCreationResult (
      structure::tracks::TrackUuid     track_id,
      units::sample_t                  start_position,
      const utils::audio::AudioBuffer &initial_frames,
      size_t                           lane_index)>
      audio_clip;

    /** Creates an empty MidiClip. */
    std::function<ClipCreationResult (
      structure::tracks::TrackUuid track_id,
      units::sample_t              start_position,
      size_t                       lane_index)>
      midi_clip;

    /** Creates a MidiNote inside a clip. */
    std::function<void (
      structure::arrangement::MidiClip &clip,
      units::sample_t                   start_position,
      units::sample_t                   end_position,
      int                               pitch,
      int                               velocity,
      int                               channel)>
      midi_note;

    /** Creates a MidiControlEvent inside a clip. */
    std::function<void (
      structure::arrangement::MidiClip                   &clip,
      units::sample_t                                     position,
      structure::arrangement::MidiControlEvent::EventType type,
      int                                                 channel,
      int                                                 controller,
      int                                                 value)>
      midi_control_event;
  };

  explicit RecordingMaterializer (
    RecordingCoordinator  &recording_coordinator,
    undo::UndoStack       &undo_stack,
    ArrangerObjectCreators creators,
    RecordingModeProvider  recording_mode_provider,
    QObject *              parent = nullptr);

  ~RecordingMaterializer () override;

  Q_DISABLE_COPY_MOVE (RecordingMaterializer)

private:
  /**
   * @brief Per-track state tracking the current recording context.
   *
   * Shared by both audio and MIDI paths. A track can only be in one session
   * (audio or MIDI) at a time, so the state is keyed by TrackUuid.
   */
  struct TrackRecordingState
  {
    /** A note-on waiting for its matching note-off. */
    struct PendingNote
    {
      units::sample_t start_position;
      uint8_t         velocity{};
      uint8_t         channel{};
    };

    /** Currently active clip (AudioClip or MidiClip). */
    std::optional<structure::arrangement::ArrangerObjectUuidReference>
      current_clip;
    /** End sample of the last contiguous packet (used for gap detection). */
    std::optional<units::sample_t> last_end_position;
    /** Lane index for the next clip creation (increments on discontinuity). */
    size_t current_lane_index = 0;
    /**
     * Notes with note-on received but no note-off yet.
     * Key: (channel << 8) | pitch. Value: FIFO queue of pending notes
     * (supports repeated note-ons on the same pitch before note-off).
     */
    std::unordered_map<uint16_t, std::deque<PendingNote>> unended_notes;
  };

  /** Handles RecordingCoordinator::audioDataReady signal. */
  void on_audio_data_ready (
    structure::tracks::TrackUuid             track_id,
    const std::vector<RecordingAudioPacket> &packets);

  /** Creates a new AudioClip or returns the existing one for this track. */
  std::optional<structure::arrangement::ArrangerObjectUuidReference>
  get_or_create_clip (
    TrackRecordingState             &state,
    structure::tracks::TrackUuid     track_id,
    units::sample_t                  start_position,
    const utils::audio::AudioBuffer &initial_frames);

  static dsp::FileAudioSource *
  get_audio_source_for_clip (structure::arrangement::AudioClip &clip);

  /** Handles RecordingCoordinator::midiDataReady signal. */
  void on_midi_data_ready (
    structure::tracks::TrackUuid            track_id,
    const std::vector<RecordingMidiPacket> &packets);

  /** Creates a MidiClip if none exists for this track. Returns false on
   * failure. */
  bool ensure_midi_clip (
    TrackRecordingState         &state,
    structure::tracks::TrackUuid track_id,
    units::sample_t              start_position);

  /**
   * @brief Called when a gap is detected between packets.
   *
   * Force-completes any pending MIDI notes, handles lane increment/muting
   * based on recording mode, and resets the current clip.
   */
  void handle_discontinuity (TrackRecordingState &state);

  /**
   * @brief Creates notes for any pending note-ons that never received a
   * note-off.
   *
   * Notes are ended at last_end_position. Called during discontinuity and
   * session finalization.
   */
  void force_complete_pending_notes (TrackRecordingState &state);

  /** Begins the undo macro if not already active. */
  void ensure_recording_macro ();

  /** Ends the undo macro and clears all per-track state. */
  void finalize_recording_macro ();

  RecordingCoordinator     &recording_coordinator_;
  QPointer<undo::UndoStack> undo_stack_;
  ArrangerObjectCreators    creators_;
  RecordingModeProvider     recording_mode_provider_;

  /** Per-track recording state, populated on first packet for each track. */
  std::unordered_map<structure::tracks::TrackUuid, TrackRecordingState>
    track_states_;

  /**
   * Pre-allocated scratch buffer reused across on_audio_data_ready()
   * calls to avoid per-packet heap allocations. Resized as needed when
   * the incoming frame count exceeds the current capacity.
   */
  utils::audio::AudioBuffer scratch_buf_{ 2, 0 };

  bool recording_macro_active_ = false;
};

}
