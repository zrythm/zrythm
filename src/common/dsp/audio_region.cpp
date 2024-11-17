// SPDX-FileCopyrightText: © 2018-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <memory>

#include "common/dsp/audio_region.h"
#include "common/dsp/audio_track.h"
#include "common/dsp/clip.h"
#include "common/dsp/pool.h"
#include "common/dsp/stretcher.h"
#include "common/dsp/tempo_track.h"
#include "common/dsp/tracklist.h"
#include "common/utils/audio.h"
#include "common/utils/debug.h"
#include "common/utils/dsp.h"
#include "common/utils/flags.h"
#include "common/utils/logger.h"
#include "common/utils/math.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/settings_manager.h"

#include <fmt/format.h>

using namespace zrythm;

AudioRegion::AudioRegion (QObject * parent)
    : ArrangerObject (Type::Region), LengthableObject (), QObject (parent)
{
  ArrangerObject::parent_base_qproperties (*this);
  LengthableObject::parent_base_qproperties (*this);
}

void
AudioRegion::init_default_constructed (
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
  read_from_pool_ = read_from_pool;
  id_ = RegionIdentifier (RegionType::Audio);

  z_return_if_fail (start_pos.frames_ >= 0);

  AudioClip * clip = nullptr;
  bool        recording = false;
  if (pool_id == -1)
    {
      std::unique_ptr<AudioClip> tmp_clip;
      if (filename)
        {
          tmp_clip = std::make_unique<AudioClip> (*filename);
        }
      else if (frames)
        {
          z_return_if_fail (clip_name.has_value ());
          tmp_clip = std::make_unique<AudioClip> (
            frames, nframes, channels, bit_depth, *clip_name);
        }
      else
        {
          tmp_clip = std::make_unique<AudioClip> (2, nframes, *clip_name);
          recording = true;
        }
      z_return_if_fail (tmp_clip != nullptr);

      if (read_from_pool)
        {
          pool_id_ = AUDIO_POOL->add_clip (std::move (tmp_clip));
          z_return_if_fail (pool_id_ > -1);
          clip = AUDIO_POOL->get_clip (pool_id_);
        }
      else
        {
          clip_ = std::move (tmp_clip);
          clip = clip_.get ();
        }
    }
  else
    {
      pool_id_ = pool_id;
      clip = AUDIO_POOL->get_clip (pool_id);
      z_return_if_fail (clip);
    }

  /* set end pos to sample end */
  Position end_pos = start_pos;
  end_pos.add_frames (clip->num_frames_);

  /* init */
  init (start_pos, end_pos, track_name_hash, lane_pos, idx_inside_lane);

  (void) recording;
  z_return_if_fail (get_clip ());
}

AudioClip *
AudioRegion::get_clip () const
{
  z_return_val_if_fail (
    (!read_from_pool_ && clip_) || (read_from_pool_ && pool_id_ >= 0), nullptr);

  AudioClip * clip = nullptr;
  if (read_from_pool_) [[likely]]
    {
      z_return_val_if_fail (pool_id_ >= 0, nullptr);
      clip = AUDIO_POOL->get_clip (pool_id_);
    }
  else
    {
      clip = clip_.get ();
    }

  z_return_val_if_fail (clip && clip->num_frames_ > 0, nullptr);

  return clip;
}

void
AudioRegion::set_clip_id (int clip_id)
{
  pool_id_ = clip_id;

  /* TODO update identifier - needed? */
}

bool
AudioRegion::get_muted (bool check_parent) const
{
  if (check_parent)
    {
      auto lane = get_lane ();
      z_return_val_if_fail (lane, true);
      if (lane->is_effectively_muted ())
        return true;
    }
  return muted_;
}

void
AudioRegion::replace_frames (
  const float *    frames,
  unsigned_frame_t start_frame,
  unsigned_frame_t num_frames,
  bool             duplicate_clip)
{
  AudioClip * clip = get_clip ();
  z_return_if_fail (clip);

  if (duplicate_clip)
    {
      z_warn_if_reached ();

      int prev_id = clip->pool_id_;
      int id = AUDIO_POOL->duplicate_clip (clip->pool_id_, false);
      if (id != prev_id || id < 0)
        {
          throw ZrythmException (QObject::tr ("Failed to duplicate audio clip"));
        }
      clip = AUDIO_POOL->get_clip (id);
      z_return_if_fail (clip);

      pool_id_ = clip->pool_id_;
    }

  /* this is needed because if the file hash doesn't change
   * the actual file write is skipped to save time */
  clip->file_hash_.clear ();

  dsp_copy (
    &clip->frames_.getWritePointer (0)[start_frame * clip->channels_], frames,
    num_frames * clip->channels_);
  clip->update_channel_caches (start_frame);

  clip->write_to_pool (false, F_NOT_BACKUP);
}

static void
timestretch_buf (
  AudioTrack *        self,
  const AudioRegion * r,
  AudioClip *         clip,
  unsigned_frame_t    in_frame_offset,
  double              timestretch_ratio,
  float *             lbuf_after_ts,
  float *             rbuf_after_ts,
  unsigned_frame_t    out_frame_offset,
  unsigned_frame_t    frames_to_process)
{
  z_return_if_fail (r && self->rt_stretcher_);
  stretcher_set_time_ratio (self->rt_stretcher_, 1.0 / timestretch_ratio);
  unsigned_frame_t in_frames_to_process =
    (unsigned_frame_t) (frames_to_process * timestretch_ratio);
  z_info (
    "{}: in frame offset %" PRIu64
    ", "
    "out frame offset %" PRIu64
    ", "
    "in frames to process %" PRIu64
    ", "
    "out frames to process %" PRIu64,
    __func__, in_frame_offset, out_frame_offset, in_frames_to_process,
    frames_to_process);
  z_return_if_fail (
    (in_frame_offset + in_frames_to_process) <= clip->num_frames_);
  auto retrieved = stretcher_stretch (
    self->rt_stretcher_, &clip->ch_frames_.getReadPointer (0)[in_frame_offset],
    clip->channels_ == 1
      ? &clip->ch_frames_.getWritePointer (0)[in_frame_offset]
      : &clip->ch_frames_.getWritePointer (1)[in_frame_offset],
    in_frames_to_process, &lbuf_after_ts[out_frame_offset],
    &rbuf_after_ts[out_frame_offset], (size_t) frames_to_process);
  z_return_if_fail ((unsigned_frame_t) retrieved == frames_to_process);
}

void
AudioRegion::fill_stereo_ports (
  const EngineProcessTimeInfo &time_nfo,
  StereoPorts                 &stereo_ports) const
{
  AudioClip * clip = get_clip ();
  z_return_if_fail (clip);
  auto * track = std::get<AudioTrack *> (get_track ());

  /* if timestretching in the timeline, skip processing */
#if 0
  if (
    Q_UNLIKELY (
      ZRYTHM_HAVE_UI && MW_TIMELINE
      && MW_TIMELINE->action == UiOverlayAction::StretchingR))
    {
      dsp_fill (
        &stereo_ports.get_l ().buf_[time_nfo.local_offset_],
        DENORMAL_PREVENTION_VAL (AUDIO_ENGINE), time_nfo.nframes_);
      dsp_fill (
        &stereo_ports.get_r ().buf_[time_nfo.local_offset_],
        DENORMAL_PREVENTION_VAL (AUDIO_ENGINE), time_nfo.nframes_);
      return;
    }
#endif

  /* restretch if necessary */
  Position g_start_pos;
  g_start_pos.from_frames ((signed_frame_t) time_nfo.g_start_frame_w_offset_);
  bpm_t  cur_bpm = P_TEMPO_TRACK->get_bpm_at_pos (g_start_pos);
  double timestretch_ratio = 1.0;
  bool   needs_rt_timestretch = false;
  if (get_musical_mode () && !math_floats_equal (clip->bpm_, cur_bpm))
    {
      needs_rt_timestretch = true;
      timestretch_ratio = (double) cur_bpm / (double) clip->bpm_;
      z_debug (
        "timestretching: (cur bpm {} clip bpm {}) {}", (double) cur_bpm,
        (double) clip->bpm_, timestretch_ratio);
    }

  /* buffers after timestretch */
  auto lbuf_after_ts = tmp_bufs_[0].data ();
  auto rbuf_after_ts = tmp_bufs_[1].data ();
  dsp_fill (lbuf_after_ts, 0, time_nfo.nframes_);
  dsp_fill (rbuf_after_ts, 0, time_nfo.nframes_);

#if 0
  double r_local_ticks_at_start =
    g_start_pos.ticks - self->base.pos.ticks;
  double r_len =
    arranger_object_get_length_in_ticks (r_obj);
  double ratio =
    r_local_ticks_at_start / r_len;
  z_info ("ratio {:f}", ratio);
#endif

  signed_frame_t r_local_frames_at_start = timeline_frames_to_local (
    (signed_frame_t) time_nfo.g_start_frame_w_offset_, F_NORMALIZE);

#if 0
  Position r_local_pos_at_start;
  position_from_frames (
    &r_local_pos_at_start, r_local_frames_at_start);
  Position r_local_pos_at_end;
  position_from_frames (
    &r_local_pos_at_end,
    r_local_frames_at_start + time_nfo->nframes);

  z_info ("region");
  position_print_range (
    &self->base.pos, &self->base.end_pos_);
  z_info ("g start pos");
  position_print (&g_start_pos);
  z_info ("region local pos start/end");
  position_print_range (
    &r_local_pos_at_start, &r_local_pos_at_end);
#endif

  size_t    buff_index_start = (size_t) clip->num_frames_ + 16;
  size_t    buff_size = 0;
  nframes_t prev_offset = time_nfo.local_offset_;
  for (
    auto j =
      (unsigned_frame_t) ((r_local_frames_at_start < 0) ? -r_local_frames_at_start : 0);
    j < time_nfo.nframes_; j++)
    {
      unsigned_frame_t current_local_frame = time_nfo.local_offset_ + j;
      signed_frame_t   r_local_pos = timeline_frames_to_local (
        (signed_frame_t) (time_nfo.g_start_frame_w_offset_ + j), F_NORMALIZE);
      if (r_local_pos < 0 || j > AUDIO_ENGINE->block_length_)
        {
          z_error (
            "invalid r_local_pos {}, j {}, "
            "g_start_frames (with offset) {}, cycle offset {}, nframes {}",
            r_local_pos, j, time_nfo.g_start_frame_w_offset_,
            time_nfo.local_offset_, time_nfo.nframes_);
          return;
        }

      auto buff_index = r_local_pos;

      auto stretch = [&] () {
        timestretch_buf (
          track, this, clip, buff_index_start, timestretch_ratio, lbuf_after_ts,
          rbuf_after_ts, prev_offset,
          (unsigned_frame_t) ((current_local_frame - prev_offset) + 1));
      };

      /* if we are starting at a new
       * point in the audio clip */
      if (needs_rt_timestretch)
        {
          buff_index = (decltype (buff_index)) (buff_index * timestretch_ratio);
          if (buff_index < (decltype (buff_index)) buff_index_start)
            {
              z_info (
                "buff index ({}) < "
                "buff index start ({})",
                buff_index, buff_index_start);
              /* set the start point (
               * used when
               * timestretching) */
              buff_index_start = (size_t) buff_index;

              /* timestretch the material
               * up to this point */
              if (buff_size > 0)
                {
                  z_info ("buff size ({}) > 0", buff_size);
                  stretch ();
                  prev_offset = current_local_frame;
                }
              buff_size = 0;
            }
          /* else if last sample */
          else if (j == (time_nfo.nframes_ - 1))
            {
              stretch ();
              prev_offset = current_local_frame;
            }
          else
            {
              buff_size++;
            }
        }
      /* else if no need for timestretch */
      else
        {
          z_return_if_fail_cmp (buff_index, >=, 0);
          if (buff_index >= (decltype (buff_index)) clip->num_frames_)
            [[unlikely]]
            {
              z_error (
                "Buffer index {} exceeds {} "
                "frames in clip '{}'",
                buff_index, clip->num_frames_, clip->name_);
              return;
            }
          lbuf_after_ts[j] = clip->ch_frames_.getSample (0, buff_index);
          rbuf_after_ts[j] =
            clip->channels_ == 1
              ? clip->ch_frames_.getSample (0, buff_index)
              : clip->ch_frames_.getSample (1, buff_index);
        }
    }

  /* apply gain */
  if (!math_floats_equal (gain_, 1.f))
    {
      dsp_mul_k2 (&lbuf_after_ts[0], gain_, time_nfo.nframes_);
      dsp_mul_k2 (&rbuf_after_ts[0], gain_, time_nfo.nframes_);
    }

  /* copy frames */
  dsp_copy (
    &stereo_ports.get_l ().buf_[time_nfo.local_offset_], &lbuf_after_ts[0],
    time_nfo.nframes_);
  dsp_copy (
    &stereo_ports.get_r ().buf_[time_nfo.local_offset_], &rbuf_after_ts[0],
    time_nfo.nframes_);

  /* apply fades */
  const signed_frame_t num_frames_in_fade_in_area = fade_in_pos_.frames_;
  const signed_frame_t num_frames_in_fade_out_area =
    end_pos_->frames_ - (fade_out_pos_.frames_ + pos_->frames_);
  const signed_frame_t local_builtin_fade_out_start_frames =
    end_pos_->frames_ - (AUDIO_REGION_BUILTIN_FADE_FRAMES + pos_->frames_);
  for (nframes_t j = 0; j < time_nfo.nframes_; j++)
    {
      const unsigned_frame_t current_cycle_frame = time_nfo.local_offset_ + j;

      /* current frame local to region start */
      const signed_frame_t current_local_frame =
        (signed_frame_t) (time_nfo.g_start_frame_w_offset_ + j) - pos_->frames_;

      /* skip to fade out (or builtin fade out) if not in any fade area */
      if (
        current_local_frame >= std::max (
          fade_in_pos_.frames_,
          static_cast<decltype (fade_in_pos_.frames_)> (
            AUDIO_REGION_BUILTIN_FADE_FRAMES))
        && current_local_frame < std::min (
             fade_out_pos_.frames_, local_builtin_fade_out_start_frames))
        [[likely]]
        {
          j +=
            std::min (fade_out_pos_.frames_, local_builtin_fade_out_start_frames)
            - current_local_frame;
          j--;
          continue;
        }

      /* if inside object fade in */
      if (
        current_local_frame >= 0
        && current_local_frame < num_frames_in_fade_in_area)
        {
          float fade_in = (float) fade_get_y_normalized (
            fade_in_opts_,
            (double) current_local_frame / (double) num_frames_in_fade_in_area,
            true);

          stereo_ports.get_l ().buf_[current_cycle_frame] *= fade_in;
          stereo_ports.get_r ().buf_[current_cycle_frame] *= fade_in;
        }
      /* if inside object fade out */
      if (current_local_frame >= fade_out_pos_.frames_)
        {
          z_return_if_fail_cmp (num_frames_in_fade_out_area, >, 0);
          signed_frame_t num_frames_from_fade_out_start =
            current_local_frame - fade_out_pos_.frames_;
          z_return_if_fail_cmp (
            num_frames_from_fade_out_start, <=, num_frames_in_fade_out_area);
          float fade_out = (float) fade_get_y_normalized (
            fade_out_opts_,
            (double) num_frames_from_fade_out_start
              / (double) num_frames_in_fade_out_area,
            false);

          stereo_ports.get_l ().buf_[current_cycle_frame] *= fade_out;
          stereo_ports.get_r ().buf_[current_cycle_frame] *= fade_out;
        }
      /* if inside builtin fade in, apply builtin fade in */
      if (
        current_local_frame >= 0
        && current_local_frame < AUDIO_REGION_BUILTIN_FADE_FRAMES)
        {
          float fade_in =
            (float) current_local_frame
            / (float) AUDIO_REGION_BUILTIN_FADE_FRAMES;

          stereo_ports.get_l ().buf_[current_cycle_frame] *= fade_in;
          stereo_ports.get_r ().buf_[current_cycle_frame] *= fade_in;
        }
      /* if inside builtin fade out, apply builtin
       * fade out */
      if (current_local_frame >= local_builtin_fade_out_start_frames)
        {
          signed_frame_t num_frames_from_fade_out_start =
            current_local_frame - local_builtin_fade_out_start_frames;
          z_return_if_fail_cmp (
            num_frames_from_fade_out_start, <=,
            AUDIO_REGION_BUILTIN_FADE_FRAMES);
          float fade_out =
            1.f
            - ((float) num_frames_from_fade_out_start / (float) AUDIO_REGION_BUILTIN_FADE_FRAMES);

          stereo_ports.get_l ().buf_[current_cycle_frame] *= fade_out;
          stereo_ports.get_r ().buf_[current_cycle_frame] *= fade_out;
        }
    }
}

float
AudioRegion::detect_bpm (std::vector<float> &candidates)
{
  AudioClip * clip = get_clip ();
  z_return_val_if_fail (clip, 0.f);

  return audio_detect_bpm (
    clip->ch_frames_.getReadPointer (0), (size_t) clip->num_frames_,
    (unsigned int) AUDIO_ENGINE->sample_rate_, candidates);
}

bool
AudioRegion::get_musical_mode () const
{
#if ZRYTHM_TARGET_VER_MAJ == 1
  /* off for v1 */
  return false;
#endif

  switch (musical_mode_)
    {
    case MusicalMode::Inherit:
      return gui::SettingsManager::musicalMode ();
    case MusicalMode::Off:
      return false;
    case MusicalMode::On:
      return true;
    }
  z_return_val_if_reached (false);
}

bool
AudioRegion::fix_positions (double frames_per_tick)
{
  AudioClip * clip = get_clip ();
  z_return_val_if_fail (clip, false);

  /* verify that the loop does not contain more frames than available in the
   * clip */
  /* use global positions because sometimes the loop appears to have 1 more
   * frame due to rounding to nearest frame*/
  signed_frame_t loop_start_global = Position::get_frames_from_ticks (
    pos_->ticks_ + loop_start_pos_.ticks_, frames_per_tick);
  signed_frame_t loop_end_global = Position::get_frames_from_ticks (
    pos_->ticks_ + loop_end_pos_.ticks_, frames_per_tick);
  signed_frame_t loop_len = loop_end_global - loop_start_global;
  /*z_debug ("loop  len: %" SIGNED_FRAME_FORMAT, loop_len);*/
  signed_frame_t region_len = end_pos_->frames_ - pos_->frames_;

  signed_frame_t extra_loop_frames =
    loop_len - (signed_frame_t) clip->num_frames_;
  if (extra_loop_frames > 0)
    {
      /* only fix if the difference is 1 frame, which happens
       * due to rounding - if more than 1 then some other big
       * problem exists */
      if (extra_loop_frames == 1)
        {
          z_debug (
            "fixing position for audio region. before: {}", print_to_str ());
          if (loop_len == region_len)
            {
              end_pos_->add_frames (-1);
              if (fade_out_pos_.frames_ == loop_end_pos_.frames_)
                {
                  fade_out_pos_.add_frames (-1);
                }
            }
          loop_end_pos_.add_frames (-1);
          z_debug (
            "fixed position for audio region. after: {}", print_to_str ());
          return true;
        }
      else
        {
          z_error (fmt::format (
            "Audio region loop length in frames ({}) is greater than the number of frames in the clip ({}). ",
            loop_len, clip->num_frames_));
          return false;
        }
    }

  return false;
}

std::optional<ClipEditorArrangerSelectionsPtrVariant>
AudioRegion::get_arranger_selections () const
{
  return AUDIO_SELECTIONS;
}

bool
AudioRegion::validate (bool is_project, double frames_per_tick) const
{
  if (PROJECT->loaded_ && !AUDIO_ENGINE->updating_frames_per_tick_)
    {
      auto clip = get_clip ();
      z_return_val_if_fail (clip, false);

      /* verify that the loop does not contain more frames than available in
      the clip. use global positions because sometimes the loop appears to have
      1 more frame due to rounding to nearest frame*/
      signed_frame_t loop_start_global = Position::get_frames_from_ticks (
        pos_->ticks_ + loop_start_pos_.ticks_, frames_per_tick);
      signed_frame_t loop_end_global = Position::get_frames_from_ticks (
        pos_->ticks_ + loop_end_pos_.ticks_, frames_per_tick);
      signed_frame_t loop_len = loop_end_global - loop_start_global;

      if (loop_len > (signed_frame_t) clip->num_frames_)

        {
          z_error (
            "Audio region loop length in frames ({}) is greater than the "
            "number of frames in the clip ({})",
            loop_len, clip->num_frames_);
          return false;
        }
    }

  return true;
}

void
AudioRegion::init_loaded ()
{
  ArrangerObject::init_loaded_base ();
  NameableObject::init_loaded_base ();
  read_from_pool_ = true;
  z_return_if_fail (get_clip ());
}

void
AudioRegion::init_after_cloning (const AudioRegion &other)
{
  init_default_constructed (
    other.pool_id_, std::nullopt, true,
    other.clip_ ? other.clip_->frames_.getReadPointer (0) : nullptr,
    other.clip_ ? other.clip_->num_frames_ : 0,
    other.clip_ ? std::make_optional (other.clip_->name_) : std::nullopt,
    other.clip_ ? other.clip_->channels_ : 0,
    other.clip_ ? other.clip_->bit_depth_ : ENUM_INT_TO_VALUE (BitDepth, 0),
    *other.pos_, other.id_.track_name_hash_, other.id_.lane_pos_,
    other.id_.idx_);
  pool_id_ = other.pool_id_;
  gain_ = other.gain_;
  musical_mode_ = other.musical_mode_;
  LaneOwnedObjectImpl::copy_members_from (other);
  RegionImpl::copy_members_from (other);
  FadeableObject::copy_members_from (other);
  TimelineObject::copy_members_from (other);
  NameableObject::copy_members_from (other);
  LoopableObject::copy_members_from (other);
  MuteableObject::copy_members_from (other);
  LengthableObject::copy_members_from (other);
  ColoredObject::copy_members_from (other);
  ArrangerObject::copy_members_from (other);
}