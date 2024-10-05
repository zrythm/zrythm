// SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "common/dsp/track.h"
#include "gui/cpp/backend/project.h"
#include "gui/cpp/backend/zrythm.h"

#include "tests/helpers/project_helper.h"

TEST_F (ZrythmFixture, NewTrack)
{
  auto track = Track::create_track (
    Track::Type::Instrument, "Test Instrument Track 1",
    TRACKLIST->get_last_pos ());
  ASSERT_NONNULL (track);
  ASSERT_NONEMPTY (track->name_);
}

class AddRegionFixture : public ZrythmFixture
{
protected:
  Position start, end;

  void SetUp () override
  {
    ZrythmFixture::SetUp ();
    start.set_to_bar (2);
    end.set_to_bar (4);
  }

  void check_track_name_hash (const auto &region, const auto &track)
  {
    ASSERT_FALSE (track->name_.empty ());
    ASSERT_NE (region->track_name_hash_, 0);
    ASSERT_EQ (region->track_name_hash_, track->get_name_hash ());
    ASSERT_EQ (region->id_.track_name_hash_, region->track_name_hash_);
  }
};

TEST_F (AddRegionFixture, LanedRegion)
{
  auto           midi_track = Track::create_empty_with_action<MidiTrack> ();
  constexpr auto lane_pos = 0;
  auto           region = std::make_shared<MidiRegion> (
    start, end, midi_track->get_name_hash (), lane_pos, 0);
  midi_track->add_region (region, nullptr, lane_pos, true, false);
  check_track_name_hash (region, midi_track);
}

TEST_F (AddRegionFixture, ChordRegion)
{
  auto chord_track = P_CHORD_TRACK;
  auto region = std::make_shared<ChordRegion> (start, end, 0);
  chord_track->Track::add_region (region, nullptr, -1, true, false);
  check_track_name_hash (region, chord_track);
}

TEST_F (AddRegionFixture, AutomationRegion)
{
  auto master = P_MASTER_TRACK;
  master->set_automation_visible (true);
  auto &atl = master->get_automation_tracklist ();
  auto  first_vis_at = atl.visible_ats_.front ();

  auto region = std::make_shared<AutomationRegion> (
    start, end, master->get_name_hash (), first_vis_at->index_, 0);
  master->add_region (region, first_vis_at, -1, true, false);
  check_track_name_hash (region, master);
}

TEST_F (ZrythmFixture, GetDirectFolderParent)
{
  auto audio_group = Track::create_empty_with_action<AudioGroupTrack> ();
  ASSERT_NONNULL (audio_group);

  auto audio_group2 = Track::create_empty_with_action<AudioGroupTrack> ();
  ASSERT_NONNULL (audio_group2);
  audio_group2->select (true, false, false);
  TRACKLIST->handle_move_or_copy (
    *audio_group, TrackWidgetHighlight::TRACK_WIDGET_HIGHLIGHT_INSIDE,
    GDK_ACTION_MOVE);

  auto audio_group3 = Track::create_empty_with_action<AudioGroupTrack> ();
  ASSERT_NONNULL (audio_group3);
  audio_group3->select (true, false, false);
  TRACKLIST->handle_move_or_copy (
    *audio_group2, TrackWidgetHighlight::TRACK_WIDGET_HIGHLIGHT_INSIDE,
    GDK_ACTION_MOVE);

  ASSERT_EQ (audio_group->pos_, 5);
  ASSERT_EQ (audio_group->size_, 3);
  ASSERT_EQ (audio_group2->pos_, 6);
  ASSERT_EQ (audio_group2->size_, 2);
  ASSERT_EQ (audio_group3->pos_, 7);
  ASSERT_EQ (audio_group3->size_, 1);

  auto direct_folder_parent = audio_group3->get_direct_folder_parent ();
  ASSERT_TRUE (direct_folder_parent == audio_group2);
}