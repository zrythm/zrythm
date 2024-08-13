// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/wrapped_object_with_change_signal.h"
#include "gui/widgets/cc_bindings_tree.h"
#include "gui/widgets/item_factory.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

#include "gtk_wrapper.h"

G_DEFINE_TYPE (CcBindingsTreeWidget, cc_bindings_tree_widget, GTK_TYPE_BOX)

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

  z_info ("right click");

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
        &path, &column, nullptr, nullptr))
    {
      z_info ("no path at position %d %d", x, y);
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
cc_bindings_tree_widget_refresh (CcBindingsTreeWidget * self)
{
  GListStore * store = z_gtk_column_view_get_list_store (self->column_view);

  g_list_store_remove_all (store);

  for (auto &mapping : MIDI_MAPPINGS->mappings_)
    {
      auto gobj = wrapped_object_with_change_signal_new (
        mapping.get (), WrappedObjectType::WRAPPED_OBJECT_TYPE_MIDI_MAPPING);
      g_list_store_append (store, gobj);
      g_object_unref (gobj);
    }
}

static void
generate_column_view (CcBindingsTreeWidget * self)
{
  GListStore * store = g_list_store_new (WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE);
  GtkMultiSelection * sel =
    GTK_MULTI_SELECTION (gtk_multi_selection_new (G_LIST_MODEL (store)));
  self->column_view =
    GTK_COLUMN_VIEW (gtk_column_view_new (GTK_SELECTION_MODEL (sel)));
  gtk_scrolled_window_set_child (self->scroll, GTK_WIDGET (self->column_view));

  /* column for checkbox */
  ItemFactory::generate_and_append_column (
    self->column_view, self->item_factories, ItemFactory::Type::Toggle,
    Z_F_EDITABLE, Z_F_RESIZABLE, nullptr, _ ("On"));

  /* column for control */
  ItemFactory::generate_and_append_column (
    self->column_view, self->item_factories, ItemFactory::Type::Text,
    Z_F_NOT_EDITABLE, Z_F_RESIZABLE, nullptr, _ ("Note/Control"));

  /* column for path */
  ItemFactory::generate_and_append_column (
    self->column_view, self->item_factories, ItemFactory::Type::Text,
    Z_F_NOT_EDITABLE, Z_F_RESIZABLE, nullptr, _ ("Destination"));

  /* connect right click handler */
  GtkGestureClick * mp = GTK_GESTURE_CLICK (gtk_gesture_click_new ());
  gtk_widget_add_controller (
    GTK_WIDGET (self->column_view), GTK_EVENT_CONTROLLER (mp));
  gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (mp), GDK_BUTTON_SECONDARY);
  g_signal_connect (G_OBJECT (mp), "pressed", G_CALLBACK (on_right_click), self);
}

CcBindingsTreeWidget *
cc_bindings_tree_widget_new (void)
{
  CcBindingsTreeWidget * self = static_cast<CcBindingsTreeWidget *> (
    g_object_new (CC_BINDINGS_TREE_WIDGET_TYPE, nullptr));

  return self;
}

static void
cc_bindings_tree_finalize (CcBindingsTreeWidget * self)
{
  self->item_factories.~ItemFactoryPtrVector ();

  G_OBJECT_CLASS (cc_bindings_tree_widget_parent_class)
    ->finalize (G_OBJECT (self));
}

static void
cc_bindings_tree_widget_class_init (CcBindingsTreeWidgetClass * klass)
{
  GObjectClass * oklass = G_OBJECT_CLASS (klass);
  oklass->finalize = (GObjectFinalizeFunc) cc_bindings_tree_finalize;
}

static void
cc_bindings_tree_widget_init (CcBindingsTreeWidget * self)
{
  new (&self->item_factories) ItemFactoryPtrVector ();

  gtk_orientable_set_orientation (
    GTK_ORIENTABLE (self), GTK_ORIENTATION_VERTICAL);

  self->scroll = GTK_SCROLLED_WINDOW (gtk_scrolled_window_new ());
  gtk_box_append (GTK_BOX (self), GTK_WIDGET (self->scroll));
  gtk_widget_set_hexpand (GTK_WIDGET (self->scroll), true);
  gtk_widget_set_vexpand (GTK_WIDGET (self->scroll), true);

  generate_column_view (self);

  self->toolbar = GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2));
  self->delete_btn =
    GTK_BUTTON (gtk_button_new_from_icon_name ("box-icons-trash"));
  gtk_actionable_set_action_name (
    GTK_ACTIONABLE (self->delete_btn), "app.delete-midi-cc-bindings");
  gtk_box_append (self->toolbar, GTK_WIDGET (self->delete_btn));
  gtk_box_append (GTK_BOX (self), GTK_WIDGET (self->toolbar));
}
