// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/midi_playback_cache.h"
#include "structure/arrangement/midi_note.h"
#include "structure/arrangement/midi_region.h"
#include "structure/arrangement/playback_cache_builder.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace zrythm::structure::arrangement
{

class MockMidiRegionOwner : public QObject, public ArrangerObjectOwner<MidiRegion>
{
public:
  MockMidiRegionOwner (
    ArrangerObjectRegistry       &registry,
    dsp::FileAudioSourceRegistry &file_audio_source_registry)
      : ArrangerObjectOwner (registry, file_audio_source_registry, *this)
  {
  }

  MOCK_METHOD (
    std::string,
    get_field_name_for_serialization,
    (const MidiRegion * obj),
    (const override));
};

class PlaybackCacheBuilderTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    tempo_map = std::make_unique<dsp::TempoMap> (44100.0);
    obj_registry = std::make_unique<arrangement::ArrangerObjectRegistry> ();
    file_audio_source_registry =
      std::make_unique<dsp::FileAudioSourceRegistry> ();

    // Create TrackLane as MIDI region owner
    midi_lane = std::make_unique<MockMidiRegionOwner> (
      *obj_registry, *file_audio_source_registry);

    // Create a MIDI region for testing
    auto midi_region_ref = obj_registry->create_object<arrangement::MidiRegion> (
      *tempo_map, *obj_registry, *file_audio_source_registry);
    midi_region = midi_region_ref.get_object_as<arrangement::MidiRegion> ();
    midi_region->position ()->setTicks (100);
    midi_region->bounds ()->length ()->setTicks (200);

    // Create a MIDI note and add it to the region
    auto note_ref =
      obj_registry->create_object<arrangement::MidiNote> (*tempo_map);
    note_ref.get_object_as<arrangement::MidiNote> ()->setPitch (60);
    note_ref.get_object_as<arrangement::MidiNote> ()->setVelocity (90);
    note_ref.get_object_as<arrangement::MidiNote> ()->position ()->setTicks (50);
    note_ref.get_object_as<arrangement::MidiNote> ()
      ->bounds ()
      ->length ()
      ->setTicks (50);
    midi_region->add_object (note_ref);

    // Add region to lane
    midi_lane->arrangement::ArrangerObjectOwner<
      arrangement::MidiRegion>::add_object (midi_region_ref);
  }

  void TearDown () override
  {
    midi_lane.reset ();
    file_audio_source_registry.reset ();
    obj_registry.reset ();
    tempo_map.reset ();
  }

  std::unique_ptr<dsp::TempoMap>                       tempo_map;
  std::unique_ptr<arrangement::ArrangerObjectRegistry> obj_registry;
  std::unique_ptr<dsp::FileAudioSourceRegistry> file_audio_source_registry;
  std::unique_ptr<MockMidiRegionOwner>          midi_lane;
  arrangement::MidiRegion *                     midi_region{};
};

TEST_F (PlaybackCacheBuilderTest, GenerateCacheNoAffectedRange)
{
  dsp::MidiPlaybackCache cache;

  // Generate cache without affected range
  std::vector<const PlaybackCacheBuilder::MidiRegionOwner *> owners = {
    midi_lane.get ()
  };
  PlaybackCacheBuilder::generate_midi_cache_for_midi_region_collections (
    cache, owners, *tempo_map, {});

  // Should have events from the MIDI region
  const auto &events = cache.cached_events ();
  EXPECT_GT (events.getNumEvents (), 0);

  // Should have note on and note off events
  bool has_note_on = false;
  bool has_note_off = false;
  for (int i = 0; i < events.getNumEvents (); ++i)
    {
      const auto * event = events.getEventPointer (i);
      if (event->message.isNoteOn ())
        {
          has_note_on = true;
          EXPECT_EQ (event->message.getNoteNumber (), 60);
          EXPECT_EQ (event->message.getVelocity (), 90);
        }
      else if (event->message.isNoteOff ())
        {
          has_note_off = true;
          EXPECT_EQ (event->message.getNoteNumber (), 60);
        }
    }

  EXPECT_TRUE (has_note_on);
  EXPECT_TRUE (has_note_off);
}

TEST_F (PlaybackCacheBuilderTest, GenerateCacheWithAffectedRange)
{
  dsp::MidiPlaybackCache cache;

  // Generate cache with affected range that includes our region
  const double affected_start = 50.0;
  const double affected_end = 300.0;
  std::vector<const PlaybackCacheBuilder::MidiRegionOwner *> owners = {
    midi_lane.get ()
  };
  PlaybackCacheBuilder::generate_midi_cache_for_midi_region_collections (
    cache, owners, *tempo_map,
    { std::make_pair (affected_start, affected_end) });

  // Should have events from the MIDI region
  const auto &events = cache.cached_events ();
  EXPECT_GT (events.getNumEvents (), 0);

  // Convert affected range to samples for verification
  const auto sample_start = tempo_map->tick_to_samples_rounded (affected_start);
  const auto sample_end = tempo_map->tick_to_samples_rounded (affected_end);

  // All events should be within the sample interval
  for (int i = 0; i < events.getNumEvents (); ++i)
    {
      const auto * event = events.getEventPointer (i);
      const double event_time = event->message.getTimeStamp ();
      EXPECT_GE (event_time, static_cast<double> (sample_start));
      EXPECT_LE (event_time, static_cast<double> (sample_end));
    }
}

TEST_F (PlaybackCacheBuilderTest, GenerateCacheOutsideAffectedRange)
{
  dsp::MidiPlaybackCache cache;

  // Generate cache with affected range that doesn't include our region
  const double affected_start = 500.0;
  const double affected_end = 600.0;
  std::vector<const PlaybackCacheBuilder::MidiRegionOwner *> owners = {
    midi_lane.get ()
  };
  PlaybackCacheBuilder::generate_midi_cache_for_midi_region_collections (
    cache, owners, *tempo_map,
    { std::make_pair (affected_start, affected_end) });

  // Should have no events since region is outside affected range
  const auto &events = cache.cached_events ();
  EXPECT_EQ (events.getNumEvents (), 0);
}

TEST_F (PlaybackCacheBuilderTest, GenerateCachePartialOverlap)
{
  dsp::MidiPlaybackCache cache;

  // Create a second TrackLane with a MIDI region that partially overlaps with
  // affected range
  auto second_lane = std::make_unique<MockMidiRegionOwner> (
    *obj_registry, *file_audio_source_registry);

  // Create a second MIDI region
  auto second_region_ref = obj_registry->create_object<arrangement::MidiRegion> (
    *tempo_map, *obj_registry, *file_audio_source_registry);
  auto second_region =
    second_region_ref.get_object_as<arrangement::MidiRegion> ();
  second_region->position ()->setTicks (400);
  second_region->bounds ()->length ()->setTicks (100);

  // Add note to second region
  auto note_ref =
    obj_registry->create_object<arrangement::MidiNote> (*tempo_map);
  note_ref.get_object_as<arrangement::MidiNote> ()->setPitch (64);
  note_ref.get_object_as<arrangement::MidiNote> ()->setVelocity (80);
  note_ref.get_object_as<arrangement::MidiNote> ()->position ()->setTicks (20);
  note_ref.get_object_as<arrangement::MidiNote> ()
    ->bounds ()
    ->length ()
    ->setTicks (30);
  second_region->add_object (note_ref);

  // Add second region to second lane
  second_lane->arrangement::ArrangerObjectOwner<
    arrangement::MidiRegion>::add_object (second_region_ref);

  // Generate cache with affected range that overlaps with second region
  const double affected_start = 410.0;
  const double affected_end = 450.0;
  std::vector<const PlaybackCacheBuilder::MidiRegionOwner *> owners = {
    midi_lane.get (), second_lane.get ()
  };
  PlaybackCacheBuilder::generate_midi_cache_for_midi_region_collections (
    cache, owners, *tempo_map,
    { std::make_pair (affected_start, affected_end) });

  // Should have events only from the second region
  const auto &events = cache.cached_events ();
  EXPECT_GT (events.getNumEvents (), 0);

  // All events should be from the second region (pitch 64)
  for (int i = 0; i < events.getNumEvents (); ++i)
    {
      const auto * event = events.getEventPointer (i);
      if (event->message.isNoteOn () || event->message.isNoteOff ())
        {
          EXPECT_EQ (event->message.getNoteNumber (), 64);
        }
    }
}

TEST_F (PlaybackCacheBuilderTest, GenerateCacheEmptyCollections)
{
  dsp::MidiPlaybackCache cache;

  // Create empty collections
  std::vector<const PlaybackCacheBuilder::MidiRegionOwner *> empty_owners;

  // Generate cache with empty collections
  PlaybackCacheBuilder::generate_midi_cache_for_midi_region_collections (
    cache, empty_owners, *tempo_map, {});

  // Should have no events
  const auto &events = cache.cached_events ();
  EXPECT_EQ (events.getNumEvents (), 0);
}

TEST_F (PlaybackCacheBuilderTest, GenerateCacheWithMutedNote)
{
  dsp::MidiPlaybackCache cache;

  // Mute the note in our region
  auto note_view = midi_region->get_children_view ();
  if (!note_view.empty ())
    {
      note_view[0]->mute ()->setMuted (true);
    }

  // Generate cache
  std::vector<const PlaybackCacheBuilder::MidiRegionOwner *> owners = {
    midi_lane.get ()
  };
  PlaybackCacheBuilder::generate_midi_cache_for_midi_region_collections (
    cache, owners, *tempo_map, {});

  // Should have no events since the only note is muted
  const auto &events = cache.cached_events ();
  EXPECT_EQ (events.getNumEvents (), 0);
}

TEST_F (PlaybackCacheBuilderTest, GenerateCacheMultipleRegions)
{
  dsp::MidiPlaybackCache cache;

  // Create multiple lanes with different pitches
  std::vector<int>                                  pitches = { 60, 64, 67 };
  std::vector<std::unique_ptr<MockMidiRegionOwner>> additional_lanes;

  for (int i = 0; i < 3; ++i)
    {
      auto lane = std::make_unique<MockMidiRegionOwner> (
        *obj_registry, *file_audio_source_registry);

      auto region_ref = obj_registry->create_object<arrangement::MidiRegion> (
        *tempo_map, *obj_registry, *file_audio_source_registry);
      auto region = region_ref.get_object_as<arrangement::MidiRegion> ();
      region->position ()->setTicks (100 + i * 100);
      region->bounds ()->length ()->setTicks (50);

      auto note_ref =
        obj_registry->create_object<arrangement::MidiNote> (*tempo_map);
      note_ref.get_object_as<arrangement::MidiNote> ()->setPitch (pitches[i]);
      note_ref.get_object_as<arrangement::MidiNote> ()->setVelocity (90);
      note_ref.get_object_as<arrangement::MidiNote> ()->position ()->setTicks (
        25);
      note_ref.get_object_as<arrangement::MidiNote> ()
        ->bounds ()
        ->length ()
        ->setTicks (25);
      region->add_object (note_ref);

      lane->arrangement::ArrangerObjectOwner<
        arrangement::MidiRegion>::add_object (region_ref);

      additional_lanes.push_back (std::move (lane));
    }

  // Generate cache with all lanes
  std::vector<const PlaybackCacheBuilder::MidiRegionOwner *> owners;
  owners.push_back (midi_lane.get ());
  for (const auto &lane : additional_lanes)
    {
      owners.push_back (lane.get ());
    }
  PlaybackCacheBuilder::generate_midi_cache_for_midi_region_collections (
    cache, owners, *tempo_map, {});

  // Should have events from all regions
  const auto &events = cache.cached_events ();
  EXPECT_GT (events.getNumEvents (), 0);

  // Should have all pitches including duplicates
  std::vector<int> found_pitches;
  for (int i = 0; i < events.getNumEvents (); ++i)
    {
      const auto * event = events.getEventPointer (i);
      if (event->message.isNoteOn ())
        {
          found_pitches.push_back (event->message.getNoteNumber ());
        }
    }

  // Should have pitches from all regions (3 additional + 1 original = 4 pitches)
  EXPECT_EQ (found_pitches.size (), 4);

  // Check that all expected pitches are present
  EXPECT_TRUE (std::ranges::contains (found_pitches, 60));
  EXPECT_TRUE (std::ranges::contains (found_pitches, 64));
  EXPECT_TRUE (std::ranges::contains (found_pitches, 67));
}

TEST_F (PlaybackCacheBuilderTest, GenerateCacheWithExistingCache)
{
  dsp::MidiPlaybackCache cache;

  // First generate cache normally
  std::vector<const PlaybackCacheBuilder::MidiRegionOwner *> owners = {
    midi_lane.get ()
  };
  PlaybackCacheBuilder::generate_midi_cache_for_midi_region_collections (
    cache, owners, *tempo_map, {});

  const auto initial_event_count = cache.cached_events ().getNumEvents ();
  EXPECT_GT (initial_event_count, 0);

  // Generate cache again with affected range that should clear and regenerate
  const double affected_start = 50.0;
  const double affected_end = 300.0;
  PlaybackCacheBuilder::generate_midi_cache_for_midi_region_collections (
    cache, owners, *tempo_map,
    { std::make_pair (affected_start, affected_end) });

  // Should still have events
  const auto &events = cache.cached_events ();
  EXPECT_GT (events.getNumEvents (), 0);
  EXPECT_LE (events.getNumEvents (), initial_event_count); // Might be fewer due
                                                           // to range filtering
}

TEST_F (PlaybackCacheBuilderTest, GenerateCacheEdgeCaseZeroLengthRegion)
{
  dsp::MidiPlaybackCache cache;

  // Create a lane with a zero length region
  auto zero_length_lane = std::make_unique<MockMidiRegionOwner> (
    *obj_registry, *file_audio_source_registry);

  // Create a region with zero length
  auto zero_length_region_ref =
    obj_registry->create_object<arrangement::MidiRegion> (
      *tempo_map, *obj_registry, *file_audio_source_registry);
  auto zero_length_region =
    zero_length_region_ref.get_object_as<arrangement::MidiRegion> ();
  zero_length_region->position ()->setTicks (200);
  zero_length_region->bounds ()->length ()->setTicks (0);

  // Add to lane
  zero_length_lane->arrangement::ArrangerObjectOwner<
    arrangement::MidiRegion>::add_object (zero_length_region_ref);

  // Generate cache with both lanes
  std::vector<const PlaybackCacheBuilder::MidiRegionOwner *> owners = {
    midi_lane.get (), zero_length_lane.get ()
  };
  PlaybackCacheBuilder::generate_midi_cache_for_midi_region_collections (
    cache, owners, *tempo_map, {});

  // Should still have events from the original region, not the zero-length one
  const auto &events = cache.cached_events ();
  EXPECT_GT (events.getNumEvents (), 0);

  // All events should be from the original region (pitch 60)
  for (int i = 0; i < events.getNumEvents (); ++i)
    {
      const auto * event = events.getEventPointer (i);
      if (event->message.isNoteOn () || event->message.isNoteOff ())
        {
          EXPECT_EQ (event->message.getNoteNumber (), 60);
        }
    }
}

} // namespace zrythm::structure::tracks
