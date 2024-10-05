// SPDX-FileCopyrightText: © 2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "gui/cpp/backend/project.h"
#include "gui/cpp/backend/zrythm.h"

#include "tests/helpers/project_helper.h"
#include "tests/helpers/zrythm_helper.h"

#include "common/dsp/midi_region.h"
#include "common/dsp/transport.h"

TEST_F (ZrythmFixture, AddMarker)
{
  auto track = P_MARKER_TRACK;
  ASSERT_SIZE_EQ (track->markers_, 2);

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

      ASSERT_EQ (marker.get (), track->markers_[i + 2].get ());

      for (int j = 0; j < i + 2; j++)
        {
          ASSERT_NONNULL (track->markers_[j]);
        }
    }
}
