// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/port_identifier.h"
#include "utils/gtest_wrapper.h"

using namespace zrythm::dsp;

TEST (PortIdentifier, Construction)
{
  // Default construction
  PortIdentifier id1;
  EXPECT_EQ (id1.owner_type_, PortIdentifier::OwnerType::AudioEngine);
  EXPECT_EQ (id1.type_, PortType::Unknown);
  EXPECT_EQ (id1.flow_, PortFlow::Unknown);
  EXPECT_EQ (id1.port_index_, 0);
  EXPECT_EQ (id1.track_name_hash_, 0);
  EXPECT_TRUE (id1.label_.empty ());
}

TEST (PortIdentifier, PortTypes)
{
  PortIdentifier id;

  id.type_ = PortType::Control;
  EXPECT_TRUE (id.is_control ());
  EXPECT_FALSE (id.is_audio ());

  id.type_ = PortType::Audio;
  EXPECT_TRUE (id.is_audio ());
  EXPECT_FALSE (id.is_midi ());

  id.type_ = PortType::Event;
  EXPECT_TRUE (id.is_midi ());
  EXPECT_FALSE (id.is_cv ());

  id.type_ = PortType::CV;
  EXPECT_TRUE (id.is_cv ());
  EXPECT_FALSE (id.is_control ());
}

TEST (PortIdentifier, PortFlow)
{
  PortIdentifier id;

  id.flow_ = PortFlow::Input;
  EXPECT_TRUE (id.is_input ());
  EXPECT_FALSE (id.is_output ());

  id.flow_ = PortFlow::Output;
  EXPECT_TRUE (id.is_output ());
  EXPECT_FALSE (id.is_input ());
}

TEST (PortIdentifier, MidiChannels)
{
  PortIdentifier id;

  // Test MIDI pitch bend
  id.flags2_ = PortIdentifier::Flags2::MidiPitchBend;
  id.port_index_ = 2;
  EXPECT_EQ (id.get_midi_channel (), 3); // 1-based channel numbering

  // Test MIDI CC
  id.flags2_ = (PortIdentifier::Flags2) 0;
  id.flags_ = PortIdentifier::Flags::MidiAutomatable;
  id.port_index_ = 256; // Channel 3 (256/128 + 1)
  EXPECT_EQ (id.get_midi_channel (), 3);

  // Test non-MIDI port
  id.flags_ = (PortIdentifier::Flags) 0;
  EXPECT_EQ (id.get_midi_channel (), -1);
}

TEST (PortIdentifier, Comparison)
{
  PortIdentifier id1;
  id1.label_ = "Test Port 1";
  id1.sym_ = "test_port_1";
  id1.owner_type_ = PortIdentifier::OwnerType::Plugin;

  PortIdentifier id2;
  id2.label_ = "Test Port 1";
  id2.sym_ = "test_port_1";
  id2.owner_type_ = PortIdentifier::OwnerType::Plugin;

  EXPECT_EQ (id1, id2);

  id2.sym_ = "test_port_2";
  EXPECT_NE (id1, id2);
}

TEST (PortIdentifier, Hashing)
{
  PortIdentifier id1;
  id1.label_ = "Test Port";
  id1.sym_ = "test_port";
  id1.owner_type_ = PortIdentifier::OwnerType::Track;
  id1.type_ = PortType::Audio;

  PortIdentifier id2;
  id2.label_ = "Test Port";
  id2.sym_ = "test_port";
  id2.owner_type_ = PortIdentifier::OwnerType::Track;
  id2.type_ = PortType::Audio;

  EXPECT_EQ (id1.get_hash (), id2.get_hash ());
}

TEST (PortIdentifier, PortUnitStrings)
{
  EXPECT_EQ (PortIdentifier::port_unit_to_string (PortUnit::None), "none");
  EXPECT_EQ (PortIdentifier::port_unit_to_string (PortUnit::Hz), "Hz");
  EXPECT_EQ (PortIdentifier::port_unit_to_string (PortUnit::Db), "dB");
  EXPECT_EQ (PortIdentifier::port_unit_to_string (PortUnit::Ms), "ms");
}

TEST (PortIdentifier, Serialization)
{
  PortIdentifier id1;
  id1.label_ = "Test Port";
  id1.sym_ = "test_port";
  id1.uri_ = "test://uri";
  id1.comment_ = "Test comment";
  id1.owner_type_ = PortIdentifier::OwnerType::Plugin;
  id1.type_ = PortType::Audio;
  id1.flow_ = PortFlow::Input;
  id1.unit_ = PortUnit::Hz;
  id1.flags_ = PortIdentifier::Flags::MainPort;
  id1.flags2_ = PortIdentifier::Flags2::MidiPitchBend;
  id1.track_name_hash_ = 12345;
  id1.port_index_ = 42;
  id1.port_group_ = "group1";
  id1.ext_port_id_ = "ext1";

  // Serialize to JSON
  auto json_str = id1.serialize_to_json_string ();

  // Create new object and deserialize
  PortIdentifier id2;
  id2.deserialize_from_json_string (json_str.c_str ());

  // Verify all fields match
  EXPECT_EQ (id1.label_, id2.label_);
  EXPECT_EQ (id1.sym_, id2.sym_);
  EXPECT_EQ (id1.uri_, id2.uri_);
  EXPECT_EQ (id1.comment_, id2.comment_);
  EXPECT_EQ (id1.owner_type_, id2.owner_type_);
  EXPECT_EQ (id1.type_, id2.type_);
  EXPECT_EQ (id1.flow_, id2.flow_);
  EXPECT_EQ (id1.unit_, id2.unit_);
  EXPECT_EQ (id1.flags_, id2.flags_);
  EXPECT_EQ (id1.flags2_, id2.flags2_);
  EXPECT_EQ (id1.track_name_hash_, id2.track_name_hash_);
  EXPECT_EQ (id1.port_index_, id2.port_index_);
  EXPECT_EQ (id1.port_group_, id2.port_group_);
  EXPECT_EQ (id1.ext_port_id_, id2.ext_port_id_);
}
