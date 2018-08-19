/*
 * gui/widget_manager.c - Manages GUI widgets
 *
 * Copyright (C) 2018 Alexandros Theodotou
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

#include "project.h"
#include "gui/widget_manager.h"

#define GET_WIDGET_FROM_BUILDER(object_name) GTK_WIDGET ( \
        gtk_builder_get_object (builder, object_name))

#define REGISTER_WIDGET(object_name) \
  register_widget_from_builder (builder, \
                                object_name)

void
register_widgets (GtkBuilder * builder)
{
  REGISTER_WIDGET ("gapplicationwindow-main");
  REGISTER_WIDGET ("gbutton-close-window");
  REGISTER_WIDGET ("gbutton-minimize-window");
  REGISTER_WIDGET ("gbutton-maximize-window");
  REGISTER_WIDGET ("gpaned-instruments");
  REGISTER_WIDGET ("gdrawingarea-ruler");
  REGISTER_WIDGET ("gdrawingarea-timeline");
  REGISTER_WIDGET ("gscrolledwindow-ruler");
  REGISTER_WIDGET ("gviewport-ruler");
  REGISTER_WIDGET ("gviewport-timeline");
  REGISTER_WIDGET ("goverlay-timeline");
  REGISTER_WIDGET ("gscrolledwindow-timeline");
  REGISTER_WIDGET ("gscrolledwindow-instruments");
  REGISTER_WIDGET ("gbutton-play");
  REGISTER_WIDGET ("gbutton-stop");
  REGISTER_WIDGET ("gbutton-backward");
  REGISTER_WIDGET ("gbutton-forward");
  REGISTER_WIDGET ("gtogglebutton-record");
  REGISTER_WIDGET ("gtoolbutton-add-instrument");
  REGISTER_WIDGET ("gexpander-types");
  REGISTER_WIDGET ("gexpander-collections");
  REGISTER_WIDGET ("gexpander-categories");
  REGISTER_WIDGET ("ggrid-browser");
  REGISTER_WIDGET ("gpaned-browser");
}

void
init_widget_manager ()
{
  project->widget_manager = malloc (sizeof (Widget_Manager));
  project->widget_manager->widgets =
    g_hash_table_new (g_str_hash,
                      g_str_equal);
}

void
register_widget_from_builder (GtkBuilder * builder,
                              gchar       * key)
{
  GtkWidget * widget = GET_WIDGET_FROM_BUILDER (key);
  gboolean result = g_hash_table_insert (
      project->widget_manager->widgets,
      key,
      widget);
  if (!result)
    {
      g_error ("Failed registering widget for %s",
               key);
    }
}

