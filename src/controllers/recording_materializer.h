// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <functional>
#include <unordered_map>

#include "controllers/recording_coordinator.h"
#include "structure/arrangement/arranger_object_all.h"
#include "undo/undo_stack.h"
#include "utils/audio.h"
#include "utils/qt.h"

#include <QObject>

namespace zrythm::controllers
{

/**
 * @brief Converts raw recording audio packets into arrangement regions.
 *
 * Subscribes to RecordingCoordinator::audioDataReady and creates or expands
 * AudioRegion objects for each track. The first packet for a given continuous
 * range creates a new region via the RegionCreator callback; subsequent
 * packets append frames to the region's clip.
 *
 * All region creations within a single recording take (transport rolling to
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
  using RegionCreator = std::function<std::optional<
    structure::arrangement::ArrangerObjectUuidReference> (
    structure::tracks::TrackUuid     track_id,
    units::sample_t                  start_position,
    const utils::audio::AudioBuffer &initial_frames)>;

  explicit RecordingMaterializer (
    RecordingCoordinator &recording_coordinator,
    undo::UndoStack      &undo_stack,
    RegionCreator         region_creator,
    QObject *             parent = nullptr);

  ~RecordingMaterializer () override;

  Q_DISABLE_COPY_MOVE (RecordingMaterializer)

private:
  struct TrackRecordingState
  {
    std::optional<structure::arrangement::ArrangerObjectUuidReference>
                                   current_region;
    std::optional<units::sample_t> last_end_position;
  };

  void on_audio_data_ready (
    structure::tracks::TrackUuid             track_id,
    const std::vector<RecordingAudioPacket> &packets);

  std::optional<structure::arrangement::ArrangerObjectUuidReference>
  get_or_create_region (
    structure::tracks::TrackUuid     track_id,
    units::sample_t                  start_position,
    const utils::audio::AudioBuffer &initial_frames);

  static dsp::FileAudioSource *
  get_clip_for_region (structure::arrangement::AudioRegion &region);

  void finalize_recording_macro ();

  RecordingCoordinator     &recording_coordinator_;
  QPointer<undo::UndoStack> undo_stack_;
  RegionCreator             region_creator_;

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
