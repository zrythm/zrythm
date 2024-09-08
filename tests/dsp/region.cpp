// SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "dsp/midi_region.h"
#include "dsp/region.h"
#include "project.h"
#include "utils/flags.h"
#include "zrythm.h"

#include "tests/helpers/zrythm_helper.h"

TEST_SUITE_BEGIN ("dsp/region");

class RegionFixture : public ZrythmFixture
{
protected:
  std::unique_ptr<MidiRegion> midi_region_;

public:
  RegionFixture ()
  {
    /* needed to set TRANSPORT */
    AUDIO_ENGINE->update_frames_per_tick (4, 140, 44000, true, true, false);

    Position start_pos, end_pos;
    start_pos.set_to_bar (2);
    end_pos.set_to_bar (4);
    midi_region_ = std::make_unique<MidiRegion> (start_pos, end_pos, 0, 0, 0);
  }
  ~RegionFixture () { midi_region_.reset (); }
};

TEST_CASE_FIXTURE (ZrythmFixture, "region is hit by range")
{
  Position pos, end_pos;
  pos.set_to_bar (4);
  end_pos.set_to_bar (5);
  Position other_start_pos;
  other_start_pos.set_to_bar (3);
  MidiRegion region (pos, end_pos, 0, -1, -1);
  ASSERT_TRUE (region.is_hit_by_range (
    other_start_pos.frames_, pos.frames_, true, true, false));
}

TEST_CASE_FIXTURE (RegionFixture, "region is hit")
{
  /*
   * Position: Region start
   *
   * Expected result:
   * Returns true either exclusive or inclusive.
   */
  {
    auto pos = midi_region_->pos_;
    pos.update_frames_from_ticks (0.0);
    ASSERT_TRUE (midi_region_->is_hit (pos.frames_));
    ASSERT_TRUE (midi_region_->is_hit (pos.frames_, true));
  }

  /*
   * Position: Region start - 1 frame
   *
   * Expected result:
   * Returns false either exclusive or inclusive.
   */
  {
    auto pos = midi_region_->pos_;
    pos.update_frames_from_ticks (0.0);
    pos.add_frames (-1);
    ASSERT_FALSE (midi_region_->is_hit (pos.frames_, false));
    ASSERT_FALSE (midi_region_->is_hit (pos.frames_, true));
  }

  /*
   * Position: Region end
   *
   * Expected result:
   * Returns true for inclusive, false for not.
   */
  {
    auto pos = midi_region_->end_pos_;
    pos.update_frames_from_ticks (0.0);
    ASSERT_FALSE (midi_region_->is_hit (pos.frames_, false));
    ASSERT_TRUE (midi_region_->is_hit (pos.frames_, true));
  }

  /*
   * Position: Region end - 1
   *
   * Expected result:
   * Returns true for both.
   */
  {
    auto pos = midi_region_->end_pos_;
    pos.update_frames_from_ticks (0.0);
    pos.add_frames (-1);
    ASSERT_TRUE (midi_region_->is_hit (pos.frames_, false));
    ASSERT_TRUE (midi_region_->is_hit (pos.frames_, true));
  }

  /*
   * Position: Region end + 1
   *
   * Expected result:
   * Returns false for both.
   */
  {
    auto pos = midi_region_->end_pos_;
    pos.update_frames_from_ticks (0.0);
    pos.add_frames (1);
    ASSERT_FALSE (midi_region_->is_hit (pos.frames_, false));
    ASSERT_FALSE (midi_region_->is_hit (pos.frames_, true));
  }
}

TEST_CASE_FIXTURE (ZrythmFixture, "new region")
{
  Position start_pos, end_pos, tmp;
  start_pos.set_to_bar (2);
  end_pos.set_to_bar (4);
  MidiRegion region (start_pos, end_pos, 0, 0, 0);

  ASSERT_EQ (region.id_.type_, RegionType::Midi);
  ASSERT_EQ (start_pos, region.pos_);
  ASSERT_EQ (end_pos, region.end_pos_);
  tmp.zero ();
  ASSERT_EQ (tmp, region.clip_start_pos_);

  ASSERT_FALSE (region.muted_);
  ASSERT_EMPTY (region.midi_notes_);

  tmp = region.pos_;
  tmp.add_ticks (12);
  if (region.is_position_valid (tmp, ArrangerObject::PositionType::Start))
    {
      region.set_position (
        &tmp, ArrangerObject::PositionType::Start, F_NO_VALIDATE);
    }
}

TEST_CASE_FIXTURE (ZrythmFixture, "timeline frames to local")
{
  auto track = Track::create_empty_with_action<MidiTrack> ();

  Position pos, end_pos;
  end_pos.set_to_bar (4);
  MidiRegion region (pos, end_pos, track->get_name_hash (), 0, 0);
  auto localp = region.timeline_frames_to_local( 13'000, true);
  ASSERT_EQ (localp, 13000);
  localp = region.timeline_frames_to_local(13'000, false);
  ASSERT_EQ (localp, 13000);
}

TEST_SUITE_END;