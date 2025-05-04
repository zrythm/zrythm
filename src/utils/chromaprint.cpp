/*
 * SPDX-FileCopyrightText: © 2020-2022 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

#include "zrythm-config.h"

// TODO
#if HAVE_CHROMAPRINT && 0

#  include "utils/audio.h"
#  include "utils/chromaprint.h"
#  include "utils/math.h"
#  include "utils/objects.h"

#  include <sndfile.h>

std::unique_ptr<ChromaprintFingerprint>
z_chromaprint_get_fingerprint (const char * file1, unsigned_frame_t max_frames)
{
  int ret;

  SF_INFO sfinfo;
  memset (&sfinfo, 0, sizeof (sfinfo));
  sfinfo.format = sfinfo.format | SF_FORMAT_PCM_16;
  SNDFILE * sndfile = sf_open (file1, SFM_READ, &sfinfo);
  z_return_val_if_fail (sndfile, nullptr);
  z_return_val_if_fail (sfinfo.frames > 0, nullptr);

  ChromaprintContext * ctx = chromaprint_new (CHROMAPRINT_ALGORITHM_DEFAULT);
  z_return_val_if_fail (ctx, nullptr);
  ret = chromaprint_start (ctx, sfinfo.samplerate, sfinfo.channels);
  z_return_val_if_fail (ret == 1, nullptr);
  const auto buf_size = sfinfo.frames * sfinfo.channels;
  short *    data = object_new_n ((size_t) buf_size, short);
  sf_count_t frames_read = sf_readf_short (sndfile, data, sfinfo.frames);
  z_return_val_if_fail (frames_read == sfinfo.frames, nullptr);
  z_info ("read {} frames for {}", frames_read, file1);

  ret = chromaprint_feed (ctx, data, static_cast<int> (buf_size));
  z_return_val_if_fail (ret == 1, nullptr);

  ret = chromaprint_finish (ctx);
  z_return_val_if_fail (ret == 1, nullptr);

  auto fp = std::make_unique<ChromaprintFingerprint> ();
  ret = chromaprint_get_fingerprint (ctx, &fp->compressed_str);
  z_return_val_if_fail (ret == 1, nullptr);
  ret = chromaprint_get_raw_fingerprint (ctx, &fp->fp, &fp->size);
  z_return_val_if_fail (ret == 1, nullptr);

  z_info ("fingerprint {} [{}]", fp->compressed_str, fp->size);

  chromaprint_free (ctx);
  free (data);

  ret = sf_close (sndfile);
  z_return_val_if_fail (ret == 0, nullptr);

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
  const unsigned_frame_t max_frames = std::min (
    zrythm::utils::audio::get_num_frames (file1),
    zrythm::utils::audio::get_num_frames (file2));
  auto fp1 = z_chromaprint_get_fingerprint (file1, max_frames);
  z_return_if_fail (fp1);
  z_return_if_fail (fp1->size == expected_size);
  auto fp2 = z_chromaprint_get_fingerprint (file2, max_frames);
  z_return_if_fail (fp2);

  int min = std::min (fp1->size, fp2->size);
  z_return_if_fail (min != 0);
  int rate = 0;
  for (int i = 0; i < min; i++)
    {
      if (fp1->fp[i] == fp2->fp[i])
        rate++;
    }

  double rated = (double) rate / (double) min;
  int rate_perc = (int) zrythm::utils::math::round_to_signed_32 (rated * 100.0);
  z_info ("{} out of {} ({}%%)", rate, min, rate_perc);

  z_return_if_fail (rate_perc >= perc);
}

#endif /* HAVE_CHROMAPRINT */
