// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/chord_descriptor.h"
#include "dsp/midi_event.h"
#include "structure/arrangement/arranger_object_factory.h"
#include "structure/tracks/chord_track.h"
#include "structure/tracks/track_processor.h"
#include "utils/midi.h"
#include "utils/object_registry.h"

#include "helpers/scoped_qcoreapplication.h"

#include "unit/dsp/graph_helpers.h"
#include <gtest/gtest.h>

namespace zrythm::structure::tracks
{

class ChordTrackTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    chord_c_major_.setRootNote (dsp::MusicalNote::C);
    chord_c_major_.setChordType (dsp::ChordType::Major);
  }

  dsp::ChordDescriptor chord_c_major_;
};

TEST_F (ChordTrackTest, TransformChordExpandsSingleNoteToChord)
{
  auto src = dsp::MidiEventBuffer::make_reserved ();
  // C3 (root for C major)
  {
    const auto _ev =
      dsp::midi_event::make_note_on (0, 48, 100, units::samples (0));
    src.push_back (_ev.time_, _ev.data ());
  }
  {
    const auto _ev = dsp::midi_event::make_note_off (0, 48, units::samples (10));
    src.push_back (_ev.time_, _ev.data ());
  }

  auto dest = dsp::MidiEventBuffer::make_reserved ();
  auto chord_mapper = [this] (midi_byte_t note)
    -> std::optional<dsp::ChordDescriptor::ChordPitches> {
    if (note == 48)
      return chord_c_major_.getMidiPitches ();
    return std::nullopt;
  };

  ChordTrack::transform_chord_and_append (
    dest, src, chord_mapper, 100,
    std::pair{ units::samples (0u), units::samples (20u) });

  // C major chord should have 3 notes (C, E, G): 3 note-ons + 3 note-offs
  ASSERT_EQ (dest.size (), 6);

  std::vector<midi_byte_t> note_on_pitches;
  std::vector<midi_byte_t> note_off_pitches;
  for (const auto &ev : dest)
    {
      auto d = ev.data ();
      if (utils::midi::midi_is_note_on (d))
        note_on_pitches.push_back (utils::midi::midi_get_note_number (d));
      else if (utils::midi::midi_is_note_off (d))
        note_off_pitches.push_back (utils::midi::midi_get_note_number (d));
    }

  ASSERT_EQ (note_on_pitches.size (), 3u);
  ASSERT_EQ (note_off_pitches.size (), 3u);

  std::ranges::sort (note_on_pitches);
  EXPECT_EQ (note_on_pitches[0], 48);
  EXPECT_EQ (note_on_pitches[1], 52);
  EXPECT_EQ (note_on_pitches[2], 55);

  std::ranges::sort (note_off_pitches);
  EXPECT_EQ (note_off_pitches[0], 48);
  EXPECT_EQ (note_off_pitches[1], 52);
  EXPECT_EQ (note_off_pitches[2], 55);
}

TEST_F (ChordTrackTest, ChordPitchesEquality)
{
  auto a = chord_c_major_.getMidiPitches ();
  auto b = chord_c_major_.getMidiPitches ();
  EXPECT_EQ (a, b);

  dsp::ChordDescriptor chord_d_minor;
  chord_d_minor.setRootNote (dsp::MusicalNote::D);
  chord_d_minor.setChordType (dsp::ChordType::Minor);
  auto c = chord_d_minor.getMidiPitches ();
  EXPECT_NE (a, c);
}

TEST_F (ChordTrackTest, ChordPitchesEmptyNotEqual)
{
  auto                               a = chord_c_major_.getMidiPitches ();
  dsp::ChordDescriptor::ChordPitches empty;
  EXPECT_NE (a, empty);
  EXPECT_NE (empty, a);
  EXPECT_EQ (empty, empty);
}

TEST_F (ChordTrackTest, ChordPitchesSelfEquality)
{
  dsp::ChordDescriptor chord_d_minor;
  chord_d_minor.setRootNote (dsp::MusicalNote::D);
  chord_d_minor.setChordType (dsp::ChordType::Minor);
  auto pitches = chord_d_minor.getMidiPitches ();
  EXPECT_EQ (pitches, pitches);
}

// ========================================================================
// Chord region cache tests
// ========================================================================

class ChordTrackCacheTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    app_ = std::make_unique<test_helpers::ScopedQCoreApplication> ();

    registry_ = std::make_unique<utils::ObjectRegistry> ();
    tempo_map_ = std::make_unique<dsp::TempoMap> (units::sample_rate (44100));
    tempo_map_wrapper_ = std::make_unique<dsp::TempoMapWrapper> (*tempo_map_);
    transport_ = std::make_unique<dsp::graph_test::MockTransport> ();

    final_deps_ = std::make_unique<FinalTrackDependencies> (
      *tempo_map_wrapper_, *registry_, *transport_, [] { return false; },
      TrackRecordingCallback{});

    chord_track_ = std::make_unique<ChordTrack> (*final_deps_);

    sample_rate_provider_ = [] () { return units::sample_rate (44100); };
    bpm_provider_ = [] () { return 120.0; };

    factory_ = std::make_unique<arrangement::ArrangerObjectFactory> (
      arrangement::ArrangerObjectFactory::Dependencies{
        .tempo_map_ = *tempo_map_,
        .registry_ = *registry_,
        .musical_mode_getter_ = [] () { return true; },
        .last_timeline_obj_len_provider_ = [] () { return 100.0; },
        .last_editor_obj_len_provider_ = [] () { return 50.0; },
        .automation_curve_algorithm_provider_ =
          [] () { return dsp::CurveOptions::Algorithm::Exponent; },
      },
      sample_rate_provider_, bpm_provider_);
  }

  std::unique_ptr<test_helpers::ScopedQCoreApplication>  app_;
  std::unique_ptr<utils::ObjectRegistry>                 registry_;
  std::unique_ptr<dsp::TempoMap>                         tempo_map_;
  std::unique_ptr<dsp::TempoMapWrapper>                  tempo_map_wrapper_;
  std::unique_ptr<dsp::graph_test::MockTransport>        transport_;
  std::unique_ptr<FinalTrackDependencies>                final_deps_;
  std::unique_ptr<ChordTrack>                            chord_track_;
  arrangement::ArrangerObjectFactory::SampleRateProvider sample_rate_provider_;
  arrangement::ArrangerObjectFactory::BpmProvider        bpm_provider_;
  std::unique_ptr<arrangement::ArrangerObjectFactory>    factory_;
};

TEST_F (ChordTrackCacheTest, EmptyTrackCacheIsNoOp)
{
  chord_track_->regeneratePlaybackCaches ({});

  const auto &provider =
    chord_track_->get_track_processor ()->timeline_midi_data_provider ();
  const auto events = provider.midi_events ();
  EXPECT_TRUE (events.empty ());
}

TEST_F (ChordTrackCacheTest, ChordRegionPopulatesMidiCache)
{
  auto region_ref =
    factory_->get_builder<arrangement::ChordRegion> ()
      .with_start_ticks (units::ticks (0))
      .with_end_ticks (units::ticks (3840))
      .build_in_registry ();
  chord_track_->arrangement::ArrangerObjectOwner<
    arrangement::ChordRegion>::add_object (region_ref);
  auto * region = region_ref.get_object_as<arrangement::ChordRegion> ();

  auto chord_ref =
    factory_->get_builder<arrangement::ChordObject> ()
      .with_start_ticks (units::ticks (0))
      .with_chord_descriptor (dsp::MusicalNote::C, dsp::ChordType::Major)
      .build_in_registry ();
  region->add_object (chord_ref);

  chord_track_->regeneratePlaybackCaches (
    utils::ExpandableTickRange{ std::make_pair (0.0, 3840.0) });

  const auto &provider =
    chord_track_->get_track_processor ()->timeline_midi_data_provider ();
  const auto events = provider.midi_events ();
  ASSERT_FALSE (events.empty ());

  std::vector<midi_byte_t> note_pitches;
  for (const auto &ev : events)
    {
      if (utils::midi::midi_is_note_on (ev.data ()))
        note_pitches.push_back (utils::midi::midi_get_note_number (ev.data ()));
    }
  std::ranges::sort (note_pitches);

  // C Major at base pitch 48 (C3): C=48, E=52, G=55
  ASSERT_GE (note_pitches.size (), 3u);
  EXPECT_EQ (note_pitches[0], 48);
  EXPECT_EQ (note_pitches[1], 52);
  EXPECT_EQ (note_pitches[2], 55);
}

// ========================================================================
// previewChord tests
// ========================================================================

TEST_F (ChordTrackCacheTest, PreviewChordStartsAndAutoStopsOnTimeout)
{
  dsp::ChordDescriptor c_major;
  c_major.setRootNote (dsp::MusicalNote::C);
  c_major.setChordType (dsp::ChordType::Major);

  const units::sample_rate_t sample_rate{ units::sample_rate (44100) };
  const units::sample_u32_t  block_length{ units::samples (256u) };

  auto * processor = chord_track_->get_track_processor ();
  processor->prepare_for_processing (nullptr, sample_rate, block_length);

  const auto time_nfo = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));

  const auto collect = [] (const auto &buffer, bool want_on) {
    std::vector<midi_byte_t> pitches;
    for (const auto &ev : buffer)
      {
        const bool match =
          want_on
            ? utils::midi::midi_is_note_on (ev.data ())
            : utils::midi::midi_is_note_off (ev.data ());
        if (match)
          pitches.push_back (utils::midi::midi_get_note_number (ev.data ()));
      }
    std::ranges::sort (pitches);
    return pitches;
  };

  // nullptr is a safe no-op.
  chord_track_->previewChord (nullptr);

  // Start a short preview; the next block produces the chord's note-ons.
  chord_track_->previewChord (&c_major, /*duration_ms=*/10);
  processor->process_block (time_nfo, *transport_, *tempo_map_);
  {
    const auto &out = processor->get_midi_out_port ();
    const auto  ons = collect (out.buffer_, true);
    ASSERT_EQ (ons.size (), 3u);
    EXPECT_EQ (ons[0], 48);
    EXPECT_EQ (ons[1], 52);
    EXPECT_EQ (ons[2], 55);
  }

  // Pump the event loop long enough for the C++-owned auto-stop timer to fire.
  test_helpers::ScopedQCoreApplication::process_events_until_timeout (
    std::chrono::milliseconds (60));

  // The next block sends note-offs: the note-off is owned by ChordTrack, so no
  // QML Timer is required.
  processor->process_block (time_nfo, *transport_, *tempo_map_);
  {
    const auto &out = processor->get_midi_out_port ();
    EXPECT_TRUE (collect (out.buffer_, true).empty ());
    const auto offs = collect (out.buffer_, false);
    ASSERT_EQ (offs.size (), 3u);
    EXPECT_EQ (offs[0], 48);
    EXPECT_EQ (offs[1], 52);
    EXPECT_EQ (offs[2], 55);
  }
}

TEST_F (ChordTrackCacheTest, PreviewChordRetriggerStopsPrevious)
{
  dsp::ChordDescriptor c_major;
  c_major.setRootNote (dsp::MusicalNote::C);
  c_major.setChordType (dsp::ChordType::Major);

  dsp::ChordDescriptor d_minor;
  d_minor.setRootNote (dsp::MusicalNote::D);
  d_minor.setChordType (dsp::ChordType::Minor);

  const units::sample_rate_t sample_rate{ units::sample_rate (44100) };
  const units::sample_u32_t  block_length{ units::samples (256u) };

  auto * processor = chord_track_->get_track_processor ();
  processor->prepare_for_processing (nullptr, sample_rate, block_length);

  const auto time_nfo = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (256));

  const auto note_ons = [] (const auto &buffer) {
    std::vector<midi_byte_t> pitches;
    for (const auto &ev : buffer)
      if (utils::midi::midi_is_note_on (ev.data ()))
        pitches.push_back (utils::midi::midi_get_note_number (ev.data ()));
    std::ranges::sort (pitches);
    return pitches;
  };

  // Long duration so the timer does not fire during the test; retriggering must
  // stop the previous preview first.
  chord_track_->previewChord (&c_major, /*duration_ms=*/10000);
  chord_track_->previewChord (&d_minor, /*duration_ms=*/10000);

  processor->process_block (time_nfo, *transport_, *tempo_map_);
  const auto &out = processor->get_midi_out_port ();
  const auto  ons = note_ons (out.buffer_);
  // Only D minor is active: D=50, F=53, A=57.
  ASSERT_EQ (ons.size (), 3u);
  EXPECT_EQ (ons[0], 50);
  EXPECT_EQ (ons[1], 53);
  EXPECT_EQ (ons[2], 57);
}

} // namespace zrythm::structure::tracks
