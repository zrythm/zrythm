/*
 * Copyright (C) 2021 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * File browser.
 */

#include "audio/engine.h"
#include "audio/sample_processor.h"
#include "gui/widgets/file_auditioner_controls.h"
#include "gui/widgets/volume.h"
#include "plugins/plugin_manager.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/objects.h"
#include "utils/resources.h"
#include "zrythm.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  FileAuditionerControlsWidget,
  file_auditioner_controls_widget,
  GTK_TYPE_BOX)

static void
on_play_clicked (
  GtkButton *          toolbutton,
  FileAuditionerControlsWidget * self)
{
  SupportedFile * descr =
    self->selected_file_getter (self->owner);
  if (!descr)
    return;

  sample_processor_queue_file (
    SAMPLE_PROCESSOR, descr);
}

static void
on_stop_clicked (
  GtkButton *          toolbutton,
  FileAuditionerControlsWidget * self)
{
  sample_processor_stop_file_playback (
    SAMPLE_PROCESSOR);
}

static void
on_settings_menu_items_changed (
  GMenuModel *             model,
  int                      position,
  int                      removed,
  int                      added,
  FileAuditionerControlsWidget * self)
{
  self->refilter_files (self->owner);
}

static void
on_instrument_changed (
  GtkComboBox *            cb,
  FileAuditionerControlsWidget * self)
{
  const char * active_id =
    gtk_combo_box_get_active_id (cb);

  g_message ("changed: %s", active_id);

  PluginDescriptor * descr =
    (PluginDescriptor *)
    yaml_deserialize (
      active_id, &plugin_descriptor_schema);

  if (SAMPLE_PROCESSOR->instrument_setting &&
      plugin_descriptor_is_same_plugin (
         SAMPLE_PROCESSOR->instrument_setting->
           descr,
         descr))
    return;

  EngineState state;
  engine_wait_for_pause (
    AUDIO_ENGINE, &state, false);

  /* clear previous instrument setting */
  if (SAMPLE_PROCESSOR->instrument_setting)
    {
      object_free_w_func_and_null (
        plugin_setting_free,
        SAMPLE_PROCESSOR->instrument_setting);
    }

  /* set setting */
  PluginSetting * existing_setting =
    plugin_settings_find (
      S_PLUGIN_SETTINGS, descr);
  if (existing_setting)
    {
      SAMPLE_PROCESSOR->instrument_setting =
        plugin_setting_clone (
          existing_setting, F_VALIDATE);
    }
  else
    {
      SAMPLE_PROCESSOR->instrument_setting =
        plugin_setting_new_default (descr);
    }
  g_return_if_fail (
    SAMPLE_PROCESSOR->instrument_setting);

  /* save setting */
  char * setting_yaml =
    yaml_serialize (
      SAMPLE_PROCESSOR->instrument_setting,
      &plugin_setting_schema);
  g_settings_set_string (
    S_UI_FILE_BROWSER, "instrument", setting_yaml);
  g_free (setting_yaml);

  plugin_descriptor_free (descr);

  engine_resume (AUDIO_ENGINE, &state);
}

static void
setup_instrument_cb (
  FileAuditionerControlsWidget * self)
{
  /* populate instruments */
  for (size_t i = 0;
       i < PLUGIN_MANAGER->plugin_descriptors->
         len;
       i++)
    {
      PluginDescriptor * descr =
        g_ptr_array_index (
          PLUGIN_MANAGER->plugin_descriptors, i);
      if (plugin_descriptor_is_instrument (descr))
        {
          char * id =
            yaml_serialize (
              descr, &plugin_descriptor_schema);
          gtk_combo_box_text_append (
            self->instrument_cb, id,
            descr->name);
          g_free (id);
        }
    }

  /* set selected instrument */
  if (SAMPLE_PROCESSOR->instrument_setting)
    {
      char * id =
        yaml_serialize (
          SAMPLE_PROCESSOR->instrument_setting->
            descr,
          &plugin_descriptor_schema);
      gtk_combo_box_set_active_id (
        GTK_COMBO_BOX (self->instrument_cb), id);
    }

  /* add instrument signal handler */
  g_signal_connect (
    G_OBJECT (self->instrument_cb), "changed",
    G_CALLBACK (on_instrument_changed), self);
}

/**
 * Sets up a FileAuditionerControlsWidget.
 */
void
file_auditioner_controls_widget_setup (
  FileAuditionerControlsWidget * self,
  GtkWidget *                    owner,
  SelectedFileGetter             selected_file_getter,
  GenericCallback                refilter_files_cb)
{
  self->owner = owner;
  self->selected_file_getter = selected_file_getter;
  self->refilter_files = refilter_files_cb;

  volume_widget_setup (
    self->volume, SAMPLE_PROCESSOR->fader->amp);

  setup_instrument_cb (self);
}

static void
file_auditioner_controls_widget_class_init (
  FileAuditionerControlsWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "file_auditioner_controls.ui");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, FileAuditionerControlsWidget, x)

  BIND_CHILD (volume);
  BIND_CHILD (play_btn);
  BIND_CHILD (stop_btn);
  BIND_CHILD (file_settings_btn);
  BIND_CHILD (instrument_cb);

#undef BIND_CHILD

#define BIND_SIGNAL(sig) \
  gtk_widget_class_bind_template_callback ( \
    klass, sig)

  BIND_SIGNAL (on_play_clicked);
  BIND_SIGNAL (on_stop_clicked);

#undef BIND_SIGNAL
}

static void
file_auditioner_controls_widget_init (
  FileAuditionerControlsWidget * self)
{
  g_type_ensure (VOLUME_WIDGET_TYPE);

  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_widget_add_css_class (
    GTK_WIDGET (self), "file-auditioner-controls");

  /* set menu */
  GSimpleActionGroup * action_group =
    g_simple_action_group_new ();
  GAction * action =
    g_settings_create_action (
      S_UI_FILE_BROWSER, "autoplay");
  g_action_map_add_action (
    G_ACTION_MAP (action_group), action);
  action =
    g_settings_create_action (
      S_UI_FILE_BROWSER, "show-unsupported-files");
  g_action_map_add_action (
    G_ACTION_MAP (action_group), action);
  action =
    g_settings_create_action (
      S_UI_FILE_BROWSER, "show-hidden-files");
  g_action_map_add_action (
    G_ACTION_MAP (action_group), action);
  gtk_widget_insert_action_group (
    GTK_WIDGET (self->file_settings_btn),
    "settings-btn",
    G_ACTION_GROUP (action_group));

  GMenu * menu = g_menu_new ();
  g_menu_append (
    menu, _("Autoplay"), "settings-btn.autoplay");
  g_menu_append (
    menu, _("Show unsupported files"),
    "settings-btn.show-unsupported-files");
  g_menu_append (
    menu, _("Show hidden files"),
    "settings-btn.show-hidden-files");
  gtk_menu_button_set_menu_model (
    self->file_settings_btn,
    G_MENU_MODEL (menu));

  g_signal_connect (
    menu, "items-changed",
    G_CALLBACK (on_settings_menu_items_changed),
    self);
}
