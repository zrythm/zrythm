/*
 * Copyright (C) 2019-2022 Alexandros Theodotou <alex at zrythm dot org>
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

#include "actions/tracklist_selections.h"
#include "audio/midi_region.h"
#include "audio/region.h"
#include "audio/transport.h"
#include "project.h"
#include "utils/flags.h"
#include "zrythm.h"

#include "tests/helpers/zrythm.h"

typedef struct
{
  ZRegion * midi_region;
} RegionFixture;

static void
fixture_set_up (RegionFixture * fixture)
{
  /* needed to set TRANSPORT */
  engine_update_frames_per_tick (
    AUDIO_ENGINE, 4, 140, 44000, true, true, false);

  Position start_pos, end_pos;
  position_set_to_bar (&start_pos, 2);
  position_set_to_bar (&end_pos, 4);
  fixture->midi_region =
    midi_region_new (&start_pos, &end_pos, 0, 0, 0);
}

static void
test_region_is_hit_by_range (void)
{
  Position pos, end_pos;
  position_set_to_bar (&pos, 4);
  position_set_to_bar (&end_pos, 5);
  Position other_start_pos;
  position_set_to_bar (&other_start_pos, 3);
  ZRegion * region =
    midi_region_new (&pos, &end_pos, 0, -1, -1);
  g_assert_true (region_is_hit_by_range (
    region, other_start_pos.frames, pos.frames,
    false));
}

static void
test_region_is_hit (void)
{
  RegionFixture   _fixture;
  RegionFixture * fixture = &_fixture;
  fixture_set_up (fixture);

  ZRegion *        r = fixture->midi_region;
  ArrangerObject * r_obj = (ArrangerObject *) r;

  Position pos;
  int      ret;

  /*
   * Position: ZRegion start
   *
   * Expected result:
   * Returns true either exclusive or inclusive.
   */
  position_set_to_pos (&pos, &r_obj->pos);
  position_update_frames_from_ticks (&pos);
  ret = region_is_hit (r, pos.frames, 0);
  g_assert_cmpint (ret, ==, 1);
  ret = region_is_hit (r, pos.frames, 1);
  g_assert_cmpint (ret, ==, 1);

  /*
   * Position: ZRegion start - 1 frame
   *
   * Expected result:
   * Returns false either exclusive or inclusive.
   */
  position_set_to_pos (&pos, &r_obj->pos);
  position_update_frames_from_ticks (&pos);
  position_add_frames (&pos, -1);
  ret = region_is_hit (r, pos.frames, 0);
  g_assert_cmpint (ret, ==, 0);
  ret = region_is_hit (r, pos.frames, 1);
  g_assert_cmpint (ret, ==, 0);

  /*
   * Position: ZRegion end
   *
   * Expected result:
   * Returns true for inclusive, false for not.
   */
  position_set_to_pos (&pos, &r_obj->end_pos);
  position_update_frames_from_ticks (&pos);
  ret = region_is_hit (r, pos.frames, 0);
  g_assert_cmpint (ret, ==, 0);
  ret = region_is_hit (r, pos.frames, 1);
  g_assert_cmpint (ret, ==, 1);

  /*
   * Position: ZRegion end - 1
   *
   * Expected result:
   * Returns true for both.
   */
  position_set_to_pos (&pos, &r_obj->end_pos);
  position_update_frames_from_ticks (&pos);
  position_add_frames (&pos, -1);
  ret = region_is_hit (r, pos.frames, 0);
  g_assert_cmpint (ret, ==, 1);
  ret = region_is_hit (r, pos.frames, 1);
  g_assert_cmpint (ret, ==, 1);

  /*
   * Position: ZRegion end + 1
   *
   * Expected result:
   * Returns false for both.
   */
  position_set_to_pos (&pos, &r_obj->end_pos);
  position_update_frames_from_ticks (&pos);
  position_add_frames (&pos, 1);
  ret = region_is_hit (r, pos.frames, 0);
  g_assert_cmpint (ret, ==, 0);
  ret = region_is_hit (r, pos.frames, 1);
  g_assert_cmpint (ret, ==, 0);
}

static void
test_new_region (void)
{
  RegionFixture   _fixture;
  RegionFixture * fixture = &_fixture;
  fixture_set_up (fixture);

  Position start_pos, end_pos, tmp;
  position_set_to_bar (&start_pos, 2);
  position_set_to_bar (&end_pos, 4);
  ZRegion * region =
    midi_region_new (&start_pos, &end_pos, 0, 0, 0);
  ArrangerObject * r_obj = (ArrangerObject *) region;

  g_assert_nonnull (region);
  g_assert_true (
    region->id.type == REGION_TYPE_MIDI);
  g_assert_true (
    position_is_equal (&start_pos, &r_obj->pos));
  g_assert_true (
    position_is_equal (&end_pos, &r_obj->end_pos));
  position_init (&tmp);
  g_assert_true (position_is_equal (
    &tmp, &r_obj->clip_start_pos));

  g_assert_false (r_obj->muted);
  g_assert_cmpint (region->num_midi_notes, ==, 0);

  position_set_to_pos (&tmp, &r_obj->pos);
  position_add_ticks (&tmp, 12);
  if (arranger_object_validate_pos (
        r_obj, &tmp,
        ARRANGER_OBJECT_POSITION_TYPE_START))
    {
      arranger_object_set_position (
        r_obj, &tmp,
        ARRANGER_OBJECT_POSITION_TYPE_START,
        F_NO_VALIDATE);
    }
}

static void
test_timeline_frames_to_local (void)
{
  track_create_empty_with_action (
    TRACK_TYPE_MIDI, NULL);

  Track * track =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1];

  Position pos, end_pos;
  position_init (&pos);
  position_set_to_bar (&end_pos, 4);
  ZRegion * region = midi_region_new (
    &pos, &end_pos, track_get_name_hash (track), 0,
    0);
  signed_frame_t localp =
    region_timeline_frames_to_local (
      region, 13000, true);
  g_assert_cmpint (localp, ==, 13000);
  localp = region_timeline_frames_to_local (
    region, 13000, false);
  g_assert_cmpint (localp, ==, 13000);
}

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

  test_helper_zrythm_init ();

#define TEST_PREFIX "/audio/region/"

  g_test_add_func (
    TEST_PREFIX "test new region",
    (GTestFunc) test_new_region);
  g_test_add_func (
    TEST_PREFIX "test region is hit",
    (GTestFunc) test_region_is_hit);
  g_test_add_func (
    TEST_PREFIX "test region is hit by range",
    (GTestFunc) test_region_is_hit_by_range);
  g_test_add_func (
    TEST_PREFIX "test_timeline_frames_to_local",
    (GTestFunc) test_timeline_frames_to_local);

  return g_test_run ();
}
