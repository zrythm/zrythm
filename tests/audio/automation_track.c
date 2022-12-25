// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "actions/arranger_selections.h"
#include "audio/automation_region.h"
#include "audio/automation_track.h"
#include "audio/channel.h"
#include "audio/master_track.h"
#include "project.h"
#include "utils/arrays.h"
#include "zrythm.h"

#include <glib.h>

#include "tests/helpers/zrythm.h"

#include <locale.h>

static void
test_set_at_index (void)
{
  test_helper_zrythm_init ();

  Track * master = P_MASTER_TRACK;
  track_set_automation_visible (master, true);
  AutomationTracklist * atl =
    track_get_automation_tracklist (master);
  AutomationTrack ** visible_ats =
    calloc (100000, sizeof (AutomationTrack *));
  int num_visible = 0;
  automation_tracklist_get_visible_tracks (
    atl, visible_ats, &num_visible);
  AutomationTrack * first_vis_at = visible_ats[0];

  /* create a region and set it as clip editor
   * region */
  Position start, end;
  position_set_to_bar (&start, 2);
  position_set_to_bar (&end, 4);
  ZRegion * region = automation_region_new (
    &start, &end, track_get_name_hash (master),
    first_vis_at->index, 0);
  bool success = track_add_region (
    master, region, first_vis_at, -1, F_GEN_NAME,
    F_NO_PUBLISH_EVENTS, NULL);
  g_assert_true (success);
  arranger_object_select (
    (ArrangerObject *) region, F_SELECT, F_NO_APPEND,
    F_NO_PUBLISH_EVENTS);
  arranger_selections_action_perform_create (
    (ArrangerSelections *) TL_SELECTIONS, NULL);

  clip_editor_set_region (
    CLIP_EDITOR, region, F_NO_PUBLISH_EVENTS);

  AutomationTrack * first_invisible_at =
    automation_tracklist_get_first_invisible_at (atl);
  automation_tracklist_set_at_index (
    atl, first_invisible_at, first_vis_at->index,
    F_NO_PUSH_DOWN);

  /* check that clip editor region can be found */
  clip_editor_get_region (CLIP_EDITOR);

  test_helper_zrythm_cleanup ();
}

/**
 * There was a case where
 * arranger_object__get_muted() assumed
 * that the region was an audio/MIDI region and
 * threw errors.
 *
 * This replicates the issue and tests that this
 * does not happen.
 */
static void
test_region_in_2nd_automation_track_get_muted (void)
{
  test_helper_zrythm_init ();

  Track * master = P_MASTER_TRACK;
  track_set_automation_visible (master, true);
  AutomationTracklist * atl =
    track_get_automation_tracklist (master);
  AutomationTrack ** visible_ats =
    calloc (100000, sizeof (AutomationTrack *));
  int num_visible = 0;
  automation_tracklist_get_visible_tracks (
    atl, visible_ats, &num_visible);
  AutomationTrack * first_vis_at = visible_ats[0];

  /* create a new automation track */
  AutomationTrack * new_at =
    automation_tracklist_get_first_invisible_at (atl);
  if (!new_at->created)
    new_at->created = 1;
  new_at->visible = true;

  /* move it after the clicked
   * automation track */
  automation_tracklist_set_at_index (
    atl, new_at, first_vis_at->index + 1, true);

  /* create a region and set it as clip editor
   * region */
  Position start, end;
  position_set_to_bar (&start, 2);
  position_set_to_bar (&end, 4);
  ZRegion * region = automation_region_new (
    &start, &end, track_get_name_hash (master), new_at->index,
    0);
  bool success = track_add_region (
    master, region, new_at, -1, F_GEN_NAME,
    F_NO_PUBLISH_EVENTS, NULL);
  g_assert_true (success);
  arranger_object_select (
    (ArrangerObject *) region, F_SELECT, F_NO_APPEND,
    F_NO_PUBLISH_EVENTS);
  arranger_selections_action_perform_create (
    (ArrangerSelections *) TL_SELECTIONS, NULL);

  clip_editor_set_region (
    CLIP_EDITOR, region, F_NO_PUBLISH_EVENTS);

  engine_wait_n_cycles (AUDIO_ENGINE, 3);

  /* assert not muted */
  g_assert_false (arranger_object_get_muted (
    (ArrangerObject *) region, true));

  test_helper_zrythm_cleanup ();
}

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/audio/automation_track/"

  g_test_add_func (
    TEST_PREFIX "test set at index",
    (GTestFunc) test_set_at_index);
  g_test_add_func (
    TEST_PREFIX
    "test region in 2nd automation track get muted",
    (GTestFunc) test_region_in_2nd_automation_track_get_muted);

  return g_test_run ();
}
