// SPDX-FileCopyrightText: © 2020-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "actions/port_connection_action.h"
#include "actions/undo_manager.h"
#include "actions/undoable_action.h"
#include "dsp/audio_region.h"
#include "dsp/automation_region.h"
#include "dsp/chord_region.h"
#include "dsp/control_port.h"
#include "dsp/master_track.h"
#include "dsp/midi_note.h"
#include "dsp/region.h"
#include "plugins/plugin_manager.h"
#include "project.h"
#include "utils/flags.h"
#include "zrythm.h"

#include <glib.h>

#include "tests/helpers/plugin_manager.h"
#include "tests/helpers/project_helper.h"

#ifdef HAVE_CARLA
#  ifdef HAVE_AMS_LFO
static void
test_modulator_connection (
  const char * pl_bundle,
  const char * pl_uri,
  bool         is_instrument,
  bool         with_carla)
{
  PluginSetting * setting =
    test_plugin_manager_get_plugin_setting (pl_bundle, pl_uri, with_carla);
  g_return_if_fail (setting);

  /* fix the descriptor (for some reason lilv
   * reports it as Plugin instead of Instrument if
   * you don't do lilv_world_load_all) */
  if (is_instrument)
    {
      setting->descr->category = ZPluginCategory::Z_PLUGIN_CATEGORY_INSTRUMENT;
    }
  g_free (setting->descr->category_str);
  setting->descr->category_str =
    plugin_descriptor_category_to_string (setting->descr->category);

  /* create a modulator */
  mixer_selections_action_perform_create (
    ZPluginSlotType::Z_PLUGIN_SLOT_MODULATOR,
    track_get_name_hash (*P_MODULATOR_TRACK), 0, setting, 1, NULL);
  plugin_setting_free (setting);

  ModulatorMacroProcessor * macro = P_MODULATOR_TRACK->modulator_macros[0];
  Plugin *                  pl = P_MODULATOR_TRACK->modulators[0];
  Port *                    pl_cv_port = NULL;
  Port *                    pl_control_port = NULL;
  GPtrArray *               ports = g_ptr_array_new ();
  plugin_append_ports (pl, ports);
  for (size_t i = 0; i < ports->len; i++)
    {
      Port * port = (Port *) g_ptr_array_index (ports, i);
      if (port->id_.type_ == PortType::CV && port->id_.flow_ == PortFlow::Output)
        {
          pl_cv_port = port;
          if (pl_control_port)
            {
              break;
            }
        }
      else if (
        port->id_.type_ == PortType::Control
        && port->id_.flow_ == PortFlow::Input)
        {
          pl_control_port = port;
          if (pl_cv_port)
            {
              break;
            }
        }
    }
  object_free_w_func_and_null (g_ptr_array_unref, ports);

  /* connect the plugin's CV out to the macro
   * button */
  port_connection_action_perform_connect (
    &pl_cv_port->id_, &macro->cv_in->id_, NULL);

  /* expect messages */
  LOG->use_structured_for_console = false;
  LOG->min_log_level_for_test_console = G_LOG_LEVEL_WARNING;
  g_test_expect_message (
    G_LOG_DOMAIN, G_LOG_LEVEL_WARNING, "*ports cannot be connected*");
  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING, "*FAILED*");
  g_test_expect_message (
    G_LOG_DOMAIN, G_LOG_LEVEL_WARNING, "*action not performed*");
  /*g_test_expect_message (*/
  /*G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,*/
  /*"*failed to perform action*");*/

  /* connect the macro button to the plugin's
   * control input */
  port_connection_action_perform_connect (
    &macro->cv_out->id_, &pl_control_port->id_, NULL);

  /* let the engine run */
  g_usleep (1000000);

  /* assert expected messages */
  g_test_assert_expected_messages ();
}

static void
_test_port_connection (
  const char * pl_bundle,
  const char * pl_uri,
  bool         is_instrument,
  bool         with_carla)
{
  PluginSetting * setting =
    test_plugin_manager_get_plugin_setting (pl_bundle, pl_uri, with_carla);
  g_return_if_fail (setting);

  /* fix the descriptor (for some reason lilv
   * reports it as Plugin instead of Instrument if
   * you don't do lilv_world_load_all) */
  if (is_instrument)
    {
      setting->descr->category = ZPluginCategory::Z_PLUGIN_CATEGORY_INSTRUMENT;
    }
  g_free (setting->descr->category_str);
  setting->descr->category_str =
    plugin_descriptor_category_to_string (setting->descr->category);

  /* create an extra track */
  Track * target_track =
    track_create_empty_with_action (TrackType::TRACK_TYPE_AUDIO_BUS, NULL);

  if (is_instrument)
    {
      /* create an instrument track from helm */
      track_create_for_plugin_at_idx_w_action (
        TrackType::TRACK_TYPE_INSTRUMENT, setting, TRACKLIST->num_tracks, NULL);
    }
  else
    {
      /* create an audio fx track and add the
       * plugin */
      track_create_empty_with_action (TrackType::TRACK_TYPE_AUDIO_BUS, NULL);
      Track * last_track = tracklist_get_last_track (
        TRACKLIST, TracklistPinOption::TRACKLIST_PIN_OPTION_BOTH, false);
      mixer_selections_action_perform_create (
        ZPluginSlotType::Z_PLUGIN_SLOT_INSERT,
        track_get_name_hash (*last_track), 0, setting, 1, NULL);
    }

  plugin_setting_free (setting);

  Track * src_track = TRACKLIST->tracks[TRACKLIST->num_tracks - 1];

  /* connect a plugin CV out to the track's
   * balance */
  Port *      src_port1 = NULL;
  Port *      src_port2 = NULL;
  GPtrArray * ports = g_ptr_array_new ();
  track_append_ports (src_track, ports, F_INCLUDE_PLUGINS);
  for (size_t i = 0; i < ports->len; i++)
    {
      Port * port = (Port *) g_ptr_array_index (ports, i);
      if (
        port->id_.owner_type_ == PortIdentifier::OwnerType::PLUGIN
        && port->id_.type_ == PortType::CV
        && port->id_.flow_ == PortFlow::Output)
        {
          if (src_port1)
            {
              src_port2 = port;
              break;
            }
          else
            {
              src_port1 = port;
              continue;
            }
        }
    }
  g_return_if_fail (src_port1);
  g_return_if_fail (src_port2);
  g_ptr_array_remove_range (ports, 0, ports->len);

  Port * dest_port = NULL;
  track_append_ports (target_track, ports, F_INCLUDE_PLUGINS);
  g_assert_nonnull (ports);
  for (size_t i = 0; i < ports->len; i++)
    {
      Port * port = (Port *) g_ptr_array_index (ports, i);
      if (
        port->id_.owner_type_ == PortIdentifier::OwnerType::FADER
        && ENUM_BITSET_TEST (
          PortIdentifier::Flags, port->id_.flags_,
          PortIdentifier::Flags::STEREO_BALANCE))
        {
          dest_port = port;
          break;
        }
    }
  object_free_w_func_and_null (g_ptr_array_unref, ports);

  g_assert_nonnull (dest_port);
  g_return_if_fail (dest_port);
  g_assert_true (src_port1->is_in_active_project ());
  g_assert_true (src_port2->is_in_active_project ());
  g_assert_true (dest_port->is_in_active_project ());
  g_assert_cmpint (dest_port->srcs_.size (), ==, 0);
  g_assert_cmpint (src_port1->dests_.size (), ==, 0);

  port_connection_action_perform_connect (
    &src_port1->id_, &dest_port->id_, NULL);

  g_assert_cmpint (dest_port->srcs_.size (), ==, 1);
  g_assert_cmpint (src_port1->dests_.size (), ==, 1);

  undo_manager_undo (UNDO_MANAGER, NULL);

  g_assert_cmpint (dest_port->srcs_.size (), ==, 0);
  g_assert_cmpint (src_port1->dests_.size (), ==, 0);

  undo_manager_redo (UNDO_MANAGER, NULL);

  g_assert_cmpint (dest_port->srcs_.size (), ==, 1);
  g_assert_cmpint (src_port1->dests_.size (), ==, 1);

  port_connection_action_perform_connect (
    &src_port2->id_, &dest_port->id_, NULL);

  g_assert_cmpint (dest_port->srcs_.size (), ==, 2);
  g_assert_cmpint (src_port1->dests_.size (), ==, 1);
  g_assert_cmpint (src_port2->dests_.size (), ==, 1);
  g_assert_nonnull (port_connections_manager_find_connection (
    PORT_CONNECTIONS_MGR, &src_port1->id_, &dest_port->id_));
  g_assert_nonnull (port_connections_manager_find_connection (
    PORT_CONNECTIONS_MGR, &src_port2->id_, &dest_port->id_));
  g_assert_true (dest_port->srcs_[0] == src_port1);
  g_assert_true (dest_port == src_port1->dests_[0]);
  g_assert_true (dest_port->srcs_[1] == src_port2);
  g_assert_true (dest_port == src_port2->dests_[0]);

  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_redo (UNDO_MANAGER, NULL);

  /* let the engine run */
  g_usleep (1000000);
}
#  endif /* HAVE_AMS_LFO */
#endif   /* HAVE_CARLA */

static void
test_port_connection (void)
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

static void
test_cv_to_control_connection (void)
{
#if defined(HAVE_AMS_LFO)
  test_helper_zrythm_init ();

  test_plugin_manager_create_tracks_from_plugin (
    LOWPASS_FILTER_BUNDLE, LOWPASS_FILTER_URI, false, true, 1);
  test_plugin_manager_create_tracks_from_plugin (
    AMS_LFO_BUNDLE, AMS_LFO_URI, false, true, 1);
  Track *  lp_filter_track = TRACKLIST->tracks[TRACKLIST->num_tracks - 2];
  Track *  ams_lfo_track = TRACKLIST->tracks[TRACKLIST->num_tracks - 1];
  Plugin * lp_filter = lp_filter_track->channel->inserts[0];
  Plugin * ams_lfo = ams_lfo_track->channel->inserts[0];

  Port * cv_out_port = ams_lfo->out_ports[3];
  Port * freq_port = lp_filter->in_ports[4];
  port_connection_action_perform_connect (
    &cv_out_port->id_, &freq_port->id_, NULL);

  test_helper_zrythm_cleanup ();
#endif // HAVE_AMS_LFO
}

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/actions/port_connection/"

  g_test_add_func (
    TEST_PREFIX "test_port_connection", (GTestFunc) test_port_connection);
  g_test_add_func (
    TEST_PREFIX "test_cv_to_control_connection",
    (GTestFunc) test_cv_to_control_connection);

  return g_test_run ();
}
