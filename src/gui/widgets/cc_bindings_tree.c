/*
 * Copyright (C) 2018-2022 Alexandros Theodotou <alex at zrythm dot org>
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

#include "actions/midi_mapping_action.h"
#include "gui/backend/wrapped_object_with_change_signal.h"
#include "gui/widgets/cc_bindings_tree.h"
#include "gui/widgets/item_factory.h"
#include "project.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/resources.h"
#include "utils/string.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

G_DEFINE_TYPE (
  CcBindingsTreeWidget,
  cc_bindings_tree_widget,
  GTK_TYPE_BOX)

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

#if 0
static void
show_context_menu (
  CcBindingsTreeWidget * self,
  int                    binding_idx)
{
  GMenu * menu = g_menu_new ();
  char tmp[700];
  sprintf (
    tmp, "app.delete-cc-binding::%d", binding_idx);
  g_menu_append (menu, _("Delete"), tmp);

  GtkPopoverMenu * popover_menu =
    GTK_POPOVER_MENU (
      gtk_popover_menu_new_from_model (
        G_MENU_MODEL (menu)));
  gtk_popover_set_has_arrow (
    GTK_POPOVER (popover_menu), false);
  gtk_widget_set_parent (
    GTK_WIDGET (popover_menu), GTK_WIDGET (self));
  gtk_popover_present (
    GTK_POPOVER (popover_menu));
}
#endif

static void
on_right_click (
  GtkGestureClick *      gesture,
  gint                   n_press,
  gdouble                x_dbl,
  gdouble                y_dbl,
  CcBindingsTreeWidget * self)
{
  if (n_press != 1)
    return;

  g_message ("right click");

#if 0
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
#endif
}

/**
 * Refreshes the tree model.
 */
void
cc_bindings_tree_widget_refresh (
  CcBindingsTreeWidget * self)
{
  GListStore * store =
    z_gtk_column_view_get_list_store (
      self->column_view);

  g_list_store_remove_all (store);

  for (int i = 0; i < MIDI_MAPPINGS->num_mappings;
       i++)
    {
      MidiMapping * mm = MIDI_MAPPINGS->mappings[i];
      g_return_if_fail (G_IS_OBJECT (mm->gobj));
      g_list_store_append (store, mm->gobj);
    }
}

static void
generate_column_view (CcBindingsTreeWidget * self)
{
  self->item_factories =
    g_ptr_array_new_with_free_func (
      item_factory_free_func);

  GListStore * store = g_list_store_new (
    WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE);
  GtkMultiSelection * sel = GTK_MULTI_SELECTION (
    gtk_multi_selection_new (G_LIST_MODEL (store)));
  self->column_view = GTK_COLUMN_VIEW (
    gtk_column_view_new (GTK_SELECTION_MODEL (sel)));
  gtk_scrolled_window_set_child (
    self->scroll, GTK_WIDGET (self->column_view));

  /* column for checkbox */
  item_factory_generate_and_append_column (
    self->column_view, self->item_factories,
    ITEM_FACTORY_TOGGLE, Z_F_EDITABLE,
    Z_F_RESIZABLE, NULL, _ ("On"));

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
  item_factory_generate_and_append_column (
    self->column_view, self->item_factories,
    ITEM_FACTORY_TEXT, Z_F_NOT_EDITABLE,
    Z_F_RESIZABLE, NULL, _ ("Note/Control"));

  /* column for path */
  item_factory_generate_and_append_column (
    self->column_view, self->item_factories,
    ITEM_FACTORY_TEXT, Z_F_NOT_EDITABLE,
    Z_F_RESIZABLE, NULL, _ ("Destination"));

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
  GtkGestureClick * mp =
    GTK_GESTURE_CLICK (gtk_gesture_click_new ());
  gtk_widget_add_controller (
    GTK_WIDGET (self->column_view),
    GTK_EVENT_CONTROLLER (mp));
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (mp), GDK_BUTTON_SECONDARY);
  g_signal_connect (
    G_OBJECT (mp), "pressed",
    G_CALLBACK (on_right_click), self);
}

CcBindingsTreeWidget *
cc_bindings_tree_widget_new ()
{
  CcBindingsTreeWidget * self = g_object_new (
    CC_BINDINGS_TREE_WIDGET_TYPE, NULL);

  return self;
}

static void
cc_bindings_tree_finalize (
  CcBindingsTreeWidget * self)
{
  g_ptr_array_unref (self->item_factories);

  G_OBJECT_CLASS (
    cc_bindings_tree_widget_parent_class)
    ->finalize (G_OBJECT (self));
}

static void
cc_bindings_tree_widget_class_init (
  CcBindingsTreeWidgetClass * klass)
{
  GObjectClass * oklass = G_OBJECT_CLASS (klass);
  oklass->finalize = (GObjectFinalizeFunc)
    cc_bindings_tree_finalize;
}

static void
cc_bindings_tree_widget_init (
  CcBindingsTreeWidget * self)
{
  self->scroll = GTK_SCROLLED_WINDOW (
    gtk_scrolled_window_new ());
  gtk_box_append (
    GTK_BOX (self), GTK_WIDGET (self->scroll));
  gtk_widget_set_hexpand (
    GTK_WIDGET (self->scroll), true);

  generate_column_view (self);
}
