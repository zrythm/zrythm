// SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_AUDIO_REGION_H__
#define __AUDIO_AUDIO_REGION_H__

#include "gui/dsp/clip.h"
#include "gui/dsp/fadeable_object.h"
#include "gui/dsp/lane_owned_object.h"
#include "gui/dsp/region.h"

#include "dsp/position.h"
#include "utils/audio.h"
#include "utils/types.h"

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
    : public QObject,
      public RegionImpl<AudioRegion>,
      public LaneOwnedObjectImpl<AudioRegion>,
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
  AudioRegion (QObject * parent = nullptr);

  /**
   * @brief Creates an audio region from a pool ID.
   *
   * @see init_default_constructed().
   *
   * @throw ZrythmException if the region couldn't be created.
   */
  AudioRegion (
    AudioClip::PoolId              pool_id,
    Position                       start_pos,
    dsp::PortIdentifier::TrackUuid track_uuid,
    int                            lane_pos,
    int                            idx_inside_lane,
    QObject *                      parent = nullptr)
      : AudioRegion (parent)
  {
    init_default_constructed (
      pool_id, std::nullopt, true, nullptr, std::nullopt, std::nullopt,
      std::nullopt, std::nullopt, std::move (start_pos), track_uuid, lane_pos,
      idx_inside_lane);
  }

  /**
   * @brief Creates an audio region from an audio file.
   *
   * @see init_default_constructed().
   *
   * @throw ZrythmException if the region couldn't be created.
   */
  AudioRegion (
    fs::path                       filepath,
    Position                       start_pos,
    dsp::PortIdentifier::TrackUuid track_uuid,
    int                            lane_pos,
    int                            idx_inside_lane,
    QObject *                      parent = nullptr)
      : AudioRegion (parent)
  {
    init_default_constructed (
      std::nullopt, std::move (filepath), false, nullptr, std::nullopt,
      std::nullopt, std::nullopt, std::nullopt, std::move (start_pos),
      track_uuid, lane_pos, idx_inside_lane);
  }

  /**
   * @brief Creates a region from an audio buffer.
   *
   * @see init_default_constructed().
   *
   * @throw ZrythmException if the region couldwn't be created.
   */
  AudioRegion (
    const utils::audio::AudioBuffer &audio_buffer,
    bool                             read_from_pool,
    std::string                      clip_name,
    AudioClip::BitDepth              bit_depth,
    Position                         start_pos,
    dsp::PortIdentifier::TrackUuid   track_uuid,
    int                              lane_pos,
    int                              idx_inside_lane,
    QObject *                        parent = nullptr)
      : AudioRegion (parent)
  {
    init_default_constructed (
      std::nullopt, std::nullopt, read_from_pool, &audio_buffer, std::nullopt,
      std::nullopt, std::move (clip_name), bit_depth, std::move (start_pos),
      track_uuid, lane_pos, idx_inside_lane);
  }

  /**
   * @brief Creates a region for recording.
   *
   * @see init_default_constructed().
   *
   * @throw ZrythmException if the region couldwn't be created.
   */
  AudioRegion (
    bool                           read_from_pool,
    std::string                    clip_name,
    unsigned_frame_t               num_frames_for_recording,
    channels_t                     num_channels_for_recording,
    Position                       start_pos,
    dsp::PortIdentifier::TrackUuid track_uuid,
    int                            lane_pos,
    int                            idx_inside_lane,
    QObject *                      parent = nullptr)
      : AudioRegion (parent)
  {
    init_default_constructed (
      std::nullopt, std::nullopt, read_from_pool, nullptr,
      num_frames_for_recording, num_channels_for_recording,
      std::move (clip_name), BitDepth::BIT_DEPTH_32, std::move (start_pos),
      track_uuid, lane_pos, idx_inside_lane);
  }

  using LaneOwnedObjectT = LaneOwnedObjectImpl<AudioRegion>;

  /**
   * @brief Initializes a default-constructed audio region.
   *
   * This is called by the explicit constructor.
   *
   * @param pool_id The pool ID. This is used when creating clone regions
   * (non-main) and must be -1 when creating a new clip.
   * @param filepath File path, if loading from file.
   * @param read_from_pool
   * Whether to save the given @p filename or @p frames to pool and read the
   * data from the pool. Only used if @p filename or @p frames is given.
   * @param num_frames_for_recording Number of frames for recording.
   * @param num_channels_for_recording Number of channels for recording.
   * @param audio_buffer Samples (optional).
   * @param clip_name Name of audio clip, if not loading from file.
   * @param bit_depth Bit depth, if using @ref frames.
   *
   * @throw ZrythmException if the region couldwn't be created.
   */
  void init_default_constructed (
    std::optional<AudioClip::PoolId>      pool_id,
    std::optional<fs::path>               filepath,
    bool                                  read_from_pool,
    const utils::audio::AudioBuffer *     audio_buffer,
    std::optional<unsigned_frame_t>       num_frames_for_recording,
    std::optional<channels_t>             num_channels_for_recording,
    std::optional<std::string>            clip_name,
    std::optional<utils::audio::BitDepth> bit_depth,
    Position                              start_pos,
    dsp::PortIdentifier::TrackUuid        track_uuid,
    int                                   lane_pos,
    int                                   idx_inside_lane);

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
   * @warning Not realtime safe.
   *
   * @param duplicate_clip Whether to duplicate the clip (eg, when other
   * regions refer to it).
   * @param frames Source frames.
   * @param start_frame Frame to start copying to (@p src_frames are always
   * copied from the start).
   *
   * @throw ZrythmException if the frames couldn't be replaced.
   */
  void replace_frames (
    const utils::audio::AudioBuffer &src_frames,
    unsigned_frame_t                 start_frame,
    bool                             duplicate_clip);

  /**
   * Replaces the region's frames starting from @p start_frame with @p frames.
   *
   * @warning Not realtime safe.
   *
   * @param duplicate_clip Whether to duplicate the clip (eg, when other
   * regions refer to it).
   * @param frames Frames, interleaved.
   * @param start_frame Frame to start copying to (@p src_frames are always
   * copied from the start).
   *
   * @throw ZrythmException if the frames couldn't be replaced.
   */
  void replace_frames_from_interleaved (
    const float *    interleaved_frames,
    unsigned_frame_t start_frame,
    unsigned_frame_t num_frames_per_channel,
    channels_t       channels,
    bool             duplicate_clip);

  /**
   * Fills audio data from the region.
   *
   * @note The caller already splits calls to this function at each sub-loop
   * inside the region, so region loop related logic is not needed.
   *
   * @param stereo_ports StereoPorts to fill.
   */
  ATTR_NONBLOCKING
  ATTR_HOT void fill_stereo_ports (
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
  bool fix_positions (double frames_per_tick);

  bool validate (bool is_project, double frames_per_tick) const override;

  void init_after_cloning (const AudioRegion &other, ObjectCloneType clone_type)
    override;

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
  mutable std::array<std::array<float, 0x4000>, 2> tmp_bufs_{};
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
  return fmt::format ("AudioRegion[id: {}]", ar.id_);
})

/**
 * @}
 */

#endif // __AUDIO_AUDIO_REGION_H__
