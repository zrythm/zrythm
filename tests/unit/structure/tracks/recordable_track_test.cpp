// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/tracks/recordable_track.h"

#include <QObject>
#include <QSignalSpy>

#include <gtest/gtest.h>

namespace zrythm::structure::tracks
{

class RecordableTrackTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    // Create test track
    track_ = std::make_unique<RecordableTrackMixin> (
      deps_, [] { return u8"Test Track"; }, [] { return true; });
  }

  void TearDown () override { track_.reset (); }

  dsp::PortRegistry               port_registry_;
  dsp::ProcessorParameterRegistry param_registry_{ port_registry_ };
  dsp::ProcessorBase::ProcessorBaseDependencies deps_{
    .port_registry_ = port_registry_,
    .param_registry_ = param_registry_
  };
  std::unique_ptr<RecordableTrackMixin> track_;
};

TEST_F (RecordableTrackTest, ConstructionAndBasicProperties)
{
  // Test that track was created successfully
  EXPECT_NE (track_, nullptr);

  // Test initial recording state
  EXPECT_FALSE (track_->recording ());
}

TEST_F (RecordableTrackTest, SetRecording)
{
  // Test setting recording to true
  track_->setRecording (true);
  EXPECT_TRUE (track_->recording ());

  // Test setting recording to false
  track_->setRecording (false);
  EXPECT_FALSE (track_->recording ());
}

TEST_F (RecordableTrackTest, RecordingChangedSignal)
{
  QSignalSpy spy (track_.get (), &RecordableTrackMixin::recordingChanged);

  // Test signal emission when changing recording state
  track_->setRecording (true);
  EXPECT_EQ (spy.count (), 1);
  EXPECT_TRUE (spy.takeFirst ().at (0).toBool ());

  // Test signal emission when changing to false
  track_->setRecording (false);
  EXPECT_EQ (spy.count (), 1);
  EXPECT_FALSE (spy.takeFirst ().at (0).toBool ());
}

TEST_F (RecordableTrackTest, NoSignalOnSameValue)
{
  QSignalSpy spy (track_.get (), &RecordableTrackMixin::recordingChanged);

  // Set initial state
  track_->setRecording (true);
  spy.clear ();

  // Test no signal when setting same value
  track_->setRecording (true);
  EXPECT_EQ (spy.count (), 0);
}

TEST_F (RecordableTrackTest, AutoArmEnabled)
{
  // Create track with auto-arm enabled
  auto track = std::make_unique<RecordableTrackMixin> (
    deps_, [] { return u8"Auto-arm Track"; }, [] { return true; });

  // Test auto-arm when selected
  track->onRecordableTrackSelectedChanged (true);
  EXPECT_TRUE (track->recording ());

  // Test auto-disarm when deselected
  track->onRecordableTrackSelectedChanged (false);
  EXPECT_FALSE (track->recording ());
}

TEST_F (RecordableTrackTest, AutoArmDisabled)
{
  // Create track with auto-arm disabled
  auto track = std::make_unique<RecordableTrackMixin> (
    deps_, [] { return u8"No Auto-arm Track"; }, [] { return false; });

  // Test no auto-arm when selected
  track->onRecordableTrackSelectedChanged (true);
  EXPECT_FALSE (track->recording ());

  // Test no change when deselected
  track->onRecordableTrackSelectedChanged (false);
  EXPECT_FALSE (track->recording ());
}

TEST_F (RecordableTrackTest, JsonSerializationRoundtrip)
{
  // Set some state
  track_->setRecording (true);

  // Serialize to JSON
  nlohmann::json j = *track_;

  // Create new track from JSON
  auto deserialized_track = std::make_unique<RecordableTrackMixin> (
    deps_, [] { return u8"Deserialized Track"; }, [] { return true; });

  // Deserialize from JSON
  from_json (j, *deserialized_track);

  // Verify properties
  EXPECT_TRUE (deserialized_track->recording ());
}

TEST_F (RecordableTrackTest, JsonSerializationDefaultState)
{
  // Test serialization of default state
  EXPECT_FALSE (track_->recording ());

  nlohmann::json j = *track_;

  // Create new track from JSON
  auto deserialized_track = std::make_unique<RecordableTrackMixin> (
    deps_, [] { return u8"Default Track"; }, [] { return true; });

  from_json (j, *deserialized_track);

  // Verify default state
  EXPECT_FALSE (deserialized_track->recording ());
}

TEST_F (RecordableTrackTest, MultipleStateChanges)
{
  QSignalSpy spy (track_.get (), &RecordableTrackMixin::recordingChanged);

  // Test multiple state changes
  track_->setRecording (true);
  track_->setRecording (false);
  track_->setRecording (true);
  track_->setRecording (true); // Same value, should not emit
  track_->setRecording (false);

  // Should have 4 signals (true, false, true, false)
  EXPECT_EQ (spy.count (), 4);

  // Verify signal values
  EXPECT_TRUE (spy.at (0).at (0).toBool ());
  EXPECT_FALSE (spy.at (1).at (0).toBool ());
  EXPECT_TRUE (spy.at (2).at (0).toBool ());
  EXPECT_FALSE (spy.at (3).at (0).toBool ());
}

TEST_F (RecordableTrackTest, AutoArmWithManualOverride)
{
  // Create track with auto-arm enabled
  auto track = std::make_unique<RecordableTrackMixin> (
    deps_, [] { return u8"Auto-arm Manual Track"; }, [] { return true; });

  // Manually set recording
  track->setRecording (true);
  EXPECT_TRUE (track->recording ());

  // Select another track (should not auto-disarm since it was manually set)
  track->onRecordableTrackSelectedChanged (false);
  EXPECT_TRUE (track->recording ());
}

TEST_F (RecordableTrackTest, RecordingParameterAccess)
{
  // Test that we can access the recording parameter
  auto &recording_param = track_->get_recording_param ();

  // Test parameter properties
  EXPECT_EQ (recording_param.label (), QString ("Track record"));
  EXPECT_TRUE (recording_param.range ().is_toggled (1.0f));
  EXPECT_FALSE (recording_param.range ().is_toggled (0.0f));
}

TEST_F (RecordableTrackTest, NameProviderFunctionality)
{
  bool name_provider_called = false;

  // Create track with name provider that tracks calls
  auto track = std::make_unique<RecordableTrackMixin> (
    deps_,
    [&name_provider_called] {
      name_provider_called = true;
      return u8"Custom Name Track";
    },
    [] { return true; });

  // Trigger an operation that should use the name provider
  track->setRecording (true);

  // Name provider should have been called for logging
  EXPECT_TRUE (name_provider_called);
}

} // namespace zrythm::structure::tracks
