// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/audio_port.h"
#include "dsp/cv_port.h"
#include "dsp/midi_port.h"
#include "dsp/port_span.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace zrythm::dsp
{

class PortSpanTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    // Create test ports in the registry
    audio_in1_ =
      registry_.create_object<AudioPort> (u8"AudioIn1", PortFlow::Input);
    audio_out1_ =
      registry_.create_object<AudioPort> (u8"AudioOut1", PortFlow::Output);
    midi_in1_ = registry_.create_object<MidiPort> (u8"MIDIIn1", PortFlow::Input);
    cv_out1_ = registry_.create_object<CVPort> (u8"CVOut1", PortFlow::Output);
    audio_in2_ =
      registry_.create_object<AudioPort> (u8"AudioIn2", PortFlow::Input);
  }

  void TearDown () override
  {
    // Registry will clean up objects automatically
  }

  PortRegistry      registry_;
  PortUuidReference audio_in1_{ registry_ };
  PortUuidReference audio_out1_{ registry_ };
  PortUuidReference midi_in1_{ registry_ };
  PortUuidReference cv_out1_{ registry_ };
  PortUuidReference audio_in2_{ registry_ };
};

TEST_F (PortSpanTest, BasicConstruction)
{
  // Test construction with registry and UUIDs
  std::vector<PortUuid> uuids = {
    audio_in1_.id (), audio_out1_.id (), midi_in1_.id ()
  };

  PortSpan span (registry_, uuids);
  EXPECT_EQ (span.size (), 3);
  EXPECT_FALSE (span.empty ());
}

TEST_F (PortSpanTest, EmptyConstruction)
{
  // Test construction with empty UUID list
  std::vector<PortUuid> empty_uuids;
  PortSpan              span (registry_, empty_uuids);

  EXPECT_EQ (span.size (), 0);
  EXPECT_TRUE (span.empty ());
}

TEST_F (PortSpanTest, SingleObjectConstruction)
{
  // Test construction with single object
  PortSpan span (audio_in1_.get_object ());
  EXPECT_EQ (span.size (), 1);
  EXPECT_FALSE (span.empty ());
}

TEST_F (PortSpanTest, LabelProjection)
{
  // Test the static label_projection method
  std::vector<PortUuid> uuids = {
    audio_in1_.id (), audio_out1_.id (), midi_in1_.id (), cv_out1_.id ()
  };

  PortSpan span (registry_, uuids);

  // Collect labels using label_projection
  std::vector<utils::Utf8String> labels;
  for (const auto &port_var : span)
    {
      labels.push_back (PortSpan::label_projection (port_var));
    }

  EXPECT_THAT (
    labels,
    ::testing::UnorderedElementsAre (
      u8"AudioIn1", u8"AudioOut1", u8"MIDIIn1", u8"CVOut1"));
}

TEST_F (PortSpanTest, IteratorFunctionality)
{
  // Test iterator operations
  std::vector<PortUuid> uuids = {
    audio_in1_.id (), audio_out1_.id (), midi_in1_.id ()
  };

  PortSpan span (registry_, uuids);

  // Test begin/end
  auto it = span.begin ();
  auto end = span.end ();
  EXPECT_NE (it, end);

  // Test increment
  ++it;
  EXPECT_NE (it, end);

  // Test dereference
  auto first_port = *span.begin ();
  EXPECT_TRUE (std::holds_alternative<AudioPort *> (first_port));

  // Test distance
  EXPECT_EQ (std::distance (span.begin (), span.end ()), 3);
}

TEST_F (PortSpanTest, RandomAccess)
{
  // Test random access functionality
  std::vector<PortUuid> uuids = {
    audio_in1_.id (), audio_out1_.id (), midi_in1_.id (), cv_out1_.id ()
  };

  PortSpan span (registry_, uuids);

  // Test operator[]
  EXPECT_EQ (PortSpan::label_projection (span[0]), "AudioIn1");
  EXPECT_EQ (PortSpan::label_projection (span[1]), "AudioOut1");
  EXPECT_EQ (PortSpan::label_projection (span[2]), "MIDIIn1");
  EXPECT_EQ (PortSpan::label_projection (span[3]), "CVOut1");

  // Test front/back
  EXPECT_EQ (PortSpan::label_projection (span.front ()), "AudioIn1");
  EXPECT_EQ (PortSpan::label_projection (span.back ()), "CVOut1");

  // Test at() with bounds checking
  EXPECT_EQ (PortSpan::label_projection (span.at (1)), "AudioOut1");
  EXPECT_THROW (span.at (4), std::out_of_range);
}

TEST_F (PortSpanTest, RangeConcepts)
{
  // Verify that PortSpan satisfies random_access_range concept
  std::vector<PortUuid> uuids = { audio_in1_.id (), audio_out1_.id () };

  PortSpan span (registry_, uuids);

  // Test range-based for loop
  std::vector<utils::Utf8String> labels;
  for (const auto &port_var : span)
    {
      labels.push_back (PortSpan::label_projection (port_var));
    }

  EXPECT_THAT (
    labels, ::testing::UnorderedElementsAre (u8"AudioIn1", u8"AudioOut1"));

  // Test std::ranges algorithms
  auto has_audio_in1 = std::ranges::any_of (span, [] (const auto &port_var) {
    return PortSpan::label_projection (port_var) == u8"AudioIn1";
  });
  EXPECT_TRUE (has_audio_in1);
}

TEST_F (PortSpanTest, ConstructionFromUuidReferences)
{
  // Test construction from span of UuidReferences
  std::vector<PortUuidReference> refs = { audio_in1_, audio_out1_, midi_in1_ };

  PortSpan span{ std::span<const PortUuidReference> (refs) };
  EXPECT_EQ (span.size (), 3);

  std::vector<utils::Utf8String> labels;
  for (const auto &port_var : span)
    {
      labels.push_back (PortSpan::label_projection (port_var));
    }

  EXPECT_THAT (
    labels,
    ::testing::UnorderedElementsAre (u8"AudioIn1", u8"AudioOut1", u8"MIDIIn1"));
}

TEST_F (PortSpanTest, ConstructionFromObjects)
{
  // Test construction from span of objects
  std::vector<PortPtrVariant> objects = {
    audio_in1_.get_object (), audio_out1_.get_object (), midi_in1_.get_object ()
  };

  PortSpan span{ std::span<const PortPtrVariant> (objects) };
  EXPECT_EQ (span.size (), 3);

  std::vector<utils::Utf8String> labels;
  for (const auto &port_var : span)
    {
      labels.push_back (PortSpan::label_projection (port_var));
    }

  EXPECT_THAT (
    labels,
    ::testing::UnorderedElementsAre (u8"AudioIn1", u8"AudioOut1", u8"MIDIIn1"));
}

} // namespace zrythm::dsp
