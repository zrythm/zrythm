// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/tracks/track_lane.h"

#include <QObject>

#include <gtest/gtest.h>

namespace zrythm::structure::tracks
{

class TrackLaneTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    obj_registry_ = std::make_unique<arrangement::ArrangerObjectRegistry> ();
    file_audio_source_registry_ =
      std::make_unique<dsp::FileAudioSourceRegistry> ();

    // Create dependencies for TrackLane
    TrackLane::TrackLaneDependencies deps{
      .obj_registry_ = *obj_registry_,
      .file_audio_source_registry_ = *file_audio_source_registry_,
      .soloed_lanes_exist_func_ = [this] () { return soloed_lanes_exist_; }
    };

    midi_lane_ = std::make_unique<TrackLane> (deps);
    audio_lane_ = std::make_unique<TrackLane> (deps);
  }

  void TearDown () override
  {
    midi_lane_.reset ();
    audio_lane_.reset ();
  }

  std::unique_ptr<arrangement::ArrangerObjectRegistry> obj_registry_;
  std::unique_ptr<dsp::FileAudioSourceRegistry> file_audio_source_registry_;
  bool                                          soloed_lanes_exist_ = false;
  std::unique_ptr<TrackLane>                    midi_lane_;
  std::unique_ptr<TrackLane>                    audio_lane_;
};

TEST_F (TrackLaneTest, ConstructionAndBasicProperties)
{
  // Test default construction
  EXPECT_NE (midi_lane_, nullptr);
  EXPECT_NE (audio_lane_, nullptr);

  // Test default values
  EXPECT_TRUE (midi_lane_->name ().isEmpty ());
  EXPECT_TRUE (audio_lane_->name ().isEmpty ());
  EXPECT_DOUBLE_EQ (midi_lane_->height (), TrackLane::DEFAULT_HEIGHT);
  EXPECT_DOUBLE_EQ (audio_lane_->height (), TrackLane::DEFAULT_HEIGHT);
  EXPECT_FALSE (midi_lane_->muted ());
  EXPECT_FALSE (audio_lane_->muted ());
  EXPECT_FALSE (midi_lane_->soloed ());
  EXPECT_FALSE (audio_lane_->soloed ());
}

TEST_F (TrackLaneTest, NameProperty)
{
  // Test name setting and getting
  midi_lane_->setName ("Test MIDI Lane");
  EXPECT_EQ (midi_lane_->name (), QString ("Test MIDI Lane"));

  // Test empty name
  midi_lane_->setName ("");
  EXPECT_EQ (midi_lane_->name (), QString (""));

  // Test name with special characters
  midi_lane_->setName ("MIDI Lane 2 - ドラム");
  EXPECT_EQ (midi_lane_->name (), QString ("MIDI Lane 2 - ドラム"));

  // Test name change signal
  bool    name_changed = false;
  QString new_name;
  QObject::connect (
    midi_lane_.get (), &TrackLane::nameChanged, midi_lane_.get (),
    [&] (const QString &name) {
      name_changed = true;
      new_name = name;
    });
  midi_lane_->setName ("New Name");
  EXPECT_TRUE (name_changed);
  EXPECT_EQ (new_name, QString ("New Name"));

  // Test default name generation
  midi_lane_->generate_name (0);
  EXPECT_EQ (midi_lane_->name (), QString ("Lane 1"));
}

TEST_F (TrackLaneTest, HeightProperty)
{
  // Test height setting and getting
  audio_lane_->setHeight (100.0);
  EXPECT_DOUBLE_EQ (audio_lane_->height (), 100.0);

  // Test height change signal
  bool   height_changed = false;
  double new_height = 0.0;
  QObject::connect (
    audio_lane_.get (), &TrackLane::heightChanged, audio_lane_.get (),
    [&] (double height) {
      height_changed = true;
      new_height = height;
    });
  audio_lane_->setHeight (75.5);
  EXPECT_TRUE (height_changed);
  EXPECT_DOUBLE_EQ (new_height, 75.5);
}

TEST_F (TrackLaneTest, MuteProperty)
{
  // Test mute setting and getting
  midi_lane_->setMuted (true);
  EXPECT_TRUE (midi_lane_->muted ());

  midi_lane_->setMuted (false);
  EXPECT_FALSE (midi_lane_->muted ());

  // Test mute change signal
  bool mute_changed = false;
  bool new_mute = false;
  QObject::connect (
    midi_lane_.get (), &TrackLane::muteChanged, midi_lane_.get (),
    [&] (bool mute) {
      mute_changed = true;
      new_mute = mute;
    });
  midi_lane_->setMuted (true);
  EXPECT_TRUE (mute_changed);
  EXPECT_TRUE (new_mute);
}

TEST_F (TrackLaneTest, SoloProperty)
{
  // Test solo setting and getting
  audio_lane_->setSoloed (true);
  EXPECT_TRUE (audio_lane_->soloed ());

  audio_lane_->setSoloed (false);
  EXPECT_FALSE (audio_lane_->soloed ());

  // Test solo change signal
  bool solo_changed = false;
  bool new_solo = false;
  QObject::connect (
    audio_lane_.get (), &TrackLane::soloChanged, audio_lane_.get (),
    [&] (bool solo) {
      solo_changed = true;
      new_solo = solo;
    });
  audio_lane_->setSoloed (true);
  EXPECT_TRUE (solo_changed);
  EXPECT_TRUE (new_solo);
}

TEST_F (TrackLaneTest, EffectivelyMutedLogic)
{
  // Test 1: No soloed lanes, lane not muted
  soloed_lanes_exist_ = false;
  midi_lane_->setMuted (false);
  midi_lane_->setSoloed (false);
  EXPECT_FALSE (midi_lane_->effectivelyMuted ());

  // Test 2: No soloed lanes, lane muted
  midi_lane_->setMuted (true);
  EXPECT_TRUE (midi_lane_->effectivelyMuted ());

  // Test 3: Soloed lanes exist, lane not soloed
  soloed_lanes_exist_ = true;
  midi_lane_->setMuted (false);
  midi_lane_->setSoloed (false);
  EXPECT_TRUE (midi_lane_->effectivelyMuted ());

  // Test 4: Soloed lanes exist, lane soloed
  midi_lane_->setSoloed (true);
  EXPECT_FALSE (midi_lane_->effectivelyMuted ());

  // Test 5: Soloed lanes exist, lane soloed but muted
  midi_lane_->setMuted (true);
  midi_lane_->setSoloed (true);
  EXPECT_TRUE (midi_lane_->effectivelyMuted ());

  // Test 6: Soloed lanes exist, lane not muted but soloed
  midi_lane_->setMuted (false);
  midi_lane_->setSoloed (true);
  EXPECT_FALSE (midi_lane_->effectivelyMuted ());
}

TEST_F (TrackLaneTest, MidiRegionManagement)
{
  // Test adding MIDI regions
  EXPECT_EQ (midi_lane_->midiRegions ()->rowCount (), 0);

  // Create a MIDI region
  dsp::TempoMap tempo_map{ 44100 };
  auto midi_region_ref = obj_registry_->create_object<arrangement::MidiRegion> (
    tempo_map, *obj_registry_, *file_audio_source_registry_);

  // Add region to lane
  midi_lane_->arrangement::ArrangerObjectOwner<
    arrangement::MidiRegion>::add_object (midi_region_ref);
  EXPECT_EQ (midi_lane_->midiRegions ()->rowCount (), 1);
  EXPECT_EQ (
    midi_lane_
      ->arrangement::ArrangerObjectOwner<
        arrangement::MidiRegion>::get_children_vector ()
      .at (0)
      .id (),
    midi_region_ref.id ());

  // Test removing MIDI regions
  midi_lane_->arrangement::ArrangerObjectOwner<
    arrangement::MidiRegion>::remove_object (midi_region_ref.id ());
  EXPECT_EQ (midi_lane_->midiRegions ()->rowCount (), 0);
}

TEST_F (TrackLaneTest, AudioRegionManagement)
{
  // Test adding audio regions
  EXPECT_EQ (audio_lane_->audioRegions ()->rowCount (), 0);

  // Create an audio region
  auto          musical_mode_getter = [] { return false; };
  dsp::TempoMap tempo_map{ 44100 };
  auto audio_region_ref = obj_registry_->create_object<arrangement::AudioRegion> (
    tempo_map, *obj_registry_, *file_audio_source_registry_,
    musical_mode_getter);

  // Add region to lane
  audio_lane_->arrangement::ArrangerObjectOwner<
    arrangement::AudioRegion>::add_object (audio_region_ref);
  EXPECT_EQ (audio_lane_->audioRegions ()->rowCount (), 1);
  EXPECT_EQ (
    audio_lane_
      ->arrangement::ArrangerObjectOwner<
        arrangement::AudioRegion>::get_children_vector ()
      .at (0)
      .id (),
    audio_region_ref.id ());

  // Test removing audio regions
  audio_lane_->arrangement::ArrangerObjectOwner<
    arrangement::AudioRegion>::remove_object (audio_region_ref.id ());
  EXPECT_EQ (audio_lane_->audioRegions ()->rowCount (), 0);
}

TEST_F (TrackLaneTest, JsonSerializationRoundtrip)
{
  // Set up test data
  midi_lane_->setName ("Test MIDI Lane");
  midi_lane_->setHeight (64.0);
  midi_lane_->setMuted (true);
  midi_lane_->setSoloed (false);

  // Add a MIDI region
  dsp::TempoMap tempo_map{ 44100 };
  auto midi_region_ref = obj_registry_->create_object<arrangement::MidiRegion> (
    tempo_map, *obj_registry_, *file_audio_source_registry_);
  midi_lane_->arrangement::ArrangerObjectOwner<
    arrangement::MidiRegion>::add_object (midi_region_ref);

  // Serialize to JSON
  nlohmann::json j = *midi_lane_;

  // Create new lane from JSON
  TrackLane::TrackLaneDependencies deps{
    .obj_registry_ = *obj_registry_,
    .file_audio_source_registry_ = *file_audio_source_registry_,
    .soloed_lanes_exist_func_ = [this] () { return soloed_lanes_exist_; }
  };
  TrackLane deserialized_lane (deps);

  // Deserialize from JSON
  from_json (j, deserialized_lane);

  // Verify properties
  EXPECT_EQ (deserialized_lane.name (), QString ("Test MIDI Lane"));
  EXPECT_DOUBLE_EQ (deserialized_lane.height (), 64.0);
  EXPECT_TRUE (deserialized_lane.muted ());
  EXPECT_FALSE (deserialized_lane.soloed ());
  EXPECT_EQ (deserialized_lane.midiRegions ()->rowCount (), 1);
  EXPECT_EQ (
    deserialized_lane
      .arrangement::ArrangerObjectOwner<
        arrangement::MidiRegion>::get_children_vector ()
      .at (0)
      .id (),
    midi_region_ref.id ());
}

TEST_F (TrackLaneTest, JsonSerializationAudioRoundtrip)
{
  // Set up test data
  audio_lane_->setName ("Test Audio Lane");
  audio_lane_->setHeight (96.0);
  audio_lane_->setMuted (false);
  audio_lane_->setSoloed (true);

  // Add an audio region
  auto          musical_mode_getter = [] { return false; };
  dsp::TempoMap tempo_map{ 44100 };
  auto audio_region_ref = obj_registry_->create_object<arrangement::AudioRegion> (
    tempo_map, *obj_registry_, *file_audio_source_registry_,
    musical_mode_getter);
  audio_lane_->arrangement::ArrangerObjectOwner<
    arrangement::AudioRegion>::add_object (audio_region_ref);

  // Serialize to JSON
  nlohmann::json j = *audio_lane_;

  // Create new lane from JSON
  TrackLane::TrackLaneDependencies deps{
    .obj_registry_ = *obj_registry_,
    .file_audio_source_registry_ = *file_audio_source_registry_,
    .soloed_lanes_exist_func_ = [this] () { return soloed_lanes_exist_; }
  };
  TrackLane deserialized_lane (deps);

  // Deserialize from JSON
  from_json (j, deserialized_lane);

  // Verify properties
  EXPECT_EQ (deserialized_lane.name (), QString ("Test Audio Lane"));
  EXPECT_DOUBLE_EQ (deserialized_lane.height (), 96.0);
  EXPECT_FALSE (deserialized_lane.muted ());
  EXPECT_TRUE (deserialized_lane.soloed ());
  EXPECT_EQ (deserialized_lane.audioRegions ()->rowCount (), 1);
  EXPECT_EQ (
    deserialized_lane
      .arrangement::ArrangerObjectOwner<
        arrangement::AudioRegion>::get_children_vector ()
      .at (0)
      .id (),
    audio_region_ref.id ());
}

TEST_F (TrackLaneTest, NameGeneration)
{
  // Test name generation with different positions
  midi_lane_->generate_name (1);
  EXPECT_EQ (midi_lane_->name (), QString ("Lane 2"));

  midi_lane_->generate_name (0);
  EXPECT_EQ (midi_lane_->name (), QString ("Lane 1"));

  midi_lane_->generate_name (5);
  EXPECT_EQ (midi_lane_->name (), QString ("Lane 6"));

  // Test with existing name
  midi_lane_->setName ("Custom Name");
  midi_lane_->generate_name (3);
  EXPECT_EQ (midi_lane_->name (), QString ("Lane 4")); // Should override
}

TEST_F (TrackLaneTest, EmptyLaneSerialization)
{
  // Test serialization of empty lane
  midi_lane_->setName ("Empty Lane");
  midi_lane_->setHeight (TrackLane::DEFAULT_HEIGHT);
  midi_lane_->setMuted (false);
  midi_lane_->setSoloed (false);

  nlohmann::json j = *midi_lane_;

  TrackLane::TrackLaneDependencies deps{
    .obj_registry_ = *obj_registry_,
    .file_audio_source_registry_ = *file_audio_source_registry_,
    .soloed_lanes_exist_func_ = [this] () { return soloed_lanes_exist_; }
  };
  TrackLane deserialized_lane (deps);

  from_json (j, deserialized_lane);

  EXPECT_EQ (deserialized_lane.name (), QString ("Empty Lane"));
  EXPECT_DOUBLE_EQ (deserialized_lane.height (), TrackLane::DEFAULT_HEIGHT);
  EXPECT_FALSE (deserialized_lane.muted ());
  EXPECT_FALSE (deserialized_lane.soloed ());
  EXPECT_EQ (deserialized_lane.midiRegions ()->rowCount (), 0);
  EXPECT_EQ (deserialized_lane.audioRegions ()->rowCount (), 0);
}

TEST_F (TrackLaneTest, PropertyChangeNotifications)
{
  int name_changes = 0;
  int height_changes = 0;
  int mute_changes = 0;
  int solo_changes = 0;

  QObject::connect (
    midi_lane_.get (), &TrackLane::nameChanged, midi_lane_.get (),
    [&] (const QString &) { name_changes++; });
  QObject::connect (
    midi_lane_.get (), &TrackLane::heightChanged, midi_lane_.get (),
    [&] (double) { height_changes++; });
  QObject::connect (
    midi_lane_.get (), &TrackLane::muteChanged, midi_lane_.get (),
    [&] (bool) { mute_changes++; });
  QObject::connect (
    midi_lane_.get (), &TrackLane::soloChanged, midi_lane_.get (),
    [&] (bool) { solo_changes++; });

  // Test that setting same values doesn't emit signals
  midi_lane_->setName (midi_lane_->name ());
  midi_lane_->setHeight (midi_lane_->height ());
  midi_lane_->setMuted (midi_lane_->muted ());
  midi_lane_->setSoloed (midi_lane_->soloed ());

  EXPECT_EQ (name_changes, 0);
  EXPECT_EQ (height_changes, 0);
  EXPECT_EQ (mute_changes, 0);
  EXPECT_EQ (solo_changes, 0);

  // Test that setting different values emits signals
  midi_lane_->setName ("New Name");
  midi_lane_->setHeight (100.0);
  midi_lane_->setMuted (true);
  midi_lane_->setSoloed (true);

  EXPECT_EQ (name_changes, 1);
  EXPECT_EQ (height_changes, 1);
  EXPECT_EQ (mute_changes, 1);
  EXPECT_EQ (solo_changes, 1);
}

TEST_F (TrackLaneTest, RegionTypeSeparation)
{
  // Verify that MIDI and audio regions are managed separately
  dsp::TempoMap tempo_map{ 44100 };

  // Add MIDI region to MIDI lane
  auto midi_region_ref = obj_registry_->create_object<arrangement::MidiRegion> (
    tempo_map, *obj_registry_, *file_audio_source_registry_);
  midi_lane_->arrangement::ArrangerObjectOwner<
    arrangement::MidiRegion>::add_object (midi_region_ref);

  // Add audio region to audio lane
  auto musical_mode_getter = [] { return false; };
  auto audio_region_ref = obj_registry_->create_object<arrangement::AudioRegion> (
    tempo_map, *obj_registry_, *file_audio_source_registry_,
    musical_mode_getter);
  audio_lane_->arrangement::ArrangerObjectOwner<
    arrangement::AudioRegion>::add_object (audio_region_ref);

  // Verify separation
  EXPECT_EQ (midi_lane_->midiRegions ()->rowCount (), 1);
  EXPECT_EQ (midi_lane_->audioRegions ()->rowCount (), 0);

  EXPECT_EQ (audio_lane_->audioRegions ()->rowCount (), 1);
  EXPECT_EQ (audio_lane_->midiRegions ()->rowCount (), 0);
}

} // namespace zrythm::structure::tracks
