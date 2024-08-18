// SPDX-FileCopyrightText: © 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/audio.h"
#include "utils/debug.h"
#include "utils/exceptions.h"
#include "utils/math.h"
#include "utils/objects.h"
#include "utils/resampler.h"

#include <glib/gi18n.h>

#include <soxr.h>

constexpr size_t SOXR_BLOCK_SZ = 1028;

class Resampler::Impl
{
public:
  Impl (
    const float *      in_frames,
    const size_t       num_in_frames,
    const double       input_rate,
    const double       output_rate,
    const unsigned int num_channels,
    const Quality      quality)
  {
    z_return_if_fail (in_frames);

    z_debug (
      "creating new resampler for {} frames at {:.2f} Hz to {:.2f} Hz - {} channels",
      num_in_frames, input_rate, output_rate, num_channels);

    in_frames_.resize (num_in_frames * num_channels);
    std::copy_n (in_frames, num_in_frames * num_channels, in_frames_.data ());
    num_in_frames_ = num_in_frames;
    input_rate_ = input_rate;
    output_rate_ = output_rate;
    num_channels_ = num_channels;
    quality_ = quality;

    unsigned long quality_recipe, quality_flags = 0;
    switch (quality)
      {
      case Quality::Quick:
        quality_recipe = SOXR_QQ;
        break;
      case Quality::Low:
        quality_recipe = SOXR_LQ;
        break;
      case Quality::Medium:
        quality_recipe = SOXR_MQ;
        break;
      case Quality::High:
        quality_recipe = SOXR_HQ;
        break;
      case Quality::VeryHigh:
      default:
        quality_recipe = SOXR_VHQ;
        break;
      }
    soxr_quality_spec_t quality_spec =
      soxr_quality_spec (quality_recipe, quality_flags);

    soxr_error_t serror;
    soxr_t       soxr = soxr_create (
      input_rate, output_rate, num_channels, &serror, nullptr, &quality_spec,
      nullptr);

    if (serror)
      {
        throw ZrythmException (
          format_str (_ ("Failed to create soxr instance: {}"), serror));
      }

    serror = soxr_set_input_fn (soxr, input_func, this, SOXR_BLOCK_SZ);
    if (serror)
      {
        throw ZrythmException (
          format_str (_ ("Failed to set soxr input function: {}"), serror));
      }

    priv_ = soxr;

    double resample_ratio = output_rate / input_rate;
    num_out_frames_ = (size_t) ceil ((double) num_in_frames_ * resample_ratio);
    /* soxr accesses a few bytes outside apparently so allocate a bit more */
    out_frames_.resize ((num_out_frames_ + 8) * num_channels_);
  }

  ~Impl () { soxr_delete (priv_); }

  static size_t
  input_func (void * input_func_state, soxr_in_t * data, size_t requested_len)
  {
    auto * self = static_cast<Resampler::Impl *> (input_func_state);

    z_return_val_if_fail_cmp (self->num_in_frames_, >=, self->frames_read_, 0);
    size_t len_to_provide =
      std::min (self->num_in_frames_ - self->frames_read_, requested_len);

    if (len_to_provide == 0)
      {
        return 0;
      }

    *data = &self->in_frames_[self->frames_read_ * self->num_channels_];

    self->frames_read_ += len_to_provide;

#if 0
  z_debug (
    "providing %zu frames (%zu requested). total frames read %zu",
    len_to_provide, requested_len, self->frames_read);
#endif

    return len_to_provide;
  }

  void               process ();
  bool               is_done () const;
  std::vector<float> get_out_frames () const;

public:
  /** Private data. */
  soxr_t priv_ = nullptr;

  double       input_rate_ = 0;
  double       output_rate_ = 0;
  unsigned int num_channels_ = 0;

  /** Given input (interleaved) frames .*/
  std::vector<float> in_frames_;

  /** Number of frames per channel. */
  size_t num_in_frames_ = 0;

  /** Number of frames read so far. */
  size_t frames_read_ = 0;

  /** Output (interleaved) frames to be allocated during
   * resampler_new(). */
  std::vector<float> out_frames_;

  /** Number of frames per channel. */
  size_t num_out_frames_ = 0;

  /** Number of frames written so far. */
  size_t frames_written_ = 0;

  Quality quality_;
};

Resampler::Resampler (
  const float *      in_frames,
  const size_t       num_in_frames,
  const double       input_rate,
  const double       output_rate,
  const unsigned int num_channels,
  const Quality      quality)
    : pimpl_ (std::make_unique<Impl> (
        in_frames,
        num_in_frames,
        input_rate,
        output_rate,
        num_channels,
        quality))
{
}

// Destructor needs to be defined here where Impl is a complete type
Resampler::~Resampler () = default;

void
Resampler::Impl::process ()
{
  z_return_if_fail (num_out_frames_ > frames_written_);
  size_t frames_to_write_this_time =
    std::min (num_out_frames_ - frames_written_, SOXR_BLOCK_SZ);
  z_return_if_fail_cmp (
    frames_written_ + frames_to_write_this_time, <=, num_out_frames_);
  size_t frames_written_now = soxr_output (
    (soxr_t) priv_, &out_frames_[frames_written_ * num_channels_],
    frames_to_write_this_time);
  z_return_if_fail_cmp (
    frames_written_ + frames_written_now, <=, num_out_frames_);

  soxr_error_t serror = soxr_error ((soxr_t) priv_);
  if (serror)
    {
      throw ZrythmException (
        format_str (_ ("soxr_process() error: {}"), serror));
    }

  if (math_doubles_equal (input_rate_, output_rate_))
    {
      audio_frames_equal (
        &in_frames_[frames_written_ * num_channels_],
        &out_frames_[frames_written_ * num_channels_],
        frames_written_now * num_channels_, 0.00000001f);
    }

  z_return_if_fail (frames_written_ + frames_written_now <= num_out_frames_);
  frames_written_ += frames_written_now;

#if 0
  z_debug (
    "resampler processed: frames written now %zu, total frames written %zu, expected total frames %zu",
    frames_written_now, self->frames_written,
    self->num_out_frames);
#endif

  if (frames_written_now == 0)
    {
      /* in case the calculation for out frames is off by 1,
       * assume finished */
      z_info (
        "no more frames to write at %zu frames (expected %zu frames)",
        frames_written_, num_out_frames_);
      num_out_frames_ = frames_written_;
    }
}

bool
Resampler::Impl::is_done () const
{
  return frames_written_ == num_out_frames_;
}

std::vector<float>
Resampler::Impl::get_out_frames () const
{
  return out_frames_;
}

void
Resampler::process ()
{
  pimpl_->process ();
}

bool
Resampler::is_done () const
{
  return pimpl_->is_done ();
}

std::vector<float>
Resampler::get_out_frames () const
{
  return pimpl_->get_out_frames ();
}