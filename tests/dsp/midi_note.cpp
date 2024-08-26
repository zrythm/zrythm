// SPDX-FileCopyrightText: © 2019, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "dsp/midi_note.h"
#include "dsp/region.h"
#include "project.h"
#include "utils/flags.h"
#include "zrythm.h"

#include "helpers/project_helper.h"
#include "helpers/zrythm_helper.h"

TEST_SUITE_BEGIN ("dsp/midi note");

#if 0
typedef struct
{
  Region * region;
} MidiNoteFixture;

static void
fixture_set_up (
  MidiNoteFixture * fixture)
{
  engine_update_frames_per_tick (
    AUDIO_ENGINE, 4, 140, 44000);

  Position start_pos, end_pos;
  start_pos.set_to_bar (2);
  end_pos.set_to_bar (6);
  fixture->region =
    midi_region_new (
      &start_pos, &end_pos, 0, 0, 0);
}

static void
test_new_midi_note ()
{
  MidiNoteFixture _fixture;
  MidiNoteFixture * fixture =
    &_fixture;
  fixture_set_up (fixture);

  int val = 23, vel = 90;
  Position start_pos, end_pos;
  start_pos.set_to_bar (2);
  end_pos.set_to_bar (3);
  MidiNote * mn =
    midi_note_new (
      &fixture->region->id,
      &start_pos, &end_pos, val,
      VELOCITY_DEFAULT);
  ArrangerObject * mn_obj =
    (ArrangerObject *) mn;

  REQUIRE_NONNULL (mn);
  REQUIRE (
    position_is_equal (
      &start_pos, &mn_obj->pos));
  REQUIRE (
    position_is_equal (
      &end_pos, &mn_obj->end_pos_));

  REQUIRE_EQ (mn->vel->vel, vel);
  REQUIRE_EQ (mn->val, val);
  REQUIRE_FALSE (mn->muted);

  Region * region =
    arranger_object_get_region (
      (ArrangerObject *) mn);
  Region * r_clone =
    (Region *)
    arranger_object_clone (
      (ArrangerObject *) region,
      ARRANGER_OBJECT_CLONE_COPY);
  midi_note_set_region_and_index (mn, r_clone, 0);

  g_assert_cmpint (
    region_identifier_is_equal (
      &mn_obj->region_id, &r_clone->id), ==, 1);

  MidiNote * mn_clone =
    (MidiNote *)
    arranger_object_clone (
      (ArrangerObject *) mn,
      ARRANGER_OBJECT_CLONE_COPY);
  g_assert_cmpint (
    midi_note_is_equal (mn, mn_clone), ==, 1);
}
#endif

TEST_CASE_FIXTURE (
  BootstrapTimelineFixture,
  "midi note has correct track name hash after addding to region")
{
  auto midi_track = TRACKLIST->get_track_by_type<MidiTrack> (Track::Type::Midi);
  REQUIRE_NE (midi_track->name_hash_, 0);
  auto mr = std::make_shared<MidiRegion> (
    p1_, p2_, midi_track->name_hash_, MIDI_REGION_LANE, 0);
  midi_track->add_region (mr, nullptr, MIDI_REGION_LANE, true, false);
  auto mn = std::make_shared<MidiNote> (mr->id_, p1_, p2_, MN_VAL, MN_VEL);
  mr->append_object (mn);
  REQUIRE_EQ (mn->track_name_hash_, midi_track->name_hash_);
  REQUIRE_EQ (mn->vel_->track_name_hash_, midi_track->name_hash_);
}

TEST_SUITE_END;