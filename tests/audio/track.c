// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "audio/track.h"
#include "project.h"
#include "utils/flags.h"
#include "zrythm.h"

#include <glib.h>

#include "tests/helpers/project.h"

#include <locale.h>

static void
test_new_track (void)
{
  test_helper_zrythm_init ();

  Track * track = track_new (
    TRACK_TYPE_INSTRUMENT, TRACKLIST->num_tracks,
    "Test Instrument Track 1", F_WITH_LANE);
  g_assert_true (IS_TRACK_AND_NONNULL (track));

  g_assert_nonnull (track->name);

  object_free_w_func_and_null (track_free, track);

  test_helper_zrythm_cleanup ();
}

static void
test_get_direct_folder_parent (void)
{
  test_helper_zrythm_init ();

  Track * audio_group =
    track_create_empty_with_action (
      TRACK_TYPE_AUDIO_GROUP, NULL);
  g_assert_nonnull (audio_group);

  Track * audio_group2 =
    track_create_empty_with_action (
      TRACK_TYPE_AUDIO_GROUP, NULL);
  g_assert_nonnull (audio_group2);
  track_select (
    audio_group2, F_SELECT, F_NOT_EXCLUSIVE,
    F_NO_PUBLISH_EVENTS);
  tracklist_handle_move_or_copy (
    TRACKLIST, audio_group,
    TRACK_WIDGET_HIGHLIGHT_INSIDE, GDK_ACTION_MOVE);

  Track * audio_group3 =
    track_create_empty_with_action (
      TRACK_TYPE_AUDIO_GROUP, NULL);
  g_assert_nonnull (audio_group3);
  track_select (
    audio_group3, F_SELECT, F_NOT_EXCLUSIVE,
    F_NO_PUBLISH_EVENTS);
  tracklist_handle_move_or_copy (
    TRACKLIST, audio_group2,
    TRACK_WIDGET_HIGHLIGHT_INSIDE, GDK_ACTION_MOVE);

  g_assert_cmpint (audio_group->pos, ==, 5);
  g_assert_cmpint (audio_group->size, ==, 3);
  g_assert_cmpint (audio_group2->pos, ==, 6);
  g_assert_cmpint (audio_group2->size, ==, 2);
  g_assert_cmpint (audio_group3->pos, ==, 7);
  g_assert_cmpint (audio_group3->size, ==, 1);

  Track * direct_folder_parent =
    track_get_direct_folder_parent (audio_group3);
  g_assert_true (
    direct_folder_parent == audio_group2);

  test_helper_zrythm_cleanup ();
}

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/audio/track/"

  g_test_add_func (
    TEST_PREFIX "test new track",
    (GTestFunc) test_new_track);
  g_test_add_func (
    TEST_PREFIX "test get_direct folder parent",
    (GTestFunc) test_get_direct_folder_parent);

  return g_test_run ();
}
