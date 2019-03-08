/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */


/**
 * \file
 *
 * Audio utils.
 */

#include <math.h>

#include "audio/engine.h"
#include "ext/audio_decoder/ad.h"
#include "project.h"
#include "utils/audio.h"

#include <sndfile.h>
#include <samplerate.h>

/**
 * Decodes the given filename (absolute path).
 */
void
audio_decode (
  struct adinfo * nfo,
  SRC_DATA *      src_data,
  float **        out_buff,
  long *          out_buff_size,
  const char *    filename)
{
  void * handle =
    ad_open (filename,
             nfo);
  long in_buff_size = nfo->frames * nfo->channels;

  /* add some extra buffer for some reason */
  float * in_buff =
    malloc (in_buff_size * sizeof (float));
  long samples_read =
    ad_read (handle,
             in_buff,
             in_buff_size);
  g_message ("in buff size: %ld, %ld samples read",
             in_buff_size,
             samples_read);

  /* resample with libsamplerate */
  double src_ratio =
    (1.0 * AUDIO_ENGINE->sample_rate) /
    nfo->sample_rate ;
  if (fabs (src_ratio - 1.0) < 1e-20)
    {
      g_message ("Target samplerate and input "
                 "samplerate are the same.");
    }
  if (src_is_valid_ratio (src_ratio) == 0)
    {
      g_warning ("Sample rate change out of valid "
                 "range.");
    }
  *out_buff_size =
    in_buff_size * src_ratio;
  /* add some extra buffer for some reason */
  *out_buff =
    malloc (*out_buff_size * sizeof (float));
  g_message ("out_buff_size %ld, sizeof float %ld, "
             "sizeof long %ld, src ratio %f",
             *out_buff_size,
             sizeof (float),
             sizeof (long),
             src_ratio);
  src_data->data_in = &in_buff[0];
  src_data->data_out = *out_buff;
  src_data->input_frames = nfo->frames;
  src_data->output_frames = nfo->frames * src_ratio;
  src_data->src_ratio = src_ratio;

  int err =
    src_simple (src_data,
                SRC_SINC_BEST_QUALITY,
                nfo->channels);
  g_message ("output frames gen %ld, out buff size %ld, "
             "input frames used %ld, err %d",
             src_data->output_frames_gen,
             *out_buff_size,
             src_data->input_frames_used,
             err);

  /* cleanup */
  ad_close (handle);
  free (in_buff);

  return;
}
