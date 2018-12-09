/*
 * utils/resources.h - resource utils
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

#include "utils/resources.h"

GtkWidget *
resources_get_icon (const char * filename) ///< the path after .../icons/
{
  char * path = g_strdup_printf ("%s%s%s",
                   RESOURCE_PATH,
                   ICON_PATH,
                   filename);
  GtkWidget * icon = gtk_image_new_from_resource (path);
  g_free (path);
  return icon;
}

/**
 * Sets class template from resource.
 *
 * Filename is part after .../ui/
 */
void
resources_set_class_template (GtkWidgetClass * klass,
                              const char * filename)
{
  char * path = g_strdup_printf ("%s%s%s",
                   RESOURCE_PATH,
                   TEMPLATE_PATH,
                   filename);
  gtk_widget_class_set_template_from_resource (
    klass,
    path);
  g_free (path);
}
