// SPDX-FileCopyrightText: © 2020-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/engine.h"
#include "dsp/port.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/plugin_properties_expander.h"
#include "plugins/carla_native_plugin.h"
#include "plugins/plugin.h"
#include "plugins/plugin_gtk.h"
#include "project.h"
#include "utils/gtk.h"
#include "utils/rt_thread_id.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  PluginPropertiesExpanderWidget,
  plugin_properties_expander_widget,
  TWO_COL_EXPANDER_BOX_WIDGET_TYPE)

static void
on_bank_changed (GtkComboBox * cb, PluginPropertiesExpanderWidget * self)
{
  if (!self->plugin)
    return;

  self->plugin->set_selected_bank_from_index (gtk_combo_box_get_active (cb));

  g_signal_handler_block (self->presets, self->pset_changed_handler);
  plugin_gtk_setup_plugin_presets_list_box (self->presets, self->plugin);
  g_signal_handler_unblock (self->presets, self->pset_changed_handler);
}

static void
on_preset_changed (GtkListBox * box, PluginPropertiesExpanderWidget * self)
{
  if (!self->plugin)
    return;

  GtkListBoxRow * row = gtk_list_box_get_selected_row (box);
  if (row)
    {

      self->plugin->set_selected_preset_from_index (
        gtk_list_box_row_get_index (row));
    }
}

static void
on_save_preset_clicked (GtkButton * btn, PluginPropertiesExpanderWidget * self)
{
  if (!self->plugin || !self->plugin->instantiated_)
    return;

  plugin_gtk_on_save_preset_activate (GTK_WIDGET (self), self->plugin);
}

static void
on_load_preset_clicked (GtkButton * btn, PluginPropertiesExpanderWidget * self)
{
  if (!self->plugin || !self->plugin->instantiated_)
    return;

  const auto &setting = self->plugin->setting_;

  GtkWidget * dialog = gtk_file_chooser_dialog_new (
    _ ("Load Preset"),
    self->plugin->window_ ? self->plugin->window_ : GTK_WINDOW (MAIN_WINDOW),
    GTK_FILE_CHOOSER_ACTION_OPEN, _ ("_Cancel"), GTK_RESPONSE_REJECT,
    _ ("_Load"), GTK_RESPONSE_ACCEPT, nullptr);
  GFile * gfile = g_file_new_for_path (g_get_home_dir ());
  gtk_file_chooser_set_current_folder (
    GTK_FILE_CHOOSER (dialog), gfile, nullptr);
  g_object_unref (gfile);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);
  if (z_gtk_dialog_run (GTK_DIALOG (dialog), false) != GTK_RESPONSE_ACCEPT)
    {
      gtk_window_destroy (GTK_WINDOW (dialog));
      return;
    }

  gfile = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (dialog));
  char * _path = g_file_get_path (gfile);
  auto   path = std::string (_path);
  g_free (_path);
  g_object_unref (gfile);

  try
    {
      if (setting.open_with_carla_)
        {
#ifdef HAVE_CARLA
          auto carla = static_cast<CarlaNativePlugin *> (self->plugin);
          carla->load_state (&path);
#else
          g_return_if_reached ();
#endif
        }
      EVENTS_PUSH (EventType::ET_PLUGIN_PRESET_LOADED, self->plugin);
    }
  catch (const ZrythmException &e)
    {
      e.handle (_ ("Failed to apply preset"));
    }

  gtk_window_destroy (GTK_WINDOW (dialog));
}

/**
 * Refreshes each field.
 */
void
plugin_properties_expander_widget_refresh (
  PluginPropertiesExpanderWidget * self,
  Plugin *                         pl)
{
  if (self->plugin == pl)
    return;

  self->plugin = pl;

  if (pl)
    {
      gtk_label_set_text (self->name, pl->get_name ().c_str ());
      gtk_label_set_text (
        self->type, pl->get_descriptor ().category_str_.c_str ());
    }
  else
    {
      gtk_label_set_text (self->name, _ ("None"));
      gtk_label_set_text (self->type, _ ("None"));
    }

  g_signal_handler_block (self->banks, self->bank_changed_handler);
  plugin_gtk_setup_plugin_banks_combo_box (self->banks, pl);
  g_signal_handler_unblock (self->banks, self->bank_changed_handler);
  g_signal_handler_block (self->presets, self->pset_changed_handler);
  plugin_gtk_setup_plugin_presets_list_box (self->presets, pl);
  g_signal_handler_unblock (self->presets, self->pset_changed_handler);
}

/**
 * Sets up the PluginPropertiesExpanderWidget for a Plugin.
 */
void
plugin_properties_expander_widget_setup (
  PluginPropertiesExpanderWidget * self,
  Plugin *                         pl)
{
  self->plugin = pl;

  /* set name and icon */
  expander_box_widget_set_label (
    Z_EXPANDER_BOX_WIDGET (self), _ ("Plugin Properties"));
  expander_box_widget_set_icon_name (
    Z_EXPANDER_BOX_WIDGET (self),
    "gnome-icon-library-general-properties-symbolic");

  GtkWidget * lbl;

#define CREATE_LABEL(x) \
  lbl = plugin_gtk_new_label (x, true, false, 0.f, 0.5f); \
  gtk_widget_add_css_class (lbl, "inspector_label"); \
  gtk_widget_set_margin_start (lbl, 2); \
  gtk_widget_set_visible (lbl, 1)

  CREATE_LABEL (_ ("Name"));
  gtk_widget_set_visible (lbl, TRUE);
  two_col_expander_box_widget_add_single (
    Z_TWO_COL_EXPANDER_BOX_WIDGET (self), lbl);
  self->name = GTK_LABEL (gtk_label_new ("dist"));
  gtk_widget_set_visible (GTK_WIDGET (self->name), TRUE);
  gtk_label_set_xalign (self->name, 0);
  gtk_widget_set_margin_start (GTK_WIDGET (self->name), 4);
  two_col_expander_box_widget_add_single (
    Z_TWO_COL_EXPANDER_BOX_WIDGET (self), GTK_WIDGET (self->name));

  CREATE_LABEL (_ ("Type"));
  gtk_widget_set_visible (lbl, TRUE);
  two_col_expander_box_widget_add_single (
    Z_TWO_COL_EXPANDER_BOX_WIDGET (self), lbl);
  self->type = GTK_LABEL (gtk_label_new ("Instrument"));
  gtk_widget_set_visible (GTK_WIDGET (self->type), TRUE);
  gtk_label_set_xalign (self->type, 0);
  gtk_widget_set_margin_start (GTK_WIDGET (self->type), 4);
  two_col_expander_box_widget_add_single (
    Z_TWO_COL_EXPANDER_BOX_WIDGET (self), GTK_WIDGET (self->type));

  CREATE_LABEL (_ ("Banks"));
  gtk_widget_set_visible (lbl, true);
  two_col_expander_box_widget_add_single (
    Z_TWO_COL_EXPANDER_BOX_WIDGET (self), lbl);
  self->banks = GTK_COMBO_BOX_TEXT (gtk_combo_box_text_new ());
  gtk_widget_set_visible (GTK_WIDGET (self->banks), true);
  two_col_expander_box_widget_add_single (
    Z_TWO_COL_EXPANDER_BOX_WIDGET (self), GTK_WIDGET (self->banks));
  self->bank_changed_handler = g_signal_connect (
    G_OBJECT (self->banks), "changed", G_CALLBACK (on_bank_changed), self);

  CREATE_LABEL (_ ("Presets"));
  gtk_widget_set_visible (lbl, true);
  two_col_expander_box_widget_add_single (
    Z_TWO_COL_EXPANDER_BOX_WIDGET (self), lbl);
  self->presets = GTK_LIST_BOX (gtk_list_box_new ());
  gtk_widget_set_visible (GTK_WIDGET (self->presets), true);
  GtkScrolledWindow * scroll = GTK_SCROLLED_WINDOW (gtk_scrolled_window_new ());
  gtk_scrolled_window_set_min_content_height (scroll, 86);
  gtk_scrolled_window_set_policy (
    scroll, GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_visible (GTK_WIDGET (scroll), true);
  gtk_scrolled_window_set_child (scroll, GTK_WIDGET (self->presets));
  two_col_expander_box_widget_add_single (
    Z_TWO_COL_EXPANDER_BOX_WIDGET (self), GTK_WIDGET (scroll));
  self->pset_changed_handler = g_signal_connect (
    G_OBJECT (self->presets), "selected-rows-changed",
    G_CALLBACK (on_preset_changed), self);

  self->save_preset_btn = z_gtk_button_new_with_icon_and_text (
    "document-save-as", _ ("Save"), true, GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_widget_set_tooltip_text (
    GTK_WIDGET (self->save_preset_btn), _ ("Save preset"));
  self->load_preset_btn = z_gtk_button_new_with_icon_and_text (
    "document-open", _ ("Load"), true, GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_widget_set_tooltip_text (
    GTK_WIDGET (self->load_preset_btn), _ ("Load preset"));
  GtkWidget * box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_widget_set_visible (box, true);
  gtk_box_append (GTK_BOX (box), GTK_WIDGET (self->save_preset_btn));
  gtk_box_append (GTK_BOX (box), GTK_WIDGET (self->load_preset_btn));
  gtk_widget_add_css_class (box, "linked");
  two_col_expander_box_widget_add_single (
    Z_TWO_COL_EXPANDER_BOX_WIDGET (self), box);
  g_signal_connect (
    G_OBJECT (self->save_preset_btn), "clicked",
    G_CALLBACK (on_save_preset_clicked), self);
  g_signal_connect (
    G_OBJECT (self->load_preset_btn), "clicked",
    G_CALLBACK (on_load_preset_clicked), self);

  plugin_properties_expander_widget_refresh (self, pl);
}

static void
plugin_properties_expander_widget_class_init (
  PluginPropertiesExpanderWidgetClass * klass)
{
}

static void
plugin_properties_expander_widget_init (PluginPropertiesExpanderWidget * self)
{
  /*two_col_expander_box_widget_add_single (*/
  /*Z_TWO_COL_EXPANDER_BOX_WIDGET (self),*/
  /*GTK_WIDGET (self->name));*/
}
