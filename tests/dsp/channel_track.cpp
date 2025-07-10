// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "structure/tracks/channel_track.h"

#include "tests/helpers/project_helper.h"

TEST_F (ZrythmFixture, CloneChannelTrack)
{
  auto track = Track::create_track (
    Track::Type::Midi, "Test MIDI Track", TRACKLIST->get_last_pos ());
  auto ch_track = dynamic_cast<MidiTrack *> (track.get ());

  auto expect_ports_have_orig_track_name_hash =
    [&ch_track] (const auto &cur_track) {
      constexpr size_t         expected_num_ports = 2191;
      std::vector<dsp::Port *> ports;
      cur_track->append_ports (ports, false);
      ASSERT_SIZE_EQ (ports, expected_num_ports);
      for (auto &port : ports)
        {
          ASSERT_EQ (port->id_.track_name_hash_, ch_track->get_name_hash ());
        }
    };

  // check for original track
  expect_ports_have_orig_track_name_hash (ch_track);

  auto cloned_track = ch_track->clone_unique ();

  // check for cloned track
  expect_ports_have_orig_track_name_hash (cloned_track);
}
