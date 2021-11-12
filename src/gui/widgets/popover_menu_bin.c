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

#include "gui/widgets/popover_menu_bin.h"
#include "utils/gtk.h"

G_DEFINE_TYPE (
  PopoverMenuBinWidget,
  popover_menu_bin_widget,
  GTK_TYPE_WIDGET)

static void
on_right_click (
  GtkGestureClick * gesture,
  gint              n_press,
  gdouble           x,
  gdouble           y,
  gpointer          user_data)
{
  PopoverMenuBinWidget * self =
    Z_POPOVER_MENU_BIN_WIDGET (user_data);
  if (!self->menu_model)
    return;

  gtk_popover_set_pointing_to (
    GTK_POPOVER (self->popover_menu),
    &Z_GDK_RECTANGLE_INIT_UNIT ((int) x, (int) y));
  gtk_popover_popup (
    GTK_POPOVER (self->popover_menu));
}

GMenuModel *
popover_menu_bin_widget_get_menu_model (
  PopoverMenuBinWidget * self)
{
  return self->menu_model;
}

/**
 * Replaces the menu model for the popover menu.
 *
 * This function takes ownership of the model.
 */
void
popover_menu_bin_widget_set_menu_model (
  PopoverMenuBinWidget * self,
  GMenuModel *           model)
{
  if (model == self->menu_model)
    return;

  gtk_popover_menu_set_menu_model (
    self->popover_menu, model);
}

void
popover_menu_bin_widget_set_child (
  PopoverMenuBinWidget * self,
  GtkWidget *            child)
{
  gtk_widget_set_parent (child, GTK_WIDGET (self));
}

GtkWidget *
popover_menu_bin_widget_get_child (
  PopoverMenuBinWidget * self)
{
  return gtk_widget_get_last_child (GTK_WIDGET (self));
}

PopoverMenuBinWidget *
popover_menu_bin_widget_new (void)
{
  return
    g_object_new (
      POPOVER_MENU_BIN_WIDGET_TYPE, NULL);
}

static void
popover_menu_bin_widget_class_init (
  PopoverMenuBinWidgetClass * _klass)
{
}

static void
popover_menu_bin_widget_init (
  PopoverMenuBinWidget * self)
{
  self->popover_menu =
    GTK_POPOVER_MENU (
      gtk_popover_menu_new_from_model (NULL));
  gtk_widget_set_parent (
    GTK_WIDGET (self->popover_menu),
    GTK_WIDGET (self));

  GtkLayoutManager * bin_layout =
    gtk_bin_layout_new ();
  gtk_widget_set_layout_manager (
    GTK_WIDGET (self), bin_layout);

  GtkGestureClick * right_click =
    GTK_GESTURE_CLICK (gtk_gesture_click_new ());
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (right_click),
    GDK_BUTTON_SECONDARY);
  g_signal_connect (
    G_OBJECT (right_click), "released",
    G_CALLBACK (on_right_click), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self),
    GTK_EVENT_CONTROLLER (right_click));
}
