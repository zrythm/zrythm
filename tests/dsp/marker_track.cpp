// SPDX-FileCopyrightText: © 2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "dsp/midi_region.h"
#include "dsp/transport.h"
#include "project.h"
#include "zrythm.h"

#include "tests/helpers/project_helper.h"
#include "tests/helpers/zrythm_helper.h"

TEST_SUITE_BEGIN ("dsp/marker track");

TEST_CASE_FIXTURE (ZrythmFixture, "add marker")
{
  auto track = P_MARKER_TRACK;
  REQUIRE_SIZE_EQ (track->markers_, 2);

  for (int i = 0; i < 15; i++)
    {
      test_project_save_and_reload ();

      track = P_MARKER_TRACK;
      auto     marker = std::make_shared<Marker> ("start");
      Position pos;
      pos.set_to_bar (1);
      marker->pos_setter (&pos);
      marker->marker_type_ = Marker::Type::Start;

      track->add_marker (marker);

      REQUIRE_EQ (marker.get (), track->markers_[i + 2].get ());

      for (int j = 0; j < i + 2; j++)
        {
          REQUIRE_NONNULL (track->markers_[j]);
        }
    }
}

TEST_SUITE_END;