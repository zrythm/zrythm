// SPDX-FileCopyrightText: © 2020-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "actions/mixer_selections_action.h"
#include "actions/port_connection_action.h"
#include "actions/undo_manager.h"
#include "actions/undoable_action.h"
#include "dsp/control_port.h"
#include "dsp/region.h"
#include "project.h"
#include "utils/flags.h"
#include "zrythm.h"

#include "tests/helpers/plugin_manager.h"

TEST_SUITE_BEGIN ("actions/port connection");

#ifdef HAVE_CARLA
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
  descr.category_str_ = PluginDescriptor::category_to_string (descr.category_);

  /* create a modulator */
  UNDO_MANAGER->perform (std::make_unique<MixerSelectionsCreateAction> (
    PluginSlotType::Modulator, *P_MODULATOR_TRACK, 0, setting));

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
  REQUIRE_THROWS (
    UNDO_MANAGER->perform (std::make_unique<PortConnectionConnectAction> (
      macro->cv_out_->id_, pl_control_port->id_)));
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
  descr.category_str_ = PluginDescriptor::category_to_string (descr.category_);

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
        PluginSlotType::Insert, *last_track, 0, setting));
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
  REQUIRE_NONNULL (src_port1);
  REQUIRE_NONNULL (src_port2);
  ports.clear ();

  ControlPort * dest_port = nullptr;
  target_track->append_ports (ports, F_INCLUDE_PLUGINS);
  for (auto port : ports)
    {
      if (
        port->id_.owner_type_ == PortIdentifier::OwnerType::Fader
        && ENUM_BITSET_TEST (
          PortIdentifier::Flags, port->id_.flags_,
          PortIdentifier::Flags::StereoBalance))
        {
          dest_port = dynamic_cast<ControlPort *> (port);
          break;
        }
    }

  REQUIRE_NONNULL (dest_port);
  REQUIRE (src_port1->is_in_active_project ());
  REQUIRE (src_port2->is_in_active_project ());
  REQUIRE (dest_port->is_in_active_project ());
  REQUIRE_EMPTY (dest_port->srcs_);
  REQUIRE_EMPTY (src_port1->dests_);

  UNDO_MANAGER->perform (std::make_unique<PortConnectionConnectAction> (
    src_port1->id_, dest_port->id_));

  REQUIRE_SIZE_EQ (dest_port->srcs_, 1);
  REQUIRE_SIZE_EQ (src_port1->dests_, 1);

  UNDO_MANAGER->undo ();

  REQUIRE_EMPTY (dest_port->srcs_);
  REQUIRE_EMPTY (src_port1->dests_);

  UNDO_MANAGER->redo ();

  REQUIRE_SIZE_EQ (dest_port->srcs_, 1);
  REQUIRE_SIZE_EQ (src_port1->dests_, 1);

  UNDO_MANAGER->perform (std::make_unique<PortConnectionConnectAction> (
    src_port2->id_, dest_port->id_));

  REQUIRE_SIZE_EQ (dest_port->srcs_, 2);
  REQUIRE_SIZE_EQ (src_port1->dests_, 1);
  REQUIRE_SIZE_EQ (src_port2->dests_, 1);
  REQUIRE_NONNULL (
    PORT_CONNECTIONS_MGR->find_connection (src_port1->id_, dest_port->id_));
  REQUIRE_NONNULL (
    PORT_CONNECTIONS_MGR->find_connection (src_port2->id_, dest_port->id_));
  REQUIRE_EQ (dest_port->srcs_[0], src_port1);
  REQUIRE_EQ (dest_port, src_port1->dests_[0]);
  REQUIRE_EQ (dest_port->srcs_[1], src_port2);
  REQUIRE_EQ (dest_port, src_port2->dests_[0]);

  UNDO_MANAGER->undo ();
  UNDO_MANAGER->redo ();

  /* let the engine run */
  std::this_thread::sleep_for (std::chrono::seconds (1));
}
#  endif /* HAVE_AMS_LFO */
#endif   /* HAVE_CARLA */

TEST_CASE ("port connection")
{
  test_helper_zrythm_init ();

#ifdef HAVE_AMS_LFO
#  ifdef HAVE_CARLA
  _test_port_connection (AMS_LFO_BUNDLE, AMS_LFO_URI, true, false);
  test_modulator_connection (AMS_LFO_BUNDLE, AMS_LFO_URI, true, false);
#  endif /* HAVE_CARLA */
#endif   /* HAVE_AMS_LFO */

  test_helper_zrythm_cleanup ();
}

TEST_CASE ("CV to control connection")
{
#if defined(HAVE_AMS_LFO)
  test_helper_zrythm_init ();

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

  REQUIRE_NOTHROW (
    UNDO_MANAGER->perform (std::make_unique<PortConnectionConnectAction> (
      cv_out_port->id_, freq_port->id_)));

  test_helper_zrythm_cleanup ();
#endif // HAVE_AMS_LFO
}

TEST_SUITE_END;