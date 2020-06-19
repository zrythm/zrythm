/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/engine.h"
#include "audio/port.h"
#include "audio/track.h"
#include "gui/widgets/editable_label.h"
#include "gui/widgets/inspector_port.h"
#include "gui/widgets/plugin_properties_expander.h"
#include "plugins/plugin.h"
#include "plugins/plugin_gtk.h"
#include "project.h"
#include "utils/gtk.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  PluginPropertiesExpanderWidget,
  plugin_properties_expander_widget,
  TWO_COL_EXPANDER_BOX_WIDGET_TYPE)

static void
on_bank_changed (
  GtkComboBox * cb,
  PluginPropertiesExpanderWidget * self)
{
  if (!self->plugin)
    return;

  plugin_set_selected_bank_from_index (
    self->plugin, gtk_combo_box_get_active (cb));

  g_signal_handler_block (
    self->presets, self->pset_changed_handler);
  plugin_gtk_setup_plugin_presets_combo_box (
    self->presets, self->plugin);
  g_signal_handler_unblock (
    self->presets, self->pset_changed_handler);
}

static void
on_preset_changed (
  GtkComboBox * cb,
  PluginPropertiesExpanderWidget * self)
{
  if (!self->plugin)
    return;

  plugin_set_selected_preset_from_index (
    self->plugin, gtk_combo_box_get_active (cb));
}

/**
 * Refreshes each field.
 */
void
plugin_properties_expander_widget_refresh (
  PluginPropertiesExpanderWidget * self,
  Plugin *                         pl)
{
  self->plugin = pl;

  if (pl)
    {
      gtk_label_set_text (
        self->name, pl->descr->name);
      gtk_label_set_text (
        self->type, pl->descr->category_str);
    }
  else
    {
      gtk_label_set_text (
        self->name, _("None"));
      gtk_label_set_text (
        self->type, _("None"));
    }

  g_signal_handler_block (
    self->banks, self->bank_changed_handler);
  plugin_gtk_setup_plugin_banks_combo_box (
    self->banks, pl);
  g_signal_handler_unblock (
    self->banks, self->bank_changed_handler);
  g_signal_handler_block (
    self->presets, self->pset_changed_handler);
  plugin_gtk_setup_plugin_presets_combo_box (
    self->presets, pl);
  g_signal_handler_unblock (
    self->presets, self->pset_changed_handler);
}

/**
 * Sets up the PluginPropertiesExpanderWidget for a Plugin.
 */
void
plugin_properties_expander_widget_setup (
  PluginPropertiesExpanderWidget * self,
  Plugin *      pl)
{
  self->plugin = pl;

  /* set name and icon */
  expander_box_widget_set_label (
    Z_EXPANDER_BOX_WIDGET (self),
    _("Plugin Properties"));

  GtkWidget * lbl;

#define CREATE_LABEL(x) \
  lbl = \
    plugin_gtk_new_label ( \
      x, true, false, 0.f, 0.5f); \
  z_gtk_widget_add_style_class ( \
    lbl, "inspector_label"); \
  gtk_widget_set_margin_start (lbl, 2); \
  gtk_widget_set_visible (lbl, 1)

  CREATE_LABEL (_("Name"));
  gtk_widget_set_visible (lbl, TRUE);
  two_col_expander_box_widget_add_single (
    Z_TWO_COL_EXPANDER_BOX_WIDGET (self), lbl);
  self->name =
    GTK_LABEL (gtk_label_new ("dist"));
  gtk_widget_set_visible (
    GTK_WIDGET (self->name), TRUE);
  gtk_label_set_xalign (
    self->name, 0);
  gtk_widget_set_margin_start (
    GTK_WIDGET (self->name), 4);
  two_col_expander_box_widget_add_single (
    Z_TWO_COL_EXPANDER_BOX_WIDGET (self),
    GTK_WIDGET (self->name));

  CREATE_LABEL (_("Type"));
  gtk_widget_set_visible (lbl, TRUE);
  two_col_expander_box_widget_add_single (
    Z_TWO_COL_EXPANDER_BOX_WIDGET (self), lbl);
  self->type =
    GTK_LABEL (gtk_label_new ("Instrument"));
  gtk_widget_set_visible (
    GTK_WIDGET (self->type), TRUE);
  gtk_label_set_xalign (
    self->type, 0);
  gtk_widget_set_margin_start (
    GTK_WIDGET (self->type), 4);
  two_col_expander_box_widget_add_single (
    Z_TWO_COL_EXPANDER_BOX_WIDGET (self),
    GTK_WIDGET (self->type));

  CREATE_LABEL (_("Banks"));
  gtk_widget_set_visible (lbl, TRUE);
  two_col_expander_box_widget_add_single (
    Z_TWO_COL_EXPANDER_BOX_WIDGET (self), lbl);
  self->banks =
    GTK_COMBO_BOX_TEXT (gtk_combo_box_text_new ());
  gtk_widget_set_visible (
    GTK_WIDGET (self->banks), TRUE);
  two_col_expander_box_widget_add_single (
    Z_TWO_COL_EXPANDER_BOX_WIDGET (self),
    GTK_WIDGET (self->banks));
  self->bank_changed_handler =
    g_signal_connect (
      G_OBJECT (self->banks), "changed",
      G_CALLBACK (on_bank_changed), self);

  CREATE_LABEL (_("Presets"));
  gtk_widget_set_visible (lbl, TRUE);
  two_col_expander_box_widget_add_single (
    Z_TWO_COL_EXPANDER_BOX_WIDGET (self), lbl);
  self->presets =
    GTK_COMBO_BOX_TEXT (gtk_combo_box_text_new ());
  gtk_widget_set_visible (
    GTK_WIDGET (self->presets), TRUE);
  two_col_expander_box_widget_add_single (
    Z_TWO_COL_EXPANDER_BOX_WIDGET (self),
    GTK_WIDGET (self->presets));
  self->pset_changed_handler =
    g_signal_connect (
      G_OBJECT (self->presets), "changed",
      G_CALLBACK (on_preset_changed), self);

  plugin_properties_expander_widget_refresh (
    self, pl);
}

static void
plugin_properties_expander_widget_class_init (
  PluginPropertiesExpanderWidgetClass * klass)
{
}

static void
plugin_properties_expander_widget_init (
  PluginPropertiesExpanderWidget * self)
{
  /*two_col_expander_box_widget_add_single (*/
    /*Z_TWO_COL_EXPANDER_BOX_WIDGET (self),*/
    /*GTK_WIDGET (self->name));*/
}
