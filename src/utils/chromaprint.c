/*
 * Copyright (C) 2020-2022 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "zrythm-config.h"

#ifdef HAVE_CHROMAPRINT

#  include "utils/audio.h"
#  include "utils/chromaprint.h"
#  include "utils/math.h"
#  include "utils/objects.h"

#  include <glib.h>

#  include <sndfile.h>

void
z_chromaprint_fingerprint_free (ChromaprintFingerprint * self)
{
  object_free_w_func_and_null (chromaprint_dealloc, self->fp);
  object_free_w_func_and_null (
    chromaprint_dealloc, self->compressed_str);
  object_zero_and_free (self);
}

ChromaprintFingerprint *
z_chromaprint_get_fingerprint (
  const char *     file1,
  unsigned_frame_t max_frames)
{
  int ret;

  SF_INFO sfinfo;
  memset (&sfinfo, 0, sizeof (sfinfo));
  sfinfo.format = sfinfo.format | SF_FORMAT_PCM_16;
  SNDFILE * sndfile = sf_open (file1, SFM_READ, &sfinfo);
  g_return_val_if_fail (sndfile, NULL);
  g_return_val_if_fail (sfinfo.frames > 0, NULL);

  ChromaprintContext * ctx =
    chromaprint_new (CHROMAPRINT_ALGORITHM_DEFAULT);
  g_return_val_if_fail (ctx, NULL);
  ret = chromaprint_start (
    ctx, sfinfo.samplerate, sfinfo.channels);
  g_return_val_if_fail (ret == 1, NULL);
  int     buf_size = sfinfo.frames * sfinfo.channels;
  short * data = calloc ((size_t) buf_size, sizeof (short));
  sf_count_t frames_read =
    sf_readf_short (sndfile, data, sfinfo.frames);
  g_return_val_if_fail (frames_read == sfinfo.frames, NULL);
  g_message ("read %ld frames for %s", frames_read, file1);

  ret = chromaprint_feed (ctx, data, buf_size);
  g_return_val_if_fail (ret == 1, NULL);

  ret = chromaprint_finish (ctx);
  g_return_val_if_fail (ret == 1, NULL);

  ChromaprintFingerprint * fp =
    object_new (ChromaprintFingerprint);
  ret = chromaprint_get_fingerprint (ctx, &fp->compressed_str);
  g_return_val_if_fail (ret == 1, NULL);
  ret =
    chromaprint_get_raw_fingerprint (ctx, &fp->fp, &fp->size);
  g_return_val_if_fail (ret == 1, NULL);

  g_message (
    "fingerprint %s [%d]", fp->compressed_str, fp->size);

  chromaprint_free (ctx);
  free (data);

  ret = sf_close (sndfile);
  g_return_val_if_fail (ret == 0, NULL);

  return fp;
}

/**
 * @param perc Minimum percentage of equal
 *   fingerprints required.
 */
void
z_chromaprint_check_fingerprint_similarity (
  const char * file1,
  const char * file2,
  int          perc,
  int          expected_size)
{
  const unsigned_frame_t max_frames = MIN (
    audio_get_num_frames (file1),
    audio_get_num_frames (file2));
  ChromaprintFingerprint * fp1 =
    z_chromaprint_get_fingerprint (file1, max_frames);
  g_return_if_fail (fp1);
  g_return_if_fail (fp1->size == expected_size);
  ChromaprintFingerprint * fp2 =
    z_chromaprint_get_fingerprint (file2, max_frames);
  g_return_if_fail (fp2);

  int min = MIN (fp1->size, fp2->size);
  g_return_if_fail (min != 0);
  int rate = 0;
  for (int i = 0; i < min; i++)
    {
      if (fp1->fp[i] == fp2->fp[i])
        rate++;
    }

  double rated = (double) rate / (double) min;
  int    rate_perc =
    (int) math_round_double_to_signed_32 (rated * 100.0);
  g_message ("%d out of %d (%d%%)", rate, min, rate_perc);

  g_return_if_fail (rate_perc >= perc);

  z_chromaprint_fingerprint_free (fp1);
  z_chromaprint_fingerprint_free (fp2);
}

#endif /* HAVE_CHROMAPRINT */
