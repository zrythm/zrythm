// SPDX-FileCopyrightText: © 2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "actions/tracklist_selections.h"
#include "dsp/midi_region.h"
#include "dsp/region.h"
#include "dsp/transport.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/io.h"
#include "zrythm.h"

#include "tests/helpers/project.h"
#include "tests/helpers/zrythm.h"

static void
test_add_marker (void)
{
  test_helper_zrythm_init ();

  Track * track = P_MARKER_TRACK;
  g_assert_cmpint (track->num_markers, ==, 2);

  for (int i = 0; i < 15; i++)
    {
      test_project_save_and_reload ();

      track = P_MARKER_TRACK;
      Marker *         marker = marker_new ("start");
      ArrangerObject * m_obj = (ArrangerObject *) marker;
      Position         pos;
      position_set_to_bar (&pos, 1);
      arranger_object_pos_setter (m_obj, &pos);
      marker->type = MARKER_TYPE_START;

      marker_track_add_marker (track, marker);

      g_assert_true (marker == track->markers[i + 2]);

      for (int j = 0; j < i + 2; j++)
        {
          g_assert_true (track->markers[j]);
        }
    }

  test_helper_zrythm_cleanup ();
}

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/audio/marker track/"

  g_test_add_func (TEST_PREFIX "test add marker", (GTestFunc) test_add_marker);

  return g_test_run ();
}
