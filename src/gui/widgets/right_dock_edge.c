/*
 * Copyright (C) 2019-2022 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/control_room.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/chord_pack_browser.h"
#include "gui/widgets/foldable_notebook.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/monitor_section.h"
#include "gui/widgets/panel_file_browser.h"
#include "gui/widgets/plugin_browser.h"
#include "gui/widgets/right_dock_edge.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/resources.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  RightDockEdgeWidget,
  right_dock_edge_widget,
  GTK_TYPE_BOX)

static void
on_notebook_switch_page (
  GtkNotebook *         notebook,
  GtkWidget *           page,
  guint                 page_num,
  RightDockEdgeWidget * self)
{
  g_debug (
    "setting right dock page to %u", page_num);

  g_settings_set_int (
    S_UI, "right-panel-tab", (int) page_num);
}

void
right_dock_edge_widget_setup (
  RightDockEdgeWidget * self)
{
  foldable_notebook_widget_setup (
    self->right_notebook,
    MW_CENTER_DOCK->center_right_paned,
    GTK_POS_RIGHT, false);

  monitor_section_widget_setup (
    self->monitor_section, CONTROL_ROOM);

  GtkNotebook * notebook =
    foldable_notebook_widget_get_notebook (
      self->right_notebook);
  int page_num =
    g_settings_get_int (S_UI, "right-panel-tab");
  gtk_notebook_set_current_page (notebook, page_num);

  g_signal_connect (
    G_OBJECT (notebook), "switch-page",
    G_CALLBACK (on_notebook_switch_page), self);
}

static void
right_dock_edge_widget_init (
  RightDockEdgeWidget * self)
{
  g_type_ensure (MONITOR_SECTION_WIDGET_TYPE);

  gtk_widget_init_template (GTK_WIDGET (self));

  self->right_notebook->pos_in_paned = GTK_POS_RIGHT;

  GtkBox *      box;
  GtkNotebook * notebook =
    foldable_notebook_widget_get_notebook (
      self->right_notebook);

  /* add plugin browser */
  self->plugin_browser =
    plugin_browser_widget_new ();
  box = GTK_BOX (
    gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
  self->plugin_browser_box = box;
  gtk_box_append (
    GTK_BOX (box),
    GTK_WIDGET (self->plugin_browser));
  foldable_notebook_widget_add_page (
    self->right_notebook, GTK_WIDGET (box),
    "plugin-solid", _ ("Plugins"),
    _ ("Plugin browser"));

  /* add file browser */
  self->file_browser =
    panel_file_browser_widget_new ();
  box = GTK_BOX (
    gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
  self->file_browser_box = box;
  gtk_box_append (
    GTK_BOX (box), GTK_WIDGET (self->file_browser));
  foldable_notebook_widget_add_page (
    self->right_notebook, GTK_WIDGET (box),
    "folder-music-line", _ ("Files"),
    _ ("File browser"));

  /* add control room */
  self->monitor_section =
    monitor_section_widget_new ();
  box = GTK_BOX (
    gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
  self->monitor_section_box = box;
  gtk_box_append (
    GTK_BOX (box),
    GTK_WIDGET (self->monitor_section));
  foldable_notebook_widget_add_page (
    self->right_notebook, GTK_WIDGET (box),
    "speaker", _ ("Monitor"), _ ("Monitor section"));

  /* add chord preset browser */
  self->chord_pack_browser =
    chord_pack_browser_widget_new ();
  box = GTK_BOX (
    gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
  self->chord_pack_browser_box = box;
  gtk_box_append (
    GTK_BOX (box),
    GTK_WIDGET (self->chord_pack_browser));
  foldable_notebook_widget_add_page (
    self->right_notebook, GTK_WIDGET (box),
    "minuet-chords", _ ("Chords"),
    _ ("Chord preset browser"));

  /* add file browser button */
  GtkButton * tb = GTK_BUTTON (
    gtk_button_new_from_icon_name ("hdd"));
  gtk_widget_set_tooltip_text (
    GTK_WIDGET (tb), _ ("Show file browser"));
  gtk_actionable_set_action_name (
    GTK_ACTIONABLE (tb), "app.show-file-browser");
  gtk_notebook_set_action_widget (
    notebook, GTK_WIDGET (tb), GTK_PACK_END);

  gtk_notebook_set_current_page (notebook, 0);
}

static void
right_dock_edge_widget_class_init (
  RightDockEdgeWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "right_dock_edge.ui");

  gtk_widget_class_set_css_name (
    klass, "right-dock-edge");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, RightDockEdgeWidget, x)

  BIND_CHILD (right_notebook);
}
