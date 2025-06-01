// SPDX-FileCopyrightText: © 2020-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "dsp/midi_event.h"
#include "engine/device_io/engine.h"
#include "engine/session/router.h"
#include "gui/backend/backend/actions/tracklist_selections_action.h"
#include "gui/backend/backend/project.h"
#include "structure/tracks/fader.h"
#include "structure/tracks/master_track.h"
#include "structure/tracks/tracklist.h"

#include "helpers/project_helper.h"
#include "tests/helpers/plugin_manager.h"
#include "tests/helpers/zrythm_helper.h"

static void
test_fader_process_with_instrument (
  const char * pl_bundle,
  const char * pl_uri,
  bool         with_carla)
{
  test_plugin_manager_create_tracks_from_plugin (
    pl_bundle, pl_uri, true, with_carla, 1);

  auto * track = TRACKLIST->get_last_track<InstrumentTrack> ();

  /* stop dummy audio engine processing so we can process manually */
  test_project_stop_dummy_engine ();

  /* send a note then wait for playback */
  track->processor_->midi_in_->midi_events_.queued_events_.add_note_on (
    1, 82, 74, 2);

  /* run engine twice (running one is not enough to
   * make the note make sound) */
  for (int i = 0; i < 2; i++)
    {
      AUDIO_ENGINE->process (AUDIO_ENGINE->block_length_);
    }

  /* test fader */
  SemaphoreRAII<> sem{ ROUTER->graph_access_sem_, true };
  bool            has_signal = false;
  auto           &l = track->channel_->fader_->stereo_out_->get_l ();
  for (nframes_t i = 0; i < AUDIO_ENGINE->block_length_; i++)
    {
      if (l.buf_[i] > 0.0001f)
        {
          has_signal = true;
          break;
        }
    }
  ASSERT_TRUE (has_signal);
}

TEST_F (ZrythmFixture, FaderProcess)
{
  test_fader_process_with_instrument (
    TRIPLE_SYNTH_BUNDLE, TRIPLE_SYNTH_URI, true);
}

static bool
track_has_sound (ChannelTrack * track)
{
  for (nframes_t i = 0; i < AUDIO_ENGINE->block_length_; i++)
    {
      if (
        std::abs (track->channel_->fader_->stereo_out_->get_l ().buf_[i])
        > 0.0001f)
        {
          return true;
        }
    }
  return false;
}

static void
test_track_has_sound (ChannelTrack * track, bool expect_sound)
{
  Position pos;
  pos.set_to_bar (1);
  TRANSPORT->set_playhead_pos_rt_safe (pos);
  TRANSPORT->requestRoll (true);

  AUDIO_ENGINE->process (AUDIO_ENGINE->block_length_);
  AUDIO_ENGINE->process (AUDIO_ENGINE->block_length_);
  AUDIO_ENGINE->process (AUDIO_ENGINE->block_length_);

  ASSERT_TRUE (track_has_sound (track) == expect_sound);

  TRANSPORT->requestPause (true);
  AUDIO_ENGINE->process (AUDIO_ENGINE->block_length_);
}

TEST_F (ZrythmFixture, FaderSolo)
{
  /* create audio track */
  FileDescriptor file (fs::path (TESTS_SRCDIR) / "test.wav");
  Track::create_with_action (
    Track::Type::Audio, nullptr, &file, &PLAYHEAD, TRACKLIST->get_num_tracks (),
    1, -1, nullptr);
  auto audio_track = TRACKLIST->get_last_track<AudioTrack> ();

  /* create audio track 2 */
  Track::create_with_action (
    Track::Type::Audio, nullptr, &file, &PLAYHEAD, TRACKLIST->get_num_tracks (),
    1, -1, nullptr);
  auto audio_track2 = TRACKLIST->get_last_track<AudioTrack> ();

  /* create group track */
  auto group_track = Track::create_empty_with_action<AudioGroupTrack> ();

  /* route audio tracks to group track */
  audio_track->select (true, true, false);
  UNDO_MANAGER->perform (std::make_unique<ChangeTracksDirectOutAction> (
    *TRACKLIST_SELECTIONS->gen_tracklist_selections (), *PORT_CONNECTIONS_MGR,
    *group_track));
  audio_track2->select (true, true, false);
  UNDO_MANAGER->perform (std::make_unique<ChangeTracksDirectOutAction> (
    *TRACKLIST_SELECTIONS->gen_tracklist_selections (), *PORT_CONNECTIONS_MGR,
    *group_track));

  /* stop dummy audio engine processing so we can process manually */
  test_project_stop_dummy_engine ();

  /* test solo group makes sound */
  group_track->set_soloed (true, true, true, false);
  test_track_has_sound (P_MASTER_TRACK, true);
  test_track_has_sound (group_track, true);
  test_track_has_sound (audio_track, true);
  test_track_has_sound (audio_track2, true);
  UNDO_MANAGER->undo ();

  /* test solo audio track makes sound */
  audio_track->set_soloed (true, true, true, false);
  test_track_has_sound (P_MASTER_TRACK, true);
  test_track_has_sound (group_track, true);
  test_track_has_sound (audio_track, true);
  test_track_has_sound (audio_track2, false);
  UNDO_MANAGER->undo ();

  /* test solo both audio tracks */
  audio_track->select (true, true, false);
  audio_track2->select (true, false, false);
  UNDO_MANAGER->perform (std::make_unique<SoloTracksAction> (
    *TRACKLIST_SELECTIONS->gen_tracklist_selections (), true));
  test_track_has_sound (P_MASTER_TRACK, true);
  test_track_has_sound (group_track, true);
  test_track_has_sound (audio_track, true);
  test_track_has_sound (audio_track2, true);
  UNDO_MANAGER->undo ();

  /* test undo/redo */
  audio_track->select (true, true, false);
  UNDO_MANAGER->perform (std::make_unique<SoloTracksAction> (
    *TRACKLIST_SELECTIONS->gen_tracklist_selections (), true));
  ASSERT_TRUE (audio_track->get_soloed ());
  ASSERT_FALSE (audio_track2->get_soloed ());
  audio_track->select (true, true, false);
  audio_track2->select (true, false, false);
  UNDO_MANAGER->perform (std::make_unique<SoloTracksAction> (
    *TRACKLIST_SELECTIONS->gen_tracklist_selections (), true));
  ASSERT_TRUE (audio_track->get_soloed ());
  ASSERT_TRUE (audio_track2->get_soloed ());
  UNDO_MANAGER->undo ();
  ASSERT_TRUE (audio_track->get_soloed ());
  ASSERT_FALSE (audio_track2->get_soloed ());
  UNDO_MANAGER->redo ();
  ASSERT_TRUE (audio_track->get_soloed ());
  ASSERT_TRUE (audio_track2->get_soloed ());
}
