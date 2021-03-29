/*
 * Copyright (C) 2018-2020 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version approved by
 * Alexandros Theodotou in a public statement of acceptance.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "actions/port_connection_action.h"
#include "gui/widgets/port_connections_tree.h"
#include "project.h"
#include "utils/gtk.h"
#include "utils/objects.h"
#include "utils/resources.h"
#include "utils/string.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

G_DEFINE_TYPE (
  PortConnectionsTreeWidget,
  port_connections_tree_widget,
  GTK_TYPE_SCROLLED_WINDOW)

enum
{
  COL_ENABLED,
  COL_SRC_PATH,
  COL_DEST_PATH,
  COL_MULTIPLIER,

  COL_SRC_PORT,
  COL_DEST_PORT,

  NUM_COLS
};

static void
on_enabled_toggled (
  GtkCellRendererToggle * cell,
  gchar                 * path_str,
  PortConnectionsTreeWidget *  self)
{
  GtkTreeModel * model = self->tree_model;
  GtkTreeIter  iter;
  GtkTreePath *path =
    gtk_tree_path_new_from_string (path_str);

  /* get toggled iter */
  gtk_tree_model_get_iter (model, &iter, path);

  /* get ports and toggled */
  gboolean enabled;
  Port * src_port = NULL, * dest_port = NULL;
  gtk_tree_model_get (
    model, &iter,
    COL_ENABLED, &enabled,
    COL_SRC_PORT, &src_port,
    COL_DEST_PORT, &dest_port,
    -1);

  /* get new value */
  enabled ^= 1;

  /* set new value on widget */
  gtk_list_store_set (
    GTK_LIST_STORE (model), &iter,
    COL_ENABLED, enabled, -1);

  /* clean up */
  gtk_tree_path_free (path);

  /* perform an undoable action */
  UndoableAction * ua =
    port_connection_action_new_enable (
      &src_port->id, &dest_port->id, enabled);
  undo_manager_perform (UNDO_MANAGER, ua);
}

static GtkTreeModel *
create_model ()
{
  GtkListStore *store;
  GtkTreeIter iter;

  /* create list store */
  store =
    gtk_list_store_new (
      NUM_COLS,
      G_TYPE_BOOLEAN,
      G_TYPE_STRING,
      G_TYPE_STRING,
      G_TYPE_STRING,
      G_TYPE_POINTER,
      G_TYPE_POINTER);

  /* add data to the list store */
  size_t max_size = 20;
  Port ** ports =
    object_new_n (max_size, Port *);
  int num_ports = 0;
  Port * port;
  port_get_all (
    &ports, &max_size, true, &num_ports);
  for (int i = 0; i < num_ports; i++)
    {
      port = ports[i];

      char path[600];
      port_get_full_designation (port, path);

      for (int j = 0; j < port->num_dests; j++)
        {
          /* skip locked connections */
          if (port->dest_locked[j])
            continue;

          Port * dest = port->dests[j];

          /* get destination */
          char dest_path[600];
          port_get_full_designation (
            dest, dest_path);

          /* get multiplier */
          char mult_str[40];
          sprintf (
            mult_str, "%.4f",
            (double)
            port_get_multiplier (port, dest));

          gtk_list_store_append (
            store, &iter);
          gtk_list_store_set (
            store, &iter,
            COL_ENABLED,
            port_get_enabled (port, dest),
            COL_SRC_PATH, path,
            COL_DEST_PATH, dest_path,
            COL_MULTIPLIER, mult_str,
            COL_SRC_PORT, port,
            COL_DEST_PORT, dest,
            -1);
        }
    }
  free (ports);

  return GTK_TREE_MODEL (store);
}

static void
on_delete_activate (
  GtkMenuItem *               menuitem,
  PortConnectionsTreeWidget * self)
{
  UndoableAction * ua =
    port_connection_action_new_disconnect (
      &self->src_port->id, &self->dest_port->id);
  undo_manager_perform (UNDO_MANAGER, ua);
}

static void
show_context_menu (
  PortConnectionsTreeWidget * self)
{
  GtkWidget * menuitem;
  GtkWidget * menu = gtk_menu_new ();

  menuitem =
    gtk_menu_item_new_with_label (_("Delete"));
  gtk_widget_set_visible (
    GTK_WIDGET (menuitem), true);
  gtk_menu_shell_append (
    GTK_MENU_SHELL (menu), GTK_WIDGET (menuitem));
  g_signal_connect_data (
    G_OBJECT (menuitem), "activate",
    G_CALLBACK (on_delete_activate), self, NULL, 0);

  gtk_menu_attach_to_widget (
    GTK_MENU (menu),
    GTK_WIDGET (self), NULL);
  gtk_menu_popup_at_pointer (GTK_MENU (menu), NULL);
}

static void
on_right_click (
  GtkGestureMultiPress * gesture,
  gint                   n_press,
  gdouble                x_dbl,
  gdouble                y_dbl,
  PortConnectionsTreeWidget * self)
{
  if (n_press != 1)
    return;

  g_message ("right click");

  GtkTreePath *path;
  GtkTreeViewColumn *column;

  int x, y;
  gtk_tree_view_convert_widget_to_bin_window_coords (
    self->tree, (int) x_dbl, (int) y_dbl, &x, &y);

  GtkTreeSelection * selection =
    gtk_tree_view_get_selection (self->tree);
  if (!gtk_tree_view_get_path_at_pos (
        GTK_TREE_VIEW (self->tree), x, y,
        &path, &column, NULL, NULL))
    {
      g_message ("no path at position %d %d", x, y);
      return;
    }

  gtk_tree_selection_unselect_all (selection);
  gtk_tree_selection_select_path (selection, path);
  GtkTreeIter iter;
  gtk_tree_model_get_iter (
    GTK_TREE_MODEL (self->tree_model),
    &iter, path);
  GValue init_value = G_VALUE_INIT;
  GValue value = init_value;
  gtk_tree_model_get_value (
    GTK_TREE_MODEL (self->tree_model),
    &iter, COL_SRC_PORT, &value);
  self->src_port = g_value_get_pointer (&value);
  value = init_value;
  gtk_tree_model_get_value (
    GTK_TREE_MODEL (self->tree_model),
    &iter, COL_DEST_PORT, &value);
  self->dest_port = g_value_get_pointer (&value);

  gtk_tree_path_free (path);

  show_context_menu (self);
}

static void
tree_view_setup (
  PortConnectionsTreeWidget * self,
  GtkTreeView *         tree_view)
{
  /* init tree view */
  GtkCellRenderer * renderer;
  GtkTreeViewColumn * column;

  /* column for checkbox */
  renderer = gtk_cell_renderer_toggle_new ();
  column =
    gtk_tree_view_column_new_with_attributes (
      _("On"), renderer,
      "active", COL_ENABLED,
      NULL);
  gtk_tree_view_append_column (
    GTK_TREE_VIEW (tree_view), column);
  g_signal_connect (
    renderer, "toggled",
    G_CALLBACK (on_enabled_toggled), self);

  /* column for src path */
  renderer = gtk_cell_renderer_text_new ();
  column =
    gtk_tree_view_column_new_with_attributes (
      _("Source"), renderer,
      "text", COL_SRC_PATH,
      NULL);
  gtk_tree_view_append_column (
    GTK_TREE_VIEW (tree_view), column);
  g_object_set (
    renderer, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  gtk_tree_view_column_set_resizable (
    GTK_TREE_VIEW_COLUMN (column), true);
  gtk_tree_view_column_set_min_width (
    GTK_TREE_VIEW_COLUMN (column), 120);
  gtk_tree_view_column_set_expand (
    GTK_TREE_VIEW_COLUMN (column), true);

  /* column for src path */
  renderer = gtk_cell_renderer_text_new ();
  column =
    gtk_tree_view_column_new_with_attributes (
      _("Destination"), renderer,
      "text", COL_DEST_PATH,
      NULL);
  gtk_tree_view_append_column (
    GTK_TREE_VIEW (tree_view), column);
  g_object_set (
    renderer, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  gtk_tree_view_column_set_resizable (
    GTK_TREE_VIEW_COLUMN (column), true);
  gtk_tree_view_column_set_min_width (
    GTK_TREE_VIEW_COLUMN (column), 120);
  gtk_tree_view_column_set_expand (
    GTK_TREE_VIEW_COLUMN (column), true);

  /* column for multiplier */
  renderer = gtk_cell_renderer_text_new ();
  column =
    gtk_tree_view_column_new_with_attributes (
      _("Multiplier"), renderer,
      "text", COL_MULTIPLIER,
      NULL);
  gtk_tree_view_append_column (
    GTK_TREE_VIEW (tree_view), column);

  /* connect right click handler */
  GtkGestureMultiPress * mp =
    GTK_GESTURE_MULTI_PRESS (
      gtk_gesture_multi_press_new (
        GTK_WIDGET (tree_view)));
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (mp),
    GDK_BUTTON_SECONDARY);
  g_signal_connect (
    G_OBJECT (mp), "pressed",
    G_CALLBACK (on_right_click), self);
}

/**
 * Refreshes the tree model.
 */
void
port_connections_tree_widget_refresh (
  PortConnectionsTreeWidget * self)
{
  GtkTreeModel * model = self->tree_model;
  self->tree_model = create_model ();
  gtk_tree_view_set_model (
    self->tree, self->tree_model);

  if (model)
    {
      g_object_unref (model);
    }
}

PortConnectionsTreeWidget *
port_connections_tree_widget_new ()
{
  PortConnectionsTreeWidget * self =
    g_object_new (
      PORT_CONNECTIONS_TREE_WIDGET_TYPE, NULL);

  /* setup tree */
  tree_view_setup (self, self->tree);

  return self;
}

static void
port_connections_tree_widget_class_init (
  PortConnectionsTreeWidgetClass * _klass)
{
}

static void
port_connections_tree_widget_init (
  PortConnectionsTreeWidget * self)
{
  self->tree =
    GTK_TREE_VIEW (gtk_tree_view_new ());
  gtk_widget_set_visible (
    GTK_WIDGET (self->tree), 1);
  gtk_container_add (
    GTK_CONTAINER (self),
    GTK_WIDGET (self->tree));
}
