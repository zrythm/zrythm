// SPDX-FileCopyrightText: © 2021-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/cpp/gtk_widgets/popovers/popover_menu_bin.h"

#include "common/utils/gtk.h"
#include "common/utils/logger.h"

G_DEFINE_TYPE (PopoverMenuBinWidget, popover_menu_bin_widget, GTK_TYPE_WIDGET)

static void
on_right_click (
  GtkGestureClick * gesture,
  gint              n_press,
  gdouble           x,
  gdouble           y,
  gpointer          user_data)
{
  PopoverMenuBinWidget * self = Z_POPOVER_MENU_BIN_WIDGET (user_data);
  z_debug ("right click");
  if (!self->menu_model)
    {
      z_debug ("no menu model");
      return;
    }

  /* see gtklistitemwidget.c in gtk */
  gtk_widget_activate_action (
    GTK_WIDGET (self), "listitem.select", "(bb)", true, true);

  GdkRectangle tmp_rect = Z_GDK_RECTANGLE_INIT_UNIT ((int) x, (int) y);
  gtk_popover_set_pointing_to (GTK_POPOVER (self->popover_menu), &tmp_rect);
  z_debug ("popping up");
  gtk_popover_popup (GTK_POPOVER (self->popover_menu));
}

GMenuModel *
popover_menu_bin_widget_get_menu_model (PopoverMenuBinWidget * self)
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

  self->menu_model = model;

  gtk_popover_menu_set_menu_model (self->popover_menu, model);
}

void
popover_menu_bin_widget_set_child (PopoverMenuBinWidget * self, GtkWidget * child)
{
  self->child = child;
  if (child)
    gtk_widget_set_parent (child, GTK_WIDGET (self));
}

GtkWidget *
popover_menu_bin_widget_get_child (PopoverMenuBinWidget * self)
{
  return self->child;
}

PopoverMenuBinWidget *
popover_menu_bin_widget_new (void)
{
  return Z_POPOVER_MENU_BIN_WIDGET (
    g_object_new (POPOVER_MENU_BIN_WIDGET_TYPE, nullptr));
}

static void
dispose (PopoverMenuBinWidget * self)
{
  gtk_widget_unparent (GTK_WIDGET (self->popover_menu));
  if (self->child)
    gtk_widget_unparent (GTK_WIDGET (self->child));

  G_OBJECT_CLASS (popover_menu_bin_widget_parent_class)
    ->dispose (G_OBJECT (self));
}

static void
popover_menu_bin_widget_class_init (PopoverMenuBinWidgetClass * _klass)
{
  GObjectClass * oklass = G_OBJECT_CLASS (_klass);
  oklass->dispose = (GObjectFinalizeFunc) dispose;
}

static void
popover_menu_bin_widget_init (PopoverMenuBinWidget * self)
{
  self->popover_menu =
    GTK_POPOVER_MENU (gtk_popover_menu_new_from_model (nullptr));
  gtk_widget_set_parent (GTK_WIDGET (self->popover_menu), GTK_WIDGET (self));

  GtkLayoutManager * bin_layout = gtk_bin_layout_new ();
  gtk_widget_set_layout_manager (GTK_WIDGET (self), bin_layout);

  GtkGestureClick * right_click = GTK_GESTURE_CLICK (gtk_gesture_click_new ());
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (right_click), GDK_BUTTON_SECONDARY);
  g_signal_connect (
    G_OBJECT (right_click), "released", G_CALLBACK (on_right_click), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (right_click));
}
