// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "dsp/channel_track.h"
#include "project.h"
#include "zrythm.h"

#include "tests/helpers/project_helper.h"

TEST_SUITE_BEGIN ("dsp/channel_track");

TEST_CASE_FIXTURE (ZrythmFixture, "clone channel track")
{
  auto track = Track::create_track (
    Track::Type::Midi, "Test MIDI Track", TRACKLIST->get_last_pos ());
  auto ch_track = dynamic_cast<MidiTrack *> (track.get ());

  auto expect_ports_have_orig_track_name_hash = [&ch_track] (const auto &track) {
    constexpr size_t    expected_num_ports = 2191;
    std::vector<Port *> ports;
    track->append_ports (ports, false);
    REQUIRE_SIZE_EQ (ports, expected_num_ports);
    for (auto &port : ports)
      {
        REQUIRE_EQ (port->id_.track_name_hash_, ch_track->get_name_hash ());
      }
  };

  // check for original track
  expect_ports_have_orig_track_name_hash (ch_track);

  auto cloned_track = ch_track->clone_unique ();

  // check for cloned track
  expect_ports_have_orig_track_name_hash (cloned_track);
}

TEST_SUITE_END;