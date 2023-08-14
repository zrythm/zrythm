// SPDX-FileCopyrightText: © 2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "actions/channel_send_action.h"
#include "actions/undoable_action.h"
#include "dsp/audio_region.h"
#include "dsp/automation_region.h"
#include "dsp/chord_region.h"
#include "dsp/control_port.h"
#include "dsp/foldable_track.h"
#include "dsp/graph_export.h"
#include "dsp/master_track.h"
#include "dsp/midi_event.h"
#include "dsp/midi_note.h"
#include "dsp/region.h"
#include "dsp/router.h"
#include "project.h"
#include "utils/color.h"
#include "utils/flags.h"
#include "utils/math.h"
#include "zrythm.h"

#include <glib.h>

#include "tests/helpers/plugin_manager.h"
#include "tests/helpers/project.h"

static void
test_route_master_send_to_fx (void)
{
  test_helper_zrythm_init ();

  /* create audio fx track */
  Track * audio_fx = track_create_empty_with_action (
    TRACK_TYPE_AUDIO_BUS, NULL);

  /* expect messages */
  LOG->use_structured_for_console = false;
  LOG->min_log_level_for_test_console = G_LOG_LEVEL_WARNING;
  g_test_expect_message (
    G_LOG_DOMAIN, G_LOG_LEVEL_WARNING, "*FAILED*");
  g_test_expect_message (
    G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
    "*action not performed*");

  /* route master to it */
  GError * err = NULL;
  bool     ret = channel_send_action_perform_connect_audio (
    P_MASTER_TRACK->channel->sends[0],
    audio_fx->processor->stereo_in, &err);

  /* let engine run for a few cycles */
  engine_wait_n_cycles (AUDIO_ENGINE, 3);

  g_assert_false (ret);

  /* assert expected messages */
  g_test_assert_expected_messages ();

  test_helper_zrythm_cleanup ();
}

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/actions/channel_send/"

  g_test_add_func (
    TEST_PREFIX "test route master send to fx",
    (GTestFunc) test_route_master_send_to_fx);

  return g_test_run ();
}
