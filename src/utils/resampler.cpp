// SPDX-FileCopyrightText: © 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/audio.h"
#include "utils/debug.h"
#include "utils/exceptions.h"
#include "utils/math.h"
#include "utils/resampler.h"

#include <soxr.h>

class Resampler::Impl
{
public:
  Impl (
    const juce::AudioSampleBuffer &in_frames,
    const double                   input_rate,
    const double                   output_rate,
    const Quality                  quality,
    size_t                         block_size)
      : input_rate_ (input_rate), output_rate_ (output_rate),
        block_size_ (block_size), in_frames_ (in_frames), quality_ (quality)
  {
    z_debug (
      "creating new resampler for {} frames (per channel) in {} channels at {:.2f} Hz to {:.2f} Hz",
      in_frames.getNumSamples (), in_frames.getNumChannels (), input_rate,
      output_rate);

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

    auto io_spec = soxr_io_spec (SOXR_FLOAT32_I, SOXR_FLOAT32_I);

    soxr_error_t serror;
    priv_ = soxr_create (
      input_rate, output_rate, in_frames_.getNumChannels (), &serror, &io_spec,
      &quality_spec, nullptr);

    if (serror)
      {
        throw ZrythmException (
          format_str (_ ("Failed to create soxr instance: {}"), serror));
      }

    serror = soxr_set_input_fn (priv_, input_func, this, block_size_);
    if (serror)
      {
        throw ZrythmException (
          format_str (_ ("Failed to set soxr input function: {}"), serror));
      }
  }

  ~Impl () { soxr_delete (priv_); }

  static size_t input_func (
    void *                          input_func_state,
    soxr_in_t /* sorxr_cbufs_t */ * data,
    size_t                          requested_len)
  {
    auto * self = static_cast<Resampler::Impl *> (input_func_state);

    const auto num_channels = self->in_frames_.getNumChannels ();
    z_return_val_if_fail_cmp (
      self->in_frames_.getNumSamples (), >=, (int) self->frames_read_, 0);
    size_t len_to_provide = std::min (
      self->in_frames_.getNumSamples () - self->frames_read_, requested_len);

    if (len_to_provide == 0)
      {
        return 0;
      }

    self->interleaved_in_.resize (len_to_provide * num_channels);
    for (size_t i = 0; i < len_to_provide; ++i)
      {
        for (int channel = 0; channel < num_channels; ++channel)
          {
            self->interleaved_in_[i * num_channels + channel] =
              self->in_frames_.getSample (channel, self->frames_read_ + i);
          }
      }
    *data = self->interleaved_in_.data ();

    self->frames_read_ += len_to_provide;

    z_trace (
      "providing {} frames ({} requested). total frames read {}",
      len_to_provide, requested_len, self->frames_read_);

    return len_to_provide;
  }

  void                    process ();
  bool                    is_done () const;
  juce::AudioSampleBuffer get_out_frames () const;

public:
  /** Private data. */
  soxr_t priv_ = nullptr;

  double input_rate_ = 0;
  double output_rate_ = 0;
  size_t block_size_ = 0;

  /** Given input (interleaved) frames .*/
  const juce::AudioSampleBuffer &in_frames_;

  /** Number of frames read so far. */
  size_t frames_read_ = 0;

  /** Resulting frames. */
  juce::AudioSampleBuffer out_frames_;

  /** Number of frames written so far. */
  size_t frames_written_ = 0;

  Quality quality_;

  std::vector<float> interleaved_in_;
  std::vector<float> interleaved_out_;

  std::atomic_bool done_{ false };
};

Resampler::Resampler (
  const juce::AudioSampleBuffer &in_frames,
  const double                   input_rate,
  const double                   output_rate,
  const Quality                  quality,
  size_t                         block_size)
    : pimpl_ (std::make_unique<
              Impl> (in_frames, input_rate, output_rate, quality, block_size))
{
}

// Destructor needs to be defined here where Impl is a complete type
Resampler::~Resampler () = default;

void
Resampler::Impl::process ()
{
  size_t frames_to_write_this_time = block_size_;

  // prepare interlaved buffer
  const int num_channels = in_frames_.getNumChannels ();
  interleaved_out_.resize (frames_to_write_this_time * num_channels);

  size_t frames_written_now = soxr_output (
    (soxr_t) priv_, interleaved_out_.data (), frames_to_write_this_time);

  soxr_error_t serror = soxr_error ((soxr_t) priv_);
  if (serror)
    {
      throw ZrythmException (
        format_str (_ ("soxr_process() error: {}"), serror));
    }

  // De-interleave the output data into out_frames_
  out_frames_.setSize (
    num_channels, frames_written_ + frames_written_now, true, false, true);
  for (size_t i = 0; i < frames_written_now; ++i)
    {
      for (int channel = 0; channel < num_channels; ++channel)
        {
          out_frames_.setSample (
            channel, frames_written_ + i,
            interleaved_out_[i * num_channels + channel]);
        }
    }

  if (math_doubles_equal (input_rate_, output_rate_))
    {
      for (int i = 0; i < num_channels; ++i)
        {
          audio_frames_equal (
            in_frames_.getReadPointer (i, frames_written_),
            out_frames_.getReadPointer (i, frames_written_), frames_written_now,
            0.00000001f);
        }
    }

  frames_written_ += frames_written_now;

  z_trace (
    "resampler processed: frames written now {}, total frames written {}, expected total frames {}",
    frames_written_now, frames_written_, in_frames_.getNumSamples ());

  if (frames_written_now == 0)
    {
      done_.store (true);
    }
}

bool
Resampler::Impl::is_done () const
{
  return done_.load ();
}

juce::AudioSampleBuffer
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

juce::AudioSampleBuffer
Resampler::get_out_frames () const
{
  if (!is_done ())
    {
      throw ZrythmException ("Cannot get out frames before resampler is done");
    }

  // resize because we might have allocated more frames than needed
  return pimpl_->get_out_frames ();
}