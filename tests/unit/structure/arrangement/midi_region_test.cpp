// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/midi_event.h"
#include "structure/arrangement/arranger_object_all.h"

#include <gtest/gtest.h>

namespace zrythm::structure::arrangement
{
class MidiRegionTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    tempo_map = std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));
    region = std::make_unique<MidiRegion> (
      *tempo_map, registry, file_audio_source_registry, nullptr);

    // Set up region properties
    region->position ()->setTicks (100);
    region->bounds ()->length ()->setTicks (200);
    region->loopRange ()->loopStartPosition ()->setTicks (50);
    region->loopRange ()->loopEndPosition ()->setTicks (150);
    region->loopRange ()->clipStartPosition ()->setTicks (0);
  }

  void add_midi_note (
    int    pitch,
    int    velocity,
    double position_ticks,
    double length_ticks)
  {
    // Create MidiNote using registry
    auto note_ref = registry.create_object<MidiNote> (*tempo_map);
    note_ref.get_object_as<MidiNote> ()->setPitch (pitch);
    note_ref.get_object_as<MidiNote> ()->setVelocity (velocity);
    note_ref.get_object_as<MidiNote> ()->position ()->setTicks (position_ticks);
    note_ref.get_object_as<MidiNote> ()->bounds ()->length ()->setTicks (
      length_ticks);

    // Add to region
    region->add_object (note_ref);
  }

  std::unique_ptr<dsp::TempoMap> tempo_map;
  ArrangerObjectRegistry         registry;
  dsp::FileAudioSourceRegistry   file_audio_source_registry;
  std::unique_ptr<MidiRegion>    region;
};

TEST_F (MidiRegionTest, InitialState)
{
  EXPECT_EQ (region->type (), ArrangerObject::Type::MidiRegion);
  EXPECT_EQ (region->position ()->ticks (), 100);
  EXPECT_NE (region->bounds (), nullptr);
  EXPECT_NE (region->loopRange (), nullptr);
  EXPECT_NE (region->name (), nullptr);
  EXPECT_NE (region->color (), nullptr);
  EXPECT_NE (region->mute (), nullptr);
  EXPECT_EQ (region->get_children_vector ().size (), 0);
}

TEST_F (MidiRegionTest, Serialization)
{
  // Add a note
  add_midi_note (60, 90, 100, 50);
  const auto midi_note_id = region->get_children_view ()[0]->get_uuid ();

  // Serialize
  nlohmann::json j;
  to_json (j, *region);

  // Create new region
  auto new_region = std::make_unique<MidiRegion> (
    *tempo_map, registry, file_audio_source_registry, nullptr);
  from_json (j, *new_region);

  // Verify deserialization
  EXPECT_EQ (new_region->get_children_vector ().size (), 1);
  auto note = new_region->get_children_view ()[0];
  EXPECT_EQ (note->pitch (), 60);
  EXPECT_EQ (note->velocity (), 90);
  EXPECT_EQ (midi_note_id, note->get_uuid ());
}

} // namespace zrythm::structure::arrangement
