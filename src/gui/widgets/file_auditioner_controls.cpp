// SPDX-FileCopyrightText: © 2021-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * File browser.
 */

#include "dsp/engine.h"
#include "dsp/sample_processor.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/backend/wrapped_object_with_change_signal.h"
#include "gui/widgets/file_auditioner_controls.h"
#include "gui/widgets/volume.h"
#include "io/serialization/plugin.h"
#include "plugins/plugin_manager.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/objects.h"
#include "utils/resources.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  FileAuditionerControlsWidget,
  file_auditioner_controls_widget,
  GTK_TYPE_BOX)

static void
on_play_clicked (GtkButton * toolbutton, FileAuditionerControlsWidget * self)
{
  WrappedObjectWithChangeSignal * wrapped_obj =
    self->selected_file_getter (self->owner);
  if (!wrapped_obj)
    return;

  switch (wrapped_obj->type)
    {
    case WrappedObjectType::WRAPPED_OBJECT_TYPE_SUPPORTED_FILE:
      sample_processor_queue_file (
        SAMPLE_PROCESSOR, (SupportedFile *) wrapped_obj->obj);
      break;
    case WrappedObjectType::WRAPPED_OBJECT_TYPE_CHORD_PSET:
      sample_processor_queue_chord_preset (
        SAMPLE_PROCESSOR, (ChordPreset *) wrapped_obj->obj);
      break;
    default:
      break;
    }
}

static void
on_stop_clicked (GtkButton * toolbutton, FileAuditionerControlsWidget * self)
{
  sample_processor_stop_file_playback (SAMPLE_PROCESSOR);
}

static void
on_settings_menu_items_changed (
  GMenuModel *                   model,
  int                            position,
  int                            removed,
  int                            added,
  FileAuditionerControlsWidget * self)
{
  self->refilter_files (self->owner);
}

static void
on_instrument_changed (GObject * gobject, GParamSpec * pspec, void * data)
{
  GtkDropDown *                   dropdown = GTK_DROP_DOWN (gobject);
  WrappedObjectWithChangeSignal * wrapped_obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (
      gtk_drop_down_get_selected_item (dropdown));
  const PluginDescriptor * descr = (PluginDescriptor *) wrapped_obj->obj;

  if (
    SAMPLE_PROCESSOR->instrument_setting
    && plugin_descriptor_is_same_plugin (
      SAMPLE_PROCESSOR->instrument_setting->descr, descr))
    return;

  EngineState state;
  engine_wait_for_pause (AUDIO_ENGINE, &state, false, true);

  /* clear previous instrument setting */
  if (SAMPLE_PROCESSOR->instrument_setting)
    {
      object_free_w_func_and_null (
        plugin_setting_free, SAMPLE_PROCESSOR->instrument_setting);
    }

  /* set setting */
  PluginSetting * existing_setting =
    plugin_settings_find (S_PLUGIN_SETTINGS, descr);
  if (existing_setting)
    {
      SAMPLE_PROCESSOR->instrument_setting =
        plugin_setting_clone (existing_setting, F_VALIDATE);
    }
  else
    {
      SAMPLE_PROCESSOR->instrument_setting = plugin_setting_new_default (descr);
    }
  g_return_if_fail (SAMPLE_PROCESSOR->instrument_setting);

  /* save setting */
  yyjson_mut_doc * doc = yyjson_mut_doc_new (NULL);
  yyjson_mut_val * root = yyjson_mut_obj (doc);
  g_return_if_fail (root);
  yyjson_mut_doc_set_root (doc, root);
  GError * err = NULL;
  plugin_setting_serialize_to_json (
    doc, root, SAMPLE_PROCESSOR->instrument_setting, &err);
  char * setting_json = yyjson_mut_write (doc, YYJSON_WRITE_NOFLAG, NULL);
  if (setting_json)
    {
      g_settings_set_string (S_UI_FILE_BROWSER, "instrument", setting_json);
      g_free (setting_json);
    }
  else
    {
      HANDLE_ERROR_LITERAL (err, "Failed to serialize instrument setting");
    }

  engine_resume (AUDIO_ENGINE, &state);

  EVENTS_PUSH (EventType::ET_FILE_BROWSER_INSTRUMENT_CHANGED, NULL);
}

static void
setup_instrument_dropdown (FileAuditionerControlsWidget * self)
{
  GListStore * store = g_list_store_new (WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE);

  /* populate instruments */
  int selected = -1;
  int num_added = 0;
  for (size_t i = 0; i < PLUGIN_MANAGER->plugin_descriptors->len; i++)
    {
      PluginDescriptor * descr = static_cast<PluginDescriptor *> (
        g_ptr_array_index (PLUGIN_MANAGER->plugin_descriptors, i));
      if (plugin_descriptor_is_instrument (descr))
        {
          WrappedObjectWithChangeSignal * wrapped_descr =
            wrapped_object_with_change_signal_new (
              descr, WrappedObjectType::WRAPPED_OBJECT_TYPE_PLUGIN_DESCR);
          g_list_store_append (store, wrapped_descr);

          /* set selected instrument */
          if (
            SAMPLE_PROCESSOR->instrument_setting
            && plugin_descriptor_is_same_plugin (
              SAMPLE_PROCESSOR->instrument_setting->descr, descr))
            {
              selected = num_added;
            }

          num_added++;
        }
    }

  gtk_drop_down_set_model (self->instrument_dropdown, G_LIST_MODEL (store));

  GtkExpression * expr = gtk_cclosure_expression_new (
    G_TYPE_STRING, NULL, 0, NULL,
    G_CALLBACK (wrapped_object_with_change_signal_get_display_name), NULL, NULL);
  gtk_drop_down_set_expression (self->instrument_dropdown, expr);

  if (selected >= 0)
    {
      gtk_drop_down_set_selected (
        self->instrument_dropdown, (unsigned int) selected);
    }

  /* add instrument signal handler */
  g_signal_connect (
    G_OBJECT (self->instrument_dropdown), "notify::selected-item",
    G_CALLBACK (on_instrument_changed), self);
}

/**
 * Sets up a FileAuditionerControlsWidget.
 */
void
file_auditioner_controls_widget_setup (
  FileAuditionerControlsWidget * self,
  GtkWidget *                    owner,
  bool                           for_files,
  SelectedFileGetter             selected_file_getter,
  GenericCallback                refilter_files_cb)
{
  self->owner = owner;
  self->selected_file_getter = selected_file_getter;
  self->refilter_files = refilter_files_cb;

  volume_widget_setup (self->volume, SAMPLE_PROCESSOR->fader->amp);

  self->for_files = for_files;
  GMenu * menu = g_menu_new ();
  g_menu_append (menu, _ ("Autoplay"), "settings-btn.autoplay");
  if (for_files)
    {
      g_menu_append (
        menu, _ ("Show unsupported files"),
        "settings-btn.show-unsupported-files");
      g_menu_append (
        menu, _ ("Show hidden files"), "settings-btn.show-hidden-files");
    }
  gtk_menu_button_set_menu_model (self->file_settings_btn, G_MENU_MODEL (menu));
  g_signal_connect (
    menu, "items-changed", G_CALLBACK (on_settings_menu_items_changed), self);

  setup_instrument_dropdown (self);
}

static void
file_auditioner_controls_widget_class_init (
  FileAuditionerControlsWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass, "file_auditioner_controls.ui");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child (klass, FileAuditionerControlsWidget, x)

  BIND_CHILD (volume);
  BIND_CHILD (play_btn);
  BIND_CHILD (stop_btn);
  BIND_CHILD (file_settings_btn);
  BIND_CHILD (instrument_dropdown);

#undef BIND_CHILD

#define BIND_SIGNAL(sig) gtk_widget_class_bind_template_callback (klass, sig)

  BIND_SIGNAL (on_play_clicked);
  BIND_SIGNAL (on_stop_clicked);

#undef BIND_SIGNAL
}

static void
file_auditioner_controls_widget_init (FileAuditionerControlsWidget * self)
{
  g_type_ensure (VOLUME_WIDGET_TYPE);

  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_widget_add_css_class (GTK_WIDGET (self), "file-auditioner-controls");

  /* set menu */
  GSimpleActionGroup * action_group = g_simple_action_group_new ();
  GAction * action = g_settings_create_action (S_UI_FILE_BROWSER, "autoplay");
  g_action_map_add_action (G_ACTION_MAP (action_group), action);
  action =
    g_settings_create_action (S_UI_FILE_BROWSER, "show-unsupported-files");
  g_action_map_add_action (G_ACTION_MAP (action_group), action);
  action = g_settings_create_action (S_UI_FILE_BROWSER, "show-hidden-files");
  g_action_map_add_action (G_ACTION_MAP (action_group), action);
  gtk_widget_insert_action_group (
    GTK_WIDGET (self->file_settings_btn), "settings-btn",
    G_ACTION_GROUP (action_group));
}
