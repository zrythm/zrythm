// SPDX-FileCopyrightText: © 2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "dsp/transport.h"
#include "gui/backend/backend/actions/transport_action.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/dsp/control_port.h"

#include "tests/helpers/plugin_manager.h"
#include "tests/helpers/project_helper.h"

TEST_F (ZrythmFixture, ChangeBPMAndTimeSignature)
{
  /* import audio */
  FileDescriptor file_descr (fs::path (TESTS_SRCDIR) / "test.wav");
  Position       pos;
  pos.set_to_bar (4);
  Track::create_with_action (
    Track::Type::Audio, nullptr, &file_descr, &pos,
    TRACKLIST->get_num_tracks (), 1, -1, nullptr);
  auto audio_track =
    TRACKLIST->get_track<AudioTrack> (TRACKLIST->get_num_tracks () - 1);
  int audio_track_pos = audio_track->pos_;
  (void) audio_track_pos;

  /* loop the region */
  const auto &r = audio_track->lanes_[0]->regions_[0];
  ASSERT_NO_THROW (
    r->resize (false, ArrangerObject::ResizeType::Loop, 40000, false));

  ASSERT_EQ (P_TEMPO_TRACK->get_beat_unit (), 4);

  /* change time sig to 4/16 */
  {
    ControlPort::ChangeEvent change;
    change.flag = PortIdentifier::Flags::BeatUnit;
    change.beat_unit = BeatUnit::Sixteen;
    ROUTER->queue_control_port_change (change);
  }

  AUDIO_ENGINE->wait_n_cycles (3);

  ASSERT_EQ (P_TEMPO_TRACK->get_beat_unit (), 16);

  /* perform the change */
  UNDO_MANAGER->perform (
    std::make_unique<TransportAction> (
      TransportAction::Type::BeatUnitChange, 4, 16, true));
  ASSERT_EQ (P_TEMPO_TRACK->get_beat_unit (), 16);
  AUDIO_ENGINE->wait_n_cycles (3);
  ASSERT_EQ (P_TEMPO_TRACK->get_beat_unit (), 16);

  test_project_save_and_reload ();

  /* undo */
  UNDO_MANAGER->undo ();
  ASSERT_EQ (P_TEMPO_TRACK->get_beat_unit (), 4);
  AUDIO_ENGINE->wait_n_cycles (3);
  ASSERT_EQ (P_TEMPO_TRACK->get_beat_unit (), 4);

  /* redo */
  UNDO_MANAGER->redo ();
  ASSERT_EQ (P_TEMPO_TRACK->get_beat_unit (), 16);
  AUDIO_ENGINE->wait_n_cycles (3);
  ASSERT_EQ (P_TEMPO_TRACK->get_beat_unit (), 16);

  /* print region */
  audio_track = TRACKLIST->get_track<AudioTrack> (audio_track_pos);
  ASSERT_NONNULL (audio_track);
  ASSERT_NONNULL (audio_track->lanes_[0]->regions_[0]);

  /* change BPM to 145 */
  bpm_t bpm_before = P_TEMPO_TRACK->get_current_bpm ();
  {
    ControlPort::ChangeEvent change;
    change.flag = PortIdentifier::Flags::Bpm;
    change.real_val = 145.f;
    ROUTER->queue_control_port_change (change);
  }

  AUDIO_ENGINE->wait_n_cycles (3);
  ASSERT_NEAR (P_TEMPO_TRACK->get_current_bpm (), 145.f, 0.001f);

  /* print region */
  audio_track = TRACKLIST->get_track<AudioTrack> (audio_track_pos);
  ASSERT_NONNULL (audio_track);
  ASSERT_NONNULL (audio_track->lanes_[0]->regions_[0]);

  /* perform the change to 150 */
  UNDO_MANAGER->perform (
    std::make_unique<TransportAction> (bpm_before, 150.f, false));
  ASSERT_NEAR (P_TEMPO_TRACK->get_current_bpm (), 150.f, 0.001f);
  AUDIO_ENGINE->wait_n_cycles (3);
  ASSERT_NEAR (P_TEMPO_TRACK->get_current_bpm (), 150.f, 0.001f);

  audio_track = TRACKLIST->get_track<AudioTrack> (audio_track_pos);
  ASSERT_NONNULL (audio_track);
  ASSERT_NONNULL (audio_track->lanes_[0]->regions_[0]);

  UNDO_MANAGER->undo ();
  UNDO_MANAGER->redo ();

  /* change bpm to 130 */
  bpm_before = P_TEMPO_TRACK->get_current_bpm ();
  {
    ControlPort::ChangeEvent change;
    change.flag = PortIdentifier::Flags::Bpm;
    change.real_val = 130.f;
    ROUTER->queue_control_port_change (change);
  }

  AUDIO_ENGINE->wait_n_cycles (3);
  ASSERT_NEAR (P_TEMPO_TRACK->get_current_bpm (), 130.f, 0.001f);

  /* validate */
  ASSERT_TRUE (audio_track->lanes_[0]->regions_[0]->validate (true, 0));

  /* perform the change to 130 */
  UNDO_MANAGER->perform (
    std::make_unique<TransportAction> (bpm_before, 130.f, false));
  ASSERT_NEAR (P_TEMPO_TRACK->get_current_bpm (), 130.f, 0.001f);
  AUDIO_ENGINE->wait_n_cycles (3);
  ASSERT_NEAR (P_TEMPO_TRACK->get_current_bpm (), 130.f, 0.001f);

  audio_track = TRACKLIST->get_track<AudioTrack> (audio_track_pos);
  ASSERT_NONNULL (audio_track);
  ASSERT_NONNULL (audio_track->lanes_[0]->regions_[0]);

  /* validate */
  ASSERT_TRUE (audio_track->lanes_[0]->regions_[0]->validate (true, 0));
}

TEST_F (ZrythmFixture, ChangeBPMTwiceDuringPlayback)
{
  /* import audio */
  char audio_file_path[2000];
  sprintf (
    audio_file_path, "%s%s%s", TESTS_SRCDIR, G_DIR_SEPARATOR_S, "test.wav");
  FileDescriptor file_descr = FileDescriptor (audio_file_path);
  Position       pos;
  pos.set_to_bar (4);
  Track::create_with_action (
    Track::Type::Audio, nullptr, &file_descr, &pos,
    TRACKLIST->get_num_tracks (), 1, -1, nullptr);
  auto audio_track =
    TRACKLIST->get_track<AudioTrack> (TRACKLIST->get_num_tracks () - 1);
  int audio_track_pos = audio_track->pos_;
  (void) audio_track_pos;

  /* loop the region */
  const auto &r = audio_track->lanes_.front ()->regions_.front ();
  ASSERT_NO_THROW (
    r->resize (false, ArrangerObject::ResizeType::Loop, 40000, false));
  ASSERT_TRUE (r->validate (true, 0));

  /* start playback */
  TRANSPORT->requestRoll (true);
  AUDIO_ENGINE->wait_n_cycles (3);

  bpm_t bpm_before = P_TEMPO_TRACK->get_current_bpm ();

  /* change BPM to 40 */
  UNDO_MANAGER->perform (
    std::make_unique<TransportAction> (bpm_before, 40.f, false));
  ASSERT_NEAR (P_TEMPO_TRACK->get_current_bpm (), 40.f, 0.001f);
  AUDIO_ENGINE->wait_n_cycles (3);
  ASSERT_NEAR (P_TEMPO_TRACK->get_current_bpm (), 40.f, 0.001f);

  TRANSPORT->requestRoll (true);
  AUDIO_ENGINE->wait_n_cycles (3);

  /* change bpm to 140 */
  UNDO_MANAGER->perform (
    std::make_unique<TransportAction> (bpm_before, 140.f, false));

  ASSERT_NEAR (P_TEMPO_TRACK->get_current_bpm (), 140.f, 0.001f);
  AUDIO_ENGINE->wait_n_cycles (3);
  ASSERT_NEAR (P_TEMPO_TRACK->get_current_bpm (), 140.f, 0.001f);

  TRANSPORT->requestRoll (true);
  AUDIO_ENGINE->wait_n_cycles (3);

  /* validate */
  ASSERT_TRUE (
    audio_track->lanes_.front ()->regions_.front ()->validate (true, 0));
}
