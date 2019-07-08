/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

G_DEFINE_TYPE (ViewToolbarWidget,
               view_toolbar_widget,
               GTK_TYPE_TOOLBAR)

static void
view_toolbar_widget_init (ViewToolbarWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

#define SET_TOOLTIP(x, tooltip) \
  z_gtk_set_tooltip_for_actionable ( \
    GTK_ACTIONABLE (self->x), \
    _(tooltip))
  SET_TOOLTIP (left_panel, "Toggle Left Panel");
  SET_TOOLTIP (bot_panel, "Toggle Bottom Panel");
  SET_TOOLTIP (right_panel, "Toggle Right Panel");
  SET_TOOLTIP (status_bar, "Toggle Status Bar");
  SET_TOOLTIP (zoom_in, "Zoom In");
  SET_TOOLTIP (zoom_out, "Zoom Out");
  SET_TOOLTIP (original_size, "Original Size");
  SET_TOOLTIP (best_fit, "Best Fit");
  SET_TOOLTIP (fullscreen, "Fullscreen");
#undef SET_TOOLTIP
}

static void
view_toolbar_widget_class_init (ViewToolbarWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "view_toolbar.ui");

  gtk_widget_class_set_css_name (
    klass, "view-toolbar");

  gtk_widget_class_bind_template_child (
    klass,
    ViewToolbarWidget,
    left_panel);
  gtk_widget_class_bind_template_child (
    klass,
    ViewToolbarWidget,
    bot_panel);
  gtk_widget_class_bind_template_child (
    klass,
    ViewToolbarWidget,
    right_panel);
  gtk_widget_class_bind_template_child (
    klass,
    ViewToolbarWidget,
    status_bar);
  gtk_widget_class_bind_template_child (
    klass,
    ViewToolbarWidget,
    zoom_in);
  gtk_widget_class_bind_template_child (
    klass,
    ViewToolbarWidget,
    zoom_out);
  gtk_widget_class_bind_template_child (
    klass,
    ViewToolbarWidget,
    original_size);
  gtk_widget_class_bind_template_child (
    klass,
    ViewToolbarWidget,
    best_fit);
  gtk_widget_class_bind_template_child (
    klass,
    ViewToolbarWidget,
    fullscreen);
}
