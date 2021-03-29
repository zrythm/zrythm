/*
 * Copyright (C) 2020-2021 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version approved by
 * Alexandros Theodotou in a public statement of acceptance.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "zrythm-test-config.h"

#include "actions/tracklist_selections.h"
#include "actions/undoable_action.h"
#include "audio/audio_region.h"
#include "audio/automation_region.h"
#include "audio/chord_region.h"
#include "audio/control_port.h"
#include "audio/master_track.h"
#include "audio/midi_event.h"
#include "audio/midi_note.h"
#include "audio/region.h"
#include "audio/router.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/color.h"
#include "zrythm.h"

#include "tests/helpers/plugin_manager.h"
#include "tests/helpers/project.h"

#include <glib.h>

static Track *
get_ins_track (void)
{
  return TRACKLIST->tracks[3];
}

static void
_test_edit_tracks (
  EditTracksActionType type,
  const char * pl_bundle,
  const char * pl_uri,
  bool         is_instrument,
  bool         with_carla)
{
  UndoableAction * action;

  PluginSetting * setting =
    test_plugin_manager_get_plugin_setting (
      pl_bundle, pl_uri, with_carla);

  /* create a track with an instrument */
  action =
    tracklist_selections_action_new_create (
      is_instrument ?
        TRACK_TYPE_INSTRUMENT :
        TRACK_TYPE_AUDIO_BUS,
      setting, NULL, 3, NULL, 1);
  undo_manager_perform (UNDO_MANAGER, action);
  Track * ins_track = get_ins_track ();
  if (is_instrument)
    {
      g_assert_true (
        ins_track->type == TRACK_TYPE_INSTRUMENT);

      g_assert_true (
        ins_track->channel->instrument->instantiated);
      g_assert_true (
        ins_track->channel->instrument->activated);
    }
  else
    {
      g_assert_true (
        ins_track->channel->inserts[0]->instantiated);
      g_assert_true (
        ins_track->channel->inserts[0]->activated);
    }

  track_select (
    ins_track, F_SELECT, F_EXCLUSIVE,
    F_NO_PUBLISH_EVENTS);

  UndoableAction * ua = NULL;
  switch (type)
    {
    case EDIT_TRACK_ACTION_TYPE_MUTE:
      {
        ua =
          tracklist_selections_action_new_edit_mute (
            TRACKLIST_SELECTIONS, true);
        undo_manager_perform (
          UNDO_MANAGER, ua);
        if (is_instrument)
          {
            g_assert_true (
              ins_track->channel->instrument->
                instantiated);
            g_assert_true (
              ins_track->channel->instrument->
                activated);
          }
        else
          {
            g_assert_true (
              ins_track->channel->inserts[0]->
                instantiated);
            g_assert_true (
              ins_track->channel->inserts[0]->
                activated);
          }
      }
      break;
    case EDIT_TRACK_ACTION_TYPE_DIRECT_OUT:
      {
        if (!is_instrument)
          break;

        /* create a MIDI track */
        action =
          tracklist_selections_action_new_create (
            TRACK_TYPE_MIDI,
            NULL, NULL, 2, NULL, 1);
        undo_manager_perform (UNDO_MANAGER, action);
        Track * midi_track = TRACKLIST->tracks[2];
        track_select (
          midi_track, F_SELECT, F_EXCLUSIVE,
          F_NO_PUBLISH_EVENTS);

        g_assert_true (
          !midi_track->channel->has_output);

        /* change the direct out to the
         * instrument */
        ua =
          tracklist_selections_action_new_edit_direct_out (
            TRACKLIST_SELECTIONS, ins_track);
        undo_manager_perform (UNDO_MANAGER, ua);

        /* verify direct out established */
        g_assert_true (
          midi_track->channel->has_output);
        g_assert_cmpint (
          midi_track->channel->output_pos, ==,
          ins_track->pos);

        /* undo and re-verify */
        undo_manager_undo (UNDO_MANAGER);

        g_assert_true (
          !midi_track->channel->has_output);

        /* redo and test moving track afterwards */
        undo_manager_redo (UNDO_MANAGER);
        ins_track = TRACKLIST->tracks[4];
        g_assert_true (
          ins_track->type == TRACK_TYPE_INSTRUMENT);
        track_select (
          ins_track, F_SELECT, F_EXCLUSIVE,
          F_NO_PUBLISH_EVENTS);
        ua =
          tracklist_selections_action_new_move (
            TRACKLIST_SELECTIONS, 1);
        undo_manager_perform (UNDO_MANAGER, ua);
        undo_manager_undo (UNDO_MANAGER);

        /* create an audio group track and test
         * routing instrument track to audio
         * group */
        ua =
          tracklist_selections_action_new_create_audio_group (
            TRACKLIST->num_tracks, 1);
        undo_manager_perform (UNDO_MANAGER, ua);
        undo_manager_undo (UNDO_MANAGER);
        undo_manager_redo (UNDO_MANAGER);
        Track * audio_group =
          TRACKLIST->tracks[
            TRACKLIST->num_tracks - 1];
        track_select (
          ins_track, F_SELECT, F_EXCLUSIVE,
          F_NO_PUBLISH_EVENTS);
        ua =
          tracklist_selections_action_new_edit_direct_out (
            TRACKLIST_SELECTIONS, audio_group);
        undo_manager_perform (UNDO_MANAGER, ua);
        undo_manager_undo (UNDO_MANAGER);
        undo_manager_redo (UNDO_MANAGER);
        undo_manager_undo (UNDO_MANAGER);
        undo_manager_undo (UNDO_MANAGER);
      }
      break;
    case EDIT_TRACK_ACTION_TYPE_SOLO:
      {
        if (!is_instrument)
          break;

        /* create an audio group track */
        action =
          tracklist_selections_action_new_create (
            TRACK_TYPE_AUDIO_GROUP,
            NULL, NULL, 2, NULL, 1);
        undo_manager_perform (UNDO_MANAGER, action);
        Track * group_track = TRACKLIST->tracks[2];

        g_assert_cmpint (
          ins_track->channel->track_pos, !=,
          ins_track->channel->output_pos);

        /* route the instrument to the group
         * track */
        track_select (
          ins_track, F_SELECT, F_EXCLUSIVE,
          F_NO_PUBLISH_EVENTS);
        ua =
          tracklist_selections_action_new_edit_direct_out (
            TRACKLIST_SELECTIONS, group_track);
        undo_manager_perform (UNDO_MANAGER, ua);

        /* solo the group track */
        track_select (
          group_track, F_SELECT, F_EXCLUSIVE,
          F_NO_PUBLISH_EVENTS);
        ua =
          tracklist_selections_action_new_edit_solo (
            TRACKLIST_SELECTIONS, true);
        undo_manager_perform (UNDO_MANAGER, ua);

        /* run the engine for 1 cycle to clear any
         * pending events */
        engine_process (
          AUDIO_ENGINE, AUDIO_ENGINE->block_length);

        /* play a note on the instrument track
         * and verify that signal comes out
         * in both tracks */
        midi_events_add_note_on (
          ins_track->processor->midi_in->midi_events,
          1, 62, 74, 2, true);

        bool has_signal = false;

        /* run the engine for 2 cycles (the event
         * does not have enough time to make audible
         * sound in 1 cycle */
        engine_process (
          AUDIO_ENGINE, AUDIO_ENGINE->block_length);
        engine_process (
          AUDIO_ENGINE, AUDIO_ENGINE->block_length);

        Port * l =
          ins_track->channel->fader->stereo_out->l;
#if 0
        Port * ins_out_l =
          plugin_get_port_by_symbol (
          ins_track->channel->instrument,
          /*"lv2_audio_out_1");*/
          "lv2_audio_out_1");
#endif
        for (nframes_t i = 0;
             i < AUDIO_ENGINE->block_length; i++)
          {
#if 0
            g_message (
              "[%u] %.16f", i, (double) l->buf[i]);
            g_message (
              "[%u i] %.16f", i,
              (double) ins_out_l->buf[i]);
#endif
            if (l->buf[i] > 0.0001f)
              {
                has_signal = true;
                break;
              }
          }
        g_assert_true (has_signal);

        /* undo and re-verify */
        undo_manager_undo (UNDO_MANAGER);
      }
      break;
    case EDIT_TRACK_ACTION_TYPE_RENAME:
      {
        const char * new_name = "new name";
        char * name_before =
          g_strdup (ins_track->name);
        track_set_name_with_action (
          ins_track, new_name);
        g_assert_true (
          string_is_equal (
            ins_track->name, new_name));

        /* undo/redo and re-verify */
        undo_manager_undo (UNDO_MANAGER);
        g_assert_true (
          string_is_equal (
            ins_track->name, name_before));
        undo_manager_redo (UNDO_MANAGER);
        g_assert_true (
          string_is_equal (
            ins_track->name, new_name));

        g_free (name_before);

        /* undo to go back to original state */
        undo_manager_undo (UNDO_MANAGER);
      }
      break;
    case EDIT_TRACK_ACTION_TYPE_VOLUME:
    case EDIT_TRACK_ACTION_TYPE_PAN:
      {
        float new_val = 0.23f;
        float val_before =
          fader_get_amp (
            ins_track->channel->fader);
        if (type == EDIT_TRACK_ACTION_TYPE_PAN)
          {
            val_before =
              channel_get_balance_control (
                ins_track->channel);
          }
        ua =
          tracklist_selections_action_new_edit_single_float (
            type,
            ins_track, val_before, new_val,
            false);
        undo_manager_perform (UNDO_MANAGER, ua);

        /* verify */
        if (type == EDIT_TRACK_ACTION_TYPE_PAN)
          {
            g_assert_cmpfloat_with_epsilon (
              new_val,
              channel_get_balance_control (
                ins_track->channel),
              0.0001f);
          }
        else if (type ==
                   EDIT_TRACK_ACTION_TYPE_VOLUME)
          {
            g_assert_cmpfloat_with_epsilon (
              new_val,
              fader_get_amp (
                ins_track->channel->fader),
              0.0001f);
          }

        /* undo/redo and re-verify */
        undo_manager_undo (UNDO_MANAGER);
        if (type == EDIT_TRACK_ACTION_TYPE_PAN)
          {
            g_assert_cmpfloat_with_epsilon (
              val_before,
              channel_get_balance_control (
                ins_track->channel),
              0.0001f);
          }
        else if (type ==
                   EDIT_TRACK_ACTION_TYPE_VOLUME)
          {
            g_assert_cmpfloat_with_epsilon (
              val_before,
              fader_get_amp (
                ins_track->channel->fader),
              0.0001f);
          }
        undo_manager_redo (UNDO_MANAGER);
        if (type == EDIT_TRACK_ACTION_TYPE_PAN)
          {
            g_assert_cmpfloat_with_epsilon (
              new_val,
              channel_get_balance_control (
                ins_track->channel),
              0.0001f);
          }
        else if (type ==
                   EDIT_TRACK_ACTION_TYPE_VOLUME)
          {
            g_assert_cmpfloat_with_epsilon (
              new_val,
              fader_get_amp (
                ins_track->channel->fader),
              0.0001f);
          }

        /* undo to go back to original state */
        undo_manager_undo (UNDO_MANAGER);
      }
      break;
    case EDIT_TRACK_ACTION_TYPE_COLOR:
      {
        GdkRGBA new_color = {
          .red = 0.8, .green = 0.7,
          .blue = 0.2, .alpha = 1.0 };
        GdkRGBA color_before = ins_track->color;
        track_set_color (
          ins_track, &new_color, F_UNDOABLE,
          F_NO_PUBLISH_EVENTS);
        g_assert_true (
          color_is_same (
            &ins_track->color, &new_color));

        test_project_save_and_reload ();

        ins_track = get_ins_track ();
        g_assert_true (
          color_is_same (
            &ins_track->color, &new_color));

        /* undo/redo and re-verify */
        undo_manager_undo (UNDO_MANAGER);
        g_assert_true (
          color_is_same (
            &ins_track->color, &color_before));
        undo_manager_redo (UNDO_MANAGER);
        g_assert_true (
          color_is_same (
            &ins_track->color, &new_color));

        /* undo to go back to original state */
        undo_manager_undo (UNDO_MANAGER);
      }
      break;
    case EDIT_TRACK_ACTION_TYPE_ICON:
      {
        const char * new_icon = "icon2";
        char * icon_before =
          g_strdup (ins_track->icon_name);
        track_set_icon (
          ins_track, new_icon, F_UNDOABLE,
          F_NO_PUBLISH_EVENTS);
        g_assert_true (
          string_is_equal (
            ins_track->icon_name, new_icon));

        test_project_save_and_reload ();

        ins_track = get_ins_track ();

        /* undo/redo and re-verify */
        undo_manager_undo (UNDO_MANAGER);
        g_assert_true (
          string_is_equal (
            ins_track->icon_name, icon_before));
        undo_manager_redo (UNDO_MANAGER);
        g_assert_true (
          string_is_equal (
            ins_track->icon_name, new_icon));

        /* undo to go back to original state */
        undo_manager_undo (UNDO_MANAGER);

        g_free (icon_before);
      }
      break;
    case EDIT_TRACK_ACTION_TYPE_COMMENT:
      {
        const char * new_icon = "icon2";
        char * icon_before =
          g_strdup (ins_track->comment);
        track_set_comment (
          ins_track, new_icon, F_UNDOABLE);
        g_assert_true (
          string_is_equal (
            ins_track->comment, new_icon));

        test_project_save_and_reload ();

        ins_track = get_ins_track ();

        /* undo/redo and re-verify */
        undo_manager_undo (UNDO_MANAGER);
        g_assert_true (
          string_is_equal (
            ins_track->comment, icon_before));
        undo_manager_redo (UNDO_MANAGER);
        g_assert_true (
          string_is_equal (
            ins_track->comment, new_icon));

        /* undo to go back to original state */
        undo_manager_undo (UNDO_MANAGER);

        g_free (icon_before);
      }
      break;
    default:
      break;
    }
}

static void __test_edit_tracks (bool with_carla)
{
  for (EditTracksActionType i =
         EDIT_TRACK_ACTION_TYPE_SOLO;
       i <= EDIT_TRACK_ACTION_TYPE_ICON; i++)
    {
      test_helper_zrythm_init ();

      /* stop dummy audio engine processing so we can
       * process manually */
      AUDIO_ENGINE->stop_dummy_audio_thread = true;
      g_usleep (1000000);

#ifdef HAVE_CHIPWAVE
      _test_edit_tracks (
        i, CHIPWAVE_BUNDLE, CHIPWAVE_URI, true,
        with_carla);
#endif
#ifdef HAVE_HELM
      _test_edit_tracks (
        i, HELM_BUNDLE, HELM_URI, true, with_carla);
#endif
#ifdef HAVE_LSP_COMPRESSOR
      _test_edit_tracks (
        i, LSP_COMPRESSOR_BUNDLE, LSP_COMPRESSOR_URI,
        false, with_carla);
#endif

      test_helper_zrythm_cleanup ();
    }
}

static void test_edit_tracks ()
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
  Track * ins_track =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1];
  track_select (
    ins_track, F_SELECT, F_EXCLUSIVE,
    F_NO_PUBLISH_EVENTS);

  char ** midi_files =
    io_get_files_in_dir_ending_in (
      MIDILIB_TEST_MIDI_FILES_PATH,
      F_RECURSIVE, ".MID", false);
  g_assert_nonnull (midi_files);

  /* create the MIDI track from a MIDI file */
  SupportedFile * file =
    supported_file_new_from_path (midi_files[0]);
  UndoableAction * ua =
    tracklist_selections_action_new_create (
      TRACK_TYPE_MIDI, NULL, file,
      TRACKLIST->num_tracks, PLAYHEAD, 1);
  undo_manager_perform (UNDO_MANAGER, ua);
  Track * midi_track =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1];
  track_select (
    midi_track, F_SELECT, F_EXCLUSIVE,
    F_NO_PUBLISH_EVENTS);
  g_strfreev (midi_files);

  /* route the MIDI track to the instrument track */
  ua =
    tracklist_selections_action_new_edit_direct_out (
      TRACKLIST_SELECTIONS, ins_track);
  undo_manager_perform (UNDO_MANAGER, ua);

  Channel * ch = midi_track->channel;
  Track * direct_out =
    channel_get_output_track (ch);
  g_assert_true (IS_TRACK (direct_out));
  g_assert_true (direct_out == ins_track);
  g_assert_cmpint (ins_track->num_children, ==, 1);

  /* delete the instrument, undo and verify that
   * the MIDI track's output is the instrument */
  track_select (
    ins_track, F_SELECT, F_EXCLUSIVE,
    F_NO_PUBLISH_EVENTS);

  ua =
    tracklist_selections_action_new_delete (
      TRACKLIST_SELECTIONS);
  undo_manager_perform (UNDO_MANAGER, ua);

  direct_out =
    channel_get_output_track (ch);
  g_assert_null (direct_out);

  undo_manager_undo (UNDO_MANAGER);

  ins_track =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 2];
  direct_out =
    channel_get_output_track (ch);
  g_assert_true (IS_TRACK (direct_out));
  g_assert_true (direct_out == ins_track);

  test_helper_zrythm_cleanup ();
#endif
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/actions/tracklist_selections_edit/"

  g_test_add_func (
    TEST_PREFIX "test edit midi direct out to ins",
    (GTestFunc) test_edit_midi_direct_out_to_ins);
  g_test_add_func (
    TEST_PREFIX "test_edit_tracks",
    (GTestFunc) test_edit_tracks);

  return g_test_run ();
}

