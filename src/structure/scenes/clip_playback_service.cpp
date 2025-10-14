// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/scenes/clip_playback_service.h"
#include "structure/tracks/track_processor.h"

namespace zrythm::structure::scenes
{

ClipPlaybackService::ClipPlaybackService (
  arrangement::ArrangerObjectRegistry &object_registry,
  const tracks::TrackCollection       &track_collection,
  QObject *                            parent)
    : QObject (parent), object_registry_ (object_registry),
      track_collection_ (track_collection)
{
}

void
ClipPlaybackService::launchClip (
  tracks::Track *                       track,
  ClipSlot *                            clipSlot,
  structure::tracks::ClipQuantizeOption quantize)
{
  if ((track == nullptr) || (clipSlot == nullptr))
    {
      z_warning ("Invalid track or clip slot provided to launchClip");
      return;
    }

  auto * region = clipSlot->region ();
  if (region == nullptr)
    {
      z_warning ("No region in clip slot");
      return;
    }

  // Schedule the launch with quantization
  scheduleClipLaunch (track, clipSlot, quantize);
}

void
ClipPlaybackService::stopClip (
  tracks::Track *                       track,
  ClipSlot *                            clipSlot,
  structure::tracks::ClipQuantizeOption quantize)
{
  if ((track == nullptr) || (clipSlot == nullptr))
    {
      z_warning ("Invalid track or clip slot provided to stopClip");
      return;
    }

  // Schedule the stop with quantization
  scheduleClipStop (track, clipSlot, quantize);
}

void
ClipPlaybackService::launchScene (
  Scene *                               scene,
  structure::tracks::ClipQuantizeOption quantize)
{
  if (scene == nullptr)
    {
      z_warning ("Invalid scene provided to launchScene");
      return;
    }

  // Get all clip slots in the scene
  auto * clipSlotList = scene->clipSlots ();
  if (clipSlotList == nullptr)
    {
      z_warning ("Scene has no clip slot list");
      return;
    }

  // Launch each clip that has a region
  for (const auto &track : track_collection_.tracks ())
    {
      auto * clipSlot =
        clipSlotList->clipSlotForTrack (track.get_object_base ());
      if ((clipSlot != nullptr) && (clipSlot->region () != nullptr))
        {
          launchClip (track.get_object_base (), clipSlot, quantize);
        }
    }

  Q_EMIT sceneLaunched (scene);
}

void
ClipPlaybackService::stopScene (
  Scene *                               scene,
  structure::tracks::ClipQuantizeOption quantize)
{
  if (scene == nullptr)
    {
      z_warning ("Invalid scene provided to stopScene");
      return;
    }

  // Get all clip slots in the scene
  auto * clipSlotList = scene->clipSlots ();
  if (clipSlotList == nullptr)
    {
      z_warning ("Scene has no clip slot list");
      return;
    }

  // Stop each playing clip in the scene
  for (const auto &track : track_collection_.tracks ())
    {
      auto * clipSlot =
        clipSlotList->clipSlotForTrack (track.get_object_base ());
      if ((clipSlot != nullptr) && isClipPlaying (clipSlot))
        {
          stopClip (track.get_object_base (), clipSlot, quantize);
        }
    }

  Q_EMIT sceneStopped (scene);
}

void
ClipPlaybackService::stopAllClips (
  structure::tracks::ClipQuantizeOption quantize)
{
  // Stop all currently playing clips
  for (const auto &[trackUuid, clipSlot] : playing_clips_)
    {
      auto * track =
        track_collection_.track_ref_at_id (trackUuid).get_object_base ();
      if (track != nullptr)
        {
          stopClip (track, clipSlot, quantize);
        }
    }
}

bool
ClipPlaybackService::isClipPlaying (ClipSlot * clipSlot) const
{
  if (clipSlot == nullptr)
    return false;

  return clipSlot->state () == ClipSlot::ClipState::Playing
         || clipSlot->state () == ClipSlot::ClipState::PlayQueued;
}

double
ClipPlaybackService::getClipPlaybackPosition (ClipSlot * clipSlot) const
{
  if (clipSlot == nullptr)
    return -1.0;

  // Find the track that has this clip slot
  for (const auto &[trackUuid, playingClipSlot] : playing_clips_)
    {
      if (playingClipSlot == clipSlot)
        {
          // Get the track
          auto * track =
            track_collection_.track_ref_at_id (trackUuid).get_object_base ();
          if (track == nullptr)
            return -1.0;

          // Get the track processor
          auto * processor = track->get_track_processor ();
          if (processor == nullptr)
            return -1.0;

          // Get the internal buffer position from the event provider
          auto internal_position =
            processor->clip_playback_data_provider ()
              .current_playback_position_in_clip ();

          // Get the region to determine its length
          auto * region = clipSlot->region ();
          if (region == nullptr)
            return -1.0;

          // Convert the region length to samples
          auto region_length_samples = region->bounds ()->length ()->samples ();

          // Calculate the position as a percentage
          if (region_length_samples > 0)
            {
              return static_cast<double> (internal_position.in (units::samples))
                     / static_cast<double> (region_length_samples);
            }

          return 0.0;
        }
    }

  // If we get here, the clip is not playing
  return -1.0;
}

QList<ClipSlot *>
ClipPlaybackService::getPlayingClips () const
{
  QList<ClipSlot *> result;

  for (const auto &[trackUuid, clipSlot] : playing_clips_)
    {
      if (isClipPlaying (clipSlot))
        {
          result.append (clipSlot);
        }
    }

  return result;
}

void
ClipPlaybackService::setTrackToClipLauncherMode (tracks::Track * track)
{
  if (track == nullptr)
    return;

  auto * processor = track->get_track_processor ();
  if (processor == nullptr)
    return;

  // Set the track to clip launcher mode
  track->setClipLauncherMode (true);
}

void
ClipPlaybackService::setTrackToTimelineMode (tracks::Track * track)
{
  if (track == nullptr)
    return;

  auto * processor = track->get_track_processor ();
  if (processor == nullptr)
    return;

  // Cancel any pending clip operations for this track
  cancelPendingClipOperations (track);

  // Set the track to timeline mode
  track->setClipLauncherMode (false);
}

void
ClipPlaybackService::scheduleClipLaunch (
  tracks::Track *                       track,
  ClipSlot *                            clipSlot,
  structure::tracks::ClipQuantizeOption quantize)
{
  if ((track == nullptr) || (clipSlot == nullptr))
    return;

  auto * region = clipSlot->region ();
  if (region == nullptr)
    return;

  // Update clip slot state to queued
  updateClipSlotState (clipSlot, ClipSlot::ClipState::PlayQueued);

  // Ensure track is in clip launcher mode
  setTrackToClipLauncherMode (track);

  // Generate events using the clip launcher event provider
  // The provider handles quantization internally
  if (auto * midi_region = dynamic_cast<arrangement::MidiRegion *> (region))
    {
      generateClipEvents (track, *midi_region, quantize);
    }
  else if (
    auto * audio_region = dynamic_cast<arrangement::AudioRegion *> (region))
    {
      generateClipEvents (track, *audio_region, quantize);
    }

  // Track the playing clip
  playing_clips_[track->get_uuid ()] = clipSlot;

  // Start monitoring for the actual playback state change
  startPlaybackStateMonitor (track, clipSlot, ClipSlot::ClipState::Playing);

  // Emit signal
  Q_EMIT clipLaunched (track, clipSlot);
}

void
ClipPlaybackService::scheduleClipStop (
  tracks::Track *                       track,
  ClipSlot *                            clipSlot,
  structure::tracks::ClipQuantizeOption quantize)
{
  if ((track == nullptr) || (clipSlot == nullptr))
    return;

  // Update clip slot state to queued
  updateClipSlotState (clipSlot, ClipSlot::ClipState::StopQueued);

  auto * processor = track->get_track_processor ();
  if (processor == nullptr)
    return;

  // Queue stop playback with quantization in the clip launcher event
  // provider
  processor->clip_playback_data_provider ().queue_stop_playback (quantize);

  // Start monitoring for the actual playback state change
  startPlaybackStateMonitor (track, clipSlot, ClipSlot::ClipState::Stopped);

  // Emit signal
  Q_EMIT clipStopped (track, clipSlot);
}

void
ClipPlaybackService::updateClipSlotState (
  ClipSlot *          clipSlot,
  ClipSlot::ClipState newState)
{
  if (clipSlot == nullptr)
    return;

  clipSlot->setState (newState);
}

void
ClipPlaybackService::startPlaybackStateMonitor (
  tracks::Track *     track,
  ClipSlot *          clipSlot,
  ClipSlot::ClipState targetState)
{
  if ((track == nullptr) || (clipSlot == nullptr))
    return;

  // Create a timer to monitor the playback state
  auto timer = utils::make_qobject_unique<QTimer> (this);
  timer->setInterval (16); // Check every frame (approximately 60fps)

  // Connect the timer's timeout signal to our check method
  QObject::connect (
    timer.get (), &QTimer::timeout, this,
    [this, track, clipSlot, targetState] () {
      auto * processor = track->get_track_processor ();
      if (!processor)
        return;

      // Check if the track processor has reached the target state
      bool isPlaying = processor->clip_playback_data_provider ().playing ();

      if (
        (targetState == ClipSlot::ClipState::Playing && isPlaying)
        || (targetState == ClipSlot::ClipState::Stopped && !isPlaying))
        {
          // Stop the timer
          auto it = playback_state_monitors_.find (track->get_uuid ());
          if (it != playback_state_monitors_.end ())
            {
              playback_state_monitors_.erase (it);
            }

          // Update the clip slot state
          updateClipSlotState (clipSlot, targetState);

          // If stopping, remove from playing clips tracking
          if (targetState == ClipSlot::ClipState::Stopped)
            {
              playing_clips_.erase (track->get_uuid ());
            }
        }
    });

  // Start the timer
  timer->start ();

  // Store the timer for later access
  playback_state_monitors_[track->get_uuid ()] =
    std::make_tuple (std::move (timer), clipSlot, targetState);
}

void
ClipPlaybackService::checkPlaybackState ()
{
  // This method can be called periodically to check all timers
  // The actual checking is done by the individual timer callbacks
}

void
ClipPlaybackService::cancelPendingClipOperations (tracks::Track * track)
{
  if (track == nullptr)
    return;

  const auto trackUuid = track->get_uuid ();

  // Check if there's a pending operation for this track
  auto it = playback_state_monitors_.find (trackUuid);
  if (it != playback_state_monitors_.end ())
    {
      // Get the clip slot and target state
      auto   timer = std::move (std::get<0> (it->second));
      auto * clipSlot = std::get<1> (it->second);

      // Stop the timer
      if (timer)
        {
          timer->stop ();
        }

      // Remove the monitor
      playback_state_monitors_.erase (it);

      // If the clip is in a queued state, set it back to stopped
      if (
        (clipSlot != nullptr)
        && (clipSlot->state () == ClipSlot::ClipState::PlayQueued || clipSlot->state () == ClipSlot::ClipState::StopQueued))
        {
          updateClipSlotState (clipSlot, ClipSlot::ClipState::Stopped);
        }

      // Remove from playing clips tracking if it was there
      playing_clips_.erase (trackUuid);
    }
}
}
