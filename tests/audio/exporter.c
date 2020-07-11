/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
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

#include "zrythm-test-config.h"

#include "actions/create_tracks_action.h"
#include "audio/encoder.h"
#include "audio/exporter.h"
#include "audio/supported_file.h"
#include "helpers/zrythm.h"
#include "project.h"
#include "utils/math.h"
#include "zrythm.h"

#include <glib.h>

#include <chromaprint.h>
#include <sndfile.h>

/**
 * Chroma fingerprint info for a specific file.
 */
typedef struct ChromaprintFingerprint
{
  uint32_t * fp;
  int        size;
  char *     compressed_str;
} ChromaprintFingerprint;

static void
chromaprint_fingerprint_free (
  ChromaprintFingerprint * self)
{
  chromaprint_dealloc (self->fp);
  chromaprint_dealloc (self->compressed_str);

}

static long
get_num_frames (
  const char * file)
{
  SF_INFO sfinfo;
  memset (&sfinfo, 0, sizeof (sfinfo));
  sfinfo.format =
    sfinfo.format | SF_FORMAT_PCM_16;
  SNDFILE * sndfile =
    sf_open (file, SFM_READ, &sfinfo);
  g_assert_nonnull (sndfile);
  g_assert_cmpint (sfinfo.frames, >, 0);
  long frames = sfinfo.frames;

  int ret = sf_close (sndfile);
  g_assert_cmpint (ret, ==, 0);

  return frames;
}

static ChromaprintFingerprint *
get_fingerprint (
  const char * file1,
  long         max_frames)
{
  int ret;

  SF_INFO sfinfo;
  memset (&sfinfo, 0, sizeof (sfinfo));
  sfinfo.format =
    sfinfo.format | SF_FORMAT_PCM_16;
  SNDFILE * sndfile =
    sf_open (file1, SFM_READ, &sfinfo);
  g_assert_nonnull (sndfile);
  g_assert_cmpint (sfinfo.frames, >, 0);

  ChromaprintContext * ctx =
    chromaprint_new (CHROMAPRINT_ALGORITHM_DEFAULT);
  g_assert_nonnull (ctx);
  ret = chromaprint_start (
    ctx, sfinfo.samplerate, sfinfo.channels);
  g_assert_cmpint (ret, ==, 1);
  int buf_size = sfinfo.frames * sfinfo.channels;
  short data[buf_size];
  sf_count_t frames_read =
    sf_readf_short (sndfile, data, sfinfo.frames);
  g_assert_cmpint (frames_read, ==, sfinfo.frames);
  g_message (
    "read %ld frames for %s", frames_read, file1);

  ret = chromaprint_feed (ctx, data, buf_size);
  g_assert_cmpint (ret, ==, 1);

  ret = chromaprint_finish (ctx);
  g_assert_cmpint (ret, ==, 1);

  ChromaprintFingerprint * fp =
    calloc (1, sizeof (ChromaprintFingerprint));
  ret = chromaprint_get_fingerprint (
    ctx, &fp->compressed_str);
  g_assert_cmpint (ret, ==, 1);
  ret = chromaprint_get_raw_fingerprint (
    ctx, &fp->fp, &fp->size);
  g_assert_cmpint (ret, ==, 1);

  g_message (
    "fingerprint %s [%d]",
    fp->compressed_str, fp->size);

  chromaprint_free (ctx);

  ret = sf_close (sndfile);
  g_assert_cmpint (ret, ==, 0);

  return fp;
}

/**
 * @param perc Minimum percentage of equal
 *   fingerprints required.
 */
static void
check_fingerprint_similarity (
  const char * file1,
  const char * file2,
  int          perc)
{
  const long max_frames =
    MIN (
      get_num_frames (file1),
      get_num_frames (file2));
  ChromaprintFingerprint * fp1 =
    get_fingerprint (file1, max_frames);
  g_assert_cmpint (fp1->size, ==, 6);
  ChromaprintFingerprint * fp2 =
    get_fingerprint (file2, max_frames);

  int min = MIN (fp1->size, fp2->size);
  g_assert_cmpint (min, !=, 0);
  int rate = 0;
  for (int i = 0; i < min; i++)
    {
      if (fp1->fp[i] == fp2->fp[i])
        rate++;
    }

  double rated = (double) rate / (double) min;
  int rate_perc =
    math_round_double_to_int (rated * 100.0);
  g_message (
    "%d out of %d (%d%%)", rate, min, rate_perc);

  g_assert_cmpint (rate_perc, >=, perc);

  chromaprint_fingerprint_free (fp1);
  chromaprint_fingerprint_free (fp2);
}

static void
test_export_wav ()
{
  int ret;

  char * filepath =
    g_build_filename (
      TESTS_SRCDIR, "test.wav", NULL);
  SupportedFile * file =
    supported_file_new_from_path (filepath);
  UndoableAction * action =
    create_tracks_action_new (
      TRACK_TYPE_AUDIO, NULL, file,
      TRACKLIST->num_tracks, PLAYHEAD, 1);
  undo_manager_perform (UNDO_MANAGER, action);

  char * tmp_dir =
    g_dir_make_tmp ("test_wav_prj_XXXXXX", NULL);
  ret =
    project_save (
      PROJECT, tmp_dir, 0, 0, F_NO_ASYNC);
  g_free (tmp_dir);
  g_assert_cmpint (ret, ==, 0);

  ExportSettings settings;
  settings.format = AUDIO_FORMAT_WAV;
  settings.artist = g_strdup ("Test Artist");
  settings.genre = g_strdup ("Test Genre");
  settings.depth = BIT_DEPTH_16;
  settings.mode = EXPORT_MODE_FULL;
  settings.time_range = TIME_RANGE_LOOP;
  char * exports_dir =
    project_get_path (
      PROJECT, PROJECT_PATH_EXPORTS, false);
  settings.file_uri =
    g_build_filename (
      exports_dir, "test_wav.wav", NULL);
  ret = exporter_export (&settings);
  g_assert_cmpint (ret, ==, 0);

  check_fingerprint_similarity (
    filepath, settings.file_uri, 100);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  test_helper_zrythm_init ();

#define TEST_PREFIX "/audio/exporter/"

  g_test_add_func (
    TEST_PREFIX "test export wav",
    (GTestFunc) test_export_wav);

  return g_test_run ();
}

