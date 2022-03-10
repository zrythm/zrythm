/*
 * Copyright (C) 2019, 2021-2022 Alexandros Theodotou <alex at zrythm dot org>
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

#include "gui/widgets/view_toolbar.h"
#include "utils/gtk.h"
#include "utils/resources.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  ViewToolbarWidget, view_toolbar_widget,
  GTK_TYPE_BOX)

static void
view_toolbar_widget_init (ViewToolbarWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

#define SET_TOOLTIP(x, tooltip) \
  z_gtk_set_tooltip_for_actionable ( \
    GTK_ACTIONABLE (self->x), \
    tooltip)
  SET_TOOLTIP (status_bar, _("Toggle Status Bar"));
  SET_TOOLTIP (fullscreen, _("Fullscreen"));
  SET_TOOLTIP (left_panel, _("Toggle Left Panel"));
  SET_TOOLTIP (bot_panel, _("Toggle Bottom Panel"));
  SET_TOOLTIP (top_panel, _("Toggle Top Panel"));
  SET_TOOLTIP (
    right_panel, _("Toggle Right Panel"));
#undef SET_TOOLTIP

#if 0
  GtkWidget * img;
#define SET_TOGGLE_PX_SIZE(x) \
  img = \
    gtk_bin_get_child ( \
      GTK_BIN (self->x)); \
  img = \
    gtk_bin_get_child ( \
      GTK_BIN (img)); \
  img = \
    z_gtk_container_get_single_child ( \
      GTK_CONTAINER (img)); \
  gtk_image_set_pixel_size ( \
    GTK_IMAGE (img), 16)

  SET_TOGGLE_PX_SIZE (left_panel);
  SET_TOGGLE_PX_SIZE (right_panel);
  SET_TOGGLE_PX_SIZE (bot_panel);
  SET_TOGGLE_PX_SIZE (top_panel);

#undef SET_TOGGLE_PX_SIZE
#endif
}

static void
view_toolbar_widget_class_init (ViewToolbarWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "view_toolbar.ui");

  gtk_widget_class_set_css_name (
    klass, "view-toolbar");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, \
    ViewToolbarWidget, \
    x)

  BIND_CHILD (status_bar);
  BIND_CHILD (fullscreen);
  BIND_CHILD (left_panel);
  BIND_CHILD (bot_panel);
  BIND_CHILD (top_panel);
  BIND_CHILD (right_panel);

#undef BIND_CHILD
}
