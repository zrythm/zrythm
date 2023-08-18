// SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/audio.h"
#include "utils/math.h"
#include "utils/objects.h"
#include "utils/resampler.h"

#include <glib/gi18n.h>

#include <soxr.h>

typedef enum
{
  Z_UTILS_RESAMPLER_ERROR_FAILED,
} ZUtilsResamplerError;

#define Z_UTILS_RESAMPLER_ERROR \
  z_utils_resampler_error_quark ()
GQuark
z_utils_resampler_error_quark (void);
G_DEFINE_QUARK (
  z - utils - resampler - error - quark,
  z_utils_resampler_error)

#define SOXR_BLOCK_SZ 1028

static size_t
input_func (
  void *      input_func_state,
  soxr_in_t * data,
  size_t      requested_len)
{
  Resampler * self = (Resampler *) input_func_state;

  size_t len_to_provide = MIN (
    self->num_in_frames - self->frames_read, requested_len);

  *data =
    &self->in_frames[self->frames_read * self->num_channels];

  self->frames_read += len_to_provide;

#if 0
  g_debug (
    "providing %zu frames (%zu requested). total frames read %zu",
    len_to_provide, requested_len, self->frames_read);
#endif

  return len_to_provide;
}

/**
 * Creates a new instance of a Resampler with the given
 * settings.
 *
 * Resampler.num_out_frames will be set to the number of output
 * frames required for resampling with the given settings.
 */
Resampler *
resampler_new (
  const float *          in_frames,
  const size_t           num_in_frames,
  const double           input_rate,
  const double           output_rate,
  const unsigned int     num_channels,
  const ResamplerQuality quality,
  GError **              error)
{
  g_return_val_if_fail (in_frames, NULL);

  Resampler * self = object_new (Resampler);

  g_debug (
    "creating new resampler for %zu frames at %.2f Hz to %.2f Hz - %u channels",
    num_in_frames, input_rate, output_rate, num_channels);

  self->in_frames = in_frames;
  self->num_in_frames = num_in_frames;
  self->input_rate = input_rate;
  self->output_rate = output_rate;
  self->num_channels = num_channels;
  self->quality = quality;

  unsigned long quality_recipe, quality_flags = 0;
  switch (quality)
    {
    case RESAMPLER_QUALITY_QUICK:
      quality_recipe = SOXR_QQ;
      break;
    case RESAMPLER_QUALITY_LOW:
      quality_recipe = SOXR_LQ;
      break;
    case RESAMPLER_QUALITY_MEDIUM:
      quality_recipe = SOXR_MQ;
      break;
    case RESAMPLER_QUALITY_HIGH:
      quality_recipe = SOXR_HQ;
      break;
    case RESAMPLER_QUALITY_VERY_HIGH:
    default:
      quality_recipe = SOXR_VHQ;
      break;
    }
  soxr_quality_spec_t quality_spec =
    soxr_quality_spec (quality_recipe, quality_flags);

  soxr_error_t serror;
  soxr_t       soxr = soxr_create (
    input_rate, output_rate, num_channels, &serror, NULL,
    &quality_spec, NULL);

  if (serror)
    {
      g_set_error (
        error, Z_UTILS_RESAMPLER_ERROR,
        Z_UTILS_RESAMPLER_ERROR_FAILED,
        _ ("Failed to create soxr instance: %s"), serror);
      return NULL;
    }

  serror =
    soxr_set_input_fn (soxr, input_func, self, SOXR_BLOCK_SZ);
  if (serror)
    {
      g_set_error (
        error, Z_UTILS_RESAMPLER_ERROR,
        Z_UTILS_RESAMPLER_ERROR_FAILED,
        _ ("Failed to set soxr input function: %s"), serror);
      return NULL;
    }

  self->priv = soxr;

  double resample_ratio = output_rate / input_rate;
  self->num_out_frames = (size_t) ceil (
    (double) self->num_in_frames * resample_ratio);
  self->out_frames = object_new_n (
    self->num_out_frames * self->num_channels, float);

  return self;
}

/**
 * To be called periodically until resampler_is_done() returns
 * true.
 */
bool
resampler_process (Resampler * self, GError ** error)
{
  size_t frames_written_now = soxr_output (
    (soxr_t) self->priv,
    &self->out_frames[self->frames_written * self->num_channels],
    MIN (
      self->num_out_frames - self->frames_written,
      SOXR_BLOCK_SZ));

  soxr_error_t serror = soxr_error ((soxr_t) self->priv);
  if (serror)
    {
      g_set_error (
        error, Z_UTILS_RESAMPLER_ERROR,
        Z_UTILS_RESAMPLER_ERROR_FAILED,
        _ ("soxr_process() error: %s"), serror);
      return false;
    }

  if (math_doubles_equal (self->input_rate, self->output_rate))
    {
      audio_frames_equal (
        &self->in_frames
           [self->frames_written * self->num_channels],
        &self->out_frames
           [self->frames_written * self->num_channels],
        frames_written_now * self->num_channels, 0.00000001f);
    }

  self->frames_written += frames_written_now;

#if 0
  g_debug (
    "resampler processed: frames written now %zu, total frames written %zu, expected total frames %zu",
    frames_written_now, self->frames_written,
    self->num_out_frames);
#endif

  if (frames_written_now == 0)
    {
      /* in case the calculation for out frames is off by 1,
       * assume finished */
      g_message (
        "no more frames to write at %zu frames (expected %zu frames)",
        self->frames_written, self->num_out_frames);
      self->num_out_frames = self->frames_written;
    }

  return true;
}

/**
 * Indicates whether resampling is finished.
 */
bool
resampler_is_done (Resampler * self)
{
  return self->frames_written == self->num_out_frames;
}

void
resampler_free (Resampler * self)
{
  object_free_w_func_and_null_cast (
    soxr_delete, soxr_t, self->priv);

  if (self->out_frames)
    {
      free (self->out_frames);
    }

  object_zero_and_free (self);
}
