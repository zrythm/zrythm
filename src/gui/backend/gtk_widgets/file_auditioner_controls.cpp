// SPDX-FileCopyrightText: © 2021-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/engine.h"
#include "common/dsp/sample_processor.h"
#include "common/plugins/plugin_manager.h"
#include "common/utils/resources.h"
#include "common/utils/rt_thread_id.h"
#include "gui/backend/backend/event.h"
#include "gui/backend/backend/event_manager.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/settings/g_settings_manager.h"
#include "gui/backend/backend/settings/settings.h"
#include "gui/backend/backend/wrapped_object_with_change_signal.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/backend/gtk_widgets/file_auditioner_controls.h"
#include "gui/backend/gtk_widgets/volume.h"
#include "gui/backend/gtk_widgets/zrythm_app.h"

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
      SAMPLE_PROCESSOR->queue_file (
        *std::get<FileDescriptor *> (wrapped_obj->obj));
      break;
    case WrappedObjectType::WRAPPED_OBJECT_TYPE_CHORD_PSET:
      SAMPLE_PROCESSOR->queue_chord_preset (
        *std::get<ChordPreset *> (wrapped_obj->obj));
      break;
    default:
      break;
    }
}

static void
on_stop_clicked (GtkButton * toolbutton, FileAuditionerControlsWidget * self)
{
  SAMPLE_PROCESSOR->stop_file_playback ();
}

static void
on_settings_menu_items_changed (
  GMenuModel *                   model,
  int                            position,
  int                            removed,
  int                            added,
  FileAuditionerControlsWidget * self)
{
  self->refilter_files ();
}

static void
on_instrument_changed (GObject * gobject, GParamSpec * pspec, void * data)
{
  GtkDropDown *                   dropdown = GTK_DROP_DOWN (gobject);
  WrappedObjectWithChangeSignal * wrapped_obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (
      gtk_drop_down_get_selected_item (dropdown));
  const zrythm::plugins::PluginDescriptor * descr =
    std::get<zrythm::plugins::PluginDescriptor *> (wrapped_obj->obj);

  if (
    SAMPLE_PROCESSOR->instrument_setting_
    && SAMPLE_PROCESSOR->instrument_setting_->descr_.is_same_plugin (*descr))
    return;

  AudioEngine::State state;
  AUDIO_ENGINE->wait_for_pause (state, false, true);

  /* set setting */
  PluginSetting * existing_setting = S_PLUGIN_SETTINGS->find (*descr);
  if (existing_setting)
    {
      SAMPLE_PROCESSOR->instrument_setting_ =
        std::make_unique<PluginSetting> (*existing_setting);
      SAMPLE_PROCESSOR->instrument_setting_->validate ();
    }
  else
    {
      SAMPLE_PROCESSOR->instrument_setting_ =
        std::make_unique<PluginSetting> (*descr);
    }

  /* save setting */
  try
    {
      auto json =
        SAMPLE_PROCESSOR->instrument_setting_->serialize_to_json_string ();
      g_settings_set_string (S_UI_FILE_BROWSER, "instrument", json.c_str ());
    }
  catch (const ZrythmException &e)
    {
      e.handle ("Failed to serialize instrument setting");
    }

  AUDIO_ENGINE->resume (state);

  EVENTS_PUSH (EventType::ET_FILE_BROWSER_INSTRUMENT_CHANGED, nullptr);
}

static void
setup_instrument_dropdown (FileAuditionerControlsWidget * self)
{
  GListStore * store = g_list_store_new (WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE);

  /* populate instruments */
  int selected = -1;
  int num_added = 0;
  for (auto &descr : PLUGIN_MANAGER->plugin_descriptors_)
    {
      if (descr.is_instrument ())
        {
          WrappedObjectWithChangeSignal * wrapped_descr =
            wrapped_object_with_change_signal_new_with_free_func (
              new zrythm::plugins::PluginDescriptor (descr),
              WrappedObjectType::WRAPPED_OBJECT_TYPE_PLUGIN_DESCR,
              [] (void * ptr) {
                delete static_cast<zrythm::plugins::PluginDescriptor *> (ptr);
              });
          g_list_store_append (store, wrapped_descr);

          /* set selected instrument */
          if (
            SAMPLE_PROCESSOR->instrument_setting_
            && SAMPLE_PROCESSOR->instrument_setting_->descr_.is_same_plugin (
              descr))
            {
              selected = num_added;
            }

          num_added++;
        }
    }

  gtk_drop_down_set_model (self->instrument_dropdown, G_LIST_MODEL (store));

  GtkExpression * expr = gtk_cclosure_expression_new (
    G_TYPE_STRING, nullptr, 0, nullptr,
    G_CALLBACK (wrapped_object_with_change_signal_get_display_name), nullptr,
    nullptr);
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

  volume_widget_setup (self->volume, SAMPLE_PROCESSOR->fader_->amp_.get ());

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
