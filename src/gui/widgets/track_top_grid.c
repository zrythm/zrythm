/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "gui/widgets/track.h"
#include "gui/widgets/track_top_grid.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/resources.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

G_DEFINE_TYPE (
  TrackTopGridWidget,
  track_top_grid_widget,
  GTK_TYPE_GRID)

static void
track_top_grid_widget_init (TrackTopGridWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

static void
track_top_grid_widget_class_init (
  TrackTopGridWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "track_top_grid.ui");

  gtk_widget_class_set_css_name (
    klass, "track-top-grid");

  gtk_widget_class_bind_template_child (
    klass,
    TrackTopGridWidget,
    name);
  gtk_widget_class_bind_template_child (
    klass,
    TrackTopGridWidget,
    upper_controls);
  gtk_widget_class_bind_template_child (
    klass,
    TrackTopGridWidget,
    right_activity_box);
  gtk_widget_class_bind_template_child (
    klass,
    TrackTopGridWidget,
    mid_controls);
  gtk_widget_class_bind_template_child (
    klass,
    TrackTopGridWidget,
    bot_controls);
  gtk_widget_class_bind_template_child (
    klass,
    TrackTopGridWidget,
    icon);
}
