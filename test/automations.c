#include "audio/channel.h"
#include "audio/automatable.h"
#include "audio/automation_track.h"
#include "project.h"
#include "utils/arrays.h"
#include "zrythm.h"

#include "test/automations.h"

#include <glib.h>

void
at_fixture_set_up (
  AutomationTrackFixture * fixture,
  gconstpointer user_data)
{
  AutomationPoint * ap;
  AutomationCurve * ac;

  /* needed to set TRANSPORT */
  transport_init (TRANSPORT, 0);
  engine_update_frames_per_tick (
    4, 140, 44000);

  fixture->at =
    calloc (1, sizeof (AutomationTrack));
  ap =
    calloc (1, sizeof (AutomationPoint));
  position_set_to_bar (&ap->pos, 2);
  array_append (fixture->at->aps,
                fixture->at->num_aps,
                ap);
  ac =
    calloc (1, sizeof (AutomationCurve));
  position_set_to_bar (&ap->pos, 3);
  array_append (fixture->at->acs,
                fixture->at->num_acs,
                ac);
  ap =
    calloc (1, sizeof (AutomationPoint));
  position_set_to_bar (&ap->pos, 4);
  array_append (fixture->at->aps,
                fixture->at->num_aps,
                ap);
  ac =
    calloc (1, sizeof (AutomationCurve));
  position_set_to_bar (&ap->pos, 5);
  array_append (fixture->at->acs,
                fixture->at->num_acs,
                ac);
  ap =
    calloc (1, sizeof (AutomationPoint));
  position_set_to_bar (&ap->pos, 6);
  array_append (fixture->at->aps,
                fixture->at->num_aps,
                ap);
  ac =
    calloc (1, sizeof (AutomationCurve));
  position_set_to_bar (&ap->pos, 7);
  array_append (fixture->at->acs,
                fixture->at->num_acs,
                ac);
  ap =
    calloc (1, sizeof (AutomationPoint));
  position_set_to_bar (&ap->pos, 8);
  array_append (fixture->at->aps,
                fixture->at->num_acs,
                ap);
}

void
at_fixture_tear_down (
  AutomationTrackFixture * fixture,
  gconstpointer user_data)
{
}

/**
 * Tests automation point and curve position-related
 * functions
 * (automation_track_get_ap_before_pos, etc.).
 */
void
test_at_get_x_relevant_to_pos (
  AutomationTrackFixture * fixture,
  gconstpointer user_data)
{
  Position pos;
  AutomationPoint * ap;

  /* test when pos is before first ap */
  position_set_to_bar (&pos, 1);
  ap =
    automation_track_get_ap_before_pos (
      fixture->at, &pos);
  g_assert (
    ap == NULL);

  /* test when pos is before 3rd ap */
  position_set_to_pos (
    &pos, &fixture->at->aps[1]->pos);
  position_add_ticks (
    &pos, 1);
  ap =
    automation_track_get_ap_before_pos (
      fixture->at, &pos);
  g_assert (
    ap == fixture->at->aps[1]);

  /* test when pos is after all aps */
  position_set_to_pos (
    &pos, &fixture->at->aps[
            fixture->at->num_aps - 1]->
              pos);
  position_add_ticks (
    &pos, 1);
  ap =
    automation_track_get_ap_before_pos (
      fixture->at, &pos);
  g_assert (
    ap == fixture->at->aps[
            fixture->at->num_aps - 1]);

  /* test when there are no APs */
  /*int tmp = fixture->at->num_aps;*/
  fixture->at->num_aps = 0;
  ap =
    automation_track_get_ap_before_pos (
      fixture->at, &pos);
  g_assert (
    ap == NULL);
}

