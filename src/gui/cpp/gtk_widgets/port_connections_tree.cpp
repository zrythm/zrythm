// SPDX-FileCopyrightText: © 2018-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/cpp/backend/actions/port_connection_action.h"
#include "gui/cpp/backend/project.h"
#include "gui/cpp/backend/zrythm.h"
#include "gui/cpp/gtk_widgets/gtk_wrapper.h"
#include "gui/cpp/gtk_widgets/port_connections_tree.h"
#include "gui/cpp/gtk_widgets/zrythm_app.h"

#include <glib/gi18n.h>

#include "common/dsp/port_connections_manager.h"
#include "common/utils/gtk.h"
#include "common/utils/ui.h"

G_DEFINE_TYPE (
  PortConnectionsTreeWidget,
  port_connections_tree_widget,
  GTK_TYPE_BOX)

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
  GtkCellRendererToggle *     cell,
  gchar *                     path_str,
  PortConnectionsTreeWidget * self)
{
  GtkTreeModel * model = self->tree_model;
  GtkTreeIter    iter;
  GtkTreePath *  path = gtk_tree_path_new_from_string (path_str);

  /* get toggled iter */
  gtk_tree_model_get_iter (model, &iter, path);

  /* get ports and toggled */
  gboolean enabled;
  Port *   src_port = nullptr, *dest_port = NULL;
  gtk_tree_model_get (
    model, &iter, COL_ENABLED, &enabled, COL_SRC_PORT, &src_port, COL_DEST_PORT,
    &dest_port, -1);

  /* get new value */
  enabled ^= 1;

  /* set new value on widget */
  gtk_list_store_set (GTK_LIST_STORE (model), &iter, COL_ENABLED, enabled, -1);

  /* clean up */
  gtk_tree_path_free (path);

  /* perform an undoable action */
  try
    {
      UNDO_MANAGER->perform (std::make_unique<PortConnectionEnableAction> (
        src_port->id_, dest_port->id_));
    }
  catch (const ZrythmException &e)
    {
      e.handle (format_str (
        _ ("Failed to enable connection from {} to {}"), src_port->get_label (),
        dest_port->get_label ()));
    }
}

static GtkTreeModel *
create_model (void)
{
  GtkListStore * store;
  GtkTreeIter    iter;

  /* create list store */
  store = gtk_list_store_new (
    NUM_COLS, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
    G_TYPE_POINTER, G_TYPE_POINTER);

  /* add data to the list store */
  for (auto &conn : PORT_CONNECTIONS_MGR->connections_)
    {
      /* skip locked connections */
      if (conn.locked_)
        continue;

      Port * src_port = Port::find_from_identifier (conn.src_id_);
      Port * dest_port = Port::find_from_identifier (conn.dest_id_);
      z_return_val_if_fail (src_port && dest_port, nullptr);

      auto src_path = src_port->get_full_designation ();
      auto dest_path = dest_port->get_full_designation ();

      /* get multiplier */
      auto mult_str = fmt::format ("{:.4f}", conn.multiplier_);

      gtk_list_store_append (store, &iter);
      gtk_list_store_set (
        store, &iter, COL_ENABLED, conn.enabled_, COL_SRC_PATH,
        src_path.c_str (), COL_DEST_PATH, dest_path.c_str (), COL_MULTIPLIER,
        mult_str.c_str (), COL_SRC_PORT, src_port, COL_DEST_PORT, dest_port, -1);
    }

  return GTK_TREE_MODEL (store);
}

static void
show_context_menu (PortConnectionsTreeWidget * self, double x, double y)
{
  GMenu *     menu = g_menu_new ();
  GMenuItem * menuitem;

  menuitem = z_gtk_create_menu_item (
    _ ("Delete"), nullptr, "app.port-connection-remove");
  g_menu_append_item (menu, menuitem);

  z_gtk_show_context_menu_from_g_menu (self->popover_menu, x, y, menu);
}

static void
on_right_click (
  GtkGestureClick *           gesture,
  gint                        n_press,
  gdouble                     x_dbl,
  gdouble                     y_dbl,
  PortConnectionsTreeWidget * self)
{
  if (n_press != 1)
    return;

  z_info ("right click");

  GtkTreePath *       path;
  GtkTreeViewColumn * column;

  int x, y;
  gtk_tree_view_convert_widget_to_bin_window_coords (
    self->tree, (int) x_dbl, (int) y_dbl, &x, &y);

  GtkTreeSelection * selection = gtk_tree_view_get_selection (self->tree);
  if (!gtk_tree_view_get_path_at_pos (
        GTK_TREE_VIEW (self->tree), x, y, &path, &column, nullptr, nullptr))
    {
      z_info ("no path at position {} {}", x, y);
      return;
    }

  gtk_tree_selection_unselect_all (selection);
  gtk_tree_selection_select_path (selection, path);
  GtkTreeIter iter;
  gtk_tree_model_get_iter (GTK_TREE_MODEL (self->tree_model), &iter, path);
  GValue init_value = G_VALUE_INIT;
  GValue value = init_value;
  gtk_tree_model_get_value (
    GTK_TREE_MODEL (self->tree_model), &iter, COL_SRC_PORT, &value);
  self->src_port = static_cast<Port *> (g_value_get_pointer (&value));
  value = init_value;
  gtk_tree_model_get_value (
    GTK_TREE_MODEL (self->tree_model), &iter, COL_DEST_PORT, &value);
  self->dest_port = static_cast<Port *> (g_value_get_pointer (&value));

  gtk_tree_path_free (path);

  show_context_menu (self, x, y);
}

static void
tree_view_setup (PortConnectionsTreeWidget * self, GtkTreeView * tree_view)
{
  /* init tree view */
  GtkCellRenderer *   renderer;
  GtkTreeViewColumn * column;

  /* column for checkbox */
  renderer = gtk_cell_renderer_toggle_new ();
  column = gtk_tree_view_column_new_with_attributes (
    _ ("On"), renderer, "active", COL_ENABLED, nullptr);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);
  g_signal_connect (renderer, "toggled", G_CALLBACK (on_enabled_toggled), self);

  /* column for src path */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
    _ ("Source"), renderer, "text", COL_SRC_PATH, nullptr);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);
  g_object_set (renderer, "ellipsize", PANGO_ELLIPSIZE_END, nullptr);
  gtk_tree_view_column_set_resizable (GTK_TREE_VIEW_COLUMN (column), true);
  gtk_tree_view_column_set_min_width (GTK_TREE_VIEW_COLUMN (column), 120);
  gtk_tree_view_column_set_expand (GTK_TREE_VIEW_COLUMN (column), true);

  /* column for src path */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
    _ ("Destination"), renderer, "text", COL_DEST_PATH, nullptr);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);
  g_object_set (renderer, "ellipsize", PANGO_ELLIPSIZE_END, nullptr);
  gtk_tree_view_column_set_resizable (GTK_TREE_VIEW_COLUMN (column), true);
  gtk_tree_view_column_set_min_width (GTK_TREE_VIEW_COLUMN (column), 120);
  gtk_tree_view_column_set_expand (GTK_TREE_VIEW_COLUMN (column), true);

  /* column for multiplier */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
    _ ("Multiplier"), renderer, "text", COL_MULTIPLIER, nullptr);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);

  /* connect right click handler */
  GtkGestureClick * mp = GTK_GESTURE_CLICK (gtk_gesture_click_new ());
  gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (mp), GDK_BUTTON_SECONDARY);
  g_signal_connect (G_OBJECT (mp), "pressed", G_CALLBACK (on_right_click), self);
  gtk_widget_add_controller (GTK_WIDGET (tree_view), GTK_EVENT_CONTROLLER (mp));
}

/**
 * Refreshes the tree model.
 */
void
port_connections_tree_widget_refresh (PortConnectionsTreeWidget * self)
{
  GtkTreeModel * model = self->tree_model;
  self->tree_model = create_model ();
  gtk_tree_view_set_model (self->tree, self->tree_model);

  if (model)
    {
      g_object_unref (model);
    }
}

PortConnectionsTreeWidget *
port_connections_tree_widget_new (void)
{
  PortConnectionsTreeWidget * self = Z_PORT_CONNECTIONS_TREE_WIDGET (
    g_object_new (PORT_CONNECTIONS_TREE_WIDGET_TYPE, nullptr));

  /* setup tree */
  tree_view_setup (self, self->tree);

  return self;
}

static void
port_connections_tree_widget_class_init (PortConnectionsTreeWidgetClass * _klass)
{
}

static void
port_connections_tree_widget_init (PortConnectionsTreeWidget * self)
{
  self->scroll = GTK_SCROLLED_WINDOW (gtk_scrolled_window_new ());
  gtk_box_append (GTK_BOX (self), GTK_WIDGET (self->scroll));
  gtk_widget_set_hexpand (GTK_WIDGET (self->scroll), true);

  self->tree = GTK_TREE_VIEW (gtk_tree_view_new ());
  gtk_scrolled_window_set_child (self->scroll, GTK_WIDGET (self->tree));

  self->popover_menu =
    GTK_POPOVER_MENU (gtk_popover_menu_new_from_model (nullptr));
  gtk_box_append (GTK_BOX (self), GTK_WIDGET (self->popover_menu));
}
