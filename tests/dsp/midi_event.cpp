// SPDX-FileCopyrightText: © 2021-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "dsp/midi_event.h"
#include "utils/midi.h"

#include "tests/helpers/zrythm_helper.h"

TEST_SUITE_BEGIN ("dsp/midi_event");

TEST_CASE_FIXTURE (ZrythmFixture, "add note ons from chord descriptor")
{
  midi_time_t _time = 402;

  const auto &descr = CHORD_EDITOR->chords_[0];
  MidiEvents  events;

  /* check at least 3 notes are valid */
  events.active_events_.add_note_ons_from_chord_descr (descr, 1, 121, _time);
  for (int i = 0; i < 3; i++)
    {
      const auto &ev = events.active_events_.at (i);

      REQUIRE_EQ (ev.time_, _time);
      REQUIRE (midi_is_note_on (ev.raw_buffer_.data ()));
      REQUIRE_EQ (midi_get_velocity (ev.raw_buffer_.data ()), 121);
    }
}

TEST_CASE ("add pitch bend")
{
  midi_time_t _time = 402;

  MidiEvents                 events;
  std::array<midi_byte_t, 3> buf = { 0xE3, 0x54, 0x39 };

  events.active_events_.add_event_from_buf (_time, buf.data (), 3);
  const auto &ev = events.active_events_.front ();
  REQUIRE_EQ (ev.time_, _time);
  REQUIRE (midi_is_pitch_wheel (ev.raw_buffer_.data ()));
  REQUIRE_EQ (
    midi_get_pitchwheel_value (ev.raw_buffer_.data ()),
    midi_get_pitchwheel_value (buf.data ()));
  REQUIRE_EQ (midi_get_pitchwheel_value (ev.raw_buffer_.data ()), 0x1CD4);
  REQUIRE_EQ (midi_get_pitchwheel_value (ev.raw_buffer_.data ()), 7380);
  REQUIRE_GE (midi_get_pitchwheel_value (ev.raw_buffer_.data ()), 0);
  REQUIRE_LT (midi_get_pitchwheel_value (ev.raw_buffer_.data ()), 0x4000);
}

TEST_SUITE_END ();