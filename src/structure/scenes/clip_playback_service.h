// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/arranger_object_all.h"
#include "structure/scenes/clip_slot.h"
#include "structure/tracks/track_collection.h"

#include <QtQmlIntegration>

namespace zrythm::structure::scenes
{

/**
 * @brief Service for managing clip playback operations.
 *
 * This service handles launching and stopping clips with proper quantization,
 * managing track processor states, and coordinating between the UI and audio
 * engine.
 */
class ClipPlaybackService : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  explicit ClipPlaybackService (
    arrangement::ArrangerObjectRegistry &object_registry,
    const tracks::TrackCollection       &track_collection,
    QObject *                            parent = nullptr);

  // ========================================================================
  // Clip Operations
  // ========================================================================

  /**
   * @brief Launch a clip with quantization.
   *
   * @param track The track to play the clip on
   * @param clipSlot The clip slot containing the clip to launch
   * @param quantize Quantization option for timing
   */
  Q_INVOKABLE void launchClip (
    tracks::Track *                       track,
    ClipSlot *                            clipSlot,
    structure::tracks::ClipQuantizeOption quantize =
      structure::tracks::ClipQuantizeOption::NextBar);

  /**
   * @brief Stop a clip with quantization.
   *
   * @param track The track containing the playing clip
   * @param clipSlot The clip slot to stop
   * @param quantize Quantization option for timing
   */
  Q_INVOKABLE void stopClip (
    tracks::Track *                       track,
    ClipSlot *                            clipSlot,
    structure::tracks::ClipQuantizeOption quantize =
      structure::tracks::ClipQuantizeOption::NextBar);

  /**
   * @brief Launch all clips in a scene.
   *
   * @param scene The scene to launch
   * @param quantize Quantization option for timing
   */
  Q_INVOKABLE void launchScene (
    Scene *                               scene,
    structure::tracks::ClipQuantizeOption quantize =
      structure::tracks::ClipQuantizeOption::NextBar);

  /**
   * @brief Stop all clips in a scene.
   *
   * @param scene The scene to stop
   * @param quantize Quantization option for timing
   */
  Q_INVOKABLE void stopScene (
    Scene *                               scene,
    structure::tracks::ClipQuantizeOption quantize =
      structure::tracks::ClipQuantizeOption::NextBar);

  /**
   * @brief Stop all playing clips across all tracks.
   *
   * @param quantize Quantization option for timing
   */
  Q_INVOKABLE void stopAllClips (
    structure::tracks::ClipQuantizeOption quantize =
      structure::tracks::ClipQuantizeOption::NextBar);

  // ========================================================================
  // State Queries
  // ========================================================================

  /**
   * @brief Check if a clip is currently playing.
   *
   * @param clipSlot The clip slot to check
   * @return true if the clip is playing
   */
  Q_INVOKABLE bool isClipPlaying (ClipSlot * clipSlot) const;

  /**
   * @brief Get the current playback position of a clip, in.
   *
   * @param clipSlot The clip slot to get the position for
   * @return Position in the clip as a percentage (0.0 to 1.0), or -1 if not
   * playing
   */
  Q_INVOKABLE double getClipPlaybackPosition (ClipSlot * clipSlot) const;

  /**
   * @brief Get all currently playing clips.
   *
   * @return List of playing clip slots
   */
  Q_INVOKABLE QList<ClipSlot *> getPlayingClips () const;

  // ========================================================================
  // Track Playback Mode Management
  // ========================================================================

  /**
   * @brief Set a track to use clip launcher mode.
   *
   * @param track The track to configure
   */
  void setTrackToClipLauncherMode (tracks::Track * track);

  /**
   * @brief Set a track to use timeline mode.
   *
   * @param track The track to configure
   */
  Q_INVOKABLE void setTrackToTimelineMode (tracks::Track * track);

  /**
   * @brief Cancel any pending clip operations for a track.
   *
   * This is called when switching modes to ensure clips don't get stuck in
   * queued state.
   *
   * @param track The track to cancel operations for
   */
  void cancelPendingClipOperations (tracks::Track * track);

Q_SIGNALS:
  /**
   * @brief Emitted when a clip is launched.
   */
  void clipLaunched (tracks::Track * track, ClipSlot * clipSlot);

  /**
   * @brief Emitted when a clip is stopped.
   */
  void clipStopped (tracks::Track * track, ClipSlot * clipSlot);

  /**
   * @brief Emitted when a scene is launched.
   */
  void sceneLaunched (Scene * scene);

  /**
   * @brief Emitted when a scene is stopped.
   */
  void sceneStopped (Scene * scene);

private:
  // ========================================================================
  // Internal Implementation
  // ========================================================================

  /**
   * @brief Schedule a clip launch at the appropriate quantized position.
   */
  void scheduleClipLaunch (
    tracks::Track *                       track,
    ClipSlot *                            clipSlot,
    structure::tracks::ClipQuantizeOption quantize);

  /**
   * @brief Schedule a clip stop at the appropriate quantized position.
   */
  void scheduleClipStop (
    tracks::Track *                       track,
    ClipSlot *                            clipSlot,
    structure::tracks::ClipQuantizeOption quantize);

  /**
   * @brief Generate events for a clip (region).
   */
  template <arrangement::RegionObject RegionT>
  void generateClipEvents (
    tracks::Track *                       track,
    const RegionT                        &region,
    structure::tracks::ClipQuantizeOption quantize)
  {
    if (track == nullptr)
      return;

    auto * processor = track->get_track_processor ();
    if (processor == nullptr)
      return;

    // Generate events using the clip launcher event provider
    if constexpr (std::is_same_v<RegionT, arrangement::MidiRegion>)
      {
        processor->clip_playback_data_provider ().generate_midi_events (
          region, quantize);
      }
    else if constexpr (std::is_same_v<RegionT, arrangement::AudioRegion>)
      {
        processor->clip_playback_data_provider ().generate_audio_events (
          region, quantize);
      }
  }

  /**
   * @brief Update clip slot state and emit appropriate signals.
   */
  void updateClipSlotState (ClipSlot * clipSlot, ClipSlot::ClipState newState);

  /**
   * @brief Start a timer to monitor playback state changes.
   */
  void startPlaybackStateMonitor (
    tracks::Track *     track,
    ClipSlot *          clipSlot,
    ClipSlot::ClipState targetState);

  /**
   * @brief Check if the track processor has reached the target state.
   */
  void checkPlaybackState ();

private:
  arrangement::ArrangerObjectRegistry &object_registry_;
  const tracks::TrackCollection       &track_collection_;

  /**
   * @brief Track of currently playing clips by track UUID.
   */
  std::unordered_map<tracks::Track::Uuid, ClipSlot *> playing_clips_;

  /**
   * @brief Timers for monitoring playback state changes.
   */
  std::unordered_map<
    tracks::Track::Uuid,
    std::tuple<utils::QObjectUniquePtr<QTimer>, ClipSlot *, ClipSlot::ClipState>>
    playback_state_monitors_;
};

}
