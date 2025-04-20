// SPDX-FileCopyrightText: © 2019-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef DSP_AUDIO_REGION_H
#define DSP_AUDIO_REGION_H

#include "gui/dsp/clip.h"
#include "gui/dsp/fadeable_object.h"
#include "gui/dsp/lane_owned_object.h"
#include "gui/dsp/region.h"
#include "utils/audio.h"

/**
 * @addtogroup dsp
 *
 * @{
 */

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
      public ICloneable<AudioRegion>,
      public zrythm::utils::serialization::ISerializable<AudioRegion>
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_ARRANGER_OBJECT_QML_PROPERTIES (AudioRegion)
  DEFINE_REGION_QML_PROPERTIES (AudioRegion)

public:
  using BitDepth = AudioClip::BitDepth;

public:
  AudioRegion (const DeserializationDependencyHolder &dh);
  AudioRegion (
    ArrangerObjectRegistry &obj_registry,
    TrackResolver           track_resolver,
    AudioClipResolverFunc   clip_resolver,
    QObject *               parent = nullptr);

  void init_loaded () override;

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

  DECLARE_DEFINE_FIELDS_METHOD ();

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
};

inline bool
operator== (const AudioRegion &lhs, const AudioRegion &rhs)
{
  return static_cast<const Region &> (lhs) == static_cast<const Region &> (rhs)
         && static_cast<const TimelineObject &> (lhs)
              == static_cast<const TimelineObject &> (rhs)
         && static_cast<const NamedObject &> (lhs)
              == static_cast<const NamedObject &> (rhs)
         && static_cast<const LoopableObject &> (lhs)
              == static_cast<const LoopableObject &> (rhs)
         && static_cast<const ColoredObject &> (lhs)
              == static_cast<const ColoredObject &> (rhs)
         && static_cast<const MuteableObject &> (lhs)
              == static_cast<const MuteableObject &> (rhs)
         && static_cast<const BoundedObject &> (lhs)
              == static_cast<const BoundedObject &> (rhs)
         && static_cast<const ArrangerObject &> (lhs)
              == static_cast<const ArrangerObject &> (rhs);
}

DEFINE_OBJECT_FORMATTER (AudioRegion, AudioRegion, [] (const AudioRegion &ar) {
  return fmt::format (
    "AudioRegion[id: {}, {}]", ar.get_uuid (), static_cast<const Region &> (ar));
})

/**
 * @}
 */

#endif // DSP_AUDIO_REGION_H
