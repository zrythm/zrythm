// SPDX-FileCopyrightText: © 2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "engine/session/transport.h"
#include "gui/backend/backend/actions/tracklist_selections_action.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "structure/arrangement/midi_region.h"
#include "structure/arrangement/region.h"
#include "utils/io.h"

#include "tests/helpers/project_helper.h"
#include "tests/helpers/zrythm_helper.h"

TEST_F (BootstrapTimelineFixture, GetChordAtPos)
{
  Position p1, p2, loop;

  p1.set_to_bar (12);
  p2.set_to_bar (24);
  loop.set_to_bar (5);

  auto &r = P_CHORD_TRACK->regions_[0];
  auto &co1 = r->chord_objects_[0];
  auto &co2 = r->chord_objects_[1];
  co1->chord_index_ = 0;
  co2->chord_index_ = 2;
  r->set_end_pos_full_size (&p2);
  r->set_start_pos_full_size (&p1);
  r->loop_end_position_setter_validated (&loop);
  r->print ();

  Position pos;

  auto assert_chord_at_pos_null = [&] (const Position &check_pos) {
    auto chord = P_CHORD_TRACK->get_chord_at_pos (check_pos);
    ASSERT_NULL (chord);
  };

  auto assert_chord_at_pos_eq =
    [&] (const Position &check_pos, const auto &expected_chord) {
      auto chord = P_CHORD_TRACK->get_chord_at_pos (check_pos);
      ASSERT_EQ (chord, expected_chord.get ());
    };

  pos.zero ();
  assert_chord_at_pos_null (pos);

  pos.set_to_bar (2);
  assert_chord_at_pos_null (pos);

  pos.set_to_bar (3);
  assert_chord_at_pos_null (pos);

  pos.set_to_bar (12);
  assert_chord_at_pos_null (pos);

  pos.set_to_bar (13);
  assert_chord_at_pos_eq (pos, co1);

  pos.set_to_bar (14);
  assert_chord_at_pos_eq (pos, co1);

  pos.set_to_bar (15);
  assert_chord_at_pos_eq (pos, co2);

  pos.set_to_bar (16);
  assert_chord_at_pos_null (pos);

  pos.set_to_bar (17);
  assert_chord_at_pos_eq (pos, co1);

  pos.set_to_bar (18);
  assert_chord_at_pos_eq (pos, co1);

  pos.set_to_bar (19);
  assert_chord_at_pos_eq (pos, co2);

  pos.set_to_bar (100);
  assert_chord_at_pos_null (pos);
}
