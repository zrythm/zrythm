// SPDX-FileCopyrightText: © 2021-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "gui/backend/backend/arranger_selections.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"

#include "tests/helpers/zrythm_helper.h"

#include "gui/dsp/midi_region.h"
#include "gui/dsp/tracklist.h"

static void
test_region_length_in_ticks (Track * track, int bar_start, int bar_end)
{
  Position p1, p2;
  p1.set_to_bar (bar_start);
  p2.set_to_bar (bar_end);
  auto r = std::make_shared<MidiRegion> (p1, p2, track->get_name_hash (), 0, 0);
  track->add_region (r, nullptr, 0, true, false);

  r->select (true, false, false);

  double length = TL_SELECTIONS->get_length_in_ticks ();
  ASSERT_NEAR (
    length, TRANSPORT->ticks_per_bar_ * (bar_end - bar_start), 0.00001);
}

TEST_F (ZrythmFixture, GetLengthInTicks)
{
  auto track = Track::create_empty_with_action<MidiTrack> ();

  test_region_length_in_ticks (track, 3, 4);
  test_region_length_in_ticks (track, 100, 102);
  test_region_length_in_ticks (track, 1000, 1010);
}

TEST_F (ZrythmFixture, GetLastObject)
{
  auto track = Track::create_empty_with_action<MidiTrack> ();

  Position p1, p2;
  p1.set_to_bar (3);
  p2.set_to_bar (4);
  auto r = std::make_shared<MidiRegion> (p1, p2, track->get_name_hash (), 0, 0);
  track->add_region (r, nullptr, 0, true, false);

  p1.from_frames (-40000);
  p2.from_frames (-4000);
  auto mn = std::make_shared<MidiNote> (r->id_, p1, p2, 60, 60);
  r->append_object (mn);

  mn->select (true, false, false);

  auto [last_obj, last_pos] =
    MIDI_SELECTIONS->get_last_object_and_pos (false, true);
  ASSERT_NONNULL (last_obj);
  ASSERT_TRUE (last_obj == mn.get ());
}

TEST_F (ZrythmFixture, ContainsObjectWithProperty)
{
  auto track = Track::create_empty_with_action<MidiTrack> ();

  Position p1, p2;
  p1.set_to_bar (3);
  p2.set_to_bar (4);
  auto r = std::make_shared<MidiRegion> (p1, p2, track->get_name_hash (), 0, 0);
  track->add_region (r, nullptr, 0, true, false);

  p1.from_frames (-40000);
  p2.from_frames (-4000);
  auto mn = std::make_shared<MidiNote> (r->id_, p1, p2, 60, 60);
  r->append_object (mn);

  mn->select (true, false, false);

  ASSERT_TRUE (MIDI_SELECTIONS->contains_object_with_property (
    ArrangerSelections::Property::HasLength, true));
  ASSERT_FALSE (MIDI_SELECTIONS->contains_object_with_property (
    ArrangerSelections::Property::HasLength, false));
}
