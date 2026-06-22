// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/midi_event.h"
#include "structure/arrangement/arranger_object_all.h"
#include "utils/object_registry.h"

#include <gtest/gtest.h>

namespace zrythm::structure::arrangement
{
class MidiRegionTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    tempo_map = std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));
    tempo_map_wrapper = std::make_unique<dsp::TempoMapWrapper> (*tempo_map);
    region =
      std::make_unique<MidiRegion> (*tempo_map_wrapper, registry, nullptr);

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
    auto note_ref =
      utils::create_object<MidiNote> (registry, *tempo_map_wrapper);
    note_ref.get_object_as<MidiNote> ()->setPitch (pitch);
    note_ref.get_object_as<MidiNote> ()->setVelocity (velocity);
    note_ref.get_object_as<MidiNote> ()->position ()->setTicks (position_ticks);
    note_ref.get_object_as<MidiNote> ()->bounds ()->length ()->setTicks (
      length_ticks);

    // Add to region
    region->ArrangerObjectOwner<MidiNote>::add_object (note_ref);
  }

  void add_midi_control_event (
    MidiControlEvent::EventType type,
    int                         channel,
    int                         controller,
    int                         value,
    double                      position_ticks)
  {
    auto ev_ref =
      utils::create_object<MidiControlEvent> (registry, *tempo_map_wrapper);
    auto * ev = ev_ref.get_object_as<MidiControlEvent> ();
    ev->setControlType (type);
    ev->setChannel (channel);
    ev->setController (controller);
    ev->setValue (value);
    ev->position ()->setTicks (position_ticks);
    region->ArrangerObjectOwner<MidiControlEvent>::add_object (ev_ref);
  }

  std::unique_ptr<dsp::TempoMap>        tempo_map;
  std::unique_ptr<dsp::TempoMapWrapper> tempo_map_wrapper;
  utils::ObjectRegistry                 registry;
  std::unique_ptr<MidiRegion>           region;
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
  EXPECT_EQ (
    region->ArrangerObjectOwner<MidiNote>::get_children_vector ().size (), 0);
}

TEST_F (MidiRegionTest, Serialization)
{
  // Add a note
  add_midi_note (60, 90, 100, 50);
  const auto midi_note_id =
    region->ArrangerObjectOwner<MidiNote>::get_children_view ()[0]->get_uuid ();

  // Serialize
  nlohmann::json j;
  to_json (j, *region);

  // Create new region
  auto new_region =
    std::make_unique<MidiRegion> (*tempo_map_wrapper, registry, nullptr);
  from_json (j, *new_region);

  // Verify deserialization
  EXPECT_EQ (
    new_region->ArrangerObjectOwner<MidiNote>::get_children_vector ().size (),
    1);
  auto note = new_region->ArrangerObjectOwner<MidiNote>::get_children_view ()[0];
  EXPECT_EQ (note->pitch (), 60);
  EXPECT_EQ (note->velocity (), 90);
  EXPECT_EQ (midi_note_id, note->get_uuid ());
}

TEST_F (MidiRegionTest, SerializationWithControlEvents)
{
  add_midi_control_event (
    MidiControlEvent::EventType::ControlChange, 0, 74, 100, 120);

  const auto control_event_id =
    region->ArrangerObjectOwner<MidiControlEvent>::get_children_view ()[0]
      ->get_uuid ();

  nlohmann::json j;
  to_json (j, *region);

  auto new_region =
    std::make_unique<MidiRegion> (*tempo_map_wrapper, registry, nullptr);
  from_json (j, *new_region);

  EXPECT_EQ (
    new_region->ArrangerObjectOwner<MidiControlEvent>::get_children_vector ()
      .size (),
    1);
  auto * ev =
    new_region->ArrangerObjectOwner<MidiControlEvent>::get_children_view ()[0];
  EXPECT_EQ (ev->controlType (), MidiControlEvent::EventType::ControlChange);
  EXPECT_EQ (ev->midiChannel (), 0);
  EXPECT_EQ (ev->midiController (), 74);
  EXPECT_EQ (ev->midiValue (), 100);
  EXPECT_EQ (control_event_id, ev->get_uuid ());
}

TEST_F (MidiRegionTest, SerializationWithBothNotesAndControlEvents)
{
  add_midi_note (60, 90, 100, 50);
  add_midi_control_event (
    MidiControlEvent::EventType::PitchBend, 1, 0, 8192, 110);

  const auto note_id =
    region->ArrangerObjectOwner<MidiNote>::get_children_view ()[0]->get_uuid ();
  const auto control_id =
    region->ArrangerObjectOwner<MidiControlEvent>::get_children_view ()[0]
      ->get_uuid ();

  nlohmann::json j;
  to_json (j, *region);

  auto new_region =
    std::make_unique<MidiRegion> (*tempo_map_wrapper, registry, nullptr);
  from_json (j, *new_region);

  EXPECT_EQ (
    new_region->ArrangerObjectOwner<MidiNote>::get_children_vector ().size (),
    1);
  EXPECT_EQ (
    new_region->ArrangerObjectOwner<MidiControlEvent>::get_children_vector ()
      .size (),
    1);

  auto * note =
    new_region->ArrangerObjectOwner<MidiNote>::get_children_view ()[0];
  EXPECT_EQ (note->pitch (), 60);
  EXPECT_EQ (note_id, note->get_uuid ());

  auto * ev =
    new_region->ArrangerObjectOwner<MidiControlEvent>::get_children_view ()[0];
  EXPECT_EQ (ev->controlType (), MidiControlEvent::EventType::PitchBend);
  EXPECT_EQ (ev->midiChannel (), 1);
  EXPECT_EQ (ev->midiValue (), 8192);
  EXPECT_EQ (control_id, ev->get_uuid ());
}

} // namespace zrythm::structure::arrangement
