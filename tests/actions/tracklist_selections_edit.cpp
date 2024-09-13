// SPDX-FileCopyrightText: © 2020-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "actions/channel_send_action.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "actions/tracklist_selections.h"
#include "actions/undoable_action.h"
#include "dsp/audio_region.h"
#include "dsp/master_track.h"
#include "dsp/midi_event.h"
#include "dsp/region.h"
#include "dsp/router.h"
#include "project.h"
#include "utils/color.h"
#include "utils/flags.h"
#include "zrythm.h"

#include "tests/helpers/plugin_manager.h"
#include "tests/helpers/project_helper.h"

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
      ASSERT_EQ (ins_track->type_, Track::Type::Instrument);

      ASSERT_TRUE (ins_track->channel_->instrument_->instantiated_);
      ASSERT_TRUE (ins_track->channel_->instrument_->activated_);
    }
  else
    {
      ASSERT_TRUE (ins_track->channel_->inserts_[0]->instantiated_);
      ASSERT_TRUE (ins_track->channel_->inserts_[0]->activated_);
    }

  ins_track->select (true, true, false);

  switch (type)
    {
    case TracklistSelectionsAction::EditType::Mute:
      {
        UNDO_MANAGER->perform (std::make_unique<MuteTracksAction> (
          *TRACKLIST_SELECTIONS->gen_tracklist_selections (), true));
        if (is_instrument)
          {
            ASSERT_TRUE (ins_track->channel_->instrument_->instantiated_);
            ASSERT_TRUE (ins_track->channel_->instrument_->activated_);
          }
        else
          {
            ASSERT_TRUE (ins_track->channel_->inserts_[0]->instantiated_);
            ASSERT_TRUE (ins_track->channel_->inserts_[0]->activated_);
          }
      }
      break;
    case TracklistSelectionsAction::EditType::DirectOut:
      {
        if (!is_instrument)
          break;

        /* create a MIDI track */
        Track::create_empty_at_idx_with_action (Track::Type::Midi, 2);
        auto midi_track = TRACKLIST->get_track<MidiTrack> (2);
        midi_track->select (true, true, false);

        ASSERT_TRUE (!midi_track->channel_->has_output_);

        /* change the direct out to the instrument */
        UNDO_MANAGER->perform (std::make_unique<ChangeTracksDirectOutAction> (
          *TRACKLIST_SELECTIONS->gen_tracklist_selections (),
          *PORT_CONNECTIONS_MGR, *ins_track));

        /* verify direct out established */
        ASSERT_TRUE (midi_track->channel_->has_output_);
        ASSERT_EQ (
          midi_track->channel_->output_name_hash_, ins_track->get_name_hash ());

        /* undo and re-verify */
        UNDO_MANAGER->undo ();

        ASSERT_FALSE (midi_track->channel_->has_output_);

        /* redo and test moving track afterwards */
        UNDO_MANAGER->redo ();
        ins_track = TRACKLIST->get_track<InstrumentTrack> (4);
        ASSERT_EQ (ins_track->type_, Track::Type::Instrument);
        ins_track->select (true, true, false);
        UNDO_MANAGER->perform (std::make_unique<MoveTracksAction> (
          *TRACKLIST_SELECTIONS->gen_tracklist_selections (), 1));
        UNDO_MANAGER->undo ();

        /* create an audio group track and test routing instrument track to
         * audio group */
        Track::create_empty_with_action<AudioGroupTrack> ();
        UNDO_MANAGER->undo ();
        UNDO_MANAGER->redo ();
        auto audio_group = TRACKLIST->get_last_track<AudioGroupTrack> ();
        ins_track->select (true, true, false);
        UNDO_MANAGER->perform (std::make_unique<ChangeTracksDirectOutAction> (
          *TRACKLIST_SELECTIONS->gen_tracklist_selections (),
          *PORT_CONNECTIONS_MGR, *audio_group));
        UNDO_MANAGER->undo ();
        UNDO_MANAGER->redo ();
        UNDO_MANAGER->undo ();
        UNDO_MANAGER->undo ();
      }
      break;
    case TracklistSelectionsAction::EditType::Solo:
      {
        if (!is_instrument)
          break;

        /* create an audio group track */
        Track::create_empty_at_idx_with_action (Track::Type::AudioGroup, 2);
        auto group_track = TRACKLIST->get_track<AudioGroupTrack> (2);

        ASSERT_NE (
          ins_track->get_name_hash (), ins_track->channel_->output_name_hash_);

        /* route the instrument to the group track */
        ins_track->select (true, true, false);
        UNDO_MANAGER->perform (std::make_unique<ChangeTracksDirectOutAction> (
          *TRACKLIST_SELECTIONS->gen_tracklist_selections (),
          *PORT_CONNECTIONS_MGR, *group_track));

        /* solo the group track */
        group_track->select (true, true, false);
        UNDO_MANAGER->perform (std::make_unique<SoloTracksAction> (
          *TRACKLIST_SELECTIONS->gen_tracklist_selections (), true));

        /* run the engine for 1 cycle to clear any pending events */
        AUDIO_ENGINE->process (AUDIO_ENGINE->block_length_);

        /* play a note on the instrument track and verify that signal comes out
         * in both tracks */
        ins_track->processor_->midi_in_->midi_events_.queued_events_
          .add_note_on (1, 62, 74, 2);

        bool has_signal = false;

        /* run the engine for 2 cycles (the event
         * does not have enough time to make audible
         * sound in 1 cycle */
        AUDIO_ENGINE->process (AUDIO_ENGINE->block_length_);
        AUDIO_ENGINE->process (AUDIO_ENGINE->block_length_);

        const auto &l = ins_track->channel_->fader_->stereo_out_->get_l ();
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
        ASSERT_TRUE (has_signal);

        /* undo and re-verify */
        UNDO_MANAGER->undo ();
      }
      break;
    case TracklistSelectionsAction::EditType::Rename:
      {
        constexpr auto new_name = "new name";
        auto           name_before = ins_track->name_;
        ins_track->set_name_with_action (new_name);
        ASSERT_EQ (ins_track->name_, new_name);

        /* undo/redo and re-verify */
        UNDO_MANAGER->undo ();
        ASSERT_EQ (ins_track->name_, name_before);
        UNDO_MANAGER->redo ();
        ASSERT_EQ (ins_track->name_, new_name);

        /* undo to go back to original state */
        UNDO_MANAGER->undo ();
      }
      break;
    case TracklistSelectionsAction::EditType::RenameLane:
      {
        constexpr auto new_name = "new name";
        auto          &lane = ins_track->lanes_[0];
        auto           name_before = lane->name_;
        lane->rename (new_name, true);
        ASSERT_EQ (lane->name_, new_name);

        /* undo/redo and re-verify */
        UNDO_MANAGER->undo ();
        ASSERT_EQ (lane->name_, name_before);
        UNDO_MANAGER->redo ();
        ASSERT_EQ (lane->name_, new_name);

        /* undo to go back to original state */
        UNDO_MANAGER->undo ();
      }
      break;
    case TracklistSelectionsAction::EditType::Volume:
    case TracklistSelectionsAction::EditType::Pan:
      {
        const float new_val = 0.23f;
        float       val_before = ins_track->channel_->fader_->get_amp ();
        if (type == TracklistSelectionsAction::EditType::Pan)
          {
            val_before = ins_track->channel_->get_balance_control ();
          }
        UNDO_MANAGER->perform (std::make_unique<SingleTrackFloatAction> (
          type, ins_track, val_before, new_val, false));

        /* verify */
        if (type == TracklistSelectionsAction::EditType::Pan)
          {
            ASSERT_NEAR (
              new_val, ins_track->channel_->get_balance_control (), 0.0001f);
          }
        else if (type == TracklistSelectionsAction::EditType::Volume)
          {
            ASSERT_NEAR (
              new_val, ins_track->channel_->fader_->get_amp (), 0.0001f);
          }

        /* undo/redo and re-verify */
        UNDO_MANAGER->undo ();
        if (type == TracklistSelectionsAction::EditType::Pan)
          {
            ASSERT_NEAR (
              val_before, ins_track->channel_->get_balance_control (), 0.0001f);
          }
        else if (type == TracklistSelectionsAction::EditType::Volume)
          {
            ASSERT_NEAR (
              val_before, ins_track->channel_->fader_->get_amp (), 0.0001f);
          }
        UNDO_MANAGER->redo ();
        if (type == TracklistSelectionsAction::EditType::Pan)
          {
            ASSERT_NEAR (
              new_val, ins_track->channel_->get_balance_control (), 0.0001f);
          }
        else if (type == TracklistSelectionsAction::EditType::Volume)
          {
            ASSERT_NEAR (
              new_val, ins_track->channel_->fader_->get_amp (), 0.0001f);
          }

        /* undo to go back to original state */
        UNDO_MANAGER->undo ();
      }
      break;
    case TracklistSelectionsAction::EditType::Color:
      {
        const Color new_color{ 0.8f, 0.7f, 0.2f, 1.f };
        const auto  color_before = ins_track->color_;
        ins_track->set_color (new_color, F_UNDOABLE, false);
        ASSERT_TRUE (ins_track->color_.is_same (new_color));

        test_project_save_and_reload ();

        ins_track = get_ins_track ();
        ASSERT_TRUE (ins_track->color_.is_same (new_color));

        /* undo/redo and re-verify */
        UNDO_MANAGER->undo ();
        ASSERT_TRUE (ins_track->color_.is_same (color_before));
        UNDO_MANAGER->redo ();
        ASSERT_TRUE (ins_track->color_.is_same (new_color));

        /* undo to go back to original state */
        UNDO_MANAGER->undo ();
      }
      break;
    case TracklistSelectionsAction::EditType::Icon:
      {
        constexpr auto new_icon = "icon2";
        const auto     icon_before = ins_track->icon_name_;
        ins_track->set_icon (new_icon, F_UNDOABLE, false);
        ASSERT_EQ (ins_track->icon_name_, new_icon);

        test_project_save_and_reload ();

        ins_track = get_ins_track ();

        /* undo/redo and re-verify */
        UNDO_MANAGER->undo ();
        ASSERT_EQ (ins_track->icon_name_, icon_before);
        UNDO_MANAGER->redo ();
        ASSERT_EQ (ins_track->icon_name_, new_icon);

        /* undo to go back to original state */
        UNDO_MANAGER->undo ();
      }
      break;
    case TracklistSelectionsAction::EditType::Comment:
      {
        constexpr auto new_icon = "icon2";
        const auto     icon_before = ins_track->comment_;
        ins_track->set_comment (new_icon, F_UNDOABLE);
        ASSERT_EQ (ins_track->comment_, new_icon);

        test_project_save_and_reload ();

        ins_track = get_ins_track ();

        /* undo/redo and re-verify */
        UNDO_MANAGER->undo ();
        ASSERT_EQ (ins_track->comment_, icon_before);
        UNDO_MANAGER->redo ();
        ASSERT_EQ (ins_track->comment_, new_icon);

        /* undo to go back to original state */
        UNDO_MANAGER->undo ();
      }
      break;
    default:
      break;
    }
}
#endif

class EditTracksTestFixture
    : public ::testing::TestWithParam<::testing::tuple<bool, int>>
{
protected:
  ZrythmFixture zrythm_fixture_;

  void SetUp () override { zrythm_fixture_.SetUp (); }
  void TearDown () override { zrythm_fixture_.TearDown (); }
};

TEST_P (EditTracksTestFixture, EditTracks)
{
  [[maybe_unused]] const auto &[with_carla, edit_type_int] = GetParam ();
  [[maybe_unused]] const auto edit_type =
    ENUM_INT_TO_VALUE (TracklistSelectionsAction::EditType, edit_type_int);

  /* stop dummy audio engine processing so we can process manually */
  test_project_stop_dummy_engine ();

#ifdef HAVE_CHIPWAVE
  _test_edit_tracks (edit_type, CHIPWAVE_BUNDLE, CHIPWAVE_URI, true, with_carla);
#endif
#ifdef HAVE_HELM
  _test_edit_tracks (edit_type, HELM_BUNDLE, HELM_URI, true, with_carla);
#endif
#ifdef HAVE_LSP_COMPRESSOR
  _test_edit_tracks (
    edit_type, LSP_COMPRESSOR_BUNDLE, LSP_COMPRESSOR_URI, false, with_carla);
#endif
}

INSTANTIATE_TEST_CASE_P (
  EditTracksSuite,
  EditTracksTestFixture,
  ::testing::Combine (
#ifdef HAVE_CARLA
    ::testing::Values (false, true),
#else
    ::testing::Values (false),
#endif
    ::testing::Range (
      ENUM_VALUE_TO_INT (TracklistSelectionsAction::EditType::Solo),
      ENUM_VALUE_TO_INT (TracklistSelectionsAction::EditType::Icon) + 1)));

TEST_F (ZrythmFixture, EditMidiDirectOutToInstrumentTrack)
{
#ifdef HAVE_HELM
  /* create the instrument track */
  test_plugin_manager_create_tracks_from_plugin (
    HELM_BUNDLE, HELM_URI, true, false, 1);
  auto ins_track = TRACKLIST->get_last_track<InstrumentTrack> ();
  ins_track->select (true, true, false);

  auto midi_files = io_get_files_in_dir_ending_in (
    MIDILIB_TEST_MIDI_FILES_PATH, F_RECURSIVE, ".MID");
  ASSERT_FALSE (midi_files.isEmpty ());

  /* create the MIDI track from a MIDI file */
  FileDescriptor file = FileDescriptor (midi_files[0]);
  Track::create_with_action (
    Track::Type::Midi, nullptr, &file, &PLAYHEAD, TRACKLIST->get_num_tracks (),
    1, -1, nullptr);
  auto midi_track = TRACKLIST->get_last_track<MidiTrack> ();
  midi_track->select (true, true, false);

  /* route the MIDI track to the instrument track */
  UNDO_MANAGER->perform (std::make_unique<ChangeTracksDirectOutAction> (
    *TRACKLIST_SELECTIONS->gen_tracklist_selections (), *PORT_CONNECTIONS_MGR,
    *ins_track));

  const auto &ch = midi_track->channel_;
  auto        direct_out = ch->get_output_track ();
  ASSERT_TRUE (IS_TRACK (direct_out));
  ASSERT_TRUE (direct_out == ins_track);
  ASSERT_SIZE_EQ (ins_track->children_, 1);

  /* delete the instrument, undo and verify that
   * the MIDI track's output is the instrument */
  ins_track->select (true, true, false);

  UNDO_MANAGER->perform (std::make_unique<DeleteTracksAction> (
    *TRACKLIST_SELECTIONS->gen_tracklist_selections (), *PORT_CONNECTIONS_MGR));

  direct_out = ch->get_output_track ();
  g_assert_null (direct_out);

  UNDO_MANAGER->undo ();

  ins_track =
    TRACKLIST->get_track<InstrumentTrack> (TRACKLIST->get_num_tracks () - 2);
  direct_out = ch->get_output_track ();
  ASSERT_TRUE (IS_TRACK (direct_out));
  ASSERT_TRUE (direct_out == ins_track);
#endif
}

TEST_F (ZrythmFixture, EditMultiTTrackDirectOut)
{
#ifdef HAVE_HELM
  /* create 2 instrument tracks */
  test_plugin_manager_create_tracks_from_plugin (
    HELM_BUNDLE, HELM_URI, true, false, 2);
  auto ins_track =
    TRACKLIST->get_track<InstrumentTrack> (TRACKLIST->get_num_tracks () - 2);
  auto ins_track2 =
    TRACKLIST->get_track<InstrumentTrack> (TRACKLIST->get_num_tracks () - 1);

  /* create an audio group */
  auto audio_group = Track::create_empty_with_action<AudioGroupTrack> ();

  /* route the ins tracks to the audio group */
  ins_track->select (true, true, false);
  ins_track2->select (true, false, false);
  UNDO_MANAGER->perform (std::make_unique<ChangeTracksDirectOutAction> (
    *TRACKLIST_SELECTIONS->gen_tracklist_selections (), *PORT_CONNECTIONS_MGR,
    *audio_group));

  /* change the name of the group track - tests track_update_children() */
  UNDO_MANAGER->perform (std::make_unique<RenameTrackAction> (
    *audio_group, *PORT_CONNECTIONS_MGR, "new name"));

  const auto &ch = ins_track->channel_;
  const auto &ch2 = ins_track2->channel_;
  auto        direct_out = ch->get_output_track ();
  auto        direct_out2 = ch2->get_output_track ();
  ASSERT_NONNULL (direct_out);
  ASSERT_NONNULL (direct_out2);
  ASSERT_EQ (direct_out, audio_group);
  ASSERT_EQ (direct_out2, audio_group);
  ASSERT_SIZE_EQ (audio_group->children_, 2);

  /* delete the audio group, undo and verify that
   * the ins track output is the audio group */
  audio_group->select (true, true, false);
  UNDO_MANAGER->perform (std::make_unique<DeleteTracksAction> (
    *TRACKLIST_SELECTIONS->gen_tracklist_selections (), *PORT_CONNECTIONS_MGR));

  direct_out = ch->get_output_track ();
  direct_out2 = ch2->get_output_track ();
  ASSERT_NULL (direct_out);
  ASSERT_NULL (direct_out2);

  /* as a second action, route the tracks to
   * master */
  ins_track->select (true, true, false);
  ins_track2->select (true, false, false);
  UNDO_MANAGER->perform (std::make_unique<ChangeTracksDirectOutAction> (
    *TRACKLIST_SELECTIONS->gen_tracklist_selections (), *PORT_CONNECTIONS_MGR,
    *P_MASTER_TRACK));
  auto ua = UNDO_MANAGER->get_last_action ();
  ua->set_num_actions (2);

  direct_out = ch->get_output_track ();
  direct_out2 = ch2->get_output_track ();
  ASSERT_TRUE (direct_out == P_MASTER_TRACK);
  ASSERT_TRUE (direct_out2 == P_MASTER_TRACK);

  UNDO_MANAGER->undo ();

  audio_group = TRACKLIST->get_last_track<AudioGroupTrack> ();
  direct_out = ch->get_output_track ();
  direct_out2 = ch2->get_output_track ();
  ASSERT_TRUE (IS_TRACK (direct_out));
  ASSERT_TRUE (IS_TRACK (direct_out2));
  ASSERT_TRUE (direct_out == audio_group);
  ASSERT_TRUE (direct_out2 == audio_group);
#endif
}

TEST_F (ZrythmFixture, RenameMidiTrackWithEvents)
{
  /* create a MIDI track from a file */
  auto midi_files = io_get_files_in_dir_ending_in (
    MIDILIB_TEST_MIDI_FILES_PATH, F_RECURSIVE, ".MID");
  ASSERT_NONNULL (midi_files);
  FileDescriptor file (midi_files[0]);
  Track::create_with_action (
    Track::Type::Midi, nullptr, &file, &PLAYHEAD, TRACKLIST->get_num_tracks (),
    1, -1, nullptr);
  auto midi_track = TRACKLIST->get_last_track<MidiTrack> ();
  midi_track->select (true, true, false);

  /* change the name of the track */
  UNDO_MANAGER->perform (std::make_unique<RenameTrackAction> (
    *midi_track, *PORT_CONNECTIONS_MGR, "new name"));

  TRACKLIST->validate ();

  /* play and let engine run */
  TRANSPORT->request_roll (true);
  AUDIO_ENGINE->wait_n_cycles (3);

  UNDO_MANAGER->undo ();
  UNDO_MANAGER->redo ();

  /* duplicate and let engine run */
  UNDO_MANAGER->perform (std::make_unique<CopyTracksAction> (
    *TRACKLIST_SELECTIONS->gen_tracklist_selections (), *PORT_CONNECTIONS_MGR,
    TRACKLIST->get_num_tracks ()));

  /* play and let engine run */
  TRANSPORT->request_roll (true);
  AUDIO_ENGINE->wait_n_cycles (3);
}

TEST_F (ZrythmFixture, RenameTrackWithSend)
{
  /* create an audio group */
  auto audio_group = Track::create_empty_with_action<AudioGroupTrack> ();

  /* create an audio fx */
  auto audio_fx = Track::create_empty_with_action<AudioBusTrack> ();

  /* send from group to fx */
  UNDO_MANAGER->perform (std::make_unique<ChannelSendConnectStereoAction> (
    *audio_group->channel_->sends_[0], *audio_fx->processor_->stereo_in_,
    *PORT_CONNECTIONS_MGR));

  /* change the name of the fx track */
  UNDO_MANAGER->perform (std::make_unique<RenameTrackAction> (
    *audio_fx, *PORT_CONNECTIONS_MGR, "new name"));

  TRACKLIST->validate ();

  /* play and let engine run */
  TRANSPORT->request_roll (true);
  AUDIO_ENGINE->wait_n_cycles (3);

  /* change the name of the group track */
  UNDO_MANAGER->perform (std::make_unique<RenameTrackAction> (
    *audio_group, *PORT_CONNECTIONS_MGR, "new name2"));

  TRACKLIST->validate ();

  /* play and let engine run */
  TRANSPORT->request_roll (true);
  AUDIO_ENGINE->wait_n_cycles (3);

  UNDO_MANAGER->undo ();
  UNDO_MANAGER->undo ();
  UNDO_MANAGER->redo ();
  UNDO_MANAGER->redo ();
}