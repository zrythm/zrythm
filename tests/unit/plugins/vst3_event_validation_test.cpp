// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "plugins/vst3_event_validation.h"

#include <gtest/gtest.h>

namespace zrythm::plugins
{

using Steinberg::Vst::Event;

static Event
make_valid_note_on ()
{
  Event ev{};
  ev.type = Event::kNoteOnEvent;
  ev.sampleOffset = 10;
  ev.noteOn.channel = 0;
  ev.noteOn.pitch = 60;
  ev.noteOn.velocity = 0.8f;
  return ev;
}

TEST (Vst3EventValidationTest, ValidNoteOnPasses)
{
  EXPECT_FALSE (validate_vst3_output_event (make_valid_note_on (), 256));
}

TEST (Vst3EventValidationTest, ValidNoteOffPasses)
{
  auto ev = make_valid_note_on ();
  ev.type = Event::kNoteOffEvent;
  ev.noteOff.velocity = 0.5f;
  EXPECT_FALSE (validate_vst3_output_event (ev, 256));
}

TEST (Vst3EventValidationTest, BoundaryOffsetsPass)
{
  auto ev = make_valid_note_on ();
  ev.sampleOffset = 0;
  EXPECT_FALSE (validate_vst3_output_event (ev, 256));
  ev.sampleOffset = 255;
  EXPECT_FALSE (validate_vst3_output_event (ev, 256));
}

TEST (Vst3EventValidationTest, NegativeSampleOffsetRejected)
{
  auto ev = make_valid_note_on ();
  ev.sampleOffset = -1;
  EXPECT_TRUE (validate_vst3_output_event (ev, 256));
}

TEST (Vst3EventValidationTest, SampleOffsetBeyondBlockRejected)
{
  auto ev = make_valid_note_on ();
  ev.sampleOffset = 256;
  EXPECT_TRUE (validate_vst3_output_event (ev, 256));
  ev.sampleOffset = 1000;
  EXPECT_TRUE (validate_vst3_output_event (ev, 256));
}

TEST (Vst3EventValidationTest, ChannelOutOfRangeRejected)
{
  auto ev = make_valid_note_on ();
  ev.noteOn.channel = 16;
  EXPECT_TRUE (validate_vst3_output_event (ev, 256));
  ev.noteOn.channel = -1;
  EXPECT_TRUE (validate_vst3_output_event (ev, 256));
  ev.noteOn.channel = 15;
  EXPECT_FALSE (validate_vst3_output_event (ev, 256));
}

TEST (Vst3EventValidationTest, PitchOutOfRangeRejected)
{
  auto ev = make_valid_note_on ();
  ev.noteOn.pitch = 128;
  EXPECT_TRUE (validate_vst3_output_event (ev, 256));
  ev.noteOn.pitch = -1;
  EXPECT_TRUE (validate_vst3_output_event (ev, 256));
  ev.noteOn.pitch = 127;
  EXPECT_FALSE (validate_vst3_output_event (ev, 256));
}

TEST (Vst3EventValidationTest, VelocityOutOfRangeRejected)
{
  auto ev = make_valid_note_on ();
  ev.noteOn.velocity = -0.1f;
  EXPECT_TRUE (validate_vst3_output_event (ev, 256));
  ev.noteOn.velocity = 1.1f;
  EXPECT_TRUE (validate_vst3_output_event (ev, 256));
  ev.noteOn.velocity = std::numeric_limits<float>::quiet_NaN ();
  EXPECT_TRUE (validate_vst3_output_event (ev, 256));
  ev.noteOn.velocity = 0.0f;
  EXPECT_FALSE (validate_vst3_output_event (ev, 256));
  ev.noteOn.velocity = 1.0f;
  EXPECT_FALSE (validate_vst3_output_event (ev, 256));
}

TEST (Vst3EventValidationTest, NoteOffVelocityOutOfRangeRejected)
{
  auto ev = make_valid_note_on ();
  ev.type = Event::kNoteOffEvent;
  ev.noteOff.velocity = 2.0f;
  EXPECT_TRUE (validate_vst3_output_event (ev, 256));
}

TEST (Vst3EventValidationTest, NonNoteEventsOnlyNeedValidOffset)
{
  Event ev{};
  ev.type = Event::kDataEvent;
  ev.sampleOffset = 0;
  EXPECT_FALSE (validate_vst3_output_event (ev, 256));
  ev.sampleOffset = 256;
  EXPECT_TRUE (validate_vst3_output_event (ev, 256));
}

} // namespace zrythm::plugins
