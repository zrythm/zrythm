// SPDX-FileCopyrightText: © 2020-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include <cmath>

#include "dsp/audio_track.h"
#include "dsp/engine_dummy.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/math.h"
#include "utils/objects.h"
#include "zrythm.h"

#include <glib.h>

#include "tests/helpers/project_helper.h"
#include "tests/helpers/zrythm_helper.h"

#include <locale.h>

#define BUFFER_SIZE 20
#define LARGE_BUFFER_SIZE 2000

#define LOOP_BAR 4

static void
test_fill_when_region_starts_on_loop_end (void)
{
  test_helper_zrythm_init ();

  test_project_stop_dummy_engine ();

  TRANSPORT->play_state = PlayState::PLAYSTATE_ROLLING;

  /* prepare loop */
  transport_set_loop (TRANSPORT, true, true);
  position_set_to_bar (&TRANSPORT->loop_end_pos, LOOP_BAR);
  position_set_to_bar (&TRANSPORT->loop_start_pos, LOOP_BAR);
  position_add_frames (&TRANSPORT->loop_start_pos, -31);

  /* create audio track with region */
  char * filepath =
    g_build_filename (TESTS_SRCDIR, "test_start_with_signal.mp3", NULL);
  SupportedFile * file = supported_file_new_from_path (filepath);
  int             num_tracks_before = TRACKLIST->num_tracks;

  transport_request_pause (TRANSPORT, true);
  track_create_with_action (
    TrackType::TRACK_TYPE_AUDIO, NULL, file, &TRANSPORT->loop_end_pos,
    num_tracks_before, 1, -1, NULL, NULL);
  Track * track = tracklist_get_track (TRACKLIST, num_tracks_before);
  /*transport_request_roll (TRANSPORT);*/
  TRANSPORT->play_state = PlayState::PLAYSTATE_ROLLING;

  StereoPorts * ports = new StereoPorts (
    false, "ports", "ports", PortIdentifier::OwnerType::TRACK, track);
  ports->allocate_bufs ();

  /* run until loop end and make sure sample is
   * never played */
  int      nframes = 120;
  Position pos;
  position_set_to_bar (&pos, LOOP_BAR);
  position_add_frames (&pos, -nframes);
  EngineProcessTimeInfo time_nfo = {
    .g_start_frame = (unsigned_frame_t) pos.frames,
    .g_start_frame_w_offset = (unsigned_frame_t) pos.frames,
    .local_offset = 0,
    .nframes = (nframes_t) nframes,
  };
  track_fill_events (track, &time_nfo, NULL, ports);
  for (int j = 0; j < nframes; j++)
    {
      g_assert_cmpfloat_with_epsilon (ports->get_l ().buf_[j], 0.f, 0.0000001f);
      g_assert_cmpfloat_with_epsilon (ports->get_r ().buf_[j], 0.f, 0.0000001f);
    }

  /* run after loop end and make sure sample is
   * played */
  position_set_to_bar (&pos, LOOP_BAR);
  time_nfo.g_start_frame = (unsigned_frame_t) pos.frames;
  time_nfo.g_start_frame_w_offset = (unsigned_frame_t) pos.frames;
  time_nfo.local_offset = 0;
  time_nfo.nframes = (nframes_t) nframes;
  track_fill_events (track, &time_nfo, NULL, ports);
  for (int j = 0; j < nframes; j++)
    {
      /* take into account builtin fades */
      if (j == 0)
        {
          g_assert_true (math_floats_equal (ports->get_l ().buf_[j], 0.f));
          g_assert_true (math_floats_equal (ports->get_r ().buf_[j], 0.f));
        }
      else
        {
          g_assert_true (fabsf (ports->get_l ().buf_[j]) > 0.0000001f);
          g_assert_true (fabsf (ports->get_r ().buf_[j]) > 0.0000001f);
        }
    }

  object_delete_and_null (ports);

  test_helper_zrythm_cleanup ();
}

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/audio/audio_track/"

  g_test_add_func (
    TEST_PREFIX "test fill when region starts on loop end",
    (GTestFunc) test_fill_when_region_starts_on_loop_end);

  return g_test_run ();
}
