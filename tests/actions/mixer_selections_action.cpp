// SPDX-FileCopyrightText: © 2020-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "common/dsp/control_port.h"
#include "common/dsp/port_identifier.h"
#include "common/plugins/carla_discovery.h"
#include "common/utils/flags.h"
#include "gui/backend/backend/actions/arranger_selections.h"
#include "gui/backend/backend/actions/mixer_selections_action.h"
#include "gui/backend/backend/actions/port_connection_action.h"
#include "gui/backend/backend/actions/tracklist_selections.h"
#include "gui/backend/backend/actions/undo_manager.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"

#include "tests/helpers/plugin_manager.h"
#include "tests/helpers/project_helper.h"

static int num_master_children = 0;

TEST_F (ZrythmFixture, DeleteMidiFxSlot)
{
  /* create MIDI track */
  Track::create_empty_with_action<MidiTrack> ();

#ifdef HAVE_MIDI_CC_MAP
  /* add plugin to slot */
  int  slot = 0;
  auto setting = test_plugin_manager_get_plugin_setting (
    MIDI_CC_MAP_BUNDLE, MIDI_CC_MAP_URI, false);
  auto track_pos = TRACKLIST->get_last_pos ();
  auto track = TRACKLIST->get_track<ChannelTrack> (track_pos);
  ASSERT_NO_THROW (
    UNDO_MANAGER->perform (std::make_unique<MixerSelectionsCreateAction> (
      PluginSlotType::MidiFx, *track, slot, setting)));

  auto pl = track->channel_->midi_fx_[slot].get ();

  /* set the value to check if it is brought
   * back on undo */
  auto port = pl->get_port_by_symbol<ControlPort> ("ccin");
  port->set_control_value (120.f, F_NOT_NORMALIZED, false);

  /* delete slot */
  pl->select (true, true);
  UNDO_MANAGER->perform (std::make_unique<MixerSelectionsDeleteAction> (
    *MIXER_SELECTIONS->gen_full_from_this (), *PORT_CONNECTIONS_MGR));

  /* undo and check port value is restored */
  UNDO_MANAGER->undo ();
  pl = track->channel_->midi_fx_[slot].get ();
  port = pl->get_port_by_symbol<ControlPort> ("ccin");
  ASSERT_NEAR (port->control_, 120.f, 0.0001f);

  UNDO_MANAGER->redo ();
#endif
}

static void
_test_copy_plugins (
  const char * pl_bundle,
  const char * pl_uri,
  bool         is_instrument,
  bool         with_carla)
{
  std::this_thread::sleep_for (std::chrono::microseconds (100));

  /* create the plugin track */
  test_plugin_manager_create_tracks_from_plugin (
    pl_bundle, pl_uri, is_instrument, with_carla, 1);

  num_master_children++;
  ASSERT_SIZE_EQ (P_MASTER_TRACK->children_, num_master_children);
  auto track = TRACKLIST->get_last_track ();
  ASSERT_EQ (
    P_MASTER_TRACK->children_[num_master_children - 1], track->get_name_hash ());
  track = TRACKLIST->get_track (5);
  ASSERT_EQ (P_MASTER_TRACK->children_[0], track->get_name_hash ());

  /* save and reload the project */
  test_project_save_and_reload ();

  ASSERT_SIZE_EQ (P_MASTER_TRACK->children_, num_master_children);
  track = TRACKLIST->get_last_track ();
  ASSERT_EQ (
    P_MASTER_TRACK->children_[num_master_children - 1], track->get_name_hash ());
  track = TRACKLIST->get_track (5);
  ASSERT_EQ (P_MASTER_TRACK->children_[0], track->get_name_hash ());

  /* select track */
  auto selected_track = TRACKLIST->get_last_track ();
  track->select (true, true, false);

  UNDO_MANAGER->perform (std::make_unique<CopyTracksAction> (
    *TRACKLIST_SELECTIONS->gen_tracklist_selections (), *PORT_CONNECTIONS_MGR,
    TRACKLIST->get_num_tracks ()));
  num_master_children++;
  ASSERT_SIZE_EQ (P_MASTER_TRACK->children_, num_master_children);
  track = TRACKLIST->get_last_track ();
  ASSERT_EQ (
    P_MASTER_TRACK->children_[num_master_children - 1], track->get_name_hash ());
  track = TRACKLIST->get_track (5);
  ASSERT_EQ (P_MASTER_TRACK->children_[0], track->get_name_hash ());
  track = TRACKLIST->get_track (6);
  ASSERT_EQ (P_MASTER_TRACK->children_[1], track->get_name_hash ());
  auto new_track = TRACKLIST->get_last_track ();

  /* if instrument, copy tracks, otherwise copy
   * plugins */
  if (is_instrument)
    {
#if 0
      if (!with_carla)
        {
          ASSERT_TRUE (
            new_track->channel->instrument->lv2->ui);
        }
      ua =
        mixer_selections_action_new_create (
          PluginSlotType::Insert,
          new_track->pos, 0, descr, 1);
      undo_manager_perform (UNDO_MANAGER, ua);

      mixer_selections_clear (
        MIXER_SELECTIONS, false);
      mixer_selections_add_slot (
        MIXER_SELECTIONS, new_track,
        PluginSlotType::Insert, 0, F_NO_CLONE);
      ua =
        mixer_selections_action_new_copy (
          MIXER_SELECTIONS, PluginSlotType::Insert,
          -1, 0);
      undo_manager_perform (UNDO_MANAGER, ua);
      UNDO_MANAGER->undo();
      UNDO_MANAGER->redo();
      UNDO_MANAGER->undo();
#endif
    }
  else
    {
      MIXER_SELECTIONS->clear (false);
      MIXER_SELECTIONS->add_slot (
        *selected_track, PluginSlotType::Insert, 0, false);
      UNDO_MANAGER->perform (std::make_unique<MixerSelectionsCopyAction> (
        *MIXER_SELECTIONS->gen_full_from_this (), *PORT_CONNECTIONS_MGR,
        PluginSlotType::Insert, new_track, 1));
    }

  std::this_thread::sleep_for (std::chrono::microseconds (100));
}

TEST_F (ZrythmFixture, CopyPlugins)
{
  _test_copy_plugins (TRIPLE_SYNTH_BUNDLE, TRIPLE_SYNTH_URI, true, false);
#if HAVE_CARLA
  _test_copy_plugins (TRIPLE_SYNTH_BUNDLE, TRIPLE_SYNTH_URI, true, true);
#endif /* HAVE_CARLA */
#ifdef HAVE_NO_DELAY_LINE
  _test_copy_plugins (NO_DELAY_LINE_BUNDLE, NO_DELAY_LINE_URI, false, false);
#  if HAVE_CARLA
  _test_copy_plugins (NO_DELAY_LINE_BUNDLE, NO_DELAY_LINE_URI, false, true);
#  endif /* HAVE_CARLA */
#endif   /* HAVE_NO_DELAY_LINE */
}

static void
_test_create_plugins (
  PluginProtocol prot,
  const char *   pl_bundle,
  const char *   pl_uri,
  bool           is_instrument,
  bool           with_carla)
{
  std::optional<PluginSetting> setting;

#ifdef HAVE_SHERLOCK_ATOM_INSPECTOR
  if (string_is_equal (pl_uri, SHERLOCK_ATOM_INSPECTOR_URI))
    {
      /* expect messages */
      for (int i = 0; i < 7; i++)
        {
          g_test_expect_message (
            G_LOG_DOMAIN, G_LOG_LEVEL_WARNING, "*Failed from water*");
        }
      z_return_if_reached ();
    }
#endif

  switch (prot)
    {
    case PluginProtocol::LV2:
    case PluginProtocol::VST:
      setting =
        test_plugin_manager_get_plugin_setting (pl_bundle, pl_uri, with_carla);
      break;
    default:
      break;
    }
  ASSERT_TRUE (setting);

  if (is_instrument)
    {
      /* create an instrument track from helm */
      Track::create_with_action (
        Track::Type::Instrument, &(*setting), nullptr, nullptr,
        TRACKLIST->get_num_tracks (), 1, -1, nullptr);
    }
  else
    {
      /* create an audio fx track and add the plugin */
      auto track = Track::create_empty_with_action<AudioBusTrack> ();
      UNDO_MANAGER->perform (std::make_unique<MixerSelectionsCreateAction> (
        PluginSlotType::Insert, *track, 0, *setting));
    }

  setting.reset ();

  /* let the engine run */
  std::this_thread::sleep_for (std::chrono::seconds (1));

  test_project_save_and_reload ();

  auto src_track_pos = TRACKLIST->get_last_pos ();
  auto src_track = TRACKLIST->get_track<ChannelTrack> (src_track_pos);

  if (is_instrument)
    {
      ASSERT_NONNULL (src_track->channel_->instrument_);
    }
  else
    {
      ASSERT_NONNULL (src_track->channel_->inserts_[0]);
    }

  /* duplicate the track */
  src_track->select (true, true, false);
  ASSERT_TRUE (src_track->validate ());
  UNDO_MANAGER->perform (std::make_unique<CopyTracksAction> (
    *TRACKLIST_SELECTIONS->gen_tracklist_selections (), *PORT_CONNECTIONS_MGR,
    TRACKLIST->get_num_tracks ()));

  auto dest_track_pos = TRACKLIST->get_last_pos ();
  auto dest_track = TRACKLIST->get_track<ChannelTrack> (dest_track_pos);

  ASSERT_TRUE (src_track->validate ());
  ASSERT_TRUE (dest_track->validate ());

  UNDO_MANAGER->undo ();
  UNDO_MANAGER->undo ();
  UNDO_MANAGER->redo ();
  UNDO_MANAGER->redo ();

  /* let the engine run */
  std::this_thread::sleep_for (std::chrono::seconds (1));

  test_project_save_and_reload ();

#ifdef HAVE_SHERLOCK_ATOM_INSPECTOR
  if (string_is_equal (pl_uri, SHERLOCK_ATOM_INSPECTOR_URI))
    {
      /* assert expected messages */
      g_test_assert_expected_messages ();
      z_return_if_reached ();

      /* remove to prevent further warnings */
      UNDO_MANAGER->undo ();
      UNDO_MANAGER->undo ();
    }
#endif

  z_info ("done");
}

TEST_F (ZrythmFixture, CreatePlugins)
{
  /* only run with carla */
  for (int i = 1; i < 2; i++)
    {
      if (i == 1)
        {
#if HAVE_CARLA
#  ifdef HAVE_NOIZEMAKER
          _test_create_plugins (
            PluginProtocol::VST, NOIZEMAKER_PATH, nullptr, true, i);
#  endif
#else
          break;
#endif
        }

#ifdef HAVE_SHERLOCK_ATOM_INSPECTOR
#  if 0
      /* need to refactor the error handling code because this is expected to fail */
      _test_create_plugins (
        PluginProtocol::LV2, SHERLOCK_ATOM_INSPECTOR_BUNDLE,
        SHERLOCK_ATOM_INSPECTOR_URI, false, i);
#  endif
#endif
      _test_create_plugins (
        PluginProtocol::LV2, TRIPLE_SYNTH_BUNDLE, TRIPLE_SYNTH_URI, true, i);
#ifdef HAVE_LSP_COMPRESSOR
      _test_create_plugins (
        PluginProtocol::LV2, LSP_COMPRESSOR_BUNDLE, LSP_COMPRESSOR_URI, false,
        i);
#endif
#if HAVE_CARLA_RACK
      _test_create_plugins (
        PluginProtocol::LV2, CARLA_RACK_BUNDLE, CARLA_RACK_URI, true, i);
#endif
#if defined(HAVE_CALF_COMPRESSOR)
      _test_create_plugins (
        PluginProtocol::LV2, CALF_COMPRESSOR_BUNDLE, CALF_COMPRESSOR_URI, true,
        i);
#endif
    }
}

#ifdef HAVE_LSP_COMPRESSOR
static void
_test_port_and_plugin_track_pos_after_move (
  const char * pl_bundle,
  const char * pl_uri,
  bool         with_carla)
{
  auto setting =
    test_plugin_manager_get_plugin_setting (pl_bundle, pl_uri, with_carla);

  /* create an instrument track from helm */
  Track::create_with_action (
    Track::Type::AudioBus, &setting, nullptr, nullptr,
    TRACKLIST->get_num_tracks (), 1, -1, nullptr);

  int src_track_pos = TRACKLIST->get_last_pos ();
  int dest_track_pos = src_track_pos + 1;

  /* select it */
  auto src_track = TRACKLIST->get_track<AutomatableTrack> (src_track_pos);
  src_track->select (true, true, false);

  /* get an automation track */
  auto       &atl = src_track->get_automation_tracklist ();
  const auto &at = atl.ats_.back ();
  at->created_ = true;
  atl.set_at_visible (*at, true);

  /* create an automation region */
  Position start_pos, end_pos;
  start_pos.set_to_bar (2);
  end_pos.set_to_bar (4);
  auto region = std::make_shared<AutomationRegion> (
    start_pos, end_pos, src_track->get_name_hash (), at->index_,
    at->regions_.size ());
  src_track->add_region (region, at.get (), -1, true, false);
  region->select (true, false, false);
  UNDO_MANAGER->perform (
    std::make_unique<CreateArrangerSelectionsAction> (*TL_SELECTIONS));

  /* create some automation points */
  auto port = Port::find_from_identifier<ControlPort> (at->port_id_);
  start_pos.set_to_bar (1);
  auto ap = std::make_shared<AutomationPoint> (
    port->deff_, port->real_val_to_normalized (port->deff_), start_pos);
  region->append_object (ap);
  ap->select (true, false, false);
  UNDO_MANAGER->perform (
    std::make_unique<CreateArrangerSelectionsAction> (*AUTOMATION_SELECTIONS));

  /* duplicate it */
  ASSERT_TRUE (src_track->validate ());
  UNDO_MANAGER->perform (std::make_unique<CopyTracksAction> (
    *TRACKLIST_SELECTIONS->gen_tracklist_selections (), *PORT_CONNECTIONS_MGR,
    TRACKLIST->get_num_tracks ()));

  auto dest_track = TRACKLIST->get_track (dest_track_pos);

  ASSERT_TRUE (src_track->validate ());
  ASSERT_TRUE (dest_track->validate ());

  /* move plugin from 1st track to 2nd track and undo/redo */
  MIXER_SELECTIONS->clear ();
  MIXER_SELECTIONS->add_slot (*src_track, PluginSlotType::Insert, 0, false);
  UNDO_MANAGER->perform (std::make_unique<MixerSelectionsMoveAction> (
    *MIXER_SELECTIONS->gen_full_from_this (), *PORT_CONNECTIONS_MGR,
    PluginSlotType::Insert, dest_track, 1));

  /* let the engine run */
  std::this_thread::sleep_for (std::chrono::seconds (1));

  ASSERT_TRUE (src_track->validate ());
  ASSERT_TRUE (dest_track->validate ());

  UNDO_MANAGER->undo ();

  ASSERT_TRUE (src_track->validate ());
  ASSERT_TRUE (dest_track->validate ());

  UNDO_MANAGER->redo ();

  ASSERT_TRUE (src_track->validate ());
  ASSERT_TRUE (dest_track->validate ());

  UNDO_MANAGER->undo ();

  /* move plugin from 1st slot to the 2nd slot and undo/redo */
  MIXER_SELECTIONS->clear ();
  MIXER_SELECTIONS->add_slot (*src_track, PluginSlotType::Insert, 0, false);
  UNDO_MANAGER->perform (std::make_unique<MixerSelectionsMoveAction> (
    *MIXER_SELECTIONS->gen_full_from_this (), *PORT_CONNECTIONS_MGR,
    PluginSlotType::Insert, src_track, 1));
  UNDO_MANAGER->undo ();
  UNDO_MANAGER->redo ();

  /* let the engine run */
  std::this_thread::sleep_for (std::chrono::seconds (1));

  /* move the plugin to a new track */
  MIXER_SELECTIONS->clear ();
  src_track = TRACKLIST->get_track<AutomatableTrack> (src_track_pos);
  MIXER_SELECTIONS->add_slot (*src_track, PluginSlotType::Insert, 1, false);
  UNDO_MANAGER->perform (std::make_unique<MixerSelectionsMoveAction> (
    *MIXER_SELECTIONS->gen_full_from_this (), *PORT_CONNECTIONS_MGR,
    PluginSlotType::Insert, nullptr, 0));
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

TEST_F (ZrythmFixture, CheckPortAndPluginTrackPositionAfterMove)
{
  return;

#ifdef HAVE_LSP_COMPRESSOR
  _test_port_and_plugin_track_pos_after_move (
    LSP_COMPRESSOR_BUNDLE, LSP_COMPRESSOR_URI, false);
#endif
}

#if HAVE_CARLA
TEST_F (ZrythmFixture, CheckPortAndPluginTrackPositionAfterMoveWithCarla)
{
  return;

#  ifdef HAVE_LSP_COMPRESSOR
  _test_port_and_plugin_track_pos_after_move (
    LSP_COMPRESSOR_BUNDLE, LSP_COMPRESSOR_URI, true);
#  endif
}
#endif

TEST_F (ZrythmFixture, MoveTwoPluginsOneSlotUp)
{
#ifdef HAVE_LSP_COMPRESSOR

  /* create a track with an insert */
  auto setting = test_plugin_manager_get_plugin_setting (
    LSP_COMPRESSOR_BUNDLE, LSP_COMPRESSOR_URI, false);
  Track::create_for_plugin_at_idx_w_action (
    Track::Type::AudioBus, &setting, TRACKLIST->get_num_tracks ());
  UNDO_MANAGER->undo ();
  UNDO_MANAGER->redo ();

  int track_pos = TRACKLIST->get_last_pos ();

  auto get_track_and_validate = [&] (bool validate = true) {
    auto t = TRACKLIST->get_track<ChannelTrack> (track_pos);
    EXPECT_NONNULL (t);
    if (validate)
      {
        EXPECT_TRUE (t->validate ());
      }
    return t;
  };

  /* select it */
  auto track = get_track_and_validate ();
  track->select (true, true, false);

  /* save and reload the project */
  test_project_save_and_reload ();
  track = get_track_and_validate ();

  {
    /* get an automation track */
    auto       &atl = track->get_automation_tracklist ();
    const auto &at = atl.ats_.back ();
    at->created_ = true;
    atl.set_at_visible (*at, true);

    /* create an automation region */
    Position start_pos, end_pos;
    start_pos.set_to_bar (2);
    end_pos.set_to_bar (4);
    auto region = std::make_shared<AutomationRegion> (
      start_pos, end_pos, track->get_name_hash (), at->index_,
      at->regions_.size ());
    track->add_region (region, at.get (), -1, true, false);
    region->select (true, false, false);
    UNDO_MANAGER->perform (
      std::make_unique<CreateArrangerSelectionsAction> (*TL_SELECTIONS));
    UNDO_MANAGER->undo ();
    UNDO_MANAGER->redo ();
  }

  /* save and reload the project */
  test_project_save_and_reload ();
  track = get_track_and_validate ();
  auto &atl = track->get_automation_tracklist ();
  ;
  const auto &at = atl.ats_.back ();

  /* create some automation points */
  auto     port = Port::find_from_identifier<ControlPort> (at->port_id_);
  Position start_pos;
  start_pos.set_to_bar (1);
  ASSERT_NONEMPTY (at->regions_);
  const auto &region = at->regions_.front ();
  auto        ap = std::make_shared<AutomationPoint> (
    port->deff_, port->real_val_to_normalized (port->deff_), start_pos);
  region->append_object (ap);
  ap->select (true, false, false);
  UNDO_MANAGER->perform (
    std::make_unique<CreateArrangerSelectionsAction> (*AUTOMATION_SELECTIONS));
  UNDO_MANAGER->undo ();
  UNDO_MANAGER->redo ();

  /* save and reload the project */
  test_project_save_and_reload ();
  track = get_track_and_validate ();

  /* duplicate the plugin to the 2nd slot */
  MIXER_SELECTIONS->clear ();
  MIXER_SELECTIONS->add_slot (*track, PluginSlotType::Insert, 0, false);
  UNDO_MANAGER->perform (std::make_unique<MixerSelectionsCopyAction> (
    *MIXER_SELECTIONS->gen_full_from_this (), *PORT_CONNECTIONS_MGR,
    PluginSlotType::Insert, track, 1));
  UNDO_MANAGER->undo ();
  UNDO_MANAGER->redo ();

  /* at this point we have a plugin at slot#0 and
   * its clone at slot#1 */

  /* remove slot #0 and undo */
  MIXER_SELECTIONS->clear ();
  MIXER_SELECTIONS->add_slot (*track, PluginSlotType::Insert, 0, false);
  UNDO_MANAGER->perform (std::make_unique<MixerSelectionsDeleteAction> (
    *MIXER_SELECTIONS->gen_full_from_this (), *PORT_CONNECTIONS_MGR));
  UNDO_MANAGER->undo ();
  UNDO_MANAGER->redo ();
  UNDO_MANAGER->undo ();

  /* save and reload the project */
  test_project_save_and_reload ();
  track = get_track_and_validate ();

  /* move the 2 plugins to start at slot#1 (2nd
   * slot) */
  MIXER_SELECTIONS->clear ();
  MIXER_SELECTIONS->add_slot (*track, PluginSlotType::Insert, 0, false);
  MIXER_SELECTIONS->add_slot (*track, PluginSlotType::Insert, 1, false);
  UNDO_MANAGER->perform (std::make_unique<MixerSelectionsMoveAction> (
    *MIXER_SELECTIONS->gen_full_from_this (), *PORT_CONNECTIONS_MGR,
    PluginSlotType::Insert, track, 1));
  ASSERT_TRUE (track->validate ());
  UNDO_MANAGER->undo ();
  ASSERT_TRUE (track->validate ());
  UNDO_MANAGER->redo ();
  ASSERT_TRUE (track->validate ());

  /* save and reload the project */
  test_project_save_and_reload ();
  track = get_track_and_validate ();

  /* move the 2 plugins to start at slot 2 (3rd
   * slot) */
  MIXER_SELECTIONS->clear ();
  MIXER_SELECTIONS->add_slot (*track, PluginSlotType::Insert, 1, false);
  MIXER_SELECTIONS->add_slot (*track, PluginSlotType::Insert, 2, false);
  UNDO_MANAGER->perform (std::make_unique<MixerSelectionsMoveAction> (
    *MIXER_SELECTIONS->gen_full_from_this (), *PORT_CONNECTIONS_MGR,
    PluginSlotType::Insert, track, 2));
  ASSERT_TRUE (track->validate ());
  UNDO_MANAGER->undo ();
  ASSERT_TRUE (track->validate ());
  UNDO_MANAGER->redo ();
  ASSERT_TRUE (track->validate ());

  /* save and reload the project */
  test_project_save_and_reload ();
  track = get_track_and_validate ();

  /* move the 2 plugins to start at slot 1 (2nd
   * slot) */
  MIXER_SELECTIONS->clear ();
  MIXER_SELECTIONS->add_slot (*track, PluginSlotType::Insert, 2, false);
  MIXER_SELECTIONS->add_slot (*track, PluginSlotType::Insert, 3, false);
  UNDO_MANAGER->perform (std::make_unique<MixerSelectionsMoveAction> (
    *MIXER_SELECTIONS->gen_full_from_this (), *PORT_CONNECTIONS_MGR,
    PluginSlotType::Insert, track, 1));
  ASSERT_TRUE (track->validate ());
  UNDO_MANAGER->undo ();
  ASSERT_TRUE (track->validate ());
  UNDO_MANAGER->redo ();
  ASSERT_TRUE (track->validate ());

  /* save and reload the project */
  test_project_save_and_reload ();
  track = get_track_and_validate ();

  /* move the 2 plugins to start back at slot 0 (1st
   * slot) */
  MIXER_SELECTIONS->clear ();
  MIXER_SELECTIONS->add_slot (*track, PluginSlotType::Insert, 2, false);
  MIXER_SELECTIONS->add_slot (*track, PluginSlotType::Insert, 1, false);
  UNDO_MANAGER->perform (std::make_unique<MixerSelectionsMoveAction> (
    *MIXER_SELECTIONS->gen_full_from_this (), *PORT_CONNECTIONS_MGR,
    PluginSlotType::Insert, track, 0));
  UNDO_MANAGER->undo ();
  UNDO_MANAGER->redo ();

  ASSERT_TRUE (track->validate ());
  ASSERT_NONNULL (track->channel_->inserts_[0]);
  ASSERT_NONNULL (track->channel_->inserts_[1]);

  /* move 2nd plugin to 1st plugin (replacing it) */
  MIXER_SELECTIONS->clear ();
  MIXER_SELECTIONS->add_slot (*track, PluginSlotType::Insert, 1, false);
  UNDO_MANAGER->perform (std::make_unique<MixerSelectionsMoveAction> (
    *MIXER_SELECTIONS->gen_full_from_this (), *PORT_CONNECTIONS_MGR,
    PluginSlotType::Insert, track, 0));

  /* verify that first plugin was replaced by 2nd
   * plugin */
  ASSERT_NONNULL (track->channel_->inserts_[0]);
  ASSERT_NONNULL (track->channel_->inserts_[1]);

  /* undo and verify that both plugins are back */
  UNDO_MANAGER->undo ();
  ASSERT_NONNULL (track->channel_->inserts_[0]);
  ASSERT_NONNULL (track->channel_->inserts_[1]);
  UNDO_MANAGER->redo ();
  UNDO_MANAGER->undo ();
  ASSERT_NONNULL (track->channel_->inserts_[0]);
  ASSERT_EQ (
    track->channel_->inserts_[0]->setting_.descr_.uri_, LSP_COMPRESSOR_URI);
  ASSERT_NONNULL (track->channel_->inserts_[1]);

  test_project_save_and_reload ();
  track = get_track_and_validate ();

  /* TODO verify that custom connections are back */

#  ifdef HAVE_MIDI_CC_MAP
  /* add plugin to slot 0 (replacing current) */
  setting = test_plugin_manager_get_plugin_setting (
    MIDI_CC_MAP_BUNDLE, MIDI_CC_MAP_URI, false);
  UNDO_MANAGER->perform (std::make_unique<MixerSelectionsCreateAction> (
    PluginSlotType::Insert, *track, 0, setting, 1));

  /* undo and verify that the original plugin is
   * back */
  UNDO_MANAGER->undo ();
  ASSERT_NONNULL (track->channel_->inserts_.at (0));
  ASSERT_EQ (
    track->channel_->inserts_[0]->setting_.descr_.uri_, LSP_COMPRESSOR_URI);
  ASSERT_NONNULL (track->channel_->inserts_.at (1));

  /* redo */
  UNDO_MANAGER->redo ();
  ASSERT_NONNULL (track->channel_->inserts_.at (0));
  ASSERT_EQ (
    track->channel_->inserts_[0]->setting_.descr_.uri_, setting.descr_.uri_);
  ASSERT_NONNULL (track->channel_->inserts_.at (1));

  auto pl = track->channel_->inserts_[0].get ();

  /* set the value to check if it is brought back on undo */
  port = pl->get_port_by_symbol<ControlPort> ("ccin");
  port->set_control_value (120.f, F_NOT_NORMALIZED, true);

  ASSERT_NEAR (port->control_, 120.f, 0.0001f);

  /* move 2nd plugin to 1st plugin (replacing it) */
  MIXER_SELECTIONS->clear ();
  MIXER_SELECTIONS->add_slot (*track, PluginSlotType::Insert, 1, false);
  UNDO_MANAGER->perform (std::make_unique<MixerSelectionsMoveAction> (
    *MIXER_SELECTIONS->gen_full_from_this (), *PORT_CONNECTIONS_MGR,
    PluginSlotType::Insert, track, 0));

  test_project_save_and_reload ();
  track = get_track_and_validate ();

  ASSERT_NONNULL (track->channel_->inserts_.at (0));
  ASSERT_NULL (track->channel_->inserts_[1]);

  /* undo and check plugin and port value are restored */
  UNDO_MANAGER->undo ();
  pl = track->channel_->inserts_[0].get ();
  ASSERT_EQ (pl->setting_.descr_.uri_, setting.descr_.uri_);
  port = pl->get_port_by_symbol<ControlPort> ("ccin");
  ASSERT_NEAR (port->control_, 120.f, 0.0001f);

  ASSERT_NONNULL (track->channel_->inserts_[0]);
  ASSERT_NONNULL (track->channel_->inserts_[1]);

  test_project_save_and_reload ();
  track = get_track_and_validate ();

  UNDO_MANAGER->redo ();
#  endif // HAVE_MIDI_CC_MAP

  ASSERT_TRUE (track->validate ());

  /* let the engine run */
  std::this_thread::sleep_for (std::chrono::seconds (1));

  test_project_save_and_reload ();

  UNDO_MANAGER->undo ();
  UNDO_MANAGER->redo ();

  UNDO_MANAGER->undo ();
  UNDO_MANAGER->undo ();
  UNDO_MANAGER->undo ();
  UNDO_MANAGER->undo ();
#endif // HAVE_LSP_COMPRESSOR
}

TEST_F (ZrythmFixture, CreateModulator)
{
#ifdef HAVE_AMS_LFO
#  if HAVE_CARLA
  /* create a track with an insert */
  auto setting =
    test_plugin_manager_get_plugin_setting (AMS_LFO_BUNDLE, AMS_LFO_URI, false);
  UNDO_MANAGER->perform (std::make_unique<MixerSelectionsCreateAction> (
    PluginSlotType::Modulator, *P_MODULATOR_TRACK,
    P_MODULATOR_TRACK->modulators_.size (), setting, 1));
  UNDO_MANAGER->undo ();
  UNDO_MANAGER->redo ();
  UNDO_MANAGER->undo ();
  UNDO_MANAGER->redo ();

  /* create another one */
  UNDO_MANAGER->perform (std::make_unique<MixerSelectionsCreateAction> (
    PluginSlotType::Modulator, *P_MODULATOR_TRACK,
    P_MODULATOR_TRACK->modulators_.size (), setting, 1));

  /* connect a cv output from the first modulator to a control of the 2nd */
  auto p1 =
    P_MODULATOR_TRACK->modulators_[P_MODULATOR_TRACK->modulators_.size () - 2]
      .get ();
  auto     p2 = P_MODULATOR_TRACK->modulators_.back ().get ();
  CVPort * cv_out = NULL;
  for (auto p : p1->out_ports_ | type_is<CVPort> ())
    {
      cv_out = p;
    }
  auto ctrl_in = p2->get_port_by_symbol<ControlPort> ("freq");
  ASSERT_NONNULL (cv_out);
  ASSERT_NONNULL (ctrl_in);
  PortIdentifier cv_out_id = cv_out->id_;
  PortIdentifier ctrl_in_id = ctrl_in->id_;

  /* connect the ports */
  UNDO_MANAGER->perform (
    std::make_unique<PortConnectionConnectAction> (cv_out->id_, ctrl_in->id_));
  UNDO_MANAGER->undo ();
  UNDO_MANAGER->redo ();

  auto sel = std::make_unique<FullMixerSelections> ();
  sel->add_slot (
    *P_MODULATOR_TRACK, PluginSlotType::Modulator,
    P_MODULATOR_TRACK->modulators_.size () - 2, false);
  UNDO_MANAGER->perform (std::make_unique<MixerSelectionsDeleteAction> (
    *sel, *PORT_CONNECTIONS_MGR));
  UNDO_MANAGER->undo ();

  /* verify port connection is back */
  cv_out = Port::find_from_identifier<CVPort> (cv_out_id);
  ctrl_in = Port::find_from_identifier<ControlPort> (ctrl_in_id);
  ASSERT_TRUE (cv_out->is_connected_to (*ctrl_in));

  UNDO_MANAGER->redo ();

#  endif /* HAVE_CARLA */
#endif   /* HAVE_AMS_LFO */
}

TEST_F (ZrythmFixture, MovePluginAfterDuplicatingTrack)
{
#if defined(HAVE_LSP_SIDECHAIN_COMPRESSOR)

  test_plugin_manager_create_tracks_from_plugin (
    LSP_SIDECHAIN_COMPRESSOR_BUNDLE, LSP_SIDECHAIN_COMPRESSOR_URI, false, false,
    1);
  test_plugin_manager_create_tracks_from_plugin (
    TRIPLE_SYNTH_BUNDLE, TRIPLE_SYNTH_URI, true, false, 1);

  auto ins_track =
    TRACKLIST->get_track<InstrumentTrack> (TRACKLIST->get_last_pos ());
  auto lsp_track =
    TRACKLIST->get_track<AudioBusTrack> (TRACKLIST->get_last_pos () - 1);
  ASSERT_NONNULL (ins_track);
  ASSERT_NONNULL (lsp_track);
  auto lsp = lsp_track->channel_->inserts_[0].get ();

  AudioPort * sidechain_port = nullptr;
  for (auto port : lsp->in_ports_ | type_is<AudioPort> ())
    {
      if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags, port->id_.flags_,
          PortIdentifier::Flags::Sidechain))
        {
          sidechain_port = port;
          break;
        }
    }
  ASSERT_NONNULL (sidechain_port);

  /* create sidechain connection from instrument track to lsp plugin in lsp
   * track */
  UNDO_MANAGER->perform (std::make_unique<PortConnectionConnectAction> (
    ins_track->channel_->fader_->stereo_out_->get_l ().id_,
    sidechain_port->id_));

  /* duplicate instrument track */
  ins_track->select (true, true, false);
  UNDO_MANAGER->perform (std::make_unique<CopyTracksAction> (
    *TRACKLIST_SELECTIONS->gen_tracklist_selections (), *PORT_CONNECTIONS_MGR,
    TRACKLIST->get_num_tracks ()));

  auto dest_track = TRACKLIST->get_last_track ();

  /* move lsp plugin to newly created track */
  MIXER_SELECTIONS->clear ();
  MIXER_SELECTIONS->add_slot (*lsp_track, PluginSlotType::Insert, 0, false);
  UNDO_MANAGER->perform (std::make_unique<MixerSelectionsMoveAction> (
    *MIXER_SELECTIONS->gen_full_from_this (), *PORT_CONNECTIONS_MGR,
    PluginSlotType::Insert, dest_track, 1));

#endif
}

TEST_F (ZrythmFixture, MovePluginFromInsertsToMidiFx)
{
#ifdef HAVE_MIDI_CC_MAP
  /* create a track with an insert */
  auto track = Track::create_empty_with_action<MidiTrack> ();
  int  track_pos = TRACKLIST->get_last_pos ();
  auto setting = test_plugin_manager_get_plugin_setting (
    MIDI_CC_MAP_BUNDLE, MIDI_CC_MAP_URI, false);
  UNDO_MANAGER->perform (std::make_unique<MixerSelectionsCreateAction> (
    PluginSlotType::Insert, *track, 0, setting, 1));

  /* select it */
  track->select (true, true, false);

  /* move to midi fx */
  MIXER_SELECTIONS->clear ();
  MIXER_SELECTIONS->add_slot (*track, PluginSlotType::Insert, 0, false);
  UNDO_MANAGER->perform (std::make_unique<MixerSelectionsMoveAction> (
    *MIXER_SELECTIONS->gen_full_from_this (), *PORT_CONNECTIONS_MGR,
    PluginSlotType::MidiFx, track, 0));
  ASSERT_NONNULL (track->channel_->midi_fx_[0]);
  ASSERT_TRUE (track->validate ());
  UNDO_MANAGER->undo ();
  ASSERT_TRUE (track->validate ());
  UNDO_MANAGER->redo ();
  ASSERT_TRUE (track->validate ());
  ASSERT_NONNULL (track->channel_->midi_fx_[0]);

  /* save and reload the project */
  test_project_save_and_reload ();
  track = TRACKLIST->get_track<MidiTrack> (track_pos);
  ASSERT_TRUE (track->validate ());
#endif
}

TEST_F (ZrythmFixture, UndoDeletionOfMultipleInserts)
{
  test_plugin_manager_create_tracks_from_plugin (
    TRIPLE_SYNTH_BUNDLE, TRIPLE_SYNTH_URI, true, false, 1);

  auto ins_track = TRACKLIST->get_last_track<InstrumentTrack> ();

  /* add 2 inserts */
  int  slot = 0;
  auto setting = test_plugin_manager_get_plugin_setting (
    COMPRESSOR_BUNDLE, COMPRESSOR_URI, false);
  UNDO_MANAGER->perform (std::make_unique<MixerSelectionsCreateAction> (
    PluginSlotType::Insert, *ins_track, slot, setting, 1));

  slot = 1;
  setting = test_plugin_manager_get_plugin_setting (
    CUBIC_DISTORTION_BUNDLE, CUBIC_DISTORTION_URI, false);
  UNDO_MANAGER->perform (std::make_unique<MixerSelectionsCreateAction> (
    PluginSlotType::Insert, *ins_track, slot, setting, 1));

  auto compressor = ins_track->channel_->inserts_[0].get ();
  auto no_delay_line = ins_track->channel_->inserts_[1].get ();
  ASSERT_NONNULL (compressor);
  ASSERT_NONNULL (no_delay_line);
  compressor->select (true, true);
  no_delay_line->select (true, false);

  /* delete inserts */
  UNDO_MANAGER->perform (std::make_unique<MixerSelectionsDeleteAction> (
    *MIXER_SELECTIONS->gen_full_from_this (), *PORT_CONNECTIONS_MGR));

  /* undo deletion */
  UNDO_MANAGER->undo ();
}

static void
_test_replace_instrument (
  PluginProtocol prot,
  const char *   pl_bundle,
  const char *   pl_uri,
  bool           with_carla)
{
#ifdef HAVE_LSP_COMPRESSOR
  using SrcTrackType = InstrumentTrack;
  using LspTrackType = AudioBusTrack;

  PluginSetting setting;

  switch (prot)
    {
    case PluginProtocol::LV2:
    case PluginProtocol::VST:
      setting =
        test_plugin_manager_get_plugin_setting (pl_bundle, pl_uri, with_carla);
      break;
    default:
      z_return_if_reached ();
      break;
    }

  /* create an fx track from a plugin */
  test_plugin_manager_create_tracks_from_plugin (
    LSP_SIDECHAIN_COMPRESSOR_BUNDLE, LSP_SIDECHAIN_COMPRESSOR_URI, false, false,
    1);

  /* create an instrument track */
  Track::create_for_plugin_at_idx_w_action (
    Track::Type::Instrument, &setting, TRACKLIST->get_num_tracks ());
  int  src_track_pos = TRACKLIST->get_last_pos ();
  auto src_track = TRACKLIST->get_track<SrcTrackType> (src_track_pos);
  ASSERT_TRUE (src_track->validate ());

  /* let the engine run */
  std::this_thread::sleep_for (std::chrono::seconds (1));

  test_project_save_and_reload ();

  src_track = TRACKLIST->get_track<SrcTrackType> (src_track_pos);
  ASSERT_NONNULL (src_track->channel_->instrument_);

  /* create a port connection */
#  if 0
  int num_port_connection_actions =
  std::accumulate(UNDO_MANAGER->undo_stack_->actions_.begin (),
  UNDO_MANAGER->undo_stack_->actions_.end (), 0,
  [](int sum, auto & action) {
    if (auto * port_connection_action =
          dynamic_cast<PortConnectionAction *> (action.get ()))
      {
        sum++;
      }
    return sum;
  });
#  endif
  auto           lsp_track_pos = TRACKLIST->get_num_tracks () - 2;
  auto           lsp_track = TRACKLIST->get_track<LspTrackType> (lsp_track_pos);
  auto           lsp = lsp_track->channel_->inserts_[0].get ();
  AudioPort *    sidechain_port = nullptr;
  PortIdentifier sidechain_port_id = PortIdentifier ();
  for (auto port : lsp->in_ports_ | type_is<AudioPort> ())
    {
      if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags, port->id_.flags_,
          PortIdentifier::Flags::Sidechain))
        {
          sidechain_port = port;
          sidechain_port_id = port->id_;
          break;
        }
    }
  ASSERT_NONNULL (sidechain_port);

  PortIdentifier helm_l_out_port_id = PortIdentifier ();
  helm_l_out_port_id = src_track->channel_->instrument_->l_out_->id_;
  UNDO_MANAGER->perform (std::make_unique<PortConnectionConnectAction> (
    src_track->channel_->instrument_->l_out_->id_, sidechain_port->id_));
  ASSERT_SIZE_EQ (sidechain_port->srcs_, 1);
  auto connection_action = dynamic_cast<PortConnectionAction *> (
    UNDO_MANAGER->undo_stack_->actions_.back ().get ());
  ASSERT_EQ (
    helm_l_out_port_id.sym_, connection_action->connection_->src_id_.sym_);

  /*test_project_save_and_reload ();*/
  src_track = TRACKLIST->get_track<InstrumentTrack> (src_track_pos);

  /* get an automation track */
  auto atl = &src_track->get_automation_tracklist ();
  auto at = atl->ats_.back ().get ();
  ASSERT_EQ (at->port_id_.owner_type_, PortIdentifier::OwnerType::Plugin);
  at->created_ = true;
  atl->set_at_visible (*at, true);

  /* create an automation region */
  Position start_pos, end_pos;
  start_pos.set_to_bar (2);
  end_pos.set_to_bar (4);
  auto region = std::make_shared<AutomationRegion> (
    start_pos, end_pos, src_track->get_name_hash (), at->index_,
    at->regions_.size ());
  src_track->add_region (region, at, -1, true, false);
  region->select (true, false, false);
  UNDO_MANAGER->perform (
    std::make_unique<CreateArrangerSelectionsAction> (*TL_SELECTIONS));
  ASSERT_EQ (atl->get_num_regions (), 1);

  /* create some automation points */
  auto port = Port::find_from_identifier<ControlPort> (at->port_id_);
  start_pos.set_to_bar (1);
  auto ap = std::make_shared<AutomationPoint> (
    port->deff_, port->real_val_to_normalized (port->deff_), start_pos);
  region->append_object (ap);
  ap->select (true, false, false);
  UNDO_MANAGER->perform (
    std::make_unique<CreateArrangerSelectionsAction> (*AUTOMATION_SELECTIONS));
  ASSERT_EQ (atl->get_num_regions (), 1);

  const int num_ats = atl->ats_.size ();

  /* replace the instrument with a new instance */
  UNDO_MANAGER->perform (std::make_unique<MixerSelectionsCreateAction> (
    PluginSlotType::Instrument, *src_track, -1, setting, 1));
  ASSERT_TRUE (src_track->validate ());

  src_track = TRACKLIST->get_track<SrcTrackType> (src_track_pos);
  atl = &src_track->get_automation_tracklist ();

  /* verify automation is gone */
  ASSERT_EQ (atl->get_num_regions (), 0);

  UNDO_MANAGER->undo ();

  /* verify automation is back */
  atl = &src_track->get_automation_tracklist ();
  z_return_if_fail (atl);
  ASSERT_EQ (atl->get_num_regions (), 1);

  UNDO_MANAGER->redo ();

  /* verify automation is gone */
  ASSERT_EQ (atl->get_num_regions (), 0);

  ASSERT_EQ (num_ats, atl->ats_.size ());
  ASSERT_EMPTY (sidechain_port->srcs_);
  ASSERT_EQ (
    helm_l_out_port_id.sym_, connection_action->connection_->src_id_.sym_);

  /* test undo and redo */
  ASSERT_NONNULL (src_track->channel_->instrument_);
  UNDO_MANAGER->undo ();
  ASSERT_NONNULL (src_track->channel_->instrument_);

  /* verify automation is back */
  src_track = TRACKLIST->get_track<SrcTrackType> (src_track_pos);
  atl = &src_track->get_automation_tracklist ();
  z_return_if_fail (atl);
  ASSERT_EQ (atl->get_num_regions (), 1);

  UNDO_MANAGER->redo ();
  ASSERT_NONNULL (src_track->channel_->instrument_);

  /* let the engine run */
  std::this_thread::sleep_for (std::chrono::seconds (1));

  /*yaml_set_log_level (CYAML_LOG_INFO);*/
  test_project_save_and_reload ();
  /*yaml_set_log_level (CYAML_LOG_WARNING);*/

  src_track = TRACKLIST->get_track<SrcTrackType> (src_track_pos);
  atl = &src_track->get_automation_tracklist ();
  z_return_if_fail (atl);
  ASSERT_EQ (atl->get_num_regions (), 0);

  UNDO_MANAGER->undo ();

  /* verify automation is back */
  atl = &src_track->get_automation_tracklist ();
  z_return_if_fail (atl);
  ASSERT_EQ (atl->get_num_regions (), 1);

  UNDO_MANAGER->undo ();
  UNDO_MANAGER->redo ();
  UNDO_MANAGER->redo ();

  /* duplicate the track */
  src_track = TRACKLIST->get_track<SrcTrackType> (src_track_pos);
  src_track->select (true, true, false);
  ASSERT_TRUE (src_track->validate ());
  UNDO_MANAGER->perform (std::make_unique<CopyTracksAction> (
    *TRACKLIST_SELECTIONS->gen_tracklist_selections (), *PORT_CONNECTIONS_MGR,
    TRACKLIST->get_num_tracks ()));

  Track * dest_track = TRACKLIST->get_last_track ();

  ASSERT_TRUE (src_track->validate ());
  ASSERT_TRUE (dest_track->validate ());

  UNDO_MANAGER->undo ();
  UNDO_MANAGER->undo ();
  UNDO_MANAGER->undo ();
  UNDO_MANAGER->undo ();
  UNDO_MANAGER->undo ();
  /*#if 0*/
  UNDO_MANAGER->undo ();
  UNDO_MANAGER->redo ();
  /*#endif*/
  UNDO_MANAGER->redo ();
  UNDO_MANAGER->redo ();
  UNDO_MANAGER->redo ();
  UNDO_MANAGER->redo ();
  UNDO_MANAGER->redo ();

  z_info ("letting engine run...");

  /* let the engine run */
  std::this_thread::sleep_for (std::chrono::seconds (1));

  test_project_save_and_reload ();

#endif /* HAVE LSP_COMPRESSOR */
}

TEST_F (ZrythmFixture, ReplaceInstrument)
{
  for (int i = 0; i < 2; i++)
    {
      if (i == 1)
        {
#if HAVE_CARLA
#  ifdef HAVE_NOIZEMAKER
          _test_replace_instrument (
            PluginProtocol::VST, NOIZEMAKER_PATH, nullptr, i);
#  endif
#else
          break;
#endif
        }

      _test_replace_instrument (
        PluginProtocol::LV2, TRIPLE_SYNTH_BUNDLE, TRIPLE_SYNTH_URI, i);
#if HAVE_CARLA_RACK
      _test_replace_instrument (
        PluginProtocol::LV2, CARLA_RACK_BUNDLE, CARLA_RACK_URI, i);
#endif
    }
}

TEST_F (ZrythmFixture, SaveModulators)
{
#if defined(HAVE_CARLA) && defined(HAVE_GEONKICK)
  PluginSetting * setting = test_plugin_manager_get_plugin_setting (
    GEONKICK_BUNDLE, GEONKICK_URI, false);
  z_return_if_fail (setting);
  bool ret = mixer_selections_action_perform_create (
    PluginSlotType::Modulator, P_MODULATOR_TRACK->get_name_hash (),
    P_MODULATOR_TRACK->num_modulators, setting, 1, nullptr);
  ASSERT_TRUE (ret);
  plugin_setting_free (setting);

  test_project_save_and_reload ();
#endif
}