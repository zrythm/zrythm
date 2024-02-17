// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "dsp/engine.h"
#include "gui/widgets/active_hardware_mb.h"
#include "gui/widgets/file_chooser_entry.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/preferences.h"
#include "plugins/plugin_gtk.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/io.h"
#include "utils/localization.h"
#include "utils/objects.h"
#include "utils/resources.h"
#include "utils/string.h"
#include "utils/ui.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include <locale.h>

G_DEFINE_TYPE (PreferencesWidget, preferences_widget, ADW_TYPE_PREFERENCES_DIALOG)

static const char * reset_to_factory_key = "reset-to-factory";

typedef struct CallbackData
{
  PreferencesWidget * preferences_widget;
  SubgroupInfo *      info;
  char *              key;
  GtkWidget *         widget;
} CallbackData;

#define KEY_IS(a, b, c) \
  (string_is_equal (group, _ (a)) && string_is_equal (subgroup, _ (b)) \
   && string_is_equal (key, c))

static void
on_simple_string_entry_changed (GtkEditable * editable, CallbackData * data)
{
  char * str = gtk_editable_get_chars (editable, 0, -1);
  g_return_if_fail (str);
  g_settings_set_string (data->info->settings, data->key, str);
  g_free (str);
}

static void
on_path_entry_changed (GtkEditable * editable, CallbackData * data)
{
  char * str = gtk_editable_get_chars (editable, 0, -1);
  g_return_if_fail (str);
  char ** split_str = g_strsplit (str, G_SEARCHPATH_SEPARATOR_S, 0);
  g_settings_set_strv (
    data->info->settings, data->key, (const char * const *) split_str);
  g_strfreev (split_str);
  g_free (str);
}

static void
on_enum_drop_down_selection_changed (
  GObject *    gobject,
  GParamSpec * pspec,
  gpointer     user_data)
{
  CallbackData * data = (CallbackData *) user_data;
  unsigned int   idx;
  if (GTK_IS_DROP_DOWN (gobject))
    {
      GtkDropDown * dropdown = GTK_DROP_DOWN (gobject);
      idx = gtk_drop_down_get_selected (dropdown);
    }
  else if (ADW_IS_COMBO_ROW (gobject))
    {
      AdwComboRow * combo_row = ADW_COMBO_ROW (gobject);
      idx = adw_combo_row_get_selected (combo_row);
    }
  else
    {
      g_return_if_reached ();
    }
  g_settings_set_enum (data->info->settings, data->key, (gint) idx);
}

static void
on_string_drop_down_selection_changed (
  GObject *    gobject,
  GParamSpec * pspec,
  gpointer     user_data)
{
  CallbackData *    data = (CallbackData *) user_data;
  GtkStringObject * str_obj = NULL;
  if (GTK_IS_DROP_DOWN (gobject))
    {
      GtkDropDown * dropdown = GTK_DROP_DOWN (gobject);
      str_obj = GTK_STRING_OBJECT (gtk_drop_down_get_selected_item (dropdown));
    }
  else if (ADW_IS_COMBO_ROW (gobject))
    {
      AdwComboRow * combo_row = ADW_COMBO_ROW (gobject);
      str_obj = GTK_STRING_OBJECT (adw_combo_row_get_selected_item (combo_row));
    }
  else
    {
      g_return_if_reached ();
    }
  g_return_if_fail (str_obj);
  const char * str = gtk_string_object_get_string (str_obj);
  g_settings_set_string (data->info->settings, data->key, str);
}

static void
on_file_set (GObject * gobject, GParamSpec * pspec, gpointer user_data)
{
  CallbackData *        data = (CallbackData *) user_data;
  IdeFileChooserEntry * fc_entry = IDE_FILE_CHOOSER_ENTRY (gobject);

  GFile * file = ide_file_chooser_entry_get_file (fc_entry);
  if (!file)
    {
      g_warning ("no file selected");
      return;
    }
  char * str = g_file_get_path (file);
  g_settings_set_string (data->info->settings, data->key, str);
  g_free (str);
  g_object_unref (file);
}

static void
font_scale_adjustment_changed (GtkAdjustment * adjustment, void * data)
{
  double factor = gtk_adjustment_get_value (adjustment);
  zrythm_app_set_font_scale (zrythm_app, factor);
}

static void
reset_to_factory_response_cb (
  AdwAlertDialog * dialog,
  char *           response,
  AdwDialog *      preferences_window)
{
  if (string_is_equal (response, "reset"))
    {
      settings_reset_to_factory (false, false);
      adw_dialog_force_close (preferences_window);

      ui_show_notification_idle (_ (
        "All preferences have been successfully reset to initial "
        "values"));
    }
}

static void
on_reset_to_factory_clicked (GtkButton * btn, gpointer user_data)
{
  CallbackData * data = (CallbackData *) user_data;

  AdwAlertDialog * dialog =
    ADW_ALERT_DIALOG (adw_alert_dialog_new (_ ("Reset Settings?"), NULL));
  adw_alert_dialog_format_body_markup (
    dialog,
    _ ("This will reset Zrythm to factory settings. <b>You will lose "
       "all your preferences</b>."));
  adw_alert_dialog_add_responses (
    dialog, "cancel", _ ("_Cancel"), "reset", _ ("_Reset"), NULL);

  adw_alert_dialog_set_response_appearance (
    dialog, "reset", ADW_RESPONSE_DESTRUCTIVE);
  adw_alert_dialog_set_default_response (dialog, "cancel");
  adw_alert_dialog_set_close_response (dialog, "cancel");

  g_signal_connect (
    dialog, "response", G_CALLBACK (reset_to_factory_response_cb),
    data->preferences_widget);

  adw_alert_dialog_choose (
    dialog, GTK_WIDGET (data->preferences_widget), NULL, NULL, NULL);
}

static void
on_closure_notify_delete_data (CallbackData * data, GClosure * closure)
{
  free (data);
}

/** Path type. */
typedef enum PathType
{
  /** Not a path. */
  PATH_TYPE_NONE,

  /** Single entry separated by G_SEARCHPATH_SEPARATOR_S. */
  PATH_TYPE_ENTRY,

  /** File chooser button. */
  PATH_TYPE_FILE,

  /** File chooser button for directories. */
  PATH_TYPE_DIRECTORY,
} PathType;

/**
 * Returns if the key is a path or not.
 */
static PathType
get_path_type (const char * group, const char * subgroup, const char * key)
{
  if (KEY_IS ("General", "Paths", "zrythm-dir"))
    {
      return PATH_TYPE_DIRECTORY;
    }
  else if (
    KEY_IS ("Plugins", "Paths", "vst2-search-paths")
    || KEY_IS ("Plugins", "Paths", "vst3-search-paths")
    || KEY_IS ("Plugins", "Paths", "clap-search-paths")
    || KEY_IS ("Plugins", "Paths", "lv2-search-paths")
    || KEY_IS ("Plugins", "Paths", "dssi-search-paths")
    || KEY_IS ("Plugins", "Paths", "ladspa-search-paths")
    || KEY_IS ("Plugins", "Paths", "jsfx-search-paths")
    || KEY_IS ("Plugins", "Paths", "sfz-search-paths")
    || KEY_IS ("Plugins", "Paths", "sf2-search-paths"))
    {
      return PATH_TYPE_ENTRY;
    }

  return PATH_TYPE_NONE;
}

static bool
should_be_hidden (const char * group, const char * subgroup, const char * key)
{
  AudioBackend audio_backend =
    (AudioBackend) g_settings_get_enum (S_P_GENERAL_ENGINE, "audio-backend");
  bool audio_backend_has_opts =
    audio_backend == AUDIO_BACKEND_SDL
    || audio_backend_is_rtaudio (audio_backend);

  return
#if defined(_WOE32) || defined(__APPLE__)
    KEY_IS ("Plugins", "Paths", "dssi-search-paths")
    || KEY_IS ("Plugins", "Paths", "ladspa-search-paths") ||
#endif
#ifndef HAVE_CARLA
    KEY_IS ("Plugins", "Paths", "sfz-search-paths")
    || KEY_IS ("Plugins", "Paths", "sf2-search-paths") ||
    || KEY_IS ("Plugins", "Paths", "jsfx-search-paths") ||
#endif
    (audio_backend != AUDIO_BACKEND_SDL
     && KEY_IS ("General", "Engine", "sdl-audio-device-name"))
    || (!audio_backend_is_rtaudio (audio_backend) && KEY_IS ("General", "Engine", "rtaudio-audio-device-name"))
    || (!audio_backend_has_opts && KEY_IS ("General", "Engine", "sample-rate"))
    || (!audio_backend_has_opts && KEY_IS ("General", "Engine", "buffer-size"));
}

static void
get_range_vals (
  GVariant *           range,
  GVariant *           current_var,
  const GVariantType * type,
  double *             lower,
  double *             upper,
  double *             current)
{
  GVariant * range_vals = g_variant_get_child_value (range, 1);
  range_vals = g_variant_get_child_value (range_vals, 0);
  GVariant * lower_var = g_variant_get_child_value (range_vals, 0);
  GVariant * upper_var = g_variant_get_child_value (range_vals, 1);

#define TYPE_EQUALS(type2) \
  string_is_equal ((const char *) type, (const char *) G_VARIANT_TYPE_##type2)

  if (TYPE_EQUALS (INT32))
    {
      *lower = (double) g_variant_get_int32 (lower_var);
      *upper = (double) g_variant_get_int32 (upper_var);
      *current = (double) g_variant_get_int32 (current_var);
    }
  else if (TYPE_EQUALS (UINT32))
    {
      *lower = (double) g_variant_get_uint32 (lower_var);
      *upper = (double) g_variant_get_uint32 (upper_var);
      *current = (double) g_variant_get_uint32 (current_var);
    }
  else if (TYPE_EQUALS (DOUBLE))
    {
      *lower = g_variant_get_double (lower_var);
      *upper = (double) g_variant_get_double (upper_var);
      *current = (double) g_variant_get_double (current_var);
    }
#undef TYPE_EQUALS

  g_variant_unref (range_vals);
  g_variant_unref (lower_var);
  g_variant_unref (upper_var);
}

static void
on_audio_backend_selected_item_changed (
  GObject *    gobject,
  GParamSpec * pspec,
  gpointer     user_data)
{
  PreferencesWidget * self = Z_PREFERENCES_WIDGET (user_data);
  AudioBackend        backend =
    (AudioBackend) g_settings_get_enum (S_P_GENERAL_ENGINE, "audio-backend");
  bool show_opts = audio_backend_is_rtaudio (backend);
  gtk_widget_set_visible (
    GTK_WIDGET (self->audio_backend_rtaudio_device_row), show_opts);
  ui_setup_audio_device_name_combo_row (
    ADW_COMBO_ROW (self->audio_backend_rtaudio_device_row),
    show_opts && MAIN_WINDOW, false);
  gtk_widget_set_visible (
    GTK_WIDGET (self->audio_backend_buffer_size_row), show_opts);
  gtk_widget_set_visible (
    GTK_WIDGET (self->audio_backend_sample_rate_row), show_opts);
}

static GtkWidget *
make_control (
  PreferencesWidget * self,
  int                 group_idx,
  int                 subgroup_idx,
  const char *        key)
{
  SubgroupInfo * info = &self->subgroup_infos[group_idx][subgroup_idx];
  const char *   group = info->group_name;
  const char *   subgroup = info->name;

  if (string_is_equal (key, reset_to_factory_key))
    {
      GtkWidget *    widget = gtk_button_new_with_label (_ ("Reset"));
      CallbackData * data = object_new (CallbackData);
      data->info = info;
      data->preferences_widget = self;
      data->key = g_strdup (key);
      g_signal_connect_data (
        G_OBJECT (widget), "clicked", G_CALLBACK (on_reset_to_factory_clicked),
        data, (GClosureNotify) on_closure_notify_delete_data, G_CONNECT_AFTER);
      return widget;
    }

  GSettingsSchemaKey * schema_key =
    g_settings_schema_get_key (info->schema, key);
  const GVariantType * type = g_settings_schema_key_get_value_type (schema_key);
  GVariant *           current_var = g_settings_get_value (info->settings, key);
  GVariant *           range = g_settings_schema_key_get_range (schema_key);

#if 0
  g_message ("%s",
    g_variant_get_type_string (current_var));
#endif

#define TYPE_EQUALS(type2) \
  string_is_equal ((const char *) type, (const char *) G_VARIANT_TYPE_##type2)

  GtkWidget * widget = NULL;
  if (
    KEY_IS ("General", "Engine", "rtaudio-audio-device-name")
    || KEY_IS ("General", "Engine", "sdl-audio-device-name"))
    {
      widget = adw_combo_row_new ();
      ui_setup_audio_device_name_combo_row (
        ADW_COMBO_ROW (widget), MAIN_WINDOW != NULL, true);
      if (KEY_IS ("General", "Engine", "rtaudio-audio-device-name"))
        {
          self->audio_backend_rtaudio_device_row = ADW_PREFERENCES_ROW (widget);
        }
    }
  else if (KEY_IS ("General", "Engine", "midi-backend"))
    {
      widget = GTK_WIDGET (ui_gen_midi_backends_combo_row (true));
    }
  else if (KEY_IS ("General", "Engine", "audio-backend"))
    {
      widget = GTK_WIDGET (ui_gen_audio_backends_combo_row (true));
      g_signal_connect (
        G_OBJECT (widget), "notify::selected-item",
        G_CALLBACK (on_audio_backend_selected_item_changed), self);
    }
  else if (KEY_IS ("General", "Engine", "audio-inputs"))
    {
      widget = g_object_new (ACTIVE_HARDWARE_MB_WIDGET_TYPE, NULL);
      active_hardware_mb_widget_setup (
        Z_ACTIVE_HARDWARE_MB_WIDGET (widget), F_INPUT, F_NOT_MIDI,
        S_P_GENERAL_ENGINE, "audio-inputs");
    }
  else if (KEY_IS ("General", "Engine", "midi-controllers"))
    {
      widget = g_object_new (ACTIVE_HARDWARE_MB_WIDGET_TYPE, NULL);
      active_hardware_mb_widget_setup (
        Z_ACTIVE_HARDWARE_MB_WIDGET (widget), F_INPUT, F_MIDI,
        S_P_GENERAL_ENGINE, "midi-controllers");
    }
  else if (KEY_IS ("UI", "General", "font-scale"))
    {
      double lower = 0.f, upper = 1.f, current = 0.f;
      get_range_vals (range, current_var, type, &lower, &upper, &current);
      widget = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
      gtk_widget_set_visible (widget, true);
      GtkWidget * scale = gtk_scale_new_with_range (
        GTK_ORIENTATION_HORIZONTAL, lower, upper, 0.1);
      gtk_widget_set_visible (scale, true);
      gtk_widget_set_hexpand (scale, true);
      gtk_scale_add_mark (GTK_SCALE (scale), 1.0, GTK_POS_TOP, NULL);
      gtk_box_append (GTK_BOX (widget), scale);
      GtkAdjustment * adj = gtk_range_get_adjustment (GTK_RANGE (scale));
      gtk_adjustment_set_value (adj, current);
      g_settings_bind (
        info->settings, key, adj, "value", G_SETTINGS_BIND_DEFAULT);
      g_signal_connect (
        adj, "value-changed", G_CALLBACK (font_scale_adjustment_changed), NULL);
    }
  else if (
    KEY_IS ("UI", "General", "css-theme")
    || KEY_IS ("UI", "General", "icon-theme"))
    {
      char * user_css_theme_path = zrythm_get_dir (
        (KEY_IS ("UI", "General", "css-theme"))
          ? ZRYTHM_DIR_USER_THEMES_CSS
          : ZRYTHM_DIR_USER_THEMES_ICONS);
      char ** css_themes =
        io_get_files_in_dir_as_basenames (user_css_theme_path, true, true);
      const char * default_themes[] = {
        (KEY_IS ("UI", "General", "css-theme"))
          ? "zrythm-theme.css"
          : "zrythm-dark",
        NULL
      };
      GtkStringList * string_list = gtk_string_list_new (default_themes);
      gtk_string_list_splice (string_list, 1, 0, (const char **) css_themes);
      g_strfreev (css_themes);
      widget = adw_combo_row_new ();
      adw_combo_row_set_model (
        ADW_COMBO_ROW (widget), G_LIST_MODEL (string_list));

      /* select */
      char * selected_str = g_settings_get_string (info->settings, key);
      guint  n_items = g_list_model_get_n_items (G_LIST_MODEL (string_list));
      guint  selected_idx = 0;
      for (guint i = 0; i < n_items; i++)
        {
          const char * cur_str = gtk_string_list_get_string (string_list, i);
          if (string_is_equal (cur_str, selected_str))
            {
              selected_idx = i;
              break;
            }
        }
      g_free (selected_str);
      adw_combo_row_set_selected (ADW_COMBO_ROW (widget), selected_idx);

      CallbackData * data = object_new (CallbackData);
      data->info = info;
      data->preferences_widget = self;
      data->key = g_strdup (key);
      g_signal_connect_data (
        G_OBJECT (widget), "notify::selected",
        G_CALLBACK (on_string_drop_down_selection_changed), data,
        (GClosureNotify) on_closure_notify_delete_data, G_CONNECT_AFTER);
    }
  else if (TYPE_EQUALS (BOOLEAN))
    {
      widget = adw_switch_row_new ();
      g_settings_bind (
        info->settings, key, widget, "active", G_SETTINGS_BIND_DEFAULT);
    }
  else if (TYPE_EQUALS (INT32) || TYPE_EQUALS (UINT32) || TYPE_EQUALS (DOUBLE))
    {
      double lower = 0.f, upper = 1.f, current = 0.f;
      get_range_vals (range, current_var, type, &lower, &upper, &current);
      GtkAdjustment * adj =
        gtk_adjustment_new (current, lower, upper, 1.0, 1.0, 1.0);
      widget = adw_spin_row_new (adj, 1, TYPE_EQUALS (DOUBLE) ? 3 : 0);
      g_settings_bind (
        info->settings, key, widget, "value", G_SETTINGS_BIND_DEFAULT);
    }
  else if (TYPE_EQUALS (STRING))
    {
      PathType path_type = get_path_type (info->group_name, info->name, key);
      if (path_type == PATH_TYPE_DIRECTORY || path_type == PATH_TYPE_FILE)
        {
          widget = GTK_WIDGET (ide_file_chooser_entry_new (
            path_type == PATH_TYPE_DIRECTORY
              ? _ ("Select a folder")
              : _ ("Select a file"),
            path_type == PATH_TYPE_DIRECTORY
              ? GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER
              : GTK_FILE_CHOOSER_ACTION_OPEN));
          char *  path = g_settings_get_string (info->settings, key);
          GFile * gf_path = g_file_new_for_path (path);
          ide_file_chooser_entry_set_file (
            IDE_FILE_CHOOSER_ENTRY (widget), gf_path);
          g_free (path);
          g_object_unref (gf_path);
          CallbackData * data = object_new (CallbackData);
          data->info = info;
          data->preferences_widget = self;
          data->key = g_strdup (key);
          data->widget = widget;
          g_signal_connect_data (
            widget, "notify::file", G_CALLBACK (on_file_set), data,
            (GClosureNotify) on_closure_notify_delete_data, G_CONNECT_AFTER);
        }
      else if (path_type == PATH_TYPE_NONE)
        {
          /* map enums */
          const char **          strv = NULL;
          const cyaml_strval_t * cyaml_strv = NULL;
          size_t                 size = 0;

#define SET_STRV_IF_MATCH(a, b, c, arr_name) \
  if (KEY_IS (a, b, c)) \
    { \
      strv = arr_name; \
      size = G_N_ELEMENTS (arr_name); \
    }

#define SET_STRV_IF_MATCH_W_COUNT(a, b, c, arr_name, count) \
  if (KEY_IS (a, b, c)) \
    { \
      strv = arr_name; \
      size = count; \
    }

#define SET_STRV_FROM_CYAML_IF_MATCH(a, b, c, arr_name) \
  if (KEY_IS (a, b, c)) \
    { \
      cyaml_strv = arr_name; \
      size = G_N_ELEMENTS (arr_name); \
    }

          SET_STRV_IF_MATCH (
            "General", "Engine", "audio-backend", audio_backend_str);
          SET_STRV_IF_MATCH (
            "General", "Engine", "midi-backend", midi_backend_str);
          SET_STRV_IF_MATCH (
            "General", "Engine", "sample-rate", sample_rate_str);
          SET_STRV_IF_MATCH (
            "General", "Engine", "buffer-size", buffer_size_str);
          SET_STRV_FROM_CYAML_IF_MATCH (
            "Editing", "Audio", "fade-algorithm", curve_algorithm_strings);
          SET_STRV_FROM_CYAML_IF_MATCH (
            "Editing", "Automation", "curve-algorithm", curve_algorithm_strings);
          SET_STRV_IF_MATCH_W_COUNT (
            "UI", "General", "language",
            localization_get_language_strings_w_codes (), NUM_LL_LANGUAGES);
          SET_STRV_IF_MATCH ("UI", "General", "graphic-detail", ui_detail_str);
          SET_STRV_IF_MATCH ("DSP", "Pan", "pan-algorithm", pan_algorithm_str);
          SET_STRV_IF_MATCH ("DSP", "Pan", "pan-law", pan_law_str);

#undef SET_STRV_IF_MATCH

          if (strv || cyaml_strv)
            {
              GtkStringList * string_list = gtk_string_list_new (NULL);
              for (size_t i = 0; i < size; i++)
                {
                  if (cyaml_strv)
                    {
                      gtk_string_list_append (
                        string_list, _ (cyaml_strv[i].str));
                    }
                  else if (strv)
                    {
                      gtk_string_list_append (string_list, _ (strv[i]));
                    }
                }
              widget = adw_combo_row_new ();
              adw_combo_row_set_model (
                ADW_COMBO_ROW (widget), G_LIST_MODEL (string_list));
              adw_combo_row_set_selected (
                ADW_COMBO_ROW (widget),
                (unsigned int) g_settings_get_enum (info->settings, key));
              CallbackData * data = object_new (CallbackData);
              data->info = info;
              data->preferences_widget = self;
              data->key = g_strdup (key);
              g_signal_connect_data (
                G_OBJECT (widget), "notify::selected",
                G_CALLBACK (on_enum_drop_down_selection_changed), data,
                (GClosureNotify) on_closure_notify_delete_data, G_CONNECT_AFTER);
            }
          /* else if not a string array */
          else
            {
              /* create basic string entry control */
              widget = adw_entry_row_new ();
              char * current_val = g_settings_get_string (info->settings, key);
              gtk_editable_set_text (GTK_EDITABLE (widget), current_val);
              g_free (current_val);
              CallbackData * data = object_new (CallbackData);
              data->info = info;
              data->preferences_widget = self;
              data->key = g_strdup (key);
              g_signal_connect_data (
                G_OBJECT (widget), "changed",
                G_CALLBACK (on_simple_string_entry_changed), data,
                (GClosureNotify) on_closure_notify_delete_data, G_CONNECT_AFTER);
            }

          if (KEY_IS ("General", "Engine", "sample-rate"))
            {
              self->audio_backend_sample_rate_row = ADW_PREFERENCES_ROW (widget);
            }
          else if (KEY_IS ("General", "Engine", "buffer-size"))
            {
              self->audio_backend_buffer_size_row = ADW_PREFERENCES_ROW (widget);
            }
        } /* endif PATH_TYPE_NONE */
    }
  else if (TYPE_EQUALS (STRING_ARRAY))
    {
      if (get_path_type (info->group_name, info->name, key) == PATH_TYPE_ENTRY)
        {
          widget = adw_entry_row_new ();
          char ** paths = g_settings_get_strv (info->settings, key);
          char *  joined_str = g_strjoinv (G_SEARCHPATH_SEPARATOR_S, paths);
          gtk_editable_set_text (GTK_EDITABLE (widget), joined_str);
          g_free (joined_str);
          g_strfreev (paths);
          CallbackData * data = object_new (CallbackData);
          data->info = info;
          data->preferences_widget = self;
          data->key = g_strdup (key);
          g_signal_connect_data (
            G_OBJECT (widget), "changed", G_CALLBACK (on_path_entry_changed),
            data, (GClosureNotify) on_closure_notify_delete_data,
            G_CONNECT_AFTER);
        }
    }
#if 0
  else if (string_is_equal (
             g_variant_get_type_string (
               current_var), "ai"))
    {
    }
#endif

#undef TYPE_EQUALS

  g_warn_if_fail (widget);

  return widget;
}

static void
add_subgroup (
  PreferencesWidget *  self,
  AdwPreferencesPage * page,
  int                  group_idx,
  int                  subgroup_idx,
  GtkSizeGroup *       size_group)
{
  SubgroupInfo * info = &self->subgroup_infos[group_idx][subgroup_idx];

  const char * localized_subgroup_name = info->name;
  g_message ("adding subgroup %s (%s)", info->name, localized_subgroup_name);

  /* create a section for the subgroup */
  AdwPreferencesGroup * subgroup =
    ADW_PREFERENCES_GROUP (adw_preferences_group_new ());
  adw_preferences_group_set_title (subgroup, localized_subgroup_name);
  if (
    string_is_equal (localized_subgroup_name, _ ("Paths"))
    && string_is_equal (info->group_name, _ ("Plugins")))
    {
      char * descr = g_strdup_printf (
        _ ("Multiple search paths must be separated by the %s character. Environment variables are supported with the following syntax: ${ENV_VAR_NAME}. Leave blank to use default paths."),
        G_SEARCHPATH_SEPARATOR_S);
      adw_preferences_group_set_description (subgroup, descr);
    }
  adw_preferences_page_add (page, subgroup);

  GStrvBuilder * builder = g_strv_builder_new ();
  char **        keys = g_settings_schema_list_keys (info->schema);
  g_strv_builder_addv (builder, (const char **) keys);
  g_strfreev (keys);

  /* add option to reset to factory */
  if (group_idx == 0 && string_is_equal (localized_subgroup_name, _ ("Other")))
    {
      g_strv_builder_add (builder, reset_to_factory_key);
    }

  keys = g_strv_builder_end (builder);

  int    i = 0;
  int    num_controls = 0;
  char * key;
  while ((key = keys[i++]))
    {
      const char * summary;
      const char * description;

      if (string_is_equal (reset_to_factory_key, key))
        {
          summary = _ ("Reset to factory settings");
          description = _ (
            "Reset all preferences to initial "
            "values.");
        }
      else
        {
          GSettingsSchemaKey * schema_key =
            g_settings_schema_get_key (info->schema, key);
          /* note: this is already translated */
          summary = g_settings_schema_key_get_summary (schema_key);
          /* note: this is already translated */
          description = g_settings_schema_key_get_description (schema_key);
        }

      if (string_is_equal (key, "info"))
        continue;

      g_message ("adding control for %s", key);

      /* add control */
      GtkWidget * widget = make_control (self, group_idx, subgroup_idx, key);
      if (widget)
        {
          bool                widget_is_row = ADW_IS_PREFERENCES_ROW (widget);
          AdwPreferencesRow * row =
            widget_is_row
              ? ADW_PREFERENCES_ROW (widget)
              : ADW_PREFERENCES_ROW (adw_action_row_new ());

          adw_preferences_row_set_title (row, summary);
          if (ADW_IS_ACTION_ROW (row))
            {
              adw_action_row_set_subtitle (ADW_ACTION_ROW (row), description);
            }
          else
            {
              gtk_widget_set_tooltip_text (GTK_WIDGET (row), description);
            }
          adw_preferences_group_add (subgroup, GTK_WIDGET (row));

          if (!widget_is_row)
            {
              if (GTK_IS_SWITCH (widget))
                {
                  gtk_widget_set_halign (widget, GTK_ALIGN_START);
                }
              else
                {
                  gtk_widget_set_hexpand (widget, true);
                }
              gtk_widget_set_valign (widget, GTK_ALIGN_CENTER);
              adw_action_row_add_suffix (ADW_ACTION_ROW (row), widget);
            }

          if (should_be_hidden (info->group_name, info->name, key))
            {
              gtk_widget_set_visible (GTK_WIDGET (row), false);
            }

          num_controls++;
        }
      else
        {
          g_warning ("no widget for %s", key);
        }
    }

  g_strfreev (keys);

  /* Remove label if no controls added */
  if (num_controls == 0)
    {
      adw_preferences_page_remove (page, subgroup);
    }
}

static const char *
get_group_icon (const char * schema_str)
{
  const char * icon_name = NULL;
  if (string_contains_substr (schema_str, "preferences.general"))
    {
      icon_name = "info";
    }
  else if (string_contains_substr (schema_str, "preferences.scripting"))
    {
      icon_name = "amarok_scripts";
    }
  else if (string_contains_substr (schema_str, "preferences.dsp"))
    {
      icon_name = "gnome-icon-library-terminal-symbolic";
    }
  else if (string_contains_substr (schema_str, "preferences.plugins"))
    {
      icon_name = "gnome-icon-library-puzzle-piece-symbolic";
    }
  else if (string_contains_substr (schema_str, "preferences.editing"))
    {
      icon_name = "pencil";
    }
  else if (string_contains_substr (schema_str, "preferences.projects"))
    {
      icon_name = "gnome-icon-library-rich-text-symbolic";
    }
  else if (string_contains_substr (schema_str, "preferences.ui"))
    {
      icon_name = "gnome-icon-library-display-symbolic";
    }

  g_debug ("icon name for %s: %s", schema_str, icon_name);

  return icon_name;
}

static void
add_group (PreferencesWidget * self, int group_idx)
{
  GSettingsSchemaSource * source = g_settings_schema_source_get_default ();
  char **                 non_relocatable;
  g_settings_schema_source_list_schemas (source, 1, &non_relocatable, NULL);

  /* loop once to get the max subgroup index and group name */
  char *       schema_str;
  const char * group_name = NULL;
  const char * subgroup_name = NULL;
  int          i = 0;
  int          max_subgroup_idx = 0;
  while ((schema_str = non_relocatable[i++]))
    {
      if (!string_contains_substr (
            schema_str, GSETTINGS_ZRYTHM_PREFIX ".preferences"))
        continue;

      /* get the preferences.x.y schema */
      GSettingsSchema * schema =
        g_settings_schema_source_lookup (source, schema_str, 1);
      g_return_if_fail (schema);

      GSettings * settings = g_settings_new (schema_str);
      GVariant *  info_val = g_settings_get_value (settings, "info");
      int         this_group_idx =
        (int) g_variant_get_int32 (g_variant_get_child_value (info_val, 0));

      if (this_group_idx != group_idx)
        continue;

      GSettingsSchemaKey * info_key = g_settings_schema_get_key (schema, "info");
      /* note: this is already translated */
      group_name = g_settings_schema_key_get_summary (info_key);
      /* note: this is already translated */
      subgroup_name = g_settings_schema_key_get_description (info_key);

      /* get max subgroup index */
      int subgroup_idx =
        (int) g_variant_get_int32 (g_variant_get_child_value (info_val, 1));
      SubgroupInfo * nfo = &self->subgroup_infos[group_idx][subgroup_idx];
      nfo->schema = schema;
      nfo->settings = settings;
      nfo->group_name = group_name;
      nfo->name = subgroup_name;
      nfo->group_idx = group_idx;
      nfo->subgroup_idx = subgroup_idx;
      nfo->group_icon = get_group_icon (schema_str);
      if (subgroup_idx > max_subgroup_idx)
        max_subgroup_idx = subgroup_idx;
    }

  const char * localized_group_name = group_name;
  g_message ("adding group %s (%s)", group_name, localized_group_name);

  /* create a page for the group */
  AdwPreferencesPage * page = ADW_PREFERENCES_PAGE (adw_preferences_page_new ());
  adw_preferences_page_set_title (page, localized_group_name);
  adw_preferences_dialog_add (ADW_PREFERENCES_DIALOG (self), page);

  /* set icon */
  if (self->subgroup_infos[group_idx][0].group_icon)
    {
      adw_preferences_page_set_icon_name (
        page, self->subgroup_infos[group_idx][0].group_icon);
    }

  /* create a sizegroup for the labels */
  GtkSizeGroup * size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  /* add each subgroup */
  for (int j = 0; j <= max_subgroup_idx; j++)
    {
      add_subgroup (self, page, group_idx, j, size_group);
    }
}

static void
on_window_closed (GtkWidget * object, PreferencesWidget * self)
{
  ui_show_notification_idle_printf (
    _ ("Some changes will only take effect after you restart %s"), PROGRAM_NAME);

  if (MAIN_WINDOW)
    {
      MAIN_WINDOW->preferences_opened = false;
    }
}

/**
 * Sets up the preferences widget.
 */
PreferencesWidget *
preferences_widget_new (void)
{
  PreferencesWidget * self =
    g_object_new (PREFERENCES_WIDGET_TYPE, "title", _ ("Preferences"), NULL);

  for (int i = 0; i <= 6; i++)
    {
      /* skip scripting for now - out of scope */
      if (i == 6)
        continue;

      add_group (self, i);
    }

  g_signal_connect (
    G_OBJECT (self), "destroy", G_CALLBACK (on_window_closed), self);

  return self;
}

static void
preferences_widget_class_init (PreferencesWidgetClass * _klass)
{
}

static void
preferences_widget_init (PreferencesWidget * self)
{
#if 0
  self->group_notebook =
    GTK_NOTEBOOK (gtk_notebook_new ());
  gtk_widget_set_visible (
    GTK_WIDGET (self->group_notebook), true);
  gtk_box_append (
    GTK_BOX (
      gtk_dialog_get_content_area (
        GTK_DIALOG (self))),
    GTK_WIDGET (self->group_notebook));
#endif
}
