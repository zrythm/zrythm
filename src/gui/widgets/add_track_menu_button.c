// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-FileCopyrightText: © 2022 Robert Panovics <robert dot panovics at gmail dot com>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/widgets/add_track_menu_button.h"
#include "gui/widgets/tracklist.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

G_DEFINE_TYPE (
  AddTrackMenuButtonWidget,
  add_track_menu_button_widget,
  GTK_TYPE_WIDGET)

static void
set_menu_model (GtkMenuButton * menu_btn)
{
  GMenuModel * menu_model = G_MENU_MODEL (
    tracklist_widget_generate_add_track_menu ());
  gtk_menu_button_set_menu_model (menu_btn, menu_model);
  gtk_popover_set_position (
    gtk_menu_button_get_popover (menu_btn), GTK_POS_RIGHT);
}

AddTrackMenuButtonWidget *
add_track_menu_button_widget_new (void)
{
  return g_object_new (
    ADD_TRACK_MENU_BUTTON_WIDGET_TYPE, NULL);
}

static void
dispose (AddTrackMenuButtonWidget * self)
{
  gtk_widget_unparent (GTK_WIDGET (self->menu_btn));

  G_OBJECT_CLASS (add_track_menu_button_widget_parent_class)
    ->dispose (G_OBJECT (self));
}

static void
finalize (AddTrackMenuButtonWidget * self)
{
  G_OBJECT_CLASS (add_track_menu_button_widget_parent_class)
    ->finalize (G_OBJECT (self));
}

static void
add_track_menu_button_widget_init (
  AddTrackMenuButtonWidget * self)
{
  gtk_widget_set_focusable (GTK_WIDGET (self), false);

  self->menu_btn = GTK_MENU_BUTTON (gtk_menu_button_new ());
  set_menu_model (self->menu_btn);
  gtk_widget_add_css_class (
    GTK_WIDGET (self), "add-track-menu-button");
  gtk_widget_set_tooltip_text (
    GTK_WIDGET (self->menu_btn), _ ("Add track"));
  gtk_menu_button_set_icon_name (self->menu_btn, "add");
  gtk_widget_set_parent (
    GTK_WIDGET (self->menu_btn), GTK_WIDGET (self));
}

static void
add_track_menu_button_widget_class_init (
  AddTrackMenuButtonWidgetClass * _klass)
{
  GObjectClass *   klass = G_OBJECT_CLASS (_klass);
  GtkWidgetClass * wklass = GTK_WIDGET_CLASS (_klass);

  gtk_widget_class_set_layout_manager_type (
    wklass, GTK_TYPE_BIN_LAYOUT);

  klass->dispose = (GObjectFinalizeFunc) dispose;
  klass->finalize = (GObjectFinalizeFunc) finalize;
}
