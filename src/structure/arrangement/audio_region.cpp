// SPDX-FileCopyrightText: © 2018-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/arranger_object_all.h"
#include "utils/debug.h"
#include "utils/dsp.h"
#include "utils/logger.h"
#include "utils/math.h"

#include <fmt/format.h>

namespace zrythm::structure::arrangement
{

AudioRegion::AudioRegion (
  const dsp::TempoMap          &tempo_map,
  ArrangerObjectRegistry       &object_registry,
  dsp::FileAudioSourceRegistry &file_audio_source_registry,
  GlobalMusicalModeGetter       musical_mode_getter,
  QObject *                     parent) noexcept
    : QObject (parent), ArrangerObject (Type::AudioRegion, tempo_map, *this),
      ArrangerObjectOwner (object_registry, file_audio_source_registry, *this),
      file_audio_source_registry_ (file_audio_source_registry),
      region_mixin_ (utils::make_qobject_unique<RegionMixin> (*position ())),
      fade_range_ (
        utils::make_qobject_unique<ArrangerObjectFadeRange> (tempo_map, this)),
      global_musical_mode_getter_ (std::move (musical_mode_getter))
{
}

void
AudioRegion::set_source (const ArrangerObjectUuidReference &source)
{
  add_object (source);
}

void
AudioRegion::fill_stereo_ports (
  const EngineProcessTimeInfo                  &time_nfo,
  std::pair<std::span<float>, std::span<float>> stereo_output) const
{
  /* if timestretching in the timeline, skip processing */
#if 0
  if (
    Q_UNLIKELY (
      ZRYTHM_HAVE_UI && MW_TIMELINE
      && MW_TIMELINE->action == UiOverlayAction::StretchingR))
    {
      utils::float_ranges::fill (
        &stereo_ports.get_l ().buf_[time_nfo.local_offset_],
        DENORMAL_PREVENTION_VAL (AUDIO_ENGINE), time_nfo.nframes_);
      utils::float_ranges::fill (
        &stereo_ports.get_r ().buf_[time_nfo.local_offset_],
        DENORMAL_PREVENTION_VAL (AUDIO_ENGINE), time_nfo.nframes_);
      return;
    }
#endif

  auto &audio_source = get_audio_source ();

  /* restretch if necessary */
  // TODO
  double timestretch_ratio = 1.0;
  bool   needs_rt_timestretch = false;
#if 0
  Position g_start_pos;
  g_start_pos.from_frames (
    (signed_frame_t) time_nfo.g_start_frame_w_offset_,
    AUDIO_ENGINE->ticks_per_frame_);
  bpm_t  cur_bpm = P_TEMPO_TRACK->get_bpm_at_pos (g_start_pos);
  if (
    get_musical_mode ()
    && !utils::math::floats_equal (clip->get_bpm (), cur_bpm))
    {
      needs_rt_timestretch = true;
      timestretch_ratio = (double) cur_bpm / (double) clip->get_bpm ();
      z_debug (
        "timestretching: (cur bpm {} clip bpm {}) {}", (double) cur_bpm,
        (double) clip->get_bpm (), timestretch_ratio);
    }
#endif

  /* buffers after timestretch */
  auto * lbuf_after_ts = tmp_buf_->getWritePointer (0);
  auto * rbuf_after_ts = tmp_buf_->getWritePointer (1);
  utils::float_ranges::fill (lbuf_after_ts, 0, time_nfo.nframes_);
  utils::float_ranges::fill (rbuf_after_ts, 0, time_nfo.nframes_);

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
    *this, (signed_frame_t) time_nfo.g_start_frame_w_offset_, true);

  size_t buff_index_start = audio_source.getTotalLength () + 16;
  size_t buff_size = 0;
  [[maybe_unused]] nframes_t prev_offset = time_nfo.local_offset_;
  for (
    auto j =
      (unsigned_frame_t) ((r_local_frames_at_start < 0) ? -r_local_frames_at_start : 0);
    j < time_nfo.nframes_; j++)
    {
      unsigned_frame_t current_local_frame = time_nfo.local_offset_ + j;
      signed_frame_t   r_local_pos = timeline_frames_to_local (
        *this, (signed_frame_t) (time_nfo.g_start_frame_w_offset_ + j), true);

      auto buff_index = r_local_pos;

      auto stretch = [&] () {
// TODO
#if 0
        auto * track = std::get<tracks::AudioTrack *> (get_track ());
        track->timestretch_buf (
          this, clip, buff_index_start, timestretch_ratio, lbuf_after_ts,
          rbuf_after_ts, prev_offset,
          (unsigned_frame_t) ((current_local_frame - prev_offset) + 1));
#endif
      };

      /* if we are starting at a new
       * point in the audio clip */
      if (needs_rt_timestretch)
        {
          buff_index =
            (decltype (buff_index)) (static_cast<double> (buff_index)
                                     * timestretch_ratio);
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
          if (
            buff_index >= (decltype (buff_index)) audio_source.getTotalLength ())
            [[unlikely]]
            {
              z_error (
                "Buffer index {} exceeds {} "
                "frames in clip",
                buff_index, audio_source.getTotalLength ());
              return;
            }

          // get samples from source
          // FIXME: this is extremely inefficient - we are reading 1 sample each
          // time
          {
            audio_source.setNextReadPosition (buff_index);
            juce::AudioSourceChannelInfo source_ch_nfo{
              audio_source_buffer_.get (), 0, 1
            };
            audio_source.getNextAudioBlock (source_ch_nfo);
          }

          lbuf_after_ts[j] = audio_source_buffer_->getSample (0, 0);
          rbuf_after_ts[j] = audio_source_buffer_->getSample (1, 0);
        }
    }

  /* apply gain */
  const auto current_gain = gain ();
  if (!utils::math::floats_equal (current_gain, 1.f))
    {
      utils::float_ranges::mul_k2 (
        &lbuf_after_ts[0], current_gain, time_nfo.nframes_);
      utils::float_ranges::mul_k2 (
        &rbuf_after_ts[0], current_gain, time_nfo.nframes_);
    }

  /* copy frames */
  utils::float_ranges::copy (
    &stereo_output.first[time_nfo.local_offset_], &lbuf_after_ts[0],
    time_nfo.nframes_);
  utils::float_ranges::copy (
    &stereo_output.second[time_nfo.local_offset_], &rbuf_after_ts[0],
    time_nfo.nframes_);

  /* apply fades */
  const auto region_position_in_frames = position ()->samples ();
  const auto region_length_in_frames =
    region_mixin_->bounds ()->length ()->samples ();
  const auto fade_in_pos_in_frames = fade_range_->startOffset ()->samples ();
  const auto fade_out_pos_in_frames =
    region_length_in_frames - fade_range_->endOffset ()->samples ();
  const signed_frame_t num_frames_in_fade_in_area = fade_in_pos_in_frames;
  const signed_frame_t num_frames_in_fade_out_area =
    fade_range_->endOffset ()->samples ();
  const signed_frame_t local_builtin_fade_out_start_frames =
    region_length_in_frames - BUILTIN_FADE_FRAMES;
  for (nframes_t j = 0; j < time_nfo.nframes_; j++)
    {
      const unsigned_frame_t current_cycle_frame = time_nfo.local_offset_ + j;

      /* current frame local to region start */
      const signed_frame_t current_local_frame =
        (signed_frame_t) (time_nfo.g_start_frame_w_offset_ + j)
        - region_position_in_frames;

      /* skip to fade out (or builtin fade out) if not in any fade area */
      if (
        current_local_frame >= std::max<signed_frame_t> (
          fade_in_pos_in_frames, BUILTIN_FADE_FRAMES)
        && current_local_frame < std::min (
             fade_out_pos_in_frames,
             static_cast<qint64> (local_builtin_fade_out_start_frames)))
        [[likely]]
        {
          j +=
            std::min (
              fade_out_pos_in_frames,
              static_cast<qint64> (local_builtin_fade_out_start_frames))
            - current_local_frame;
          j--;
          continue;
        }

      /* if inside object fade in */
      if (
        current_local_frame >= 0
        && current_local_frame < num_frames_in_fade_in_area)
        {
          auto fade_in = (float) fade_range_->get_normalized_y_for_fade (
            (double) current_local_frame / (double) num_frames_in_fade_in_area,
            true);

          stereo_output.first[current_cycle_frame] *= fade_in;
          stereo_output.second[current_cycle_frame] *= fade_in;
        }
      /* if inside object fade out */
      if (current_local_frame >= fade_out_pos_in_frames)
        {
          z_return_if_fail_cmp (num_frames_in_fade_out_area, >, 0);
          signed_frame_t num_frames_from_fade_out_start =
            current_local_frame - fade_out_pos_in_frames;
          z_return_if_fail_cmp (
            num_frames_from_fade_out_start, <=, num_frames_in_fade_out_area);
          auto fade_out = (float) fade_range_->get_normalized_y_for_fade (
            (double) num_frames_from_fade_out_start
              / (double) num_frames_in_fade_out_area,
            false);

          stereo_output.first[current_cycle_frame] *= fade_out;
          stereo_output.second[current_cycle_frame] *= fade_out;
        }
      /* if inside builtin fade in, apply builtin fade in */
      if (current_local_frame >= 0 && current_local_frame < BUILTIN_FADE_FRAMES)
        {
          float fade_in =
            (float) current_local_frame / (float) BUILTIN_FADE_FRAMES;

          stereo_output.first[current_cycle_frame] *= fade_in;
          stereo_output.second[current_cycle_frame] *= fade_in;
        }
      /* if inside builtin fade out, apply builtin fade out */
      if (current_local_frame >= local_builtin_fade_out_start_frames)
        {
          signed_frame_t num_frames_from_fade_out_start =
            current_local_frame - local_builtin_fade_out_start_frames;
          z_return_if_fail_cmp (
            num_frames_from_fade_out_start, <=, BUILTIN_FADE_FRAMES);
          float fade_out =
            1.f
            - ((float) num_frames_from_fade_out_start / (float) BUILTIN_FADE_FRAMES);

          stereo_output.first[current_cycle_frame] *= fade_out;
          stereo_output.second[current_cycle_frame] *= fade_out;
        }
    }
}

bool
AudioRegion::effectivelyInMusicalMode () const
{
  switch (musical_mode_)
    {
    case MusicalMode::Inherit:
      return global_musical_mode_getter_ ();
    case MusicalMode::Off:
      return false;
    case MusicalMode::On:
      return true;
    }
  z_return_val_if_reached (false);
}

void
init_from (
  AudioRegion           &obj,
  const AudioRegion     &other,
  utils::ObjectCloneType clone_type)
{
  obj.gain_.store (other.gain_.load ());
  obj.musical_mode_ = other.musical_mode_;
  init_from (
    static_cast<ArrangerObject &> (obj),
    static_cast<const ArrangerObject &> (other), clone_type);
  init_from (
    static_cast<ArrangerObjectOwner<AudioSourceObject> &> (obj),
    static_cast<const ArrangerObjectOwner<AudioSourceObject> &> (other),
    clone_type);
  init_from (*obj.region_mixin_, *other.region_mixin_, clone_type);
  init_from (*obj.fade_range_, *other.fade_range_, clone_type);
}

juce::PositionableAudioSource &
AudioRegion::get_audio_source () const
{
  assert (get_children_vector ().size () == 1);
  auto * audio_source_obj = get_children_view ()[0];
  return audio_source_obj->get_audio_source ();
}

void
AudioRegion::prepare_to_play (size_t max_expected_samples, double sample_rate)
{
  tmp_buf_ = std::make_unique<juce::AudioSampleBuffer> (2, max_expected_samples);
  audio_source_buffer_ =
    std::make_unique<juce::AudioSampleBuffer> (2, max_expected_samples);
  get_audio_source ().prepareToPlay (
    static_cast<int> (max_expected_samples), sample_rate);
}

void
AudioRegion::release_resources ()
{
  tmp_buf_.reset ();
  audio_source_buffer_.reset ();
  get_audio_source ().releaseResources ();
}
}
