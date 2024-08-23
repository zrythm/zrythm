// SPDX-FileCopyrightText: © 2020-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

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

#include "tests/helpers/plugin_manager.h"
#include "tests/helpers/project_helper.h"

TEST_SUITE_BEGIN ("actions/tracklist selections edit");

#if defined(HAVE_CHIPWAVE) || defined(HAVE_HELM) || defined(HAVE_LSP_COMPRESSOR)
static InstrumentTrack *
get_ins_track (void)
{
  return TRACKLIST->get_track<InstrumentTrack> (3);
}

static void
_test_edit_tracks (
  TracklistSelectionsAction::EditType type,
  const char *                        pl_bundle,
  const char *                        pl_uri,
  bool                                is_instrument,
  bool                                with_carla)
{
  auto setting =
    test_plugin_manager_get_plugin_setting (pl_bundle, pl_uri, with_carla);

  /* create a track with an instrument */
  Track::create_for_plugin_at_idx_w_action (
    is_instrument ? Track::Type::Instrument : Track::Type::AudioBus, &setting,
    3);
  auto ins_track = get_ins_track ();
  if (is_instrument)
    {
      REQUIRE_EQ (ins_track->type_, Track::Type::Instrument);

      REQUIRE (ins_track->channel_->instrument_->instantiated_);
      REQUIRE (ins_track->channel_->instrument_->activated_);
    }
  else
    {
      REQUIRE (ins_track->channel_->inserts_[0]->instantiated_);
      REQUIRE (ins_track->channel_->inserts_[0]->activated_);
    }

  ins_track->select (true, true, false);

  UndoableAction * ua = NULL;
  switch (type)
    {
    case EditTrackActionType::EDIT_TRACK_ACTION_TYPE_MUTE:
      {
        tracklist_selections_action_perform_edit_mute (
          TRACKLIST_SELECTIONS, true, nullptr);
        if (is_instrument)
          {
            REQUIRE (ins_track->channel->instrument->instantiated);
            REQUIRE (ins_track->channel->instrument->activated);
          }
        else
          {
            REQUIRE (ins_track->channel->inserts[0]->instantiated);
            REQUIRE (ins_track->channel->inserts[0]->activated);
          }
      }
      break;
    case EditTrackActionType::EDIT_TRACK_ACTION_TYPE_DIRECT_OUT:
      {
        if (!is_instrument)
          break;

        /* create a MIDI track */
        track_create_empty_at_idx_with_action (Track::Type::MIDI, 2, nullptr);
        Track * midi_track = TRACKLIST->tracks[2];
        midi_track->select (true, true, false);

        REQUIRE (!midi_track->channel->has_output);

        /* change the direct out to the
         * instrument */
        tracklist_selections_action_perform_set_direct_out (
          TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, ins_track, nullptr);

        /* verify direct out established */
        REQUIRE (midi_track->channel->has_output);
        g_assert_cmpuint (
          midi_track->channel->output_name_hash, ==,
          ins_track->get_name_hash ());

        /* undo and re-verify */
        UNDO_MANAGER->undo ();

        REQUIRE (!midi_track->channel->has_output);

        /* redo and test moving track afterwards */
        UNDO_MANAGER->redo ();
        ins_track = TRACKLIST->tracks[4];
        REQUIRE (ins_track->type == Track::Type::INSTRUMENT);
        ins_track->select (true, true, false);
        tracklist_selections_action_perform_move (
          TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, 1, nullptr);
        UNDO_MANAGER->undo ();

        /* create an audio group track and test
         * routing instrument track to audio
         * group */
        track_create_empty_with_action (Track::Type::AUDIO_GROUP, nullptr);
        UNDO_MANAGER->undo ();
        UNDO_MANAGER->redo ();
        Track * audio_group = TRACKLIST->tracks[TRACKLIST->tracks.size () - 1];
        ins_track->select (true, true, false);
        tracklist_selections_action_perform_set_direct_out (
          TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, audio_group, nullptr);
        UNDO_MANAGER->undo ();
        UNDO_MANAGER->redo ();
        UNDO_MANAGER->undo ();
        UNDO_MANAGER->undo ();
      }
      break;
    case EditTrackActionType::EDIT_TRACK_ACTION_TYPE_SOLO:
      {
        if (!is_instrument)
          break;

        /* create an audio group track */
        track_create_empty_at_idx_with_action (
          Track::Type::AUDIO_GROUP, 2, nullptr);
        Track * group_track = TRACKLIST->tracks[2];

        g_assert_cmpuint (
          ins_track->get_name_hash (), !=, ins_track->channel->output_name_hash);

        /* route the instrument to the group
         * track */
        ins_track->select (true, true, false);
        tracklist_selections_action_perform_set_direct_out (
          TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, group_track, nullptr);

        /* solo the group track */
        group_track->select (true, true, false);
        tracklist_selections_action_perform_edit_solo (
          TRACKLIST_SELECTIONS, true, nullptr);

        /* run the engine for 1 cycle to clear any
         * pending events */
        AUDIO_ENGINE->process (AUDIO_ENGINE->block_length_);

        /* play a note on the instrument track
         * and verify that signal comes out
         * in both tracks */
        midi_events_add_note_on (
          ins_track->processor->midi_in->midi_events_, 1, 62, 74, 2, true);

        bool has_signal = false;

        /* run the engine for 2 cycles (the event
         * does not have enough time to make audible
         * sound in 1 cycle */
        AUDIO_ENGINE->process (AUDIO_ENGINE->block_length_);
        AUDIO_ENGINE->process (AUDIO_ENGINE->block_length_);

        Port &l = ins_track->channel->fader->stereo_out->get_l ();
#  if 0
        Port * ins_out_l =
          plugin_get_port_by_symbol (
          ins_track->channel->instrument,
          /*"lv2_audio_out_1");*/
          "lv2_audio_out_1");
#  endif
        for (nframes_t i = 0; i < AUDIO_ENGINE->block_length_; i++)
          {
#  if 0
            z_info (
              "[{}] %.16f", i, (double) l->buf[i]);
            z_info (
              "[{} i] %.16f", i,
              (double) ins_out_l->buf[i]);
#  endif
            if (l.buf_[i] > 0.0001f)
              {
                has_signal = true;
                break;
              }
          }
        REQUIRE (has_signal);

        /* undo and re-verify */
        UNDO_MANAGER->undo ();
      }
      break;
    case EditTrackActionType::EDIT_TRACK_ACTION_TYPE_RENAME:
      {
        const char * new_name = "new name";
        char *       name_before = g_strdup (ins_track->name);
        track_set_name_with_action (ins_track, new_name);
        REQUIRE (string_is_equal (ins_track->name, new_name));

        /* undo/redo and re-verify */
        UNDO_MANAGER->undo ();
        REQUIRE (string_is_equal (ins_track->name, name_before));
        UNDO_MANAGER->redo ();
        REQUIRE (string_is_equal (ins_track->name, new_name));

        g_free (name_before);

        /* undo to go back to original state */
        UNDO_MANAGER->undo ();
      }
      break;
    case EditTrackActionType::EDIT_TRACK_ACTION_TYPE_RENAME_LANE:
      {
        const char * new_name = "new name";
        TrackLane *  lane = ins_track->lanes[0];
        char *       name_before = g_strdup (lane->name);
        track_lane_rename (lane, new_name, true);
        REQUIRE (string_is_equal (lane->name, new_name));

        /* undo/redo and re-verify */
        UNDO_MANAGER->undo ();
        REQUIRE (string_is_equal (lane->name, name_before));
        UNDO_MANAGER->redo ();
        REQUIRE (string_is_equal (lane->name, new_name));

        g_free (name_before);

        /* undo to go back to original state */
        UNDO_MANAGER->undo ();
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
        REQUIRE_NONNULL (ua);
        undo_manager_perform (UNDO_MANAGER, ua, nullptr);

        /* verify */
        if (type == EditTrackActionType::EDIT_TRACK_ACTION_TYPE_PAN)
          {
            REQUIRE_FLOAT_NEAR (
              new_val, ins_track->channel->get_balance_control (), 0.0001f);
          }
        else if (type == EditTrackActionType::EDIT_TRACK_ACTION_TYPE_VOLUME)
          {
            REQUIRE_FLOAT_NEAR (
              new_val, fader_get_amp (ins_track->channel->fader), 0.0001f);
          }

        /* undo/redo and re-verify */
        UNDO_MANAGER->undo ();
        if (type == EditTrackActionType::EDIT_TRACK_ACTION_TYPE_PAN)
          {
            REQUIRE_FLOAT_NEAR (
              val_before, ins_track->channel->get_balance_control (), 0.0001f);
          }
        else if (type == EditTrackActionType::EDIT_TRACK_ACTION_TYPE_VOLUME)
          {
            REQUIRE_FLOAT_NEAR (
              val_before, fader_get_amp (ins_track->channel->fader), 0.0001f);
          }
        UNDO_MANAGER->redo ();
        if (type == EditTrackActionType::EDIT_TRACK_ACTION_TYPE_PAN)
          {
            REQUIRE_FLOAT_NEAR (
              new_val, ins_track->channel->get_balance_control (), 0.0001f);
          }
        else if (type == EditTrackActionType::EDIT_TRACK_ACTION_TYPE_VOLUME)
          {
            REQUIRE_FLOAT_NEAR (
              new_val, fader_get_amp (ins_track->channel->fader), 0.0001f);
          }

        /* undo to go back to original state */
        UNDO_MANAGER->undo ();
      }
      break;
    case EditTrackActionType::EDIT_TRACK_ACTION_TYPE_COLOR:
      {
        GdkRGBA new_color = { 0.8f, 0.7f, 0.2f, 1.f };
        GdkRGBA color_before = ins_track->color;
        track_set_color (ins_track, &new_color, F_UNDOABLE, false);
        REQUIRE (color_is_same (&ins_track->color, &new_color));

        test_project_save_and_reload ();

        ins_track = get_ins_track ();
        REQUIRE (color_is_same (&ins_track->color, &new_color));

        /* undo/redo and re-verify */
        UNDO_MANAGER->undo ();
        REQUIRE (color_is_same (&ins_track->color, &color_before));
        UNDO_MANAGER->redo ();
        REQUIRE (color_is_same (&ins_track->color, &new_color));

        /* undo to go back to original state */
        UNDO_MANAGER->undo ();
      }
      break;
    case EditTrackActionType::EDIT_TRACK_ACTION_TYPE_ICON:
      {
        const char * new_icon = "icon2";
        char *       icon_before = g_strdup (ins_track->icon_name);
        track_set_icon (ins_track, new_icon, F_UNDOABLE, false);
        REQUIRE (string_is_equal (ins_track->icon_name, new_icon));

        test_project_save_and_reload ();

        ins_track = get_ins_track ();

        /* undo/redo and re-verify */
        UNDO_MANAGER->undo ();
        REQUIRE (string_is_equal (ins_track->icon_name, icon_before));
        UNDO_MANAGER->redo ();
        REQUIRE (string_is_equal (ins_track->icon_name, new_icon));

        /* undo to go back to original state */
        UNDO_MANAGER->undo ();

        g_free (icon_before);
      }
      break;
    case EditTrackActionType::EDIT_TRACK_ACTION_TYPE_COMMENT:
      {
        const char * new_icon = "icon2";
        char *       icon_before = g_strdup (ins_track->comment_);
        track_set_comment (ins_track, new_icon, F_UNDOABLE);
        REQUIRE (string_is_equal (ins_track->comment_, new_icon));

        test_project_save_and_reload ();

        ins_track = get_ins_track ();

        /* undo/redo and re-verify */
        UNDO_MANAGER->undo ();
        REQUIRE (string_is_equal (ins_track->comment_, icon_before));
        UNDO_MANAGER->redo ();
        REQUIRE (string_is_equal (ins_track->comment_, new_icon));

        /* undo to go back to original state */
        UNDO_MANAGER->undo ();

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

      ZrythmFixture fixture;

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
    }
}

TEST_CASE ("Edit tracks")
{
  __test_edit_tracks (false);
#ifdef HAVE_CARLA
  __test_edit_tracks (true);
#endif
}

TEST_CASE_FIXTURE (ZrythmFixture, "Edit MIDI direct out to instrument track")
{
#ifdef HAVE_HELM
  /* create the instrument track */
  test_plugin_manager_create_tracks_from_plugin (
    HELM_BUNDLE, HELM_URI, true, false, 1);
  Track * ins_track = TRACKLIST->tracks[TRACKLIST->tracks.size () - 1];
  ins_track->select (true, true, false);

  char ** midi_files = io_get_files_in_dir_ending_in (
    MIDILIB_TEST_MIDI_FILES_PATH, F_RECURSIVE, ".MID", false);
  REQUIRE_NONNULL (midi_files);

  /* create the MIDI track from a MIDI file */
  FileDescriptor file = FileDescriptor (midi_files[0]);
  track_create_with_action (
    Track::Type::MIDI, nullptr, &file, PLAYHEAD, TRACKLIST->tracks.size (), 1,
    -1, nullptr, nullptr);
  Track * midi_track = TRACKLIST->tracks[TRACKLIST->tracks.size () - 1];
  midi_track->select (true, true, false);
  g_strfreev (midi_files);

  /* route the MIDI track to the instrument track */
  tracklist_selections_action_perform_set_direct_out (
    TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, ins_track, nullptr);

  Channel * ch = midi_track->channel;
  Track *   direct_out = ch->get_output_track ();
  REQUIRE (IS_TRACK (direct_out));
  REQUIRE (direct_out == ins_track);
  REQUIRE_EQ (ins_track->num_children, 1);

  /* delete the instrument, undo and verify that
   * the MIDI track's output is the instrument */
  ins_track->select (true, true, false);

  tracklist_selections_action_perform_delete (
    TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, nullptr);

  direct_out = ch->get_output_track ();
  g_assert_null (direct_out);

  UNDO_MANAGER->undo ();

  ins_track = TRACKLIST->tracks[TRACKLIST->tracks.size () - 2];
  direct_out = ch->get_output_track ();
  REQUIRE (IS_TRACK (direct_out));
  REQUIRE (direct_out == ins_track);
#endif
}

TEST_CASE_FIXTURE (ZrythmFixture, "edit multi track direct out")
{
#ifdef HAVE_HELM
  /* create 2 instrument tracks */
  test_plugin_manager_create_tracks_from_plugin (
    HELM_BUNDLE, HELM_URI, true, false, 2);
  Track * ins_track = TRACKLIST->tracks[TRACKLIST->tracks.size () - 2];
  Track * ins_track2 = TRACKLIST->tracks[TRACKLIST->tracks.size () - 1];

  /* create an audio group */
  Track * audio_group =
    track_create_empty_with_action (Track::Type::AUDIO_GROUP, nullptr);

  /* route the ins tracks to the audio group */
  ins_track->select (true, true, false);
  ins_track2->select (true, false, false);
  tracklist_selections_action_perform_set_direct_out (
    TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, audio_group, nullptr);

  /* change the name of the group track - tests
   * track_update_children() */
  tracklist_selections_action_perform_edit_rename (
    audio_group, PORT_CONNECTIONS_MGR, "new name", nullptr);

  Channel * ch = ins_track->channel;
  Channel * ch2 = ins_track2->channel;
  Track *   direct_out = ch->get_output_track ();
  Track *   direct_out2 = ch2->get_output_track ();
  REQUIRE (IS_TRACK (direct_out));
  REQUIRE (IS_TRACK (direct_out2));
  REQUIRE (direct_out == audio_group);
  REQUIRE (direct_out2 == audio_group);
  REQUIRE_EQ (audio_group->num_children, 2);

  /* delete the audio group, undo and verify that
   * the ins track output is the audio group */
  audio_group->select (true, true, false);
  tracklist_selections_action_perform_delete (
    TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, nullptr);

  direct_out = ch->get_output_track ();
  direct_out2 = ch2->get_output_track ();
  g_assert_null (direct_out);
  g_assert_null (direct_out2);

  /* as a second action, route the tracks to
   * master */
  ins_track->select (true, true, false);
  ins_track2->select (true, false, false);
  tracklist_selections_action_perform_set_direct_out (
    TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, P_MASTER_TRACK, nullptr);
  UndoableAction * ua = undo_manager_get_last_action (UNDO_MANAGER);
  undoable_action_set_num_actions (ua, 2);

  direct_out = ch->get_output_track ();
  direct_out2 = ch2->get_output_track ();
  REQUIRE (direct_out == P_MASTER_TRACK);
  REQUIRE (direct_out2 == P_MASTER_TRACK);

  UNDO_MANAGER->undo ();

  audio_group = TRACKLIST->tracks[TRACKLIST->tracks.size () - 1];
  direct_out = ch->get_output_track ();
  direct_out2 = ch2->get_output_track ();
  REQUIRE (IS_TRACK (direct_out));
  REQUIRE (IS_TRACK (direct_out2));
  REQUIRE (direct_out == audio_group);
  REQUIRE (direct_out2 == audio_group);
#endif
}

TEST_CASE_FIXTURE (ZrythmFixture, "rename MIDI track with events")
{
  /* create a MIDI track from a file */
  char ** midi_files = io_get_files_in_dir_ending_in (
    MIDILIB_TEST_MIDI_FILES_PATH, F_RECURSIVE, ".MID", false);
  REQUIRE_NONNULL (midi_files);
  FileDescriptor file = FileDescriptor (midi_files[0]);
  track_create_with_action (
    Track::Type::MIDI, nullptr, &file, PLAYHEAD, TRACKLIST->tracks.size (), 1,
    -1, nullptr, nullptr);
  Track * midi_track = tracklist_get_last_track (
    TRACKLIST, TracklistPinOption::TRACKLIST_PIN_OPTION_BOTH, false);
  midi_track->select (true, true, false);
  g_strfreev (midi_files);

  /* change the name of the track */
  tracklist_selections_action_perform_edit_rename (
    midi_track, PORT_CONNECTIONS_MGR, "new name", nullptr);

  TRACKLIST->validate ();

  /* play and let engine run */
  TRANSPORT->request_roll (true);
  AUDIO_ENGINE->wait_n_cycles (3);

  UNDO_MANAGER->undo ();
  UNDO_MANAGER->redo ();

  /* duplicate and let engine run */
  tracklist_selections_action_perform_copy (
    TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, TRACKLIST->tracks.size (),
    nullptr);

  /* play and let engine run */
  TRANSPORT->request_roll (true);
  AUDIO_ENGINE->wait_n_cycles (3);
}

TEST_CASE_FIXTURE (ZrythmFixture, "rename track with send")
{
  /* create an audio group */
  Track * audio_group =
    track_create_empty_with_action (Track::Type::AUDIO_GROUP, nullptr);

  /* create an audio fx */
  Track * audio_fx =
    track_create_empty_with_action (Track::Type::AUDIO_BUS, nullptr);

  /* send from group to fx */
  channel_send_action_perform_connect_audio (
    audio_group->channel->sends[0], audio_fx->processor->stereo_in, nullptr);

  /* change the name of the fx track */
  tracklist_selections_action_perform_edit_rename (
    audio_fx, PORT_CONNECTIONS_MGR, "new name", nullptr);

  TRACKLIST->validate ();

  /* play and let engine run */
  TRANSPORT->request_roll (true);
  AUDIO_ENGINE->wait_n_cycles (3);

  /* change the name of the group track */
  tracklist_selections_action_perform_edit_rename (
    audio_group, PORT_CONNECTIONS_MGR, "new name2", nullptr);

  TRACKLIST->validate ();

  /* play and let engine run */
  TRANSPORT->request_roll (true);
  AUDIO_ENGINE->wait_n_cycles (3);

  UNDO_MANAGER->undo ();
  UNDO_MANAGER->undo ();
  UNDO_MANAGER->redo ();
  UNDO_MANAGER->redo ();
}

TEST_SUITE_END;