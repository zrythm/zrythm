// SPDX-FileCopyrightText: © 2020-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "actions/mixer_selections_action.h"
#include "actions/undo_manager.h"
#include "actions/undoable_action.h"
#include "audio/control_port.h"
#include "plugins/carla/carla_discovery.h"
#include "plugins/plugin_manager.h"
#include "project.h"
#include "utils/flags.h"
#include "zrythm.h"

#include <glib.h>

#include "tests/helpers/plugin_manager.h"
#include "tests/helpers/project.h"

#if defined(HAVE_HELM) \
  || defined(HAVE_NO_DELAY_LINE)
static int num_master_children = 0;

static void
_test_copy_plugins (
  const char * pl_bundle,
  const char * pl_uri,
  bool         is_instrument,
  bool         with_carla)
{
  g_usleep (100);

  /* create the plugin track */
  test_plugin_manager_create_tracks_from_plugin (
    pl_bundle, pl_uri, is_instrument, with_carla,
    1);

  num_master_children++;
  g_assert_cmpint (
    P_MASTER_TRACK->num_children, ==,
    num_master_children);
  Track * track = tracklist_get_track (
    TRACKLIST, TRACKLIST->num_tracks - 1);
  g_assert_cmpuint (
    P_MASTER_TRACK
      ->children[num_master_children - 1],
    ==, track_get_name_hash (track));
  track = tracklist_get_track (TRACKLIST, 5);
  g_assert_cmpuint (
    P_MASTER_TRACK->children[0], ==,
    track_get_name_hash (track));

  /* save and reload the project */
  test_project_save_and_reload ();

  g_assert_cmpint (
    P_MASTER_TRACK->num_children, ==,
    num_master_children);
  track = tracklist_get_track (
    TRACKLIST, TRACKLIST->num_tracks - 1);
  g_assert_cmpuint (
    P_MASTER_TRACK
      ->children[num_master_children - 1],
    ==, track_get_name_hash (track));
  track = tracklist_get_track (TRACKLIST, 5);
  g_assert_cmpuint (
    P_MASTER_TRACK->children[0], ==,
    track_get_name_hash (track));

  /* select track */
  Track * selected_track =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1];
  track_select (
    track, F_SELECT, F_EXCLUSIVE,
    F_NO_PUBLISH_EVENTS);

  bool ret =
    tracklist_selections_action_perform_copy (
      TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR,
      TRACKLIST->num_tracks, NULL);
  g_assert_true (ret);
  num_master_children++;
  g_assert_cmpint (
    P_MASTER_TRACK->num_children, ==,
    num_master_children);
  track = tracklist_get_track (
    TRACKLIST, TRACKLIST->num_tracks - 1);
  g_assert_cmpuint (
    P_MASTER_TRACK
      ->children[num_master_children - 1],
    ==, track_get_name_hash (track));
  track = tracklist_get_track (TRACKLIST, 5);
  g_assert_cmpuint (
    P_MASTER_TRACK->children[0], ==,
    track_get_name_hash (track));
  track = tracklist_get_track (TRACKLIST, 6);
  g_assert_cmpuint (
    P_MASTER_TRACK->children[1], ==,
    track_get_name_hash (track));
  Track * new_track =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1];

  /* if instrument, copy tracks, otherwise copy
   * plugins */
  if (is_instrument)
    {
#  if 0
      if (!with_carla)
        {
          g_assert_true (
            new_track->channel->instrument->lv2->ui);
        }
      ua =
        mixer_selections_action_new_create (
          PLUGIN_SLOT_INSERT,
          new_track->pos, 0, descr, 1);
      undo_manager_perform (UNDO_MANAGER, ua);

      mixer_selections_clear (
        MIXER_SELECTIONS, F_NO_PUBLISH_EVENTS);
      mixer_selections_add_slot (
        MIXER_SELECTIONS, new_track,
        PLUGIN_SLOT_INSERT, 0, F_NO_CLONE);
      ua =
        mixer_selections_action_new_copy (
          MIXER_SELECTIONS, PLUGIN_SLOT_INSERT,
          -1, 0);
      undo_manager_perform (UNDO_MANAGER, ua);
      undo_manager_undo (UNDO_MANAGER, NULL);
      undo_manager_redo (UNDO_MANAGER, NULL);
      undo_manager_undo (UNDO_MANAGER, NULL);
#  endif
    }
  else
    {
      mixer_selections_clear (
        MIXER_SELECTIONS, F_NO_PUBLISH_EVENTS);
      mixer_selections_add_slot (
        MIXER_SELECTIONS, selected_track,
        PLUGIN_SLOT_INSERT, 0, F_NO_CLONE);
      ret = mixer_selections_action_perform_copy (
        MIXER_SELECTIONS, PORT_CONNECTIONS_MGR,
        PLUGIN_SLOT_INSERT,
        track_get_name_hash (new_track), 1, NULL);
      g_assert_true (ret);
    }

  g_usleep (100);
}
#endif

static void
test_copy_plugins (void)
{
  test_helper_zrythm_init ();

#ifdef HAVE_HELM
  _test_copy_plugins (
    HELM_BUNDLE, HELM_URI, true, false);
#  ifdef HAVE_CARLA
  _test_copy_plugins (
    HELM_BUNDLE, HELM_URI, true, true);
#  endif /* HAVE_CARLA */
#endif   /* HAVE_HELM */
#ifdef HAVE_NO_DELAY_LINE
  _test_copy_plugins (
    NO_DELAY_LINE_BUNDLE, NO_DELAY_LINE_URI, false,
    false);
#  ifdef HAVE_CARLA
  _test_copy_plugins (
    NO_DELAY_LINE_BUNDLE, NO_DELAY_LINE_URI, false,
    true);
#  endif /* HAVE_CARLA */
#endif   /* HAVE_NO_DELAY_LINE */

  test_helper_zrythm_cleanup ();
}

static void
test_midi_fx_slot_deletion (void)
{
  test_helper_zrythm_init ();

  /* create MIDI track */
  track_create_empty_with_action (
    TRACK_TYPE_MIDI, NULL);

#ifdef HAVE_MIDI_CC_MAP
  /* add plugin to slot */
  int             slot = 0;
  PluginSetting * setting =
    test_plugin_manager_get_plugin_setting (
      MIDI_CC_MAP_BUNDLE, MIDI_CC_MAP_URI, false);
  int     track_pos = TRACKLIST->num_tracks - 1;
  Track * track = TRACKLIST->tracks[track_pos];
  bool ret = mixer_selections_action_perform_create (
    PLUGIN_SLOT_MIDI_FX,
    track_get_name_hash (track), slot, setting, 1,
    NULL);
  g_assert_true (ret);

  Plugin * pl = track->channel->midi_fx[slot];

  /* set the value to check if it is brought
   * back on undo */
  Port * port =
    plugin_get_port_by_symbol (pl, "ccin");
  port_set_control_value (
    port, 120.f, F_NOT_NORMALIZED, false);

  /* delete slot */
  plugin_select (pl, F_SELECT, F_EXCLUSIVE);
  ret = mixer_selections_action_perform_delete (
    MIXER_SELECTIONS, PORT_CONNECTIONS_MGR, NULL);
  g_assert_true (ret);

  /* undo and check port value is restored */
  undo_manager_undo (UNDO_MANAGER, NULL);
  pl = track->channel->midi_fx[slot];
  port = plugin_get_port_by_symbol (pl, "ccin");
  g_assert_cmpfloat_with_epsilon (
    port->control, 120.f, 0.0001f);

  undo_manager_redo (UNDO_MANAGER, NULL);
#endif

  test_helper_zrythm_cleanup ();
}

#if defined(HAVE_HELM) \
  || defined(HAVE_LSP_COMPRESSOR) \
  || defined(HAVE_CARLA_RACK) \
  || (defined(HAVE_CARLA) && defined(HAVE_NOIZEMAKER)) \
  || defined(HAVE_SHERLOCK_ATOM_INSPECTOR) \
  || (defined(HAVE_UNLIMITED_MEM) && defined(HAVE_CALF_COMPRESSOR))
static void
_test_create_plugins (
  PluginProtocol prot,
  const char *   pl_bundle,
  const char *   pl_uri,
  bool           is_instrument,
  bool           with_carla)
{
  PluginSetting * setting = NULL;

#  ifdef HAVE_SHERLOCK_ATOM_INSPECTOR
  if (string_is_equal (
        pl_uri, SHERLOCK_ATOM_INSPECTOR_URI))
    {
      /* expect messages */
      LOG->use_structured_for_console = false;
      LOG->min_log_level_for_test_console =
        G_LOG_LEVEL_WARNING;
      for (int i = 0; i < 7; i++)
        {
          g_test_expect_message (
            G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
            "*Failed from water*");
        }
    }
#  endif

  switch (prot)
    {
    case PROT_LV2:
      setting =
        test_plugin_manager_get_plugin_setting (
          pl_bundle, pl_uri, with_carla);
      g_return_if_fail (setting);
      setting = plugin_setting_clone (
        setting, F_NO_VALIDATE);
      break;
    case PROT_VST:
#  ifdef HAVE_CARLA
      {
        PluginDescriptor ** descriptors =
          z_carla_discovery_create_descriptors_from_file (
            pl_bundle, ARCH_64, PROT_VST);
        setting = plugin_setting_new_default (
          descriptors[0]);
        free (descriptors);
      }
#  endif
      break;
    default:
      break;
    }
  g_return_if_fail (setting);

  if (is_instrument)
    {
      /* create an instrument track from helm */
      track_create_with_action (
        TRACK_TYPE_INSTRUMENT, setting, NULL, NULL,
        TRACKLIST->num_tracks, 1, NULL);
    }
  else
    {
      /* create an audio fx track and add the
       * plugin */
      track_create_empty_with_action (
        TRACK_TYPE_AUDIO_BUS, NULL);
      int track_pos = TRACKLIST->num_tracks - 1;
      Track * track = TRACKLIST->tracks[track_pos];
      bool    ret =
        mixer_selections_action_perform_create (
          PLUGIN_SLOT_INSERT,
          track_get_name_hash (track), 0, setting,
          1, NULL);
      g_assert_true (ret);

      if (
        string_is_equal (
          pl_uri,
          "http://open-music-kontrollers.ch/lv2/sherlock#atom_inspector")
        && !with_carla)
        {
          Plugin * pl =
            TRACKLIST
              ->tracks[TRACKLIST->num_tracks - 1]
              ->channel->inserts[0];
          g_assert_true (pl->lv2->want_position);
        }
    }

  plugin_setting_free (setting);

  /* let the engine run */
  g_usleep (1000000);

  test_project_save_and_reload ();

  int src_track_pos = TRACKLIST->num_tracks - 1;
  Track * src_track =
    TRACKLIST->tracks[src_track_pos];

  if (is_instrument)
    {
      g_assert_true (
        src_track->channel->instrument);
    }
  else
    {
      g_assert_true (
        src_track->channel->inserts[0]);
    }

  /* duplicate the track */
  track_select (
    src_track, F_SELECT, true, F_NO_PUBLISH_EVENTS);
  g_assert_true (track_validate (src_track));
  tracklist_selections_action_perform_copy (
    TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR,
    TRACKLIST->num_tracks, NULL);

  int dest_track_pos = TRACKLIST->num_tracks - 1;
  Track * dest_track =
    TRACKLIST->tracks[dest_track_pos];

  g_assert_true (track_validate (src_track));
  g_assert_true (track_validate (dest_track));

  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_redo (UNDO_MANAGER, NULL);
  undo_manager_redo (UNDO_MANAGER, NULL);

  g_message ("letting engine run...");

  /* let the engine run */
  g_usleep (1000000);

  test_project_save_and_reload ();

#  ifdef HAVE_SHERLOCK_ATOM_INSPECTOR
  if (string_is_equal (
        pl_uri, SHERLOCK_ATOM_INSPECTOR_URI))
    {
      /* assert expected messages */
      g_test_assert_expected_messages ();
      LOG->use_structured_for_console = true;
      LOG->min_log_level_for_test_console =
        G_LOG_LEVEL_DEBUG;

      /* remove to prevent further warnings */
      undo_manager_undo (UNDO_MANAGER, NULL);
      undo_manager_undo (UNDO_MANAGER, NULL);
    }
#  endif

  g_message ("done");
}
#endif

static void
test_create_plugins (void)
{
  test_helper_zrythm_init ();

  /* only run with carla */
  for (int i = 1; i < 2; i++)
    {
      if (i == 1)
        {
#ifdef HAVE_CARLA
#  ifdef HAVE_NOIZEMAKER
          _test_create_plugins (
            PROT_VST, NOIZEMAKER_PATH, NULL, true,
            i);
#  endif
#else
          break;
#endif
        }

#ifdef HAVE_SHERLOCK_ATOM_INSPECTOR
      _test_create_plugins (
        PROT_LV2, SHERLOCK_ATOM_INSPECTOR_BUNDLE,
        SHERLOCK_ATOM_INSPECTOR_URI, false, i);
#endif
#ifdef HAVE_HELM
      _test_create_plugins (
        PROT_LV2, HELM_BUNDLE, HELM_URI, true, i);
#endif
#ifdef HAVE_LSP_COMPRESSOR
      _test_create_plugins (
        PROT_LV2, LSP_COMPRESSOR_BUNDLE,
        LSP_COMPRESSOR_URI, false, i);
#endif
#ifdef HAVE_CARLA_RACK
      _test_create_plugins (
        PROT_LV2, CARLA_RACK_BUNDLE,
        CARLA_RACK_URI, true, i);
#endif
#if defined(HAVE_UNLIMITED_MEM) \
  && defined(HAVE_CALF_COMPRESSOR)
      _test_create_plugins (
        PROT_LV2, CALF_COMPRESSOR_BUNDLE,
        CALF_COMPRESSOR_URI, true, i);
#endif
    }

  test_helper_zrythm_cleanup ();
}

#ifdef HAVE_LSP_COMPRESSOR
static void
_test_port_and_plugin_track_pos_after_move (
  const char * pl_bundle,
  const char * pl_uri,
  bool         with_carla)
{
  PluginSetting * setting =
    test_plugin_manager_get_plugin_setting (
      pl_bundle, pl_uri, with_carla);
  g_return_if_fail (setting);

  /* create an instrument track from helm */
  track_create_with_action (
    TRACK_TYPE_AUDIO_BUS, setting, NULL, NULL,
    TRACKLIST->num_tracks, 1, NULL);

  plugin_setting_free (setting);

  int src_track_pos = TRACKLIST->num_tracks - 1;
  int dest_track_pos = TRACKLIST->num_tracks;

  /* select it */
  Track * src_track =
    TRACKLIST->tracks[src_track_pos];
  track_select (
    src_track, F_SELECT, true, F_NO_PUBLISH_EVENTS);

  /* get an automation track */
  AutomationTracklist * atl =
    track_get_automation_tracklist (src_track);
  AutomationTrack * at = atl->ats[atl->num_ats - 1];
  at->created = true;
  at->visible = true;

  /* create an automation region */
  Position start_pos, end_pos;
  position_set_to_bar (&start_pos, 2);
  position_set_to_bar (&end_pos, 4);
  ZRegion * region = automation_region_new (
    &start_pos, &end_pos,
    track_get_name_hash (src_track), at->index,
    at->num_regions);
  track_add_region (
    src_track, region, at, -1, F_GEN_NAME,
    F_NO_PUBLISH_EVENTS);
  arranger_object_select (
    (ArrangerObject *) region, true, false,
    F_NO_PUBLISH_EVENTS);
  bool ret =
    arranger_selections_action_perform_create (
      TL_SELECTIONS, NULL);
  g_assert_true (ret);

  /* create some automation points */
  Port * port =
    port_find_from_identifier (&at->port_id);
  position_set_to_bar (&start_pos, 1);
  AutomationPoint * ap = automation_point_new_float (
    port->deff,
    control_port_real_val_to_normalized (
      port, port->deff),
    &start_pos);
  automation_region_add_ap (
    region, ap, F_NO_PUBLISH_EVENTS);
  arranger_object_select (
    (ArrangerObject *) ap, true, false,
    F_NO_PUBLISH_EVENTS);
  ret = arranger_selections_action_perform_create (
    AUTOMATION_SELECTIONS, NULL);
  g_assert_true (ret);

  /* duplicate it */
  g_assert_true (track_validate (src_track));
  tracklist_selections_action_perform_copy (
    TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR,
    TRACKLIST->num_tracks, NULL);

  Track * dest_track =
    TRACKLIST->tracks[dest_track_pos];

  g_assert_true (track_validate (src_track));
  g_assert_true (track_validate (dest_track));

  /* move plugin from 1st track to 2nd track and
   * undo/redo */
  mixer_selections_clear (
    MIXER_SELECTIONS, F_NO_PUBLISH_EVENTS);
  mixer_selections_add_slot (
    MIXER_SELECTIONS, src_track,
    PLUGIN_SLOT_INSERT, 0, F_NO_CLONE);
  ret = mixer_selections_action_perform_move (
    MIXER_SELECTIONS, PORT_CONNECTIONS_MGR,
    PLUGIN_SLOT_INSERT,
    track_get_name_hash (dest_track), 1, NULL);
  g_assert_true (ret);

  /* let the engine run */
  g_usleep (1000000);

  g_assert_true (track_validate (src_track));
  g_assert_true (track_validate (dest_track));

  undo_manager_undo (UNDO_MANAGER, NULL);

  g_assert_true (track_validate (src_track));
  g_assert_true (track_validate (dest_track));

  undo_manager_redo (UNDO_MANAGER, NULL);

  g_assert_true (track_validate (src_track));
  g_assert_true (track_validate (dest_track));

  undo_manager_undo (UNDO_MANAGER, NULL);

  /* move plugin from 1st slot to the 2nd slot and
   * undo/redo */
  mixer_selections_clear (
    MIXER_SELECTIONS, F_NO_PUBLISH_EVENTS);
  mixer_selections_add_slot (
    MIXER_SELECTIONS, src_track,
    PLUGIN_SLOT_INSERT, 0, F_NO_CLONE);
  ret = mixer_selections_action_perform_move (
    MIXER_SELECTIONS, PORT_CONNECTIONS_MGR,
    PLUGIN_SLOT_INSERT,
    track_get_name_hash (src_track), 1, NULL);
  g_assert_true (ret);
  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_redo (UNDO_MANAGER, NULL);

  /* let the engine run */
  g_usleep (1000000);

  /* move the plugin to a new track */
  mixer_selections_clear (
    MIXER_SELECTIONS, F_NO_PUBLISH_EVENTS);
  src_track = TRACKLIST->tracks[src_track_pos];
  mixer_selections_add_slot (
    MIXER_SELECTIONS, src_track,
    PLUGIN_SLOT_INSERT, 1, F_NO_CLONE);
  ret = mixer_selections_action_new_move (
    MIXER_SELECTIONS, PORT_CONNECTIONS_MGR,
    PLUGIN_SLOT_INSERT, 0, 0, NULL);
  g_assert_true (ret);
  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_redo (UNDO_MANAGER, NULL);

  /* let the engine run */
  g_usleep (1000000);

  /* go back to the start */
  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_undo (UNDO_MANAGER, NULL);
}
#endif

static void
test_port_and_plugin_track_pos_after_move (void)
{
  return;
  test_helper_zrythm_init ();

#ifdef HAVE_LSP_COMPRESSOR
  _test_port_and_plugin_track_pos_after_move (
    LSP_COMPRESSOR_BUNDLE, LSP_COMPRESSOR_URI,
    false);
#endif

  test_helper_zrythm_cleanup ();
}

#ifdef HAVE_CARLA
static void
test_port_and_plugin_track_pos_after_move_with_carla (
  void)
{
  return;
  test_helper_zrythm_init ();

#  ifdef HAVE_LSP_COMPRESSOR
  _test_port_and_plugin_track_pos_after_move (
    LSP_COMPRESSOR_BUNDLE, LSP_COMPRESSOR_URI,
    true);
#  endif

  test_helper_zrythm_cleanup ();
}
#endif

static void
test_move_two_plugins_one_slot_up (void)
{
  test_helper_zrythm_init ();

#ifdef HAVE_LSP_COMPRESSOR
  /* create a track with an insert */
  PluginSetting * setting =
    test_plugin_manager_get_plugin_setting (
      LSP_COMPRESSOR_BUNDLE, LSP_COMPRESSOR_URI,
      false);
  g_return_if_fail (setting);
  track_create_for_plugin_at_idx_w_action (
    TRACK_TYPE_AUDIO_BUS, setting,
    TRACKLIST->num_tracks, NULL);
  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_redo (UNDO_MANAGER, NULL);
  plugin_setting_free (setting);

  int track_pos = TRACKLIST->num_tracks - 1;

  /* select it */
  Track * track = TRACKLIST->tracks[track_pos];
  track_select (
    track, F_SELECT, true, F_NO_PUBLISH_EVENTS);

  /* save and reload the project */
  test_project_save_and_reload ();
  track = TRACKLIST->tracks[track_pos];
  g_assert_true (track_validate (track));

  /* get an automation track */
  AutomationTracklist * atl =
    track_get_automation_tracklist (track);
  AutomationTrack * at = atl->ats[atl->num_ats - 1];
  g_message (
    "automation track %s", at->port_id.label);
  at->created = true;
  at->visible = true;

  /* create an automation region */
  Position start_pos, end_pos;
  position_set_to_bar (&start_pos, 2);
  position_set_to_bar (&end_pos, 4);
  ZRegion * region = automation_region_new (
    &start_pos, &end_pos,
    track_get_name_hash (track), at->index,
    at->num_regions);
  track_add_region (
    track, region, at, -1, F_GEN_NAME,
    F_NO_PUBLISH_EVENTS);
  arranger_object_select (
    (ArrangerObject *) region, true, false,
    F_NO_PUBLISH_EVENTS);
  bool ret =
    arranger_selections_action_perform_create (
      TL_SELECTIONS, NULL);
  g_assert_true (ret);
  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_redo (UNDO_MANAGER, NULL);

  /* save and reload the project */
  test_project_save_and_reload ();
  track = TRACKLIST->tracks[track_pos];
  g_assert_true (track_validate (track));
  atl = track_get_automation_tracklist (track);
  at = atl->ats[atl->num_ats - 1];

  /* create some automation points */
  Port * port =
    port_find_from_identifier (&at->port_id);
  position_set_to_bar (&start_pos, 1);
  atl = track_get_automation_tracklist (track);
  at = atl->ats[atl->num_ats - 1];
  g_assert_cmpint (at->num_regions, >, 0);
  region = at->regions[0];
  AutomationPoint * ap = automation_point_new_float (
    port->deff,
    control_port_real_val_to_normalized (
      port, port->deff),
    &start_pos);
  automation_region_add_ap (
    region, ap, F_NO_PUBLISH_EVENTS);
  arranger_object_select (
    (ArrangerObject *) ap, true, false,
    F_NO_PUBLISH_EVENTS);
  ret = arranger_selections_action_perform_create (
    AUTOMATION_SELECTIONS, NULL);
  g_assert_true (ret);
  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_redo (UNDO_MANAGER, NULL);

  /* save and reload the project */
  test_project_save_and_reload ();
  track = TRACKLIST->tracks[track_pos];
  g_assert_true (track_validate (track));

  /* duplicate the plugin to the 2nd slot */
  mixer_selections_clear (
    MIXER_SELECTIONS, F_NO_PUBLISH_EVENTS);
  mixer_selections_add_slot (
    MIXER_SELECTIONS, track, PLUGIN_SLOT_INSERT, 0,
    F_NO_CLONE);
  ret = mixer_selections_action_perform_copy (
    MIXER_SELECTIONS, PORT_CONNECTIONS_MGR,
    PLUGIN_SLOT_INSERT,
    track_get_name_hash (track), 1, NULL);
  g_assert_true (ret);
  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_redo (UNDO_MANAGER, NULL);

  /* at this point we have a plugin at slot#0 and
   * its clone at slot#1 */

  /* remove slot #0 and undo */
  mixer_selections_clear (
    MIXER_SELECTIONS, F_NO_PUBLISH_EVENTS);
  mixer_selections_add_slot (
    MIXER_SELECTIONS, track, PLUGIN_SLOT_INSERT, 0,
    F_NO_CLONE);
  ret = mixer_selections_action_perform_delete (
    MIXER_SELECTIONS, PORT_CONNECTIONS_MGR, NULL);
  g_assert_true (ret);
  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_redo (UNDO_MANAGER, NULL);
  undo_manager_undo (UNDO_MANAGER, NULL);

  /* save and reload the project */
  test_project_save_and_reload ();
  track = TRACKLIST->tracks[track_pos];
  g_assert_true (track_validate (track));

  /* move the 2 plugins to start at slot#1 (2nd
   * slot) */
  mixer_selections_clear (
    MIXER_SELECTIONS, F_NO_PUBLISH_EVENTS);
  mixer_selections_add_slot (
    MIXER_SELECTIONS, track, PLUGIN_SLOT_INSERT, 0,
    F_NO_CLONE);
  mixer_selections_add_slot (
    MIXER_SELECTIONS, track, PLUGIN_SLOT_INSERT, 1,
    F_NO_CLONE);
  ret = mixer_selections_action_perform_move (
    MIXER_SELECTIONS, PORT_CONNECTIONS_MGR,
    PLUGIN_SLOT_INSERT,
    track_get_name_hash (track), 1, NULL);
  g_assert_true (ret);
  g_assert_true (track_validate (track));
  undo_manager_undo (UNDO_MANAGER, NULL);
  g_assert_true (track_validate (track));
  undo_manager_redo (UNDO_MANAGER, NULL);
  g_assert_true (track_validate (track));

  /* save and reload the project */
  test_project_save_and_reload ();
  track = TRACKLIST->tracks[track_pos];
  g_assert_true (track_validate (track));

  /* move the 2 plugins to start at slot 2 (3rd
   * slot) */
  mixer_selections_clear (
    MIXER_SELECTIONS, F_NO_PUBLISH_EVENTS);
  mixer_selections_add_slot (
    MIXER_SELECTIONS, track, PLUGIN_SLOT_INSERT, 1,
    F_NO_CLONE);
  mixer_selections_add_slot (
    MIXER_SELECTIONS, track, PLUGIN_SLOT_INSERT, 2,
    F_NO_CLONE);
  ret = mixer_selections_action_perform_move (
    MIXER_SELECTIONS, PORT_CONNECTIONS_MGR,
    PLUGIN_SLOT_INSERT,
    track_get_name_hash (track), 2, NULL);
  g_assert_true (ret);
  g_assert_true (track_validate (track));
  undo_manager_undo (UNDO_MANAGER, NULL);
  g_assert_true (track_validate (track));
  undo_manager_redo (UNDO_MANAGER, NULL);
  g_assert_true (track_validate (track));

  /* save and reload the project */
  test_project_save_and_reload ();
  track = TRACKLIST->tracks[track_pos];
  g_assert_true (track_validate (track));

  /* move the 2 plugins to start at slot 1 (2nd
   * slot) */
  mixer_selections_clear (
    MIXER_SELECTIONS, F_NO_PUBLISH_EVENTS);
  mixer_selections_add_slot (
    MIXER_SELECTIONS, track, PLUGIN_SLOT_INSERT, 2,
    F_NO_CLONE);
  mixer_selections_add_slot (
    MIXER_SELECTIONS, track, PLUGIN_SLOT_INSERT, 3,
    F_NO_CLONE);
  ret = mixer_selections_action_perform_move (
    MIXER_SELECTIONS, PORT_CONNECTIONS_MGR,
    PLUGIN_SLOT_INSERT,
    track_get_name_hash (track), 1, NULL);
  g_assert_true (ret);
  g_assert_true (track_validate (track));
  undo_manager_undo (UNDO_MANAGER, NULL);
  g_assert_true (track_validate (track));
  undo_manager_redo (UNDO_MANAGER, NULL);
  g_assert_true (track_validate (track));

  /* save and reload the project */
  test_project_save_and_reload ();
  track = TRACKLIST->tracks[track_pos];
  g_assert_true (track_validate (track));

  /* move the 2 plugins to start back at slot 0 (1st
   * slot) */
  mixer_selections_clear (
    MIXER_SELECTIONS, F_NO_PUBLISH_EVENTS);
  mixer_selections_add_slot (
    MIXER_SELECTIONS, track, PLUGIN_SLOT_INSERT, 2,
    F_NO_CLONE);
  mixer_selections_add_slot (
    MIXER_SELECTIONS, track, PLUGIN_SLOT_INSERT, 1,
    F_NO_CLONE);
  ret = mixer_selections_action_perform_move (
    MIXER_SELECTIONS, PORT_CONNECTIONS_MGR,
    PLUGIN_SLOT_INSERT,
    track_get_name_hash (track), 0, NULL);
  g_assert_true (ret);
  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_redo (UNDO_MANAGER, NULL);

  g_assert_true (track_validate (track));
  g_assert_true (
    IS_PLUGIN (track->channel->inserts[0]));
  g_assert_true (
    IS_PLUGIN (track->channel->inserts[1]));

  /* move 2nd plugin to 1st plugin (replacing it) */
  mixer_selections_clear (
    MIXER_SELECTIONS, F_NO_PUBLISH_EVENTS);
  mixer_selections_add_slot (
    MIXER_SELECTIONS, track, PLUGIN_SLOT_INSERT, 1,
    F_NO_CLONE);
  ret = mixer_selections_action_perform_move (
    MIXER_SELECTIONS, PORT_CONNECTIONS_MGR,
    PLUGIN_SLOT_INSERT,
    track_get_name_hash (track), 0, NULL);
  g_assert_true (ret);

  /* verify that first plugin was replaced by 2nd
   * plugin */
  g_assert_true (IS_PLUGIN_AND_NONNULL (
    track->channel->inserts[0]));
  g_assert_null (track->channel->inserts[1]);

  /* undo and verify that both plugins are back */
  undo_manager_undo (UNDO_MANAGER, NULL);
  g_assert_true (
    IS_PLUGIN (track->channel->inserts[0]));
  g_assert_true (
    IS_PLUGIN (track->channel->inserts[1]));
  undo_manager_redo (UNDO_MANAGER, NULL);
  undo_manager_undo (UNDO_MANAGER, NULL);
  g_assert_true (
    IS_PLUGIN (track->channel->inserts[0]));
  g_assert_cmpstr (
    track->channel->inserts[0]->setting->descr->uri,
    ==, LSP_COMPRESSOR_URI);
  g_assert_true (
    IS_PLUGIN (track->channel->inserts[1]));

  test_project_save_and_reload ();
  track = TRACKLIST->tracks[track_pos];

  /* TODO verify that custom connections are back */

#  ifdef HAVE_MIDI_CC_MAP
  /* add plugin to slot 0 (replacing current) */
  setting = test_plugin_manager_get_plugin_setting (
    MIDI_CC_MAP_BUNDLE, MIDI_CC_MAP_URI, false);
  ret = mixer_selections_action_perform_create (
    PLUGIN_SLOT_INSERT, track_get_name_hash (track),
    0, setting, 1, NULL);
  g_assert_true (ret);

  /* undo and verify that the original plugin is
   * back */
  undo_manager_undo (UNDO_MANAGER, NULL);
  g_assert_true (
    IS_PLUGIN (track->channel->inserts[0]));
  g_assert_cmpstr (
    track->channel->inserts[0]->setting->descr->uri,
    ==, LSP_COMPRESSOR_URI);
  g_assert_true (
    IS_PLUGIN (track->channel->inserts[1]));

  /* redo */
  undo_manager_redo (UNDO_MANAGER, NULL);
  g_assert_true (
    IS_PLUGIN (track->channel->inserts[0]));
  g_assert_cmpstr (
    track->channel->inserts[0]->setting->descr->uri,
    ==, setting->descr->uri);
  g_assert_true (
    IS_PLUGIN (track->channel->inserts[1]));

  Plugin * pl = track->channel->inserts[0];

  /* set the value to check if it is brought
   * back on undo */
  port = plugin_get_port_by_symbol (pl, "ccin");
  port_set_control_value (
    port, 120.f, F_NOT_NORMALIZED, true);

  g_assert_cmpfloat_with_epsilon (
    port->control, 120.f, 0.0001f);

  /* move 2nd plugin to 1st plugin (replacing it) */
  mixer_selections_clear (
    MIXER_SELECTIONS, F_NO_PUBLISH_EVENTS);
  mixer_selections_add_slot (
    MIXER_SELECTIONS, track, PLUGIN_SLOT_INSERT, 1,
    F_NO_CLONE);
  ret = mixer_selections_action_perform_move (
    MIXER_SELECTIONS, PORT_CONNECTIONS_MGR,
    PLUGIN_SLOT_INSERT,
    track_get_name_hash (track), 0, NULL);
  g_assert_true (ret);

  test_project_save_and_reload ();
  track = TRACKLIST->tracks[track_pos];

  g_assert_true (
    IS_PLUGIN (track->channel->inserts[0]));
  g_assert_null (track->channel->inserts[1]);

  /* undo and check plugin and port value are
   * restored */
  undo_manager_undo (UNDO_MANAGER, NULL);
  pl = track->channel->inserts[0];
  g_assert_cmpstr (
    pl->setting->descr->uri, ==,
    setting->descr->uri);
  port = plugin_get_port_by_symbol (pl, "ccin");
  g_assert_cmpfloat_with_epsilon (
    port->control, 120.f, 0.0001f);

  g_assert_true (
    IS_PLUGIN (track->channel->inserts[0]));
  g_assert_true (
    IS_PLUGIN (track->channel->inserts[1]));

  test_project_save_and_reload ();
  track = TRACKLIST->tracks[track_pos];

  undo_manager_redo (UNDO_MANAGER, NULL);
#  endif

  g_assert_true (track_validate (track));

  /* let the engine run */
  g_usleep (1000000);

  test_project_save_and_reload ();

  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_redo (UNDO_MANAGER, NULL);

  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_undo (UNDO_MANAGER, NULL);
#endif

  test_helper_zrythm_cleanup ();
}

static void
test_create_modulator (void)
{
  test_helper_zrythm_init ();

#ifdef HAVE_AMS_LFO
#  ifdef HAVE_CARLA
  /* create a track with an insert */
  PluginSetting * setting =
    test_plugin_manager_get_plugin_setting (
      AMS_LFO_BUNDLE, AMS_LFO_URI, false);
  g_return_if_fail (setting);
  bool ret = mixer_selections_action_perform_create (
    PLUGIN_SLOT_MODULATOR,
    track_get_name_hash (P_MODULATOR_TRACK),
    P_MODULATOR_TRACK->num_modulators, setting, 1,
    NULL);
  g_assert_true (ret);
  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_redo (UNDO_MANAGER, NULL);
  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_redo (UNDO_MANAGER, NULL);

  /* create another one */
  ret = mixer_selections_action_perform_create (
    PLUGIN_SLOT_MODULATOR,
    track_get_name_hash (P_MODULATOR_TRACK),
    P_MODULATOR_TRACK->num_modulators, setting, 1,
    NULL);
  g_assert_true (ret);

  /* connect a cv output from the first modulator
   * to a control of the 2nd */
  Plugin * p1 =
    P_MODULATOR_TRACK->modulators
      [P_MODULATOR_TRACK->num_modulators - 2];
  Plugin * p2 =
    P_MODULATOR_TRACK->modulators
      [P_MODULATOR_TRACK->num_modulators - 1];
  Port * cv_out = NULL;
  for (int i = 0; i < p1->num_out_ports; i++)
    {
      Port * p = p1->out_ports[i];
      if (p->id.type != TYPE_CV)
        continue;
      cv_out = p;
    }
  Port * ctrl_in =
    plugin_get_port_by_symbol (p2, "freq");
  g_return_if_fail (cv_out);
  g_return_if_fail (ctrl_in);
  PortIdentifier cv_out_id = cv_out->id;
  PortIdentifier ctrl_in_id = ctrl_in->id;

  /* connect the ports */
  ret = port_connection_action_perform_connect (
    &cv_out->id, &ctrl_in->id, NULL);
  g_assert_true (ret);
  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_redo (UNDO_MANAGER, NULL);

  MixerSelections * sel = mixer_selections_new ();
  mixer_selections_add_slot (
    sel, P_MODULATOR_TRACK, PLUGIN_SLOT_MODULATOR,
    P_MODULATOR_TRACK->num_modulators - 2,
    F_NO_CLONE);
  ret = mixer_selections_action_perform_delete (
    sel, PORT_CONNECTIONS_MGR, NULL);
  g_assert_true (ret);
  undo_manager_undo (UNDO_MANAGER, NULL);

  /* verify port connection is back */
  cv_out = port_find_from_identifier (&cv_out_id);
  ctrl_in = port_find_from_identifier (&ctrl_in_id);
  g_assert_true (ports_connected (cv_out, ctrl_in));

  undo_manager_redo (UNDO_MANAGER, NULL);
  mixer_selections_free (sel);

  plugin_setting_free (setting);
#  endif /* HAVE_CARLA */
#endif   /* HAVE_AMS_LFO */

  test_helper_zrythm_cleanup ();
}

static void
test_move_pl_after_duplicating_track (void)
{
  test_helper_zrythm_init ();

#if defined(HAVE_LSP_SIDECHAIN_COMPRESSOR) \
  && defined(HAVE_HELM)

  test_plugin_manager_create_tracks_from_plugin (
    LSP_SIDECHAIN_COMPRESSOR_BUNDLE,
    LSP_SIDECHAIN_COMPRESSOR_URI, false, false, 1);
  test_plugin_manager_create_tracks_from_plugin (
    HELM_BUNDLE, HELM_URI, true, false, 1);

  Track * ins_track =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1];
  Track * lsp_track =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 2];
  Plugin * lsp = lsp_track->channel->inserts[0];

  Port * sidechain_port = NULL;
  for (int i = 0; i < lsp->num_in_ports; i++)
    {
      Port * port = lsp->in_ports[i];
      if (port->id.flags & PORT_FLAG_SIDECHAIN)
        {
          sidechain_port = port;
          break;
        }
    }
  g_assert_nonnull (sidechain_port);

  /* create sidechain connection from instrument
   * track to lsp plugin in lsp track */
  bool ret = port_connection_action_perform_connect (
    &ins_track->channel->fader->stereo_out->l->id,
    &sidechain_port->id, NULL);
  g_assert_true (ret);

  /* duplicate instrument track */
  track_select (
    ins_track, F_SELECT, F_EXCLUSIVE,
    F_NO_PUBLISH_EVENTS);
  tracklist_selections_action_perform_copy (
    TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR,
    TRACKLIST->num_tracks, NULL);

  Track * dest_track =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1];

  /* move lsp plugin to newly created track */
  mixer_selections_clear (
    MIXER_SELECTIONS, F_NO_PUBLISH_EVENTS);
  mixer_selections_add_slot (
    MIXER_SELECTIONS, lsp_track,
    PLUGIN_SLOT_INSERT, 0, F_NO_CLONE);
  ret = mixer_selections_action_perform_move (
    MIXER_SELECTIONS, PORT_CONNECTIONS_MGR,
    PLUGIN_SLOT_INSERT,
    track_get_name_hash (dest_track), 1, NULL);
  g_assert_true (ret);

#endif

  test_helper_zrythm_cleanup ();
}

static void
test_move_plugin_from_inserts_to_midi_fx (void)
{
#ifdef HAVE_MIDI_CC_MAP
  test_helper_zrythm_init ();

  /* create a track with an insert */
  track_create_empty_with_action (
    TRACK_TYPE_MIDI, NULL);
  int     track_pos = TRACKLIST->num_tracks - 1;
  Track * track = TRACKLIST->tracks[track_pos];
  PluginSetting * setting =
    test_plugin_manager_get_plugin_setting (
      MIDI_CC_MAP_BUNDLE, MIDI_CC_MAP_URI, false);
  g_return_if_fail (setting);
  bool ret = mixer_selections_action_perform_create (
    PLUGIN_SLOT_INSERT, track_get_name_hash (track),
    0, setting, 1, NULL);
  g_assert_true (ret);
  plugin_setting_free (setting);

  /* select it */
  track_select (
    track, F_SELECT, true, F_NO_PUBLISH_EVENTS);

  /* move to midi fx */
  mixer_selections_clear (
    MIXER_SELECTIONS, F_NO_PUBLISH_EVENTS);
  mixer_selections_add_slot (
    MIXER_SELECTIONS, track, PLUGIN_SLOT_INSERT, 0,
    F_NO_CLONE);
  ret = mixer_selections_action_perform_move (
    MIXER_SELECTIONS, PORT_CONNECTIONS_MGR,
    PLUGIN_SLOT_MIDI_FX,
    track_get_name_hash (track), 0, NULL);
  g_assert_true (ret);
  g_assert_nonnull (track->channel->midi_fx[0]);
  g_assert_true (track_validate (track));
  undo_manager_undo (UNDO_MANAGER, NULL);
  g_assert_true (track_validate (track));
  undo_manager_redo (UNDO_MANAGER, NULL);
  g_assert_true (track_validate (track));
  g_assert_nonnull (track->channel->midi_fx[0]);

  /* save and reload the project */
  test_project_save_and_reload ();
  track = TRACKLIST->tracks[track_pos];
  g_assert_true (track_validate (track));

  test_helper_zrythm_cleanup ();
#endif
}

static void
test_undoing_deletion_of_multiple_inserts (void)
{
  test_helper_zrythm_init ();

#if defined(HAVE_LSP_SIDECHAIN_COMPRESSOR) \
  && defined(HAVE_HELM) \
  && defined(HAVE_NO_DELAY_LINE)

  test_plugin_manager_create_tracks_from_plugin (
    HELM_BUNDLE, HELM_URI, true, false, 1);

  Track * ins_track =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1];

  /* add 2 inserts */
  int             slot = 0;
  PluginSetting * setting =
    test_plugin_manager_get_plugin_setting (
      LSP_SIDECHAIN_COMPRESSOR_BUNDLE,
      LSP_SIDECHAIN_COMPRESSOR_URI, false);
  bool ret = mixer_selections_action_perform_create (
    PLUGIN_SLOT_INSERT,
    track_get_name_hash (ins_track), slot, setting,
    1, NULL);
  g_assert_true (ret);

  slot = 1;
  setting = test_plugin_manager_get_plugin_setting (
    NO_DELAY_LINE_BUNDLE, NO_DELAY_LINE_URI, false);
  ret = mixer_selections_action_perform_create (
    PLUGIN_SLOT_INSERT,
    track_get_name_hash (ins_track), slot, setting,
    1, NULL);
  g_assert_true (ret);

  Plugin * compressor =
    ins_track->channel->inserts[0];
  Plugin * no_delay_line =
    ins_track->channel->inserts[1];
  g_assert_true (
    IS_PLUGIN_AND_NONNULL (compressor));
  g_assert_true (
    IS_PLUGIN_AND_NONNULL (no_delay_line));
  plugin_select (compressor, F_SELECT, F_EXCLUSIVE);
  plugin_select (
    no_delay_line, F_SELECT, F_NOT_EXCLUSIVE);

  /* delete inserts */
  ret = mixer_selections_action_perform_delete (
    MIXER_SELECTIONS, PORT_CONNECTIONS_MGR, NULL);
  g_assert_true (ret);

  /* undo deletion */
  undo_manager_undo (UNDO_MANAGER, NULL);

#endif

  test_helper_zrythm_cleanup ();
}

#if defined(HAVE_HELM) \
  || defined(HAVE_CARLA_RACK) \
  || (defined(HAVE_CARLA) && defined(HAVE_NOIZEMAKER))
static void
_test_replace_instrument (
  PluginProtocol prot,
  const char *   pl_bundle,
  const char *   pl_uri,
  bool           with_carla)
{
#  ifdef HAVE_LSP_COMPRESSOR
  PluginSetting * setting = NULL;

  switch (prot)
    {
    case PROT_LV2:
      setting =
        test_plugin_manager_get_plugin_setting (
          pl_bundle, pl_uri, with_carla);
      g_return_if_fail (setting);
      setting = plugin_setting_clone (
        setting, F_NO_VALIDATE);
      break;
    case PROT_VST:
#    ifdef HAVE_CARLA
      {
        PluginDescriptor ** descriptors =
          z_carla_discovery_create_descriptors_from_file (
            pl_bundle, ARCH_64, PROT_VST);
        setting = plugin_setting_new_default (
          descriptors[0]);
        free (descriptors);
      }
#    endif
      break;
    default:
      break;
    }

  /* create an fx track from a plugin */
  test_plugin_manager_create_tracks_from_plugin (
    LSP_SIDECHAIN_COMPRESSOR_BUNDLE,
    LSP_SIDECHAIN_COMPRESSOR_URI, false, false, 1);

  /* create an instrument track */
  track_create_for_plugin_at_idx_w_action (
    TRACK_TYPE_INSTRUMENT, setting,
    TRACKLIST->num_tracks, NULL);
  int src_track_pos = TRACKLIST->num_tracks - 1;
  Track * src_track =
    TRACKLIST->tracks[src_track_pos];
  g_assert_true (track_validate (src_track));

  /* let the engine run */
  g_usleep (1000000);

  test_project_save_and_reload ();

  src_track = TRACKLIST->tracks[src_track_pos];
  g_assert_true (IS_PLUGIN_AND_NONNULL (
    src_track->channel->instrument));

  /* create a port connection */
  int num_port_connection_actions =
    UNDO_MANAGER->undo_stack
      ->num_port_connection_actions;
  int lsp_track_pos = TRACKLIST->num_tracks - 2;
  Track * lsp_track =
    TRACKLIST->tracks[lsp_track_pos];
  Plugin * lsp = lsp_track->channel->inserts[0];
  Port *   sidechain_port = NULL;
  PortIdentifier sidechain_port_id;
  memset (
    &sidechain_port_id, 0, sizeof (PortIdentifier));
  port_identifier_init (&sidechain_port_id);
  for (int i = 0; i < lsp->num_in_ports; i++)
    {
      Port * port = lsp->in_ports[i];
      if (port->id.flags & PORT_FLAG_SIDECHAIN)
        {
          sidechain_port = port;
          port_identifier_copy (
            &sidechain_port_id, &port->id);
          break;
        }
    }
  g_return_if_fail (
    IS_PORT_AND_NONNULL (sidechain_port));

  /*#if 0*/
  PortIdentifier helm_l_out_port_id;
  memset (
    &helm_l_out_port_id, 0,
    sizeof (PortIdentifier));
  port_identifier_init (&helm_l_out_port_id);
  port_identifier_copy (
    &helm_l_out_port_id,
    &src_track->channel->instrument->l_out->id);
  port_connection_action_perform_connect (
    &src_track->channel->instrument->l_out->id,
    &sidechain_port->id, NULL);
  g_assert_cmpint (sidechain_port->num_srcs, ==, 1);
  g_assert_true (string_is_equal (
    helm_l_out_port_id.sym,
    UNDO_MANAGER->undo_stack
      ->port_connection_actions
        [num_port_connection_actions]
      ->connection->src_id->sym));
  /*#endif*/

  /*test_project_save_and_reload ();*/
  src_track = TRACKLIST->tracks[src_track_pos];

  /* get an automation track */
  AutomationTracklist * atl =
    track_get_automation_tracklist (src_track);
  AutomationTrack * at = atl->ats[atl->num_ats - 1];
  g_assert_true (
    at->port_id.owner_type
    == PORT_OWNER_TYPE_PLUGIN);
  at->created = true;
  at->visible = true;

  /* create an automation region */
  Position start_pos, end_pos;
  position_set_to_bar (&start_pos, 2);
  position_set_to_bar (&end_pos, 4);
  ZRegion * region = automation_region_new (
    &start_pos, &end_pos,
    track_get_name_hash (src_track), at->index,
    at->num_regions);
  track_add_region (
    src_track, region, at, -1, F_GEN_NAME,
    F_NO_PUBLISH_EVENTS);
  arranger_object_select (
    (ArrangerObject *) region, true, false,
    F_NO_PUBLISH_EVENTS);
  arranger_selections_action_perform_create (
    TL_SELECTIONS, NULL);
  int num_regions =
    automation_tracklist_get_num_regions (atl);
  g_assert_cmpint (num_regions, ==, 1);

  /* create some automation points */
  Port * port =
    port_find_from_identifier (&at->port_id);
  position_set_to_bar (&start_pos, 1);
  AutomationPoint * ap = automation_point_new_float (
    port->deff,
    control_port_real_val_to_normalized (
      port, port->deff),
    &start_pos);
  automation_region_add_ap (
    region, ap, F_NO_PUBLISH_EVENTS);
  arranger_object_select (
    (ArrangerObject *) ap, true, false,
    F_NO_PUBLISH_EVENTS);
  arranger_selections_action_perform_create (
    AUTOMATION_SELECTIONS, NULL);
  num_regions =
    automation_tracklist_get_num_regions (atl);
  g_assert_cmpint (num_regions, ==, 1);

  int num_ats = atl->num_ats;

  /* replace the instrument with a new instance */
  mixer_selections_action_perform_create (
    PLUGIN_SLOT_INSTRUMENT,
    track_get_name_hash (src_track), -1, setting,
    1, NULL);
  g_assert_true (track_validate (src_track));

  src_track = TRACKLIST->tracks[src_track_pos];
  atl = track_get_automation_tracklist (src_track);

  /* verify automation is gone */
  num_regions =
    automation_tracklist_get_num_regions (atl);
  g_assert_cmpint (num_regions, ==, 0);

  undo_manager_undo (UNDO_MANAGER, NULL);

  /* verify automation is back */
  atl = track_get_automation_tracklist (src_track);
  num_regions =
    automation_tracklist_get_num_regions (atl);
  g_assert_cmpint (num_regions, ==, 1);

  undo_manager_redo (UNDO_MANAGER, NULL);

  /* verify automation is gone */
  num_regions =
    automation_tracklist_get_num_regions (atl);
  g_assert_cmpint (num_regions, ==, 0);

  g_assert_cmpint (num_ats, ==, atl->num_ats);
  g_assert_cmpint (sidechain_port->num_srcs, ==, 0);
  g_assert_true (string_is_equal (
    helm_l_out_port_id.sym,
    UNDO_MANAGER->undo_stack
      ->port_connection_actions
        [num_port_connection_actions]
      ->connection->src_id->sym));

  /* test undo and redo */
  g_assert_true (IS_PLUGIN_AND_NONNULL (
    src_track->channel->instrument));
  undo_manager_undo (UNDO_MANAGER, NULL);
  g_assert_true (IS_PLUGIN_AND_NONNULL (
    src_track->channel->instrument));

  /* verify automation is back */
  src_track = TRACKLIST->tracks[src_track_pos];
  atl = track_get_automation_tracklist (src_track);
  num_regions =
    automation_tracklist_get_num_regions (atl);
  g_assert_cmpint (num_regions, ==, 1);

  undo_manager_redo (UNDO_MANAGER, NULL);
  g_assert_true (IS_PLUGIN_AND_NONNULL (
    src_track->channel->instrument));

  /* let the engine run */
  g_usleep (1000000);

  /*yaml_set_log_level (CYAML_LOG_INFO);*/
  test_project_save_and_reload ();
  /*yaml_set_log_level (CYAML_LOG_WARNING);*/

  src_track = TRACKLIST->tracks[src_track_pos];
  atl = track_get_automation_tracklist (src_track);
  num_regions =
    automation_tracklist_get_num_regions (atl);
  g_assert_cmpint (num_regions, ==, 0);

  undo_manager_undo (UNDO_MANAGER, NULL);

  /* verify automation is back */
  atl = track_get_automation_tracklist (src_track);
  num_regions =
    automation_tracklist_get_num_regions (atl);
  g_assert_cmpint (num_regions, ==, 1);

  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_redo (UNDO_MANAGER, NULL);
  undo_manager_redo (UNDO_MANAGER, NULL);

  /* duplicate the track */
  src_track = TRACKLIST->tracks[src_track_pos];
  track_select (
    src_track, F_SELECT, true, F_NO_PUBLISH_EVENTS);
  g_assert_true (track_validate (src_track));
  tracklist_selections_action_perform_copy (
    TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR,
    TRACKLIST->num_tracks, NULL);

  int dest_track_pos = TRACKLIST->num_tracks - 1;
  Track * dest_track =
    TRACKLIST->tracks[dest_track_pos];

  g_assert_true (track_validate (src_track));
  g_assert_true (track_validate (dest_track));

  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_undo (UNDO_MANAGER, NULL);
  /*#if 0*/
  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_redo (UNDO_MANAGER, NULL);
  /*#endif*/
  undo_manager_redo (UNDO_MANAGER, NULL);
  undo_manager_redo (UNDO_MANAGER, NULL);
  undo_manager_redo (UNDO_MANAGER, NULL);
  undo_manager_redo (UNDO_MANAGER, NULL);
  undo_manager_redo (UNDO_MANAGER, NULL);

  g_message ("letting engine run...");

  /* let the engine run */
  g_usleep (1000000);

  test_project_save_and_reload ();

  plugin_setting_free (setting);
#  endif /* HAVE LSP_COMPRESSOR */
}
#endif

static void
test_replace_instrument (void)
{
  test_helper_zrythm_init ();

  for (int i = 0; i < 2; i++)
    {
      if (i == 1)
        {
#ifdef HAVE_CARLA
#  ifdef HAVE_NOIZEMAKER
          _test_replace_instrument (
            PROT_VST, NOIZEMAKER_PATH, NULL, i);
#  endif
#else
          break;
#endif
        }

#ifdef HAVE_HELM
      _test_replace_instrument (
        PROT_LV2, HELM_BUNDLE, HELM_URI, i);
#endif
#ifdef HAVE_CARLA_RACK
      _test_replace_instrument (
        PROT_LV2, CARLA_RACK_BUNDLE,
        CARLA_RACK_URI, i);
#endif
    }

  test_helper_zrythm_cleanup ();
}

static void
test_save_modulators (void)
{
  test_helper_zrythm_init ();

#if defined(HAVE_CARLA) && defined(HAVE_GEONKICK)
  PluginSetting * setting =
    test_plugin_manager_get_plugin_setting (
      GEONKICK_BUNDLE, GEONKICK_URI, false);
  g_return_if_fail (setting);
  bool ret = mixer_selections_action_perform_create (
    PLUGIN_SLOT_MODULATOR,
    track_get_name_hash (P_MODULATOR_TRACK),
    P_MODULATOR_TRACK->num_modulators, setting, 1,
    NULL);
  g_assert_true (ret);
  plugin_setting_free (setting);

  test_project_save_and_reload ();
#endif

  test_helper_zrythm_cleanup ();
}

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX \
  "/actions/mixer_selections_action/"

  g_test_add_func (
    TEST_PREFIX "test save modulators",
    (GTestFunc) test_save_modulators);
  g_test_add_func (
    TEST_PREFIX "test create plugins",
    (GTestFunc) test_create_plugins);
#if 0
  /* needs to know if port is sidechain, not
   * implemented in carla yet */
  g_test_add_func (
    TEST_PREFIX
    "test move pl after duplicating track",
    (GTestFunc)
    test_move_pl_after_duplicating_track);
  g_test_add_func (
    TEST_PREFIX "test replace instrument",
    (GTestFunc) test_replace_instrument);
#endif
  g_test_add_func (
    TEST_PREFIX "test move two plugins one slot up",
    (GTestFunc) test_move_two_plugins_one_slot_up);
  g_test_add_func (
    TEST_PREFIX "test copy plugins",
    (GTestFunc) test_copy_plugins);
  g_test_add_func (
    TEST_PREFIX
    "test undoing deletion of multiple inserts",
    (GTestFunc)
      test_undoing_deletion_of_multiple_inserts);
  g_test_add_func (
    TEST_PREFIX
    "test move plugin from inserts to midi fx",
    (GTestFunc)
      test_move_plugin_from_inserts_to_midi_fx);
  g_test_add_func (
    TEST_PREFIX "test create modulator",
    (GTestFunc) test_create_modulator);
  g_test_add_func (
    TEST_PREFIX "test MIDI fx slot deletion",
    (GTestFunc) test_midi_fx_slot_deletion);
  g_test_add_func (
    TEST_PREFIX
    "test port and plugin track pos after move",
    (GTestFunc)
      test_port_and_plugin_track_pos_after_move);
#ifdef HAVE_CARLA
  g_test_add_func (
    TEST_PREFIX
    "test port and plugin track pos after move with carla",
    (GTestFunc)
      test_port_and_plugin_track_pos_after_move_with_carla);
#endif

  (void) test_move_pl_after_duplicating_track;
  (void) test_replace_instrument;

  return g_test_run ();
}
