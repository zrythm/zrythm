// SPDX-FileCopyrightText: © 2020-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "gui/cpp/backend/actions/arranger_selections.h"
#include "gui/cpp/backend/actions/range_action.h"
#include "gui/cpp/backend/actions/undo_manager.h"
#include "gui/cpp/backend/project.h"
#include "gui/cpp/backend/zrythm.h"

#include "tests/helpers/project_helper.h"

#include "common/dsp/region.h"

constexpr auto RANGE_START_BAR = 4;
constexpr auto RANGE_END_BAR = 10;
constexpr auto RANGE_SIZE_IN_BARS = RANGE_END_BAR - RANGE_START_BAR; /* 6 */

/* cue is before the range */
constexpr auto CUE_BEFORE = RANGE_START_BAR - 1;

/* playhead is after the range */
constexpr auto PLAYHEAD_BEFORE = RANGE_END_BAR + 2;
constexpr auto LOOP_START_BEFORE = 1;

/* loop end is inside the range */
constexpr auto LOOP_END_BEFORE = RANGE_START_BAR + 1;

/* audio region starts before the range start and
 * ends in the middle of the range */
constexpr auto AUDIO_REGION_START_BAR = RANGE_START_BAR - 1;
constexpr auto AUDIO_REGION_END_BAR = RANGE_START_BAR + 1;

/* midi region starting after the range end */
constexpr auto MIDI_REGION_START_BAR = RANGE_END_BAR + 2;
constexpr auto MIDI_REGION_END_BAR = MIDI_REGION_START_BAR + 2;

/* midi region starting even further after range
 * end */
constexpr auto MIDI_REGION2_START_BAR = MIDI_REGION_START_BAR + 10;
constexpr auto MIDI_REGION2_END_BAR = MIDI_REGION2_START_BAR + 8;

/* midi region starting before the range and ending
 * inside the range */
constexpr auto MIDI_REGION3_START_BAR = RANGE_START_BAR - 1;
constexpr auto MIDI_REGION3_END_BAR = RANGE_START_BAR + 2;

/* another midi region starting before the range and
 * ending inside the range */
constexpr auto MIDI_REGION4_START_BAR = RANGE_START_BAR - 2;
constexpr auto MIDI_REGION4_END_BAR = RANGE_START_BAR + 1;

/* midi region starting before the range and ending
 * after the range */
constexpr auto MIDI_REGION5_START_BAR = RANGE_START_BAR - 1;
constexpr auto MIDI_REGION5_END_BAR = RANGE_END_BAR + 1;

/* midi region starting inside the range and ending
 * after the range */
constexpr auto MIDI_REGION6_START_BAR = RANGE_START_BAR + 1;
constexpr auto MIDI_REGION6_END_BAR = RANGE_END_BAR + 1;

/* midi region ending before the range start */
constexpr auto MIDI_REGION7_START_BAR = 1;
constexpr auto MIDI_REGION7_END_BAR = 2;

static int midi_track_pos = -1;
static int audio_track_pos = -1;

static auto perform_create_arranger_sel = [] (const auto &selections) {
  UNDO_MANAGER->perform (
    std::make_unique<CreateArrangerSelectionsAction> (*selections));
};

class RangeActionTestFixture : public ZrythmFixture
{
public:
  void SetUp () override
  {
    ZrythmFixture::SetUp ();

    /* create MIDI track with region */
    auto midi_track = Track::create_empty_with_action<MidiTrack> ();
    midi_track_pos = midi_track->pos_;

    auto add_midi_region = [&] (const auto start_bar, const auto end_bar) {
      Position start, end;
      start.set_to_bar (start_bar);
      end.set_to_bar (end_bar);
      auto midi_region = std::make_shared<MidiRegion> (
        start, end, midi_track->get_name_hash (), 0,
        midi_track->lanes_[0]->regions_.size ());
      ASSERT_NO_THROW (
        midi_track->add_region (midi_region, nullptr, 0, true, false));
      ASSERT_GE (midi_region->id_.idx_, 0);
      midi_region->select (true, false, false);
      perform_create_arranger_sel (TL_SELECTIONS);
    };

    /* add all the midi regions */
    add_midi_region (MIDI_REGION_START_BAR, MIDI_REGION_END_BAR);
    add_midi_region (MIDI_REGION2_START_BAR, MIDI_REGION2_END_BAR);
    add_midi_region (MIDI_REGION3_START_BAR, MIDI_REGION3_END_BAR);
    add_midi_region (MIDI_REGION4_START_BAR, MIDI_REGION4_END_BAR);
    add_midi_region (MIDI_REGION5_START_BAR, MIDI_REGION5_END_BAR);
    add_midi_region (MIDI_REGION6_START_BAR, MIDI_REGION6_END_BAR);
    add_midi_region (MIDI_REGION7_START_BAR, MIDI_REGION7_END_BAR);

    /* create audio track with region */
    FileDescriptor file (fs::path (TESTS_SRCDIR) / "test.wav");
    Position       start, end;
    start.set_to_bar (AUDIO_REGION_START_BAR);
    end.set_to_bar (AUDIO_REGION_END_BAR);
    Track::create_with_action (
      Track::Type::Audio, nullptr, &file, &start, TRACKLIST->get_num_tracks (),
      1, -1, nullptr);
    auto audio_track = dynamic_cast<AudioTrack *> (
      TRACKLIST->get_last_track (Tracklist::PinOption::Both, false));
    audio_track_pos = audio_track->pos_;
    const auto &audio_region = audio_track->lanes_[0]->regions_[0];

    /* resize audio region */
    audio_region->select (true, false, false);
    auto   sel_before = TL_SELECTIONS->clone_unique ();
    double audio_region_size_ticks = audio_region->get_length_in_ticks ();
    double missing_ticks = (end.ticks_ - start.ticks_) - audio_region_size_ticks;
    ASSERT_NO_THROW (audio_region->resize (
      false, ArrangerObject::ResizeType::Loop, missing_ticks, false));
    UNDO_MANAGER->perform (
      std::make_unique<ArrangerSelectionsAction::ResizeAction> (
        *sel_before, TL_SELECTIONS.get (),
        ArrangerSelectionsAction::ResizeType::RLoop, missing_ticks));
    ASSERT_POSITION_EQ (end, audio_region->end_pos_);

    /* set transport positions */
    TRANSPORT->cue_pos_.set_to_bar (CUE_BEFORE);
    TRANSPORT->playhead_pos_.set_to_bar (PLAYHEAD_BEFORE);
    TRANSPORT->loop_start_pos_.set_to_bar (LOOP_START_BEFORE);
    TRANSPORT->loop_end_pos_.set_to_bar (LOOP_END_BEFORE);
  }

  void TearDown () override { ZrythmFixture::TearDown (); }
};

static void
check_start_end_markers ()
{
  /* check that start/end markers exist */
  const auto &start_m = P_MARKER_TRACK->get_start_marker ();
  ASSERT_TRUE (start_m->is_start ());
  ASSERT_NONNULL (start_m);
  auto end_m = P_MARKER_TRACK->get_end_marker ();
  ASSERT_TRUE (end_m->is_end ());
  ASSERT_NONNULL (end_m);

  /* check positions are valid */
  Position init_pos;
  ASSERT_GE (start_m->pos_, init_pos);
  ASSERT_GE (end_m->pos_, init_pos);
}

static void
check_before_insert ()
{
  auto midi_track = TRACKLIST->get_track<MidiTrack> (midi_track_pos);
  ASSERT_SIZE_EQ (midi_track->lanes_[0]->regions_, 7);

#define GET_MIDI_REGION(name, idx) \
  const auto &name = midi_track->lanes_[0]->regions_[idx]

  GET_MIDI_REGION (midi_region, 0);
  GET_MIDI_REGION (midi_region2, 1);
  GET_MIDI_REGION (midi_region3, 2);
  GET_MIDI_REGION (midi_region4, 3);
  GET_MIDI_REGION (midi_region5, 4);
  GET_MIDI_REGION (midi_region6, 5);
  GET_MIDI_REGION (midi_region7, 6);

#undef GET_MIDI_REGION

  auto        audio_track = TRACKLIST->get_track<AudioTrack> (audio_track_pos);
  const auto &audio_region = audio_track->lanes_[0]->regions_[0];
  ASSERT_SIZE_EQ (audio_track->lanes_[0]->regions_, 1);

  auto check_pos = [&] (auto &obj, const auto &start_bar, const auto &end_bar) {
    Position start, end;
    start.set_to_bar (start_bar);
    end.set_to_bar (end_bar);
    ASSERT_POSITION_EQ (obj->pos_, start);
    ASSERT_POSITION_EQ (obj->end_pos_, end);
  };

  check_pos (midi_region, MIDI_REGION_START_BAR, MIDI_REGION_END_BAR);
  check_pos (midi_region2, MIDI_REGION2_START_BAR, MIDI_REGION2_END_BAR);
  check_pos (midi_region3, MIDI_REGION3_START_BAR, MIDI_REGION3_END_BAR);
  check_pos (midi_region4, MIDI_REGION4_START_BAR, MIDI_REGION4_END_BAR);
  check_pos (midi_region5, MIDI_REGION5_START_BAR, MIDI_REGION5_END_BAR);
  check_pos (midi_region6, MIDI_REGION6_START_BAR, MIDI_REGION6_END_BAR);
  check_pos (midi_region7, MIDI_REGION7_START_BAR, MIDI_REGION7_END_BAR);
  check_pos (audio_region, AUDIO_REGION_START_BAR, AUDIO_REGION_END_BAR);

  check_start_end_markers ();
}

static void
check_after_insert ()
{

#define GET_MIDI_REGION(name, idx) \
  const auto &name = midi_track->lanes_[0]->regions_[idx];

  /* get objects */
  auto midi_track = TRACKLIST->get_track<MidiTrack> (midi_track_pos);
  ASSERT_SIZE_EQ (midi_track->lanes_[0]->regions_, 10);
  GET_MIDI_REGION (midi_region1, 0);
  GET_MIDI_REGION (midi_region2, 1);
  GET_MIDI_REGION (midi_region3, 2);
  GET_MIDI_REGION (midi_region4, 3);
  GET_MIDI_REGION (midi_region5, 4);
  GET_MIDI_REGION (midi_region6, 5);
  GET_MIDI_REGION (midi_region7, 6);
  GET_MIDI_REGION (midi_region8, 7);
  GET_MIDI_REGION (midi_region9, 8);
  GET_MIDI_REGION (midi_region10, 9);
#undef GET_MIDI_REGION

  /* get new audio regions */
  auto audio_track = TRACKLIST->get_track<AudioTrack> (audio_track_pos);
  ASSERT_SIZE_EQ (audio_track->lanes_[0]->regions_, 2);
  const auto &audio_region1 = audio_track->lanes_[0]->regions_[0];
  const auto &audio_region2 = audio_track->lanes_[0]->regions_[1];

  auto check_position =
    [&] (const auto &obj, const auto start_bar, const auto end_bar) {
      Position start_expected, end_expected;
      start_expected.set_to_bar (start_bar);
      end_expected.set_to_bar (end_bar);
      ASSERT_POSITION_EQ (obj->pos_, start_expected);
      ASSERT_POSITION_EQ (obj->end_pos_, end_expected);
    };

  /* check midi region positions */
  Position start_expected, end_expected;
  check_position (
    midi_region1, MIDI_REGION_START_BAR + RANGE_SIZE_IN_BARS,
    MIDI_REGION_END_BAR + RANGE_SIZE_IN_BARS);
  check_position (
    midi_region2, MIDI_REGION2_START_BAR + RANGE_SIZE_IN_BARS,
    MIDI_REGION2_END_BAR + RANGE_SIZE_IN_BARS);

  /* orig 3 */
  check_position (midi_region9, MIDI_REGION3_START_BAR, RANGE_START_BAR);
  check_position (
    midi_region10, RANGE_END_BAR, MIDI_REGION3_END_BAR + RANGE_SIZE_IN_BARS);

  /* orig 4 */
  check_position (midi_region7, MIDI_REGION4_START_BAR, RANGE_START_BAR);
  check_position (
    midi_region8, RANGE_END_BAR, MIDI_REGION4_END_BAR + RANGE_SIZE_IN_BARS);

  /* orig 5 */
  check_position (midi_region5, MIDI_REGION5_START_BAR, RANGE_START_BAR);
  check_position (
    midi_region6, RANGE_END_BAR, MIDI_REGION5_END_BAR + RANGE_SIZE_IN_BARS);

  /* orig 6 */
  check_position (
    midi_region3, MIDI_REGION6_START_BAR + RANGE_SIZE_IN_BARS,
    MIDI_REGION6_END_BAR + RANGE_SIZE_IN_BARS);

  /* orig 7 */
  check_position (midi_region4, MIDI_REGION7_START_BAR, MIDI_REGION7_END_BAR);

  /* check audio region positions */
  Position audio_region1_end_after_expected, audio_region2_start_after_expected,
    audio_region2_end_after_expected;
  audio_region1_end_after_expected.set_to_bar (RANGE_START_BAR);
  audio_region2_start_after_expected.set_to_bar (RANGE_END_BAR);
  audio_region2_end_after_expected.set_to_bar (
    AUDIO_REGION_END_BAR + RANGE_SIZE_IN_BARS);
  ASSERT_POSITION_EQ (audio_region1->end_pos_, audio_region1_end_after_expected);
  ASSERT_POSITION_EQ (audio_region2->pos_, audio_region2_start_after_expected);
  ASSERT_POSITION_EQ (audio_region2->end_pos_, audio_region2_end_after_expected);

#define CHECK_TPOS(name, expected_bar) \
  start_expected.set_to_bar (expected_bar); \
  ASSERT_POSITION_EQ (TRANSPORT->name, start_expected);

  /* check transport positions */
  CHECK_TPOS (playhead_pos_, PLAYHEAD_BEFORE + RANGE_SIZE_IN_BARS);
  CHECK_TPOS (loop_end_pos_, LOOP_END_BEFORE + RANGE_SIZE_IN_BARS);
  CHECK_TPOS (cue_pos_, CUE_BEFORE);

#undef CHECK_TPOS

  check_start_end_markers ();
}

TEST_F (RangeActionTestFixture, InsertSilence)
{
  /* create insert silence action */
  Position start, end;
  start.set_to_bar (RANGE_START_BAR);
  end.set_to_bar (RANGE_END_BAR);
  auto ra = std::make_unique<RangeInsertSilenceAction> (start, end);

  /* verify that number of objects is as expected */
  ASSERT_EQ (ra->sel_before_->get_num_objects (), 7);

  check_before_insert ();

  /* perform action */
  UNDO_MANAGER->perform (std::move (ra));

  check_after_insert ();

  /* undo and verify things are back to previous state */
  UNDO_MANAGER->undo ();

  check_before_insert ();

  UNDO_MANAGER->redo ();

  check_after_insert ();
}

static void
check_after_remove ()
{
#define GET_MIDI_REGION(name, idx) \
  const auto &name = midi_track->lanes_[0]->regions_[idx]

  test_project_save_and_reload ();

  /* get objects */
  auto midi_track = TRACKLIST->get_track<MidiTrack> (midi_track_pos);
  ASSERT_SIZE_EQ (midi_track->lanes_[0]->regions_, 8);
  GET_MIDI_REGION (midi_region1, 0);
  GET_MIDI_REGION (midi_region2, 1);
  GET_MIDI_REGION (midi_region3, 2);
  GET_MIDI_REGION (midi_region4, 3);
  GET_MIDI_REGION (midi_region5, 4);
  GET_MIDI_REGION (midi_region6, 5);
  GET_MIDI_REGION (midi_region7, 6);
  GET_MIDI_REGION (midi_region8, 7);

  /* get new audio regions */
  auto audio_track = TRACKLIST->get_track<AudioTrack> (audio_track_pos);
  ASSERT_SIZE_EQ (audio_track->lanes_[0]->regions_, 1);
  const auto &audio_region = audio_track->lanes_[0]->regions_[0];

  auto CHECK_POS =
    [&] (const auto &obj, const auto start_bar, const auto end_bar) {
      Position start_expected, end_expected;
      start_expected.set_to_bar (start_bar);
      end_expected.set_to_bar (end_bar);
      ASSERT_POSITION_EQ (obj->pos_, start_expected);
      ASSERT_POSITION_EQ (obj->end_pos_, end_expected);
    };

  /* check midi region positions */
  Position start_expected, end_expected;

  /* orig 1 */
  CHECK_POS (
    midi_region1, MIDI_REGION_START_BAR - RANGE_SIZE_IN_BARS,
    MIDI_REGION_END_BAR - RANGE_SIZE_IN_BARS);

  /* orig 2 */
  CHECK_POS (
    midi_region2, MIDI_REGION2_START_BAR - RANGE_SIZE_IN_BARS,
    MIDI_REGION2_END_BAR - RANGE_SIZE_IN_BARS);

  /* orig 3 */
  CHECK_POS (midi_region8, MIDI_REGION3_START_BAR, RANGE_START_BAR);

  /* orig 4 */
  CHECK_POS (midi_region7, MIDI_REGION4_START_BAR, RANGE_START_BAR);

  /* orig 5 */
  CHECK_POS (midi_region5, MIDI_REGION5_START_BAR, RANGE_START_BAR);
  CHECK_POS (
    midi_region6, RANGE_START_BAR, MIDI_REGION5_END_BAR - RANGE_SIZE_IN_BARS);

  /* orig 6 */
  CHECK_POS (
    midi_region4, RANGE_START_BAR, MIDI_REGION6_END_BAR - RANGE_SIZE_IN_BARS);

  /* orig 7 */
  CHECK_POS (midi_region3, MIDI_REGION7_START_BAR, MIDI_REGION7_END_BAR);

  /* check audio region positions */
  Position audio_region_start_after_expected, audio_region_end_after_expected;
  audio_region_start_after_expected.set_to_bar (AUDIO_REGION_START_BAR);
  audio_region_end_after_expected.set_to_bar (RANGE_START_BAR);
  ASSERT_POSITION_EQ (audio_region->pos_, audio_region_start_after_expected);
  ASSERT_POSITION_EQ (audio_region->end_pos_, audio_region_end_after_expected);

#define CHECK_TPOS(name, expected_bar) \
  start_expected.set_to_bar (expected_bar); \
  ASSERT_POSITION_EQ (TRANSPORT->name, start_expected);

  /* check transport positions */
  CHECK_TPOS (playhead_pos_, PLAYHEAD_BEFORE - RANGE_SIZE_IN_BARS);
  CHECK_TPOS (loop_end_pos_, RANGE_START_BAR);
  CHECK_TPOS (cue_pos_, CUE_BEFORE);

#undef CHECK_TPOS

  check_start_end_markers ();
}

TEST_F (RangeActionTestFixture, RemoveRange)
{
  /* create remove range action */
  Position start, end;
  start.set_to_bar (RANGE_START_BAR);
  end.set_to_bar (RANGE_END_BAR);

  check_before_insert ();

  UNDO_MANAGER->perform (std::make_unique<RangeRemoveAction> (start, end));

  check_after_remove ();

  /* undo and verify things are back to previous state */
  UNDO_MANAGER->undo ();

  check_before_insert ();

  UNDO_MANAGER->redo ();

  check_after_remove ();
}

TEST_F (ZrythmFixture, RemoveRangeWithStartMarker)
{
  /* create audio track */
  audio_track_pos = TRACKLIST->get_num_tracks ();
  FileDescriptor file (TEST_WAV2);
  Track::create_with_action (
    Track::Type::Audio, nullptr, &file, nullptr, audio_track_pos, 1, -1,
    nullptr);

  /* remove range */
  Position start, end;
  start.set_to_bar (1);
  end.set_to_bar (3);
  UNDO_MANAGER->perform (std::make_unique<RangeRemoveAction> (start, end));

  check_start_end_markers ();

  /* undo */
  UNDO_MANAGER->undo ();

  check_start_end_markers ();
}

TEST_F (ZrythmFixture, RemoveRangeWithObjectsInside)
{
  /* create midi track with region */
  midi_track_pos = TRACKLIST->get_num_tracks ();
  FileDescriptor file (fs::path (TESTS_SRCDIR) / "1_track_with_data.mid");
  Track::create_with_action (
    Track::Type::Midi, nullptr, &file, nullptr, midi_track_pos, 1, -1, nullptr);
  auto midi_track = TRACKLIST->get_track<MidiTrack> (midi_track_pos);
  ASSERT_NONNULL (midi_track);

  /* create scale object */
  MusicalScale ms ((MusicalScale::Type) 0, (MusicalNote) 0);
  auto         so = std::make_shared<ScaleObject> (ms);
  P_CHORD_TRACK->add_scale (so);
  so->select (true, false, false);
  perform_create_arranger_sel (TL_SELECTIONS);

  /* remove range */
  Position start, end;
  start.set_to_bar (1);
  end.set_to_bar (14);
  UNDO_MANAGER->perform (std::make_unique<RangeRemoveAction> (start, end));

  check_start_end_markers ();

  /* check scale and midi region removed */
  ASSERT_EMPTY (midi_track->lanes_[0]->regions_);
  ASSERT_EMPTY (P_CHORD_TRACK->scales_);

  /* undo */
  UNDO_MANAGER->undo ();

  check_start_end_markers ();

  /* check scale and midi region added */
  ASSERT_SIZE_EQ (midi_track->lanes_[0]->regions_, 1);
  ASSERT_SIZE_EQ (P_CHORD_TRACK->scales_, 2);
}