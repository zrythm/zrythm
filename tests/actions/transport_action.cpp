// SPDX-FileCopyrightText: © 2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "actions/transport_action.h"
#include "dsp/control_port.h"
#include "dsp/transport.h"
#include "project.h"
#include "utils/flags.h"
#include "zrythm.h"

#include <glib.h>

#include "tests/helpers/plugin_manager.h"
#include "tests/helpers/project.h"

static void
test_change_bpm_and_time_sig (void)
{
  test_helper_zrythm_init ();

  /* import audio */
  char audio_file_path[2000];
  sprintf (
    audio_file_path, "%s%s%s", TESTS_SRCDIR, G_DIR_SEPARATOR_S, "test.wav");
  SupportedFile * file_descr = supported_file_new_from_path (audio_file_path);
  Position        pos;
  position_set_to_bar (&pos, 4);
  track_create_with_action (
    TrackType::TRACK_TYPE_AUDIO, NULL, file_descr, &pos, TRACKLIST->num_tracks,
    1, -1, NULL, NULL);
  Track * audio_track = tracklist_get_last_track (
    TRACKLIST, TracklistPinOption::TRACKLIST_PIN_OPTION_BOTH, false);
  int audio_track_pos = audio_track->pos;
  (void) audio_track_pos;
  supported_file_free (file_descr);

  /* loop the region */
  Region *         r = audio_track->lanes[0]->regions[0];
  ArrangerObject * r_obj = (ArrangerObject *) r;
  bool             success = arranger_object_resize (
    r_obj, false, ArrangerObjectResizeType::ARRANGER_OBJECT_RESIZE_LOOP, 40000,
    false, NULL);
  g_assert_true (success);

  /* print region before the change */
  g_assert_true (IS_REGION_AND_NONNULL (r));
  arranger_object_print (r_obj);

  g_assert_cmpint (tempo_track_get_beat_unit (P_TEMPO_TRACK), ==, 4);

  /* change time sig to 4/16 */
  {
    ControlPortChange change = {};
    change.flag2 = PortIdentifier::Flags2::BEAT_UNIT;
    change.beat_unit = ZBeatUnit::Z_BEAT_UNIT_16;
    router_queue_control_port_change (ROUTER, &change);
  }

  engine_wait_n_cycles (AUDIO_ENGINE, 3);

  g_assert_cmpint (tempo_track_get_beat_unit (P_TEMPO_TRACK), ==, 16);

  /* perform the change */
  transport_action_perform_time_sig_change (
    TransportActionType::TRANSPORT_ACTION_BEAT_UNIT_CHANGE, 4, 16, true, NULL);
  g_assert_cmpint (tempo_track_get_beat_unit (P_TEMPO_TRACK), ==, 16);
  engine_wait_n_cycles (AUDIO_ENGINE, 3);
  g_assert_cmpint (tempo_track_get_beat_unit (P_TEMPO_TRACK), ==, 16);

  test_project_save_and_reload ();

  /* undo */
  undo_manager_undo (UNDO_MANAGER, NULL);
  g_assert_cmpint (tempo_track_get_beat_unit (P_TEMPO_TRACK), ==, 4);
  engine_wait_n_cycles (AUDIO_ENGINE, 3);
  g_assert_cmpint (tempo_track_get_beat_unit (P_TEMPO_TRACK), ==, 4);

  /* redo */
  undo_manager_redo (UNDO_MANAGER, NULL);
  g_assert_cmpint (tempo_track_get_beat_unit (P_TEMPO_TRACK), ==, 16);
  engine_wait_n_cycles (AUDIO_ENGINE, 3);
  g_assert_cmpint (tempo_track_get_beat_unit (P_TEMPO_TRACK), ==, 16);

  /* print region */
  g_message ("-- before BPM change");
  audio_track = TRACKLIST->tracks[audio_track_pos];
  r = audio_track->lanes[0]->regions[0];
  r_obj = (ArrangerObject *) r;
  g_assert_true (IS_REGION_AND_NONNULL (r));
  arranger_object_print (r_obj);

  /* change BPM to 145 */
  bpm_t bpm_before = tempo_track_get_current_bpm (P_TEMPO_TRACK);
  {
    ControlPortChange change = {};
    change.flag1 = PortIdentifier::Flags::BPM;
    change.real_val = 145.f;
    router_queue_control_port_change (ROUTER, &change);
  }

  engine_wait_n_cycles (AUDIO_ENGINE, 3);
  g_assert_cmpfloat_with_epsilon (
    tempo_track_get_current_bpm (P_TEMPO_TRACK), 145.f, 0.001f);

  /* print region */
  g_message ("-- after first BPM change");
  audio_track = TRACKLIST->tracks[audio_track_pos];
  r = audio_track->lanes[0]->regions[0];
  r_obj = (ArrangerObject *) r;
  g_assert_true (IS_REGION_AND_NONNULL (r));
  arranger_object_print (r_obj);

  /* perform the change to 150 */
  transport_action_perform_bpm_change (bpm_before, 150.f, false, NULL);
  g_assert_cmpfloat_with_epsilon (
    tempo_track_get_current_bpm (P_TEMPO_TRACK), 150.f, 0.001f);
  engine_wait_n_cycles (AUDIO_ENGINE, 3);
  g_assert_cmpfloat_with_epsilon (
    tempo_track_get_current_bpm (P_TEMPO_TRACK), 150.f, 0.001f);

  /* print region */
  g_message ("-- after BPM change action");
  audio_track = TRACKLIST->tracks[audio_track_pos];
  r = audio_track->lanes[0]->regions[0];
  r_obj = (ArrangerObject *) r;
  g_assert_true (IS_REGION_AND_NONNULL (r));
  arranger_object_print (r_obj);

  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_redo (UNDO_MANAGER, NULL);

  /* change bpm to 130 */
  bpm_before = tempo_track_get_current_bpm (P_TEMPO_TRACK);
  {
    ControlPortChange change = {};
    change.flag1 = PortIdentifier::Flags::BPM;
    change.real_val = 130.f;
    router_queue_control_port_change (ROUTER, &change);
  }

  engine_wait_n_cycles (AUDIO_ENGINE, 3);
  g_assert_cmpfloat_with_epsilon (
    tempo_track_get_current_bpm (P_TEMPO_TRACK), 130.f, 0.001f);

  /* validate */
  g_assert_true (arranger_object_validate (r_obj));

  /* perform the change to 130 */
  transport_action_perform_bpm_change (bpm_before, 130.f, false, NULL);
  g_assert_cmpfloat_with_epsilon (
    tempo_track_get_current_bpm (P_TEMPO_TRACK), 130.f, 0.001f);
  engine_wait_n_cycles (AUDIO_ENGINE, 3);
  g_assert_cmpfloat_with_epsilon (
    tempo_track_get_current_bpm (P_TEMPO_TRACK), 130.f, 0.001f);

  /* print region */
  g_message ("-- after BPM change action (13)");
  audio_track = TRACKLIST->tracks[audio_track_pos];
  r = audio_track->lanes[0]->regions[0];
  r_obj = (ArrangerObject *) r;
  g_assert_true (IS_REGION_AND_NONNULL (r));
  arranger_object_print (r_obj);

  /* validate */
  g_assert_true (arranger_object_validate (r_obj));

  test_helper_zrythm_cleanup ();
}

static void
test_change_bpm_twice_during_playback (void)
{
  test_helper_zrythm_init ();

  /* import audio */
  char audio_file_path[2000];
  sprintf (
    audio_file_path, "%s%s%s", TESTS_SRCDIR, G_DIR_SEPARATOR_S, "test.wav");
  SupportedFile * file_descr = supported_file_new_from_path (audio_file_path);
  Position        pos;
  position_set_to_bar (&pos, 4);
  track_create_with_action (
    TrackType::TRACK_TYPE_AUDIO, NULL, file_descr, &pos, TRACKLIST->num_tracks,
    1, -1, NULL, NULL);
  Track * audio_track = tracklist_get_last_track (
    TRACKLIST, TracklistPinOption::TRACKLIST_PIN_OPTION_BOTH, false);
  int audio_track_pos = audio_track->pos;
  (void) audio_track_pos;
  supported_file_free (file_descr);

  /* loop the region */
  Region *         r = audio_track->lanes[0]->regions[0];
  ArrangerObject * r_obj = (ArrangerObject *) r;
  bool             success = arranger_object_resize (
    r_obj, false, ArrangerObjectResizeType::ARRANGER_OBJECT_RESIZE_LOOP, 40000,
    false, NULL);
  g_assert_true (success);
  g_assert_true (arranger_object_validate (r_obj));

  /* start playback */
  transport_request_roll (TRANSPORT, true);
  engine_wait_n_cycles (AUDIO_ENGINE, 3);

  bpm_t bpm_before = tempo_track_get_current_bpm (P_TEMPO_TRACK);

  /* change BPM to 40 */
  transport_action_perform_bpm_change (bpm_before, 40.f, false, NULL);
  g_assert_cmpfloat_with_epsilon (
    tempo_track_get_current_bpm (P_TEMPO_TRACK), 40.f, 0.001f);
  engine_wait_n_cycles (AUDIO_ENGINE, 3);
  g_assert_cmpfloat_with_epsilon (
    tempo_track_get_current_bpm (P_TEMPO_TRACK), 40.f, 0.001f);

  transport_request_roll (TRANSPORT, true);
  engine_wait_n_cycles (AUDIO_ENGINE, 3);

  /* change bpm to 140 */
  transport_action_perform_bpm_change (bpm_before, 140.f, false, NULL);
  g_assert_cmpfloat_with_epsilon (
    tempo_track_get_current_bpm (P_TEMPO_TRACK), 140.f, 0.001f);
  engine_wait_n_cycles (AUDIO_ENGINE, 3);
  g_assert_cmpfloat_with_epsilon (
    tempo_track_get_current_bpm (P_TEMPO_TRACK), 140.f, 0.001f);

  transport_request_roll (TRANSPORT, true);
  engine_wait_n_cycles (AUDIO_ENGINE, 3);

  /* validate */
  g_assert_true (arranger_object_validate (r_obj));

  test_helper_zrythm_cleanup ();
}

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/actions/transport/"

  g_test_add_func (
    TEST_PREFIX "test change BPM twice during playback",
    (GTestFunc) test_change_bpm_twice_during_playback);
  g_test_add_func (
    TEST_PREFIX "test change BPM and time sig",
    (GTestFunc) test_change_bpm_and_time_sig);

  return g_test_run ();
}
