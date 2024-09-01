// SPDX-FileCopyrightText: © 2020-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "actions/channel_send_action.h"
#include "actions/mixer_selections_action.h"
#include "actions/port_connection_action.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "actions/arranger_selections.h"
#include "actions/tracklist_selections.h"
#include "actions/undoable_action.h"
#include "dsp/audio_region.h"
#include "dsp/automation_region.h"
#include "dsp/chord_region.h"
#include "dsp/control_port.h"
#include "dsp/foldable_track.h"
#include "dsp/master_track.h"
#include "dsp/midi_event.h"
#include "dsp/midi_note.h"
#include "dsp/region.h"
#include "dsp/router.h"
#include "project.h"
#include "utils/color.h"
#include "utils/flags.h"
#include "utils/math.h"
#include "zrythm.h"

#include <glib.h>

#include "tests/helpers/plugin_manager.h"
#include "tests/helpers/project_helper.h"

TEST_SUITE_BEGIN ("actions/tracklist_selections");

static auto perform_create_arranger_sel = [] (const auto &selections) {
  UNDO_MANAGER->perform (
    std::make_unique<CreateArrangerSelectionsAction> (*selections));
};

#if 0
static auto perform_delete_arranger_sel = [] (const auto &selections) {
  UNDO_MANAGER->perform (
    std::make_unique<DeleteArrangerSelectionsAction> (*selections));
};
#endif

static auto perform_delete = [] () {
  UNDO_MANAGER->perform (std::make_unique<DeleteTracksAction> (
    *TRACKLIST_SELECTIONS->gen_tracklist_selections (), *PORT_CONNECTIONS_MGR));
};

static auto perform_copy_to_end = [] () {
  UNDO_MANAGER->perform (std::make_unique<CopyTracksAction> (
    *TRACKLIST_SELECTIONS->gen_tracklist_selections (), *PORT_CONNECTIONS_MGR,
    TRACKLIST->get_num_tracks ()));
};

static auto perform_move_to_end = [] () {
  UNDO_MANAGER->perform (std::make_unique<MoveTracksAction> (
    *TRACKLIST_SELECTIONS->gen_tracklist_selections (),
    TRACKLIST->get_num_tracks ()));
};

static auto perform_delete_track = [] (const auto &track) {
  track->select (true, true, false);
  z_return_if_fail (PORT_CONNECTIONS_MGR);
  UNDO_MANAGER->perform (std::make_unique<DeleteTracksAction> (
    *TRACKLIST_SELECTIONS->gen_tracklist_selections (), *PORT_CONNECTIONS_MGR));
};

static auto perform_set_direct_out = [] (const auto &from, const auto &to) {
  from->select (true, true, false);
  UNDO_MANAGER->perform (std::make_unique<ChangeTracksDirectOutAction> (
    *TRACKLIST_SELECTIONS->gen_tracklist_selections (), *PORT_CONNECTIONS_MGR,
    *to));
};

static void
test_num_tracks_with_file (const fs::path &filepath, const int num_tracks)
{
  FileDescriptor file (filepath);

  int num_tracks_before = TRACKLIST->tracks_.size ();

  REQUIRE_NOTHROW (Track::create_with_action (
    Track::Type::Midi, nullptr, &file, &PLAYHEAD, num_tracks_before, 1, -1,
    nullptr));

  auto &first_track = TRACKLIST->tracks_[num_tracks_before];

  perform_delete_track (first_track);

  UNDO_MANAGER->undo ();

  REQUIRE_SIZE_EQ (TRACKLIST->tracks_, num_tracks_before + num_tracks);
}

TEST_CASE_FIXTURE (ZrythmFixture, "create tracks from MIDI file")
{
  auto midi_file =
    fs::path (TESTS_SRCDIR) / "1_empty_track_1_track_with_data.mid";
  test_num_tracks_with_file (midi_file, 1);

  midi_file = fs::path (TESTS_SRCDIR) / "1_track_with_data.mid";
  test_num_tracks_with_file (midi_file, 1);

  auto midi_track =
    TRACKLIST->get_track<MidiTrack> (TRACKLIST->tracks_.size () - 1);
  REQUIRE_NONNULL (midi_track);
  REQUIRE_SIZE_EQ (midi_track->lanes_[0]->regions_[0]->midi_notes_, 3);

  midi_file = fs::path (TESTS_SRCDIR) / "those_who_remain.mid";
  test_num_tracks_with_file (midi_file, 1);
}

TEST_CASE_FIXTURE (
  ZrythmFixture,
  "create instrument track when redo stack is nonempty")
{
  REQUIRE_NOTHROW (Track::create_empty_with_action (Track::Type::Midi));

  UNDO_MANAGER->undo ();

#ifdef HAVE_HELM
  test_plugin_manager_create_tracks_from_plugin (
    HELM_BUNDLE, HELM_URI, true, false, 1);
  UNDO_MANAGER->undo ();

  test_plugin_manager_create_tracks_from_plugin (
    HELM_BUNDLE, HELM_URI, true, false, 1);
  UNDO_MANAGER->undo ();
#endif

  UNDO_MANAGER->redo ();
}

#if defined(HAVE_HELM) || defined(HAVE_LSP_COMPRESSOR)
static void
_test_port_and_plugin_track_pos_after_duplication (
  const char * pl_bundle,
  const char * pl_uri,
  bool         is_instrument,
  bool         with_carla)
{
  test_plugin_manager_create_tracks_from_plugin (
    pl_bundle, pl_uri, is_instrument, with_carla, 1);

  REQUIRE_GT (TRACKLIST->get_num_tracks (), 0);
  int src_track_pos = TRACKLIST->get_num_tracks () - 1;
  int dest_track_pos = TRACKLIST->get_num_tracks ();

  /* select it */
  auto src_track = TRACKLIST->get_track<ChannelTrack> (src_track_pos);
  REQUIRE_NONNULL (src_track);
  src_track->select (true, true, false);

  /* get an automation track */
  auto &at = src_track->automation_tracklist_->ats_.back ();
  at->created_ = true;
  src_track->automation_tracklist_->set_at_visible (*at, true);

  /* create an automation region */
  Position start_pos, end_pos;
  start_pos.set_to_bar (2);
  end_pos.set_to_bar (4);
  const auto track_name_hash = src_track->get_name_hash ();
  auto       region = std::make_shared<AutomationRegion> (
    start_pos, end_pos, track_name_hash, at->index_, at->regions_.size ());
  src_track->add_region (region, at.get (), -1, true, false);
  region->select (true, false, false);
  perform_create_arranger_sel (TL_SELECTIONS);

  /* create some automation points */
  auto port = Port::find_from_identifier<ControlPort> (at->port_id_);
  start_pos.set_to_bar (1);
  const auto normalized_val = port->real_val_to_normalized (port->deff_);
  math_assert_nonnann (port->deff_);
  math_assert_nonnann (normalized_val);
  auto ap =
    std::make_shared<AutomationPoint> (port->deff_, normalized_val, start_pos);
  region->append_object (ap);
  ap->select (true, false, false);
  perform_create_arranger_sel (AUTOMATION_SELECTIONS);
  math_assert_nonnann (ap->fvalue_);
  math_assert_nonnann (ap->normalized_val_);

  REQUIRE (src_track->validate ());

  /* duplicate it */
  perform_copy_to_end ();

  auto dest_track = TRACKLIST->get_track<AutomatableTrack> (dest_track_pos);

  if (is_instrument)
    {
      auto ins_track = dynamic_cast<InstrumentTrack *> (dest_track);
      /* check that track processor is connected to the instrument */
      auto conn = PORT_CONNECTIONS_MGR->get_source_or_dest (
        ins_track->processor_->midi_out_->id_, false);
      REQUIRE_HAS_VALUE (conn);

      /* check that instrument is connected to channel prefader */
      conn = PORT_CONNECTIONS_MGR->get_source_or_dest (
        ins_track->channel_->prefader_->stereo_in_->get_l ().id_, true);
      REQUIRE_HAS_VALUE (conn);
    }

  REQUIRE (src_track->validate ());
  REQUIRE (dest_track->validate ());

  /* move automation in 2nd track and undo/redo */
  const auto &atl = dest_track->get_automation_tracklist ();
  ap = atl.ats_.front ()->regions_.at (0)->aps_.at (0);
  ap->select (true, false, false);
  float prev_norm_val = ap->normalized_val_;
  math_assert_nonnann (ap->normalized_val_);
  math_assert_nonnann (prev_norm_val);
  ap->set_fvalue (prev_norm_val - 0.1f, F_NORMALIZED, false);
  UNDO_MANAGER->perform (
    std::make_unique<ArrangerSelectionsAction::MoveOrDuplicateAutomationAction> (
      *AUTOMATION_SELECTIONS, true, 0, 0.1, true));
  UNDO_MANAGER->undo ();
  UNDO_MANAGER->redo ();

  /* let the engine run */
  std::this_thread::sleep_for (std::chrono::seconds (1));

  /* go back to the start */
  UNDO_MANAGER->undo ();
  UNDO_MANAGER->undo ();
  UNDO_MANAGER->undo ();
  UNDO_MANAGER->undo ();
  UNDO_MANAGER->undo ();
}
#endif

TEST_CASE_FIXTURE (
  ZrythmFixture,
  "test port and plugin track pos after duplication")
{
#ifdef HAVE_HELM
  _test_port_and_plugin_track_pos_after_duplication (
    HELM_BUNDLE, HELM_URI, true, false);
#endif
#ifdef HAVE_LSP_COMPRESSOR
  _test_port_and_plugin_track_pos_after_duplication (
    LSP_COMPRESSOR_BUNDLE, LSP_COMPRESSOR_URI, false, false);
#endif
}

#ifdef HAVE_CARLA
TEST_CASE_FIXTURE (
  ZrythmFixture,
  "test port and plugin track pos after duplication with Carla")
{
#  ifdef HAVE_HELM
  _test_port_and_plugin_track_pos_after_duplication (
    HELM_BUNDLE, HELM_URI, true, true);
#  endif
#  ifdef HAVE_LSP_COMPRESSOR
  _test_port_and_plugin_track_pos_after_duplication (
    LSP_COMPRESSOR_BUNDLE, LSP_COMPRESSOR_URI, false, true);
#  endif
}
#endif

#ifdef HAVE_HELM
static void
_test_undo_track_deletion (
  const char * pl_bundle,
  const char * pl_uri,
  bool         is_instrument,
  bool         with_carla)
{
  test_plugin_manager_create_tracks_from_plugin (
    pl_bundle, pl_uri, is_instrument, with_carla, 1);

  /* save and reload the project */
  test_project_save_and_reload ();

  /* select it */
  auto helm_track =
    TRACKLIST->get_track<InstrumentTrack> (TRACKLIST->get_num_tracks () - 1);
  helm_track->select (true, true, false);

  /* get an automation track */
  const auto &at = helm_track->automation_tracklist_->ats_.at (40);
  at->created_ = true;
  helm_track->automation_tracklist_->set_at_visible (*at, true);

  /* create an automation region */
  Position start_pos, end_pos;
  start_pos.set_to_bar (2);
  end_pos.set_to_bar (4);
  const auto track_name_hash = helm_track->get_name_hash ();
  auto       region = std::make_shared<AutomationRegion> (
    start_pos, end_pos, track_name_hash, at->index_, at->regions_.size ());
  helm_track->add_region (region, at.get (), -1, true, false);
  region->select (true, false, false);
  perform_create_arranger_sel (TL_SELECTIONS);

  /* create some automation points */
  auto port = Port::find_from_identifier<ControlPort> (at->port_id_);
  REQUIRE_NONNULL (port);
  start_pos.set_to_bar (1);
  auto ap = std::make_shared<AutomationPoint> (
    port->deff_, port->real_val_to_normalized (port->deff_), start_pos);
  region->append_object (ap);
  ap->select (true, false, false);
  perform_create_arranger_sel (AUTOMATION_SELECTIONS);

  REQUIRE (helm_track->validate ());

  /* save and reload the project */
  test_project_save_and_reload ();

  /* delete it */
  perform_delete ();

  /* save and reload the project */
  test_project_save_and_reload ();

  /* undo deletion */
  UNDO_MANAGER->undo ();

  /* save and reload the project */
  test_project_save_and_reload ();

  REQUIRE (TRACKLIST->tracks_.back ()->validate ());

  /* let the engine run */
  std::this_thread::sleep_for (std::chrono::seconds (1));
}
#endif

TEST_CASE_FIXTURE (ZrythmFixture, "undo track deletion")
{
#ifdef HAVE_HELM
  _test_undo_track_deletion (HELM_BUNDLE, HELM_URI, true, false);
#endif
}

TEST_CASE_FIXTURE (ZrythmFixture, "group track deletion")
{
  REQUIRE_EMPTY (P_MASTER_TRACK->children_);

  /* create 2 audio fx tracks and route them to a new group track */
  auto audio_fx1 = Track::create_empty_with_action<AudioBusTrack> ();
  auto audio_fx2 = Track::create_empty_with_action<AudioBusTrack> ();
  auto group = Track::create_empty_with_action<AudioGroupTrack> ();

  REQUIRE_SIZE_EQ (P_MASTER_TRACK->children_, 3);

  /* route each fx track to the group */
  perform_set_direct_out (audio_fx1, group);
  REQUIRE_SIZE_EQ (P_MASTER_TRACK->children_, 2);
  UNDO_MANAGER->undo ();
  REQUIRE_SIZE_EQ (P_MASTER_TRACK->children_, 3);
  UNDO_MANAGER->redo ();
  REQUIRE_SIZE_EQ (P_MASTER_TRACK->children_, 2);
  perform_set_direct_out (audio_fx2, group);
  REQUIRE_SIZE_EQ (P_MASTER_TRACK->children_, 1);

  REQUIRE_SIZE_EQ (group->children_, 2);
  REQUIRE (audio_fx1->channel_->has_output_);
  REQUIRE (audio_fx2->channel_->has_output_);
  REQUIRE_EQ (audio_fx1->channel_->output_name_hash_, group->get_name_hash ());
  REQUIRE_EQ (audio_fx2->channel_->output_name_hash_, group->get_name_hash ());

  /* save and reload the project */
  int group_pos = group->pos_;
  int audio_fx1_pos = audio_fx1->pos_;
  int audio_fx2_pos = audio_fx2->pos_;
  test_project_save_and_reload ();
  group = TRACKLIST->get_track<AudioGroupTrack> (group_pos);
  audio_fx1 = TRACKLIST->get_track<AudioBusTrack> (audio_fx1_pos);
  audio_fx2 = TRACKLIST->get_track<AudioBusTrack> (audio_fx2_pos);

  /* delete the group track and check that each fx track has its output set to
   * none */
  group->select (true, true, false);
  perform_delete ();
  REQUIRE_FALSE (audio_fx1->channel_->has_output_);
  REQUIRE_FALSE (audio_fx2->channel_->has_output_);

  /* save and reload the project */
  audio_fx1_pos = audio_fx1->pos_;
  audio_fx2_pos = audio_fx2->pos_;
  test_project_save_and_reload ();
  audio_fx1 = TRACKLIST->get_track<AudioBusTrack> (audio_fx1_pos);
  audio_fx2 = TRACKLIST->get_track<AudioBusTrack> (audio_fx2_pos);

  REQUIRE_FALSE (audio_fx1->channel_->has_output_);
  REQUIRE_FALSE (audio_fx2->channel_->has_output_);

  /* undo and check that the connections are
   * established again */
  UNDO_MANAGER->undo ();
  group = TRACKLIST->get_track<AudioGroupTrack> (group_pos);
  REQUIRE_SIZE_EQ (group->children_, 2);
  REQUIRE_EQ (group->children_[0], audio_fx1->get_name_hash ());
  REQUIRE_EQ (group->children_[1], audio_fx2->get_name_hash ());
  REQUIRE (audio_fx1->channel_->has_output_);
  REQUIRE (audio_fx2->channel_->has_output_);
  REQUIRE_EQ (audio_fx1->channel_->output_name_hash_, group->get_name_hash ());
  REQUIRE_EQ (audio_fx2->channel_->output_name_hash_, group->get_name_hash ());

  /* save and reload the project */
  audio_fx1_pos = audio_fx1->pos_;
  audio_fx2_pos = audio_fx2->pos_;
  test_project_save_and_reload ();
  audio_fx1 = TRACKLIST->get_track<AudioBusTrack> (audio_fx1_pos);
  audio_fx2 = TRACKLIST->get_track<AudioBusTrack> (audio_fx2_pos);

  /* redo and check that the connections are gone again */
  UNDO_MANAGER->redo ();
  REQUIRE_FALSE (audio_fx1->channel_->has_output_);
  REQUIRE_FALSE (audio_fx2->channel_->has_output_);
}

#ifdef HAVE_AMS_LFO
/**
 * Asserts that src is connected/disconnected
 * to/from send.
 *
 * @param pl_out_port_idx Outgoing port index on
 *   src track.
 * @param pl_in_port_idx Incoming port index on
 *   src track.
 */
static void
assert_sends_connected (
  ChannelTrack * src,
  ChannelTrack * dest,
  bool           connected,
  int            pl_out_port_idx,
  int            pl_in_port_idx)
{
  if (src && dest)
    {
      const auto &prefader_send = src->channel_->sends_[0];
      const auto &postfader_send =
        src->channel_->sends_[CHANNEL_SEND_POST_FADER_START_SLOT];

      bool prefader_connected = prefader_send->is_connected_to (
        dest->processor_->stereo_in_.get (), nullptr);
      bool fader_connected = postfader_send->is_connected_to (
        dest->processor_->stereo_in_.get (), nullptr);

      bool pl_ports_connected =
        src->channel_->inserts_[0]->out_ports_[pl_out_port_idx]->is_connected_to (
          *dest->channel_->inserts_[0]->in_ports_[pl_in_port_idx]);

      REQUIRE_EQ (prefader_connected, connected);
      REQUIRE_EQ (fader_connected, connected);
      REQUIRE_EQ (pl_ports_connected, connected);
    }

  if (src)
    {
      bool prefader_connected = src->channel_->sends_[0]->is_enabled ();
      bool fader_connected =
        src->channel_->sends_[CHANNEL_SEND_POST_FADER_START_SLOT]->is_enabled ();
      bool pl_ports_connected =
        !src->channel_->inserts_[0]->out_ports_[pl_out_port_idx]->dests_.empty ();

      REQUIRE_EQ (prefader_connected, connected);
      REQUIRE_EQ (fader_connected, connected);
      REQUIRE_EQ (pl_ports_connected, connected);
    }
}

static void
test_track_deletion_with_sends (
  bool         test_deleting_target,
  const char * pl_bundle,
  const char * pl_uri)
{
  PluginSetting setting =
    test_plugin_manager_get_plugin_setting (pl_bundle, pl_uri, false);

  /* create an audio fx track and send both prefader and postfader to a new
   * audio fx track */
  auto audio_fx_for_sending = Track::create_for_plugin_at_idx_w_action<
    AudioBusTrack> (&setting, TRACKLIST->get_num_tracks ());
  auto audio_fx_for_receiving = Track::create_for_plugin_at_idx_w_action<
    AudioBusTrack> (&setting, TRACKLIST->get_num_tracks ());

  REQUIRE_EQ (
    audio_fx_for_receiving->channel_->sends_[0]->track_name_hash_,
    audio_fx_for_receiving->channel_->sends_[0]->amount_->id_.track_name_hash_);

  const auto &prefader_send = audio_fx_for_sending->channel_->sends_[0];
  UNDO_MANAGER->perform (std::make_unique<ChannelSendConnectStereoAction> (
    *prefader_send, *audio_fx_for_receiving->processor_->stereo_in_,
    *PORT_CONNECTIONS_MGR));
  const auto &postfader_send =
    audio_fx_for_sending->channel_->sends_[CHANNEL_SEND_POST_FADER_START_SLOT];
  UNDO_MANAGER->perform (std::make_unique<ChannelSendConnectStereoAction> (
    *postfader_send, *audio_fx_for_receiving->processor_->stereo_in_,
    *PORT_CONNECTIONS_MGR));

  REQUIRE (TRACKLIST->validate ());

  /* connect plugin from sending track to plugin on receiving track */
  CVPort *      out_port = NULL;
  ControlPort * in_port = NULL;
  int           out_port_idx = -1;
  int           in_port_idx = -1;

  {
    const auto &pl = audio_fx_for_sending->channel_->inserts_[0];
    for (size_t i = 0; i < pl->out_ports_.size (); i++)
      {
        const auto &port = pl->out_ports_[i];
        if (port->id_.type_ == PortType::CV)
          {
            /* connect the first out CV port */
            out_port = dynamic_cast<CVPort *> (port.get ());
            out_port_idx = i;
            break;
          }
      }
  }

  {
    const auto &pl = audio_fx_for_receiving->channel_->inserts_[0];
    for (size_t i = 0; i < pl->in_ports_.size (); i++)
      {
        const auto &port = pl->in_ports_[i];
        if (
          port->id_.type_ == PortType::Control
          && ENUM_BITSET_TEST (
            PortIdentifier::Flags, port->id_.flags_,
            PortIdentifier::Flags::PluginControl))
          {
            /* connect the first in control port */
            in_port = dynamic_cast<ControlPort *> (port.get ());
            in_port_idx = i;
            break;
          }
      }
  }

  UNDO_MANAGER->perform (std::make_unique<PortConnectionConnectAction> (
    out_port->id_, in_port->id_));

  /* assert they are connected */
  assert_sends_connected (
    audio_fx_for_sending, audio_fx_for_receiving, true, out_port_idx,
    in_port_idx);

  REQUIRE_EQ (
    prefader_send->track_name_hash_,
    prefader_send->amount_->id_.track_name_hash_);
  REQUIRE_EQ (
    postfader_send->track_name_hash_,
    postfader_send->amount_->id_.track_name_hash_);

  /* save and reload the project */
  int audio_fx_for_sending_pos = audio_fx_for_sending->pos_;
  int audio_fx_for_receiving_pos = audio_fx_for_receiving->pos_;
  test_project_save_and_reload ();
  audio_fx_for_sending =
    TRACKLIST->get_track<AudioBusTrack> (audio_fx_for_sending_pos);
  audio_fx_for_receiving =
    TRACKLIST->get_track<AudioBusTrack> (audio_fx_for_receiving_pos);

  assert_sends_connected (
    audio_fx_for_sending, audio_fx_for_receiving, true, out_port_idx,
    in_port_idx);

  if (test_deleting_target)
    {
      /* delete the receiving track and check that the sends are gone */
      audio_fx_for_receiving->select (true, true, false);
      perform_delete ();
      assert_sends_connected (
        audio_fx_for_sending, nullptr, false, out_port_idx, in_port_idx);
      audio_fx_for_sending_pos = audio_fx_for_sending->pos_;

      /* save and reload the project */
      test_project_save_and_reload ();
      audio_fx_for_sending =
        TRACKLIST->get_track<AudioBusTrack> (audio_fx_for_sending_pos);
      audio_fx_for_receiving =
        TRACKLIST->get_track<AudioBusTrack> (audio_fx_for_receiving_pos);
      assert_sends_connected (
        audio_fx_for_sending, nullptr, false, out_port_idx, in_port_idx);
    }
  else
    {
      /* wait for a few cycles */
      AUDIO_ENGINE->wait_n_cycles (40);

      /* delete the sending track */
      audio_fx_for_sending->select (true, true, false);
      perform_delete ();
      audio_fx_for_receiving_pos = audio_fx_for_receiving->pos_;

      /* save and reload the project */
      test_project_save_and_reload ();
      audio_fx_for_receiving =
        TRACKLIST->get_track<AudioBusTrack> (audio_fx_for_receiving_pos);
    }

  REQUIRE (PROJECT->validate ());

  /* undo and check that the sends are
   * established again */
  UNDO_MANAGER->undo ();

  audio_fx_for_sending =
    TRACKLIST->get_track<AudioBusTrack> (audio_fx_for_sending_pos);
  assert_sends_connected (
    audio_fx_for_sending, audio_fx_for_receiving, true, out_port_idx,
    in_port_idx);

  /* TODO test MIDI sends */
}
#endif

TEST_CASE_FIXTURE (ZrythmFixture, "delete target track with sends")
{
#ifdef HAVE_AMS_LFO
  test_track_deletion_with_sends (true, AMS_LFO_BUNDLE, AMS_LFO_URI);
#endif
}

TEST_CASE_FIXTURE (ZrythmFixture, "delete source track with sends")
{
#ifdef HAVE_AMS_LFO
  test_track_deletion_with_sends (false, AMS_LFO_BUNDLE, AMS_LFO_URI);
#endif
}

TEST_CASE_FIXTURE (ZrythmFixture, "delete audio track")
{
  auto track = Track::create_empty_with_action<AudioTrack> ();

  /* delete track and undo */
  track->select (true, true, false);

  /* set input gain and mono */
  track->processor_->input_gain_->set_control_value (1.4f, false, false);
  track->processor_->mono_->set_control_value (1.0f, false, false);

  perform_delete ();

  UNDO_MANAGER->undo ();

  track = TRACKLIST->get_track<AudioTrack> (TRACKLIST->get_num_tracks () - 1);
  REQUIRE_FLOAT_NEAR (track->processor_->input_gain_->control_, 1.4f, 0.001f);
  REQUIRE_FLOAT_NEAR (track->processor_->mono_->control_, 1.0f, 0.001f);

  UNDO_MANAGER->redo ();
}

TEST_CASE_FIXTURE (ZrythmFixture, "delete track with LV2 worker")
{
#ifdef HAVE_LSP_MULTISAMPLER_24_DO
  test_plugin_manager_create_tracks_from_plugin (
    LSP_MULTISAMPLER_24_DO_BUNDLE, LSP_MULTISAMPLER_24_DO_URI, true, false, 1);

  /* delete track and undo */
  auto track =
    TRACKLIST->get_track<InstrumentTrack> (TRACKLIST->get_num_tracks () - 1);
  track->select (true, true, false);

  perform_delete ();

  UNDO_MANAGER->undo ();

  UNDO_MANAGER->redo ();
#endif
}

TEST_CASE_FIXTURE (ZrythmFixture, "delete track with plugin")
{
  test_plugin_manager_create_tracks_from_plugin (
    COMPRESSOR_BUNDLE, COMPRESSOR_URI, false, true, 1);

  REQUIRE_NOTHROW (PROJECT->save (PROJECT->dir_, 0, 0, F_NO_ASYNC));

  /* delete track and undo */
  auto track =
    TRACKLIST->get_track<AudioBusTrack> (TRACKLIST->get_num_tracks () - 1);
  track->select (true, true, false);

  perform_delete ();

  REQUIRE_NOTHROW (PROJECT->save (PROJECT->dir_, 0, 0, F_NO_ASYNC));

  UNDO_MANAGER->undo ();

  REQUIRE_NOTHROW (PROJECT->save (PROJECT->dir_, 0, 0, F_NO_ASYNC));

  UNDO_MANAGER->redo ();

  REQUIRE_NOTHROW (PROJECT->save (PROJECT->dir_, 0, 0, F_NO_ASYNC));
}

TEST_CASE_FIXTURE (ZrythmFixture, "delete instrument track with automation")
{
#if defined(HAVE_TAL_FILTER) && defined(HAVE_NOIZE_MAKER)
  AUDIO_ENGINE->activate (false);

  /* create the instrument track */
  test_plugin_manager_create_tracks_from_plugin (
    NOIZE_MAKER_BUNDLE, NOIZE_MAKER_URI, true, false, 1);

  /* select the track */
  auto track =
    TRACKLIST->get_track<InstrumentTrack> (TRACKLIST->get_num_tracks () - 1);
  track->select (true, true, false);

  /* add an effect */
  auto setting = test_plugin_manager_get_plugin_setting (
    TAL_FILTER_BUNDLE, TAL_FILTER_URI, false);
  UNDO_MANAGER->perform (std::make_unique<MixerSelectionsCreateAction> (
    PluginSlotType::Insert, *track, 0, setting));

  AUDIO_ENGINE->activate (true);

  /* get the instrument cutoff automation track */
  auto &atl = track->get_automation_tracklist ();
  auto  at = atl.ats_[41].get ();
  at->created_ = true;
  atl.set_at_visible (*at, true);

  /* create an automation region */
  Position start_pos, end_pos;
  start_pos.set_to_bar (2);
  end_pos.set_to_bar (4);
  auto region = std::make_shared<AutomationRegion> (
    start_pos, end_pos, track->get_name_hash (), at->index_,
    at->regions_.size ());
  track->add_region (region, at, -1, true, false);
  region->select (true, false, false);
  perform_create_arranger_sel (TL_SELECTIONS);

  /* create some automation points */
  auto port = Port::find_from_identifier<ControlPort> (at->port_id_);
  start_pos.set_to_bar (1);
  auto ap = std::make_shared<AutomationPoint> (
    port->deff_, port->real_val_to_normalized (port->deff_), start_pos);
  region->append_object (ap);
  ap->select (true, false, false);
  perform_create_arranger_sel (AUTOMATION_SELECTIONS);

  REQUIRE (track->validate ());

  /* get the filter cutoff automation track */
  at = atl.ats_[141].get ();
  at->created_ = true;
  atl.set_at_visible (*at, true);

  /* create an automation region */
  start_pos.set_to_bar (2);
  end_pos.set_to_bar (4);
  region = std::make_shared<AutomationRegion> (
    start_pos, end_pos, track->get_name_hash (), at->index_,
    at->regions_.size ());
  track->add_region (region, at, -1, true, false);
  region->select (true, false, false);
  perform_create_arranger_sel (TL_SELECTIONS);

  /* create some automation points */
  port = Port::find_from_identifier<ControlPort> (at->port_id_);
  start_pos.set_to_bar (1);
  ap = std::make_shared<AutomationPoint> (
    port->deff_, port->real_val_to_normalized (port->deff_), start_pos);
  region->append_object (ap);
  ap->select (true, false, false);
  perform_create_arranger_sel (AUTOMATION_SELECTIONS);

  REQUIRE (track->validate ());

  /* save and reload the project */
  test_project_save_and_reload ();

  REQUIRE (TRACKLIST->tracks_.back ()->validate ());

  /* delete it */
  perform_delete ();

  /* save and reload the project */
  test_project_save_and_reload ();

  /* undo deletion */
  UNDO_MANAGER->undo ();

  /* save and reload the project */
  test_project_save_and_reload ();

  REQUIRE (TRACKLIST->tracks_.back ()->validate ());

  /* let the engine run */
  std::this_thread::sleep_for (std::chrono::seconds (1));
#endif
}

TEST_CASE_FIXTURE (ZrythmFixture, "check no visible tracks after deleting track")
{
  /* hide all tracks */
  for (auto &track : TRACKLIST->tracks_)
    {
      track->visible_ = false;
    }

  auto track = Track::create_empty_with_action<AudioBusTrack> ();

  /* assert a track is selected */
  REQUIRE_NONEMPTY (TRACKLIST_SELECTIONS->track_names_);

  track->select (true, true, false);

  /* delete the track */
  perform_delete ();

  /* assert a track is selected */
  REQUIRE_NONEMPTY (TRACKLIST_SELECTIONS->track_names_);

  /* assert undo history is not empty */
  REQUIRE_NONEMPTY (*UNDO_MANAGER->undo_stack_);
}

#if defined(HAVE_HELM) || defined(HAVE_LSP_COMPRESSOR)
static void
_test_move_tracks (
  const char * pl_bundle,
  const char * pl_uri,
  bool         is_instrument,
  bool         with_carla)
{
  /* move markers track to the top */
  int prev_pos = P_MARKER_TRACK->pos_;
  P_MARKER_TRACK->select (true, true, false);
  UNDO_MANAGER->perform (std::make_unique<MoveTracksAction> (
    *TRACKLIST_SELECTIONS->gen_tracklist_selections (), 0));

  REQUIRE (TRACKLIST->get_track (prev_pos)->validate ());
  REQUIRE (TRACKLIST->get_track (0)->validate ());

  auto setting =
    test_plugin_manager_get_plugin_setting (pl_bundle, pl_uri, with_carla);

  /* fix the descriptor (for some reason lilv reports it as Plugin instead of
   * Instrument if you don't do lilv_world_load_all) */
  if (is_instrument)
    {
      setting.descr_.category_ = ZPluginCategory::INSTRUMENT;
    }
  setting.descr_.category_str_ =
    PluginDescriptor::category_to_string (setting.descr_.category_);

  /* create a track with an instrument */
  Track::create_for_plugin_at_idx_w_action (
    is_instrument ? Track::Type::Instrument : Track::Type::AudioBus, &setting,
    3);
  auto ins_track = TRACKLIST->get_track<ChannelTrack> (3);
  if (is_instrument)
    {
      REQUIRE (ins_track->is_instrument ());
    }

  /* create an fx track and send to it */
  Track::create_with_action (
    Track::Type::AudioBus, nullptr, nullptr, nullptr, 4, 1, -1, nullptr);
  auto fx_track = TRACKLIST->get_track<AudioBusTrack> (4);
  REQUIRE_NONNULL (fx_track);

  {
    const auto &send = ins_track->channel_->sends_[0];
    const auto &stereo_in = fx_track->processor_->stereo_in_;
    REQUIRE_NOTHROW (send->connect_stereo (
      fx_track->processor_->stereo_in_.get (), nullptr, nullptr, false, false,
      true));

    /* check that the sends are correct */
    REQUIRE (send->is_enabled ());
    REQUIRE (send->is_connected_to (stereo_in.get (), nullptr));
  }

  /* create an automation region on the fx track */
  Position pos, end_pos;
  end_pos.add_ticks (600);
  auto ar = std::make_shared<AutomationRegion> (
    pos, end_pos, fx_track->get_name_hash (), 0, 0);
  fx_track->add_region (
    ar, fx_track->automation_tracklist_->ats_[0].get (), -1, true, false);
  ar->select (true, false, false);
  perform_create_arranger_sel (TL_SELECTIONS);

  /* make the region the clip editor region */
  CLIP_EDITOR->set_region (ar.get (), false);

  /* swap tracks */
  ins_track->select (true, true, false);
  AUDIO_ENGINE->wait_n_cycles (40);
  UNDO_MANAGER->perform (std::make_unique<MoveTracksAction> (
    *TRACKLIST_SELECTIONS->gen_tracklist_selections (), 5));

  /* check that ids are updated */
  ins_track = TRACKLIST->get_track<ChannelTrack> (4);
  fx_track = TRACKLIST->get_track<AudioBusTrack> (3);
  if (is_instrument)
    {
      REQUIRE (ins_track->is_instrument ());
    }
  REQUIRE (fx_track->type_ == Track::Type::AudioBus);

  {
    const auto &send = ins_track->channel_->sends_[0];
    const auto &stereo_in = fx_track->processor_->stereo_in_;
    REQUIRE_EQ (
      stereo_in->get_l ().id_.track_name_hash_, fx_track->get_name_hash ());
    REQUIRE_EQ (
      stereo_in->get_r ().id_.track_name_hash_, fx_track->get_name_hash ());
    REQUIRE_EQ (send->track_, ins_track);
    REQUIRE (send->is_enabled ());
    REQUIRE (send->is_connected_to (stereo_in.get (), nullptr));
    REQUIRE (ins_track->validate ());
    REQUIRE (fx_track->validate ());
  }

  /* check that the clip editor region is updated */
  auto clip_editor_region = CLIP_EDITOR->get_region ();
  REQUIRE_EQ (clip_editor_region, ar.get ());

  /* check that the stereo out of the audio fx track points to the master track */
  REQUIRE_HAS_VALUE (PORT_CONNECTIONS_MGR->find_connection (
    fx_track->channel_->stereo_out_->get_l ().id_,
    P_MASTER_TRACK->processor_->stereo_in_->get_l ().id_));
  REQUIRE_HAS_VALUE (PORT_CONNECTIONS_MGR->find_connection (
    fx_track->channel_->stereo_out_->get_r ().id_,
    P_MASTER_TRACK->processor_->stereo_in_->get_r ().id_));

  /* TODO replace below */
#  if 0
  /* verify fx track out ports */
  port_verify_src_and_dests (
    fx_track->channel->stereo_out->get_l ());
  port_verify_src_and_dests (
    fx_track->channel->stereo_out->get_r ());

  /* verify master track in ports */
  port_verify_src_and_dests (
    P_MASTER_TRACK->processor->stereo_in->get_l ());
  port_verify_src_and_dests (
    P_MASTER_TRACK->processor->stereo_in->get_r ());
#  endif

  /* unswap tracks */
  UNDO_MANAGER->undo ();

  /* check that ids are updated */
  ins_track = TRACKLIST->get_track<ChannelTrack> (3);
  fx_track = TRACKLIST->get_track<AudioBusTrack> (4);
  if (is_instrument)
    {
      REQUIRE (ins_track->is_instrument ());
    }
  REQUIRE (fx_track->type_ == Track::Type::AudioBus);

  {
    const auto &send = ins_track->channel_->sends_[0];
    const auto &stereo_in = fx_track->processor_->stereo_in_;
    REQUIRE_EQ (
      stereo_in->get_l ().id_.track_name_hash_, fx_track->get_name_hash ());
    REQUIRE_EQ (
      stereo_in->get_r ().id_.track_name_hash_, fx_track->get_name_hash ());
    REQUIRE_EQ (send->track_, ins_track);
    REQUIRE (send->is_enabled ());
    REQUIRE (send->is_connected_to (stereo_in.get (), nullptr));
    REQUIRE (ins_track->validate ());
    REQUIRE (fx_track->validate ());
  }

  /* check that the stereo out of the audio fx track points to the master track */
  REQUIRE_HAS_VALUE (PORT_CONNECTIONS_MGR->find_connection (
    fx_track->channel_->stereo_out_->get_l ().id_,
    P_MASTER_TRACK->processor_->stereo_in_->get_l ().id_));
  REQUIRE_HAS_VALUE (PORT_CONNECTIONS_MGR->find_connection (
    fx_track->channel_->stereo_out_->get_r ().id_,
    P_MASTER_TRACK->processor_->stereo_in_->get_r ().id_));

#  if 0
  /* verify fx track out ports */
  port_verify_src_and_dests (
    fx_track->channel->stereo_out->get_l ());
  port_verify_src_and_dests (
    fx_track->channel->stereo_out->get_r ());

  /* verify master track in ports */
  port_verify_src_and_dests (
    P_MASTER_TRACK->processor->stereo_in->get_l ());
  port_verify_src_and_dests (
    P_MASTER_TRACK->processor->stereo_in->get_r ());
#  endif
}
#endif

static void
__test_move_tracks (bool with_carla)
{
  ZrythmFixture fixture;

#ifdef HAVE_HELM
  _test_move_tracks (HELM_BUNDLE, HELM_URI, true, with_carla);
#endif
#ifdef HAVE_LSP_COMPRESSOR
  _test_move_tracks (
    LSP_COMPRESSOR_BUNDLE, LSP_COMPRESSOR_URI, false, with_carla);
#endif
}

TEST_CASE ("move tracks")
{
  __test_move_tracks (false);
#ifdef HAVE_CARLA
  __test_move_tracks (true);
#endif
}

TEST_CASE_FIXTURE (ZrythmFixture, "multi-track duplicate")
{
  int start_pos = TRACKLIST->get_num_tracks ();

  /* create midi track, audio fx track and audio track */
  Track::create_with_action (
    Track::Type::Midi, nullptr, nullptr, nullptr, start_pos, 1, -1, nullptr);
  Track::create_with_action (
    Track::Type::AudioBus, nullptr, nullptr, nullptr, start_pos + 1, 1, -1,
    nullptr);
  Track::create_with_action (
    Track::Type::Audio, nullptr, nullptr, nullptr, start_pos + 2, 1, -1,
    nullptr);

  REQUIRE (TRACKLIST->tracks_[start_pos]->is_midi ());
  REQUIRE (TRACKLIST->tracks_[start_pos + 1]->is_audio_bus ());
  REQUIRE (TRACKLIST->tracks_[start_pos + 2]->is_audio ());

  /* select and duplicate */
  TRACKLIST->tracks_[start_pos]->select (true, true, false);
  TRACKLIST->tracks_[start_pos + 1]->select (true, false, false);
  TRACKLIST->tracks_[start_pos + 2]->select (true, false, false);
  UNDO_MANAGER->perform (std::make_unique<CopyTracksAction> (
    *TRACKLIST_SELECTIONS->gen_tracklist_selections (), *PORT_CONNECTIONS_MGR,
    start_pos + 3));

  /* check order correct */
  REQUIRE (TRACKLIST->tracks_[start_pos]->is_midi ());
  REQUIRE (TRACKLIST->tracks_[start_pos + 1]->is_audio_bus ());
  REQUIRE (TRACKLIST->tracks_[start_pos + 2]->is_audio ());
  REQUIRE (TRACKLIST->tracks_[start_pos + 3]->is_midi ());
  REQUIRE (TRACKLIST->tracks_[start_pos + 4]->is_audio_bus ());
  REQUIRE (TRACKLIST->tracks_[start_pos + 5]->is_audio ());

  /* undo and check */
  UNDO_MANAGER->undo ();
  REQUIRE (TRACKLIST->tracks_[start_pos]->is_midi ());
  REQUIRE (TRACKLIST->tracks_[start_pos + 1]->is_audio_bus ());
  REQUIRE (TRACKLIST->tracks_[start_pos + 2]->is_audio ());

  /* select and duplicate after first */
  TRACKLIST->tracks_[start_pos]->select (true, true, false);
  TRACKLIST->tracks_[start_pos + 1]->select (true, false, false);
  TRACKLIST->tracks_[start_pos + 2]->select (true, false, false);
  UNDO_MANAGER->perform (std::make_unique<CopyTracksAction> (
    *TRACKLIST_SELECTIONS->gen_tracklist_selections (), *PORT_CONNECTIONS_MGR,
    start_pos + 1));

  /* check order correct */
  REQUIRE (TRACKLIST->tracks_[start_pos]->is_midi ());
  REQUIRE (TRACKLIST->tracks_[start_pos + 1]->is_midi ());
  REQUIRE (TRACKLIST->tracks_[start_pos + 2]->is_audio_bus ());
  REQUIRE (TRACKLIST->tracks_[start_pos + 3]->is_audio ());
  REQUIRE (TRACKLIST->tracks_[start_pos + 4]->is_audio_bus ());
  REQUIRE (TRACKLIST->tracks_[start_pos + 5]->is_audio ());
}

TEST_CASE_FIXTURE (ZrythmFixture, "delete track with midi file")
{
  const auto midi_file =
    fs::path (TESTS_SRCDIR) / "format_1_two_tracks_with_data.mid";
  test_num_tracks_with_file (midi_file, 2);

  REQUIRE_SIZE_EQ (
    TRACKLIST->get_track<MidiTrack> (TRACKLIST->tracks_.size () - 2)
      ->lanes_[0]
      ->regions_[0]
      ->midi_notes_,
    7);
  REQUIRE_SIZE_EQ (
    TRACKLIST->get_track<MidiTrack> (TRACKLIST->tracks_.size () - 1)
      ->lanes_[0]
      ->regions_[0]
      ->midi_notes_,
    6);
}

TEST_CASE_FIXTURE (ZrythmFixture, "marker track unpin")
{
  auto require_pin_status = [&] (bool pinned) {
    for (auto &m : P_MARKER_TRACK->markers_)
      {
        auto m_track = m->get_track_as<MarkerTrack> ();
        REQUIRE_EQ (m_track, P_MARKER_TRACK);
        REQUIRE_EQ (m_track->is_pinned (), pinned);
      }
  };

  require_pin_status (true);

  P_MARKER_TRACK->select (true, true, false);

  UNDO_MANAGER->perform (std::make_unique<UnpinTracksAction> (
    *TRACKLIST_SELECTIONS->gen_tracklist_selections (), *PORT_CONNECTIONS_MGR));

  require_pin_status (false);

  UNDO_MANAGER->undo ();

  require_pin_status (true);
}

/**
 * Test duplicate when the last track in the
 * tracklist is the output of the duplicated track.
 */
TEST_CASE_FIXTURE (ZrythmFixture, "duplicate with output and send")
{
  int start_pos = TRACKLIST->get_num_tracks ();

  /* create audio track + audio group track + audio fx track */
  Track::create_with_action (
    Track::Type::Audio, nullptr, nullptr, nullptr, start_pos, 1, -1, nullptr);
  Track::create_with_action (
    Track::Type::AudioGroup, nullptr, nullptr, nullptr, start_pos + 1, 1, -1,
    nullptr);
  Track::create_with_action (
    Track::Type::AudioGroup, nullptr, nullptr, nullptr, start_pos + 2, 1, -1,
    nullptr);
  Track::create_with_action (
    Track::Type::AudioBus, nullptr, nullptr, nullptr, start_pos + 3, 1, -1,
    nullptr);

  auto audio_track = TRACKLIST->get_track<AudioTrack> (start_pos);
  auto group_track = TRACKLIST->get_track<AudioGroupTrack> (start_pos + 1);
  auto group_track2 = TRACKLIST->get_track<AudioGroupTrack> (start_pos + 2);
  auto fx_track = TRACKLIST->get_track<AudioBusTrack> (start_pos + 3);

  /* send from audio track to fx track */
  UNDO_MANAGER->perform (std::make_unique<ChannelSendConnectStereoAction> (
    *audio_track->channel_->sends_[0], *fx_track->processor_->stereo_in_,
    *PORT_CONNECTIONS_MGR));

  REQUIRE (TRACKLIST->get_track (start_pos)->is_audio ());
  REQUIRE (TRACKLIST->get_track (start_pos + 1)->is_audio_group ());
  REQUIRE (TRACKLIST->get_track (start_pos + 2)->is_audio_group ());
  REQUIRE (TRACKLIST->get_track (start_pos + 3)->is_audio_bus ());

  /* route audio track to group track */
  perform_set_direct_out (TRACKLIST->get_track (start_pos), group_track);

  /* route group track to group track 2 */
  perform_set_direct_out (TRACKLIST->get_track (start_pos + 1), group_track2);

  /* duplicate audio track and group track */
  TRACKLIST->tracks_[start_pos]->select (true, false, false);
  REQUIRE_SIZE_EQ (TRACKLIST_SELECTIONS->track_names_, 2);
  REQUIRE_EQ (TRACKLIST_SELECTIONS->track_names_[0], group_track->name_);
  REQUIRE_EQ (TRACKLIST_SELECTIONS->track_names_[1], audio_track->name_);
  AUDIO_ENGINE->wait_n_cycles (40);
  UNDO_MANAGER->perform (std::make_unique<CopyTracksAction> (
    *TRACKLIST_SELECTIONS->gen_tracklist_selections (), *PORT_CONNECTIONS_MGR,
    start_pos + 1));

  /* assert group of new audio track is group of original audio track */
  auto new_track = TRACKLIST->get_track<AudioTrack> (start_pos + 1);
  REQUIRE_NONNULL (new_track);
  REQUIRE (new_track->is_audio ());
  REQUIRE_EQ (
    new_track->channel_->output_name_hash_, group_track->get_name_hash ());
  REQUIRE (new_track->channel_->stereo_out_->get_l ().is_connected_to (
    group_track->processor_->stereo_in_->get_l ()));
  REQUIRE (new_track->channel_->stereo_out_->get_r ().is_connected_to (
    group_track->processor_->stereo_in_->get_r ()));

  /* assert group of new group track is group of original group track */
  auto new_group_track = TRACKLIST->get_track<AudioGroupTrack> (start_pos + 2);
  REQUIRE (new_group_track->is_audio_group ());
  REQUIRE_EQ (
    new_group_track->channel_->output_name_hash_,
    group_track2->get_name_hash ());
  REQUIRE (new_group_track->channel_->stereo_out_->get_l ().is_connected_to (
    group_track2->processor_->stereo_in_->get_l ()));
  REQUIRE (new_group_track->channel_->stereo_out_->get_r ().is_connected_to (
    group_track2->processor_->stereo_in_->get_r ()));

  /* assert new audio track sends connected */
  const auto &send = new_track->channel_->sends_[0];
  REQUIRE (send->validate ());
  REQUIRE (send->is_enabled ());

  UNDO_MANAGER->undo ();
  UNDO_MANAGER->redo ();

  /* test delete each track */
  for (int i = 0; i < 4; i++)
    {
      auto cur_track = TRACKLIST->get_track (start_pos + i);
      perform_delete_track (cur_track);
      REQUIRE (TRACKLIST->validate ());
      test_project_save_and_reload ();
      UNDO_MANAGER->undo ();
      UNDO_MANAGER->redo ();
      UNDO_MANAGER->undo ();
    }
}

TEST_CASE_FIXTURE (ZrythmFixture, "test track deletion with mixer selections")
{
  auto first_track = Track::create_empty_with_action<AudioBusTrack> ();

  int pl_track_pos = test_plugin_manager_create_tracks_from_plugin (
    EG_AMP_BUNDLE_URI, EG_AMP_URI, false, false, 1);
  auto pl_track = TRACKLIST->get_track<AudioBusTrack> (pl_track_pos);
  REQUIRE_NONNULL (pl_track);

  MIXER_SELECTIONS->add_slot (*pl_track, PluginSlotType::Insert, 0, false);
  REQUIRE (MIXER_SELECTIONS->has_any_);
  REQUIRE_EQ (MIXER_SELECTIONS->track_name_hash_, pl_track->get_name_hash ());

  first_track->select (true, true, false);
  perform_delete ();
}

TEST_CASE_FIXTURE (ZrythmFixture, "test instrument track duplicate with send")
{
#if defined HAVE_HELM
  /* create the instrument track */
  int ins_track_pos = TRACKLIST->get_num_tracks ();
  test_plugin_manager_create_tracks_from_plugin (
    HELM_BUNDLE, HELM_URI, true, false, 1);
  auto ins_track = TRACKLIST->get_track<InstrumentTrack> (ins_track_pos);
  REQUIRE_NONNULL (ins_track);

  /* create an audio fx track */
  int audio_fx_track_pos = TRACKLIST->get_num_tracks ();
  Track::create_with_action (
    Track::Type::AudioBus, nullptr, nullptr, nullptr, audio_fx_track_pos, 1, -1,
    nullptr);
  auto audio_fx_track = TRACKLIST->get_track<AudioBusTrack> (audio_fx_track_pos);
  REQUIRE_NONNULL (audio_fx_track);

  /* send from audio track to fx track */
  UNDO_MANAGER->perform (std::make_unique<ChannelSendConnectStereoAction> (
    *ins_track->channel_->sends_[0], *audio_fx_track->processor_->stereo_in_,
    *PORT_CONNECTIONS_MGR));

  /* duplicate the instrument track */
  ins_track->select (true, true, false);
  AUDIO_ENGINE->wait_n_cycles (40);
  UNDO_MANAGER->perform (std::make_unique<CopyTracksAction> (
    *TRACKLIST_SELECTIONS->gen_tracklist_selections (), *PORT_CONNECTIONS_MGR,
    audio_fx_track_pos));
  auto new_ins_track =
    TRACKLIST->get_track<InstrumentTrack> (audio_fx_track_pos);
  REQUIRE_NONNULL (new_ins_track);

  /* assert new instrument track send is connected to audio fx track */
  const auto &send = new_ins_track->channel_->sends_[0];
  REQUIRE (send->validate ());
  REQUIRE (send->is_enabled ());

  /* save and reload the project */
  test_project_save_and_reload ();
#endif
}

TEST_CASE_FIXTURE (ZrythmFixture, "test create midi fx track")
{
  /* create an audio fx track */
  Track::create_empty_with_action<MidiBusTrack> ();

  std::this_thread::sleep_for (std::chrono::milliseconds (10));
}

TEST_CASE_FIXTURE (ZrythmFixture, "move multiple tracks")
{
  /* create 5 tracks */
  for (int i = 0; i < 5; i++)
    {
      Track::create_empty_with_action<AudioBusTrack> ();
    }

  /* move the first 2 tracks to the end */
  const auto &track1 = TRACKLIST->tracks_[TRACKLIST->tracks_.size () - 5];
  const auto &track2 = TRACKLIST->tracks_[TRACKLIST->tracks_.size () - 4];
  const auto &track3 = TRACKLIST->tracks_[TRACKLIST->tracks_.size () - 3];
  const auto &track4 = TRACKLIST->tracks_[TRACKLIST->tracks_.size () - 2];
  const auto &track5 = TRACKLIST->tracks_[TRACKLIST->tracks_.size () - 1];
  track1->select (true, true, false);
  track2->select (true, false, false);
  REQUIRE_LT (track1->pos_, track2->pos_);
  perform_move_to_end ();

  REQUIRE_EQ (track3->pos_, TRACKLIST->tracks_.size () - 5);
  REQUIRE_EQ (track4->pos_, TRACKLIST->tracks_.size () - 4);
  REQUIRE_EQ (track5->pos_, TRACKLIST->tracks_.size () - 3);
  REQUIRE_EQ (track1->pos_, TRACKLIST->tracks_.size () - 2);
  REQUIRE_EQ (track2->pos_, TRACKLIST->tracks_.size () - 1);

  UNDO_MANAGER->undo ();
  REQUIRE_EQ (track1->pos_, TRACKLIST->tracks_.size () - 5);
  REQUIRE_EQ (track2->pos_, TRACKLIST->tracks_.size () - 4);
  REQUIRE_EQ (track3->pos_, TRACKLIST->tracks_.size () - 3);
  REQUIRE_EQ (track4->pos_, TRACKLIST->tracks_.size () - 2);
  REQUIRE_EQ (track5->pos_, TRACKLIST->tracks_.size () - 1);

  UNDO_MANAGER->redo ();
  REQUIRE_EQ (track3->pos_, TRACKLIST->tracks_.size () - 5);
  REQUIRE_EQ (track4->pos_, TRACKLIST->tracks_.size () - 4);
  REQUIRE_EQ (track5->pos_, TRACKLIST->tracks_.size () - 3);
  REQUIRE_EQ (track1->pos_, TRACKLIST->tracks_.size () - 2);
  REQUIRE_EQ (track2->pos_, TRACKLIST->tracks_.size () - 1);

  UNDO_MANAGER->undo ();
  REQUIRE_EQ (track1->pos_, TRACKLIST->tracks_.size () - 5);
  REQUIRE_EQ (track2->pos_, TRACKLIST->tracks_.size () - 4);
  REQUIRE_EQ (track3->pos_, TRACKLIST->tracks_.size () - 3);
  REQUIRE_EQ (track4->pos_, TRACKLIST->tracks_.size () - 2);
  REQUIRE_EQ (track5->pos_, TRACKLIST->tracks_.size () - 1);

  /* same (move first 2 tracks to end), but select in different order */
  track2->select (true, true, false);
  track1->select (true, false, false);
  perform_move_to_end ();

  REQUIRE_EQ (track3->pos_, TRACKLIST->tracks_.size () - 5);
  REQUIRE_EQ (track4->pos_, TRACKLIST->tracks_.size () - 4);
  REQUIRE_EQ (track5->pos_, TRACKLIST->tracks_.size () - 3);
  REQUIRE_EQ (track1->pos_, TRACKLIST->tracks_.size () - 2);
  REQUIRE_EQ (track2->pos_, TRACKLIST->tracks_.size () - 1);

  UNDO_MANAGER->undo ();
  REQUIRE_EQ (track1->pos_, TRACKLIST->tracks_.size () - 5);
  REQUIRE_EQ (track2->pos_, TRACKLIST->tracks_.size () - 4);
  REQUIRE_EQ (track3->pos_, TRACKLIST->tracks_.size () - 3);
  REQUIRE_EQ (track4->pos_, TRACKLIST->tracks_.size () - 2);
  REQUIRE_EQ (track5->pos_, TRACKLIST->tracks_.size () - 1);

  UNDO_MANAGER->redo ();
  REQUIRE_EQ (track3->pos_, TRACKLIST->tracks_.size () - 5);
  REQUIRE_EQ (track4->pos_, TRACKLIST->tracks_.size () - 4);
  REQUIRE_EQ (track5->pos_, TRACKLIST->tracks_.size () - 3);
  REQUIRE_EQ (track1->pos_, TRACKLIST->tracks_.size () - 2);
  REQUIRE_EQ (track2->pos_, TRACKLIST->tracks_.size () - 1);

  UNDO_MANAGER->undo ();
  REQUIRE_EQ (track1->pos_, TRACKLIST->tracks_.size () - 5);
  REQUIRE_EQ (track2->pos_, TRACKLIST->tracks_.size () - 4);
  REQUIRE_EQ (track3->pos_, TRACKLIST->tracks_.size () - 3);
  REQUIRE_EQ (track4->pos_, TRACKLIST->tracks_.size () - 2);
  REQUIRE_EQ (track5->pos_, TRACKLIST->tracks_.size () - 1);

  /* move the last 2 tracks up */
  track4->select (true, true, false);
  track5->select (true, false, false);
  REQUIRE_LT (track4->pos_, track5->pos_);
  UNDO_MANAGER->perform (std::make_unique<MoveTracksAction> (
    *TRACKLIST_SELECTIONS->gen_tracklist_selections (),
    TRACKLIST->get_num_tracks () - 5));

  REQUIRE_EQ (track4->pos_, TRACKLIST->tracks_.size () - 5);
  REQUIRE_EQ (track5->pos_, TRACKLIST->tracks_.size () - 4);
  REQUIRE_EQ (track1->pos_, TRACKLIST->tracks_.size () - 3);
  REQUIRE_EQ (track2->pos_, TRACKLIST->tracks_.size () - 2);
  REQUIRE_EQ (track3->pos_, TRACKLIST->tracks_.size () - 1);

  UNDO_MANAGER->undo ();
  REQUIRE_EQ (track1->pos_, TRACKLIST->tracks_.size () - 5);
  REQUIRE_EQ (track2->pos_, TRACKLIST->tracks_.size () - 4);
  REQUIRE_EQ (track3->pos_, TRACKLIST->tracks_.size () - 3);
  REQUIRE_EQ (track4->pos_, TRACKLIST->tracks_.size () - 2);
  REQUIRE_EQ (track5->pos_, TRACKLIST->tracks_.size () - 1);

  UNDO_MANAGER->redo ();
  REQUIRE_EQ (track4->pos_, TRACKLIST->tracks_.size () - 5);
  REQUIRE_EQ (track5->pos_, TRACKLIST->tracks_.size () - 4);
  REQUIRE_EQ (track1->pos_, TRACKLIST->tracks_.size () - 3);
  REQUIRE_EQ (track2->pos_, TRACKLIST->tracks_.size () - 2);
  REQUIRE_EQ (track3->pos_, TRACKLIST->tracks_.size () - 1);

  UNDO_MANAGER->undo ();
  REQUIRE_EQ (track1->pos_, TRACKLIST->tracks_.size () - 5);
  REQUIRE_EQ (track2->pos_, TRACKLIST->tracks_.size () - 4);
  REQUIRE_EQ (track3->pos_, TRACKLIST->tracks_.size () - 3);
  REQUIRE_EQ (track4->pos_, TRACKLIST->tracks_.size () - 2);
  REQUIRE_EQ (track5->pos_, TRACKLIST->tracks_.size () - 1);

  /* move the first and last tracks to the middle */
  track1->select (true, true, false);
  track5->select (true, false, false);
  UNDO_MANAGER->perform (std::make_unique<MoveTracksAction> (
    *TRACKLIST_SELECTIONS->gen_tracklist_selections (),
    TRACKLIST->get_num_tracks () - 2));

  REQUIRE_EQ (track2->pos_, TRACKLIST->tracks_.size () - 5);
  REQUIRE_EQ (track3->pos_, TRACKLIST->tracks_.size () - 4);
  REQUIRE_EQ (track1->pos_, TRACKLIST->tracks_.size () - 3);
  REQUIRE_EQ (track5->pos_, TRACKLIST->tracks_.size () - 2);
  REQUIRE_EQ (track4->pos_, TRACKLIST->tracks_.size () - 1);

  UNDO_MANAGER->undo ();
  REQUIRE_EQ (track1->pos_, TRACKLIST->tracks_.size () - 5);
  REQUIRE_EQ (track2->pos_, TRACKLIST->tracks_.size () - 4);
  REQUIRE_EQ (track3->pos_, TRACKLIST->tracks_.size () - 3);
  REQUIRE_EQ (track4->pos_, TRACKLIST->tracks_.size () - 2);
  REQUIRE_EQ (track5->pos_, TRACKLIST->tracks_.size () - 1);

  UNDO_MANAGER->redo ();
  REQUIRE_EQ (track2->pos_, TRACKLIST->tracks_.size () - 5);
  REQUIRE_EQ (track3->pos_, TRACKLIST->tracks_.size () - 4);
  REQUIRE_EQ (track1->pos_, TRACKLIST->tracks_.size () - 3);
  REQUIRE_EQ (track5->pos_, TRACKLIST->tracks_.size () - 2);
  REQUIRE_EQ (track4->pos_, TRACKLIST->tracks_.size () - 1);

  UNDO_MANAGER->undo ();
  REQUIRE_EQ (track1->pos_, TRACKLIST->tracks_.size () - 5);
  REQUIRE_EQ (track2->pos_, TRACKLIST->tracks_.size () - 4);
  REQUIRE_EQ (track3->pos_, TRACKLIST->tracks_.size () - 3);
  REQUIRE_EQ (track4->pos_, TRACKLIST->tracks_.size () - 2);
  REQUIRE_EQ (track5->pos_, TRACKLIST->tracks_.size () - 1);
}

static void
_test_move_inside (
  const char * pl_bundle,
  const char * pl_uri,
  bool         is_instrument,
  bool         with_carla)
{
  ZrythmFixture fixture;

  /* create folder track */
  auto folder = Track::create_empty_with_action<FolderTrack> ();

  /* create audio fx track */
  test_plugin_manager_create_tracks_from_plugin (
    pl_bundle, pl_uri, is_instrument, with_carla, 1);

  /* move audio fx inside folder */
  REQUIRE_EQ (folder->size_, 1);
  auto audio_fx = TRACKLIST->tracks_.back ().get ();
  audio_fx->select (true, true, false);
  TRACKLIST->handle_move_or_copy (
    *folder, TrackWidgetHighlight::TRACK_WIDGET_HIGHLIGHT_INSIDE,
    GDK_ACTION_MOVE);

  /* validate */
  REQUIRE_EQ (folder->pos_, TRACKLIST->tracks_.size () - 2);
  REQUIRE_EQ (audio_fx->pos_, TRACKLIST->tracks_.size () - 1);
  REQUIRE_EQ (folder->size_, 2);
  {
    std::vector<FoldableTrack *> parents;
    audio_fx->add_folder_parents (parents, false);
    REQUIRE_SIZE_EQ (parents, 1);
    REQUIRE_EQ (parents.front (), folder);
  }

  /* create audio group and move folder inside group */
  auto audio_group = Track::create_empty_with_action<AudioGroupTrack> ();
  folder->select (true, true, false);
  audio_fx->select (true, false, false);
  TRACKLIST->handle_move_or_copy (
    *audio_group, TrackWidgetHighlight::TRACK_WIDGET_HIGHLIGHT_INSIDE,
    GDK_ACTION_MOVE);

  /* validate */
  REQUIRE_EQ (folder->size_, 2);
  REQUIRE_EQ (audio_group->size_, 3);
  REQUIRE_EQ (audio_fx->pos_, TRACKLIST->tracks_.size () - 1);
  REQUIRE_EQ (folder->pos_, TRACKLIST->tracks_.size () - 2);
  REQUIRE_EQ (audio_group->pos_, TRACKLIST->tracks_.size () - 3);
  {
    std::vector<FoldableTrack *> parents;
    audio_fx->add_folder_parents (parents, false);
    REQUIRE_SIZE_EQ (parents, 2);
    REQUIRE_EQ (parents.at (0), audio_group);
    REQUIRE_EQ (parents.at (1), folder);
  }

  {
    std::vector<FoldableTrack *> parents;
    folder->add_folder_parents (parents, false);
    REQUIRE_SIZE_EQ (parents, 1);
    REQUIRE_EQ (parents.front (), audio_group);
  }

  /* add 3 more tracks */
  auto audio_fx2 = Track::create_empty_with_action<AudioBusTrack> ();
  auto folder2 = Track::create_empty_with_action<FolderTrack> ();
  auto audio_fx3 = Track::create_empty_with_action<AudioBusTrack> ();

  /* move 2 new fx tracks to folder */
  audio_fx2->select (true, true, false);
  audio_fx3->select (true, false, false);
  TRACKLIST->handle_move_or_copy (
    *folder2, TrackWidgetHighlight::TRACK_WIDGET_HIGHLIGHT_INSIDE,
    GDK_ACTION_MOVE);

  /* validate */
  REQUIRE_EQ (folder2->size_, 3);
  REQUIRE_EQ (folder2->pos_, TRACKLIST->tracks_.size () - 3);
  REQUIRE_EQ (audio_fx2->pos_, TRACKLIST->tracks_.size () - 2);
  REQUIRE_EQ (audio_fx3->pos_, TRACKLIST->tracks_.size () - 1);
  {
    std::vector<FoldableTrack *> parents;
    audio_fx2->add_folder_parents (parents, false);
    REQUIRE_SIZE_EQ (parents, 1);
    REQUIRE_EQ (parents.front (), folder2);
  }
  {
    std::vector<FoldableTrack *> parents;
    audio_fx3->add_folder_parents (parents, false);
    REQUIRE_SIZE_EQ (parents, 1);
    REQUIRE_EQ (parents.front (), folder2);
  }

  /* undo move 2 tracks inside */
  UNDO_MANAGER->undo ();

  /* undo create 3 tracks */
  UNDO_MANAGER->undo ();
  UNDO_MANAGER->undo ();
  UNDO_MANAGER->undo ();

  /* validate */
  REQUIRE_EQ (folder->size_, 2);
  REQUIRE_EQ (audio_group->size_, 3);
  REQUIRE_EQ (audio_fx->pos_, TRACKLIST->tracks_.size () - 1);
  REQUIRE_EQ (folder->pos_, TRACKLIST->tracks_.size () - 2);
  REQUIRE_EQ (audio_group->pos_, TRACKLIST->tracks_.size () - 3);
  {
    std::vector<FoldableTrack *> parents;
    audio_fx->add_folder_parents (parents, false);
    REQUIRE_SIZE_EQ (parents, 2);
    REQUIRE_EQ (parents.front (), audio_group);
    REQUIRE_EQ (parents.at (1), folder);
  }
  {
    std::vector<FoldableTrack *> parents;
    folder->add_folder_parents (parents, false);
    REQUIRE_SIZE_EQ (parents, 1);
    REQUIRE_EQ (parents.front (), audio_group);
  }

  /* undo move folder inside group */
  UNDO_MANAGER->undo ();

  UNDO_MANAGER->undo ();
  UNDO_MANAGER->redo ();
  UNDO_MANAGER->redo ();
  UNDO_MANAGER->redo ();
  UNDO_MANAGER->redo ();
  UNDO_MANAGER->redo ();
  UNDO_MANAGER->redo ();

  /*
   * [00] Chords
   * [01] Tempo
   * [02] Modulators
   * [03] Markers
   * [04] Master
   * [05] Audio Group Track
   * [06] -- Folder Track
   * [07] ---- LSP Compressor Stereo
   * [08] Folder Track 1
   * [09] -- Audio FX Track
   * [10] -- Audio FX Track 1
   */

  audio_group = TRACKLIST->get_track<AudioGroupTrack> (5);
  folder = TRACKLIST->get_track<FolderTrack> (6);
  auto lsp_comp = TRACKLIST->get_track<AudioBusTrack> (7);
  folder2 = TRACKLIST->get_track<FolderTrack> (8);
  audio_fx = TRACKLIST->get_track<AudioBusTrack> (9);
  audio_fx2 = TRACKLIST->get_track<AudioBusTrack> (10);
  REQUIRE_EQ (audio_group->pos_, 5);
  REQUIRE_EQ (audio_group->size_, 3);
  REQUIRE_EQ (folder->pos_, 6);
  REQUIRE_EQ (folder->size_, 2);
  REQUIRE_EQ (lsp_comp->pos_, 7);
  REQUIRE_EQ (folder2->pos_, 8);
  REQUIRE_EQ (folder2->size_, 3);
  REQUIRE_EQ (audio_fx->pos_, 9);
  REQUIRE_EQ (audio_fx2->pos_, 10);

  /* move folder 2 into folder 1 */
  folder2->select (true, true, false);
  TRACKLIST->handle_move_or_copy (
    *folder, TrackWidgetHighlight::TRACK_WIDGET_HIGHLIGHT_INSIDE,
    GDK_ACTION_MOVE);

  /*
   * expect:
   * [00] Chords
   * [01] Tempo
   * [02] Modulators
   * [03] Markers
   * [04] Master
   * [05] Audio Group Track
   * [06] -- Folder Track
   * [08] ---- Folder Track 1
   * [09] ------ Audio FX Track
   * [10] ------ Audio FX Track 1
   * [07] ---- LSP Compressor Stereo
   */
  REQUIRE_EQ (audio_group->pos_, 5);
  REQUIRE_EQ (audio_group->size_, 6);
  REQUIRE_EQ (folder->pos_, 6);
  REQUIRE_EQ (folder->size_, 5);
  REQUIRE_EQ (folder2->pos_, 7);
  REQUIRE_EQ (folder2->size_, 3);
  REQUIRE_EQ (audio_fx->pos_, 8);
  REQUIRE_EQ (audio_fx2->pos_, 9);
  REQUIRE_EQ (lsp_comp->pos_, 10);

  UNDO_MANAGER->undo ();
  REQUIRE_EQ (audio_group->pos_, 5);
  REQUIRE_EQ (audio_group->size_, 3);
  REQUIRE_EQ (folder->pos_, 6);
  REQUIRE_EQ (folder->size_, 2);
  REQUIRE_EQ (lsp_comp->pos_, 7);
  REQUIRE_EQ (folder2->pos_, 8);
  REQUIRE_EQ (folder2->size_, 3);
  REQUIRE_EQ (audio_fx->pos_, 9);
  REQUIRE_EQ (audio_fx2->pos_, 10);

  UNDO_MANAGER->redo ();
  REQUIRE_EQ (audio_group->pos_, 5);
  REQUIRE_EQ (audio_group->size_, 6);
  REQUIRE_EQ (folder->pos_, 6);
  REQUIRE_EQ (folder->size_, 5);
  REQUIRE_EQ (folder2->pos_, 7);
  REQUIRE_EQ (folder2->size_, 3);
  REQUIRE_EQ (audio_fx->pos_, 8);
  REQUIRE_EQ (audio_fx2->pos_, 9);
  REQUIRE_EQ (lsp_comp->pos_, 10);

  UNDO_MANAGER->undo ();

  /*
   * [00] Chords
   * [01] Tempo
   * [02] Modulators
   * [03] Markers
   * [04] Master
   * [05] Audio Group Track
   * [06] -- Folder Track
   * [07] ---- LSP Compressor Stereo
   * [08] Folder Track 1
   * [09] -- Audio FX Track
   * [10] -- Audio FX Track 1
   */

  /* move audio fx 2 above master */
  audio_fx2->select (true, true, false);
  TRACKLIST->handle_move_or_copy (
    *P_MASTER_TRACK, TrackWidgetHighlight::TRACK_WIDGET_HIGHLIGHT_TOP,
    GDK_ACTION_MOVE);
  REQUIRE_EQ (audio_fx2->pos_, 4);
  REQUIRE_EQ (audio_group->pos_, 6);
  REQUIRE_EQ (audio_group->size_, 3);
  REQUIRE_EQ (folder->pos_, 7);
  REQUIRE_EQ (folder->size_, 2);
  REQUIRE_EQ (lsp_comp->pos_, 8);
  REQUIRE_EQ (folder2->pos_, 9);
  REQUIRE_EQ (folder2->size_, 2);
  REQUIRE_EQ (audio_fx->pos_, 10);

  UNDO_MANAGER->undo ();
  REQUIRE_EQ (audio_group->pos_, 5);
  REQUIRE_EQ (audio_group->size_, 3);
  REQUIRE_EQ (folder->pos_, 6);
  REQUIRE_EQ (folder->size_, 2);
  REQUIRE_EQ (lsp_comp->pos_, 7);
  REQUIRE_EQ (folder2->pos_, 8);
  REQUIRE_EQ (folder2->size_, 3);
  REQUIRE_EQ (audio_fx->pos_, 9);
  REQUIRE_EQ (audio_fx2->pos_, 10);

  UNDO_MANAGER->redo ();
  REQUIRE_EQ (audio_fx2->pos_, 4);
  REQUIRE_EQ (audio_group->pos_, 6);
  REQUIRE_EQ (audio_group->size_, 3);
  REQUIRE_EQ (folder->pos_, 7);
  REQUIRE_EQ (folder->size_, 2);
  REQUIRE_EQ (lsp_comp->pos_, 8);
  REQUIRE_EQ (folder2->pos_, 9);
  REQUIRE_EQ (folder2->size_, 2);
  REQUIRE_EQ (audio_fx->pos_, 10);

  UNDO_MANAGER->undo ();

  /*
   * [00] Chords
   * [01] Tempo
   * [02] Modulators
   * [03] Markers
   * [04] Master
   * [05] Audio Group Track
   * [06] -- Folder Track
   * [07] ---- LSP Compressor Stereo
   * [08] Folder Track 1
   * [09] -- Audio FX Track
   * [10] -- Audio FX Track 1
   */

  /* move audio fx 2 to audio group */
  audio_fx2->select (true, true, false);
  TRACKLIST->handle_move_or_copy (
    *audio_group, TrackWidgetHighlight::TRACK_WIDGET_HIGHLIGHT_INSIDE,
    GDK_ACTION_MOVE);
  REQUIRE_EQ (audio_group->pos_, 5);
  REQUIRE_EQ (audio_group->size_, 4);
  REQUIRE_EQ (audio_fx2->pos_, 6);
  REQUIRE_EQ (folder->pos_, 7);
  REQUIRE_EQ (folder->size_, 2);
  REQUIRE_EQ (lsp_comp->pos_, 8);
  REQUIRE_EQ (folder2->pos_, 9);
  REQUIRE_EQ (folder2->size_, 2);
  REQUIRE_EQ (audio_fx->pos_, 10);

  /* move audio fx 2 to folder */
  audio_fx2->select (true, true, false);
  TRACKLIST->handle_move_or_copy (
    *folder, TrackWidgetHighlight::TRACK_WIDGET_HIGHLIGHT_INSIDE,
    GDK_ACTION_MOVE);
  REQUIRE_EQ (audio_group->pos_, 5);
  REQUIRE_EQ (audio_group->size_, 4);
  REQUIRE_EQ (folder->pos_, 6);
  REQUIRE_EQ (folder->size_, 3);
  REQUIRE_EQ (audio_fx2->pos_, 7);
  REQUIRE_EQ (lsp_comp->pos_, 8);
  REQUIRE_EQ (folder2->pos_, 9);
  REQUIRE_EQ (folder2->size_, 2);
  REQUIRE_EQ (audio_fx->pos_, 10);

  UNDO_MANAGER->undo ();
  REQUIRE_EQ (audio_group->pos_, 5);
  REQUIRE_EQ (audio_group->size_, 4);
  REQUIRE_EQ (audio_fx2->pos_, 6);
  REQUIRE_EQ (folder->pos_, 7);
  REQUIRE_EQ (folder->size_, 2);
  REQUIRE_EQ (lsp_comp->pos_, 8);
  REQUIRE_EQ (folder2->pos_, 9);
  REQUIRE_EQ (folder2->size_, 2);
  REQUIRE_EQ (audio_fx->pos_, 10);

  UNDO_MANAGER->undo ();
  REQUIRE_EQ (audio_group->pos_, 5);
  REQUIRE_EQ (audio_group->size_, 3);
  REQUIRE_EQ (folder->pos_, 6);
  REQUIRE_EQ (folder->size_, 2);
  REQUIRE_EQ (lsp_comp->pos_, 7);
  REQUIRE_EQ (folder2->pos_, 8);
  REQUIRE_EQ (folder2->size_, 3);
  REQUIRE_EQ (audio_fx->pos_, 9);
  REQUIRE_EQ (audio_fx2->pos_, 10);

  /*
   * [00] Chords
   * [01] Tempo
   * [02] Modulators
   * [03] Markers
   * [04] Master
   * [05] Audio Group Track
   * [06] -- Folder Track
   * [07] ---- LSP Compressor Stereo
   * [08] Folder Track 1
   * [09] -- Audio FX Track
   * [10] -- Audio FX Track 1
   */

  /* move folder inside itself */
  folder->select (true, true, false);
  TRACKLIST->handle_move_or_copy (
    *folder, TrackWidgetHighlight::TRACK_WIDGET_HIGHLIGHT_INSIDE,
    GDK_ACTION_MOVE);

  /* move chanel-less track into folder and get
   * status */
  P_MARKER_TRACK->select (true, true, false);
  TRACKLIST->handle_move_or_copy (
    *folder, TrackWidgetHighlight::TRACK_WIDGET_HIGHLIGHT_INSIDE,
    GDK_ACTION_MOVE);
  folder->is_status (FolderTrack::MixerStatus::Muted);
  UNDO_MANAGER->undo ();

  /* delete a track inside the folder */
  perform_delete_track (audio_fx);
  REQUIRE_EQ (audio_group->pos_, 5);
  REQUIRE_EQ (audio_group->size_, 3);
  REQUIRE_EQ (folder->pos_, 6);
  REQUIRE_EQ (folder->size_, 2);
  REQUIRE_EQ (lsp_comp->pos_, 7);
  REQUIRE_EQ (folder2->pos_, 8);
  REQUIRE_EQ (folder2->size_, 2);
  REQUIRE_EQ (audio_fx2->pos_, 9);

  UNDO_MANAGER->undo ();
  audio_fx = TRACKLIST->get_track<AudioBusTrack> (9);
  REQUIRE_EQ (audio_group->pos_, 5);
  REQUIRE_EQ (audio_group->size_, 3);
  REQUIRE_EQ (folder->pos_, 6);
  REQUIRE_EQ (folder->size_, 2);
  REQUIRE_EQ (lsp_comp->pos_, 7);
  REQUIRE_EQ (folder2->pos_, 8);
  REQUIRE_EQ (folder2->size_, 3);
  REQUIRE_EQ (audio_fx->pos_, 9);
  REQUIRE_EQ (audio_fx2->pos_, 10);

  /*
   * [00] Chords
   * [01] Tempo
   * [02] Modulators
   * [03] Markers
   * [04] Master
   * [05] Audio Group Track
   * [06] -- Folder Track
   * [07] ---- LSP Compressor Stereo
   * [08] Folder Track 1
   * [09] -- Audio FX Track
   * [10] -- Audio FX Track 1
   */

  /* create a new track and drag it under
   * Folder Track 1 */
  auto midi = Track::create_empty_with_action<MidiTrack> ();
  REQUIRE_NONNULL (midi);

  TRACKLIST->handle_move_or_copy (
    *audio_fx2, TrackWidgetHighlight::TRACK_WIDGET_HIGHLIGHT_TOP,
    GDK_ACTION_MOVE);

  REQUIRE_EQ (audio_group->pos_, 5);
  REQUIRE_EQ (audio_group->size_, 3);
  REQUIRE_EQ (folder->pos_, 6);
  REQUIRE_EQ (folder->size_, 2);
  REQUIRE_EQ (lsp_comp->pos_, 7);
  REQUIRE_EQ (folder2->pos_, 8);
  REQUIRE_EQ (folder2->size_, 4);
  REQUIRE_EQ (audio_fx->pos_, 9);
  REQUIRE_EQ (midi->pos_, 10);
  REQUIRE_EQ (audio_fx2->pos_, 11);

  UNDO_MANAGER->undo ();
  REQUIRE_EQ (audio_group->pos_, 5);
  REQUIRE_EQ (audio_group->size_, 3);
  REQUIRE_EQ (folder->pos_, 6);
  REQUIRE_EQ (folder->size_, 2);
  REQUIRE_EQ (lsp_comp->pos_, 7);
  REQUIRE_EQ (folder2->pos_, 8);
  REQUIRE_EQ (folder2->size_, 3);
  REQUIRE_EQ (audio_fx->pos_, 9);
  REQUIRE_EQ (audio_fx2->pos_, 10);
}

TEST_CASE ("move inside")
{
  (void) _test_move_inside;
#ifdef HAVE_LSP_COMPRESSOR
  _test_move_inside (LSP_COMPRESSOR_BUNDLE, LSP_COMPRESSOR_URI, false, false);
#endif
}

TEST_CASE_FIXTURE (ZrythmFixture, "move multiple inside")
{
  /* create folder track */
  auto folder = Track::create_empty_with_action<FolderTrack> ();

  /* create 2 audio fx tracks */
  auto audio_fx1 = Track::create_empty_with_action<AudioBusTrack> ();
  auto audio_fx2 = Track::create_empty_with_action<AudioBusTrack> ();

  /* move audio fx 1 inside folder */
  audio_fx1->select (true, true, false);
  TRACKLIST->handle_move_or_copy (
    *folder, TrackWidgetHighlight::TRACK_WIDGET_HIGHLIGHT_INSIDE,
    GDK_ACTION_MOVE);
  REQUIRE_EQ (folder->size_, 2);
  REQUIRE_EQ (folder->pos_, TRACKLIST->get_num_tracks () - 3);
  REQUIRE_EQ (audio_fx1->pos_, TRACKLIST->get_num_tracks () - 2);
  REQUIRE_EQ (audio_fx2->pos_, TRACKLIST->get_num_tracks () - 1);

  UNDO_MANAGER->undo ();
  REQUIRE_EQ (folder->size_, 1);
  REQUIRE_EQ (folder->pos_, TRACKLIST->get_num_tracks () - 3);
  REQUIRE_EQ (audio_fx1->pos_, TRACKLIST->get_num_tracks () - 2);
  REQUIRE_EQ (audio_fx2->pos_, TRACKLIST->get_num_tracks () - 1);

  UNDO_MANAGER->redo ();
  REQUIRE_EQ (folder->size_, 2);
  REQUIRE_EQ (folder->pos_, TRACKLIST->get_num_tracks () - 3);
  REQUIRE_EQ (audio_fx1->pos_, TRACKLIST->get_num_tracks () - 2);
  REQUIRE_EQ (audio_fx2->pos_, TRACKLIST->get_num_tracks () - 1);

  /* create audio group */
  auto audio_group = Track::create_empty_with_action<AudioGroupTrack> ();

  /* move folder to audio group */
  folder->select (true, true, false);
  audio_fx1->select (true, false, false);
  TRACKLIST->handle_move_or_copy (
    *audio_group, TrackWidgetHighlight::TRACK_WIDGET_HIGHLIGHT_INSIDE,
    GDK_ACTION_MOVE);
  REQUIRE_EQ (folder->size_, 2);
  REQUIRE_EQ (audio_group->size_, 3);
  REQUIRE_EQ (audio_fx2->pos_, TRACKLIST->get_num_tracks () - 4);
  REQUIRE_EQ (audio_group->pos_, TRACKLIST->get_num_tracks () - 3);
  REQUIRE_EQ (folder->pos_, TRACKLIST->get_num_tracks () - 2);
  REQUIRE_EQ (audio_fx1->pos_, TRACKLIST->get_num_tracks () - 1);

  UNDO_MANAGER->undo ();
  REQUIRE_EQ (folder->size_, 2);
  REQUIRE_EQ (audio_group->size_, 1);
  REQUIRE_EQ (folder->pos_, TRACKLIST->get_num_tracks () - 4);
  REQUIRE_EQ (audio_fx1->pos_, TRACKLIST->get_num_tracks () - 3);
  REQUIRE_EQ (audio_fx2->pos_, TRACKLIST->get_num_tracks () - 2);
  REQUIRE_EQ (audio_group->pos_, TRACKLIST->get_num_tracks () - 1);

  UNDO_MANAGER->redo ();
  REQUIRE_EQ (folder->size_, 2);
  REQUIRE_EQ (audio_group->size_, 3);
  REQUIRE_EQ (audio_fx2->pos_, TRACKLIST->get_num_tracks () - 4);
  REQUIRE_EQ (audio_group->pos_, TRACKLIST->get_num_tracks () - 3);
  REQUIRE_EQ (folder->pos_, TRACKLIST->get_num_tracks () - 2);
  REQUIRE_EQ (audio_fx1->pos_, TRACKLIST->get_num_tracks () - 1);

  /*
   * [0] Chords
   * [1] Tempo
   * [2] Modulators
   * [3] Markers
   * [4] Master
   * [5] Audio FX Track 1
   * [6] Audio Group Track
   * [7] -- Folder Track
   * [8] ---- Audio FX Track
   */

  /* create 2 new MIDI tracks and drag them above Folder Track */
  auto midi = Track::create_empty_with_action<MidiTrack> ();
  REQUIRE_NONNULL (midi);
  auto midi2 = Track::create_empty_with_action<MidiTrack> ();
  REQUIRE_NONNULL (midi2);
  midi->select (true, true, false);
  midi2->select (true, false, false);

  REQUIRE_EQ (folder->size_, 2);
  REQUIRE_EQ (audio_fx2->pos_, 5);
  REQUIRE_EQ (audio_group->pos_, 6);
  REQUIRE_EQ (audio_group->size_, 3);
  REQUIRE_EQ (folder->pos_, 7);
  REQUIRE_EQ (folder->size_, 2);
  REQUIRE_EQ (audio_fx1->pos_, 8);
  REQUIRE_EQ (midi->pos_, 9);
  REQUIRE_EQ (midi2->pos_, 10);

  TRACKLIST->handle_move_or_copy (
    *folder, TrackWidgetHighlight::TRACK_WIDGET_HIGHLIGHT_TOP, GDK_ACTION_MOVE);

  REQUIRE_EQ (folder->size_, 2);
  REQUIRE_EQ (audio_fx2->pos_, 5);
  REQUIRE_EQ (audio_group->pos_, 6);
  REQUIRE_EQ (audio_group->size_, 5);
  REQUIRE_EQ (midi->pos_, 7);
  REQUIRE_EQ (midi2->pos_, 8);
  REQUIRE_EQ (folder->pos_, 9);
  REQUIRE_EQ (folder->size_, 2);
  REQUIRE_EQ (audio_fx1->pos_, 10);

  UNDO_MANAGER->undo ();
  REQUIRE_EQ (folder->size_, 2);
  REQUIRE_EQ (audio_fx2->pos_, 5);
  REQUIRE_EQ (audio_group->pos_, 6);
  REQUIRE_EQ (audio_group->size_, 3);
  REQUIRE_EQ (folder->pos_, 7);
  REQUIRE_EQ (audio_fx1->pos_, 8);
  REQUIRE_EQ (midi->pos_, 9);
  REQUIRE_EQ (midi2->pos_, 10);
}

TEST_CASE_FIXTURE (ZrythmFixture, "copy multiple inside")
{
  /* create folder track */
  auto folder = Track::create_empty_with_action<FolderTrack> ();

  /* create 2 audio fx tracks */
  auto audio_fx1 = Track::create_empty_with_action<AudioBusTrack> ();
  auto audio_fx2 = Track::create_empty_with_action<AudioBusTrack> ();

  /* move audio fx 1 inside folder */
  audio_fx1->select (true, true, false);
  TRACKLIST->handle_move_or_copy (
    *folder, TrackWidgetHighlight::TRACK_WIDGET_HIGHLIGHT_INSIDE,
    GDK_ACTION_MOVE);

  /* create audio group */
  auto audio_group = Track::create_empty_with_action<AudioGroupTrack> ();

  /* move folder to audio group */
  folder->select (true, true, false);
  audio_fx1->select (true, false, false);
  TRACKLIST->handle_move_or_copy (
    *audio_group, TrackWidgetHighlight::TRACK_WIDGET_HIGHLIGHT_INSIDE,
    GDK_ACTION_MOVE);

  /* create new folder */
  auto folder2 = Track::create_empty_with_action<FolderTrack> ();

  /* copy-move audio group inside new folder */
  audio_group->select (true, true, false);
  folder->select (true, false, false);
  audio_fx1->select (true, false, false);
  TRACKLIST->handle_move_or_copy (
    *folder2, TrackWidgetHighlight::TRACK_WIDGET_HIGHLIGHT_INSIDE,
    GDK_ACTION_COPY);
  REQUIRE_EQ (folder->size_, 2);
  REQUIRE_EQ (audio_group->size_, 3);
  REQUIRE_EQ (audio_fx2->pos_, TRACKLIST->get_num_tracks () - 8);
  REQUIRE_EQ (audio_group->pos_, TRACKLIST->get_num_tracks () - 7);
  REQUIRE_EQ (folder->pos_, TRACKLIST->get_num_tracks () - 6);
  REQUIRE_EQ (audio_fx1->pos_, TRACKLIST->get_num_tracks () - 5);
  REQUIRE_EQ (folder2->pos_, TRACKLIST->get_num_tracks () - 4);
  REQUIRE_EQ (folder2->size_, 4);
  REQUIRE (
    TRACKLIST->get_track (TRACKLIST->get_num_tracks () - 3)->is_audio_group ());
  REQUIRE (
    TRACKLIST->get_track (TRACKLIST->get_num_tracks () - 2)->is_folder ());
  REQUIRE (
    TRACKLIST->get_track (TRACKLIST->get_num_tracks () - 1)->is_audio_bus ());

  UNDO_MANAGER->undo ();
  REQUIRE_EQ (folder->size_, 2);
  REQUIRE_EQ (audio_group->size_, 3);
  REQUIRE_EQ (audio_fx2->pos_, TRACKLIST->get_num_tracks () - 5);
  REQUIRE_EQ (audio_group->pos_, TRACKLIST->get_num_tracks () - 4);
  REQUIRE_EQ (folder->pos_, TRACKLIST->get_num_tracks () - 3);
  REQUIRE_EQ (audio_fx1->pos_, TRACKLIST->get_num_tracks () - 2);
  REQUIRE_EQ (folder2->pos_, TRACKLIST->get_num_tracks () - 1);

  UNDO_MANAGER->redo ();
  REQUIRE_EQ (folder->size_, 2);
  REQUIRE_EQ (audio_group->size_, 3);
  REQUIRE_EQ (audio_fx2->pos_, TRACKLIST->get_num_tracks () - 8);
  REQUIRE_EQ (audio_group->pos_, TRACKLIST->get_num_tracks () - 7);
  REQUIRE_EQ (folder->pos_, TRACKLIST->get_num_tracks () - 6);
  REQUIRE_EQ (audio_fx1->pos_, TRACKLIST->get_num_tracks () - 5);
  REQUIRE_EQ (folder2->pos_, TRACKLIST->get_num_tracks () - 4);
  REQUIRE_EQ (folder2->size_, 4);
  REQUIRE (
    TRACKLIST->get_track (TRACKLIST->get_num_tracks () - 3)->is_audio_group ());
  REQUIRE (
    TRACKLIST->get_track (TRACKLIST->get_num_tracks () - 2)->is_folder ());
  REQUIRE (
    TRACKLIST->get_track (TRACKLIST->get_num_tracks () - 1)->is_audio_bus ());

  /*
   * [0] Chords
   * [1] Tempo
   * [2] Modulators
   * [3] Markers
   * [4] Master
   * [5] Audio FX Track 1
   * [6] Audio Group Track
   * [7] -- Folder Track
   * [8] ---- Audio FX Track
   * [009] Folder Track 1
   * [010] -- Audio Group Track 1
   * [011] ---- Folder Track 2
   * [012] ------ Audio FX Track 2
   */

  auto audio_group2 = TRACKLIST->get_track<AudioGroupTrack> (10);
  auto folder3 = TRACKLIST->get_track<FolderTrack> (11);
  auto audio_fx3 = TRACKLIST->get_track<AudioBusTrack> (12);

  /* create 2 new MIDI tracks and copy-drag them
   * above Audio Group Track 1 */
  auto midi = Track::create_empty_with_action<MidiTrack> ();
  REQUIRE_NONNULL (midi);
  auto midi2 = Track::create_empty_with_action<MidiTrack> ();
  REQUIRE_NONNULL (midi2);

  midi->select (true, true, false);
  midi2->select (true, false, false);
  TRACKLIST->handle_move_or_copy (
    *audio_group2, TrackWidgetHighlight::TRACK_WIDGET_HIGHLIGHT_TOP,
    GDK_ACTION_COPY);

  /*
   * [0] Chords
   * [1] Tempo
   * [2] Modulators
   * [3] Markers
   * [4] Master
   * [5] Audio FX Track 1
   * [6] Audio Group Track
   * [7] -- Folder Track
   * [8] ---- Audio FX Track
   * [009] Folder Track 1
   * [010] -- MIDI Track 2
   * [011] -- MIDI Track 3
   * [012] -- Audio Group Track 1
   * [013] ---- Folder Track 2
   * [014] ------ Audio FX Track 2
   * [015] -- MIDI Track
   * [016] -- MIDI Track 1
   */

  auto midi3 = TRACKLIST->get_track<MidiTrack> (10);
  auto midi4 = TRACKLIST->get_track<MidiTrack> (11);
  REQUIRE_EQ (folder2->pos_, 9);
  REQUIRE_EQ (folder2->size_, 6);
  REQUIRE_EQ (midi3->pos_, 10);
  REQUIRE_EQ (midi4->pos_, 11);
  REQUIRE_EQ (audio_group2->pos_, 12);
  REQUIRE_EQ (audio_group2->size_, 3);
  REQUIRE_EQ (folder3->pos_, 13);
  REQUIRE_EQ (folder3->size_, 2);
  REQUIRE_EQ (audio_fx3->pos_, 14);
  REQUIRE_EQ (midi->pos_, 15);
  REQUIRE_EQ (midi2->pos_, 16);

  UNDO_MANAGER->undo ();
  REQUIRE_EQ (folder2->pos_, 9);
  REQUIRE_EQ (folder2->size_, 4);
  REQUIRE_EQ (audio_group2->pos_, 10);
  REQUIRE_EQ (audio_group2->size_, 3);
  REQUIRE_EQ (folder3->pos_, 11);
  REQUIRE_EQ (folder3->size_, 2);
  REQUIRE_EQ (audio_fx3->pos_, 12);
  REQUIRE_EQ (midi->pos_, 13);
  REQUIRE_EQ (midi2->pos_, 14);

  UNDO_MANAGER->redo ();
  midi3 = TRACKLIST->get_track<MidiTrack> (10);
  midi4 = TRACKLIST->get_track<MidiTrack> (11);
  REQUIRE_EQ (folder2->pos_, 9);
  REQUIRE_EQ (folder2->size_, 6);
  REQUIRE_EQ (midi3->pos_, 10);
  REQUIRE_EQ (midi4->pos_, 11);
  REQUIRE_EQ (audio_group2->pos_, 12);
  REQUIRE_EQ (audio_group2->size_, 3);
  REQUIRE_EQ (folder3->pos_, 13);
  REQUIRE_EQ (folder3->size_, 2);
  REQUIRE_EQ (audio_fx3->pos_, 14);
  REQUIRE_EQ (midi->pos_, 15);
  REQUIRE_EQ (midi2->pos_, 16);

  UNDO_MANAGER->undo ();
  REQUIRE_EQ (folder2->pos_, 9);
  REQUIRE_EQ (folder2->size_, 4);
  REQUIRE_EQ (audio_group2->pos_, 10);
  REQUIRE_EQ (audio_group2->size_, 3);
  REQUIRE_EQ (folder3->pos_, 11);
  REQUIRE_EQ (folder3->size_, 2);
  REQUIRE_EQ (audio_fx3->pos_, 12);
  REQUIRE_EQ (midi->pos_, 13);
  REQUIRE_EQ (midi2->pos_, 14);

  /*
   * [0] Chords
   * [1] Tempo
   * [2] Modulators
   * [3] Markers
   * [4] Master
   * [5] Audio FX Track 1
   * [6] Audio Group Track
   * [7] -- Folder Track
   * [8] ---- Audio FX Track
   * [009] Folder Track 1
   * [010] -- Audio Group Track 1
   * [011] ---- Folder Track 2
   * [012] ------ Audio FX Track 2
   * [013] MIDI Track
   * [014] MIDI Track 1
   */

  /* move MIDI tracks to Folder Track 2 */
  midi->select (true, true, false);
  midi2->select (true, false, false);
  REQUIRE_EQ (TRACKLIST_SELECTIONS->get_num_tracks (), 2);
  TRACKLIST->handle_move_or_copy (
    *folder3, TrackWidgetHighlight::TRACK_WIDGET_HIGHLIGHT_BOTTOM,
    GDK_ACTION_MOVE);

  REQUIRE_EQ (folder2->pos_, 9);
  REQUIRE_EQ (folder2->size_, 6);
  REQUIRE_EQ (audio_group2->pos_, 10);
  REQUIRE_EQ (audio_group2->size_, 5);
  REQUIRE_EQ (folder3->pos_, 11);
  REQUIRE_EQ (folder3->size_, 4);
  REQUIRE_EQ (midi->pos_, 12);
  REQUIRE_EQ (midi2->pos_, 13);
  REQUIRE_EQ (audio_fx3->pos_, 14);

  /* clone the midi tracks at the end */
  midi->select (true, true, false);
  midi2->select (true, false, false);
  TRACKLIST->handle_move_or_copy (
    *audio_fx3, TrackWidgetHighlight::TRACK_WIDGET_HIGHLIGHT_BOTTOM,
    GDK_ACTION_COPY);

  midi3 = TRACKLIST->get_track<MidiTrack> (15);
  midi4 = TRACKLIST->get_track<MidiTrack> (16);
  REQUIRE_EQ (folder2->pos_, 9);
  REQUIRE_EQ (folder2->size_, 6);
  REQUIRE_EQ (audio_group2->pos_, 10);
  REQUIRE_EQ (audio_group2->size_, 5);
  REQUIRE_EQ (folder3->pos_, 11);
  REQUIRE_EQ (folder3->size_, 4);
  REQUIRE_EQ (midi->pos_, 12);
  REQUIRE_EQ (midi2->pos_, 13);
  REQUIRE_EQ (audio_fx3->pos_, 14);
  REQUIRE_EQ (midi3->pos_, 15);
  REQUIRE_EQ (midi4->pos_, 16);

  /*
   * [000] Chords
   * [001] Tempo
   * [002] Modulators
   * [003] Markers
   * [004] Master
   * [005] Audio FX Track 1
   * [006] Audio Group Track
   * [007] -- Folder Track
   * [008] ---- Audio FX Track
   * [009] Folder Track 1
   * [010] -- Audio Group Track 1
   * [011] ---- Folder Track 2
   * [012] ------ MIDI Track
   * [013] ------ MIDI Track 1
   * [014] ------ Audio FX Track 2
   * [015] MIDI Track 2
   * [016] MIDI Track 3
   */

  /* move MIDI tracks below master */
  midi3->select (true, true, false);
  midi4->select (true, false, false);
  TRACKLIST->handle_move_or_copy (
    *P_MASTER_TRACK, TrackWidgetHighlight::TRACK_WIDGET_HIGHLIGHT_BOTTOM,
    GDK_ACTION_MOVE);

  /* copy MIDI tracks below MIDI Track 1 */
  midi3->select (true, true, false);
  midi4->select (true, false, false);
  TRACKLIST->handle_move_or_copy (
    *midi2, TrackWidgetHighlight::TRACK_WIDGET_HIGHLIGHT_BOTTOM,
    GDK_ACTION_COPY);

  auto midi5 = TRACKLIST->get_track<MidiTrack> (16);
  auto midi6 = TRACKLIST->get_track<MidiTrack> (17);
  REQUIRE_EQ (folder2->pos_, 11);
  REQUIRE_EQ (folder2->size_, 8);
  REQUIRE_EQ (audio_group2->pos_, 12);
  REQUIRE_EQ (audio_group2->size_, 7);
  REQUIRE_EQ (folder3->pos_, 13);
  REQUIRE_EQ (folder3->size_, 6);
  REQUIRE_EQ (midi->pos_, 14);
  REQUIRE_EQ (midi2->pos_, 15);
  REQUIRE_EQ (midi5->pos_, 16);
  REQUIRE_EQ (midi6->pos_, 17);
  REQUIRE_EQ (audio_fx3->pos_, 18);

  /* undo */
  UNDO_MANAGER->undo ();
  REQUIRE_EQ (folder2->pos_, 11);
  REQUIRE_EQ (folder2->size_, 6);
  REQUIRE_EQ (audio_group2->pos_, 12);
  REQUIRE_EQ (audio_group2->size_, 5);
  REQUIRE_EQ (folder3->pos_, 13);
  REQUIRE_EQ (folder3->size_, 4);
  REQUIRE_EQ (midi->pos_, 14);
  REQUIRE_EQ (midi2->pos_, 15);
  REQUIRE_EQ (audio_fx3->pos_, 16);

  /* move MIDI Track 3 at the end */
  midi4->select (true, true, false);
  TRACKLIST->handle_move_or_copy (
    *audio_fx3, TrackWidgetHighlight::TRACK_WIDGET_HIGHLIGHT_BOTTOM,
    GDK_ACTION_MOVE);

  REQUIRE_EQ (folder2->pos_, 10);
  REQUIRE_EQ (folder2->size_, 6);
  REQUIRE_EQ (audio_group2->pos_, 11);
  REQUIRE_EQ (audio_group2->size_, 5);
  REQUIRE_EQ (folder3->pos_, 12);
  REQUIRE_EQ (folder3->size_, 4);
  REQUIRE_EQ (midi->pos_, 13);
  REQUIRE_EQ (midi2->pos_, 14);
  REQUIRE_EQ (audio_fx3->pos_, 15);
  REQUIRE_EQ (midi4->pos_, 16);

  /* copy MIDI Track 3 and MIDI Track 2 below
   * MIDI Track 1 */
  midi3->select (true, true, false);
  midi4->select (true, false, false);
  TRACKLIST->handle_move_or_copy (
    *midi2, TrackWidgetHighlight::TRACK_WIDGET_HIGHLIGHT_BOTTOM,
    GDK_ACTION_COPY);

  midi5 = TRACKLIST->get_track<MidiTrack> (15);
  midi6 = TRACKLIST->get_track<MidiTrack> (16);
  REQUIRE_EQ (folder2->pos_, 10);
  REQUIRE_EQ (folder2->size_, 8);
  REQUIRE_EQ (audio_group2->pos_, 11);
  REQUIRE_EQ (audio_group2->size_, 7);
  REQUIRE_EQ (folder3->pos_, 12);
  REQUIRE_EQ (folder3->size_, 6);
  REQUIRE_EQ (midi->pos_, 13);
  REQUIRE_EQ (midi2->pos_, 14);
  REQUIRE_EQ (midi5->pos_, 15);
  REQUIRE_EQ (midi6->pos_, 16);
  REQUIRE_EQ (audio_fx3->pos_, 17);
  REQUIRE_EQ (midi4->pos_, 18);
  REQUIRE_EQ (TRACKLIST->get_num_tracks (), 19);

  /* undo */
  UNDO_MANAGER->undo ();
  REQUIRE_EQ (folder2->pos_, 10);
  REQUIRE_EQ (folder2->size_, 6);
  REQUIRE_EQ (audio_group2->pos_, 11);
  REQUIRE_EQ (audio_group2->size_, 5);
  REQUIRE_EQ (folder3->pos_, 12);
  REQUIRE_EQ (folder3->size_, 4);
  REQUIRE_EQ (midi->pos_, 13);
  REQUIRE_EQ (midi2->pos_, 14);
  REQUIRE_EQ (audio_fx3->pos_, 15);
  REQUIRE_EQ (midi4->pos_, 16);
  REQUIRE_EQ (TRACKLIST->get_num_tracks (), 17);

  /* move MIDI Track 3 and MIDI Track 2 below
   * MIDI Track 1 */
  midi3->select (true, true, false);
  midi4->select (true, false, false);
  TRACKLIST->handle_move_or_copy (
    *midi2, TrackWidgetHighlight::TRACK_WIDGET_HIGHLIGHT_BOTTOM,
    GDK_ACTION_MOVE);

  REQUIRE_EQ (folder2->pos_, 9);
  REQUIRE_EQ (folder2->size_, 8);
  REQUIRE_EQ (audio_group2->pos_, 10);
  REQUIRE_EQ (audio_group2->size_, 7);
  REQUIRE_EQ (folder3->pos_, 11);
  REQUIRE_EQ (folder3->size_, 6);
  REQUIRE_EQ (midi->pos_, 12);
  REQUIRE_EQ (midi2->pos_, 13);
  REQUIRE_EQ (midi3->pos_, 14);
  REQUIRE_EQ (midi4->pos_, 15);
  REQUIRE_EQ (audio_fx3->pos_, 16);
  REQUIRE_EQ (TRACKLIST->get_num_tracks (), 17);
}

/* uninstalling does not work with carla */
#if 0
TEST_CASE ("copy track after uninstalling plugin")
{
#  ifdef HAVE_HELM
  test_helper_zrythm_init ();

  bool with_carla = false;
  test_plugin_manager_create_tracks_from_plugin (
    HELM_BUNDLE, HELM_URI, true, with_carla, 1);

  Track * helm_track = tracklist_get_last_track (
    TRACKLIST, TracklistPinOption::TRACKLIST_PIN_OPTION_BOTH, false);
  helm_track->select (true, true, false);

  /* unload bundle */
  /*test_plugin_manager_reload_lilv_world_w_path ("/tmp");*/

  for (int i = 0; i < 2; i++)
    {
      /* expect failure */
      GError * err = NULL;
      bool     ret = false;
      switch (i)
        {
        case 0:
          ret = tracklist_selections_action_perform_copy (
            TRACKLIST_SELECTIONS,

            PORT_CONNECTIONS_MGR, TRACKLIST->tracks.size (), &err);
          break;
        case 1:
          ret = tracklist_selections_action_perform_delete (
            TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, &err);
          break;
        }
      REQUIRE_FALSE (ret);
      REQUIRE_NONNULL (err);
      g_error_free_and_null (err);
    }

  test_helper_zrythm_cleanup ();
#  endif
}
#endif

TEST_CASE_FIXTURE (ZrythmFixture, "move track with clip editor region")
{
  auto midi_file =
    fs::path (TESTS_SRCDIR) / "1_empty_track_1_track_with_data.mid";
  FileDescriptor file (midi_file);
  const auto     num_tracks_before = TRACKLIST->get_num_tracks ();
  REQUIRE_NOTHROW (Track::create_with_action (
    Track::Type::Midi, nullptr, &file, &PLAYHEAD, num_tracks_before, 1, -1,
    nullptr));
  auto first_track = TRACKLIST->get_track<MidiTrack> (num_tracks_before);
  first_track->select (true, true, false);
  const auto &r = first_track->lanes_[0]->regions_[0];
  r->select (true, false, false);
  CLIP_EDITOR->set_region (r.get (), false);
  auto clip_editor_region = CLIP_EDITOR->get_region ();
  REQUIRE_NONNULL (clip_editor_region);
  perform_move_to_end ();
  clip_editor_region = CLIP_EDITOR->get_region ();
  REQUIRE_NONNULL (clip_editor_region);
  UNDO_MANAGER->undo ();
  clip_editor_region = CLIP_EDITOR->get_region ();
  REQUIRE_NONNULL (clip_editor_region);
}

TEST_SUITE_END ();