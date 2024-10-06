// SPDX-FileCopyrightText: © 2020-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "common/dsp/control_port.h"
#include "common/dsp/region.h"
#include "common/utils/flags.h"
#include "gui/backend/backend/actions/mixer_selections_action.h"
#include "gui/backend/backend/actions/port_connection_action.h"
#include "gui/backend/backend/actions/undo_manager.h"
#include "gui/backend/backend/actions/undoable_action.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"

#include "tests/helpers/plugin_manager.h"

#if HAVE_CARLA
#  ifdef HAVE_AMS_LFO
static void
test_modulator_connection (
  const char * pl_bundle,
  const char * pl_uri,
  bool         is_instrument,
  bool         with_carla)
{
  auto setting =
    test_plugin_manager_get_plugin_setting (pl_bundle, pl_uri, with_carla);

  /* fix the descriptor (for some reason lilv reports it as Plugin instead of
   * Instrument if you don't do lilv_world_load_all) */
  auto &descr = setting.descr_;
  if (is_instrument)
    {
      descr.category_ = ZPluginCategory::INSTRUMENT;
    }
  descr.category_str_ =
    zrythm::plugins::PluginDescriptor::category_to_string (descr.category_);

  /* create a modulator */
  UNDO_MANAGER->perform (std::make_unique<MixerSelectionsCreateAction> (
    zrythm::plugins::PluginSlotType::Modulator, *P_MODULATOR_TRACK, 0, setting));

  const auto         &macro = P_MODULATOR_TRACK->modulator_macro_processors_[0];
  const auto         &pl = P_MODULATOR_TRACK->modulators_[0];
  CVPort *            pl_cv_port = nullptr;
  ControlPort *       pl_control_port = nullptr;
  std::vector<Port *> ports;
  pl->append_ports (ports);
  for (const auto &port : ports)
    {
      if (port->is_cv () && port->is_output ())
        {
          pl_cv_port = dynamic_cast<CVPort *> (port);
          if (pl_control_port)
            {
              break;
            }
        }
      else if (port->is_control () && port->is_input ())
        {
          pl_control_port = dynamic_cast<ControlPort *> (port);
          if (pl_cv_port)
            {
              break;
            }
        }
    }

  /* connect the plugin's CV out to the macro button */
  UNDO_MANAGER->perform (std::make_unique<PortConnectionConnectAction> (
    pl_cv_port->id_, macro->cv_in_->id_));

  /* connect the macro button to the plugin's control input */
  ASSERT_ANY_THROW ({
    UNDO_MANAGER->perform (std::make_unique<PortConnectionConnectAction> (
      macro->cv_out_->id_, pl_control_port->id_));
  });
}

static void
_test_port_connection (
  const char * pl_bundle,
  const char * pl_uri,
  bool         is_instrument,
  bool         with_carla)
{
  auto setting =
    test_plugin_manager_get_plugin_setting (pl_bundle, pl_uri, with_carla);

  /* fix the descriptor (for some reason lilv reports it as Plugin instead of
   * Instrument if you don't do lilv_world_load_all) */
  auto &descr = setting.descr_;
  if (is_instrument)
    {
      descr.category_ = ZPluginCategory::INSTRUMENT;
    }
  descr.category_str_ =
    zrythm::plugins::PluginDescriptor::category_to_string (descr.category_);

  /* create an extra track */
  auto target_track = Track::create_empty_with_action<AudioBusTrack> ();

  if (is_instrument)
    {
      /* create an instrument track from helm */
      Track::create_for_plugin_at_idx_w_action (
        Track::Type::Instrument, &setting, TRACKLIST->get_num_tracks ());
    }
  else
    {
      /* create an audio fx track and add the plugin */
      auto last_track = Track::create_empty_with_action<AudioBusTrack> ();
      UNDO_MANAGER->perform (std::make_unique<MixerSelectionsCreateAction> (
        zrythm::plugins::PluginSlotType::Insert, *last_track, 0, setting));
    }

  auto src_track = TRACKLIST->get_track (TRACKLIST->get_num_tracks () - 1);

  /* connect a plugin CV out to the track's balance */
  CVPort *            src_port1 = nullptr;
  CVPort *            src_port2 = nullptr;
  std::vector<Port *> ports;
  src_track->append_ports (ports, F_INCLUDE_PLUGINS);
  for (const auto &port : ports)
    {
      if (
        port->id_.owner_type_ == PortIdentifier::OwnerType::Plugin
        && port->is_cv () && port->is_output ())
        {
          auto cv_port = dynamic_cast<CVPort *> (port);
          if (src_port1)
            {
              src_port2 = cv_port;
              break;
            }
          else
            {
              src_port1 = cv_port;
              continue;
            }
        }
    }
  ASSERT_NONNULL (src_port1);
  ASSERT_NONNULL (src_port2);
  ASSERT_NE (src_port1->get_hash (), src_port2->get_hash ());
  ports.clear ();

  auto get_fader_stereo_balance_port = [&target_track] () -> ControlPort * {
    std::vector<Port *> pts;
    target_track->append_ports (pts, F_INCLUDE_PLUGINS);
    for (auto * port : pts)
      {
        if (
          port->id_.owner_type_ == PortIdentifier::OwnerType::Fader
          && ENUM_BITSET_TEST (
            PortIdentifier::Flags, port->id_.flags_,
            PortIdentifier::Flags::StereoBalance))
          {
            return dynamic_cast<ControlPort *> (port);
          }
      }
    return nullptr;
  };

  auto * dest_port = get_fader_stereo_balance_port ();
  ASSERT_NONNULL (dest_port);

  auto check_num_sources = [&] (const auto &port, const auto num_sources) {
    ASSERT_SIZE_EQ (port->srcs_, num_sources);
    ASSERT_EQ (
      PORT_CONNECTIONS_MGR->get_sources (nullptr, port->id_), num_sources);
  };
  auto check_num_dests = [&] (const auto &port, const auto num_dests) {
    ASSERT_SIZE_EQ (port->dests_, num_dests);
    ASSERT_EQ (PORT_CONNECTIONS_MGR->get_dests (nullptr, port->id_), num_dests);
  };

  ASSERT_NONNULL (dest_port);
  ASSERT_TRUE (src_port1->is_in_active_project ());
  ASSERT_TRUE (src_port2->is_in_active_project ());
  ASSERT_TRUE (dest_port->is_in_active_project ());
  check_num_sources (dest_port, 0);
  check_num_dests (src_port1, 0);

  UNDO_MANAGER->perform (std::make_unique<PortConnectionConnectAction> (
    src_port1->id_, dest_port->id_));

  check_num_sources (dest_port, 1);
  check_num_dests (src_port1, 1);

  UNDO_MANAGER->undo ();

  check_num_sources (dest_port, 0);
  check_num_dests (src_port1, 0);

  UNDO_MANAGER->redo ();

  check_num_sources (dest_port, 1);
  check_num_dests (src_port1, 1);

  UNDO_MANAGER->perform (std::make_unique<PortConnectionConnectAction> (
    src_port2->id_, dest_port->id_));

  check_num_sources (dest_port, 2);
  check_num_dests (src_port1, 1);
  check_num_dests (src_port2, 1);
  ASSERT_HAS_VALUE (
    PORT_CONNECTIONS_MGR->find_connection (src_port1->id_, dest_port->id_));
  ASSERT_HAS_VALUE (
    PORT_CONNECTIONS_MGR->find_connection (src_port2->id_, dest_port->id_));
  ASSERT_EQ (dest_port->srcs_[0], src_port1);
  ASSERT_EQ (dest_port, src_port1->dests_[0]);
  ASSERT_EQ (dest_port->srcs_[1], src_port2);
  ASSERT_EQ (dest_port, src_port2->dests_[0]);

  UNDO_MANAGER->undo ();
  UNDO_MANAGER->redo ();

  /* let the engine run */
  std::this_thread::sleep_for (std::chrono::seconds (1));
}
#  endif /* HAVE_AMS_LFO */
#endif   /* HAVE_CARLA */

TEST_F (ZrythmFixture, ConnectPorts)
{
#ifdef HAVE_AMS_LFO
#  if HAVE_CARLA
  _test_port_connection (AMS_LFO_BUNDLE, AMS_LFO_URI, true, false);
  test_modulator_connection (AMS_LFO_BUNDLE, AMS_LFO_URI, true, false);
#  endif /* HAVE_CARLA */
#endif   /* HAVE_AMS_LFO */
}

TEST_F (ZrythmFixture, ConnectCvToControl)
{
#if defined(HAVE_AMS_LFO)
  test_plugin_manager_create_tracks_from_plugin (
    LOWPASS_FILTER_BUNDLE, LOWPASS_FILTER_URI, false, true, 1);
  test_plugin_manager_create_tracks_from_plugin (
    AMS_LFO_BUNDLE, AMS_LFO_URI, false, true, 1);
  auto lp_filter_track =
    TRACKLIST->get_track<AudioBusTrack> (TRACKLIST->get_num_tracks () - 2);
  auto ams_lfo_track =
    TRACKLIST->get_track<AudioBusTrack> (TRACKLIST->get_num_tracks () - 1);
  const auto &lp_filter = lp_filter_track->channel_->inserts_[0];
  const auto &ams_lfo = ams_lfo_track->channel_->inserts_[0];

  const auto &cv_out_port = ams_lfo->out_ports_[3];
  const auto &freq_port = lp_filter->in_ports_[4];

  ASSERT_NO_THROW (
    UNDO_MANAGER->perform (std::make_unique<PortConnectionConnectAction> (
      cv_out_port->id_, freq_port->id_)));
#endif // HAVE_AMS_LFO
}