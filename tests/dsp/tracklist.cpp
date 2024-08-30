// SPDX-FileCopyrightText: © 2020-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "actions/arranger_selections.h"
#include "actions/tracklist_selections.h"
#include "dsp/automation_region.h"
#include "dsp/tracklist.h"
#include "project.h"
#include "utils/flags.h"
#include "zrythm.h"

#include "tests/helpers/project_helper.h"
#include "tests/helpers/zrythm_helper.h"

TEST_SUITE_BEGIN ("dsp/tracklist");

TEST_CASE_FIXTURE (ZrythmFixture, "handle drop empty midi file")
{
  FileDescriptor file (fs::path (TESTS_SRCDIR) / "empty_midi_file_type1.mid");

  REQUIRE_THROWS (TRACKLIST->import_files (
    nullptr, &file, nullptr, nullptr, -1, &PLAYHEAD, nullptr));
}

TEST_CASE_FIXTURE (ZrythmFixture, "swap with automation regions")
{
  Track::create_with_action (
    Track::Type::Audio, nullptr, nullptr, &PLAYHEAD,
    TRACKLIST->get_num_tracks (), 1, -1, nullptr);

  auto create_automation_region = [] (int track_pos) {
    auto * track = TRACKLIST->get_track<AutomatableTrack> (track_pos);

    Position start;
    start.set_to_bar (1);
    Position end;
    end.set_to_bar (3);
    auto region = std::make_shared<AutomationRegion> (
      start, end, track->get_name_hash (), 0, 0);
    auto &atl = track->get_automation_tracklist ();
    track->add_region (region, atl.ats_[0].get (), 0, true, false);
    region->select (true, false, false);
    UNDO_MANAGER->perform (
      std::make_unique<CreateArrangerSelectionsAction> (*TL_SELECTIONS));

    auto ap = std::make_shared<AutomationPoint> (0.1f, 0.1f, start);
    region->append_object (ap);
    ap->select (true, false, false);
    UNDO_MANAGER->perform (std::make_unique<CreateArrangerSelectionsAction> (
      *AUTOMATION_SELECTIONS));
  };

  create_automation_region (TRACKLIST->get_last_pos ());

  Track::create_empty_with_action<MidiTrack> ();

  create_automation_region (TRACKLIST->get_last_pos ());

  /* swap tracks */
  auto * track1 = TRACKLIST->get_track (TRACKLIST->get_num_tracks () - 2);
  auto * track2 = TRACKLIST->get_track (TRACKLIST->get_num_tracks () - 1);
  track2->select (true, true, false);
  REQUIRE (TRACKLIST->validate ());
  UNDO_MANAGER->perform (std::make_unique<MoveTracksAction> (
    *TRACKLIST_SELECTIONS->gen_tracklist_selections (), track1->pos_));
  REQUIRE (TRACKLIST->validate ());

  for (int i = 0; i < 7; ++i)
    {
      UNDO_MANAGER->undo ();
      REQUIRE (TRACKLIST->validate ());
    }

  test_project_save_and_reload ();
  REQUIRE (TRACKLIST->validate ());

  for (int i = 0; i < 7; ++i)
    {
      UNDO_MANAGER->redo ();
      REQUIRE (TRACKLIST->validate ());
    }
}

TEST_SUITE_END;