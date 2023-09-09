// SPDX-FileCopyrightText: © 2021-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/port_connection_action.h"
#include "actions/tracklist_selections.h"
#include "dsp/tracklist.h"
#include "gui/widgets/main_window.h"
#include "plugins/carla/carla_discovery.h"
#include "plugins/lv2_plugin.h"
#include "plugins/plugin_descriptor.h"
#include "plugins/plugin_manager.h"
#include "project.h"
#include "settings/plugin_settings.h"
#include "settings/settings.h"
#include "utils/error.h"
#include "utils/file.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/objects.h"
#include "utils/string.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

#define PLUGIN_SETTINGS_FILENAME "plugin-settings.yaml"

/**
 * Creates a plugin setting with the recommended
 * settings for the given plugin descriptor based
 * on the current setup.
 */
PluginSetting *
plugin_setting_new_default (const PluginDescriptor * descr)
{
  PluginSetting * existing = NULL;
  if (S_PLUGIN_SETTINGS && !ZRYTHM_TESTING)
    {
      existing = plugin_settings_find (S_PLUGIN_SETTINGS, descr);
    }

  PluginSetting * self = NULL;
  if (existing)
    {
      self = plugin_setting_clone (existing, true);
    }
  else
    {
      self = object_new (PluginSetting);
      self->schema_version = PLUGIN_SETTING_SCHEMA_VERSION;
      self->descr = plugin_descriptor_clone (descr);
      plugin_setting_validate (self, false);
    }

  return self;
}

PluginSetting *
plugin_setting_clone (const PluginSetting * src, bool validate)
{
  PluginSetting * new_setting = object_new (PluginSetting);

  *new_setting = *src;
  new_setting->schema_version = PLUGIN_SETTING_SCHEMA_VERSION;
  new_setting->descr = plugin_descriptor_clone (src->descr);
  new_setting->ui_uri = g_strdup (src->ui_uri);
  new_setting->last_instantiated_time = src->last_instantiated_time;
  new_setting->num_instantiations = src->num_instantiations;

#ifndef HAVE_CARLA
  if (new_setting->open_with_carla)
    {
      g_message ("disabling open_with_carla for %s", new_setting->descr->name);
      new_setting->open_with_carla = false;
    }
#endif

  if (validate)
    {
      plugin_setting_validate (new_setting, false);
    }

  return new_setting;
}

bool
plugin_setting_is_equal (const PluginSetting * a, const PluginSetting * b)
{
  /* TODO */
  g_return_val_if_reached (false);
}

void
plugin_setting_print (const PluginSetting * self)
{
  g_message (
    "[PluginSetting]\n"
    "descr.uri=%s, "
    "open_with_carla=%d, "
    "force_generic_ui=%d, "
    "bridge_mode=%s, "
    "ui_uri=%s, "
    "last_instantiated_time=%" G_GINT64_FORMAT
    ", "
    "num_instantiations=%d",
    self->descr->uri, self->open_with_carla, self->force_generic_ui,
    carla_bridge_mode_strings[self->bridge_mode].str, self->ui_uri,
    self->last_instantiated_time, self->num_instantiations);
}

/**
 * Makes sure the setting is valid in the current
 * run and changes any fields to make it conform.
 *
 * For example, if the setting is set to not open
 * with carla but the descriptor is for a VST plugin,
 * this will set \ref PluginSetting.open_with_carla
 * to true.
 */
void
plugin_setting_validate (PluginSetting * self, bool print_result)
{
  const PluginDescriptor * descr = self->descr;

  /* force carla */
  self->open_with_carla = true;

#ifndef HAVE_CARLA
  if (self->open_with_carla)
    {
      g_critical (
        "Requested to open with carla - carla "
        "functionality is disabled");
      self->open_with_carla = false;
      return;
    }
#endif

  if (
    descr->protocol == Z_PLUGIN_PROTOCOL_VST
    || descr->protocol == Z_PLUGIN_PROTOCOL_VST3
    || descr->protocol == Z_PLUGIN_PROTOCOL_AU
    || descr->protocol == Z_PLUGIN_PROTOCOL_SFZ
    || descr->protocol == Z_PLUGIN_PROTOCOL_SF2
    || descr->protocol == Z_PLUGIN_PROTOCOL_DSSI
    || descr->protocol == Z_PLUGIN_PROTOCOL_LADSPA
    || descr->protocol == Z_PLUGIN_PROTOCOL_JSFX
    || descr->protocol == Z_PLUGIN_PROTOCOL_CLAP)
    {
      self->open_with_carla = true;
#ifndef HAVE_CARLA
      g_critical (
        "Invalid setting requested: open non-LV2 "
        "plugin, carla not installed");
#endif
    }

#if defined(_WOE32) && defined(HAVE_CARLA)
  /* open all LV2 plugins with custom UIs using
   * carla */
  if (descr->has_custom_ui && !self->force_generic_ui)
    {
      self->open_with_carla = true;
    }
#endif

#ifdef HAVE_CARLA
  /* in wayland open all LV2 plugins with custom UIs
   * using carla */
  if (z_gtk_is_wayland () && descr->has_custom_ui)
    {
      self->open_with_carla = true;
    }
#endif

#ifdef HAVE_CARLA
  /* if no bridge mode specified, calculate the
   * bridge mode here */
  /*g_debug ("%s: recalculating bridge mode...", __func__);*/
  if (self->bridge_mode == CARLA_BRIDGE_NONE)
    {
      self->bridge_mode = descr->min_bridge_mode;
      if (self->bridge_mode == CARLA_BRIDGE_NONE)
        {
          /* bridge if plugin is not whitelisted */
          if (!plugin_descriptor_is_whitelisted (descr))
            {
              self->open_with_carla = true;
              self->bridge_mode = CARLA_BRIDGE_FULL;
              /*g_debug (*/
              /*"plugin descriptor not whitelisted - will bridge full");*/
            }
        }
      else
        {
          self->open_with_carla = true;
        }
    }
  else
    {
      self->open_with_carla = true;
      CarlaBridgeMode mode = descr->min_bridge_mode;

      if (mode == CARLA_BRIDGE_FULL)
        {
          self->bridge_mode = mode;
        }
    }

#  if 0
  /* force bridge mode */
  if (self->bridge_mode == CARLA_BRIDGE_NONE)
    {
      self->bridge_mode = CARLA_BRIDGE_FULL;
    }
#  endif

    /*g_debug ("done recalculating bridge mode");*/
#endif

  /* if no custom UI, force generic */
  /*g_debug ("checking if plugin has custom UI...");*/
  if (!descr->has_custom_ui)
    {
      /*g_debug ("plugin %s has no custom UI", descr->name);*/
      self->force_generic_ui = true;
    }
  /*g_debug ("done checking if plugin has custom UI");*/

  /* if setting cannot have a UI URI, clear it */
  if (
    descr->protocol != Z_PLUGIN_PROTOCOL_LV2 || self->open_with_carla
    || self->force_generic_ui)
    {
      /*g_debug ("cannot have UI URI");*/
      g_free_and_null (self->ui_uri);
    }
  /* otherwise validate it */
  else
    {
      LilvNode *         lv2_uri = lilv_new_uri (LILV_WORLD, descr->uri);
      const LilvPlugin * lilv_plugin =
        lilv_plugins_get_by_uri (LILV_PLUGINS, lv2_uri);
      lilv_node_free (lv2_uri);
      if (!lilv_plugin)
        {
          g_debug ("failed to load plugin <%s>", descr->uri);
          g_free_and_null (self->ui_uri);
          self->force_generic_ui = true;
        }
      /* if already have UI uri and not supported,
       * clear it */
      else if (
        self->ui_uri && !lv2_plugin_is_ui_supported (descr->uri, self->ui_uri))
        {
          g_debug ("UI URI %s is not supported", self->ui_uri);
          g_free_and_null (self->ui_uri);
        }

      /* if no UI URI, pick one */
      /*g_debug ("picking a UI URI...");*/
      if (!self->ui_uri && lilv_plugin)
        {
          char * picked_ui = NULL;
          bool   ui_picked = lv2_plugin_pick_most_preferable_ui (
            descr->uri, &picked_ui, NULL, true, false);

          /* if a suitable UI was found, set the
           * UI URI */
          if (ui_picked)
            {
              g_debug ("picked %s", self->ui_uri);
              g_return_if_fail (picked_ui);
              self->ui_uri = picked_ui;
            }
          /* otherwise force generic UI */
          else
            {
              g_debug ("no suitable UI found");
              self->force_generic_ui = true;
            }
        }
    }

  if (print_result)
    {
      g_debug ("plugin setting validated. new setting:");
      plugin_setting_print (self);
    }
}

/**
 * Finish activating a plugin setting.
 */
static void
activate_finish (
  const PluginSetting * self,
  bool                  autoroute_multiout,
  bool                  has_stereo_outputs)
{
  bool has_errors = false;

  TrackType type = track_get_type_from_plugin_descriptor (self->descr);

  /* stop the engine so it doesn't restart all the time
   * until all the actions are performed */
  EngineState state;
  engine_wait_for_pause (AUDIO_ENGINE, &state, Z_F_NO_FORCE, true);

  if (autoroute_multiout)
    {
      int num_pairs = self->descr->num_audio_outs;
      if (has_stereo_outputs)
        num_pairs = num_pairs / 2;
      int num_actions = 0;

      /* create group */
      GError * err = NULL;
      bool ret = track_create_empty_with_action (TRACK_TYPE_AUDIO_GROUP, &err);
      if (!ret)
        {
          HANDLE_ERROR (err, "%s", _ ("Failed to create track"));
          has_errors = true;
        }
      num_actions++;

      Track * group = TRACKLIST->tracks[TRACKLIST->num_tracks - 1];

      /* create the plugin track */
      err = NULL;
      ret = track_create_for_plugin_at_idx_w_action (
        type, self, TRACKLIST->num_tracks, &err);
      if (!ret)
        {
          HANDLE_ERROR (err, "%s", _ ("Failed to create track"));
          has_errors = true;
        }
      num_actions++;

      Track * pl_track = TRACKLIST->tracks[TRACKLIST->num_tracks - 1];

      Plugin * pl = pl_track->channel->instrument;

      /* move the plugin track inside the group */
      track_select (pl_track, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
      err = NULL;
      ret = tracklist_selections_action_perform_move_inside (
        TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, group->pos, &err);
      if (!ret)
        {
          HANDLE_ERROR (err, "%s", _ ("Failed to move track"));
          has_errors = true;
        }
      num_actions++;

      /* route to nowhere */
      track_select (pl_track, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
      err = NULL;
      ret = tracklist_selections_action_perform_set_direct_out (
        TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, NULL, &err);
      if (!ret)
        {
          HANDLE_ERROR (err, "%s", _ ("Failed to set direct out"));
          has_errors = true;
        }
      num_actions++;

      /* rename group */
      char name[1200];
      sprintf (name, _ ("%s Output"), self->descr->name);
      err = NULL;
      ret = tracklist_selections_action_perform_edit_rename (
        group, PORT_CONNECTIONS_MGR, name, &err);
      if (!ret)
        {
          HANDLE_ERROR (err, "%s", _ ("Failed to rename track"));
          has_errors = true;
        }
      num_actions++;

      GPtrArray * pl_audio_outs = g_ptr_array_new ();
      for (int j = 0; j < pl->num_out_ports; j++)
        {
          Port * cur_port = pl->out_ports[j];
          if (cur_port->id.type != TYPE_AUDIO)
            continue;

          g_ptr_array_add (pl_audio_outs, cur_port);
        }

      for (int i = 0; i < num_pairs; i++)
        {
          /* create the audio fx track */
          err = NULL;
          ret = track_create_empty_with_action (TRACK_TYPE_AUDIO_BUS, &err);
          if (!ret)
            {
              HANDLE_ERROR (err, "%s", _ ("Failed to create track"));
              has_errors = true;
            }
          num_actions++;

          Track * fx_track = TRACKLIST->tracks[TRACKLIST->num_tracks - 1];

          /* rename fx track */
          sprintf (name, _ ("%s %d"), self->descr->name, i + 1);
          err = NULL;
          ret = tracklist_selections_action_perform_edit_rename (
            fx_track, PORT_CONNECTIONS_MGR, name, &err);
          if (!ret)
            {
              HANDLE_ERROR (err, "%s", _ ("Failed to rename track"));
              has_errors = true;
            }
          num_actions++;

          /* move the fx track inside the group */
          track_select (fx_track, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
          err = NULL;
          ret = tracklist_selections_action_perform_move_inside (
            TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, group->pos, &err);
          if (!ret)
            {
              HANDLE_ERROR (err, "%s", _ ("Failed to move track"));
              has_errors = true;
            }
          num_actions++;

          /* move the fx track to the end */
          track_select (fx_track, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
          err = NULL;
          ret = tracklist_selections_action_perform_move (
            TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, TRACKLIST->num_tracks,
            &err);
          if (!ret)
            {
              HANDLE_ERROR (err, "%s", _ ("Failed to move track"));
              has_errors = true;
            }
          num_actions++;

          /* route to group */
          track_select (fx_track, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
          err = NULL;
          ret = tracklist_selections_action_perform_set_direct_out (
            TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, group, &err);
          if (!ret)
            {
              HANDLE_ERROR (err, "%s", _ ("Failed to set direct out"));
              has_errors = true;
            }
          num_actions++;

          int    l_index = has_stereo_outputs ? i * 2 : i;
          Port * port = g_ptr_array_index (pl_audio_outs, l_index);

          /* route left port to audio fx */
          err = NULL;
          ret = port_connection_action_perform_connect (
            &port->id, &fx_track->processor->stereo_in->l->id, &err);
          if (!ret)
            {
              HANDLE_ERROR (err, "%s", _ ("Failed to connect L port"));
              has_errors = true;
            }
          num_actions++;

          int r_index = has_stereo_outputs ? i * 2 + 1 : i;
          port = g_ptr_array_index (pl_audio_outs, r_index);

          /* route right port to audio fx */
          err = NULL;
          ret = port_connection_action_perform_connect (
            &port->id, &fx_track->processor->stereo_in->r->id, &err);
          if (!ret)
            {
              HANDLE_ERROR (err, "%s", _ ("Failed to connect R port"));
              has_errors = true;
            }
          num_actions++;
        }

      g_ptr_array_unref (pl_audio_outs);

      UndoableAction * ua = undo_manager_get_last_action (UNDO_MANAGER);
      ua->num_actions = num_actions;
    }
  else /* else if not autoroute multiout */
    {
      GError * err = NULL;
      bool     ret = track_create_for_plugin_at_idx_w_action (
        type, self, TRACKLIST->num_tracks, &err);
      if (!ret)
        {
          HANDLE_ERROR (err, "%s", _ ("Failed to create track"));
          has_errors = true;
        }
    }

  engine_resume (AUDIO_ENGINE, &state);

  if (!has_errors)
    {
      PluginSetting * setting_clone = plugin_setting_clone (self, F_NO_VALIDATE);
      plugin_setting_increment_num_instantiations (setting_clone);
      plugin_setting_free (setting_clone);
    }
}

static void
on_outputs_stereo_response (
  AdwMessageDialog * dialog,
  char *             response,
  PluginSetting *    self)
{
  if (string_is_equal (response, "close"))
    return;

  bool stereo = string_is_equal (response, "yes");
  activate_finish (self, true, stereo);
}

static void
on_contains_multiple_outputs_response (
  GtkDialog *     dialog,
  gint            response_id,
  PluginSetting * self)
{
  bool autoroute_multiout = response_id == GTK_RESPONSE_YES;

  if (autoroute_multiout)
    {
      GtkWidget * stereo_dialog = adw_message_dialog_new (
        GTK_WINDOW (MAIN_WINDOW), _ ("Stereo?"), _ ("Are the outputs stereo?"));
      gtk_window_set_modal (GTK_WINDOW (stereo_dialog), true);
      adw_message_dialog_add_responses (
        ADW_MESSAGE_DIALOG (stereo_dialog), "yes", _ ("_Yes"), "no", _ ("_No"),
        NULL);
      adw_message_dialog_set_default_response (
        ADW_MESSAGE_DIALOG (stereo_dialog), "yes");
      PluginSetting * setting_clone = plugin_setting_clone (self, false);
      g_signal_connect_data (
        stereo_dialog, "response", G_CALLBACK (on_outputs_stereo_response),
        setting_clone, plugin_setting_free_closure, 0);
      gtk_window_present (GTK_WINDOW (stereo_dialog));
    }
  else
    {
      activate_finish (self, false, false);
    }

  gtk_window_destroy (GTK_WINDOW (dialog));
}

/**
 * Creates necessary tracks at the end of the tracklist.
 *
 * This may happen asynchronously so the caller should not
 * expect the setting to be activated on return.
 */
void
plugin_setting_activate (const PluginSetting * self)
{
  TrackType type = track_get_type_from_plugin_descriptor (self->descr);

  if (self->descr->num_audio_outs > 2 && type == TRACK_TYPE_INSTRUMENT)
    {
      GtkWidget * dialog = gtk_message_dialog_new (
        GTK_WINDOW (MAIN_WINDOW),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_QUESTION,
        GTK_BUTTONS_YES_NO, "%s",
        _ ("This plugin contains multiple "
           "audio outputs. "
           "Would you like to auto-route each "
           "output to a separate FX track?"));
      PluginSetting * setting_clone = plugin_setting_clone (self, false);
      g_signal_connect_data (
        dialog, "response", G_CALLBACK (on_contains_multiple_outputs_response),
        setting_clone, plugin_setting_free_closure, 0);
      gtk_window_present (GTK_WINDOW (dialog));
    }
  else
    {
      activate_finish (self, false, false);
    }
}

/**
 * Increments the number of times this plugin has been
 * instantiated.
 *
 * @note This also serializes all plugin settings.
 */
void
plugin_setting_increment_num_instantiations (PluginSetting * self)
{
  self->last_instantiated_time = g_get_real_time ();
  self->num_instantiations++;

  plugin_settings_set (S_PLUGIN_SETTINGS, self, true);
}

/**
 * Frees the plugin setting.
 */
void
plugin_setting_free (PluginSetting * self)
{
  object_free_w_func_and_null (plugin_descriptor_free, self->descr);

  object_zero_and_free (self);
}

void
plugin_setting_free_closure (void * self, GClosure * closure)
{
  plugin_setting_free ((PluginSetting *) self);
}

static char *
get_plugin_settings_file_path (void)
{
  char * zrythm_dir = zrythm_get_dir (ZRYTHM_DIR_USER_TOP);
  g_return_val_if_fail (zrythm_dir, NULL);

  return g_build_filename (zrythm_dir, PLUGIN_SETTINGS_FILENAME, NULL);
}

void
plugin_settings_serialize_to_file (PluginSettings * self)
{
  g_message ("Serializing plugin settings...");
  GError * err = NULL;
  char *   yaml = yaml_serialize (self, &plugin_settings_schema, &err);
  if (!yaml)
    {
      HANDLE_ERROR_LITERAL (err, _ ("Failed to serialize plugin settings"));
      return;
    }
  err = NULL;
  char * path = get_plugin_settings_file_path ();
  g_return_if_fail (path && strlen (path) > 2);
  g_message ("Writing plugin settings to %s...", path);
  bool success = g_file_set_contents (path, yaml, -1, &err);
  if (!success)
    {
      HANDLE_ERROR_LITERAL (err, _ ("Failed to write plugin settings file"));
      g_free (path);
      g_free (yaml);
      return;
    }
  g_free (path);
  g_free (yaml);
}

static bool
is_yaml_our_version (const char * yaml)
{
  bool same_version = false;
  char version_str[120];
  sprintf (version_str, "schema_version: %d\n", PLUGIN_SETTINGS_SCHEMA_VERSION);
  same_version = g_str_has_prefix (yaml, version_str);
  if (!same_version)
    {
      sprintf (
        version_str, "---\nschema_version: %d\n",
        PLUGIN_SETTINGS_SCHEMA_VERSION);
      same_version = g_str_has_prefix (yaml, version_str);
    }

  return same_version;
}

/**
 * Reads the file and fills up the object.
 */
PluginSettings *
plugin_settings_new (void)
{
  GError * err = NULL;
  char *   path = get_plugin_settings_file_path ();
  if (!file_exists (path))
    {
      g_message (
        "Plugin settings file at %s does "
        "not exist",
        path);
return_new_instance:
      g_free (path);
      PluginSettings * self = object_new (PluginSettings);
      self->schema_version = PLUGIN_SETTINGS_SCHEMA_VERSION;
      return self;
    }
  char * yaml = NULL;
  g_file_get_contents (path, &yaml, NULL, &err);
  if (err != NULL)
    {
      g_critical (
        "Failed to create PluginSettings "
        "from %s",
        path);
      g_error_free (err);
      g_free (yaml);
      g_free (path);
      return NULL;
    }

  /* if not same version, purge file and return
   * a new instance */
  if (!is_yaml_our_version (yaml))
    {
      g_message (
        "Found old plugin settings file version. "
        "Purging file and creating a new one.");
      GFile * file = g_file_new_for_path (path);
      g_file_delete (file, NULL, NULL);
      g_object_unref (file);
      g_free (yaml);
      goto return_new_instance;
    }

  PluginSettings * self =
    (PluginSettings *) yaml_deserialize (yaml, &plugin_settings_schema, &err);
  if (!self)
    {
      g_warning (
        "Failed to deserialize "
        "PluginSettings from %s: \n%s",
        path, err->message);
      g_error_free (err);
      g_free (yaml);
      goto return_new_instance;
    }
  g_free (yaml);
  g_free (path);

  return self;
}

/**
 * Finds a setting for the given plugin descriptor.
 *
 * @return The found setting or NULL.
 */
PluginSetting *
plugin_settings_find (PluginSettings * self, const PluginDescriptor * descr)
{
  for (int i = 0; i < self->num_settings; i++)
    {
      PluginSetting *    cur_setting = self->settings[i];
      PluginDescriptor * cur_descr = cur_setting->descr;
      g_return_val_if_fail (
        cur_descr->schema_version >= 0 && cur_setting->schema_version >= 0,
        NULL);
      if (plugin_descriptor_is_same_plugin (cur_descr, descr))
        {
          return cur_setting;
        }
    }

  return NULL;
}

/**
 * Replaces a setting or appends a setting to the
 * cache.
 *
 * This clones the setting before adding it.
 *
 * @param serialize Whether to serialize the updated
 *   cache now.
 */
void
plugin_settings_set (
  PluginSettings * self,
  PluginSetting *  setting,
  bool             _serialize)
{
  g_message ("Saving plugin setting");

  PluginSetting * own_setting =
    plugin_settings_find (S_PLUGIN_SETTINGS, setting->descr);

  if (own_setting)
    {
      own_setting->force_generic_ui = setting->force_generic_ui;
      own_setting->open_with_carla = setting->open_with_carla;
      own_setting->bridge_mode = setting->bridge_mode;
      own_setting->last_instantiated_time = setting->last_instantiated_time;
      own_setting->num_instantiations = setting->num_instantiations;
    }
  else
    {
      self->settings[self->num_settings++] =
        plugin_setting_clone (setting, F_VALIDATE);
    }

  if (_serialize)
    {
      plugin_settings_serialize_to_file (self);
    }
}

void
plugin_settings_free (PluginSettings * self)
{
  for (int i = 0; i < self->num_settings; i++)
    {
      object_free_w_func_and_null (plugin_setting_free, self->settings[i]);
    }

  object_zero_and_free (self);
}
