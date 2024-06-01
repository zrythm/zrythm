// SPDX-FileCopyrightText: © 2020-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "actions/tracklist_selections.h"
#include "actions/undoable_action.h"
#include "dsp/audio_region.h"
#include "dsp/automation_region.h"
#include "dsp/chord_region.h"
#include "dsp/control_port.h"
#include "dsp/master_track.h"
#include "dsp/midi_event.h"
#include "dsp/midi_note.h"
#include "dsp/region.h"
#include "dsp/router.h"
#include "project.h"
#include "utils/color.h"
#include "utils/flags.h"
#include "zrythm.h"

#include <glib.h>

#include "tests/helpers/plugin_manager.h"
#include "tests/helpers/project_helper.h"

#if defined(HAVE_CHIPWAVE) || defined(HAVE_HELM) || defined(HAVE_LSP_COMPRESSOR)
static Track *
get_ins_track (void)
{
  return TRACKLIST->tracks[3];
}

static void
_test_edit_tracks (
  EditTrackActionType type,
  const char *        pl_bundle,
  const char *        pl_uri,
  bool                is_instrument,
  bool                with_carla)
{
  PluginSetting * setting =
    test_plugin_manager_get_plugin_setting (pl_bundle, pl_uri, with_carla);

  /* create a track with an instrument */
  track_create_for_plugin_at_idx_w_action (
    is_instrument
      ? TrackType::TRACK_TYPE_INSTRUMENT
      : TrackType::TRACK_TYPE_AUDIO_BUS,
    setting, 3, NULL);
  Track * ins_track = get_ins_track ();
  if (is_instrument)
    {
      g_assert_true (ins_track->type == TrackType::TRACK_TYPE_INSTRUMENT);

      g_assert_true (ins_track->channel->instrument->instantiated);
      g_assert_true (ins_track->channel->instrument->activated);
    }
  else
    {
      g_assert_true (ins_track->channel->inserts[0]->instantiated);
      g_assert_true (ins_track->channel->inserts[0]->activated);
    }

  track_select (ins_track, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);

  UndoableAction * ua = NULL;
  switch (type)
    {
    case EditTrackActionType::EDIT_TRACK_ACTION_TYPE_MUTE:
      {
        tracklist_selections_action_perform_edit_mute (
          TRACKLIST_SELECTIONS, true, NULL);
        if (is_instrument)
          {
            g_assert_true (ins_track->channel->instrument->instantiated);
            g_assert_true (ins_track->channel->instrument->activated);
          }
        else
          {
            g_assert_true (ins_track->channel->inserts[0]->instantiated);
            g_assert_true (ins_track->channel->inserts[0]->activated);
          }
      }
      break;
    case EditTrackActionType::EDIT_TRACK_ACTION_TYPE_DIRECT_OUT:
      {
        if (!is_instrument)
          break;

        /* create a MIDI track */
        track_create_empty_at_idx_with_action (
          TrackType::TRACK_TYPE_MIDI, 2, NULL);
        Track * midi_track = TRACKLIST->tracks[2];
        track_select (midi_track, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);

        g_assert_true (!midi_track->channel->has_output);

        /* change the direct out to the
         * instrument */
        tracklist_selections_action_perform_set_direct_out (
          TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, ins_track, NULL);

        /* verify direct out established */
        g_assert_true (midi_track->channel->has_output);
        g_assert_cmpuint (
          midi_track->channel->output_name_hash, ==,
          track_get_name_hash (*ins_track));

        /* undo and re-verify */
        undo_manager_undo (UNDO_MANAGER, NULL);

        g_assert_true (!midi_track->channel->has_output);

        /* redo and test moving track afterwards */
        undo_manager_redo (UNDO_MANAGER, NULL);
        ins_track = TRACKLIST->tracks[4];
        g_assert_true (ins_track->type == TrackType::TRACK_TYPE_INSTRUMENT);
        track_select (ins_track, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
        tracklist_selections_action_perform_move (
          TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, 1, NULL);
        undo_manager_undo (UNDO_MANAGER, NULL);

        /* create an audio group track and test
         * routing instrument track to audio
         * group */
        track_create_empty_with_action (TrackType::TRACK_TYPE_AUDIO_GROUP, NULL);
        undo_manager_undo (UNDO_MANAGER, NULL);
        undo_manager_redo (UNDO_MANAGER, NULL);
        Track * audio_group = TRACKLIST->tracks[TRACKLIST->tracks.size () - 1];
        track_select (ins_track, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
        tracklist_selections_action_perform_set_direct_out (
          TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, audio_group, NULL);
        undo_manager_undo (UNDO_MANAGER, NULL);
        undo_manager_redo (UNDO_MANAGER, NULL);
        undo_manager_undo (UNDO_MANAGER, NULL);
        undo_manager_undo (UNDO_MANAGER, NULL);
      }
      break;
    case EditTrackActionType::EDIT_TRACK_ACTION_TYPE_SOLO:
      {
        if (!is_instrument)
          break;

        /* create an audio group track */
        track_create_empty_at_idx_with_action (
          TrackType::TRACK_TYPE_AUDIO_GROUP, 2, NULL);
        Track * group_track = TRACKLIST->tracks[2];

        g_assert_cmpuint (
          track_get_name_hash (*ins_track), !=,
          ins_track->channel->output_name_hash);

        /* route the instrument to the group
         * track */
        track_select (ins_track, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
        tracklist_selections_action_perform_set_direct_out (
          TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, group_track, NULL);

        /* solo the group track */
        track_select (group_track, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
        tracklist_selections_action_perform_edit_solo (
          TRACKLIST_SELECTIONS, true, NULL);

        /* run the engine for 1 cycle to clear any
         * pending events */
        engine_process (AUDIO_ENGINE, AUDIO_ENGINE->block_length);

        /* play a note on the instrument track
         * and verify that signal comes out
         * in both tracks */
        midi_events_add_note_on (
          ins_track->processor->midi_in->midi_events_, 1, 62, 74, 2, true);

        bool has_signal = false;

        /* run the engine for 2 cycles (the event
         * does not have enough time to make audible
         * sound in 1 cycle */
        engine_process (AUDIO_ENGINE, AUDIO_ENGINE->block_length);
        engine_process (AUDIO_ENGINE, AUDIO_ENGINE->block_length);

        Port &l = ins_track->channel->fader->stereo_out->get_l ();
#  if 0
        Port * ins_out_l =
          plugin_get_port_by_symbol (
          ins_track->channel->instrument,
          /*"lv2_audio_out_1");*/
          "lv2_audio_out_1");
#  endif
        for (nframes_t i = 0; i < AUDIO_ENGINE->block_length; i++)
          {
#  if 0
            g_message (
              "[%u] %.16f", i, (double) l->buf[i]);
            g_message (
              "[%u i] %.16f", i,
              (double) ins_out_l->buf[i]);
#  endif
            if (l.buf_[i] > 0.0001f)
              {
                has_signal = true;
                break;
              }
          }
        g_assert_true (has_signal);

        /* undo and re-verify */
        undo_manager_undo (UNDO_MANAGER, NULL);
      }
      break;
    case EditTrackActionType::EDIT_TRACK_ACTION_TYPE_RENAME:
      {
        const char * new_name = "new name";
        char *       name_before = g_strdup (ins_track->name);
        track_set_name_with_action (ins_track, new_name);
        g_assert_true (string_is_equal (ins_track->name, new_name));

        /* undo/redo and re-verify */
        undo_manager_undo (UNDO_MANAGER, NULL);
        g_assert_true (string_is_equal (ins_track->name, name_before));
        undo_manager_redo (UNDO_MANAGER, NULL);
        g_assert_true (string_is_equal (ins_track->name, new_name));

        g_free (name_before);

        /* undo to go back to original state */
        undo_manager_undo (UNDO_MANAGER, NULL);
      }
      break;
    case EditTrackActionType::EDIT_TRACK_ACTION_TYPE_RENAME_LANE:
      {
        const char * new_name = "new name";
        TrackLane *  lane = ins_track->lanes[0];
        char *       name_before = g_strdup (lane->name);
        track_lane_rename (lane, new_name, true);
        g_assert_true (string_is_equal (lane->name, new_name));

        /* undo/redo and re-verify */
        undo_manager_undo (UNDO_MANAGER, NULL);
        g_assert_true (string_is_equal (lane->name, name_before));
        undo_manager_redo (UNDO_MANAGER, NULL);
        g_assert_true (string_is_equal (lane->name, new_name));

        g_free (name_before);

        /* undo to go back to original state */
        undo_manager_undo (UNDO_MANAGER, NULL);
      }
      break;
    case EditTrackActionType::EDIT_TRACK_ACTION_TYPE_VOLUME:
    case EditTrackActionType::EDIT_TRACK_ACTION_TYPE_PAN:
      {
        float new_val = 0.23f;
        float val_before = fader_get_amp (ins_track->channel->fader);
        if (type == EditTrackActionType::EDIT_TRACK_ACTION_TYPE_PAN)
          {
            val_before = ins_track->channel->get_balance_control ();
          }
        GError * err = NULL;
        ua = tracklist_selections_action_new_edit_single_float (
          type, ins_track, val_before, new_val, false, &err);
        g_assert_nonnull (ua);
        undo_manager_perform (UNDO_MANAGER, ua, NULL);

        /* verify */
        if (type == EditTrackActionType::EDIT_TRACK_ACTION_TYPE_PAN)
          {
            g_assert_cmpfloat_with_epsilon (
              new_val, ins_track->channel->get_balance_control (), 0.0001f);
          }
        else if (type == EditTrackActionType::EDIT_TRACK_ACTION_TYPE_VOLUME)
          {
            g_assert_cmpfloat_with_epsilon (
              new_val, fader_get_amp (ins_track->channel->fader), 0.0001f);
          }

        /* undo/redo and re-verify */
        undo_manager_undo (UNDO_MANAGER, NULL);
        if (type == EditTrackActionType::EDIT_TRACK_ACTION_TYPE_PAN)
          {
            g_assert_cmpfloat_with_epsilon (
              val_before, ins_track->channel->get_balance_control (), 0.0001f);
          }
        else if (type == EditTrackActionType::EDIT_TRACK_ACTION_TYPE_VOLUME)
          {
            g_assert_cmpfloat_with_epsilon (
              val_before, fader_get_amp (ins_track->channel->fader), 0.0001f);
          }
        undo_manager_redo (UNDO_MANAGER, NULL);
        if (type == EditTrackActionType::EDIT_TRACK_ACTION_TYPE_PAN)
          {
            g_assert_cmpfloat_with_epsilon (
              new_val, ins_track->channel->get_balance_control (), 0.0001f);
          }
        else if (type == EditTrackActionType::EDIT_TRACK_ACTION_TYPE_VOLUME)
          {
            g_assert_cmpfloat_with_epsilon (
              new_val, fader_get_amp (ins_track->channel->fader), 0.0001f);
          }

        /* undo to go back to original state */
        undo_manager_undo (UNDO_MANAGER, NULL);
      }
      break;
    case EditTrackActionType::EDIT_TRACK_ACTION_TYPE_COLOR:
      {
        GdkRGBA new_color = { 0.8f, 0.7f, 0.2f, 1.f };
        GdkRGBA color_before = ins_track->color;
        track_set_color (ins_track, &new_color, F_UNDOABLE, F_NO_PUBLISH_EVENTS);
        g_assert_true (color_is_same (&ins_track->color, &new_color));

        test_project_save_and_reload ();

        ins_track = get_ins_track ();
        g_assert_true (color_is_same (&ins_track->color, &new_color));

        /* undo/redo and re-verify */
        undo_manager_undo (UNDO_MANAGER, NULL);
        g_assert_true (color_is_same (&ins_track->color, &color_before));
        undo_manager_redo (UNDO_MANAGER, NULL);
        g_assert_true (color_is_same (&ins_track->color, &new_color));

        /* undo to go back to original state */
        undo_manager_undo (UNDO_MANAGER, NULL);
      }
      break;
    case EditTrackActionType::EDIT_TRACK_ACTION_TYPE_ICON:
      {
        const char * new_icon = "icon2";
        char *       icon_before = g_strdup (ins_track->icon_name);
        track_set_icon (ins_track, new_icon, F_UNDOABLE, F_NO_PUBLISH_EVENTS);
        g_assert_true (string_is_equal (ins_track->icon_name, new_icon));

        test_project_save_and_reload ();

        ins_track = get_ins_track ();

        /* undo/redo and re-verify */
        undo_manager_undo (UNDO_MANAGER, NULL);
        g_assert_true (string_is_equal (ins_track->icon_name, icon_before));
        undo_manager_redo (UNDO_MANAGER, NULL);
        g_assert_true (string_is_equal (ins_track->icon_name, new_icon));

        /* undo to go back to original state */
        undo_manager_undo (UNDO_MANAGER, NULL);

        g_free (icon_before);
      }
      break;
    case EditTrackActionType::EDIT_TRACK_ACTION_TYPE_COMMENT:
      {
        const char * new_icon = "icon2";
        char *       icon_before = g_strdup (ins_track->comment);
        track_set_comment (ins_track, new_icon, F_UNDOABLE);
        g_assert_true (string_is_equal (ins_track->comment, new_icon));

        test_project_save_and_reload ();

        ins_track = get_ins_track ();

        /* undo/redo and re-verify */
        undo_manager_undo (UNDO_MANAGER, NULL);
        g_assert_true (string_is_equal (ins_track->comment, icon_before));
        undo_manager_redo (UNDO_MANAGER, NULL);
        g_assert_true (string_is_equal (ins_track->comment, new_icon));

        /* undo to go back to original state */
        undo_manager_undo (UNDO_MANAGER, NULL);

        g_free (icon_before);
      }
      break;
    default:
      break;
    }
}
#endif

static void
__test_edit_tracks (bool with_carla)
{
  for (
    size_t i =
      ENUM_VALUE_TO_INT (EditTrackActionType::EDIT_TRACK_ACTION_TYPE_SOLO);
    i <= ENUM_VALUE_TO_INT (EditTrackActionType::EDIT_TRACK_ACTION_TYPE_ICON);
    i++)
    {
      EditTrackActionType cur = ENUM_INT_TO_VALUE (EditTrackActionType, i);
      (void) cur;
      test_helper_zrythm_init ();

      /* stop dummy audio engine processing so we can
       * process manually */
      AUDIO_ENGINE->stop_dummy_audio_thread = true;
      g_usleep (1000000);

#ifdef HAVE_CHIPWAVE
      _test_edit_tracks (cur, CHIPWAVE_BUNDLE, CHIPWAVE_URI, true, with_carla);
#endif
#ifdef HAVE_HELM
      _test_edit_tracks (cur, HELM_BUNDLE, HELM_URI, true, with_carla);
#endif
#ifdef HAVE_LSP_COMPRESSOR
      _test_edit_tracks (
        cur, LSP_COMPRESSOR_BUNDLE, LSP_COMPRESSOR_URI, false, with_carla);
#endif

      test_helper_zrythm_cleanup ();
    }
}

static void
test_edit_tracks (void)
{
  __test_edit_tracks (false);
#ifdef HAVE_CARLA
  __test_edit_tracks (true);
#endif
}

static void
test_edit_midi_direct_out_to_ins (void)
{
#ifdef HAVE_HELM
  test_helper_zrythm_init ();

  /* create the instrument track */
  test_plugin_manager_create_tracks_from_plugin (
    HELM_BUNDLE, HELM_URI, true, false, 1);
  Track * ins_track = TRACKLIST->tracks[TRACKLIST->tracks.size () - 1];
  track_select (ins_track, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);

  char ** midi_files = io_get_files_in_dir_ending_in (
    MIDILIB_TEST_MIDI_FILES_PATH, F_RECURSIVE, ".MID", false);
  g_assert_nonnull (midi_files);

  /* create the MIDI track from a MIDI file */
  SupportedFile * file = supported_file_new_from_path (midi_files[0]);
  track_create_with_action (
    TrackType::TRACK_TYPE_MIDI, NULL, file, PLAYHEAD, TRACKLIST->tracks.size (),
    1, -1, NULL, NULL);
  Track * midi_track = TRACKLIST->tracks[TRACKLIST->tracks.size () - 1];
  track_select (midi_track, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  g_strfreev (midi_files);

  /* route the MIDI track to the instrument track */
  tracklist_selections_action_perform_set_direct_out (
    TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, ins_track, NULL);

  Channel * ch = midi_track->channel;
  Track *   direct_out = ch->get_output_track ();
  g_assert_true (IS_TRACK (direct_out));
  g_assert_true (direct_out == ins_track);
  g_assert_cmpint (ins_track->num_children, ==, 1);

  /* delete the instrument, undo and verify that
   * the MIDI track's output is the instrument */
  track_select (ins_track, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);

  tracklist_selections_action_perform_delete (
    TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, NULL);

  direct_out = ch->get_output_track ();
  g_assert_null (direct_out);

  undo_manager_undo (UNDO_MANAGER, NULL);

  ins_track = TRACKLIST->tracks[TRACKLIST->tracks.size () - 2];
  direct_out = ch->get_output_track ();
  g_assert_true (IS_TRACK (direct_out));
  g_assert_true (direct_out == ins_track);

  test_helper_zrythm_cleanup ();
#endif
}

static void
test_edit_multi_track_direct_out (void)
{
#ifdef HAVE_HELM
  test_helper_zrythm_init ();

  /* create 2 instrument tracks */
  test_plugin_manager_create_tracks_from_plugin (
    HELM_BUNDLE, HELM_URI, true, false, 2);
  Track * ins_track = TRACKLIST->tracks[TRACKLIST->tracks.size () - 2];
  Track * ins_track2 = TRACKLIST->tracks[TRACKLIST->tracks.size () - 1];

  /* create an audio group */
  Track * audio_group =
    track_create_empty_with_action (TrackType::TRACK_TYPE_AUDIO_GROUP, NULL);

  /* route the ins tracks to the audio group */
  track_select (ins_track, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  track_select (ins_track2, F_SELECT, F_NOT_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  tracklist_selections_action_perform_set_direct_out (
    TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, audio_group, NULL);

  /* change the name of the group track - tests
   * track_update_children() */
  tracklist_selections_action_perform_edit_rename (
    audio_group, PORT_CONNECTIONS_MGR, "new name", NULL);

  Channel * ch = ins_track->channel;
  Channel * ch2 = ins_track2->channel;
  Track *   direct_out = ch->get_output_track ();
  Track *   direct_out2 = ch2->get_output_track ();
  g_assert_true (IS_TRACK (direct_out));
  g_assert_true (IS_TRACK (direct_out2));
  g_assert_true (direct_out == audio_group);
  g_assert_true (direct_out2 == audio_group);
  g_assert_cmpint (audio_group->num_children, ==, 2);

  /* delete the audio group, undo and verify that
   * the ins track output is the audio group */
  track_select (audio_group, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  tracklist_selections_action_perform_delete (
    TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, NULL);

  direct_out = ch->get_output_track ();
  direct_out2 = ch2->get_output_track ();
  g_assert_null (direct_out);
  g_assert_null (direct_out2);

  /* as a second action, route the tracks to
   * master */
  track_select (ins_track, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  track_select (ins_track2, F_SELECT, F_NOT_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  tracklist_selections_action_perform_set_direct_out (
    TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, P_MASTER_TRACK, NULL);
  UndoableAction * ua = undo_manager_get_last_action (UNDO_MANAGER);
  undoable_action_set_num_actions (ua, 2);

  direct_out = ch->get_output_track ();
  direct_out2 = ch2->get_output_track ();
  g_assert_true (direct_out == P_MASTER_TRACK);
  g_assert_true (direct_out2 == P_MASTER_TRACK);

  undo_manager_undo (UNDO_MANAGER, NULL);

  audio_group = TRACKLIST->tracks[TRACKLIST->tracks.size () - 1];
  direct_out = ch->get_output_track ();
  direct_out2 = ch2->get_output_track ();
  g_assert_true (IS_TRACK (direct_out));
  g_assert_true (IS_TRACK (direct_out2));
  g_assert_true (direct_out == audio_group);
  g_assert_true (direct_out2 == audio_group);

  test_helper_zrythm_cleanup ();
#endif
}

static void
test_rename_midi_track_with_events (void)
{
  test_helper_zrythm_init ();

  /* create a MIDI track from a file */
  char ** midi_files = io_get_files_in_dir_ending_in (
    MIDILIB_TEST_MIDI_FILES_PATH, F_RECURSIVE, ".MID", false);
  g_assert_nonnull (midi_files);
  SupportedFile * file = supported_file_new_from_path (midi_files[0]);
  track_create_with_action (
    TrackType::TRACK_TYPE_MIDI, NULL, file, PLAYHEAD, TRACKLIST->tracks.size (),
    1, -1, NULL, NULL);
  Track * midi_track = tracklist_get_last_track (
    TRACKLIST, TracklistPinOption::TRACKLIST_PIN_OPTION_BOTH, false);
  track_select (midi_track, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  g_strfreev (midi_files);

  /* change the name of the track */
  tracklist_selections_action_perform_edit_rename (
    midi_track, PORT_CONNECTIONS_MGR, "new name", NULL);

  tracklist_validate (TRACKLIST);

  /* play and let engine run */
  transport_request_roll (TRANSPORT, true);
  engine_wait_n_cycles (AUDIO_ENGINE, 3);

  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_redo (UNDO_MANAGER, NULL);

  /* duplicate and let engine run */
  tracklist_selections_action_perform_copy (
    TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, TRACKLIST->tracks.size (), NULL);

  /* play and let engine run */
  transport_request_roll (TRANSPORT, true);
  engine_wait_n_cycles (AUDIO_ENGINE, 3);

  test_helper_zrythm_cleanup ();
}

static void
test_rename_track_with_send (void)
{
  test_helper_zrythm_init ();

  /* create an audio group */
  Track * audio_group =
    track_create_empty_with_action (TrackType::TRACK_TYPE_AUDIO_GROUP, NULL);

  /* create an audio fx */
  Track * audio_fx =
    track_create_empty_with_action (TrackType::TRACK_TYPE_AUDIO_BUS, NULL);

  /* send from group to fx */
  channel_send_action_perform_connect_audio (
    audio_group->channel->sends[0], audio_fx->processor->stereo_in, NULL);

  /* change the name of the fx track */
  tracklist_selections_action_perform_edit_rename (
    audio_fx, PORT_CONNECTIONS_MGR, "new name", NULL);

  tracklist_validate (TRACKLIST);

  /* play and let engine run */
  transport_request_roll (TRANSPORT, true);
  engine_wait_n_cycles (AUDIO_ENGINE, 3);

  /* change the name of the group track */
  tracklist_selections_action_perform_edit_rename (
    audio_group, PORT_CONNECTIONS_MGR, "new name2", NULL);

  tracklist_validate (TRACKLIST);

  /* play and let engine run */
  transport_request_roll (TRANSPORT, true);
  engine_wait_n_cycles (AUDIO_ENGINE, 3);

  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_redo (UNDO_MANAGER, NULL);
  undo_manager_redo (UNDO_MANAGER, NULL);

  test_helper_zrythm_cleanup ();
}

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/actions/tracklist_selections_edit/"

  g_test_add_func (
    TEST_PREFIX "test rename midi track with events",
    (GTestFunc) test_rename_midi_track_with_events);
  g_test_add_func (
    TEST_PREFIX "test rename track with send",
    (GTestFunc) test_rename_track_with_send);
  g_test_add_func (
    TEST_PREFIX "test edit multi track direct out",
    (GTestFunc) test_edit_multi_track_direct_out);
  g_test_add_func (TEST_PREFIX "test_edit_tracks", (GTestFunc) test_edit_tracks);
  g_test_add_func (
    TEST_PREFIX "test edit midi direct out to ins",
    (GTestFunc) test_edit_midi_direct_out_to_ins);

  return g_test_run ();
}
