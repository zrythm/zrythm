// SPDX-FileCopyrightText: © 2019-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "engine/session/clip.h"
#include "structure/arrangement/fadeable_object.h"
#include "structure/arrangement/lane_owned_object.h"
#include "structure/arrangement/region.h"
#include "utils/audio.h"

namespace zrythm::structure::arrangement
{

/**
 * Number of frames for built-in fade (additional to object fades).
 */
constexpr int AUDIO_REGION_BUILTIN_FADE_FRAMES = 10;

/**
 * An AudioRegion represents a region of audio within a Track. It is responsible
 * for managing the audio data, handling playback, and providing various
 * operations on the audio region.
 *
 * The AudioRegion class inherits from Region, LaneOwnedObject, and
 * FadeableObject, allowing it to be positioned within a Track, owned by a
 * specific Lane, and have fades applied to it.
 *
 * The class provides methods for getting the associated AudioClip, setting the
 * clip ID, replacing the audio frames, and filling the audio data into
 * StereoPorts for playback. It also includes various properties related to
 * audio stretching, gain, and musical mode.
 */
class AudioRegion final
    : public QObject,
      public RegionImpl<AudioRegion>,
      public LaneOwnedObject,
      public FadeableObject,
      public ICloneable<AudioRegion>
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_ARRANGER_OBJECT_QML_PROPERTIES (AudioRegion)
  DEFINE_REGION_QML_PROPERTIES (AudioRegion)

public:
  using BitDepth = AudioClip::BitDepth;

public:
  AudioRegion (
    ArrangerObjectRegistry &obj_registry,
    TrackResolver           track_resolver,
    AudioClipResolverFunc   clip_resolver,
    QObject *               parent = nullptr) noexcept;

  /**
   * Returns the audio clip associated with the Region.
   */
  AudioClip * get_clip () const;

  auto get_clip_id () const { return clip_id_; }

  /**
   * Sets the clip ID on the region and updates any
   * references.
   */
  void set_clip_id (const AudioClip::Uuid &clip_id) { clip_id_ = clip_id; }

  bool get_muted (bool check_parent) const override;

  /**
   * Returns whether the region is effectively in musical mode.
   */
  bool get_musical_mode () const;

  /**
   * Replaces the region's frames starting from @p start_frame with @p frames.
   *
   * @warning Not realtime safe.
   *
   * @param frames Source frames.
   * @param start_frame Frame to start copying to (@p src_frames are always
   * copied from the start).
   *
   * @throw ZrythmException if the frames couldn't be replaced.
   */
  void replace_frames (
    const utils::audio::AudioBuffer &src_frames,
    unsigned_frame_t                 start_frame);

  /**
   * Fills audio data from the region.
   *
   * @note The caller already splits calls to this function at each sub-loop
   * inside the region, so region loop related logic is not needed.
   *
   * @param stereo_ports StereoPorts to fill.
   */
  [[gnu::hot]] void fill_stereo_ports (
    const EngineProcessTimeInfo        &time_nfo,
    std::pair<AudioPort &, AudioPort &> stereo_ports) const;

  float detect_bpm (std::vector<float> &candidates);

  /**
   * Fixes off-by-one rounding errors when changing BPM or sample rate which
   * result in the looped part being longer than there are actual frames in
   * the clip.
   *
   * @param frames_per_tick Frames per tick used when validating audio
   * regions. Passing 0 will use the value from the current engine.
   *
   * @return Whether positions were adjusted.
   */
  bool fix_positions (dsp::FramesPerTick frames_per_tick);

  bool
  validate (bool is_project, dsp::FramesPerTick frames_per_tick) const override;

  void init_after_cloning (const AudioRegion &other, ObjectCloneType clone_type)
    override;

  // ==========================================================================
  // Playback caches
  // ==========================================================================
  void prepare_to_play (size_t max_expected_samples);
  void release_resources ();
  // ==========================================================================

private:
  static constexpr std::string_view kClipIdKey = "clipId";
  static constexpr std::string_view kGainKey = "gain";
  friend void to_json (nlohmann::json &j, const AudioRegion &region)
  {
    to_json (j, static_cast<const ArrangerObject &> (region));
    to_json (j, static_cast<const BoundedObject &> (region));
    to_json (j, static_cast<const LoopableObject &> (region));
    to_json (j, static_cast<const FadeableObject &> (region));
    to_json (j, static_cast<const MuteableObject &> (region));
    to_json (j, static_cast<const NamedObject &> (region));
    to_json (j, static_cast<const ColoredObject &> (region));
    to_json (j, static_cast<const Region &> (region));
    j[kClipIdKey] = region.clip_id_;
    j[kGainKey] = region.gain_;
  }
  friend void from_json (const nlohmann::json &j, AudioRegion &region)
  {
    from_json (j, static_cast<ArrangerObject &> (region));
    from_json (j, static_cast<BoundedObject &> (region));
    from_json (j, static_cast<LoopableObject &> (region));
    from_json (j, static_cast<FadeableObject &> (region));
    from_json (j, static_cast<MuteableObject &> (region));
    from_json (j, static_cast<NamedObject &> (region));
    from_json (j, static_cast<ColoredObject &> (region));
    from_json (j, static_cast<Region &> (region));
    j.at (kClipIdKey).get_to (region.clip_id_);
    j.at (kGainKey).get_to (region.gain_);
  }

public:
  AudioClipResolverFunc clip_resolver_;

  /** ID of the associated AudioClip to be resolved with @ref clip_resolver_. */
  AudioClip::Uuid clip_id_;

  /**
   * Whether to read the clip from the pool (used in most cases).
   */
  // bool read_from_pool_ = false;

  /** Gain to apply to the audio (amplitude 0.0-2.0). */
  float gain_ = 1.0f;

  /**
   * Clip to read frames from, if not from the pool.
   */
  // std::unique_ptr<AudioClip> clip_;

  /** Musical mode setting. */
  MusicalMode musical_mode_{};

  /**
   * @brief Temporary buffer used during audio processing.
   */
  std::unique_ptr<juce::AudioSampleBuffer> tmp_buf_;

  BOOST_DESCRIBE_CLASS (
    AudioRegion,
    (Region, LaneOwnedObject, FadeableObject),
    (),
    (),
    ())
};

}
