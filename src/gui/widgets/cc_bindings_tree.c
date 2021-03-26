/*
 * Copyright (C) 2018-2021 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "actions/midi_mapping_action.h"
#include "gui/widgets/cc_bindings_tree.h"
#include "project.h"
#include "utils/gtk.h"
#include "utils/resources.h"
#include "utils/string.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

G_DEFINE_TYPE (
  CcBindingsTreeWidget,
  cc_bindings_tree_widget,
  GTK_TYPE_SCROLLED_WINDOW)

enum
{
  COL_ENABLED,
  COL_CONTROL,
  COL_PATH,

  /* not an actual column, index of the mapping */
  COL_MIDI_MAPPING,

  /* TODO add the following in the future */
#if 0
  COL_DEVICE,
  COL_MIN,
  COL_MAX,
#endif

  NUM_COLS
};

static void
on_enabled_toggled (
  GtkCellRendererToggle * cell,
  gchar                 * path_str,
  CcBindingsTreeWidget *  self)
{
  GtkTreeModel * model = self->tree_model;
  GtkTreeIter  iter;
  GtkTreePath *path =
    gtk_tree_path_new_from_string (path_str);
  gboolean enabled;

  /* get toggled iter */
  gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_model_get (
    model, &iter, COL_ENABLED, &enabled, -1);

  /* get binding */
  int mapping_idx;
  gtk_tree_model_get (
    model, &iter, COL_MIDI_MAPPING, &mapping_idx, -1);

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
    midi_mapping_action_new_enable (
      mapping_idx, enabled);
  undo_manager_perform (UNDO_MANAGER, ua);
}

static GtkTreeModel *
create_model ()
{
  gint i = 0;
  GtkListStore *store;
  GtkTreeIter iter;

  /* create list store */
  store =
    gtk_list_store_new (
      NUM_COLS,
      G_TYPE_BOOLEAN,
      G_TYPE_STRING,
      G_TYPE_STRING,
      G_TYPE_INT);

  /* add data to the list store */
  for (i = 0; i < MIDI_MAPPINGS->num_mappings; i++)
    {
      MidiMapping * mm = &MIDI_MAPPINGS->mappings[i];

      /* get control */
      char ctrl_str[80];
      char ctrl[60];
      int ctrl_change_ch =
        midi_ctrl_change_get_ch_and_description (
          mm->key, ctrl);
      if (ctrl_change_ch > 0)
        {
          sprintf (
            ctrl_str, "%02X-%02X (%s)",
            mm->key[0], mm->key[1], ctrl);
        }
      else
        {
          sprintf (
            ctrl_str, "%02X-%02X",
            mm->key[0], mm->key[1]);
        }

      /* get destination */
      char path[600];
      Port * port =
        port_find_from_identifier (&mm->dest_id);
      port_get_full_designation (port, path);

      char min_str[40], max_str[40];
      sprintf (min_str, "%.4f", (double) port->minf);
      sprintf (max_str, "%.4f", (double) port->maxf);

      gtk_list_store_append (
        store, &iter);
      gtk_list_store_set (
        store, &iter,
        COL_ENABLED, mm->enabled,
        COL_CONTROL, ctrl_str,
        COL_PATH, path,
        COL_MIDI_MAPPING, i,
        -1);
    }

  return GTK_TREE_MODEL (store);
}

static void
on_delete_activate (
  GtkMenuItem * menuitem,
  MidiMapping * mapping)
{
  /* get index */
  int idx =
    midi_mapping_get_index (MIDI_MAPPINGS, mapping);
  UndoableAction * ua =
    midi_mapping_action_new_unbind (idx);
  undo_manager_perform (UNDO_MANAGER, ua);
}

static void
show_context_menu (
  CcBindingsTreeWidget * self,
  int                    binding_idx)
{
  GtkWidget * menuitem;
  GtkWidget * menu = gtk_menu_new ();

  MidiMapping * mapping =
    &MIDI_MAPPINGS->mappings[binding_idx];

  menuitem =
    gtk_menu_item_new_with_label (_("Delete"));
  gtk_widget_set_visible (
    GTK_WIDGET (menuitem), true);
  gtk_menu_shell_append (
    GTK_MENU_SHELL (menu), GTK_WIDGET (menuitem));
  g_signal_connect_data (
    G_OBJECT (menuitem), "activate",
    G_CALLBACK (on_delete_activate), mapping, NULL, 0);

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
  CcBindingsTreeWidget * self)
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
      // if we can't find path at pos, we surely don't
      // want to pop up the menu
      return;
    }

  gtk_tree_selection_unselect_all (selection);
  gtk_tree_selection_select_path (selection, path);
  GtkTreeIter iter;
  gtk_tree_model_get_iter (
    GTK_TREE_MODEL (self->tree_model),
    &iter, path);
  GValue value = G_VALUE_INIT;
  gtk_tree_model_get_value (
    GTK_TREE_MODEL (self->tree_model),
    &iter, COL_MIDI_MAPPING, &value);
  gtk_tree_path_free (path);

  int int_val = g_value_get_int (&value);

  show_context_menu (self, int_val);
}

static void
tree_view_setup (
  CcBindingsTreeWidget * self,
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

#if 0
  /* column for device */
  renderer = gtk_cell_renderer_text_new ();
  column =
    gtk_tree_view_column_new_with_attributes (
      _("Device"), renderer,
      "text", COL_DEVICE,
      NULL);
  gtk_tree_view_append_column (
    GTK_TREE_VIEW (tree_view), column);
#endif

  /* column for control */
  renderer = gtk_cell_renderer_text_new ();
  column =
    gtk_tree_view_column_new_with_attributes (
      _("Note/Control"), renderer,
      "text", COL_CONTROL,
      NULL);
  gtk_tree_view_append_column (
    GTK_TREE_VIEW (tree_view), column);
  g_object_set (
    renderer, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  gtk_tree_view_column_set_resizable (
    GTK_TREE_VIEW_COLUMN (column), true);
  gtk_tree_view_column_set_min_width (
    GTK_TREE_VIEW_COLUMN (column), 120);

  /* column for path */
  renderer = gtk_cell_renderer_text_new ();
  column =
    gtk_tree_view_column_new_with_attributes (
      _("Destination"), renderer,
      "text", COL_PATH,
      NULL);
  gtk_tree_view_append_column (
    GTK_TREE_VIEW (tree_view), column);
  gtk_tree_view_column_set_expand (
    GTK_TREE_VIEW_COLUMN (column), true);

#if 0
  /* column for min */
  renderer = gtk_cell_renderer_text_new ();
  column =
    gtk_tree_view_column_new_with_attributes (
      _("Min"), renderer,
      "text", COL_PATH,
      NULL);
  gtk_tree_view_append_column (
    GTK_TREE_VIEW (tree_view), column);

  /* column for max */
  renderer = gtk_cell_renderer_text_new ();
  column =
    gtk_tree_view_column_new_with_attributes (
      _("Max"), renderer,
      "text", COL_PATH,
      NULL);
  gtk_tree_view_append_column (
    GTK_TREE_VIEW (tree_view), column);
#endif

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
cc_bindings_tree_widget_refresh (
  CcBindingsTreeWidget * self)
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

CcBindingsTreeWidget *
cc_bindings_tree_widget_new ()
{
  CcBindingsTreeWidget * self =
    g_object_new (
      CC_BINDINGS_TREE_WIDGET_TYPE, NULL);

  /* setup tree */
  tree_view_setup (self, self->tree);

  return self;
}

static void
cc_bindings_tree_widget_class_init (
  CcBindingsTreeWidgetClass * _klass)
{
}

static void
cc_bindings_tree_widget_init (
  CcBindingsTreeWidget * self)
{
  self->tree =
    GTK_TREE_VIEW (gtk_tree_view_new ());
  gtk_widget_set_visible (
    GTK_WIDGET (self->tree), 1);
  gtk_container_add (
    GTK_CONTAINER (self),
    GTK_WIDGET (self->tree));
}
