// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "controllers/recording_materializer.h"
#include "dsp/file_audio_source.h"
#include "structure/arrangement/audio_source_object.h"
#include "utils/audio.h"
#include "utils/logger.h"

namespace zrythm::controllers
{

RecordingMaterializer::RecordingMaterializer (
  RecordingCoordinator &recording_coordinator,
  undo::UndoStack      &undo_stack,
  RegionCreator         region_creator,
  RecordingModeProvider recording_mode_provider,
  QObject *             parent)
    : QObject (parent), recording_coordinator_ (recording_coordinator),
      undo_stack_ (&undo_stack), region_creator_ (std::move (region_creator)),
      recording_mode_provider_ (std::move (recording_mode_provider))
{
  QObject::connect (
    &recording_coordinator_, &RecordingCoordinator::audioDataReady, this,
    &RecordingMaterializer::on_audio_data_ready);
  QObject::connect (
    &recording_coordinator_, &RecordingCoordinator::recordingSessionEnded, this,
    &RecordingMaterializer::finalize_recording_macro);
}

RecordingMaterializer::~RecordingMaterializer ()
{
  disconnect (&recording_coordinator_, nullptr, this, nullptr);
  if (recording_macro_active_)
    {
      z_warning (
        "RecordingMaterializer destroyed with active recording macro — "
        "finalizing");
      finalize_recording_macro ();
    }
}

void
RecordingMaterializer::on_audio_data_ready (
  structure::tracks::TrackUuid             track_id,
  const std::vector<RecordingAudioPacket> &packets)
{
  for (const auto &packet : packets)
    {
      if (!packet.transport_recording)
        continue;

      auto      &state = track_states_[track_id];
      const auto num_frames =
        static_cast<int> (packet.nframes.in (units::samples));

      const bool discontinuous =
        !state.last_end_position.has_value ()
        || packet.timeline_position != *state.last_end_position;

      if (discontinuous && state.current_region.has_value ())
        {
          const auto mode = recording_mode_provider_ ();

          if (mode == RecordingMode::Takes || mode == RecordingMode::TakesMuted)
            {
              if (mode == RecordingMode::TakesMuted)
                {
                  auto * prev_region = state.current_region->get_object_as<
                    structure::arrangement::AudioRegion> ();
                  if (prev_region != nullptr)
                    {
                      prev_region->mute ()->setMuted (true);
                    }
                }

              state.current_lane_index++;
            }

          state.current_region.reset ();
        }

      scratch_buf_.setSize (2, num_frames, false, false, true);
      scratch_buf_.copyFrom (0, 0, packet.l_frames.data (), num_frames);
      scratch_buf_.copyFrom (1, 0, packet.r_frames.data (), num_frames);

      bool newly_created = false;
      if (!state.current_region.has_value ())
        {
          state.current_region = get_or_create_region (
            state, track_id, packet.timeline_position, scratch_buf_);

          if (!state.current_region.has_value ())
            {
              z_warning (
                "Failed to create region for track {} at position {} — "
                "dropping {} frames",
                track_id, packet.timeline_position,
                packet.nframes.in (units::samples));
              continue;
            }

          newly_created = true;
        }

      auto * region = state.current_region->get_object_as<
        structure::arrangement::AudioRegion> ();
      auto * clip = get_clip_for_region (*region);

      if (!newly_created)
        {
          clip->expand_with_frames (scratch_buf_);
        }

      region->bounds ()->length ()->setSamples (
        static_cast<double> (clip->get_num_frames ()));

      state.last_end_position =
        packet.timeline_position
        + units::samples (static_cast<int64_t> (num_frames));
    }
}

std::optional<structure::arrangement::ArrangerObjectUuidReference>
RecordingMaterializer::get_or_create_region (
  TrackRecordingState             &state,
  structure::tracks::TrackUuid     track_id,
  units::sample_t                  start_position,
  const utils::audio::AudioBuffer &initial_frames)
{
  if (!recording_macro_active_)
    {
      assert (!undo_stack_.isNull ());
      undo_stack_->beginMacro (QObject::tr ("Record"));
      recording_macro_active_ = true;
    }

  auto result = region_creator_ (
    track_id, start_position, initial_frames, state.current_lane_index);
  if (!result.has_value ())
    return std::nullopt;
  state.current_lane_index = result->actual_lane_index;
  return result->region;
}

dsp::FileAudioSource *
RecordingMaterializer::get_clip_for_region (
  structure::arrangement::AudioRegion &region)
{
  auto children = region.get_children_view ();
  assert (!children.empty ());
  auto * source_obj = children.front ();
  auto   clip_ref = source_obj->audio_source_ref ();
  return clip_ref.get_object_as<dsp::FileAudioSource> ();
}

void
RecordingMaterializer::finalize_recording_macro ()
{
  if (!recording_macro_active_)
    return;

  if (!undo_stack_.isNull ())
    {
      undo_stack_->endMacro ();
    }
  recording_macro_active_ = false;
  track_states_.clear ();
}

}
