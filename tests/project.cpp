// SPDX-FileCopyrightText: © 2020-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "actions/arranger_selections.h"
#include "actions/tracklist_selections.h"
#include "dsp/chord_track.h"
#include "dsp/engine_dummy.h"
#include "dsp/marker_track.h"
#include "dsp/tempo_track.h"
#include "dsp/track.h"
#include "dsp/tracklist.h"
#include "project.h"
#include "project/project_init_flow_manager.h"
#include "utils/dsp.h"
#include "utils/flags.h"
#include "utils/io.h"
#include "utils/ui.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include "helpers/jack.h"
#include "helpers/plugin_manager.h"
#include "tests/helpers/project_helper.h"
#include "tests/helpers/zrythm_helper.h"

TEST_SUITE_BEGIN ("project");

TEST_CASE_FIXTURE (ZrythmFixture, "empty project save and load")
{
  REQUIRE_NONNULL (PROJECT);

  /* save and reload the project */
  test_project_save_and_reload ();

  /* resave it */
  REQUIRE_NOTHROW (PROJECT->save (PROJECT->dir_, F_NOT_BACKUP, 0, F_NO_ASYNC));
}

TEST_CASE_FIXTURE (BootstrapTimelineFixture, "project save and load with data")
{
  REQUIRE_NONNULL (PROJECT);

  /* save the project */
  REQUIRE_NOTHROW (PROJECT->save (PROJECT->dir_, 0, 0, false));
  auto prj_file = Glib::build_filename (PROJECT->dir_, PROJECT_FILE);

  /* stop the engine */
  AudioEngine::State state;
  AUDIO_ENGINE->wait_for_pause (state, true, true);

  /* remove objects */
  P_CHORD_TRACK->clear_objects ();
  P_MARKER_TRACK->clear_objects ();
  P_TEMPO_TRACK->clear_objects ();

  for (auto &track : TRACKLIST->tracks_ | std::views::reverse)
    {
      if (track->is_deletable ())
        {
          TRACKLIST->remove_track (*track, true, true, false, false);
        }
    }
  P_MASTER_TRACK->clear_objects ();

  /* reload the project */
  test_project_reload (prj_file);

  /* stop the engine */
  PROJECT->audio_engine_->resume (state);

  /* verify that the data is correct */
  check_vs_original_state (false);
}

TEST_CASE_FIXTURE (ZrythmFixture, "project new from template")
{
  /* add plugins */
  test_plugin_manager_create_tracks_from_plugin (
    TRIPLE_SYNTH_BUNDLE, TRIPLE_SYNTH_URI, true, false, 1);
#ifdef HAVE_CARLA
  test_plugin_manager_create_tracks_from_plugin (
    TRIPLE_SYNTH_BUNDLE, TRIPLE_SYNTH_URI, true, true, 1);
#endif

  /* create region and ensure sound */
  auto     track = TRACKLIST->get_last_track<InstrumentTrack> ();
  Position start, end;
  start.zero ();
  end.set_to_bar (3);
  auto r =
    std::make_shared<MidiRegion> (start, end, track->get_name_hash (), 0, 0);
  track->add_region (r, nullptr, 0, true, false);
  r->select (true, false, false);
  UNDO_MANAGER->perform (
    std::make_unique<CreateArrangerSelectionsAction> (*TL_SELECTIONS));
  auto mn = std::make_shared<MidiNote> (r->id_, start, end, 45, 45);
  r->append_object (mn);
  mn->select (true, false, false);
  UNDO_MANAGER->perform (
    std::make_unique<CreateArrangerSelectionsAction> (*MIDI_SELECTIONS));

  /* stop dummy audio engine processing so we can process manually */
  test_project_stop_dummy_engine ();

  /* start playback and process for a couple of cycles */
  TRANSPORT->set_playhead_to_bar (1);
  TRANSPORT->request_roll (true);
  AUDIO_ENGINE->process (AUDIO_ENGINE->block_length_);
  AUDIO_ENGINE->process (AUDIO_ENGINE->block_length_);

  REQUIRE_GT (
    dsp_abs_max (
      &MONITOR_FADER->stereo_out_->get_l ().buf_[0], AUDIO_ENGINE->block_length_),
    0.0001f);

  test_project_save_and_reload ();

  /* stop dummy audio engine processing so we can process
   * manually */
  test_project_stop_dummy_engine ();

  /* start playback and process for a couple of cycles */
  TRANSPORT->set_playhead_to_bar (1);
  TRANSPORT->request_roll (true);
  AUDIO_ENGINE->process (AUDIO_ENGINE->block_length_);
  AUDIO_ENGINE->process (AUDIO_ENGINE->block_length_);

  REQUIRE_GT (
    dsp_abs_max (
      &MONITOR_FADER->stereo_out_->get_l ().buf_[0], AUDIO_ENGINE->block_length_),
    0.0001f);

  /* create a new project using old one as template */
  auto orig_dir = PROJECT->dir_;
  REQUIRE_NONEMPTY (orig_dir);
  auto filepath = Glib::build_filename (orig_dir, "project.zpj");
  gZrythm->create_project_path_.clear ();
  char * tmp = g_dir_make_tmp ("zrythm_test_project_XXXXXX", nullptr);
  gZrythm->create_project_path_ = tmp;
  g_free (tmp);
  ProjectInitFlowManager mgr (
    filepath, true, test_helper_project_init_done_cb, nullptr);
  io_rmdir (orig_dir, true);

  /* stop dummy audio engine processing so we can process
   * manually */
  test_project_stop_dummy_engine ();

  /* start playback and process for a couple of cycles */
  TRANSPORT->set_playhead_to_bar (1);
  TRANSPORT->request_roll (true);
  AUDIO_ENGINE->process (AUDIO_ENGINE->block_length_);
  AUDIO_ENGINE->process (AUDIO_ENGINE->block_length_);

  REQUIRE_GT (
    dsp_abs_max (
      &MONITOR_FADER->stereo_out_->get_l ().buf_[0], AUDIO_ENGINE->block_length_),
    0.0001f);
}

TEST_CASE_FIXTURE (BootstrapTimelineFixture, "project save as and load with pool")
{
  /* save the project elsewhere */
  auto orig_dir = PROJECT->dir_;
  REQUIRE_NONEMPTY (orig_dir);
  char * new_dir = g_dir_make_tmp ("zrythm_test_project_XXXXXX", nullptr);
  REQUIRE_NOTHROW (PROJECT->save (new_dir, false, false, false));

  /* free the project */
  AUDIO_ENGINE->activate (false);
  PROJECT.reset ();

  /* load the new one */
  auto filepath = Glib::build_filename (new_dir, "project.zpj");
  test_project_reload (filepath);

  io_rmdir (orig_dir, true);
}

TEST_CASE_FIXTURE (
  BootstrapTimelineFixture,
  "project save backup with pool and plugins")
{
  /* add a plugin and create a duplicate track */
  int track_pos = test_plugin_manager_create_tracks_from_plugin (
    TEST_INSTRUMENT_BUNDLE_URI, TEST_INSTRUMENT_URI, true, true, 1);
  auto track = TRACKLIST->get_track<InstrumentTrack> (track_pos);
  track->select (true, true, false);
  UNDO_MANAGER->perform (std::make_unique<CopyTracksAction> (
    *TRACKLIST_SELECTIONS->gen_tracklist_selections (), *PORT_CONNECTIONS_MGR,
    TRACKLIST->get_num_tracks ()));

  auto dir = PROJECT->dir_;

  /* save a project backup */
  REQUIRE_NOTHROW (PROJECT->save (PROJECT->dir_, true, false, false));
  REQUIRE_NONEMPTY (PROJECT->backup_dir_);
  auto backup_dir = PROJECT->backup_dir_;

  /* free the project */
  AUDIO_ENGINE->activate (false);
  PROJECT.reset ();

  /* load the backup directly */
  auto filepath = Glib::build_filename (backup_dir, PROJECT_FILE);
  test_project_reload (filepath);

  /* undo history not saved with backups anymore */
#if 0
  /* test undo and redo cloning the track */
  UNDO_MANAGER->undo();
  UNDO_MANAGER->redo();
#endif

  /* free the project */
  AUDIO_ENGINE->activate (false);
  PROJECT.reset ();

  /* attempt to open the latest backup (mimic
   * behavior from UI) */
  gZrythm->open_newer_backup_ = true;
  filepath = Glib::build_filename (dir, "project.zpj");
  test_project_reload (filepath);
}

/**
 * Load a project with a plugin after saving a backup (there
 * was a bug causing plugin states to be deleted when saving
 * backups).
 */
TEST_CASE_FIXTURE (ZrythmFixture, "project load with plugin after backup")
{
  /* add a plugin */
  int track_pos = test_plugin_manager_create_tracks_from_plugin (
    COMPRESSOR_BUNDLE, COMPRESSOR_URI, false, true, 1);
  auto track = TRACKLIST->get_track<ChannelTrack> (track_pos);
  track->select (true, true, false);

  /* add an audio file too to test the pool as well */
  auto     audio_file_path = Glib::build_filename (TESTS_SRCDIR, "test.wav");
  Position pos;
  pos.zero ();
  FileDescriptor file = FileDescriptor (audio_file_path);
  Track::create_with_action (
    Track::Type::Audio, nullptr, &file, &pos, TRACKLIST->get_num_tracks (), 1,
    -1, nullptr);

  auto dir = PROJECT->dir_;

  /* save the project normally */
  REQUIRE_NOTHROW (PROJECT->save (PROJECT->dir_, false, false, false));

  /* save a project backup */
  REQUIRE_NOTHROW (PROJECT->save (PROJECT->dir_, true, false, false));
  REQUIRE_NONEMPTY (PROJECT->backup_dir_);
  auto backup_dir = PROJECT->backup_dir_;

  /* free the project */
  AUDIO_ENGINE->activate (false);
  PROJECT.reset ();

  /* attempt to open the normal project (not backup) */
  gZrythm->open_newer_backup_ = false;
  auto filepath = Glib::build_filename (dir, "project.zpj");
  test_project_reload (filepath);

  /* free the project */
  AUDIO_ENGINE->activate (false);
  PROJECT.reset ();

  /* load the backup directly */
  filepath = Glib::build_filename (backup_dir, PROJECT_FILE);
  test_project_reload (filepath);
}

static void
init_cb (bool success, GError * error, void * user_data)
{
}

TEST_CASE_FIXTURE (ZrythmFixture, "project load v1.0.0-beta.2.1.1")
{
  /* TODO test opening a project from 1.0.0 beta 2.1.1 that
   * has undo history */

  /*yaml_set_log_level (CYAML_LOG_DEBUG);*/
  (void) init_cb;
  /*project_init_flow_manager_load_or_create_default_project
   * ("/home/alex/.var/app/org.zrythm.Zrythm/data/zrythm/projects/プロジェクト名未設定*/
  /** (3)/project.zpj", false, init_cb, nullptr);*/
  /*project_load
   * ("/home/alex/.var/app/org.zrythm.Zrythm/data/zrythm/projects/プロジェクト名未設定
   * (3)/project.zpj", false);*/
}

TEST_CASE_FIXTURE (ZrythmFixtureWithPipewire, "exposed ports after load")
{
#ifdef HAVE_PIPEWIRE
  auto        track = Track::create_empty_with_action<AudioTrack> ();
  std::string buf;
  {
    const auto &port = track->channel_->stereo_out_->get_l ();
    buf = port.get_full_designation ();

    REQUIRE (port.is_exposed_to_backend ());
    assert_jack_port_exists (buf);
  }
  test_project_save_and_reload ();

  track = TRACKLIST->get_last_track<AudioTrack> ();
  const auto &port = track->channel_->stereo_out_->get_l ();
  REQUIRE (port.is_exposed_to_backend ());
  assert_jack_port_exists (buf);
#endif // HAVE_PIPEWIRE
}

TEST_SUITE_END;