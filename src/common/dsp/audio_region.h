// SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_AUDIO_REGION_H__
#define __AUDIO_AUDIO_REGION_H__

#include "common/dsp/clip.h"
#include "common/dsp/fadeable_object.h"
#include "common/dsp/lane_owned_object.h"
#include "common/dsp/position.h"
#include "common/dsp/region.h"
#include "common/utils/audio.h"
#include "common/utils/types.h"

class StereoPorts;

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
    : public RegionImpl<AudioRegion>,
      public LaneOwnedObjectImpl<AudioRegion>,
      public FadeableObject,
      public ICloneable<AudioRegion>,
      public ISerializable<AudioRegion>
{
public:
  // Rule of 0
  AudioRegion () = default;

  /**
   * @brief Creates a region for audio data.
   *
   * @see init_default_constructed().
   *
   * @throw ZrythmException if the region couldwn't be created.
   */
  AudioRegion (
    const int                        pool_id,
    const std::optional<std::string> filename,
    bool                             read_from_pool,
    const float *                    frames,
    const unsigned_frame_t           nframes,
    const std::optional<std::string> clip_name,
    const channels_t                 channels,
    BitDepth                         bit_depth,
    Position                         start_pos,
    unsigned int                     track_name_hash,
    int                              lane_pos,
    int                              idx_inside_lane)
  {
    init_default_constructed (
      pool_id, filename, read_from_pool, frames, nframes, clip_name, channels,
      bit_depth, start_pos, track_name_hash, lane_pos, idx_inside_lane);
  }

  using LaneOwnedObjectT = LaneOwnedObjectImpl<AudioRegion>;

  /**
   * @brief Initializes a default-constructed audio region.
   *
   * This is called by the explicit constructor.
   *
   * @param pool_id The pool ID. This is used when creating clone regions
   * (non-main) and must be -1 when creating a new clip.
   * @param filename Filename, if loading from file, otherwise NULL.
   * @param read_from_pool
   * Whether to save the given @p filename or @p frames to pool and read the
   * data from the pool. Only used if @p filename or @p frames is given.
   * @param frames Float array, if loading from float array, otherwise NULL.
   * @param nframes Number of frames per channel. Only used if @ref frames is
   * non-NULL.
   * @param clip_name Name of audio clip, if not loading from file.
   * @param bit_depth Bit depth, if using @ref frames.
   *
   * @throw ZrythmException if the region couldwn't be created.
   */
  void init_default_constructed (
    const int                        pool_id,
    const std::optional<std::string> filename,
    bool                             read_from_pool,
    const float *                    frames,
    const unsigned_frame_t           nframes,
    const std::optional<std::string> clip_name,
    const channels_t                 channels,
    BitDepth                         bit_depth,
    Position                         start_pos,
    unsigned int                     track_name_hash,
    int                              lane_pos,
    int                              idx_inside_lane);

  void init_loaded () override;

  /**
   * Returns the audio clip associated with the Region.
   */
  AudioClip * get_clip () const;

  /**
   * Sets the clip ID on the region and updates any
   * references.
   */
  void set_clip_id (int clip_id);

  bool get_muted (bool check_parent) const override;

  void append_children (
    std::vector<RegionOwnedObjectImpl<AudioRegion> *> &children) const override
  {
  }

  void add_ticks_to_children (const double ticks) override { }

  /**
   * Returns whether the region is effectively in musical mode.
   */
  bool get_musical_mode () const;

  /**
   * Replaces the region's frames starting from @p start_frame with @p frames.
   *
   * @param duplicate_clip Whether to duplicate the clip (eg, when other
   * regions refer to it).
   * @param frames Frames, interleaved.
   *
   * @throw ZrythmException if the frames couldn't be replaced.
   */
  void replace_frames (
    const float *    frames,
    unsigned_frame_t start_frame,
    unsigned_frame_t num_frames,
    bool             duplicate_clip);

  /**
   * Fills audio data from the region.
   *
   * @note The caller already splits calls to this function at each sub-loop
   * inside the region, so region loop related logic is not needed.
   *
   * @param stereo_ports StereoPorts to fill.
   */
  ATTR_REALTIME
  ATTR_HOT void fill_stereo_ports (
    const EngineProcessTimeInfo &time_nfo,
    StereoPorts                 &stereo_ports) const;

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
  bool fix_positions (double frames_per_tick);

  bool validate (bool is_project, double frames_per_tick) const override;

  ArrangerSelections * get_arranger_selections () const override;

  void init_after_cloning (const AudioRegion &other) override;

  DECLARE_DEFINE_FIELDS_METHOD ();

public:
  /** Audio pool ID of the associated audio file, mostly used during
   * serialization. */
  int pool_id_ = -1;

  /**
   * Whether to read the clip from the pool (used in most cases).
   */
  bool read_from_pool_ = false;

  /** Gain to apply to the audio (amplitude 0.0-2.0). */
  float gain_ = 1.0f;

  /**
   * Clip to read frames from, if not from the pool.
   */
  std::unique_ptr<AudioClip> clip_;

  /** Musical mode setting. */
  MusicalMode musical_mode_ = (MusicalMode) 0;

  /**
   * @brief Temporary buffers used during audio processing.
   */
  mutable std::array<std::array<float, 0x4000>, 2> tmp_bufs_;
};

inline bool
operator== (const AudioRegion &lhs, const AudioRegion &rhs)
{
  return static_cast<const Region &> (lhs) == static_cast<const Region &> (rhs)
         && static_cast<const TimelineObject &> (lhs)
              == static_cast<const TimelineObject &> (rhs)
         && static_cast<const NameableObject &> (lhs)
              == static_cast<const NameableObject &> (rhs)
         && static_cast<const LoopableObject &> (lhs)
              == static_cast<const LoopableObject &> (rhs)
         && static_cast<const ColoredObject &> (lhs)
              == static_cast<const ColoredObject &> (rhs)
         && static_cast<const MuteableObject &> (lhs)
              == static_cast<const MuteableObject &> (rhs)
         && static_cast<const LengthableObject &> (lhs)
              == static_cast<const LengthableObject &> (rhs)
         && static_cast<const ArrangerObject &> (lhs)
              == static_cast<const ArrangerObject &> (rhs);
}

DEFINE_OBJECT_FORMATTER (AudioRegion, [] (const AudioRegion &ar) {
  return fmt::format ("AudioRegion[id: {}]", ar.id_);
})

/**
 * @}
 */

#endif // __AUDIO_AUDIO_REGION_H__
