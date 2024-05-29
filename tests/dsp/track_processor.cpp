// SPDX-FileCopyrightText: © 2021-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include <cmath>

#include "dsp/master_track.h"
#include "dsp/track_processor.h"
#include "project.h"
#include "utils/flags.h"
#include "zrythm.h"

#include <glib.h>

#include "tests/helpers/project_helper.h"
#include "tests/helpers/zrythm_helper.h"

#include <locale.h>

/**
 * Run track_processor_process() on master and check
 * that the output buffer matches the input buffer.
 */
static void
test_process_master (void)
{
  test_helper_zrythm_init ();

  /* stop dummy audio engine processing so we can
   * process manually */
  AUDIO_ENGINE->stop_dummy_audio_thread = true;
  g_usleep (1000000);

  g_message ("testing...");

  for (nframes_t i = 0; i < AUDIO_ENGINE->block_length; i++)
    {
      P_MASTER_TRACK->processor->stereo_in->l->buf[i] = (float) (i + 1);
    }

  nframes_t             local_offset = 60;
  EngineProcessTimeInfo time_nfo = {
    .g_start_frame = 0,
    .g_start_frame_w_offset = 0,
    .local_offset = 0,
    .nframes = local_offset,
  };
  track_processor_process (P_MASTER_TRACK->processor, &time_nfo);
  time_nfo.g_start_frame = 0;
  time_nfo.g_start_frame_w_offset = local_offset;
  time_nfo.local_offset = local_offset;
  time_nfo.nframes = AUDIO_ENGINE->block_length - local_offset;
  track_processor_process (P_MASTER_TRACK->processor, &time_nfo);

  for (nframes_t i = 0; i < AUDIO_ENGINE->block_length; i++)
    {
      g_assert_cmpfloat_with_epsilon (
        P_MASTER_TRACK->processor->stereo_out->l->buf[i], (float) (i + 1),
        0.000001f);
    }

  test_helper_zrythm_cleanup ();
}

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

  /*yaml_set_log_level (CYAML_LOG_INFO);*/

#define TEST_PREFIX "/audio/tracklist/"

  g_test_add_func (
    TEST_PREFIX "test process master", (GTestFunc) test_process_master);

  return g_test_run ();
}
