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
  EXPECT_FALSE (id1.track_id_.has_value ());
  EXPECT_FALSE (id1.plugin_id_.has_value ());
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

TEST (PortIdentifier, Identifiers)
{
  PortIdentifier id;

  // Test track ID
  auto track_uuid = PortIdentifier::TrackUuid (QUuid::createUuid ());
  id.set_track_id (track_uuid);
  EXPECT_EQ (id.get_track_id (), track_uuid);

  // Test plugin ID
  auto plugin_uuid = PortIdentifier::PluginUuid (QUuid::createUuid ());
  id.set_plugin_id (plugin_uuid);
  EXPECT_EQ (id.get_plugin_id (), plugin_uuid);
}

TEST (PortIdentifier, MonitorFaderPorts)
{
  PortIdentifier id;

  id.flags2_ = PortIdentifier::Flags2::MonitorFader;
  id.flags_ = PortIdentifier::Flags::StereoL;
  EXPECT_TRUE (id.is_monitor_fader_stereo_in_or_out_port ());

  id.flags_ = PortIdentifier::Flags::StereoR;
  EXPECT_TRUE (id.is_monitor_fader_stereo_in_or_out_port ());

  id.flags_ = PortIdentifier::Flags{};
  EXPECT_FALSE (id.is_monitor_fader_stereo_in_or_out_port ());
}

TEST (PortIdentifier, Comparison)
{
  PortIdentifier id1;
  id1.label_ = u8"Test Port 1";
  id1.sym_ = u8"test_port_1";
  id1.owner_type_ = PortIdentifier::OwnerType::Plugin;

  PortIdentifier id2;
  id2.label_ = u8"Test Port 1";
  id2.sym_ = u8"test_port_1";
  id2.owner_type_ = PortIdentifier::OwnerType::Plugin;

  EXPECT_EQ (id1, id2);

  id2.sym_ = u8"test_port_2";
  EXPECT_NE (id1, id2);
}

TEST (PortIdentifier, Hashing)
{
  PortIdentifier id1;
  id1.label_ = u8"Test Port";
  id1.sym_ = u8"test_port";
  id1.owner_type_ = PortIdentifier::OwnerType::Track;
  id1.type_ = PortType::Audio;

  PortIdentifier id2;
  id2.label_ = u8"Test Port";
  id2.sym_ = u8"test_port";
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
  id1.label_ = u8"Test Port";
  id1.sym_ = u8"test_port";
  id1.uri_ = u8"test://uri";
  id1.comment_ = u8"Test comment";
  id1.owner_type_ = PortIdentifier::OwnerType::Plugin;
  id1.type_ = PortType::Audio;
  id1.flow_ = PortFlow::Input;
  id1.unit_ = PortUnit::Hz;
  id1.flags_ = PortIdentifier::Flags::MainPort;
  id1.flags2_ = PortIdentifier::Flags2::MidiPitchBend;
  id1.port_index_ = 42;
  id1.port_group_ = u8"group1";
  id1.ext_port_id_ = u8"ext1";
  id1.midi_channel_ = 1;
  id1.set_track_id (PortIdentifier::TrackUuid (QUuid::createUuid ()));
  id1.set_plugin_id (PortIdentifier::PluginUuid (QUuid::createUuid ()));

  // Serialize to JSON
  nlohmann::json json = id1;

  // Create new object and deserialize
  PortIdentifier id2;
  from_json (json, id2);

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
  EXPECT_EQ (id1.port_index_, id2.port_index_);
  EXPECT_EQ (id1.port_group_, id2.port_group_);
  EXPECT_EQ (id1.ext_port_id_, id2.ext_port_id_);
  EXPECT_EQ (id1.midi_channel_, id2.midi_channel_);
  EXPECT_EQ (id1.get_track_id (), id2.get_track_id ());
  EXPECT_EQ (id1.get_plugin_id (), id2.get_plugin_id ());
}
