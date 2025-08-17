// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/tracks/track.h"

#include "unit/dsp/graph_helpers.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace zrythm::structure::tracks
{

class MockTrack : public Track
{
public:
  using TrackFeatures = Track::TrackFeatures;

  MockTrack (
    Type                  type,
    PortType              in_signal_type,
    PortType              out_signal_type,
    TrackFeatures         features,
    BaseTrackDependencies dependencies)
      : Track (type, in_signal_type, out_signal_type, features, std::move (dependencies))
  {
    processor_ = make_track_processor (std::nullopt, std::nullopt, std::nullopt);
  }

  MOCK_METHOD (
    void,
    collect_additional_timeline_objects,
    (std::vector<ArrangerObjectPtrVariant> & objects),
    (const, override));
};

class TrackTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    port_registry_ = std::make_unique<dsp::PortRegistry> ();
    param_registry_ =
      std::make_unique<dsp::ProcessorParameterRegistry> (*port_registry_);
    plugin_registry_ = std::make_unique<plugins::PluginRegistry> ();
    file_audio_source_registry_ =
      std::make_unique<dsp::FileAudioSourceRegistry> ();
    obj_registry_ = std::make_unique<arrangement::ArrangerObjectRegistry> ();
    tempo_map_ = std::make_unique<dsp::TempoMap> (sample_rate_);
    transport_ = std::make_unique<dsp::graph_test::MockTransport> ();

    base_dependencies_ = std::make_unique<BaseTrackDependencies> (
      *tempo_map_, *file_audio_source_registry_, *plugin_registry_,
      *port_registry_, *param_registry_, *obj_registry_, *transport_,
      [] { return false; });
  }

  std::unique_ptr<MockTrack> createMockTrack (
    Track::Type              type,
    dsp::PortType            in_type = dsp::PortType::Audio,
    dsp::PortType            out_type = dsp::PortType::Audio,
    MockTrack::TrackFeatures features =
      MockTrack::TrackFeatures::Automation | MockTrack::TrackFeatures::Lanes
      | MockTrack::TrackFeatures::Modulators
      | MockTrack::TrackFeatures::Recording)
  {
    return std::make_unique<MockTrack> (
      type, in_type, out_type, features, *base_dependencies_);
  }

  std::unique_ptr<dsp::PortRegistry>               port_registry_;
  std::unique_ptr<dsp::ProcessorParameterRegistry> param_registry_;
  std::unique_ptr<plugins::PluginRegistry>         plugin_registry_;
  std::unique_ptr<dsp::FileAudioSourceRegistry>    file_audio_source_registry_;
  std::unique_ptr<arrangement::ArrangerObjectRegistry> obj_registry_;
  std::unique_ptr<dsp::TempoMap>                       tempo_map_;
  std::unique_ptr<dsp::graph_test::MockTransport>      transport_;
  std::unique_ptr<BaseTrackDependencies>               base_dependencies_;

  sample_rate_t sample_rate_{ 48000 };
};

TEST_F (TrackTest, TrackTypeEnum)
{
  EXPECT_EQ (
    static_cast<int> (Track::Type::Instrument),
    static_cast<int> (Track::get_type_for_class<InstrumentTrack> ()));
  EXPECT_EQ (
    static_cast<int> (Track::Type::Audio),
    static_cast<int> (Track::get_type_for_class<AudioTrack> ()));
  EXPECT_EQ (
    static_cast<int> (Track::Type::Midi),
    static_cast<int> (Track::get_type_for_class<MidiTrack> ()));
  EXPECT_EQ (
    static_cast<int> (Track::Type::Chord),
    static_cast<int> (Track::get_type_for_class<ChordTrack> ()));
  EXPECT_EQ (
    static_cast<int> (Track::Type::Master),
    static_cast<int> (Track::get_type_for_class<MasterTrack> ()));
  EXPECT_EQ (
    static_cast<int> (Track::Type::Modulator),
    static_cast<int> (Track::get_type_for_class<ModulatorTrack> ()));
  EXPECT_EQ (
    static_cast<int> (Track::Type::Marker),
    static_cast<int> (Track::get_type_for_class<MarkerTrack> ()));
  EXPECT_EQ (
    static_cast<int> (Track::Type::Folder),
    static_cast<int> (Track::get_type_for_class<FolderTrack> ()));
  EXPECT_EQ (
    static_cast<int> (Track::Type::AudioBus),
    static_cast<int> (Track::get_type_for_class<AudioBusTrack> ()));
  EXPECT_EQ (
    static_cast<int> (Track::Type::MidiBus),
    static_cast<int> (Track::get_type_for_class<MidiBusTrack> ()));
  EXPECT_EQ (
    static_cast<int> (Track::Type::AudioGroup),
    static_cast<int> (Track::get_type_for_class<AudioGroupTrack> ()));
  EXPECT_EQ (
    static_cast<int> (Track::Type::MidiGroup),
    static_cast<int> (Track::get_type_for_class<MidiGroupTrack> ()));
}

TEST_F (TrackTest, TrackTypeProperties)
{
  EXPECT_TRUE (Track::type_has_piano_roll (Track::Type::Instrument));
  EXPECT_TRUE (Track::type_has_piano_roll (Track::Type::Midi));
  EXPECT_FALSE (Track::type_has_piano_roll (Track::Type::Audio));
  EXPECT_FALSE (Track::type_has_piano_roll (Track::Type::Master));

  EXPECT_TRUE (Track::type_has_inputs (Track::Type::Instrument));
  EXPECT_TRUE (Track::type_has_inputs (Track::Type::Midi));
  EXPECT_TRUE (Track::type_has_inputs (Track::Type::Audio));
  EXPECT_FALSE (Track::type_has_inputs (Track::Type::Master));

  EXPECT_TRUE (Track::type_can_be_group_target (Track::Type::AudioGroup));
  EXPECT_TRUE (Track::type_can_be_group_target (Track::Type::MidiGroup));
  EXPECT_TRUE (Track::type_can_be_group_target (Track::Type::Instrument));
  EXPECT_TRUE (Track::type_can_be_group_target (Track::Type::Master));
  EXPECT_FALSE (Track::type_can_be_group_target (Track::Type::Audio));
}

TEST_F (TrackTest, TrackTypeCompatibility)
{
  EXPECT_TRUE (
    Track::type_can_have_region_type (
      Track::Type::Audio, arrangement::ArrangerObject::Type::AudioRegion));
  EXPECT_FALSE (
    Track::type_can_have_region_type (
      Track::Type::Midi, arrangement::ArrangerObject::Type::AudioRegion));
  EXPECT_TRUE (
    Track::type_can_have_region_type (
      Track::Type::Midi, arrangement::ArrangerObject::Type::MidiRegion));
  EXPECT_TRUE (
    Track::type_can_have_region_type (
      Track::Type::Instrument, arrangement::ArrangerObject::Type::MidiRegion));
  EXPECT_TRUE (
    Track::type_can_have_region_type (
      Track::Type::Chord, arrangement::ArrangerObject::Type::ChordRegion));
  EXPECT_TRUE (
    Track::type_can_have_region_type (
      Track::Type::Audio, arrangement::ArrangerObject::Type::AutomationRegion));

  EXPECT_TRUE (
    Track::type_is_compatible_for_moving (
      Track::Type::Midi, Track::Type::Instrument));
  EXPECT_TRUE (
    Track::type_is_compatible_for_moving (
      Track::Type::Instrument, Track::Type::Midi));
  EXPECT_TRUE (
    Track::type_is_compatible_for_moving (
      Track::Type::Audio, Track::Type::Audio));
  EXPECT_FALSE (
    Track::type_is_compatible_for_moving (Track::Type::Audio, Track::Type::Midi));
}

TEST_F (TrackTest, TrackCopyableAndDeletable)
{
  EXPECT_FALSE (Track::type_is_copyable (Track::Type::Master));
  EXPECT_FALSE (Track::type_is_deletable (Track::Type::Master));
  EXPECT_FALSE (Track::type_is_copyable (Track::Type::Chord));
  EXPECT_FALSE (Track::type_is_deletable (Track::Type::Chord));
  EXPECT_FALSE (Track::type_is_copyable (Track::Type::Modulator));
  EXPECT_FALSE (Track::type_is_deletable (Track::Type::Modulator));
  EXPECT_FALSE (Track::type_is_copyable (Track::Type::Marker));
  EXPECT_FALSE (Track::type_is_deletable (Track::Type::Marker));

  EXPECT_TRUE (Track::type_is_copyable (Track::Type::Audio));
  EXPECT_TRUE (Track::type_is_deletable (Track::Type::Audio));
  EXPECT_TRUE (Track::type_is_copyable (Track::Type::Instrument));
  EXPECT_TRUE (Track::type_is_deletable (Track::Type::Instrument));
  EXPECT_TRUE (Track::type_is_copyable (Track::Type::Midi));
  EXPECT_TRUE (Track::type_is_deletable (Track::Type::Midi));
  EXPECT_TRUE (Track::type_is_copyable (Track::Type::AudioGroup));
  EXPECT_TRUE (Track::type_is_deletable (Track::Type::AudioGroup));
  EXPECT_TRUE (Track::type_is_copyable (Track::Type::MidiGroup));
  EXPECT_TRUE (Track::type_is_deletable (Track::Type::MidiGroup));
}

TEST_F (TrackTest, QmlProperties)
{
  auto track = createMockTrack (Track::Type::Audio);

  // Name
  EXPECT_EQ (track->name (), QString ());
  track->setName ("Test Track");
  EXPECT_EQ (track->name (), QString ("Test Track"));

  // Color
  EXPECT_EQ (track->color (), QColor (0, 0, 0));
  track->setColor (QColor (255, 0, 0));
  EXPECT_EQ (track->color (), QColor (255, 0, 0));

  // Comment
  EXPECT_EQ (track->comment (), QString ());
  track->setComment ("Test comment");
  EXPECT_EQ (track->comment (), QString ("Test comment"));

  // Icon
  EXPECT_EQ (track->icon (), QString ());
  track->setIcon ("test-icon");
  EXPECT_EQ (track->icon (), QString ("test-icon"));

  // Visible
  EXPECT_TRUE (track->visible ());
  track->setVisible (false);
  EXPECT_FALSE (track->visible ());

  // Enabled
  EXPECT_TRUE (track->enabled ());
  track->setEnabled (false);
  EXPECT_FALSE (track->enabled ());

  // Height
  EXPECT_EQ (track->height (), Track::DEF_HEIGHT);
  track->setHeight (100.0);
  EXPECT_EQ (track->height (), 100.0);
}

TEST_F (TrackTest, TrackComponents)
{
  auto track = createMockTrack (Track::Type::Instrument);

  // Channel
  EXPECT_NE (track->channel (), nullptr);

  // Automation tracklist
  EXPECT_NE (track->automationTracklist (), nullptr);

  // Modulators
  EXPECT_NE (track->modulators (), nullptr);

  // Lanes
  EXPECT_NE (track->lanes (), nullptr);
}

TEST_F (TrackTest, TrackHeightCalculations)
{
  auto track = createMockTrack (Track::Type::Audio);

  // Default height
  EXPECT_EQ (track->height (), Track::DEF_HEIGHT);
  EXPECT_EQ (track->fullVisibleHeight (), Track::DEF_HEIGHT);

  // Set height
  track->setHeight (100.0);
  EXPECT_EQ (track->height (), 100.0);
  EXPECT_EQ (track->fullVisibleHeight (), 100.0);

  // Minimum height
  track->setHeight (Track::MIN_HEIGHT - 1);
  EXPECT_EQ (track->height (), Track::MIN_HEIGHT);
}

TEST_F (TrackTest, TrackTypeSpecificBehavior)
{
  // Audio track
  {
    auto audio_track = createMockTrack (Track::Type::Audio);
    EXPECT_FALSE (audio_track->has_piano_roll ());
    EXPECT_TRUE (audio_track->is_audio ());
    EXPECT_FALSE (audio_track->is_midi ());
    EXPECT_FALSE (audio_track->is_instrument ());
  }

  // MIDI track
  {
    auto midi_track = createMockTrack (
      Track::Type::Midi, dsp::PortType::Midi, dsp::PortType::Midi);
    EXPECT_TRUE (midi_track->has_piano_roll ());
    EXPECT_FALSE (midi_track->is_audio ());
    EXPECT_TRUE (midi_track->is_midi ());
    EXPECT_FALSE (midi_track->is_instrument ());
  }

  // Instrument track
  {
    auto instrument_track = createMockTrack (
      Track::Type::Instrument, dsp::PortType::Midi, dsp::PortType::Audio);
    EXPECT_TRUE (instrument_track->has_piano_roll ());
    EXPECT_FALSE (instrument_track->is_audio ());
    EXPECT_FALSE (instrument_track->is_midi ());
    EXPECT_TRUE (instrument_track->is_instrument ());
  }

  // Master track
  {
    auto master_track = createMockTrack (Track::Type::Master);
    EXPECT_FALSE (master_track->has_piano_roll ());
    EXPECT_TRUE (master_track->is_master ());
    EXPECT_FALSE (master_track->is_deletable ());
    EXPECT_FALSE (master_track->is_copyable ());
  }
}

TEST_F (TrackTest, PluginDescriptorValidation)
{
  // Test plugin descriptor validation for different slot types
  plugins::PluginDescriptor descriptor;

  // Audio plugin
  descriptor.num_audio_outs_ = 2;
  descriptor.num_midi_outs_ = 0;
  descriptor.num_midi_ins_ = 0;

  EXPECT_TRUE (
    Track::is_plugin_descriptor_valid_for_slot_type (
      descriptor, plugins::PluginSlotType::Insert, Track::Type::Audio));
  EXPECT_FALSE (
    Track::is_plugin_descriptor_valid_for_slot_type (
      descriptor, plugins::PluginSlotType::MidiFx, Track::Type::Audio));

  // MIDI plugin
  descriptor.num_audio_outs_ = 0;
  descriptor.num_midi_outs_ = 1;
  descriptor.num_midi_ins_ = 1;
  descriptor.category_ = plugins::PluginCategory::MIDI;

  EXPECT_TRUE (
    Track::is_plugin_descriptor_valid_for_slot_type (
      descriptor, plugins::PluginSlotType::MidiFx, Track::Type::Midi));
  EXPECT_TRUE (
    Track::is_plugin_descriptor_valid_for_slot_type (
      descriptor, plugins::PluginSlotType::Insert, Track::Type::Midi));

  // Instrument plugin
  descriptor.num_audio_outs_ = 2;
  descriptor.num_midi_outs_ = 0;
  descriptor.num_midi_ins_ = 1;
  descriptor.category_ = plugins::PluginCategory::Instrument;

  EXPECT_TRUE (
    Track::is_plugin_descriptor_valid_for_slot_type (
      descriptor, plugins::PluginSlotType::Instrument, Track::Type::Instrument));
  EXPECT_FALSE (
    Track::is_plugin_descriptor_valid_for_slot_type (
      descriptor, plugins::PluginSlotType::Instrument, Track::Type::Audio));
}

} // namespace zrythm::structure::tracks
